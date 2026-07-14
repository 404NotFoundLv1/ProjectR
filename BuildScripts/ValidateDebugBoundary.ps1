[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot,

    [Parameter(Mandatory = $true)]
    [ValidateSet('Development', 'Shipping')]
    [string]$Configuration,

    [string]$ArchiveRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$violations = [System.Collections.Generic.List[string]]::new()
$missingEvidence = [System.Collections.Generic.List[string]]::new()

function Add-Violation([string]$Message) {
    $violations.Add($Message)
}

function Add-MissingEvidence([string]$Message) {
    $missingEvidence.Add($Message)
}

function Get-NormalizedExistingPath([string]$Path, [string]$Label) {
    if (-not [System.IO.Path]::IsPathRooted($Path)) {
        Add-MissingEvidence "$Label must be an absolute path: $Path"
        return $null
    }

    try {
        return [System.IO.Path]::GetFullPath((Resolve-Path -LiteralPath $Path).Path).TrimEnd('\')
    }
    catch {
        Add-MissingEvidence "$Label does not exist: $Path"
        return $null
    }
}

function Test-IsStrictChild([string]$Parent, [string]$Child) {
    $prefix = $Parent.TrimEnd('\') + '\'
    return $Child.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)
}

function Test-ContainsReparsePoint([string]$Root, [switch]$IncludeDescendants) {
    $current = Get-Item -LiteralPath $Root -Force
    while ($null -ne $current) {
        if (($current.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            return $true
        }

        if ($null -eq $current.Parent) {
            break
        }

        $current = $current.Parent
    }

    if ($IncludeDescendants) {
        $reparse = Get-ChildItem -LiteralPath $Root -Force -Recurse -ErrorAction Stop |
            Where-Object { ($_.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0 } |
            Select-Object -First 1
        if ($null -ne $reparse) {
            return $true
        }
    }

    return $false
}

function Get-SourceFiles([string]$Root, [switch]$ExcludeTests) {
    $files = Get-ChildItem -LiteralPath $Root -Recurse -File -ErrorAction Stop |
        Where-Object { $_.Extension -in @('.h', '.cpp', '.cs') }
    if ($ExcludeTests) {
        $files = $files | Where-Object { $_.FullName -notmatch '[\\/]Tests[\\/]' }
    }
    return @($files)
}

function Test-TextPattern(
    [System.IO.FileInfo[]]$Files,
    [string]$Pattern,
    [string]$Description,
    [switch]$ViolationWhenFound
) {
    if ($Files.Count -eq 0) {
        return $false
    }

    $matches = Select-String -Path $Files.FullName -Pattern $Pattern -AllMatches -ErrorAction Stop
    if ($ViolationWhenFound -and $null -ne $matches) {
        foreach ($match in @($matches)) {
            Add-Violation "${Description}: $($match.Path):$($match.LineNumber)"
        }
    }
    return $null -ne $matches
}

function Get-Json([string]$Path, [string]$Label) {
    try {
        return Get-Content -LiteralPath $Path -Raw -Encoding utf8 | ConvertFrom-Json
    }
    catch {
        Add-MissingEvidence "Unable to parse $Label JSON: $Path ($($_.Exception.Message))"
        return $null
    }
}

$normalizedProjectRoot = Get-NormalizedExistingPath $ProjectRoot 'ProjectRoot'
if ($null -eq $normalizedProjectRoot) {
    $missingEvidence | ForEach-Object { Write-Error $_ }
    exit 3
}

$uprojectFiles = @(Get-ChildItem -LiteralPath $normalizedProjectRoot -File -Filter '*.uproject')
if ($uprojectFiles.Count -ne 1) {
    Add-MissingEvidence "ProjectRoot must contain exactly one .uproject; found $($uprojectFiles.Count)."
}

$pluginRoot = Join-Path $normalizedProjectRoot 'Plugins\ProjectRDebug'
$pluginDescriptorPath = Join-Path $pluginRoot 'ProjectRDebug.uplugin'
$projectDescriptorPath = if ($uprojectFiles.Count -eq 1) { $uprojectFiles[0].FullName } else { $null }
if (-not (Test-Path -LiteralPath $pluginDescriptorPath -PathType Leaf)) {
    Add-MissingEvidence "ProjectRDebug descriptor is missing: $pluginDescriptorPath"
}

$pluginDescriptor = if (Test-Path -LiteralPath $pluginDescriptorPath -PathType Leaf) {
    Get-Json $pluginDescriptorPath 'ProjectRDebug.uplugin'
} else { $null }
$projectDescriptor = if ($null -ne $projectDescriptorPath) {
    Get-Json $projectDescriptorPath 'uproject'
} else { $null }

if ($null -ne $pluginDescriptor) {
    if ($pluginDescriptor.CanContainContent -ne $false) {
        Add-Violation 'ProjectRDebug must set CanContainContent=false.'
    }
    if (@($pluginDescriptor.SupportedTargetPlatforms) -notcontains 'Win64') {
        Add-Violation 'ProjectRDebug must support Win64 explicitly.'
    }
    $modules = @($pluginDescriptor.Modules)
    if ($modules.Count -ne 1) {
        Add-Violation "ProjectRDebug must contain exactly one module; found $($modules.Count)."
    }
    elseif ($modules[0].Name -ne 'ProjectRDebug' -or $modules[0].Type -ne 'Runtime') {
        Add-Violation 'ProjectRDebug module must be the Runtime ProjectRDebug module.'
    }
    else {
        if (@($modules[0].PlatformAllowList) -notcontains 'Win64') {
            Add-Violation 'ProjectRDebug module must allow Win64 explicitly.'
        }
        if (@($modules[0].TargetConfigurationDenyList) -notcontains 'Shipping') {
            Add-Violation 'ProjectRDebug module must deny Shipping.'
        }
    }
}

if ($null -ne $projectDescriptor) {
    $references = @($projectDescriptor.Plugins | Where-Object { $_.Name -eq 'ProjectRDebug' })
    if ($references.Count -ne 1) {
        Add-Violation "ProjectR.uproject must contain one ProjectRDebug reference; found $($references.Count)."
    }
    else {
        if ($references[0].Enabled -ne $true) {
            Add-Violation 'ProjectRDebug must be enabled for allowed configurations.'
        }
        if (@($references[0].PlatformAllowList) -notcontains 'Win64') {
            Add-Violation 'ProjectRDebug uproject reference must allow Win64.'
        }
        if (@($references[0].TargetConfigurationDenyList) -notcontains 'Shipping') {
            Add-Violation 'ProjectRDebug uproject reference must deny Shipping.'
        }
    }
}

if (Test-Path -LiteralPath (Join-Path $pluginRoot 'Content')) {
    Add-Violation 'ProjectRDebug must not contain a Content directory.'
}

$mainSourceRoot = Join-Path $normalizedProjectRoot 'Source\ProjectR'
$mainFiles = Get-SourceFiles $mainSourceRoot
Test-TextPattern $mainFiles '(?i)ProjectRDebug|LoadModule(?:Checked|Ptr)?\s*<[^>]*ProjectRDebug|/Script/ProjectRDebug' 'Main runtime module depends on ProjectRDebug' -ViolationWhenFound | Out-Null

$productionRoot = Join-Path $pluginRoot 'Source\ProjectRDebug'
if (-not (Test-Path -LiteralPath $productionRoot -PathType Container)) {
    Add-MissingEvidence "ProjectRDebug source root is missing: $productionRoot"
}
else {
    $productionFiles = Get-SourceFiles $productionRoot -ExcludeTests
    $forbiddenProduction = '(?i)ConsoleCommand|ProcessConsoleExec|ProcessEvent\s*\(|CallFunctionByNameWithArguments|FindFunction\s*\(|SpawnActor|DeleteGameInSlot|SaveGameToSlot|AsyncSaveGameToSlot|FFileHelper|IFileManager|FPlatformFile|FPlatformProcess|FHttpModule|HttpModule|Python|SavePackage|AssetTools|PackageTools|LoadModule(?:Checked|Ptr)?\s*<'
    Test-TextPattern $productionFiles $forbiddenProduction 'Forbidden Debug production API' -ViolationWhenFound | Out-Null

    $testFiles = @(Get-ChildItem -LiteralPath $productionRoot -Recurse -File -ErrorAction Stop |
        Where-Object { $_.FullName -match '[\\/]Tests[\\/]' -and $_.Extension -in @('.h', '.cpp') })
    $forbiddenTest = '(?i)SpawnActor|DeleteGameInSlot|SaveGameToSlot|AsyncSaveGameToSlot|FFileHelper|IFileManager|FPlatformFile|FPlatformProcess|SavePackage|AssetTools|PackageTools'
    Test-TextPattern $testFiles $forbiddenTest 'Forbidden Debug test API' -ViolationWhenFound | Out-Null
}

$contentRoot = Join-Path $normalizedProjectRoot 'Content'
if (Test-Path -LiteralPath $contentRoot -PathType Container) {
    $forbiddenContent = @(Get-ChildItem -LiteralPath $contentRoot -Recurse -File -ErrorAction Stop |
        Where-Object {
            $_.BaseName -match '(?i)ProjectRDebug|WBP_.*Debug|IA_.*Debug|IMC_.*Debug'
        })
    foreach ($item in $forbiddenContent) {
        Add-Violation "Forbidden Debug content package: $($item.FullName)"
    }
}

if (-not [string]::IsNullOrWhiteSpace($ArchiveRoot)) {
    $normalizedArchiveRoot = Get-NormalizedExistingPath $ArchiveRoot 'ArchiveRoot'
    $savedRoot = Get-NormalizedExistingPath (Join-Path $normalizedProjectRoot 'Saved') 'Project Saved root'
    if ($null -ne $normalizedArchiveRoot -and $null -ne $savedRoot) {
        if (-not (Test-IsStrictChild $savedRoot $normalizedArchiveRoot)) {
            Add-MissingEvidence "ArchiveRoot must be strictly below ProjectRoot\Saved: $normalizedArchiveRoot"
        }
        if (Test-ContainsReparsePoint $normalizedArchiveRoot -IncludeDescendants) {
            Add-MissingEvidence "ArchiveRoot path chain or descendants contain a reparse point: $normalizedArchiveRoot"
        }

        & git -C $normalizedProjectRoot check-ignore -q -- $normalizedArchiveRoot
        if ($LASTEXITCODE -ne 0) {
            Add-MissingEvidence "ArchiveRoot is not covered by Git ignore rules: $normalizedArchiveRoot"
        }
        $trackedArchivePaths = @(& git -C $normalizedProjectRoot ls-files -- $normalizedArchiveRoot)
        if ($LASTEXITCODE -ne 0 -or $trackedArchivePaths.Count -ne 0) {
            Add-MissingEvidence "ArchiveRoot contains or resolves to tracked paths: $normalizedArchiveRoot"
        }

        $receiptRoots = @(
            (Join-Path $normalizedProjectRoot 'Binaries\Win64'),
            $normalizedArchiveRoot
        ) | Where-Object { Test-Path -LiteralPath $_ -PathType Container }
        $receipts = foreach ($root in $receiptRoots) {
            Get-ChildItem -LiteralPath $root -Recurse -File -Filter '*.target' -ErrorAction SilentlyContinue
        }
        $matchingReceipts = [System.Collections.Generic.List[object]]::new()
        foreach ($receipt in @($receipts)) {
            $json = Get-Json $receipt.FullName 'target receipt'
            if ($null -ne $json -and $json.Configuration -eq $Configuration -and $json.TargetName -eq 'ProjectR') {
                $matchingReceipts.Add([pscustomobject]@{ Path = $receipt.FullName; Json = $json })
            }
        }
        if ($matchingReceipts.Count -eq 0) {
            Add-MissingEvidence "No ProjectR $Configuration target receipt was found."
        }
        else {
            $receipt = $matchingReceipts | Sort-Object { (Get-Item -LiteralPath $_.Path).LastWriteTimeUtc } -Descending | Select-Object -First 1
            $buildPlugins = @($receipt.Json.BuildPlugins)
            if ($Configuration -eq 'Development' -and $buildPlugins -notcontains 'ProjectRDebug') {
                Add-Violation "Development receipt does not contain ProjectRDebug: $($receipt.Path)"
            }
            if ($Configuration -eq 'Shipping' -and $buildPlugins -contains 'ProjectRDebug') {
                Add-Violation "Shipping receipt contains ProjectRDebug: $($receipt.Path)"
            }
        }

        $runId = Split-Path -Leaf (Split-Path -Parent $normalizedArchiveRoot)
        $packageLog = Join-Path $normalizedProjectRoot "Saved\AutomationReports\$runId\PackageDevelopment-$Configuration\run.log"
        $gameUhtManifest = Join-Path $normalizedProjectRoot 'Intermediate\Build\Win64\ProjectR\ProjectR.uhtmanifest'

        if ($Configuration -eq 'Development') {
            $evidenceFiles = @($packageLog, $gameUhtManifest) | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf }
            if ($evidenceFiles.Count -eq 0) {
                Add-MissingEvidence 'Development UHT manifest or package Build log is missing.'
            }
            elseif (-not (Select-String -Path $evidenceFiles -Pattern 'ProjectRDebug' -Quiet)) {
                Add-MissingEvidence 'Development UHT manifest or package Build log lacks ProjectRDebug module evidence.'
            }
        }
        else {
            $shippingEvidenceRoots = @(
                $normalizedArchiveRoot,
                (Join-Path $normalizedProjectRoot 'Saved\StagedBuilds')
            ) | Where-Object { Test-Path -LiteralPath $_ -PathType Container }
            foreach ($root in $shippingEvidenceRoots) {
                $forbiddenPaths = @(Get-ChildItem -LiteralPath $root -Recurse -Force -ErrorAction Stop |
                    Where-Object { $_.FullName -match '(?i)ProjectRDebug' })
                foreach ($item in $forbiddenPaths) {
                    Add-Violation "Shipping output contains ProjectRDebug path: $($item.FullName)"
                }

                $textEvidence = @(Get-ChildItem -LiteralPath $root -Recurse -File -ErrorAction Stop |
                    Where-Object { $_.Extension -in @('.json', '.txt', '.log', '.xml', '.target', '.manifest') })
                Test-TextPattern $textEvidence 'ProjectRDebug' 'Shipping output text contains ProjectRDebug' -ViolationWhenFound | Out-Null
            }

            if (Test-Path -LiteralPath $packageLog -PathType Leaf) {
                $packageLines = @(Get-Content -LiteralPath $packageLog -ErrorAction Stop)
                $shippingSections = [System.Collections.Generic.List[string]]::new()

                $shippingToolchainStart = -1
                for ($index = 0; $index -lt $packageLines.Count; ++$index) {
                    if ($packageLines[$index] -match 'ProjectR Win64 Shipping:') {
                        $shippingToolchainStart = $index
                        break
                    }
                }
                if ($shippingToolchainStart -ge 0) {
                    for ($index = $shippingToolchainStart; $index -lt $packageLines.Count; ++$index) {
                        if ($index -gt $shippingToolchainStart -and $packageLines[$index] -match '=+\s*$') {
                            break
                        }
                        $shippingSections.Add($packageLines[$index])
                    }
                }

                $shippingActionsStart = -1
                for ($index = 0; $index -lt $packageLines.Count; ++$index) {
                    if ($packageLines[$index] -match '\*\* For ProjectR-Win64-Shipping \*\*') {
                        $shippingActionsStart = $index
                        break
                    }
                }
                if ($shippingActionsStart -ge 0) {
                    for ($index = $shippingActionsStart; $index -lt $packageLines.Count; ++$index) {
                        $shippingSections.Add($packageLines[$index])
                        if ($packageLines[$index] -match 'WriteMetadata ProjectR-Win64-Shipping\.target') {
                            break
                        }
                    }
                }

                if ($shippingSections.Count -eq 0) {
                    Add-MissingEvidence "Shipping target-specific Build log section is missing: $packageLog"
                }
                elseif ($shippingSections -match 'ProjectRDebug') {
                    Add-Violation "Shipping target-specific Build log contains ProjectRDebug: $packageLog"
                }
            }
            else {
                Add-MissingEvidence "Shipping package Build log is missing: $packageLog"
            }
        }
    }
}

foreach ($message in $violations) {
    Write-Host "VIOLATION: $message" -ForegroundColor Red
}
foreach ($message in $missingEvidence) {
    Write-Host "MISSING: $message" -ForegroundColor Yellow
}

if ($violations.Count -gt 0) {
    Write-Host "ProjectRDebug boundary validation failed with $($violations.Count) violation(s)." -ForegroundColor Red
    exit 2
}
if ($missingEvidence.Count -gt 0) {
    Write-Host "ProjectRDebug boundary validation lacks $($missingEvidence.Count) required evidence item(s)." -ForegroundColor Yellow
    exit 3
}

Write-Host "ProjectRDebug boundary validation PASS ($Configuration)." -ForegroundColor Green
exit 0
