param(
    [string]$Operation,
    [string]$Project,
    [string]$EngineRoot,
    [string]$Configuration,
    [string]$RunId,
    [switch]$WhatIf,
    [string]$ArchiveRoot,
    [string]$Scope,
    [switch]$Apply,
    [string]$ConfirmProjectRoot,
    [string]$EntryPoint,
    [string]$ChecksFile,
    [switch]$Help,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$RemainingArguments
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'
$script:BoundParameterNames = @($PSBoundParameters.Keys)

$script:Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$script:StartUtc = [DateTime]::UtcNow
$script:Result = $null
$script:ReportDirectory = $null
$script:CommandPath = $null
$script:LogPath = $null
$script:ResultPath = $null

$script:ExpectedMaps = @(
    '/Game/ProjectR/Maps/L_MainMenu',
    '/Game/ProjectR/Maps/L_RealityHub',
    '/Game/ProjectR/Maps/L_Network_Prototype',
    '/Game/ProjectR/Maps/L_CombatGym',
    '/Game/ProjectR/Maps/L_BossGym'
)

function Throw-PRFailure {
    param(
        [int]$ExitCode,
        [string]$FailureKind,
        [string]$Message
    )

    $exception = New-Object System.Exception($Message)
    $exception.Data['ProjectRExitCode'] = $ExitCode
    $exception.Data['ProjectRFailureKind'] = $FailureKind
    throw $exception
}

function Get-FailureExitCode {
    param([System.Exception]$Exception)

    if ($null -ne $Exception.Data['ProjectRExitCode']) {
        return [int]$Exception.Data['ProjectRExitCode']
    }
    if (($null -ne $Exception.InnerException) -and
        ($null -ne $Exception.InnerException.Data['ProjectRExitCode'])) {
        return [int]$Exception.InnerException.Data['ProjectRExitCode']
    }
    return 70
}

function Get-FailureKind {
    param([System.Exception]$Exception)

    if ($null -ne $Exception.Data['ProjectRFailureKind']) {
        return [string]$Exception.Data['ProjectRFailureKind']
    }
    if (($null -ne $Exception.InnerException) -and
        ($null -ne $Exception.InnerException.Data['ProjectRFailureKind'])) {
        return [string]$Exception.InnerException.Data['ProjectRFailureKind']
    }
    return 'internal'
}

function Write-Utf8Text {
    param([string]$Path, [string]$Text)

    [System.IO.File]::WriteAllText($Path, $Text, $script:Utf8NoBom)
}

function Protect-SensitiveText {
    param([AllowNull()][string]$Text)

    if ($null -eq $Text) {
        return $null
    }

    $assignmentPattern = '(?i)(?<![A-Za-z0-9_])((?:[A-Za-z0-9_-]*)(?:token|key|password|secret))(\s*[:=]\s*)(?:"(?:\\.|[^"\\])*"|''(?:\\.|[^''\\])*''|[^;,\r\n]*)'
    $protected = [regex]::Replace($Text, $assignmentPattern, '$1$2<redacted>')
    $argumentPattern = '(?i)(-(?:[A-Za-z0-9_-]*)(?:token|key|password|secret)\s+)(?:"(?:\\.|[^"\\])*"|''(?:\\.|[^''\\])*''|[^;\r\n]*)'
    $protected = [regex]::Replace($protected, $argumentPattern, '$1<redacted>')
    return $protected
}

function Write-RunLog {
    param([string]$Message)

    $safeMessage = Protect-SensitiveText $Message
    $line = '[{0}] {1}' -f ([DateTime]::UtcNow.ToString('o')), $safeMessage
    if ($script:LogPath) {
        [System.IO.File]::AppendAllText($script:LogPath, $line + [Environment]::NewLine, $script:Utf8NoBom)
    }
    Write-Host $line
}

function ConvertTo-DisplayArgument {
    param([AllowNull()][string]$Value)

    if ($null -eq $Value) {
        return '""'
    }
    if (($Value.Length -gt 0) -and ($Value -notmatch '[\s"]')) {
        return $Value
    }
    return '"{0}"' -f ($Value -replace '"', '\"')
}

function Format-CommandLine {
    param([string]$Executable, [string[]]$Arguments)

    $parts = New-Object System.Collections.Generic.List[string]
    $parts.Add((ConvertTo-DisplayArgument $Executable))
    foreach ($argument in $Arguments) {
        $parts.Add((ConvertTo-DisplayArgument $argument))
    }
    return Protect-SensitiveText ($parts -join ' ')
}

function ConvertTo-CmdArgument {
    param([AllowNull()][string]$Value)

    if ($null -eq $Value) {
        $Value = ''
    }
    # Percent expansion still occurs inside cmd.exe quotes; double it before dispatch.
    $escaped = $Value.Replace('%', '%%').Replace('"', '""')
    return '"{0}"' -f $escaped
}

function Get-NormalizedPath {
    param([string]$Path)

    return [System.IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
}

function Test-PathWithin {
    param([string]$Child, [string]$Parent)

    $normalizedChild = Get-NormalizedPath $Child
    $normalizedParent = Get-NormalizedPath $Parent
    $prefix = $normalizedParent + [System.IO.Path]::DirectorySeparatorChar
    return $normalizedChild.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)
}

function Get-ProjectContext {
    param([string]$ProjectArgument)

    if ([string]::IsNullOrWhiteSpace($ProjectArgument) -or
        -not [System.IO.Path]::IsPathRooted($ProjectArgument)) {
        Throw-PRFailure 64 'arguments' '-Project must be an absolute .uproject path.'
    }

    $projectFile = Get-NormalizedPath $ProjectArgument
    if (([System.IO.Path]::GetExtension($projectFile) -ine '.uproject') -or
        -not (Test-Path -LiteralPath $projectFile -PathType Leaf)) {
        Throw-PRFailure 64 'arguments' "Project file does not exist or is not a .uproject: $projectFile"
    }

    $projectRoot = Split-Path -Parent $projectFile
    $projectFiles = @(Get-ChildItem -LiteralPath $projectRoot -File -Filter '*.uproject')
    if (($projectFiles.Count -ne 1) -or
        -not [string]::Equals($projectFiles[0].FullName, $projectFile, [System.StringComparison]::OrdinalIgnoreCase)) {
        Throw-PRFailure 65 'project-contract' 'Project root must contain exactly the requested .uproject.'
    }

    try {
        $projectJson = Get-Content -Raw -Encoding UTF8 -LiteralPath $projectFile | ConvertFrom-Json
    }
    catch {
        Throw-PRFailure 65 'project-contract' "Unable to parse project descriptor: $($_.Exception.Message)"
    }

    $association = [string]$projectJson.EngineAssociation
    if ([string]::IsNullOrWhiteSpace($association)) {
        Throw-PRFailure 65 'project-contract' 'Project descriptor has no EngineAssociation.'
    }

    return [pscustomobject]@{
        ProjectFile = $projectFile
        ProjectRoot = $projectRoot
        ProjectName = [System.IO.Path]::GetFileNameWithoutExtension($projectFile)
        EngineAssociation = $association
    }
}

function Test-EngineCandidate {
    param(
        [string]$Candidate,
        [string]$Association,
        [string[]]$RequiredRelativeFiles
    )

    try {
        $root = Get-NormalizedPath $Candidate
    }
    catch {
        return $null
    }

    if (-not (Test-Path -LiteralPath $root -PathType Container)) {
        return $null
    }

    $buildVersionPath = Join-Path $root 'Engine\Build\Build.version'
    if (-not (Test-Path -LiteralPath $buildVersionPath -PathType Leaf)) {
        return $null
    }

    try {
        $buildVersion = Get-Content -Raw -Encoding UTF8 -LiteralPath $buildVersionPath | ConvertFrom-Json
    }
    catch {
        return $null
    }

    $actualAssociation = '{0}.{1}' -f $buildVersion.MajorVersion, $buildVersion.MinorVersion
    if ($actualAssociation -ne $Association) {
        return $null
    }

    foreach ($relativeFile in $RequiredRelativeFiles) {
        if (-not (Test-Path -LiteralPath (Join-Path $root $relativeFile) -PathType Leaf)) {
            return $null
        }
    }

    return [pscustomobject]@{
        Root = $root
        Version = ('{0}.{1}.{2}' -f $buildVersion.MajorVersion, $buildVersion.MinorVersion, $buildVersion.PatchVersion)
        Changelist = [long]$buildVersion.Changelist
    }
}

function Resolve-EngineContext {
    param(
        [pscustomobject]$ProjectContext,
        [string]$ExplicitRoot,
        [string[]]$RequiredRelativeFiles
    )

    $rawCandidates = New-Object System.Collections.Generic.List[string]
    $source = 'auto-discovery'

    if (-not [string]::IsNullOrWhiteSpace($ExplicitRoot)) {
        if (-not [System.IO.Path]::IsPathRooted($ExplicitRoot)) {
            Throw-PRFailure 66 'engine-discovery' '-EngineRoot must be an absolute path.'
        }
        $rawCandidates.Add($ExplicitRoot)
        $source = 'argument'
    }
    elseif (-not [string]::IsNullOrWhiteSpace($env:PROJECTR_UE_ROOT)) {
        if (-not [System.IO.Path]::IsPathRooted($env:PROJECTR_UE_ROOT)) {
            Throw-PRFailure 66 'engine-discovery' 'PROJECTR_UE_ROOT must be an absolute path.'
        }
        $rawCandidates.Add($env:PROJECTR_UE_ROOT)
        $source = 'environment'
    }
    else {
        $launcherFiles = @(
            (Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'),
            (Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\Data\LauncherInstalled.dat')
        )
        foreach ($launcherFile in $launcherFiles) {
            if (-not (Test-Path -LiteralPath $launcherFile -PathType Leaf)) {
                continue
            }
            try {
                $launcherData = Get-Content -Raw -Encoding UTF8 -LiteralPath $launcherFile | ConvertFrom-Json
                foreach ($installation in @($launcherData.InstallationList)) {
                    if (($installation.AppName -eq ('UE_' + $ProjectContext.EngineAssociation)) -or
                        ($installation.AppVersion -like ($ProjectContext.EngineAssociation + '*'))) {
                        $rawCandidates.Add([string]$installation.InstallLocation)
                    }
                }
            }
            catch {
                Write-Host "Ignoring unreadable Launcher installation data: $launcherFile"
            }
        }

        foreach ($drive in @(Get-PSDrive -PSProvider FileSystem)) {
            $rawCandidates.Add((Join-Path $drive.Root ("Unreal Engine 5\UE_{0}" -f $ProjectContext.EngineAssociation)))
            $rawCandidates.Add((Join-Path $drive.Root ("Epic Games\UE_{0}" -f $ProjectContext.EngineAssociation)))
            $rawCandidates.Add((Join-Path $drive.Root ("Program Files\Epic Games\UE_{0}" -f $ProjectContext.EngineAssociation)))
        }
    }

    $seen = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    $valid = New-Object System.Collections.Generic.List[object]
    foreach ($candidate in $rawCandidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }
        try {
            $normalized = Get-NormalizedPath $candidate
        }
        catch {
            continue
        }
        if (-not $seen.Add($normalized)) {
            continue
        }
        $engine = Test-EngineCandidate $normalized $ProjectContext.EngineAssociation $RequiredRelativeFiles
        if ($null -ne $engine) {
            $valid.Add($engine)
        }
    }

    if ($valid.Count -eq 0) {
        Throw-PRFailure 66 'engine-discovery' "No valid UE $($ProjectContext.EngineAssociation) installation was found from $source."
    }
    if ($valid.Count -gt 1) {
        $paths = ($valid | ForEach-Object { $_.Root }) -join '; '
        Throw-PRFailure 66 'engine-discovery' "Multiple valid UE $($ProjectContext.EngineAssociation) installations were found: $paths"
    }

    return $valid[0]
}

function New-RunId {
    return '{0}-{1}' -f ([DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ')), ([Guid]::NewGuid().ToString('N').Substring(0, 8))
}

function Assert-StableIdentifier {
    param([string]$Value, [string]$Name, [int]$MaximumLength = 128)

    if ([string]::IsNullOrWhiteSpace($Value) -or
        ($Value.Length -gt $MaximumLength) -or
        ($Value -notmatch '^[A-Za-z0-9._-]+$')) {
        Throw-PRFailure 64 'arguments' "$Name must use only ASCII letters, digits, dot, underscore, or hyphen."
    }
}

function Initialize-Report {
    param(
        [pscustomobject]$ProjectContext,
        [string]$ReportEntryPoint,
        [string]$ReportConfiguration,
        [string]$RequestedRunId
    )

    if ([string]::IsNullOrWhiteSpace($RequestedRunId)) {
        $RequestedRunId = New-RunId
    }
    Assert-StableIdentifier $RequestedRunId 'RunId'
    Assert-StableIdentifier $ReportEntryPoint 'EntryPoint' 64
    Assert-StableIdentifier $ReportConfiguration 'Configuration' 64

    $reportsRoot = Get-NormalizedPath (Join-Path $ProjectContext.ProjectRoot 'Saved\AutomationReports')
    $script:ReportDirectory = Join-Path (Join-Path $reportsRoot $RequestedRunId) ("{0}-{1}" -f $ReportEntryPoint, $ReportConfiguration)
    $script:ReportDirectory = Get-NormalizedPath $script:ReportDirectory
    if (-not (Test-PathWithin $script:ReportDirectory $reportsRoot)) {
        Throw-PRFailure 67 'output-path' "Automation report path escaped its approved root: $script:ReportDirectory"
    }
    Assert-NoReparsePoint $ProjectContext.ProjectRoot $script:ReportDirectory
    Assert-GitIgnoredAndUntracked $ProjectContext.ProjectRoot $script:ReportDirectory
    if (Test-Path -LiteralPath $script:ReportDirectory) {
        Throw-PRFailure 67 'output-path' "Automation report directory already exists: $script:ReportDirectory"
    }

    try {
        $null = New-Item -ItemType Directory -Path $script:ReportDirectory -Force
        $script:CommandPath = Join-Path $script:ReportDirectory 'command.txt'
        $script:LogPath = Join-Path $script:ReportDirectory 'run.log'
        $script:ResultPath = Join-Path $script:ReportDirectory 'result.json'
        Write-Utf8Text $script:CommandPath ''
        Write-Utf8Text $script:LogPath ''
    }
    catch {
        $script:ReportDirectory = $null
        Throw-PRFailure 69 'report-io' "Unable to create automation report: $($_.Exception.Message)"
    }

    $script:Result = [ordered]@{
        schemaVersion = 1
        runId = $RequestedRunId
        entryPoint = $ReportEntryPoint
        status = 'NOT_RUN'
        failureKind = $null
        startUtc = $script:StartUtc.ToString('o')
        endUtc = $null
        durationSeconds = 0.0
        projectFile = $ProjectContext.ProjectFile
        projectRoot = $ProjectContext.ProjectRoot
        engineRoot = $null
        engineVersion = $null
        target = $null
        platform = 'Win64'
        configuration = $ReportConfiguration
        maps = @()
        commandFile = $script:CommandPath
        logFile = $script:LogPath
        outputPaths = @()
        cleanedPaths = @()
        childExitCode = $null
        exitCode = 0
        checks = @()
    }

    Write-RunLog "Starting $ReportEntryPoint ($ReportConfiguration), RunId=$RequestedRunId"
}

function Set-EngineResult {
    param([pscustomobject]$EngineContext)

    $script:Result['engineRoot'] = $EngineContext.Root
    $script:Result['engineVersion'] = '{0} CL {1}' -f $EngineContext.Version, $EngineContext.Changelist
}

function Set-CommandReport {
    param([string]$Executable, [string[]]$Arguments)

    $command = Format-CommandLine $Executable $Arguments
    Write-Utf8Text $script:CommandPath ($command + [Environment]::NewLine)
    Write-RunLog "Command: $command"
}

function Complete-Report {
    param([int]$ExitCode)

    if ($null -eq $script:Result) {
        return $ExitCode
    }

    $endUtc = [DateTime]::UtcNow
    $script:Result['endUtc'] = $endUtc.ToString('o')
    $script:Result['durationSeconds'] = [Math]::Round(($endUtc - $script:StartUtc).TotalSeconds, 3)
    $script:Result['exitCode'] = $ExitCode
    try {
        $json = $script:Result | ConvertTo-Json -Depth 20
        # Windows PowerShell 5.1 escapes angle brackets; retain the frozen marker verbatim.
        $json = $json.Replace('\u003c', '<').Replace('\u003e', '>').Replace('\u003C', '<').Replace('\u003E', '>')
        Write-Utf8Text $script:ResultPath ($json + [Environment]::NewLine)
    }
    catch {
        Write-Host "Unable to write result.json: $($_.Exception.Message)"
        return 69
    }
    return $ExitCode
}

function New-Check {
    param(
        [string]$Name,
        [bool]$Required,
        [string]$Status,
        [string]$Evidence,
        [string[]]$Artifacts = @()
    )

    return [ordered]@{
        name = $Name
        required = $Required
        status = $Status
        evidence = Protect-SensitiveText $Evidence
        artifacts = @($Artifacts | ForEach-Object { Protect-SensitiveText $_ })
    }
}

function Invoke-LoggedNative {
    param([string]$Executable, [string[]]$Arguments)

    $process = $null
    try {
        $commandParts = New-Object System.Collections.Generic.List[string]
        $commandParts.Add('call')
        $commandParts.Add((ConvertTo-CmdArgument $Executable))
        foreach ($argument in $Arguments) {
            $commandParts.Add((ConvertTo-CmdArgument $argument))
        }

        $startInfo = New-Object System.Diagnostics.ProcessStartInfo
        $startInfo.FileName = $env:ComSpec
        $startInfo.Arguments = '/d /s /c ' + ($commandParts -join ' ')
        $startInfo.UseShellExecute = $false
        $startInfo.CreateNoWindow = $true
        $startInfo.RedirectStandardOutput = $true
        $startInfo.RedirectStandardError = $true
        $startInfo.StandardOutputEncoding = [Console]::OutputEncoding
        $startInfo.StandardErrorEncoding = [Console]::OutputEncoding

        $process = New-Object System.Diagnostics.Process
        $process.StartInfo = $startInfo
        if (-not $process.Start()) {
            Throw-PRFailure 70 'child-start' "Unable to start native child process: $Executable"
        }

        # Start both reads before waiting so either native pipe can fill without deadlocking.
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $stdout = [string]$stdoutTask.GetAwaiter().GetResult()
        $stderr = [string]$stderrTask.GetAwaiter().GetResult()

        foreach ($line in @($stdout -split '\r?\n')) {
            if ($line.Length -gt 0) {
                Write-RunLog ("[stdout] {0}" -f $line)
            }
        }
        foreach ($line in @($stderr -split '\r?\n')) {
            if ($line.Length -gt 0) {
                Write-RunLog ("[stderr] {0}" -f $line)
            }
        }
        return [int]$process.ExitCode
    }
    finally {
        if ($null -ne $process) {
            $process.Dispose()
        }
    }
}

function Test-ObjectProperty {
    param([object]$Object, [string]$Name)

    if ($null -eq $Object) {
        return $false
    }
    return @($Object.PSObject.Properties.Name) -contains $Name
}

function Get-ValidatedChecks {
    param([string]$ChecksPath)

    if ([string]::IsNullOrWhiteSpace($ChecksPath) -or
        -not [System.IO.Path]::IsPathRooted($ChecksPath)) {
        Throw-PRFailure 64 'checks-schema' '-ChecksFile must be an absolute JSON path.'
    }

    $normalizedChecksPath = Get-NormalizedPath $ChecksPath
    if (-not (Test-Path -LiteralPath $normalizedChecksPath -PathType Leaf)) {
        Throw-PRFailure 64 'checks-schema' "ChecksFile does not exist: $normalizedChecksPath"
    }
    $checksFileInfo = Get-Item -LiteralPath $normalizedChecksPath
    if ($checksFileInfo.Length -gt 1MB) {
        Throw-PRFailure 64 'checks-schema' 'ChecksFile must not exceed 1 MiB.'
    }

    try {
        $document = Get-Content -Raw -Encoding UTF8 -LiteralPath $normalizedChecksPath | ConvertFrom-Json
    }
    catch {
        Throw-PRFailure 64 'checks-schema' "ChecksFile is not valid JSON: $($_.Exception.Message)"
    }

    if (($null -eq $document) -or
        ($document -is [System.Array]) -or
        -not (Test-ObjectProperty $document 'checks')) {
        Throw-PRFailure 64 'checks-schema' 'ChecksFile must contain a checks array.'
    }

    if ($document.checks -isnot [System.Array]) {
        Throw-PRFailure 64 'checks-schema' 'ChecksFile checks must be a JSON array, not a scalar or object.'
    }

    $inputChecks = @($document.checks)
    if ($inputChecks.Count -eq 0) {
        Throw-PRFailure 64 'checks-schema' 'ChecksFile must contain at least one check.'
    }

    $normalizedChecks = New-Object System.Collections.Generic.List[object]
    foreach ($check in $inputChecks) {
        if (($null -eq $check) -or
            ($check -is [System.Array]) -or
            ($check -isnot [System.Management.Automation.PSCustomObject])) {
            Throw-PRFailure 64 'checks-schema' 'Every checks array element must be a non-null JSON object.'
        }
        foreach ($property in @('name', 'required', 'status', 'evidence', 'artifacts')) {
            if (-not (Test-ObjectProperty $check $property)) {
                Throw-PRFailure 64 'checks-schema' "Check is missing required property: $property"
            }
        }

        $name = [string]$check.name
        if ([string]::IsNullOrWhiteSpace($name) -or
            ($name.Length -gt 128) -or
            ($name -notmatch '^[A-Za-z0-9._-]+$')) {
            Throw-PRFailure 64 'checks-schema' 'Check names must be stable ASCII identifiers.'
        }
        if ($check.required -isnot [bool]) {
            Throw-PRFailure 64 'checks-schema' "Check '$name' required must be a JSON boolean."
        }

        $status = [string]$check.status
        if (@('PASS', 'FAIL', 'NOT_RUN') -cnotcontains $status) {
            Throw-PRFailure 64 'checks-schema' "Check '$name' has an unsupported status."
        }
        if ($check.evidence -isnot [string]) {
            Throw-PRFailure 64 'checks-schema' "Check '$name' evidence must be a string."
        }

        if ($check.artifacts -isnot [System.Array]) {
            Throw-PRFailure 64 'checks-schema' "Check '$name' artifacts must be a JSON array."
        }
        $artifacts = @($check.artifacts)
        foreach ($artifact in $artifacts) {
            if ($artifact -isnot [string]) {
                Throw-PRFailure 64 'checks-schema' "Check '$name' artifacts must contain only strings."
            }
        }

        $normalizedChecks.Add((New-Check $name ([bool]$check.required) $status ([string]$check.evidence) $artifacts))
    }

    if (@($normalizedChecks | Where-Object { $_.required }).Count -eq 0) {
        Throw-PRFailure 64 'checks-schema' 'ChecksFile must contain at least one required check.'
    }

    return $normalizedChecks.ToArray()
}

function Invoke-AutomationReportOperation {
    param([pscustomobject]$ProjectContext)

    if ([string]::IsNullOrWhiteSpace($EntryPoint)) {
        Throw-PRFailure 64 'arguments' '-EntryPoint is required for AutomationReport.'
    }
    Assert-StableIdentifier $EntryPoint 'EntryPoint' 64
    $validatedChecks = Get-ValidatedChecks $ChecksFile

    Initialize-Report $ProjectContext $EntryPoint 'None' $RunId
    $engine = Resolve-EngineContext $ProjectContext $EngineRoot @()
    Set-EngineResult $engine

    $publicEntry = Join-Path $PSScriptRoot 'AutomationReport.bat'
    $commandArguments = @(
        '-Project', $ProjectContext.ProjectFile,
        '-EngineRoot', $engine.Root,
        '-EntryPoint', $EntryPoint,
        '-ChecksFile', (Get-NormalizedPath $ChecksFile),
        '-RunId', $script:Result['runId']
    )
    if ($WhatIf) {
        $commandArguments += '-WhatIf'
    }
    Set-CommandReport $publicEntry $commandArguments

    $script:Result['checks'] = @($validatedChecks)
    if ($WhatIf) {
        $script:Result['status'] = 'NOT_RUN'
        Write-RunLog 'WhatIf: checks schema validated; result derivation was not asserted as an executed automation result.'
        return 0
    }

    $failedChecks = @($validatedChecks | Where-Object { $_.status -eq 'FAIL' })
    $requiredChecks = @($validatedChecks | Where-Object { $_.required })
    $requiredPass = @($requiredChecks | Where-Object { $_.status -eq 'PASS' })
    $requiredNotRun = @($requiredChecks | Where-Object { $_.status -eq 'NOT_RUN' })

    if ($failedChecks.Count -gt 0) {
        $script:Result['status'] = 'FAIL'
        $script:Result['failureKind'] = 'checks'
        Write-RunLog 'Automation report derived FAIL because at least one check failed.'
        return 1
    }
    if ($requiredPass.Count -eq $requiredChecks.Count) {
        $script:Result['status'] = 'PASS'
        Write-RunLog 'Automation report derived PASS; every required check passed.'
        return 0
    }
    if ($requiredNotRun.Count -eq $requiredChecks.Count) {
        $script:Result['status'] = 'NOT_RUN'
        Write-RunLog 'Automation report derived NOT_RUN; every required check was not run.'
        return 0
    }

    $script:Result['status'] = 'FAIL'
    $script:Result['failureKind'] = 'checks'
    Write-RunLog 'Automation report derived FAIL because required PASS and NOT_RUN states were mixed.'
    return 1
}

function Test-CommandTargetsProject {
    param([AllowNull()][string]$CommandLine, [pscustomobject]$ProjectContext)

    if ([string]::IsNullOrWhiteSpace($CommandLine)) {
        return $false
    }

    $nativeType = 'ProjectRAutomationCommandLineNativeMethods' -as [type]
    if ($null -eq $nativeType) {
        $nativeSource = @'
using System;
using System.Runtime.InteropServices;

public static class ProjectRAutomationCommandLineNativeMethods
{
    [DllImport("shell32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr CommandLineToArgvW(string commandLine, out int argumentCount);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr LocalFree(IntPtr memory);
}
'@
        Add-Type -TypeDefinition $nativeSource -Language CSharp
        $nativeType = 'ProjectRAutomationCommandLineNativeMethods' -as [type]
    }

    $argumentCount = 0
    $argumentVector = $nativeType::CommandLineToArgvW($CommandLine, [ref]$argumentCount)
    if ($argumentVector -eq [IntPtr]::Zero) {
        throw 'CommandLineToArgvW failed while validating process ownership.'
    }

    $arguments = New-Object System.Collections.Generic.List[string]
    try {
        for ($index = 0; $index -lt $argumentCount; $index++) {
            $pointer = [System.Runtime.InteropServices.Marshal]::ReadIntPtr(
                $argumentVector,
                $index * [IntPtr]::Size
            )
            $arguments.Add([System.Runtime.InteropServices.Marshal]::PtrToStringUni($pointer))
        }
    }
    finally {
        $null = $nativeType::LocalFree($argumentVector)
    }

    $projectFile = [System.IO.Path]::GetFullPath([string]$ProjectContext.ProjectFile).TrimEnd('\', '/')
    $projectRoot = [System.IO.Path]::GetFullPath([string]$ProjectContext.ProjectRoot).TrimEnd('\', '/')
    $projectPrefix = $projectRoot + [System.IO.Path]::DirectorySeparatorChar

    foreach ($argument in $arguments) {
        $candidate = $argument
        if ($candidate -match '^[/-][^=]+=(.*)$') {
            $candidate = $Matches[1]
        }
        $candidate = $candidate.Trim().Trim('"')
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }
        try {
            if (-not [System.IO.Path]::IsPathRooted($candidate)) {
                continue
            }
            $candidatePath = [System.IO.Path]::GetFullPath($candidate).TrimEnd('\', '/')
        }
        catch {
            continue
        }
        if ([string]::Equals($candidatePath, $projectFile, [System.StringComparison]::OrdinalIgnoreCase) -or
            [string]::Equals($candidatePath, $projectRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
            $candidatePath.StartsWith($projectPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }
    return $false
}

function Get-BlockingProjectProcesses {
    param([pscustomobject]$ProjectContext)

    try {
        $processes = @(Get-CimInstance Win32_Process)
    }
    catch {
        Throw-PRFailure 65 'process-preflight' "Unable to inspect active processes: $($_.Exception.Message)"
    }

    $processById = @{}
    $directProjectOwners = New-Object 'System.Collections.Generic.HashSet[int]'
    try {
        foreach ($process in $processes) {
            $processId = [int]$process.ProcessId
            $processById[$processId] = $process
            if (Test-CommandTargetsProject ([string]$process.CommandLine) $ProjectContext) {
                $null = $directProjectOwners.Add($processId)
            }
        }
    }
    catch {
        Throw-PRFailure 65 'process-preflight' "Unable to determine process ownership: $($_.Exception.Message)"
    }

    $blocking = New-Object System.Collections.Generic.List[string]
    foreach ($process in $processes) {
        $name = [string]$process.Name
        $commandLine = [string]$process.CommandLine
        $isEditor = $name -ieq 'UnrealEditor.exe'
        $isLiveCoding = ($name -match '(?i)^LiveCoding') -or ($commandLine -match '(?i)LiveCodingConsole')
        $isBuildTool = ($name -match '(?i)(UnrealBuildTool|AutomationTool|RunUAT)') -or
            ($commandLine -match '(?i)(UnrealBuildTool|AutomationTool|RunUAT\.bat)')
        if (-not ($isEditor -or $isLiveCoding -or $isBuildTool)) {
            continue
        }

        $processId = [int]$process.ProcessId
        $targetsProject = $directProjectOwners.Contains($processId)
        if (-not $targetsProject -and ($isLiveCoding -or $isBuildTool)) {
            $visited = New-Object 'System.Collections.Generic.HashSet[int]'
            $parentId = [int]$process.ParentProcessId
            while (($parentId -gt 0) -and $visited.Add($parentId) -and $processById.ContainsKey($parentId)) {
                if ($directProjectOwners.Contains($parentId)) {
                    $targetsProject = $true
                    break
                }
                $parentId = [int]$processById[$parentId].ParentProcessId
            }
        }

        if ($targetsProject) {
            $blocking.Add(('{0}:{1}' -f $process.ProcessId, $name))
        }
    }
    return $blocking.ToArray()
}

function Assert-NoBlockingProcesses {
    param([pscustomobject]$ProjectContext)

    $blocking = @(Get-BlockingProjectProcesses $ProjectContext)
    if ($blocking.Count -gt 0) {
        Throw-PRFailure 68 'active-process' ('Active ProjectR Editor/build processes block this operation: ' + ($blocking -join ', '))
    }
}

function Invoke-BuildEditorOperation {
    param([pscustomobject]$ProjectContext)

    $selectedConfiguration = $Configuration
    if ([string]::IsNullOrWhiteSpace($selectedConfiguration)) {
        $selectedConfiguration = 'Development'
    }
    if ($selectedConfiguration -ieq 'Development') {
        $selectedConfiguration = 'Development'
    }
    elseif ($selectedConfiguration -ieq 'DebugGame') {
        $selectedConfiguration = 'DebugGame'
    }
    else {
        Throw-PRFailure 64 'arguments' 'BuildEditor -Configuration must be Development or DebugGame.'
    }

    Initialize-Report $ProjectContext 'BuildEditor' $selectedConfiguration $RunId
    $engine = Resolve-EngineContext $ProjectContext $EngineRoot @(
        'Engine\Build\BatchFiles\Build.bat',
        'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
    )
    Set-EngineResult $engine

    $script:Result['target'] = 'ProjectREditor'
    $moduleFileName = if ($selectedConfiguration -eq 'DebugGame') {
        'UnrealEditor-ProjectR-Win64-DebugGame.dll'
    }
    else {
        'UnrealEditor-ProjectR.dll'
    }
    $targetDll = Join-Path $ProjectContext.ProjectRoot (Join-Path 'Binaries\Win64' $moduleFileName)
    $script:Result['outputPaths'] = @($targetDll)

    $buildScript = Join-Path $engine.Root 'Engine\Build\BatchFiles\Build.bat'
    $buildArguments = @(
        'ProjectREditor',
        'Win64',
        $selectedConfiguration,
        ('-Project={0}' -f $ProjectContext.ProjectFile),
        '-WaitMutex',
        '-FromMsBuild',
        '-architecture=x64',
        '-NoHotReloadFromIDE'
    )
    Set-CommandReport $buildScript $buildArguments

    $checks = New-Object System.Collections.Generic.List[object]
    $checks.Add((New-Check 'CommandPrepared' $true 'PASS' 'The exact ProjectREditor command was written before execution.' @($script:CommandPath)))

    if ($WhatIf) {
        $checks.Add((New-Check 'BuildExecuted' $true 'NOT_RUN' 'WhatIf requested; Build.bat was not invoked.'))
        $checks.Add((New-Check 'EditorDllVerified' $true 'NOT_RUN' 'No build output is asserted during WhatIf.' @($targetDll)))
        $script:Result['checks'] = $checks.ToArray()
        $script:Result['status'] = 'NOT_RUN'
        Write-RunLog 'WhatIf: BuildEditor command prepared; no child process was started.'
        return 0
    }

    Assert-NoBlockingProcesses $ProjectContext
    $childExitCode = Invoke-LoggedNative $buildScript $buildArguments
    $script:Result['childExitCode'] = $childExitCode
    if ($childExitCode -ne 0) {
        $checks.Add((New-Check 'BuildExecuted' $true 'FAIL' "Build.bat returned $childExitCode." @($script:LogPath)))
        $checks.Add((New-Check 'EditorDllVerified' $true 'NOT_RUN' 'Build failed; output was not accepted.' @($targetDll)))
        $script:Result['checks'] = $checks.ToArray()
        $script:Result['status'] = 'FAIL'
        $script:Result['failureKind'] = 'child'
        Write-RunLog "BuildEditor failed with child exit code $childExitCode."
        return $childExitCode
    }

    $checks.Add((New-Check 'BuildExecuted' $true 'PASS' 'Build.bat returned 0.' @($script:LogPath)))
    if (-not (Test-Path -LiteralPath $targetDll -PathType Leaf)) {
        $checks.Add((New-Check 'EditorDllVerified' $true 'FAIL' 'Build returned 0 but the ProjectR Editor module DLL is missing.' @($targetDll)))
        $script:Result['checks'] = $checks.ToArray()
        Throw-PRFailure 70 'postcondition' "Build output is missing: $targetDll"
    }

    $logContent = Get-Content -Raw -Encoding UTF8 -LiteralPath $script:LogPath
    if ($logContent -match '(?i)(Unable to build while Live Coding is active|cannot be built while Live Coding|Live Coding.*must be disabled)') {
        $checks.Add((New-Check 'EditorDllVerified' $true 'FAIL' 'Build log contains a Live Coding blocker.' @($targetDll, $script:LogPath)))
        $script:Result['checks'] = $checks.ToArray()
        Throw-PRFailure 70 'postcondition' 'Build log contains a Live Coding blocker despite a zero exit code.'
    }

    $checks.Add((New-Check 'EditorDllVerified' $true 'PASS' 'ProjectR Editor module DLL exists and no Live Coding blocker was logged.' @($targetDll)))
    $script:Result['checks'] = $checks.ToArray()
    $script:Result['status'] = 'PASS'
    Write-RunLog 'BuildEditor completed successfully.'
    return 0
}

function Get-PackagingMaps {
    param([pscustomobject]$ProjectContext)

    $defaultGamePath = Join-Path $ProjectContext.ProjectRoot 'Config\DefaultGame.ini'
    if (-not (Test-Path -LiteralPath $defaultGamePath -PathType Leaf)) {
        Throw-PRFailure 65 'packaging-config' "DefaultGame.ini is missing: $defaultGamePath"
    }

    $sectionName = '/Script/UnrealEd.ProjectPackagingSettings'
    $currentSection = $null
    $sectionCount = 0
    $mapList = New-Object System.Collections.Generic.List[string]
    foreach ($line in @(Get-Content -Encoding UTF8 -LiteralPath $defaultGamePath)) {
        if ($line -match '^\s*\[([^\]]+)\]\s*$') {
            $currentSection = $Matches[1]
            if ($currentSection -ceq $sectionName) {
                $sectionCount++
            }
            continue
        }
        if ($line -notmatch '(?i)^\s*[+\-!.]?MapsToCook\s*=') {
            continue
        }
        if ($currentSection -cne $sectionName) {
            Throw-PRFailure 65 'packaging-config' 'MapsToCook entries are only allowed inside the unique ProjectPackagingSettings section.'
        }
        if ($line -notmatch '^\s*\+MapsToCook=\(FilePath="([^"]+)"\)\s*$') {
            Throw-PRFailure 65 'packaging-config' "Malformed MapsToCook entry: $line"
        }
        $mapList.Add($Matches[1])
    }

    if ($sectionCount -ne 1) {
        Throw-PRFailure 65 'packaging-config' 'DefaultGame.ini must contain exactly one ProjectPackagingSettings section.'
    }
    $maps = $mapList.ToArray()
    if ($maps.Count -ne $script:ExpectedMaps.Count) {
        Throw-PRFailure 65 'packaging-config' "MapsToCook must contain exactly $($script:ExpectedMaps.Count) maps."
    }

    for ($index = 0; $index -lt $script:ExpectedMaps.Count; $index++) {
        if ($maps[$index] -cne $script:ExpectedMaps[$index]) {
            Throw-PRFailure 65 'packaging-config' "MapsToCook mismatch at index $index. Expected '$($script:ExpectedMaps[$index])', got '$($maps[$index])'."
        }
    }
    if (@($maps | Select-Object -Unique).Count -ne $maps.Count) {
        Throw-PRFailure 65 'packaging-config' 'MapsToCook contains duplicate entries.'
    }

    foreach ($map in $maps) {
        $relativePackage = $map.Substring('/Game/'.Length).Replace('/', [System.IO.Path]::DirectorySeparatorChar)
        $mapFile = Join-Path $ProjectContext.ProjectRoot (Join-Path 'Content' ($relativePackage + '.umap'))
        if (-not (Test-Path -LiteralPath $mapFile -PathType Leaf)) {
            Throw-PRFailure 65 'packaging-config' "Configured map package is missing: $map"
        }
    }

    return $maps
}

function Assert-NoReparsePoint {
    param([string]$ProjectRoot, [string]$TargetPath)

    $root = Get-NormalizedPath $ProjectRoot
    $target = Get-NormalizedPath $TargetPath
    if (-not (Test-PathWithin $target $root)) {
        Throw-PRFailure 67 'path-safety' "Path is outside the project root: $target"
    }

    if (Test-Path -LiteralPath $root) {
        $rootItem = Get-Item -Force -LiteralPath $root
        if (($rootItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            Throw-PRFailure 67 'path-safety' "Project root is a reparse point: $root"
        }
    }

    $relative = $target.Substring($root.Length).TrimStart('\', '/')
    $current = $root
    foreach ($segment in @($relative -split '[\\/]')) {
        if ([string]::IsNullOrWhiteSpace($segment)) {
            continue
        }
        $current = Join-Path $current $segment
        if (-not (Test-Path -LiteralPath $current)) {
            continue
        }
        $item = Get-Item -Force -LiteralPath $current
        if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            Throw-PRFailure 67 'path-safety' "Path contains a reparse point: $current"
        }
    }
}

function Assert-NoDescendantReparsePoint {
    param([string]$TargetPath)

    if (-not (Test-Path -LiteralPath $TargetPath)) {
        return
    }
    $targetItem = Get-Item -Force -LiteralPath $TargetPath
    if (-not $targetItem.PSIsContainer) {
        return
    }

    $pending = New-Object 'System.Collections.Generic.Queue[string]'
    $pending.Enqueue($targetItem.FullName)
    while ($pending.Count -gt 0) {
        $current = $pending.Dequeue()
        try {
            $children = @(Get-ChildItem -Force -LiteralPath $current)
        }
        catch {
            Throw-PRFailure 67 'path-safety' "Unable to inspect path descendants: $current"
        }
        foreach ($child in $children) {
            if (($child.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
                Throw-PRFailure 67 'path-safety' "Path contains a descendant reparse point: $($child.FullName)"
            }
            if ($child.PSIsContainer) {
                $pending.Enqueue($child.FullName)
            }
        }
    }
}

function Get-RepositoryRelativePath {
    param([string]$ProjectRoot, [string]$TargetPath)

    $root = Get-NormalizedPath $ProjectRoot
    $target = Get-NormalizedPath $TargetPath
    if (-not (Test-PathWithin $target $root)) {
        Throw-PRFailure 67 'path-safety' "Path is outside the project root: $target"
    }
    return $target.Substring($root.Length).TrimStart('\', '/').Replace('\', '/')
}

function Assert-GitIgnoredAndUntracked {
    param([string]$ProjectRoot, [string]$TargetPath)

    $relative = Get-RepositoryRelativePath $ProjectRoot $TargetPath
    $null = & git -C $ProjectRoot check-ignore -q -- $relative 2>$null
    $ignoreExit = $LASTEXITCODE
    if ($ignoreExit -eq 1) {
        Throw-PRFailure 67 'path-safety' "Output path is not ignored by Git: $relative"
    }
    if ($ignoreExit -ne 0) {
        Throw-PRFailure 65 'git-preflight' "git check-ignore failed with exit code $ignoreExit."
    }

    $tracked = @(& git -C $ProjectRoot ls-files -- $relative 2>$null)
    $trackedExit = $LASTEXITCODE
    if ($trackedExit -ne 0) {
        Throw-PRFailure 65 'git-preflight' "git ls-files failed with exit code $trackedExit."
    }
    if ($tracked.Count -gt 0) {
        Throw-PRFailure 67 'path-safety' "Output path contains tracked files: $relative"
    }
}

function Assert-SafeSavedOutput {
    param(
        [pscustomobject]$ProjectContext,
        [string]$TargetPath,
        [bool]$MustNotExist
    )

    $target = Get-NormalizedPath $TargetPath
    $savedRoot = Join-Path $ProjectContext.ProjectRoot 'Saved'
    if (-not (Test-PathWithin $target $savedRoot)) {
        Throw-PRFailure 67 'output-path' "Output must be below the project Saved directory: $target"
    }
    if ($MustNotExist -and (Test-Path -LiteralPath $target)) {
        Throw-PRFailure 67 'output-path' "Output path already exists: $target"
    }
    Assert-NoReparsePoint $ProjectContext.ProjectRoot $target
    Assert-NoDescendantReparsePoint $target
    Assert-GitIgnoredAndUntracked $ProjectContext.ProjectRoot $target
    return $target
}

function Get-FreeSpaceGiB {
    param([string]$Path)

    $root = [System.IO.Path]::GetPathRoot((Get-NormalizedPath $Path))
    if ([string]::IsNullOrWhiteSpace($root)) {
        Throw-PRFailure 65 'disk-space' "Unable to determine volume for path: $Path"
    }
    $driveName = $root.TrimEnd('\').TrimEnd(':')
    try {
        $drive = Get-PSDrive -Name $driveName -PSProvider FileSystem
    }
    catch {
        Throw-PRFailure 65 'disk-space' "Unable to read free space for volume: $root"
    }
    return [Math]::Round(($drive.Free / 1GB), 2)
}

function Get-PackageSpaceChecks {
    param(
        [pscustomobject]$ProjectContext,
        [pscustomobject]$EngineContext,
        [string]$ArchivePath,
        [bool]$Enforce
    )

    $requirements = @(
        [pscustomobject]@{ Name = 'SystemVolumeSpace'; Path = $env:LOCALAPPDATA; Minimum = 10 },
        [pscustomobject]@{ Name = 'EngineVolumeSpace'; Path = $EngineContext.Root; Minimum = 10 },
        [pscustomobject]@{ Name = 'ProjectVolumeSpace'; Path = $ProjectContext.ProjectRoot; Minimum = 20 },
        [pscustomobject]@{ Name = 'ArchiveVolumeSpace'; Path = $ArchivePath; Minimum = 20 }
    )
    if ([string]::IsNullOrWhiteSpace($requirements[0].Path)) {
        $requirements[0].Path = $env:SystemDrive + '\'
    }

    $checks = New-Object System.Collections.Generic.List[object]
    foreach ($requirement in $requirements) {
        $free = Get-FreeSpaceGiB $requirement.Path
        $satisfied = $free -ge $requirement.Minimum
        if ($Enforce) {
            $status = if ($satisfied) { 'PASS' } else { 'FAIL' }
        }
        else {
            $status = 'NOT_RUN'
        }
        $checks.Add((New-Check $requirement.Name $true $status ("{0} GiB free; {1} GiB required." -f $free, $requirement.Minimum)))
    }
    return $checks.ToArray()
}

function Invoke-PackageDevelopmentOperation {
    param([pscustomobject]$ProjectContext)

    $selectedConfiguration = $Configuration
    if ([string]::IsNullOrWhiteSpace($selectedConfiguration)) {
        $selectedConfiguration = 'Development'
    }
    if ($selectedConfiguration -ieq 'Development') {
        $selectedConfiguration = 'Development'
    }
    elseif ($selectedConfiguration -ieq 'Shipping') {
        $selectedConfiguration = 'Shipping'
    }
    else {
        Throw-PRFailure 64 'arguments' 'PackageDevelopment -Configuration must be Development or Shipping.'
    }

    Initialize-Report $ProjectContext 'PackageDevelopment' $selectedConfiguration $RunId
    $engine = Resolve-EngineContext $ProjectContext $EngineRoot @(
        'Engine\Build\BatchFiles\RunUAT.bat',
        'Engine\Build\BatchFiles\Build.bat',
        'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
    )
    Set-EngineResult $engine

    $maps = @(Get-PackagingMaps $ProjectContext)
    $script:Result['maps'] = $maps
    $script:Result['target'] = 'ProjectR'

    if ([string]::IsNullOrWhiteSpace($ArchiveRoot)) {
        $archivePath = Join-Path $ProjectContext.ProjectRoot ("Saved\Packages\{0}\{1}" -f $script:Result['runId'], $selectedConfiguration)
    }
    else {
        if (-not [System.IO.Path]::IsPathRooted($ArchiveRoot)) {
            Throw-PRFailure 64 'arguments' '-ArchiveRoot must be an absolute path below ProjectRoot\Saved.'
        }
        $archivePath = $ArchiveRoot
    }
    $archivePath = Assert-SafeSavedOutput $ProjectContext $archivePath $true

    $stagingPath = Join-Path $ProjectContext.ProjectRoot 'Saved\StagedBuilds'
    $null = Assert-SafeSavedOutput $ProjectContext $stagingPath $false

    $uatScript = Join-Path $engine.Root 'Engine\Build\BatchFiles\RunUAT.bat'
    $uatArguments = @(
        'BuildCookRun',
        ('-project={0}' -f $ProjectContext.ProjectFile),
        '-target=ProjectR',
        '-targetplatform=Win64',
        ('-clientconfig={0}' -f $selectedConfiguration),
        '-build',
        '-cook',
        '-stage',
        '-pak',
        '-iostore',
        '-package',
        '-archive',
        ('-archivedirectory={0}' -f $archivePath),
        ('-MapsToCook={0}' -f ($maps -join '+')),
        '-NoP4',
        '-UTF8Output',
        '-unattended'
    )
    Set-CommandReport $uatScript $uatArguments
    $script:Result['outputPaths'] = @($archivePath, $stagingPath)

    $checks = New-Object System.Collections.Generic.List[object]
    $checks.Add((New-Check 'PackagingMapsValidated' $true 'PASS' 'Five Config maps match the frozen v0.0.3 paths and exist on disk.'))
    $checks.Add((New-Check 'CommandPrepared' $true 'PASS' 'The exact BuildCookRun command was written before execution.' @($script:CommandPath)))
    foreach ($spaceCheck in @(Get-PackageSpaceChecks $ProjectContext $engine $archivePath (-not $WhatIf))) {
        $checks.Add($spaceCheck)
    }

    if ($WhatIf) {
        $checks.Add((New-Check 'PackageExecuted' $true 'NOT_RUN' 'WhatIf requested; RunUAT.bat was not invoked.'))
        $checks.Add((New-Check 'PackageArtifactsVerified' $true 'NOT_RUN' 'No package artifacts are asserted during WhatIf.' @($archivePath)))
        $script:Result['checks'] = $checks.ToArray()
        $script:Result['status'] = 'NOT_RUN'
        Write-RunLog 'WhatIf: PackageDevelopment command prepared; no child process was started.'
        return 0
    }

    $failedSpace = @($checks | Where-Object { $_.name -like '*VolumeSpace' -and $_.status -eq 'FAIL' })
    if ($failedSpace.Count -gt 0) {
        $script:Result['checks'] = $checks.ToArray()
        Throw-PRFailure 65 'disk-space' 'PackageDevelopment disk-space gate failed; see result.json checks.'
    }

    Assert-NoBlockingProcesses $ProjectContext
    $childExitCode = Invoke-LoggedNative $uatScript $uatArguments
    $script:Result['childExitCode'] = $childExitCode
    if ($childExitCode -ne 0) {
        $checks.Add((New-Check 'PackageExecuted' $true 'FAIL' "RunUAT.bat returned $childExitCode." @($script:LogPath)))
        $checks.Add((New-Check 'PackageArtifactsVerified' $true 'NOT_RUN' 'Packaging failed; output was not accepted.' @($archivePath)))
        $script:Result['checks'] = $checks.ToArray()
        $script:Result['status'] = 'FAIL'
        $script:Result['failureKind'] = 'child'
        Write-RunLog "PackageDevelopment failed with child exit code $childExitCode."
        return $childExitCode
    }

    $checks.Add((New-Check 'PackageExecuted' $true 'PASS' 'RunUAT.bat returned 0.' @($script:LogPath)))
    $executable = Join-Path $archivePath 'Windows\ProjectR.exe'
    $pakFiles = @(Get-ChildItem -LiteralPath $archivePath -Recurse -File -Filter '*.pak' -ErrorAction SilentlyContinue)
    $utocFiles = @(Get-ChildItem -LiteralPath $archivePath -Recurse -File -Filter '*.utoc' -ErrorAction SilentlyContinue)
    $ucasFiles = @(Get-ChildItem -LiteralPath $archivePath -Recurse -File -Filter '*.ucas' -ErrorAction SilentlyContinue)
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf) -or
        ($pakFiles.Count -eq 0) -or ($utocFiles.Count -eq 0) -or ($ucasFiles.Count -eq 0)) {
        $checks.Add((New-Check 'PackageArtifactsVerified' $true 'FAIL' 'Package returned 0 but executable/Pak/IoStore outputs are incomplete.' @($archivePath)))
        $script:Result['checks'] = $checks.ToArray()
        Throw-PRFailure 70 'postcondition' 'Packaged executable, Pak, UTOC, or UCAS output is missing.'
    }

    $artifacts = @($executable) + @($pakFiles.FullName) + @($utocFiles.FullName) + @($ucasFiles.FullName)
    $checks.Add((New-Check 'PackageArtifactsVerified' $true 'PASS' 'Windows executable, Pak, UTOC, and UCAS outputs exist.' $artifacts))
    $script:Result['checks'] = $checks.ToArray()
    $script:Result['status'] = 'PASS'
    Write-RunLog 'PackageDevelopment completed successfully.'
    return 0
}

function Get-CleanCandidates {
    param([pscustomobject]$ProjectContext, [string]$SelectedScope)

    $relativeCandidates = New-Object System.Collections.Generic.List[string]
    foreach ($relative in @(
        'Binaries',
        'Intermediate',
        'Saved\Cooked',
        'Saved\StagedBuilds',
        'Saved\ShaderDebugInfo',
        'Saved\Temp',
        'Saved\Sandboxes'
    )) {
        $relativeCandidates.Add($relative)
    }

    $pluginsRoot = Join-Path $ProjectContext.ProjectRoot 'Plugins'
    if (Test-Path -LiteralPath $pluginsRoot -PathType Container) {
        Assert-NoReparsePoint $ProjectContext.ProjectRoot $pluginsRoot
        $pluginDescriptors = @(Get-ChildItem -LiteralPath $pluginsRoot -Recurse -File -Filter '*.uplugin')
        foreach ($descriptor in $pluginDescriptors) {
            $pluginRoot = Split-Path -Parent $descriptor.FullName
            Assert-NoReparsePoint $ProjectContext.ProjectRoot $pluginRoot
            $relativePluginRoot = Get-RepositoryRelativePath $ProjectContext.ProjectRoot $pluginRoot
            $relativeCandidates.Add(($relativePluginRoot.Replace('/', '\') + '\Binaries'))
            $relativeCandidates.Add(($relativePluginRoot.Replace('/', '\') + '\Intermediate'))
        }
    }

    if ($SelectedScope -eq 'Deep') {
        foreach ($relative in @(
            'DerivedDataCache',
            '.vs',
            ($ProjectContext.ProjectName + '.sln'),
            ($ProjectContext.ProjectName + '.slnx')
        )) {
            $relativeCandidates.Add($relative)
        }
    }

    $seen = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    $candidates = New-Object System.Collections.Generic.List[object]
    foreach ($relative in $relativeCandidates) {
        $path = Get-NormalizedPath (Join-Path $ProjectContext.ProjectRoot $relative)
        if ($seen.Add($path)) {
            $candidates.Add([pscustomobject]@{
                RelativePath = $relative.Replace('\', '/')
                FullPath = $path
            })
        }
    }
    return $candidates.ToArray()
}

function Assert-SafeCleanCandidate {
    param([pscustomobject]$ProjectContext, [pscustomobject]$Candidate)

    $projectRoot = Get-NormalizedPath $ProjectContext.ProjectRoot
    $target = Get-NormalizedPath $Candidate.FullPath
    $driveRoot = Get-NormalizedPath ([System.IO.Path]::GetPathRoot($target))
    if ([string]::Equals($target, $projectRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
        [string]::Equals($target, $driveRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-PathWithin $target $projectRoot)) {
        Throw-PRFailure 67 'clean-path' "Unsafe clean target: $target"
    }

    $protectedRelativePaths = @(
        'Source', 'Content', 'Config', 'Docs', 'BuildScripts', '.git',
        $ProjectContext.ProjectName + '.uproject',
        'Saved\AutomationReports', 'Saved\Packages', 'Saved\Autosaves',
        'Saved\Collections', 'Saved\Config', 'Saved\SaveGames',
        'Saved\Logs', 'Saved\SourceControl', 'Saved\Screenshots'
    )
    foreach ($protectedRelative in $protectedRelativePaths) {
        $protected = Get-NormalizedPath (Join-Path $projectRoot $protectedRelative)
        if ([string]::Equals($target, $protected, [System.StringComparison]::OrdinalIgnoreCase) -or
            (Test-PathWithin $target $protected)) {
            Throw-PRFailure 67 'clean-path' "Clean target overlaps protected path: $protectedRelative"
        }
    }

    Assert-NoReparsePoint $projectRoot $target
    Assert-NoDescendantReparsePoint $target
    Assert-GitIgnoredAndUntracked $projectRoot $target
}

function Invoke-CleanGeneratedOperation {
    param([pscustomobject]$ProjectContext)

    $selectedScope = $Scope
    if ([string]::IsNullOrWhiteSpace($selectedScope)) {
        $selectedScope = 'Standard'
    }
    if ($selectedScope -ieq 'Standard') {
        $selectedScope = 'Standard'
    }
    elseif ($selectedScope -ieq 'Deep') {
        $selectedScope = 'Deep'
    }
    else {
        Throw-PRFailure 64 'arguments' 'CleanGenerated -Scope must be Standard or Deep.'
    }
    if ($Apply -and $WhatIf) {
        Throw-PRFailure 64 'arguments' 'CleanGenerated cannot combine -Apply and -WhatIf.'
    }

    Initialize-Report $ProjectContext 'CleanGenerated' $selectedScope $RunId
    $script:Result['platform'] = $null
    $script:Result['target'] = 'GeneratedOutput'

    $publicEntry = Join-Path $PSScriptRoot 'CleanGenerated.bat'
    $commandArguments = @(
        '-Project', $ProjectContext.ProjectFile,
        '-Scope', $selectedScope,
        '-RunId', $script:Result['runId']
    )
    if ($WhatIf) {
        $commandArguments += '-WhatIf'
    }
    if ($Apply) {
        $commandArguments += '-Apply'
        if (-not [string]::IsNullOrWhiteSpace($ConfirmProjectRoot)) {
            $commandArguments += @('-ConfirmProjectRoot', $ConfirmProjectRoot)
        }
    }
    Set-CommandReport $publicEntry $commandArguments

    $candidates = @(Get-CleanCandidates $ProjectContext $selectedScope)
    $candidatePaths = @($candidates | ForEach-Object { $_.FullPath })
    $script:Result['outputPaths'] = $candidatePaths

    $checks = New-Object System.Collections.Generic.List[object]
    foreach ($candidate in $candidates) {
        Assert-SafeCleanCandidate $ProjectContext $candidate
        $state = if (Test-Path -LiteralPath $candidate.FullPath) { 'present' } else { 'absent' }
        Write-RunLog ("Clean candidate [{0}]: {1} ({2})" -f $selectedScope, $candidate.FullPath, $state)
    }
    $checks.Add((New-Check 'CleanPathsValidated' $true 'PASS' 'Every enumerated candidate is contained, ignored, untracked, non-reparse, and outside protected paths.' $candidatePaths))

    if (-not $Apply) {
        $checks.Add((New-Check 'CleanApplied' $true 'NOT_RUN' 'Preview mode is the default; no path was deleted.'))
        $script:Result['checks'] = $checks.ToArray()
        $script:Result['status'] = 'PASS'
        Write-RunLog 'CleanGenerated preview completed; no files or directories were deleted.'
        return 0
    }

    if ([string]::IsNullOrWhiteSpace($ConfirmProjectRoot) -or
        -not [System.IO.Path]::IsPathRooted($ConfirmProjectRoot)) {
        $script:Result['checks'] = $checks.ToArray()
        Throw-PRFailure 64 'arguments' '-Apply requires an absolute -ConfirmProjectRoot.'
    }
    $normalizedConfirmation = Get-NormalizedPath $ConfirmProjectRoot
    if (-not [string]::Equals($normalizedConfirmation, $ProjectContext.ProjectRoot, [System.StringComparison]::Ordinal)) {
        $script:Result['checks'] = $checks.ToArray()
        Throw-PRFailure 67 'clean-confirmation' '-ConfirmProjectRoot must exactly match the normalized project root, including case.'
    }

    Assert-NoBlockingProcesses $ProjectContext
    $cleaned = New-Object System.Collections.Generic.List[string]
    foreach ($candidate in $candidates) {
        if (-not (Test-Path -LiteralPath $candidate.FullPath)) {
            continue
        }
        try {
            # Revalidate immediately before each destructive operation to narrow TOCTOU exposure.
            Assert-SafeCleanCandidate $ProjectContext $candidate
        }
        catch {
            if ($cleaned.Count -eq 0) {
                throw
            }
            $script:Result['cleanedPaths'] = $cleaned.ToArray()
            $checks.Add((New-Check 'CleanApplied' $true 'FAIL' "Clean stopped after path revalidation failed: $($_.Exception.Message)" @($candidate.FullPath)))
            $script:Result['checks'] = $checks.ToArray()
            $script:Result['status'] = 'FAIL'
            $script:Result['failureKind'] = 'partial-clean'
            Write-RunLog "CleanGenerated stopped after path revalidation failed: $($_.Exception.Message)"
            return 71
        }
        try {
            Remove-Item -LiteralPath $candidate.FullPath -Recurse -Force
            if (Test-Path -LiteralPath $candidate.FullPath) {
                throw "Path still exists after Remove-Item: $($candidate.FullPath)"
            }
            $cleaned.Add($candidate.FullPath)
            Write-RunLog "Deleted generated path: $($candidate.FullPath)"
        }
        catch {
            $script:Result['cleanedPaths'] = $cleaned.ToArray()
            $checks.Add((New-Check 'CleanApplied' $true 'FAIL' "Clean stopped after a partial failure: $($_.Exception.Message)" @($candidate.FullPath)))
            $script:Result['checks'] = $checks.ToArray()
            $script:Result['status'] = 'FAIL'
            $script:Result['failureKind'] = 'partial-clean'
            Write-RunLog "CleanGenerated stopped after a partial failure: $($_.Exception.Message)"
            return 71
        }
    }

    $script:Result['cleanedPaths'] = $cleaned.ToArray()
    $checks.Add((New-Check 'CleanApplied' $true 'PASS' 'Every existing approved generated path was removed.' @($cleaned)))
    $script:Result['checks'] = $checks.ToArray()
    $script:Result['status'] = 'PASS'
    Write-RunLog 'CleanGenerated Apply completed for the enumerated generated paths.'
    return 0
}

function Show-Usage {
    param([string]$SelectedOperation)

    switch ($SelectedOperation) {
        'BuildEditor' {
            Write-Host 'BuildEditor.bat -Project <absolute .uproject> [-EngineRoot <UE root>] [-Configuration Development|DebugGame] [-RunId <id>] [-WhatIf]'
        }
        'PackageDevelopment' {
            Write-Host 'PackageDevelopment.bat -Project <absolute .uproject> [-EngineRoot <UE root>] [-Configuration Development|Shipping] [-ArchiveRoot <path below ProjectRoot\Saved>] [-RunId <id>] [-WhatIf]'
        }
        'CleanGenerated' {
            Write-Host 'CleanGenerated.bat -Project <absolute .uproject> [-Scope Standard|Deep] [-Apply -ConfirmProjectRoot <exact root>] [-RunId <id>]'
        }
        'AutomationReport' {
            Write-Host 'AutomationReport.bat -Project <absolute .uproject> -EntryPoint <ASCII id> -ChecksFile <absolute JSON> [-EngineRoot <UE root>] [-RunId <id>] [-WhatIf]'
        }
        default {
            Write-Host 'Operations: BuildEditor, PackageDevelopment, CleanGenerated, AutomationReport'
        }
    }
}

function Assert-OperationParameterAllowlist {
    param([string]$SelectedOperation)

    $allowedByOperation = @{
        BuildEditor = @('Operation', 'Project', 'EngineRoot', 'Configuration', 'RunId', 'WhatIf', 'Help', 'RemainingArguments')
        PackageDevelopment = @('Operation', 'Project', 'EngineRoot', 'Configuration', 'ArchiveRoot', 'RunId', 'WhatIf', 'Help', 'RemainingArguments')
        CleanGenerated = @('Operation', 'Project', 'Scope', 'Apply', 'ConfirmProjectRoot', 'RunId', 'WhatIf', 'Help', 'RemainingArguments')
        AutomationReport = @('Operation', 'Project', 'EngineRoot', 'EntryPoint', 'ChecksFile', 'RunId', 'WhatIf', 'Help', 'RemainingArguments')
    }
    $allowed = $allowedByOperation[$SelectedOperation]
    $unsupported = @($script:BoundParameterNames | Where-Object { $allowed -notcontains $_ })
    if ($unsupported.Count -gt 0) {
        Throw-PRFailure 64 'arguments' ("Unsupported parameter(s) for {0}: {1}" -f $SelectedOperation, ($unsupported -join ', '))
    }
}

$finalExitCode = 0
try {
    if (@('BuildEditor', 'PackageDevelopment', 'CleanGenerated', 'AutomationReport') -notcontains $Operation) {
        Throw-PRFailure 64 'arguments' 'A valid -Operation is required. Use a public .bat entry point.'
    }
    Assert-OperationParameterAllowlist $Operation
    if ($Help) {
        Show-Usage $Operation
        exit 0
    }
    if (($null -ne $RemainingArguments) -and ($RemainingArguments.Count -gt 0)) {
        Throw-PRFailure 64 'arguments' ('Unsupported arguments: ' + ($RemainingArguments -join ' '))
    }

    $projectContext = Get-ProjectContext $Project
    switch ($Operation) {
        'BuildEditor' {
            $finalExitCode = Invoke-BuildEditorOperation $projectContext
        }
        'PackageDevelopment' {
            $finalExitCode = Invoke-PackageDevelopmentOperation $projectContext
        }
        'CleanGenerated' {
            $finalExitCode = Invoke-CleanGeneratedOperation $projectContext
        }
        'AutomationReport' {
            $finalExitCode = Invoke-AutomationReportOperation $projectContext
        }
    }
}
catch {
    $finalExitCode = Get-FailureExitCode $_.Exception
    $failureKind = Get-FailureKind $_.Exception
    $message = Protect-SensitiveText $_.Exception.Message
    if ($null -ne $script:Result) {
        $script:Result['status'] = 'FAIL'
        $script:Result['failureKind'] = $failureKind
        $existingChecks = @($script:Result['checks'])
        $script:Result['checks'] = @($existingChecks + (New-Check 'WrapperFailure' $true 'FAIL' $message))
        Write-RunLog "FAIL [$failureKind/$finalExitCode]: $message"
    }
    else {
        Write-Host "FAIL [$failureKind/$finalExitCode]: $message"
    }
}

$finalExitCode = Complete-Report $finalExitCode
exit $finalExitCode
