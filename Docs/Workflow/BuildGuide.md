---
title: "ProjectR Build、Package 与自动化报告指南"
subtitle: "UE5.8 稳定入口、参数、退出码、日志、清理保护和实际验收"
version: "v1.0"
date: "2026-07-11"
---

# 目标与冻结入口

本指南定义 v0.0.4 的公共自动化合同。所有命令可从任意工作目录调用；调用者必须传入绝对 `.uproject` 路径，不依赖 PATH 中的 Unreal、MSBuild 或 Visual Studio 工具。

| 入口 | 职责 | 默认行为 |
|---|---|---|
| `BuildScripts/BuildEditor.bat` | 构建 `ProjectREditor Win64` | Development |
| `BuildScripts/PackageDevelopment.bat` | Build/Cook/Stage/Pak/IoStore/Package/Archive Win64 客户端 | Development |
| `BuildScripts/PackageDev.bat` | 原样转发到 PackageDevelopment | 与规范入口相同 |
| `BuildScripts/CleanGenerated.bat` | 校验并列出可再生输出 | Standard Preview，不删除 |
| `BuildScripts/AutomationReport.bat` | 校验独立 checks 并生成统一报告 | 不执行被报告的操作 |

`BuildScripts/ProjectRAutomation.ps1` 是上述批处理的共同实现，不是替代公共入口。公共名称、参数语义、退出码和报告 Schema 的破坏性修改需要 ADR、消费者清单和兼容测试。

# 当前工程与工具链

- Project：`E:\MyWork\ProjectR\ProjectR.uproject`
- EngineAssociation：`5.8`
- 已验证 UE：`D:\Unreal Engine 5\UE_5.8`，5.8.0，CL 55116800。
- Build：`<EngineRoot>\Engine\Build\BatchFiles\Build.bat`
- UAT：`<EngineRoot>\Engine\Build\BatchFiles\RunUAT.bat`
- UBT：`<EngineRoot>\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll`
- Visual Studio：`D:\VisualStudio1`，18.5.2；MSVC 14.50.35717；Windows SDK 10.0.22621.0。

脚本解析 EngineRoot 的优先级为：

1. `-EngineRoot <absolute path>`。
2. 环境变量 `PROJECTR_UE_ROOT`。
3. 读取 `.uproject` 的 EngineAssociation，查询 Epic Launcher 安装数据，并在各文件系统卷的标准 `UE_5.8` 位置查找。

候选必须有匹配 Major/Minor 的 `Engine/Build/Build.version` 和当前操作需要的 Build/UAT/UBT 文件。零个或多个有效候选都以退出码 66 停止；脚本不猜测选择。

# 生成 Visual Studio 工程文件

Launcher 版 UE5.8 不提供项目根级 `GenerateProjectFiles.bat`。需要刷新 IDE 工程时，由 Codex 执行：

```powershell
& 'D:\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe' `
  /projectfiles 'E:\MyWork\ProjectR\ProjectR.uproject'
```

该操作不属于 BuildEditor 的隐式步骤；未实际运行时必须记录 `NOT RUN`。它不得修改 ProjectR Source、Content、Config 或 UE Package。

# 公共报告合同

通过 Project/操作基础参数校验并进入报告初始化后，每次调用都创建一个不可碰撞的本地目录：

```text
Saved/AutomationReports/<RunId>/<EntryPoint>-<Configuration>/
```

RunId 默认是 `yyyyMMddTHHmmssZ-<8位十六进制>`；自定义值只允许 ASCII 字母、数字、点、下划线和连字符。目录已由 `/Saved/` 忽略，CleanGenerated 永久保护该目录。

创建任何报告文件前，包装器先规范化目标路径，并要求它严格位于 `Saved/AutomationReports` 下、从项目根到目标的既有路径链不含 symlink/junction/reparse point、目标被 Git ignore 且不含 tracked path，同时目标报告目录尚不存在。任一门失败均以退出码 67 停止，不创建或覆盖报告证据。

每个操作目录包含：

- `command.txt`：执行前写入的脱敏准确命令。
- `run.log`：UTF-8 合并 stdout/stderr 和 UTC 包装器事件。
- `result.json`：SchemaVersion 1 的机器可读实际结果。

`result.json` 字段至少为：

```text
schemaVersion, runId, entryPoint, status, failureKind,
startUtc, endUtc, durationSeconds,
projectFile, projectRoot, engineRoot, engineVersion,
target, platform, configuration, maps,
commandFile, logFile, outputPaths, cleanedPaths,
childExitCode, exitCode, checks
```

`status` 和每个 check 的 status 大小写敏感，只允许 `PASS`、`FAIL`、`NOT_RUN`。参数或日志中名称匹配 Token、Key、Password、Secret 的值必须脱敏。报告目录已存在、输出路径碰撞或报告无法写入时停止，不覆盖旧证据。

`-WhatIf` 只验证参数、路径、Engine、Config 和命令生成；入口退出码可以为 0，但总体 status 必须是 `NOT_RUN`，不能据此声明真实 Build 或 Package 通过。

# BuildEditor

## 输入

```text
BuildEditor.bat
  -Project <absolute .uproject>
  [-EngineRoot <absolute UE root>]
  [-Configuration Development|DebugGame]
  [-RunId <ASCII id>]
  [-WhatIf]
```

查询帮助：

```powershell
& 'E:\MyWork\ProjectR\BuildScripts\BuildEditor.bat' -Help
```

Development 实际入口：

```powershell
& 'E:\MyWork\ProjectR\BuildScripts\BuildEditor.bat' `
  -Project 'E:\MyWork\ProjectR\ProjectR.uproject' `
  -EngineRoot 'D:\Unreal Engine 5\UE_5.8' `
  -Configuration Development
```

包装器生成的准确子命令为：

```powershell
& 'D:\Unreal Engine 5\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  ProjectREditor Win64 Development `
  '-Project=E:\MyWork\ProjectR\ProjectR.uproject' `
  -WaitMutex -FromMsBuild -architecture=x64 -NoHotReloadFromIDE
```

执行前检查同工程 UnrealEditor、Live Coding、UBT 和 UAT/AutomationTool 进程；发现占用时退出 68，不关闭或强杀进程。成功必须同时满足子进程退出 0、配置对应的模块 DLL 存在且日志没有 Live Coding 阻断：Development 对应 `Binaries/Win64/UnrealEditor-ProjectR.dll`，DebugGame 对应 `Binaries/Win64/UnrealEditor-ProjectR-Win64-DebugGame.dll`。仅看到旧 DLL 或 WhatIf 命令不算 Build 通过。

# PackageDevelopment

## 输入

```text
PackageDevelopment.bat
  -Project <absolute .uproject>
  [-EngineRoot <absolute UE root>]
  [-Configuration Development|Shipping]
  [-ArchiveRoot <absolute path below ProjectRoot\Saved>]
  [-RunId <ASCII id>]
  [-WhatIf]
```

Development 实际入口：

```powershell
& 'E:\MyWork\ProjectR\BuildScripts\PackageDevelopment.bat' `
  -Project 'E:\MyWork\ProjectR\ProjectR.uproject' `
  -EngineRoot 'D:\Unreal Engine 5\UE_5.8' `
  -Configuration Development
```

默认归档目录为：

```text
Saved/Packages/<RunId>/<Configuration>/
```

`-ArchiveRoot` 必须是 `ProjectRoot/Saved` 下方的绝对路径、被 Git ignore、未跟踪、从项目根到目标的既有路径链不含 reparse point，而且在运行前不存在。默认 staging 根 `Saved/StagedBuilds` 必须通过相同的 Saved 范围、ignore/untracked 和祖先链门；若 staging 已存在，其全部后代也不得包含 symlink、junction 或其他 reparse point。

打包唯一地图事实源是 `Config/DefaultGame.ini`：

```text
/Game/ProjectR/Maps/L_MainMenu
/Game/ProjectR/Maps/L_RealityHub
/Game/ProjectR/Maps/L_Network_Prototype
/Game/ProjectR/Maps/L_CombatGym
/Game/ProjectR/Maps/L_BossGym
```

脚本要求恰好五条、顺序和大小写精确、无重复且对应 `.umap` 存在，再由该 Config 值生成 `-MapsToCook`。不使用 `AllMaps`、`AlwaysCookMaps`、`bCookAll` 或第二份脚本地图表；`/Game/ProjectR/MCPTest/**` 不进入正式包。

Development 的准确 UAT 形态为：

```powershell
& 'D:\Unreal Engine 5\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat' BuildCookRun `
  '-project=E:\MyWork\ProjectR\ProjectR.uproject' `
  -target=ProjectR -targetplatform=Win64 -clientconfig=Development `
  -build -cook -stage -pak -iostore -package -archive `
  '-archivedirectory=E:\MyWork\ProjectR\Saved\Packages\<RunId>\Development' `
  '-MapsToCook=/Game/ProjectR/Maps/L_MainMenu+/Game/ProjectR/Maps/L_RealityHub+/Game/ProjectR/Maps/L_Network_Prototype+/Game/ProjectR/Maps/L_CombatGym+/Game/ProjectR/Maps/L_BossGym' `
  -NoP4 -UTF8Output -unattended
```

真实执行前强制检查：

| 卷用途 | 最低可用空间 |
|---|---:|
| LocalAppData/系统卷 | 10 GiB |
| EngineRoot 所在卷 | 10 GiB |
| ProjectRoot 所在卷 | 20 GiB |
| ArchiveRoot 所在卷 | 20 GiB |

成功必须同时满足 UAT 退出 0、归档中存在 `Windows/ProjectR.exe`、至少一个 `.pak`、一个 `.utoc` 和一个 `.ucas`。脚本不使用 `-clean`，也不把 WhatIf 或已有包当作本次产物。

真实 Cook 会生成本地 `Build/<Platform>/FileOpenOrder/*.log`。仓库只忽略 `Build/**/FileOpenOrder/`，不忽略整个 `Build/`，因此未来需要版本控制的平台图标和构建配置仍可进入 Git。该目录不是 CleanGenerated 候选，本版本不删除其内容。

`PackageDev.bat` 是兼容别名：

```powershell
& 'E:\MyWork\ProjectR\BuildScripts\PackageDev.bat' `
  -Project 'E:\MyWork\ProjectR\ProjectR.uproject' `
  -Configuration Development
```

它不实现独立逻辑，原样转发参数和退出码。

# CleanGenerated

## 输入

```text
CleanGenerated.bat
  -Project <absolute .uproject>
  [-Scope Standard|Deep]
  [-Apply -ConfirmProjectRoot <exact normalized root>]
  [-RunId <ASCII id>]
```

默认预览命令：

```powershell
& 'E:\MyWork\ProjectR\BuildScripts\CleanGenerated.bat' `
  -Project 'E:\MyWork\ProjectR\ProjectR.uproject' `
  -Scope Standard
```

Standard 只枚举：

```text
Binaries
Intermediate
Saved/Cooked
Saved/StagedBuilds
Saved/ShaderDebugInfo
Saved/Temp
Saved/Sandboxes
Plugins/<实际含 .uplugin 的插件>/Binaries
Plugins/<实际含 .uplugin 的插件>/Intermediate
```

Deep 额外枚举：

```text
DerivedDataCache
.vs
ProjectR.sln
ProjectR.slnx
```

永久保护：

```text
Source, Content, Config, Docs, BuildScripts, .git, ProjectR.uproject
Saved/AutomationReports, Saved/Packages, Saved/Autosaves,
Saved/Collections, Saved/Config, Saved/SaveGames,
Saved/Logs, Saved/SourceControl, Saved/Screenshots
```

每个候选都必须位于规范化项目根内、不是项目根或驱动器根、被 Git ignore、无 tracked path、不与保护路径重叠；从项目根到候选的既有祖先、候选自身及其全部既有后代均不得是或包含 symlink、junction 或其他 reparse point。Apply 在每项删除前重新执行这些门，还要求没有同工程 Editor/Live Coding/UBT/UAT 进程，且 `-ConfirmProjectRoot` 与规范化根路径按大小写逐字符一致。

仅在获得独立破坏性批准后才能执行：

```powershell
& 'E:\MyWork\ProjectR\BuildScripts\CleanGenerated.bat' `
  -Project 'E:\MyWork\ProjectR\ProjectR.uproject' `
  -Scope Standard `
  -Apply `
  -ConfirmProjectRoot 'E:\MyWork\ProjectR'
```

v0.0.4 不运行 Apply。若逐项删除中途失败，立即停止并返回 71；`cleanedPaths` 记录已经删除的准确路径，不继续扩大范围。

# AutomationReport

## 输入

```text
AutomationReport.bat
  -Project <absolute .uproject>
  -EntryPoint <stable ASCII id>
  -ChecksFile <absolute JSON file, maximum 1 MiB>
  [-EngineRoot <absolute UE root>]
  [-RunId <ASCII id>]
  [-WhatIf]
```

ChecksFile 根必须是 JSON object，`checks` 必须是非空 JSON array；每项必须是非 null JSON object，并至少提供以下五个字段：

```json
{
  "checks": [
    {
      "name": "BlueprintCompiled",
      "required": true,
      "status": "PASS",
      "evidence": "actual tool result",
      "artifacts": []
    }
  ]
}
```

`name` 使用稳定 ASCII 标识；`required` 必须是 JSON boolean；`status` 大小写敏感且只允许 `PASS`、`FAIL`、`NOT_RUN`；`evidence` 必须是 JSON string；`artifacts` 必须是只含 JSON string 的 array；并且至少有一个 required check。Schema 在报告初始化前校验，非法输入返回 64 且不创建报告目录。汇总规则：

- 任一 check 为 FAIL：总体 FAIL，退出 1。
- 每个 required check 都为 PASS：总体 PASS，退出 0。
- 每个 required check 都为 NOT_RUN：总体 NOT_RUN，退出 0。
- required PASS 与 NOT_RUN 混合：总体 FAIL，退出 1。
- `-WhatIf` 只校验 Schema，总体 NOT_RUN，退出 0。

AutomationReport 不调用 MCP、不编译 Blueprint、不保存 Package、不重启 Editor、不运行 PIE；调用者必须提交实际证据。v0.0.5 应把这些生命周期状态拆成独立 check。

# 退出码合同

| 退出码 | 语义 |
|---:|---|
| 0 | 入口合同成功；WhatIf/NOT_RUN 必须另外读取 result status，不能只看退出码 |
| 1 | AutomationReport 的 checks 汇总为 FAIL |
| 64 | 参数或 ChecksFile Schema 无效 |
| 65 | Project、Config、地图、Git、进程读取或磁盘门禁预检失败 |
| 66 | UE/Toolchain 未找到或精确版本出现多个候选 |
| 67 | 输出/清理路径不安全、碰撞、未忽略、已跟踪或含 reparse point |
| 68 | 同工程 Editor、Live Coding、UBT/UAT 等活动进程阻断 |
| 69 | 报告目录、日志或 result.json 无法写入 |
| 70 | 包装器内部或子进程退出 0 后的产物后置条件失败 |
| 71 | CleanGenerated Apply 已开始后出现部分删除失败 |

Build.bat/RunUAT.bat 的非零退出码原样返回；`result.json.failureKind=child` 和 `childExitCode` 用于区分子进程失败与包装器码。任何调用都必须同时检查进程退出码、总体 status 和 required checks。

# v0.0.4 当前实际验收状态

| 检查 | RunId/证据 | 实际状态 |
|---|---|---|
| BuildEditor Development 命令生成 | `v004-final2-build-19f4d9279e6` | `NOT_RUN`；WhatIf，未调用 Build.bat |
| PackageDevelopment Development 命令生成 | `v004-final2-pkgdev-19f4d9279e6` | `NOT_RUN`；WhatIf，五张地图已验证，未调用 RunUAT.bat |
| PackageDevelopment Shipping 命令生成 | `v004-final2-pkgship-19f4d9279e6` | `NOT_RUN`；兼容入口 WhatIf，未调用 RunUAT.bat |
| CleanGenerated Standard | `v004-final2-cleanstd-19f4d9279e6` | PASS；Preview 完成，Apply `NOT RUN`，删除 0 |
| CleanGenerated Deep | `v004-final2-cleandeep-19f4d9279e6` | PASS；Preview 完成，Apply `NOT RUN`，删除 0 |
| AutomationReport Schema 冒烟 | `v004-final2-report-19f4d9279e6` | PASS |
| 公共脚本合同回归 | `Saved/AutomationTests/ProjectRAutomation.Contract.Tests.ps1` | PASS；主线程与独立复审均退出 0，Spec compliance PASS、Code quality Approved |
| 真实 BuildEditor Development | `v004-final-build-19f4ed6c55e` | PASS；包装器/子进程退出 0，UBT Succeeded，目标 DLL 后置条件 PASS |
| 真实 PackageDevelopment Development | `v004-actual-package-19f4ece2a19` | PASS；UAT/包装器退出 0，五地图与 EXE/Pak/UTOC/UCAS 后置条件 PASS |
| PIE / MCP 写操作 | 无 | `NOT RUN`；本版本不需要 |

ProjectR Editor 已经正常关闭且没有保存提示；真实 Build 与 Development Package 均已通过。Package 前后系统卷分别为 12.47/12.10 GiB，归档和报告保持 ignored/untracked。v0.0.4 已完成，ADR-013 已接受；`CURRENT_VERSION.md`、commit 和 push 仍不由本轮推进。

# 回滚与故障处理

- 文本、脚本和五条 packaging map Config 只反向撤销本轮准确 diff。
- Build/Cook/Stage/Package/AutomationReports 留在被忽略目录作为证据；不使用 `git clean`、硬重置或 CleanGenerated Apply 代替回滚。
- 子进程失败时从 `run.log` 第一条错误定位；只修当前版本引入的问题并重跑同一入口。
- Engine 路径歧义、系统卷空间和 Editor 保存提示按 `ManualOperationsRunbook.md` 第 10 节处理。
- 本版本不修改 UE Package、Blueprint、GameplayTags、Save、Source、Targets 或 CURRENT_VERSION，不调用 MCP 写工具，不暂存、提交或推送。
