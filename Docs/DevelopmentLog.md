---
title: "ProjectR Development Log"
subtitle: "版本实施、验证、MCP 操作和兼容审查记录"
version: "v1.0"
date: "2026-07-10"
---

# 记录规则

每个公共版本追加一条记录，不重写历史。自动结果、人工判断、未运行项和已知风险分开记录。

# 2026-07-10 - 文档与工程基线审计

- UE5.8 ProjectR C++ 模板工程存在。
- Unreal MCP 已连接至 ProjectR Editor，读取当前关卡成功。
- 当前 MCP 为 19 Toolset、255 底层 Tool。
- ProjectR 尚无 Git、AGENTS.md、CURRENT_VERSION.md 和正式版本合同。
- 生成 CodexReady v6 文档体系；未实现任何游戏功能，未调用 MCP 写工具。
- 当前版本设为 v0.0.0。

# 2026-07-10 - v0.0.0 仓库基线实施（Completed）

- 清理前索引为 1299 个路径、未跟踪 293 个；生成/本地目录占暂存工作树约 4.77 GB。
- 用树 `e32a2786e2a870f622f5e715c6e502d2c179cea0` 保留索引回滚点；只清索引，工作树缺失文件 0。
- 经明确批准将 `docs` 只修正大小写为 `Docs`；22 份原文档的归一化聚合 SHA-256 前后一致。
- 创建 `.gitignore`、`.gitattributes`、`BuildScripts/README.md` 和详细仓库/UE 基线；UE Package 修改为 0。
- Git LFS 本地 hook 已安装；白名单索引为 876 个路径，753 个 UE Package 全部由 LFS 管理，保护目录索引项为 0。
- 第一次 Build 因 ProjectR Live Coding 在 UHT 后退出 1；用户授权正常关闭 ProjectR Editor 后，相同命令完成 48/48 actions，退出码 0，`Result: Succeeded`。
- ProjectR Editor 以 PID 39504 重启并重新持有 8000 端口；关卡和 `/Script/ProjectR.*` 身份验证通过。
- 自动 PIE 能启动、停止并返回有效非黑屏 PNG；三次日志增量比较新增阻断问题均为 0。
- MCP 只执行状态读取、PIE Start/Stop 和截图；没有资产保存、Blueprint 修改或 Package 写入。
- `git diff --check` 通过；全量 `git diff --cached --check` 因无 HEAD 而检查全部首次导入文件，发现模板 Source/Config 与既有文档的历史空白问题，按 KI-010 记录且不越权格式化。
- 用户按 ManualOperationsRunbook 第 6 节确认移动 PASS、镜头 PASS、跳跃 PASS；未报告失败复现。
- v0.0.0 基线提交已创建：`2bad8adb7fc2a38d27453bb07a4c91bac76e1726`（`v0.0.0 Init Steam production project`）；版本转换前工作区干净。
- push 和 GC 均未执行；`CURRENT_VERSION` 的推进由独立版本转换提交完成。
- Future Compatibility Review：未修改工程名、模块名、Targets、GameplayTag、Blueprint API、Save、Config 或资产路径；v0.0.1 可从稳定 `Docs`、构建入口和 `Source/ProjectR/Core` 扩展。

# 2026-07-10 - v0.0.1 工程规范与 DeveloperSettings（Completed）

- 创建 `CodingStandard.md` 和 `AssetNaming.md`；完善 AGENTS 必读顺序、CodexWorkflow、ManualOperationsRunbook 和 ADR-010。
- 在 `Source/ProjectR/Core` 创建 `UPRDeveloperSettings`，三个 Config 开关默认值为 Debug=false、MockDirector=true、Steam=false；未暴露 Blueprint API，未实现对应业务系统。
- 首次 Build 被运行中 Editor 的 Live Coding 阻断；确认无 PIE、无打开资产且地图未脏后正常关闭 ProjectR Editor。
- 第二次 Build 完成 UHT 和 C++ 编译，但因缺少 `DeveloperSettings` 直接模块依赖产生 11 个链接错误；按计划停止并取得用户对 `ProjectR.Build.cs` 唯一 Allowed-path 例外的明确批准。
- 在 `PublicDependencyModuleNames` 增加 `DeveloperSettings` 后，正式 `ProjectREditor Win64 Development` Build 退出 0，日志为 `Result: Succeeded`。
- ProjectR Editor 重启并重新持有 `127.0.0.1:8000`；MCP 反射类 `/Script/ProjectR.PRDeveloperSettings`，CDO 返回 `false/true/false`。
- MCP 仅执行状态、日志、类和属性读取；没有 set/save/create/move/delete、Blueprint 修改或 UE Package 写入。
- 用户按 ManualOperationsRunbook 第 7 节确认 `Game > ProjectR` 面板、`Feature Flags` 和三个默认值均为 PASS，未修改配置。
- `git diff --check`、Allowed-path、文件计数和保护路径检查通过；Source/Content/Config/地图为 89/753/5/4，CURRENT_VERSION、Targets、uproject、Config、Content 和资产路径不变。
- PIE 不属于本版本验收范围，标记 `NOT RUN`。
- v0.0.1 正式提交已创建：`cb6c22feb6e87c41630cfb880c5cd20ec68279bf`（`v0.0.1 Add coding standards and Codex workflow`）；版本转换前工作区干净。
- push 和 GC 均未执行；`CURRENT_VERSION` 的推进由独立版本转换提交完成。
- Future Compatibility Review：Core 设置类不依赖 GameplayTags、GAS、Director、Steam 或 Debug 具体类型；v0.0.2 可直接接入基础模块与 Tag Library，无需改写本版本公共设置契约。

# 2026-07-10 - v0.0.2 基础模块与 GameplayTags（Completed）

- 在 `ProjectR.Build.cs` 接入 GameplayAbilities、GameplayTags、GameplayTasks、NavigationSystem 公共依赖和 HTTP、Json、JsonUtilities、AudioMixer 私有依赖；既有 EnhancedInput、UMG、AIModule 及模板依赖保持不变。
- Runtime 启用 `GameplayAbilities`，Editor-only 启用实验性 `GameplayTagsToolset`；未启用 `GASToolsets`，未修改 `AdditionalDependencies`。
- 创建 `DefaultGameplayTags.ini`：53 个显式标签、9 个根命名空间、非空 DevComment、完整名称字典序；Config 导入和无效标签警告开启，Fast/Dynamic Replication 关闭，无 Redirect。
- 创建只供 C++ 使用的抽象 `UPRTagLibrary`，提供 16 个 checked、缓存的静态 getter；没有 UFUNCTION、Blueprint API、ASC、AttributeSet、GameplayAbility 或 GameplayEffect。
- 创建 GameplayTag taxonomy，补充跨模块接口合同、ADR-011、MCP Authoring 和人工只读备用 Runbook；没有创建或修改 UE Package。
- 静态门禁通过：Allowed paths 无越界，暂存路径 0，`git diff --check` 通过；Source/Content/Config/地图/LFS 为 91/753/6/4/753，CURRENT_VERSION、Targets、既有 Config 和 Content 未变。
- 确认无 PIE、无打开资产且默认地图未 Dirty 后，正常关闭 PID 46728 的 ProjectR Editor；正式 `ProjectREditor Win64 Development` Build 完成 5/5 actions，退出码 0，`Result: Succeeded`。
- ProjectR Editor 首次以 PID 46660 重启并重新持有 `127.0.0.1:8000`；GameplayAbilities、GameplayTagsToolset 和 ProjectR 模块均成功挂载/加载。终验静态门通过后再次正常关闭 Editor，第二次正式 Build 报告 Target up to date、0 actions、退出码 0 和 `Result: Succeeded`；Editor 最终以 PID 41844 恢复并通过最小只读 MCP 冒烟。
- MCP 实测为 20 Toolset、261 Tool；GameplayTagsToolset 含 6 个工具。`ListTags` 返回 59 个管理器节点，其中 53 个是 ini 显式标签、6 个是层级自动生成的父节点；53/53 的 `GetTagInfo` 来源均为 `DefaultGameplayTags.ini` 且注释非空。
- ObjectTools 反射 `/Script/ProjectR.PRTagLibrary`；GameplayTagsSettings CDO 返回 Import/Warn=true、Fast/Dynamic=false、Redirect 数组为空。
- MCP Operation Manifest 全部为只读，只调用 Toolset/Tag/类/属性/状态/日志查询；AddTag、RemoveTag、RenameTag 和所有 set/save/create/move/delete/Package 写工具均未调用。
- 重启后默认地图仍为 `/Game/ThirdPerson/Lvl_ThirdPerson`，PIE=false、打开资产为空、地图未 Dirty；PIE 与主观玩法验收不属于本版本，均标记 `NOT RUN`，自动证据充分，未触发人工 Gameplay Tags 面板备用步骤。
- 启动日志只有既有引擎示例自动化错误、非目标平台 SDK/采集 DLL 警告和一次重启后的旧 MCP session-id 自恢复记录；ProjectR/GameplayTags 专项日志无本版本阻断错误。
- v0.0.2 正式提交已创建：`37bad148674f5fb466c2335edcfed94ca42560ad`（`v0.0.2 Enable modules and gameplay tags`）；版本转换前工作区干净。
- push 和 GC 均未执行；`CURRENT_VERSION` 的推进由独立版本转换提交完成。
- Future Compatibility Review：v0.0.3 可直接使用 Core、模块下限和稳定 GameplayTags 创建 Game Framework；本版本没有改变工程名、模块名、Targets、构建入口、Blueprint API、Save 字段、Config 既有值或资产路径。后续 Tag 只能增量扩展；重命名/删除必须提供 Redirect、ADR、消费者清单和兼容测试。

# 2026-07-10 - v0.0.3 正式地图与 Game Framework（Completed）

- 创建 `EPRMapId`、`UPRGameInstance` 和六个正式 APR Framework 基类；`OpenMap` 冻结五个 Soft World 映射，失败显式返回并记录 `LogProjectR`。
- `APRPlayerCharacter` 只创建固定侧视 CameraComponent：位置 `(0,600,100)`、旋转 `(0,-90,0)`、FOV 60；未添加 Tick、输入、移动、平面约束、GAS、Save 或未来玩法。
- 正式 Build 首次执行即完成 11/11 actions，退出码 0，`Result: Succeeded`。
- 正常关闭 ProjectR Editor 后，使用 UE5.8 官方 `WorldPartitionRenameDuplicateBuilder` 串行创建五张正式地图；每张初始生成 65 个 External Actor、2 个 External Object 和 1 个 World Package，退出码均为 0。
- 通过 MCP 创建八个 Blueprint；全部 warnings-as-errors 编译并精确保存。三个 GameMode CDO 统一引用正式 Pawn/Controller/State；PlayerCharacter 临时使用 Quinn Simple 与 ABP_Unarmed。
- `BP_MainMenuFlow` 的 Graph DSL 为 `BeginPlay -> Delay(1.0) -> Cast UPRGameInstance -> OpenMap(RealityHub)`；未使用地图字符串或 Tick。MainMenu 放置 1 个 Flow Actor，最终新增 External Actor 精确落盘。
- 五张地图的 GameMode Override、1 个 PlayerStart、依赖和零 Dirty 状态在重启后逐张验证；MainMenu 不再依赖 `BP_ThirdPersonGameMode`。
- `DefaultEngine.ini` 将 Game/Editor 默认地图改为 L_MainMenu，GlobalDefaultGameMode 改为 BP_GameModeBase，并注册 BP_ProjectRGameInstance；其他 Config 保持不变。
- 自动 PIE 实际验证 MainMenu 在 1 秒后进入 RealityHub；RealityHub、Network Prototype、CombatGym、BossGym 均生成 1 个正式 Pawn、产生新的 Framework spawn 日志、返回非空截图数据且无新增阻断日志，每次 PIE 均已停止。
- 用户按 ManualOperationsRunbook 第 9 节确认 RealityHub、Network Prototype、CombatGym、BossGym 的固定侧视构图全部 PASS。
- MCP 实测发现三项 Experimental 工具差异：Graph DSL 在中文文化下无法匹配英文 AddEvent ID；WorldSettings 属于地图主 Package；新 External Actor 首次保存需使用其准确 Actor object ref。均通过受控、精确范围流程处理并记录在 MCP Authoring/KI-006。
- 最终 Source/Content Package/地图为 106/1102/9；新增 UE Package 349，既有 753 个 UE Package 未修改，暂存路径为 0。
- v0.0.3 实施未修改 `CURRENT_VERSION.md`、uproject、Build.cs、Targets、GameplayTags、Save 或模板/Variant 文件；正式提交已创建：`cc3cc695c1929bf2de0af40b3cd938f2af674176`（`v0.0.3 Create core framework and formal maps`），且该实施提交已与 `origin/main` 同步；GC 未执行，`CURRENT_VERSION` 的推进由独立版本转换提交完成。
- Future Compatibility Review：v0.0.4 可直接消费五个稳定地图路径、`EPRMapId`、`OpenMap`、默认启动设置和既有 Build 入口；v0.1.0 可在 `APRPlayerCharacter` 上增量实现输入与移动，无需重设 Blueprint 父类。

# 2026-07-11 - v0.0.4 自动化构建与报告入口（Completed）

- 已创建 BuildEditor、PackageDevelopment、PackageDev 兼容别名、CleanGenerated 和 AutomationReport 批处理入口及共同 PowerShell 实现；详细 CLI、退出码、日志 Schema、清理保护和回滚写入 BuildGuide。
- `DefaultGame.ini` 已按 v0.0.3 冻结顺序加入五条 `MapsToCook`；正式地图路径、Game Framework、Source、Content、uproject、Build.cs、Targets、DefaultEngine 和 GameplayTags 未修改，UE Package 写入为 0。
- 报告根固定为被忽略的 `Saved/AutomationReports/<RunId>/<EntryPoint>-<Configuration>/`，每次运行保存脱敏 command、UTF-8 log 和 SchemaVersion 1 result；WhatIf 与未执行状态不能冒充实际通过。
- 最终脚本快照的命令生成证据为：BuildEditor Development `v004-final2-build-19f4d9279e6`、Package Development `v004-final2-pkgdev-19f4d9279e6`、Package Shipping 兼容入口 `v004-final2-pkgship-19f4d9279e6`；三者均为 WhatIf，总体 `NOT_RUN`，没有调用 Build.bat 或 RunUAT.bat。
- CleanGenerated Standard/Deep 预览分别记录为 `v004-final2-cleanstd-19f4d9279e6` 和 `v004-final2-cleandeep-19f4d9279e6`，均为 PASS 且没有删除路径，Apply 为 `NOT RUN`；AutomationReport 冒烟 `v004-final2-report-19f4d9279e6` 为 PASS。
- 完整合同测试 `WindowsPowerShell -File Saved/AutomationTests/ProjectRAutomation.Contract.Tests.ps1` 在最终脚本快照实际退出 0；独立复审同样退出 0，并给出 Spec compliance PASS、Code quality Approved。覆盖原生 stdout/stderr 与退出码、严格 JSON、脱敏、配置段、路径碰撞、ancestor/descendant reparse、进程精确归属和 Clean 保护。
- 指向 `ProjectR.uproject` 的 Editor 均经正常窗口关闭退出，没有保存提示，Live Coding/UBT/UAT 和 8000 监听随后均为 0。最终 BuildEditor Development `v004-final-build-19f4ed6c55e` 包装器与子进程退出码均为 0，UBT `Result: Succeeded`，目标 DLL 后置条件 PASS。
- 用户将系统卷释放到 12.47 GiB 后，Development Package `v004-actual-package-19f4ece2a19` 实际运行 519.4 秒并 PASS：UAT/包装器退出 0，五张 Config 地图精确匹配，`Windows/ProjectR.exe`、Pak、两个 UTOC 和两个 UCAS 均存在且非空；归档与报告被 Git ignore 且未跟踪。Shipping Package 仍按合同只保留 WhatIf 证据。
- 首次真实 Cook 额外生成 `Build/Windows/FileOpenOrder/{CookerOpenOrder,EditorOpenOrder}.log`；UAT 日志明确包含 `WriteCookerOpenOrder`。`.gitignore` 只忽略 `Build/**/FileOpenOrder/`，不忽略整个 `Build/`，确保未来平台图标和构建配置仍可跟踪；合同测试已覆盖这两个边界。
- Clean Apply、工程文件生成、PIE、Editor restart、AutomationTestToolset 和 MCP 写操作均为 `NOT RUN`；本版本没有 UE Package、Blueprint 或玩法验收项。
- ADR-013 已接受；v0.0.5 草案的 External Package Allowed paths 与 Save All 冲突记录为 KI-011、E-012、E-013，留待 v0.0.5 启动审计修正，不提前修改任务合同或创建 MCPTest 资产。
- Future Compatibility Review：v0.0.5 可原样复用已通过合同回归、真实 Build 与真实 Package 的稳定入口、五地图 Config 事实源和 SchemaVersion 1 报告；`/Game/ProjectR/MCPTest/**` 未进入正式打包清单。工程名、模块名、Targets、地图路径、GameplayTags、Blueprint API、Save 和 UE Package 均未改变，无需下游大规模改写。
- v0.0.4 正式提交已创建：`380d2c1e7b4d1b5069a5252e1cb00e5e8b85b09e`（`v0.0.4 Add build scripts and build guide`），且该实施提交已与 `origin/main` 同步；GC 未执行，`CURRENT_VERSION` 的推进由独立版本转换提交完成。

# 2026-07-11 - v0.0.5 Unreal MCP 安全资产生产基线（Completed）

- KI-011 在首次 Package 写入前完成 RED→GREEN 合同门：Allowed paths 增加两个准确 External roots，Save All/空数组保存被禁止，重启顺序冻结为 compile PASS → exact save PASS → Dirty=0。
- BuildEditor Development `v005-build-20260711T022722Z` 实际退出 0；全部收尾后再次运行 `v005-final-build-20260711T034624Z`，目标 up to date、包装器/子进程退出 0、DLL 后置条件 PASS、`Result: Succeeded`。官方 `WorldPartitionRenameDuplicateBuilder` 实际退出 0，复制后为 1 Map、65 External Actor、2 External Object。
- 首次计划的 `/Script/Engine.PrimaryDataAsset` 因 UE5.8 标记 Abstract 而被 SavePackage 拒绝；用户批准只丢弃两个未落盘测试对象。随后使用具体 `PrimaryAssetLabel` 创建并保存 `DA_MCPAuthoringSmoke`，未删除或覆盖用户文件。
- 官方 Toolset 创建 `BP_MCPAuthoringSmokeActor`，配置 `SmokeData`、SmokeCube、Engine Cube、Transform 和 BeginPlay→PrintString；warnings-as-errors 编译与非空精确保存均 PASS。测试地图放置唯一 Actor 后新增 External Actor 为 66。
- 默认文化重启后整图 DSL 曾返回空；经用户批准的一次官方 node-level 只读复核确认 Graph、执行连接和 Pin 值完整存在，根因为本地化 Node Type ID 的反编译假阴性。未执行重复节点写入，也未创建 `ProjectRAuthoringTools`。
- 重启回载验证 DataAsset、Blueprint、组件、变量、Graph、引用和地图 Actor 均保持。Blueprint 依赖仅含测试 DataAsset 与 Engine Cube；DataAsset 仅被测试 Blueprint 引用；Blueprint 仅被测试 External Actor 引用，正式 Package 不引用 MCPTest。
- Selected Viewport PIE 实际为 running=true，日志新增 `ProjectR MCP smoke PASS`；截图非黑屏且测试 Cube 可见，新时间窗无阻断 Error。StopPIE 后 running=false。
- PIE 后 `/Game` inventory 为 302 项，其中 248 个可查询资产 Dirty 全为 false；54 个不可浏览 External Object 通过物理清单/SHA-256 门验证。最终新增 Package 71（3 主、66 EA、2 EO），工程 Package/地图为 1173/10；原有 1102 个 Package 与受保护工程文件哈希全部不变，暂存路径为 0。
- AutomationReport `v005-report-20260711T022722Z` 实际退出 0，14/14 required checks 为 PASS。KI-005、KI-011、KI-012 Closed；KI-004 Open，KI-006 Accepted Risk。
- Future Compatibility Review：v0.1.0 可直接复用 20/261 Toolset 矩阵、Operation Manifest、精确保存、复合 Dirty 门、重启回载、BuildEditor 和 AutomationReport。工程名、模块名、Targets、Config、Source、GameplayTags、正式地图/Framework、Save 和正式打包清单均未改变；未提前实现 v0.1.0。
- v0.0.5 正式提交已创建：`e9ba1ae3090c86688819b08b68aeb3effb0ab389`（`v0.0.5 Establish safe Unreal MCP authoring baseline`），且该实施提交已与 `origin/main` 同步；GC 未执行，`CURRENT_VERSION` 的推进由独立版本转换提交完成。

# 2026-07-11 - v0.1.0 Enhanced Input 与 2.5D 移动（Completed）

- 以 TDD 实施 `UPRInputConfigDataAsset`、Controller Mapping Context 生命周期、Character X/Z 移动/Y 平面约束/左右 Mesh 朝向和六个语义输入钩子；RED Build 因目标 Header 不存在退出 6，生产实现后的正式 Build 转为成功。
- Unreal MCP 串行创建并精确保存六个 InputAction、`IMC_Player`、`DA_PlayerInputConfig`，并修改 `BP_PlayerController` CDO；`BP_PlayerCharacter` 后续由用户明确确认保存并要求保留。九项自动非空保存 Manifest、用户批准的 Character Package 保存、重启回载与两个 Blueprint warnings-as-errors 编译均有独立证据。
- IMC 实际为 13 条基线 Mapping，A 唯一使用 `InputModifierNegate`；InputConfig 六组既有 Tag/Action 精确对应。正式输入资产不引用 `/Game/Input`、Variant 或 MCPTest；GameplayTag 仍为 53，Config、地图、uproject、Build.cs 和 Targets 未修改。
- 官方 20/261 Tool 缺少真实 PIE 输入注入，经用户批准新增最小 Editor-only `ProjectRAuthoringTools`。插件只提供 `RunPIEInputSmoke`，自动加载后 MCP 为 21/262，不保存 Package或暴露任意代码执行。
- 插件初次注册因 `Default` LoadingPhase 早于 ToolsetRegistrySubsystem 初始化而失败，改用官方 Toolset 相同的 `PostEngineInit` 后解决。首次输入调用因 `CreateSimulated` 的默认 null Viewport 在 GameViewportClient 中被解引用而崩溃；传入实际 `FSceneViewport` 与默认 InputDevice 后复验通过，崩溃没有产生 Package Dirty 或落盘变化。
- 修复后 CombatGym PIE 实测：D/A 位移 `+126.69/-94.12 cm`，键鼠/手柄跳跃上升约 `85.56 cm`，最大 Y 漂移 `0 cm`，Mesh yaw `-90/+90`，Actor 旋转与相机 Transform 稳定。Mapping Context Add 日志一次；Attack/Dodge/Interact/Execute 的 Pressed/Released 各收到一次键鼠和一次手柄事件。
- 原生自动化最终报告 `v010-native-final-20260711` 为 3 succeeded、0 warning、0 failure；加入 PIE 中断安全护栏后的最终 BuildEditor `v010-final-build-20260711b` 包装器/子进程退出 0。最终 Package/地图/Source/Config 为 1181/10/109/6，八个新 Package 具备 LFS 属性，原 1173 Package 聚合基线不变，暂存路径为 0。
- 自动 PIE 已 Stop，PIE 后 256 个可查询资产 Dirty=0；截图非黑屏且灰盒环境可读。首次人工结果暴露空中反向问题，随后按 KI-014 完成当前版本内修复和复验。
- 首次人工键鼠验收返回 FAIL：跳跃中按反方向只翻转 Mesh，实际仍沿原方向惯性移动。根因证据为正式 CharacterMovement `AirControl=0.05`、Falling lateral friction/braking 均为 0；用户确认采用空中反向时同幅翻转 X 速度的方案，不修改速度上限、跳高或资产。
- 新增 PIE 回归断言先在旧实现得到 `airReverseDeltaX≈0.000005 cm` 的预期 RED；修复后连续三次 PASS，0.1 秒反向位移约 `-44～47 cm`、Y 漂移 `0 cm`、相机/Actor 稳定；用户键鼠复验返回 PASS，KI-014 关闭。
- 用户追加要求 Mesh 转向不瞬切并批准 0.12 秒方案。实现以 Timer 驱动 Ease-In-Out，不启用永久 Tick，实际移动仍即时反向；快速 A/D 从当前 yaw 重启插值。旧实现的早期样本为端点 `90`（RED），新实现诊断得到早期 `-83.06`、最终 `90`，快速反向从当前 `62.22` 连续收敛（GREEN）。
- 最终 BuildEditor `v010-smooth-facing-final2-20260711` 退出 0；标准 PIE PASS：D/A 位移 `+126.69/-94.12 cm`、键鼠/手柄跳跃约 `85.56 cm`、Y 漂移 `0`、最终 Mesh yaw `-90/+90`、Actor/相机稳定。原生自动化报告 `v010-smooth-facing-final-20260711/Automation-Editor/index.json` 为 3/3 PASS；全 `/Game` 310 条记录中 256 个可查询资产 Dirty=0，54 条 ExternalObjects 按工具限制排除。用户平滑转向手感返回 PASS，KI-015 关闭。
- 最终 AutomationReport `v010-final-20260711/v010-final-None/result.json` 实际退出 0，22/22 required checks PASS；`PhysicalGamepadHandfeel` 为 optional `NOT_RUN`。v0.1.0 任务页和 VersionIndex 已标记 Completed。
- Future Compatibility Review（自动部分）：v0.1.1 可增量接入 ASC/AttributeSet；v0.1.3 可覆写 native Tag 钩子；v0.3.2 可增加 QTE Action/Tag/Mapping；v0.8.2 可替换玩家 Mapping。未引入未来具体类型、Save/Tag/API/正式地图破坏。
- v0.1.0 正式提交已创建：`2a71fd85a065085da48747f783ffe70cc22e1010`（`v0.1.0 Add Enhanced Input and 2.5D movement`），且该实施提交已与 `origin/main` 同步；GC 未执行，`CURRENT_VERSION` 的推进由独立版本转换提交完成。

# 2026-07-13 - v0.1.1 GAS ASC、AttributeSet 与默认属性（Completed）

- 按 TDD 新增薄 `UPRAbilitySystemComponent`、11 项 replicated `UPRAttributeSet`、统一原生属性事件，以及 PlayerState-owned ASC/Character Avatar 生命周期；RED Build 因目标 Header 不存在退出 6，生产实现后 BuildEditor 转为成功。
- `APRPlayerState` 持有 replicated Mixed-mode ASC 与 AttributeSet，Authority 只在 `WasSuccessfullyApplied()` 成功后记录默认 Instant GE 已应用；重复初始化、重新占有与 Pawn 替换不会重放默认值，旧 Pawn 不能清除新 Avatar。
- Attribute 默认值、Clamp、RepNotify Always、Max 比例调整和 MoveSpeed→CharacterMovement 同步均由 `ProjectR.GAS` 自动化覆盖；最终报告为 4 succeeded、0 warning、0 failure。
- 启用 Editor-only GASToolsets 后 MCP 实测为 24 Toolset/276 Tool；只调用 AttributeSet/AbilitySystemInspector 的 6 个只读工具，GameplayCueToolset 未调用。AttributeSetToolset 精确回读 11 项属性。
- Unreal MCP 在单次 ProgrammaticToolset 事务中创建并完整回读 `GE_DefaultAttributes` 的 11 项 Override Modifier；设置 `BP_PlayerState.DefaultAttributesEffect` 与 `NetUpdateFrequency=100` 后，GE、PlayerState、Character、Controller 四个 Blueprint warnings-as-errors 编译通过。
- 非空精确保存 Manifest 仅包含 `GE_DefaultAttributes` 与 `BP_PlayerState`；`BP_PlayerCharacter` 和 `BP_PlayerController` 未 Dirty、未保存。重启回载后引用、父类、Controller InputConfig 与 GE 配置保持正确。
- CombatGym PIE 中 Character/PlayerState Inspector 返回相同 11 项值；ActiveEffects、GrantedAbilities 和 ActiveTags 均为空。日志证明默认属性仅应用一次，Owner=PlayerState、Avatar=Character；有效非空 PNG 截图和新时间窗无阻断错误。
- v0.1.0 `RunPIEInputSmoke` 回归 PASS：D/A、键鼠/手柄跳跃、Y 漂移、空中反向、最终朝向和固定相机保持合同；StopPIE 后 257 个可查询资产 Dirty=0。
- 最终 BuildEditor `v011-final-build-20260713` 为 PASS，子进程退出 0、`Result: Succeeded`。最终 Source/Package/地图/Config 为 115/1182/10/6；GameplayTag 仍为 53，暂存路径为 0。
- 最终 AutomationReport `v011-final-20260713/v011-final-None/result.json` 实际退出 0，24/24 required checks 为 PASS；`NetworkPIEReplication` 为 optional `NOT_RUN`。
- Future Compatibility Review：v0.1.2 可在同一 ASC/AttributeSet 上增加伤害与死亡；v0.1.3 可在薄 ASC 上增加 AbilitySet/InputTag；v0.2.3 可只读属性并订阅统一事件。未新增 Ability、Damage、CombatEvent、HUD、Save、Tag 或正式地图依赖。
- v0.1.1 正式实施提交为 `3daedada51b19e389c9d590b606ae421e62f093c`，提交信息为 `v0.1.1 Add GAS ASC attributes and initialization`；该提交已同时位于 `main` 与 `origin/main`。
- `NetworkPIEReplication` 保持 optional `NOT_RUN`；本版本没有必需人工步骤，GC 未执行。
- v0.1.1 已正式关闭；版本转换将 `CURRENT_VERSION.md` 推进至 v0.1.2，不在该转换中实现 v0.1.2 功能。

# 2026-07-13 - v0.1.2 统一伤害、死亡、复活与受击反馈（Completed）

- 按 TDD 新增 DamageRequest/ReviveRequest/CombatEvent、两个窄接口、`UPRCombatSubsystem`、`UPRDamageExecutionCalculation` 与非复制 `IncomingDamage`。RED Build 仅因目标 Combat Header 缺失退出 6，生产实现后 BuildEditor 转为 PASS；资产 RED 仅因 GE/引用缺失失败。
- 伤害唯一沿 Subsystem→`GE_Damage`→Execution→IncomingDamage→AttributeSet 结算；Shield spill-over、Health Clamp、非法/超大值、Invulnerable、幂等死亡、可回滚 Revive 与七事件固定顺序由 `ProjectR.Combat` 覆盖。
- 新增 8 个 `Combat.*` Tag，显式总数 61；既有 53 个名称/注释不变，没有 Skill/QTE/测试 Tag。
- 官方工具在单次事务中创建并精确回读零 Modifier/单 Execution 的 `GE_Damage`；设置 `BP_PlayerState.DamageEffect` 后，GE 与三个 Player Blueprint warnings-as-errors 编译通过。精确保存仅包含 GE 与 PlayerState；Character/Controller 未 Dirty、未保存。重启回载与可查询资产 Dirty=0 PASS。
- 官方 24/276 Tool 无法调用 C++-only CombatSubsystem；用户批准新增一个固定 Combat PIE Tool，重启后为 25/277。工具固定 SourceId、阶段、数值与事件序列，不保存 Package或暴露任意执行能力。
- Pawn 替换测试发现 UE `DispatchRestart` 会覆盖 PossessedBy 中的死亡 MovementMode；在 Character `Restart` 后依据持续 Dead Tag 重应用死亡锁，失败用例转为 GREEN。
- `ProjectR.Combat` 最终 4/4（0 failure）、`ProjectR.GAS` 4/4、`ProjectR.Input` 3/3；BuildEditor `v012-restartfix-20260713a` 与 `v012-worldtimefix-20260713a` 均退出 0。
- CombatGym 客观 PIE PASS：硬直前台世界时间 0.115/0.100 秒；事件顺序 Damage, Damage, DamageRejected, Damage, Death, DamageRejected, Revive；最终 Health/Shield=100/50、Alive=true、IncomingDamage=0、ActiveEffects/GrantedAbilities=0。
- v0.1.0 Input smoke PASS：D/A 方向、双输入跳跃、Y 漂移 0、Mesh yaw -90/+90、Actor/相机稳定；截图非黑屏，StopPIE 后五个相关 Package Dirty=false。
- 启动日志已有 UE UnifiedError 自测、旧 MCP session-id、字体/驱动和 GameplayCue 路径警告；本轮 PIE 新时间窗没有 ProjectR Error、Ensure 或 Blueprint Runtime Error。
- 最终人工序列以 1.5 秒步进执行，短硬直实测 0.110/0.121 秒；用户持续/交替输入 A/D 后明确返回“受击手感：PASS”，死亡锁止与复活控制恢复验收通过。
- 最终 BuildEditor `v012-final-build-20260713` 的包装器与子进程退出码均为 0、`Result: Succeeded`。最终快照重新运行 `ProjectR.Combat` 4/4、`ProjectR.GAS` 4/4、`ProjectR.Input` 3/3 后，AutomationReport `v012-final-verified-20260713/v012-final-None/result.json` 退出 0，29/29 required checks PASS；`NetworkPIEReplication` 为 optional `NOT_RUN`，获批 Editor-only 工具扩展为 optional PASS。
- 最终计数复核区分主模块与 Editor 插件：`Source/ProjectR` 的 HEAD 基线实际为 113，本版本准确新增 8 项后为 121；插件 Source 从 4 增至 6。此前计划中的 115→123 混用了计数口径，不代表缺少实现文件；Package/地图/Config/Tag 最终为 1183/10/6/61。
- Future Compatibility Review：v0.1.3、v0.2.0、v0.2.1、v0.2.3 与 v0.3.2 可复用冻结的 DamageRequest/CombatEvent、接口、Tag 和唯一结算路径；未引入 Ability、敌人、HUD、QTE、Save 或具体下游消费者依赖。
- v0.1.2 正式实施提交为 `785cc2247eaad9cfc3ebb2b2b768018b3cd52a5a`，提交信息为 `v0.1.2 Add unified damage death and hit reactions`；该提交已同时位于 `main` 与 `origin/main`。
- GC 未执行。
- v0.1.2 已正式关闭；版本转换将 `CURRENT_VERSION.md` 推进至 v0.1.3，不在该转换中实现 v0.1.3 功能。

# 2026-07-13 - v0.1.3 AbilitySet、Cooldown、Cost 与 InputTag（Completed）

- 按 TDD 新增四个公共枚举/结构合同、`UPRGameplayAbility`、`UPRAbilitySetDataAsset`，并扩展 PlayerState-owned `UPRAbilitySystemComponent` 的 Authority 授予/移除、GrantId 记录、InputTag 路由、三种激活策略、生命周期事件、Dead/Revive 与 Avatar 替换。RED Build `v013-tdd-red-20260713a` 只因目标 Ability 类型缺失退出 6；生产 Build 转为 GREEN。
- 资产前 `ProjectR.Ability` 运行结果为 4/5，通过四项原生基础设施测试，唯一 RED 是七个验证 Package 不存在；资产、引用与回载完成后最终为 5/5 PASS。回归分别为 `ProjectR.GAS` 4/4、`ProjectR.Combat` 4/4、`ProjectR.Input` 3/3，无 failure/not-run；Combat 两条 warning 为既有 GameplayCue 路径提示与测试预期的超大伤害 Clamp。
- 新增 10 个 `Ability.*`/`Cooldown.Validation` Tag，显式总数 71、根 12、checked getter 30；`DefaultGame.ini` 只配置六个 UE5.8 Ability failure Tag。既有 61 个 Tag、正式 Input、Combat、AttributeSet、地图和 `GE_DefaultAttributes`/`GE_Damage` 未改变。
- 正式 `DA_DefaultAbilitySet` 保持空。隔离验证根创建两个 GE、三个 GA 与一个验证 AbilitySet；`BP_PlayerState.InitialAbilitySets` 只引用正式空 Set。Triggered Graph 为 Activate→Commit→Branch(true)→End，Held/Passive 保持活动以验证释放和生命状态。
- 官方 ObjectTools 实测不能可靠写入嵌套 `FPRAbilitySetEntry[]`；用户批准固定 `ConfigureAbilityValidationAssets()`，它只配置七个准确路径并回读、不保存。官方 Tool 同样不能触发 C++-only PIE Ability 生命周期，获批 `RunPIEAbilitySmoke()` 后总能力为 26 Toolset/279 Tool；既有 Input/Combat Tool 未修改。
- 八个 Manifest Package 全部 warnings-as-errors 编译并精确保存；Character/Controller 未 Dirty、未保存。英文文化 Graph DSL 回读意外把 Triggered GA 标脏，报告后仅对该准确 Package复核并单项保存。默认文化重启后七个新资产、Graph、PlayerState CDO 和引用保持，265 个可查询 `/Game` 资产 Dirty=false，54 个 ExternalObjects 按既有复合门排除。
- CombatGym PIE Ability smoke PASS：生命周期事件 23、验证 Spec 全部移除、Cooldown/ActiveEffect 清空，Energy/Health/Shield 恢复为 100/100/50。Input smoke PASS：D/A `+126.689/-94.123 cm`、跳跃 `89.6/85.56 cm`、Y 漂移 0、Mesh yaw `-90/+90`、Actor/相机稳定。前台 Floating PIE Combat 严格回归 PASS：硬直 0.1041/0.1046 秒、固定七事件、最终 Alive/100/50。
- 最终 BuildEditor `v013-final-build-20260713a` 包装器与子进程退出 0、目标 up to date；唯一持续 UBT warning 是插件 `.uplugin` 未声明 GameplayAbilities 依赖，因本版本禁止修改该路径记录为 KI-019。截图非黑屏但当前环境报告约 945.8 MiB 显存超预算；新 PIE 时间窗无 ProjectR Error、Ensure 或 Blueprint Runtime Error。
- 最终 AutomationReport `v013-final-report-20260713a/v013-final-None/result.json` 实际退出 0、总体 PASS：36/36 required PASS；`ProjectRAuthoringToolExtension` optional PASS，`NetworkPIEReplication` optional `NOT_RUN`。
- 实际计数复核：v0.1.2 后主模块已跟踪 Source 为 123，计划写成 121 是计数笔误；本轮准确新增 6 后为 129。插件 Source 6→8、UE Package 1183→1190、地图/Config 10/6、LFS tracked 仍为 1183，新七个 Package 属性均为 LFS，暂存路径为 0。
- Future Compatibility Review：v0.1.4 只持久化稳定 PrimaryAssetId/解锁 ID；v0.1.5 只用受控 ASC API；v0.2.0 可直接继承基类并填充正式 Set；v0.2.3 只读生命周期/RuntimeState；v0.3.2 只订阅事件；v0.4.2/v0.4.4 使用 Handle；v0.8.2 只改物理输入映射。未实现正式技能、Save、HUD、QTE、敌人或未来业务。
- v0.1.3 正式实施提交为 `f19b2a6d7ebe9dd99ae481f22026aff855bbe1ca`，提交信息为 `v0.1.3 Add ability set cooldown and input tag binding`；该提交已同时位于 `main` 与 `origin/main`。
- 本版本没有必需人工验收；`ProjectRAuthoringToolExtension` 保持 optional PASS，`NetworkPIEReplication` 保持 optional `NOT_RUN`，GC 未执行。
- v0.1.3 已正式关闭；版本转换将 `CURRENT_VERSION.md` 推进至 v0.1.4，不在该转换中实现 v0.1.4 功能。

# 2026-07-13 - v0.1.4 版本化 SaveGame、PRSV 与 A/B 存储（Completed）

- 按 TDD 新增五个 Save 枚举、最小 `UPRSaveGame`、逐版本 Migration Registry、`PRSV` Codec、受控 `ISaveGameSystem` Backend/Storage 和 `UPRSaveSubsystem`。RED Build `v014-tdd-red-20260713a` 只因目标 Save Header 不存在退出 6；实现后转为 GREEN。
- 计划中的 `PlatformFeatures` 私有模块依赖在 UE5.8 被 UBT 以 RulesError 8 拒绝。引擎源码核对证明 `IPlatformFeaturesModule` 位于既有 Engine 模块公开头；删除无效依赖后构建成功，最终 `ProjectR.Build.cs` 无 diff，未增加不必要模块。
- Schema 1 只保存 `SchemaVersion`、`SaveRevision` 和含 `ProfileId` 的 Profile。固定 16 字节 little-endian `PRSV` Header、CRC32、16 MiB 上限、严格长度/未来版本/错误 Save 类分类，以及副本式 `N -> N+1` 迁移均完成。
- A/B 状态机覆盖 Future barrier、损坏备用恢复、Profile/Revision/字节冲突、首次 A 与后续 B/A、写前观察快照、异步写后回读和失败保留上一有效代；保存队列为 single active + single trailing、last-write-wins，退出只取消未启动 trailing并使用 Weak callback。
- `ProjectR.Save` 最终 5/5 PASS：Schema、Migration、Envelope、Fake Runtime 与真实平台 Slot 五项均为 success，failure/not-run 为 0。真实平台测试只访问随机 `ProjectR_Automation_<GUID>` 的 `_A/_B`，Access Ledger 无生产槽，结束时通过平台 API 精确删除并复核两代不存在。最终独立审查又发现开发测试构建中的删除入口需二次防误用；补充 RED→GREEN 后，Production Storage 无清理能力，且删除前必须再次匹配准确 automation generation 命名，回归证明生产槽在平台访问前被拒绝。
- 独立历史回归实际 PASS：`ProjectR.Ability` 5/5、`ProjectR.GAS` 4/4、`ProjectR.Input` 3/3；`ProjectR.Combat` 4/4 全部状态 Success，其两条 warning 为既有 GameplayCue 路径提示和测试预期的超大伤害 Clamp。
- 唯一 ProjectR Editor PID 37308 持有 8000，反射 `/Script/ProjectR.PRSaveSubsystem`。两个 PIE GameInstance 均只记录 `ProjectR Save subsystem initialized without storage access.`，没有调用产品 Save API、访问生产槽或产生用户存档。
- 首个后台 In-Viewport Combat smoke 按 KI-020 得到 `0.3333s` 采样失败，已立即 StopPIE 且未复用会话；新前台 Floating PIE 严格回归为 `0.1049s/0.1004s`、七事件和最终 Health/Shield 100/50、Alive。Ability smoke 为 23 事件并清零验证 Spec/Cooldown，Input smoke 为 D/A `+126.761/-94.123 cm`、双跳跃 `85.556 cm`、Y 漂移 0、Actor/相机稳定。
- 截图非黑屏，新日志时间窗无 ProjectR Error、Ensure 或 Blueprint Runtime Error；StopPIE 后恢复 MainMenu。265 个可查询 `/Game` 资产 Dirty=false，Editor 无打开资产并正常关闭，无 Package 保存提示。
- 最终 BuildEditor `v014-delete-guard-final-build-20260713a` 包装器/子进程退出 0、`Result: Succeeded`；唯一持续 warning 为 KI-019 的插件 GameplayAbilities 元数据声明缺口。
- 原 1190 个 UE Package 的路径、长度和 SHA-256 全部不变，Package/地图/Config/Tag 为 1190/10/6/71；主模块 Source 从基线 127 增加十个 Save 文件为 137，插件 Source 保持 8。没有新增或修改 `.uasset/.umap`，暂存路径为 0。
- 最终 AutomationReport `v014-final-report3-20260713a/v014-final-None/result.json` 退出 0、总体 PASS：34/34 required PASS；`NetworkPIEReplication` 与 `ProjectRAuthoringToolExtension` 为 optional `NOT_RUN`。
- Future Compatibility Review：v0.1.5 只读 RuntimeState/Event；关系、Account/Run/Graveyard、Meta、Settings 与 Memory 通过严格 Schema 迁移加入；Steam Cloud 复用同一 A/B/PRSV 字节。没有持久化 UObject、ASC/Spec/Effect/Grant Handle、Held Input、Delegate 或 CombatEvent。
- 本版本无人工步骤，UE Package Manifest 为空；`NetworkPIEReplication` 与 `ProjectRAuthoringToolExtension` 保持 optional `NOT_RUN`。v0.1.4 正式提交已创建：`d746de9e56ab8ec8af657c01b9100315fbff45be`（`v0.1.4 Add versioned SaveGame subsystem`），且该实施提交已与 `origin/main` 同步；GC 未执行，`CURRENT_VERSION` 的推进由独立版本转换提交完成。

# 2026-07-14 - v0.1.5 隔离式 Debug 菜单（Completed）

- 已实现无 Content 的 `ProjectRDebug` Runtime 插件、11 项固定命令 Schema、弱 UObject Provider Registry、Game Thread/上下文/参数白名单、原生 Slate 面板、F1 InputProcessor、旅行/PIE 清理，以及六个主模块日志类别、独立 Debug 日志与 GUID 脱敏。
- TDD RED `v015-tdd-red-20260714a` 只因目标 Debug Header 缺失退出 6；最终 BuildEditor `v015-final-build-20260714a` 退出 0。`ProjectR.Debug` 在 Editor 和 Development Client 均为 12/12，Input/GAS/Combat/Ability/Save 回归分别为 3/3、4/4、4/4、5/5、5/5。
- Development Package `v015-dev-package-20260714b` 与 Shipping Package `v015-shipping-package-20260714a` 均退出 0；目标感知 `ValidateDebugBoundary.ps1` 对两者均退出 0。Shipping receipt/编译动作/Stage/Archive 不含 ProjectRDebug。
- Development DX11 诊断先完成 Status、两次 Damage、Revive、Save RuntimeState 只读、未来命令 NotAvailable 与 CombatGym 旅行；日志只包含 opaque Request token，旅行后面板自动关闭。Shipping DX11 诊断中两次 F1 无面板，D 键仍驱动正式角色移动。
- Development Client 首次打包因既有 Editor-only Input validation 测试在 Client 编译中调用 `IsDataValid` 失败。用户批准准确单文件例外后以 `WITH_EDITOR` 隔离该测试；Editor `ProjectR.Input` 仍为 3/3，Client 打包成功。
- 默认 D3D12/Ray Tracing 首次在 NVIDIA 566.26 `nvgpucomp64.dll` 中以 `0xc0000409` 崩溃，另一次记录 PSO `0x8007000e`；用户清洁更新到 610.47 后，同一 Development 归档稳定加载 CombatGym。默认 D3D12 完整烟测实际完成 F1、Status、Damage 25/100000、Revive、Save 只读、未来命令 NotAvailable、固定旅行及面板清理，日志无 ProjectR Error/Ensure/Blueprint Runtime Error，Windows 无新崩溃事件。
- 默认 D3D12 Shipping 归档稳定启动；自动发送两次 F1 后截图均无 Debug 面板，D 键移动改变正式场景构图，进程保持稳定，Archive 中 `ProjectRDebug` 文件数为 0。KI-021 已按新鲜 Development/Shipping 证据关闭，没有修改 Config 或默认 RHI。
- 当前 1190 个 Package 的路径、长度和 SHA-256 与基线完全一致；Package/地图/Config/Tag 为 1190/10/6/71，主模块 Source=141，两个插件 Source 合计=24，暂存路径=0。没有 UE Package、用户 Save、Config 或 GameplayTag 变更。
- ADR-020 与 DataAndInterfaceContracts 已冻结结构化命令、Provider 生命周期、Slate/F1、日志脱敏和 Shipping 边界。最终 AutomationReport `v015-final-report-after-driver-20260714b` 为 37/37 required PASS；`NetworkPIEReplication` 与 `ProjectRAuthoringToolExtension` 为 optional `NOT_RUN`。v0.1.5 正式提交已创建：`ee0ff0de295d67269aa1b05530e3e0a4c0400706`（`v0.1.5 Add isolated debug tools and log channels`），且该实施提交已与 `origin/main` 同步；GC 未执行，`CURRENT_VERSION` 的推进由独立版本转换提交完成。

# 2026-07-14 - v0.2.0-A 技能公共合同与目标选择（Completed）

- 在 `CURRENT_VERSION.md` 继续指向 v0.2.0/In Progress 的前提下完成内部检查点 A；公共版本保持 v0.2.0/In Progress，未推进。
- 新增 PlayerSkill 公共类型/接口、Skill DataAsset 与六类 Fragment、公共 GA 基类、六个无业务逻辑 native GA 空壳、Avatar Component、World Subsystem，以及 Combat Mitigation/EventSubject/AttackProxy 接口。ASC 继续独占 AbilitySet、Spec、Input、Commit、Cost/Cooldown；AttributeSet、Save 与 ProjectRDebug 未修改。
- 目标查询冻结 X/Z 投影、Source Y 平面、1 cm 容差、Pawn/WorldDynamic 候选、Visibility LOS、TargetId 去重和距离/角度/ID 稳定排序；位移使用 Authority-only `FRootMotionSource_MoveToForce`、Priority 500、Sweep/StopDistance、Timer 监控与 World/Avatar/Dead/Stunned 幂等清理，不使用运行时 `SetActorLocation` 或永久 Tick。
- Combat 在 `FPRDamageRequest` 末尾追加 ImpactOrigin/IncomingDirection，保持旧状态值并追加 `RejectedBlocked=5`；冻结 Dead → Mitigation → Invulnerable → GAS 顺序和零数值 AbilityOutcome。A 中 Character→Component Mitigation 始终返回 NotHandled，不提前实现护壁。
- GameplayTag 由 71 增至 94，根保持 12，checked getter 由 30 增至 47；InputConfig 精确由 6 增至 11，IMC 由 13 增至 26，旧 6/13 的顺序、Action、Key 和 Modifier 全部保持。AfterimageDodge 继续复用 Dodge。
- Unreal MCP 以预写 Manifest 串行创建 5 IA、12 GE、6 GA BP、6 Skill DA，修改 InputConfig/IMC；18 个 BP warnings-as-errors 编译通过，31 个准确 Package 非空保存，Dirty=0 后正常重启并逐项回读。`DA_DefaultAbilitySet` 始终为空且未保存；没有状态 GE、VFX、SFX、MCPTest 或未来资产依赖。
- TDD RED 只因目标公共类型/API 缺失。最终 BuildEditor `v020a-finalbuild7-20260714` 退出 0；`ProjectR.PlayerSkill` 5/5，Input/GAS/Combat/Ability/Save/Debug 分别 3/3、4/4、4/4、5/5、5/5、12/12。Debug 使用其冻结的 CombatGym `-game` Automation 调用；普通无 GameInstance 的 Editor 调用失败证据未被当作回归结果。
- Future Compatibility Review：B 复用 Snapshot/Forward 查询并在完成逻辑后授予 Shadow/Fire；C 复用 GroundArea、Timer、位移与 AttackProxy；D 复用 Mobility 与 Mitigation；E 才补状态/表现/完整六 Entry/PIE/人工手感。v0.2.1–v0.4.4 只消费稳定接口、Tag、PrimaryAssetId、Fragment 与事件，不保存 SpecHandle/Timer/Target/UObject。
- 回滚边界已记录于 ADR-021 和 v0.2.0 A 子合同；未获逐 Package 删除批准时不删除已保存资产。`PIE = NOT_RUN`；`NetworkPIEReplication`、`PhysicalGamepad`、`HumanSkillFeelJudgment` 与 `ProjectRAuthoringToolExtension` 为 optional `NOT_RUN`；`Package = NOT_RUN`，GC 未执行。v0.2.0-A 正式实施提交为 `e17d0b404461cba95a823cb914d94c93d718ff2c`（`v0.2.0-A Add player skill contracts and targeting foundation`），且该实施提交已与 `origin/main` 同步。

# 2026-07-15 - v0.2.0-B 影刃突刺与焚码斩（Completed）

- 在 `CURRENT_VERSION.md` 继续指向 v0.2.0/In Progress 的前提下实现 ShadowThrust Startup/受控 RootMotion/路径扫描/单 TargetId 单次命中/阻挡 Recovery，以及 FireSlash 扇形首击和唯一 Burning 注册表；所有伤害仍只走 `UPRCombatSubsystem`，生产路径不随机暴击。
- A 的 public/protected PlayerSkill、Target、Displacement、Combat、ASC、Input 和 Tag 合同未改变。新增方法、Timer、EffectHandle、Snapshot 与去重状态均为具体 GA 或 SkillSubsystem 私有实现；AttributeSet、PlayerCharacter、PlayerSkillComponent、Save 与 ProjectRDebug 未修改。
- Unreal MCP 创建 `/Game/ProjectR/Abilities/Effects/GE_State_Burning`，修改 `/Game/ProjectR/Abilities/Skills/GA_Skill_FireSlash` 与 `/Game/ProjectR/Abilities/DA_DefaultAbilitySet`，并把正式 DefaultAbilitySet 从空数组精确改为 ShadowThrust、FireSlash 两项。官方 ObjectTools 本轮可事务式写入并回读嵌套 `FPRAbilitySetEntry[]`，因此没有调用会清空正式集合的旧验证工具，也没有新增资产配置 Toolset。
- 三个准确 Package 分阶段非空保存，保存后 Dirty=0；两次正常重启均回读类、CDO、Stacking/Tag、引用及 AbilitySet 顺序。十个相关 Blueprint warnings-as-errors 编译通过；Shadow GA、两个 Skill DA、四个既有 Cost/Cooldown GE、Input 和三个 Player Blueprint 未保存。
- TDD RED 来自两个正式技能行为/资产缺失；固定 PIE 后续又以原生回归准确捕获正式 Mesh Forward 投影为零和 RootMotionSource 移除后残余速度未清的问题。`v020b-aim-regression-red-20260715` 与 `v020b-residual-velocity-red-20260715` 均只因目标行为失败，最小修复后对应 GREEN。最终 BuildEditor `v020b-fixedpie-blocked-outcome-green-20260715` PASS。
- 用户单独批准固定无参 Editor-only `RunPIEPlayerSkillCheckpointBSmoke()`；Tool 只使用正式 AbilitySet/输入/GA/GE/Combat，在 PlayWorld 创建并清理 transient 目标与 WorldStatic 墙，不保存 Package、不接受任意参数。正式侧视朝向改为 Mesh Right 的 X/Z 投影；位移结束清除 RootMotion 残余速度；墙前安全终点在 Shadow 技能层按 Blocked 处理且不发布 Outcome。两个全新固定 B PIE 会话连续 PASS，结构化结果均为两个 Spec、四个键鼠/手柄场景、三种 Startup 取消和 RuntimeClean 全 true。
- 既有 Input/Ability PIE smoke 在独立会话 PASS；既有 Combat smoke 先如实记录隐藏窗口 0.3333 秒采样失败，随后使用 UE 官方 `-Unattended -RenderOffScreen` 无窗口模式原阈值复测 PASS，两个 0.10 秒硬直实测 0.1107/0.1108 秒。所有会话均 StopPIE；无地图或测试资产保存。
- 最终 `ProjectR.PlayerSkill` 5/5；Input/GAS/Combat/Ability/Save/Debug 原生回归为 3/3、4/4、4/4、5/5、5/5、12/12，Debug 使用冻结的 CombatGym `-game` 入口。十个 Blueprint warnings-as-errors PASS；全 `/Game` 注册 349 项中排除 54 个 ExternalObjects/Actors 后，295/295 可查询资产 Dirty=false；AutomationReport `v020b-final-report-20260715` 为 38/38 required PASS。
- Future Compatibility Review：C 只能在现有两项后追加 Thunder/Afterimage，且使用独立状态注册表；D 继续通过 Mobility/Mitigation 扩展；E 才创建 VFX/SFX、补齐六项并运行完整 58 项。v0.2.1-v0.4.4 仅消费稳定接口、Tag、事件和 PrimaryAssetId，不读取或保存 Burning Timer/EffectHandle/Target/Snapshot。
- 回滚顺序冻结为 DefaultAbilitySet 恢复空数组、Fire GA 清空 Burning 引用、经逐 Package 删除批准后删除 Burning GE，再反向撤销 C++/测试/文档并重跑 A 回归。`NetworkPIEReplication`、`PhysicalGamepad` 与 `HumanSkillFeelJudgment` 为 optional `NOT_RUN`，`ProjectRAuthoringToolExtension` 为 optional `PASS`，`Package = NOT_RUN`，GC 未执行。v0.2.0-B 正式实施提交为 `3fa0ddcdec141945a2b5537308aaeb97276f1c83`（`v0.2.0-B Implement ShadowThrust and FireSlash gameplay`），且该实施提交已与 `origin/main` 同步；公共 v0.2.0 未推进。

# 2026-07-16 - v0.2.0-C 雷鸣落点与残影闪避（Completed）

- 在 `CURRENT_VERSION.md` 与 VersionIndex 继续保持 v0.2.0/In Progress 的前提下完成内部检查点 C；公共版本未推进。正式实施提交为 `9cd0bc683cbd4530fb1a4fd8f368564d166989fe`（`v0.2.0-C Implement ThunderDrop and AfterimageDodge`），且该实施提交已与 `origin/main` 同步。
- C 实现 ThunderDrop 的合法 GroundArea/450cm fallback、0.60s 延迟、220cm AoE、一次伤害、0.75s Stunned 和失效/World Cleanup；AfterimageDodge 实现 300cm/0.18s 平面位移、0.22s Invulnerable、单一残影、0.14s PerfectTiming、幂等消费与清理。所有伤害继续只走 `UPRCombatSubsystem`，没有修改 A/B 冻结公共合同。
- 实际 Package 变化为修改 `/Game/ProjectR/Abilities/DA_DefaultAbilitySet`、`/Game/ProjectR/Abilities/Skills/GA_Skill_ThunderDrop`、`/Game/ProjectR/Abilities/Skills/GA_Skill_AfterimageDodge`，以及创建 `/Game/ProjectR/Abilities/Effects/GE_State_Stunned`、`/Game/ProjectR/Abilities/Effects/GE_State_Invulnerable`。DefaultAbilitySet 当前严格为 ShadowThrust、FireSlash、ThunderDrop、AfterimageDodge 四项；未创建 VFX/SFX、未保存地图或 MCPTest。
- `c4d9303175c876b1d1b98897b415c1158e2913e9` 是 v0.2.0-C 固定无参 `RunPIEPlayerSkillCheckpointCSmoke()` 的 editor-only 自动化补充实现，仅修改 `Plugins/ProjectRAuthoringTools/Source/ProjectRAuthoringTools/Private/PRPlayerSkillAutomationToolset.cpp` 与 `Plugins/ProjectRAuthoringTools/Source/ProjectRAuthoringTools/Public/PRPlayerSkillAutomationToolset.h`；不包含文档转换。其提交信息 `docs: close v0.2.0-C and start v0.2.0-D` 与实际内容不符；该提交已推送至 `origin/main`，因此不改写历史。
- 完成门补验发现并关闭两个真实运行时边界：GroundArea 不能把合法 Pawn 的 Visibility 碰撞误判为 WorldStatic 阻挡；`MoveToForce` 在单帧超过 Duration 时必须归位至既有验证终点并作为 Completed，不能以越过终点误判 Blocked。补验提交为 `492b5cb227c4a0b673c7c5913267865a01df543a`（`v0.2.0-C Fix targeting obstruction and coarse-frame displacement`），且该提交已与 `origin/main` 同步。
- 最终补验 BuildEditor PASS；`ProjectR.PlayerSkill` 5/5，Input/GAS/Combat/Ability/Save/Debug 回归为 3/3、4/4、4/4、5/5、5/5、12/12（Combat 保留两条历史 Warning）。两个 GA、两个状态 GE 与三个 Player Blueprint warnings-as-errors 编译 PASS；重启回读 AbilitySet 的四项顺序和引用，相关 C Package、三个 Player Blueprint 与 `L_CombatGym` 均 Dirty=0。
- 固定无参 `RunPIEPlayerSkillCheckpointCSmoke()` 实际 PASS：Thunder 延迟/命中/Stunned/失效清理、Afterimage 位移/无敌/单残影/PerfectTiming/清理以及四个正式 Spec 均通过；既有固定 B PIE 回归 PASS。Tool 只调用正式受控 Ability/Input/Combat API，不保存 Package、不接收任意参数。
- 结构化补验 AutomationReport `v020c-supplement-final-report-20260716a` 为 15/15 required PASS。`NetworkPIEReplication`、`PhysicalGamepad`、`HumanSkillFeelJudgment` 与 `Package` 为 optional `NOT_RUN`，GC 未执行；没有必需人工步骤。Future Compatibility Review PASS：D 只能在四项后增量追加 Hook/Guard，v0.2.1–v0.4.4 继续只消费既有接口、Tag、事件和 PrimaryAssetId。

# 2026-07-16 - v0.2.0-D 向量钩索与反证护壁（Completed）

- 在 `CURRENT_VERSION.md` 与 VersionIndex 继续保持 v0.2.0/In Progress 的前提下完成内部检查点 D；正式实施提交为 `bb034143514394a9433e5cabcf9ba3ebbb35094e`（`v0.2.0-D Implement VectorHook and CounterProofWall`），且该实施提交已与 `origin/main` 同步；公共版本未推进。
- 实现 VectorHook 的 SingleTarget 预检、Commit 前 NoTarget/Obstructed、Light/Heavy/Anchored 受控 RootMotion 位移、120cm 停距、完成 Outcome 与 Commit 后失效清理；CounterProofWall 使用准确的 1.0s `GE_State_Guarding`、0.15s PerfectTiming、60°正面 Mitigation、多次阻断与全生命周期幂等清理。伤害继续只经过 `UPRCombatSubsystem`，Mitigation 保持早于 Invulnerable。
- Unreal MCP 创建 `GE_State_Guarding`，配置 CounterProofWall GA CDO 并将 DefaultAbilitySet 精确扩展为六项；没有 Save All、地图、MCPTest、VFX/SFX 或 Config 写入。D 固定无参 PIE 工具只使用 `L_CombatGym` 和固定正式键/Tag，临时 Light 目标启用既有 CharacterMovement 无控制器 Tick 以真实执行 RootMotionSource，不保存 Package。
- 最终 BuildEditor PASS；原生 `ProjectR.PlayerSkill` 5/5、Input 3/3、GAS 4/4、Combat 4/4、Ability 5/5、Save 5/5、Debug 12/12 全部 PASS。`RunPIEPlayerSkillCheckpointDSmoke()` 实际 PASS：六 Spec、Hook NoTarget/Obstructed/Light/Heavy/Anchored、Guard Press/Perfect/Multiple/Back/Release/Gamepad/Maximum、B/C 基线及运行时清理均为 true。
- 独立结构化 AutomationReport `v020d-final-report-20260716` 结果为 PASS；`Package`、`PhysicalGamepad`、`HumanSkillFeelJudgment` 为 optional `NOT_RUN`，GC 未执行。
- Future Compatibility Review PASS：D 不依赖具体敌人、Boss、HUD、QTE、Save 或 Debug 类型；v0.2.0-E 只追加表现、手感与完整 58 项报告，后续版本只消费稳定 Target/Mobility、AbilityOutcome/CombatEvent、DataAsset 与 AbilitySet Handle。没有必需人工步骤；Package、PhysicalGamepad、NetworkPIEReplication 与 HumanSkillFeelJudgment 保持不冒充 PASS。

# 2026-07-17 - v0.2.0-E 六技能整合、占位表现与手感验收（Completed）

- 公共 v0.2.0 已完成。正式实施提交为 `01abb8cc5931a9c75a2c70d67368b7327a6249c3`（`v0.2.0-E Add six P0 player skills`），且该实施提交已与 `origin/main` 同步；实施提交本身未开始 v0.2.1，后续版本转换另行处理。
- 新增 `UPRPlayerSkillComponent` 的通用、安全表现缓存和播放路径：从已授予 Skill DataAsset 异步预加载软 `VFX`/`SFX`，仅在本地非 Dedicated Server 播放附着 Niagara/Audio；激活不做同步加载。缺失/失败只记录一次受控 Warning 并跳过表现，不能影响能力 Commit、伤害、位移、Cost、Cooldown、状态或 AbilityOutcome。EndAbility、Avatar replacement、World Cleanup 与 Component EndPlay 释放表现组件和流式句柄，无永久 Tick。
- Unreal MCP 串行创建并精确保存六个 `/Game/ProjectR/VFX/Skills/VFX_*` 和六个 `/Game/ProjectR/Audio/Skills/SFX_*`，并仅为六个已有 `DA_Skill_*` 绑定软引用后精确保存。六个 GA warnings-as-errors 编译、全部 18 个 E 保存目标 Dirty=0，正常 Editor 重启后逐项回读引用。全版本唯一清单准确为 45 Create + 3 Modify；E 再保存的六个 DA 是 A 的 Create Package，不增加 Modify 计数。
- 新增固定无参 E 表现 smoke，以及仅供六 Spec 最终环境使用的 C/D 兼容回归入口；B/C/D 原有公共入口不改语义。E smoke、B smoke、E-C 和 E-D 回归在 `L_CombatGym` 真实 PIE PASS；所有会话 StopPIE，运行时无额外 Effect、Spec、Tag、Timer、Delegate、RootMotion、残影、表现组件或 transient Actor 泄漏。
- 新鲜自动化通过：`ProjectR.PlayerSkill` 5/5、Input 3/3、GAS 4/4、Combat 4/4、Ability 5/5、Save 5/5、Debug 12/12。`v020e-stage1-report-20260716` 如实记录人工项 NOT_RUN；用户在客观门均通过后按 `L_CombatGym` 六技能 runbook 实际验收并明确回复 PASS，最终 `Saved/AutomationReports/v020e-final-report-20260717/v020e-final-None/result.json` 为 58/58 required PASS。
- Known Issues 未出现新的真实状态变化；KI-019 的 `.uplugin` 依赖元数据 Warning 和既有粗帧隔离流程仍按既有条目保持，不以修改 Forbidden path 掩盖。GameplayTagTaxonomy 无真实 Tag 变化，未修改。`Package`、`PhysicalGamepad`、`NetworkPIEReplication` 与 GC 未运行，未被记为完成证据。
- Future Compatibility Review PASS：敌人/Boss 继续只实现稳定 Target/Combat/Mobility 和 Heavy/Anchored；HUD/QTE 只读 RuntimeState、AbilityOutcome/CombatEvent；v0.2.4 只替换 DataAsset 数值/表现软引用；Meta、QA、Shipping 只消费稳定 ID/Handle/固定输入。Save 和实时玩法不保存/依赖运行时 Handle、Target、Timer、Held Input、Delegate、RootMotion、Guarding、表现组件、网络或 LLM。

# 2026-07-17 - v0.2.1-A Enemy GAS、StateTree 与近战基础（Completed）

- 在 `CURRENT_VERSION.md` 继续保持 v0.2.1/In Progress 的前提下完成内部检查点 A；未开始 B，公共 v0.2.1 未关闭。
- 增加 Enemy 自持 Minimal ASC/AttributeSet、Prototype/Attack PrimaryDataAsset、固定 Registry 白名单、X/Z Plane Movement、原生 StateTree 节点、ServerTriggered 攻击 GA 与 12 项精确 Enemy Package。`ServerTriggered=3` 追加在既有 ActivationPolicy 后，保持旧枚举值、输入语义和玩家 AbilitySet 不变。
- 经单独授权，PlayerCharacter 仅实现 Combatant 转发适配：CombatantId 与 Damage GE 继续由 PlayerState 作为唯一事实源；没有复制 ASC、属性、Damage 或玩家生命周期状态。Enemy 不 include PlayerCharacter 或具体后续敌人/Boss。
- TDD RED 记录了缺失 Player pawn Combatant 适配和 140cm 攻击带与 75cm sweep 不一致；最小修复后 BuildEditor 通过。`DA_EnemyAttack_MeleeStrike` 只经固定无参 MCP 工具精确改为 140cm sweep，未使用 Save All。
- 验证：`ProjectR.Enemy` 4/4、Player pawn Combatant forwarding、BuildEditor、GE/GA/BP warnings-as-errors、正常 Editor 重启、12 个 Package Dirty=0、固定 `RunPIEEnemyCheckpointASmoke()` 与六技能 E smoke 均 PASS。Enemy PIE 结构化结果为 Health=100、AP=10、MoveSpeed=450、Melee CombatEvent=15、deathStopsBrain/runtimeClean=true；无 ProjectR Error、Ensure 或 Blueprint Runtime Error。
- Future Compatibility Review PASS：B 只新增 Projectile/Ranged Data，C/D 只新增 Shield/Elite Data 与同一稳定接口；v0.2.2 复用 Heavy/Anchored、v0.2.3 仅读 RuntimeState/CombatEvent、v0.3.2 只订阅稳定事件。Save 不持久化敌人、Spec、Effect、目标、Token、Timer、StateTree 或 UObject 引用。

# 2026-07-19 - v0.2.1-B RangedMinion 与安全投射物（Completed）

- 在 `CURRENT_VERSION.md` 继续保持 v0.2.1/In Progress 的前提下完成内部检查点 B；未开始 C，公共版本未关闭。
- 新增白名单 `RangedMinion`、RangedShot Data/GA/AbilitySet/Cooldown、Ranged Blueprint、Projectile Blueprint 与占位 VFX/SFX，共九项 Package；Registry 只追加 Ranged Entry，Melee Entry 保持不变，GameplayTag/Config 零增量。
- `APREnemyProjectile` 仅由 Authority 在 Active 阶段创建，以锁定 X/Z 方向、1200cm/s、1.5s 生命周期和 Attack Token 单次结算运行；所有伤害继续只经 `UPRCombatSubsystem`，并优先消费稳定 `IPRCombatAttackProxyInterface`，不依赖具体 Afterimage 类型。
- 经批准修复 `APRSkillDecoyActor` 的无碰撞 SceneRoot：SpawnActor 现在准确保留残影起点，不改变公开接口、Tag、输入、伤害、Timer 或消费幂等性。TDD RED 仅因缺根组件导致 SpawnLocation 断言失败，GREEN 后 `ProjectR.PlayerSkill.Runtime.DamageBurningAndDefense` 通过。
- 实测 BuildEditor、`ProjectR.Enemy` 5/5、`ProjectR.PlayerSkill` 5/5 与固定 `RunPIEEnemyCheckpointBSmoke()` 均 PASS；PIE 确认一次 12 点投射物事件、一次 Afterimage proxy 消费、无第二次伤害事件以及 runtime clean。

# 版本记录模板

```text
Date / Version：
Goal：
Changed files：
UE Packages：
Public contracts：
Commands and results：
Acceptance：
Future Compatibility Review：
Human judgment：
Known risks：
Commit：
```
