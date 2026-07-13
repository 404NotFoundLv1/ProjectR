---
title: "ProjectR Unreal MCP 能力与资产生产指南"
subtitle: "UE5.8 实测 Toolset、自动化分层、写入安全与验证"
version: "v1.0"
date: "2026-07-10"
---

# 已验证连接
ProjectR 的 Codex 配置指向 `http://127.0.0.1:8000/mcp`。v0.0.5 启动审计时该端口由打开 `ProjectR.uproject` 的 UnrealEditor 进程监听，协议协商为 `2025-11-25`，读取调用返回当前关卡 `/Game/ProjectR/Maps/L_MainMenu`、PIE=false、无打开或选中资产且地图未 Dirty。
## 当前暴露能力
Tool Search 模式对 Codex 暴露 list_toolsets、describe_toolset、call_tool 三个元工具；v0.0.5 再次逐 Toolset 审计，实测其后仍为 20 个 Toolset、261 个底层 Tool。
| Toolset | Tool 数 | 主要能力 |
|---|---:|---|
| AgentSkillToolset | 4 | UE AgentSkill 资产查询与创建 |
| EditorAppToolset | 21 | 视口、选择、截图、PIE、Content Browser |
| LogsToolset | 4 | 日志类别、过滤和详细度 |
| GameplayTagsToolset | 6 | 标签列表、详情、引用查询以及受控增删改；v0.0.2 仅使用只读工具 |
| ActorTools | 17 | Actor/组件、Transform、标签、父子关系 |
| AssetTools | 21 | 查找、依赖、复制、移动、保存、Dirty、文件读取 |
| BlueprintTools | 53 | 创建/父类/变量/函数/事件/节点/Pin/Graph DSL/编译 |
| CurveTableTools | 9 | CurveTable 创建和行/Key 编辑 |
| DataAssetTools | 1 | 按 UClass 创建 DataAsset |
| DataTableTools | 10 | Schema、导入、创建和行编辑 |
| MaterialTools | 22 | Material/Function/MPC、表达式和连线 |
| MaterialInstanceTools | 13 | 父材质和参数覆写 |
| ObjectTools | 6 | 类查询、属性列出/读取/设置/重置 |
| PrimitiveTools | 4 | Blueprint/Actor 基础几何组件 |
| SceneTools | 20 | 关卡加载、Actor 放置、Outliner、Level Instance |
| SkeletalMeshTools | 22 | 导入、骨骼、Socket、材质、PhysicsAsset |
| StaticMeshTools | 16 | 导入、LOD、碰撞、Nanite、材质 |
| StringTableTools | 8 | StringTable 创建和条目编辑 |
| ProgrammaticToolset | 2 | 受限 Python 串联已注册工具 |
| TextureTools | 2 | Texture2D 导入和尺寸查询 |

# 自动化能力分层
1. 先用当前通用 Toolset。
2. 任务需要时选择性启用 GAS、UMG、StateTree、AIModule、Automation、Animation、LiveCoding、Niagara、PCG；GameplayTagsToolset 已在 v0.0.2 启用。
3. 官方工具无法幂等完成时，Codex 创建 ProjectR editor-only Python/C++ Toolset。
4. 前三层不可行或需要主观/凭据/破坏性确认时才转人工。
# Blueprint 自动化
当前 BlueprintTools 可创建 Blueprint、设置父类、变量/函数/事件/Dispatcher、节点、Pin 和连线；Graph DSL 支持 event、fn、if/elif、for、while、switch 和多执行出口。与 ActorTools/ObjectTools 组合后，可添加组件和配置 CDO。每次修改必须分别报告创建、编译、重启加载和 PIE 状态。

v0.0.3 实测发现 Graph DSL 的事件前缀固定为英文 `AddEvent|`。若非英文 Editor 文化返回本地化 Node Type ID，必须先保存全部准确目标 Package、确认零 Dirty，再以运行时参数 `-culture=en` 重启同一 ProjectR Editor；重新执行 node discovery、写入、回读和 warnings-as-errors 编译，保存后以默认文化重启验证。不得把猜测 Type ID 写入 Graph，也不得修改工程文化配置绕过问题。

# World Partition 地图与 External Actor

- 普通 AssetTools Duplicate 不保证复制 World Partition 的 External Actors/External Objects；v0.0.3 使用官方 `WorldPartitionRenameDuplicateBuilder`，且 Editor 必须关闭。
- Builder 会处理既有目标 Package，因此每张地图执行前必须同时检查主 World、`__ExternalActors__` 和 `__ExternalObjects__` 目标子树均不存在；一次只复制一张，退出 0 后立刻枚举实际 Package、数量和哈希再继续。
- WorldSettings 属于 World 主 Package，不是 External Actor；修改 GameMode 后只保存准确地图 Package，不调用 `save_actor`。
- 新放置的 External Actor 在首次落盘前可能尚未被 AssetRegistry 识别，`SceneTools.save_actor` 会因此拒绝。此时可对 MCP 返回的准确 Actor object ref 调用单项 `AssetTools.save_assets`；禁止传空列表。保存后必须核对预期 External Actor 文件数量、路径和 SHA-256。
- 不使用普通文件 IO、二进制 Patch、Save All、Resave All 或未批准删除处理 External Package。
# GameplayTagsToolset 实测语义
v0.0.2 的 GameplayTagsToolset 含 `ListTags`、`GetTagInfo`、`FindReferencersByTag`、`AddTag`、`RemoveTag`、`RenameTag` 六个工具。只读验收显示 53 个 ini 显式标签和 6 个层级自动生成的父节点；计数时必须区分管理器节点与事实源条目。默认只调用前三个只读工具；增删改仍属于配置写入，必须有当前版本 Allowed path、碰撞检查、Redirect/ADR/兼容审查和明确操作清单。

# 当前未承诺的专用能力
完整 UMG Designer Tree、StateTree/BehaviorTree、AnimBlueprint 状态机、Niagara 和 Automation 的专用 Toolset 当前未启用。进入相关版本时先启用并重审计；重审计后仍有缺口才创建专用 Toolset。GASToolsets 由 v0.1.1 选择性启用，其边界见下文。
# MCP Operation Manifest
执行前记录目标 Package、只读/创建/修改类型、Toolset/Tool、碰撞与可编辑性结果、操作顺序、失败停止点、是否需要重启/批准。执行后记录实际 Package、保存、Dirty、Blueprint compile、引用验证、重启加载和 PIE。
# 安全规则
- 不通过 shell 或普通文件 IO 修改 `.uasset/.umap`。
- 写入串行；不让多个 Agent 同时写同一 Editor。
- 默认仅写 `/Game/ProjectR/`，测试写 `/Game/ProjectR/MCPTest/`。
- World Partition 测试地图只额外允许当前任务明确列出的 `/Game/__ExternalActors__/ProjectR/MCPTest/**` 与 `/Game/__ExternalObjects__/ProjectR/MCPTest/**`；生成哈希路径必须在每个写入阶段后枚举冻结。
- 未批准不覆盖、重命名、移动、删除、批量迁移或 Resave All。
- 写入前检查 exists/can_edit；写入后只使用非空精确列表调用 `save_assets`。空数组等价于 Save All，永久禁止。
- 不因 create 返回成功就声称资产有效。

# v0.0.4 自动化报告衔接

- v0.0.4 不启用 AutomationTestToolset、不依赖 8000 端口，也不执行 MCP 写入、资产保存、Editor restart 或 PIE；Build/Package 证据由 Shell/UAT 和 `Saved/AutomationReports` 提供。
- 后续 MCP 任务使用 `BuildScripts/AutomationReport.bat` 汇总已经实际取得的独立 check。报告入口不调用 MCP，也不把 create 返回值推断为 Blueprint compile、Package save、restart load 或 PIE 通过。
- v0.0.5 至少为资产创建、Blueprint compile、Manifest 精确保存、意外 Dirty=0、重启回载和 PIE 分别提供 check；未执行项必须写 `NOT_RUN`。
- `Saved/AutomationReports/<RunId>/<EntryPoint>-<Configuration>/` 是被忽略的本地证据目录，不是 UE Package 根，也不得作为 CleanGenerated Apply 的候选。
- v0.0.5 启动审计确认模板地图包含 65 个 External Actor 与 2 个 External Object Package，并已将两个准确测试 External roots 加入任务 Allowed paths；E-012/E-013 的 Save All 冲突改为“warnings-as-errors 编译通过、Manifest 非空精确保存成功、全量意外 Dirty=0 后重启”。该合同必须在首次 Package 写入前独立静态验证。

# 建议 v0.0.5 冒烟测试
1. 查询目标路径不存在。
2. 使用非抽象 `/Script/Engine.PrimaryAssetLabel` 创建测试 DataAsset，保持 Runtime Label、目录标签和显式资产集合为空/false，并验证类型、路径、保存、引用与重载；UE5.8 的 `/Script/Engine.PrimaryDataAsset` 标记为 Abstract，创建内存对象后 SavePackage 必然拒绝，不得直接实例化，也不为冒烟测试新增运行时数据类。
3. 创建 Actor Blueprint、添加组件、设置 CDO 与指向测试 DataAsset 的默认引用。
4. 写入简单 EventGraph，编译并保存。
5. 复制模板地图、放置 Actor、保存。
6. 检查 Dirty/日志/引用。
7. 重启 Editor 后重新加载。
8. PIE 并采集日志/截图。

# v0.0.5 精确保存与 Dirty 审计

- 主测试 Package 固定为 `BP_MCPAuthoringSmokeActor`、`DA_MCPAuthoringSmoke` 和 `L_MCPAuthoringSmoke`；地图复制使用 UE5.8 官方 World Partition Builder，不调用通用 AssetTools Duplicate。
- Blueprint 使用临时 Editor 参数 `-culture=en` 完成 DSL node discovery/write/read，精确保存且 Dirty=0 后再以默认文化重启验证；不修改工程文化配置。
- 新 External Actor 优先使用 `SceneTools.save_actor`；若 AssetRegistry 首次时序导致失败，只能对 MCP 返回的准确 Actor object ref 使用单项非空 `save_assets`。
- 全量 Dirty 门先对 `/Game` AssetRegistry inventory 中可查询资产逐项执行 `is_dirty`；不可浏览的 External Objects/Actors 由 Manifest 精确保存、物理 Package 清单与 SHA-256、以及正常关闭无 Save Content 提示共同验证。Actor object ref 不能直接作为磁盘 Package path 传给 `is_dirty`。
- 当前官方工具覆盖 Blueprint、具体 `PrimaryAssetLabel` DataAsset、Blueprint 组件、Graph、Actor、精确保存、重载和 PIE；地图复制由官方 Builder 覆盖，因此 v0.0.5 不创建 `ProjectRAuthoringTools`。若 compile/restart 后组件或 Graph 丢失，或 Dirty 门不能闭合，立即停止并记录缺口，不在同轮创建插件。
- v0.0.5 实测 `write_graph_dsl` 在 `-culture=en` 中写入、回读、warnings-as-errors 编译并精确保存成功。默认文化重启后的整图 DSL 反编译曾返回空，随后经用户批准使用现有官方 node-level 只读接口复核，确认 EventGraph 实际完整持久化：一个 BeginPlay 连接一个 PrintString，Pin 值和日志字符串均正确；根因是本地化 Node Type ID 导致整图 DSL 的假阴性，而非 Graph 丢失。默认文化重载与 PIE 均 PASS，因此关闭 KI-012，不创建插件。
- `AssetTools.find_assets` 会列出非 browsable 的 `__ExternalObjects__`，但 `is_dirty` 对这些路径明确返回 Asset does not exist；External Actor 哈希 Package 同样不能作为普通 browsable asset 查询。Dirty 门因此由可查询 AssetRegistry 条目逐项 `is_dirty`、Manifest 对象精确保存、External Package SHA-256 和关闭 Editor 无 Save Content 提示共同闭合，不能对 Actor object ref 的 `is_dirty=true` 作结论，因为实现会把 object ref 当磁盘 Package path 比较而产生假阳性。

# v0.1.0 Enhanced Input 与 PIE 输入注入

- 官方 DataAsset/Object/Blueprint/Asset Tool 已实际创建、配置、回读、编译和精确保存 InputAction、InputMappingContext、InputConfig 与 Controller CDO；正式 IMC 的嵌套 Mapping、Instanced `InputModifierNegate`、GameplayTag Struct 和对象引用数组均不需要自定义资产 Tool。
- 现有官方 20/261 Tool 没有键、轴或手柄注入入口。经用户单独批准，`Plugins/ProjectRAuthoringTools` 只注册一个 Editor-only `RunPIEInputSmoke`；重启后为 21 Toolset/262 Tool。该工具不得扩展为资产保存、插件配置或任意代码执行入口。
- Project Toolset 必须在 `PostEngineInit` 注册；`Default` 阶段 ToolsetRegistrySubsystem 尚不可用。`FInputKeyEventArgs::CreateSimulated` 用于 `UGameViewportClient::InputKey` 时必须显式传入 `UGameViewportClient::GetGameViewport()` 与有效 InputDevice；默认 null Viewport 会在 UE5.8 `RemapControllerInput` 中被解引用。
- 输入 Runner 使用异步 Ticker 串行执行固定序列，并在成功、失败、PIE 中断和析构路径释放全部键与轴。结果必须分别报告 Context、X 位移、两种跳跃、空中反向窗口的起止速度与位移、Y 漂移、Mesh yaw、Actor yaw 和相机 Transform；空中反向采样起止均须处于 Falling，0.1 秒位移必须进入输入方向。语义钩子次数另由新时间窗日志验证。
- Editor 低帧率下 CoreTicker 可能在同帧早于 World Timer 采样；不得把固定 0.30 秒的中间 yaw 当成 Timer 失败。标准 Runner 在完整序列结束时检查最终左右朝向；0.12 秒平滑曲线使用单独 RED→GREEN 诊断和人工手感门，避免为通过测试而改变运行时插值。
- v0.1.0 的保存 Manifest 为八个新输入 Package 加 `BP_PlayerController`；`BP_PlayerCharacter` 只有实际 Dirty 时才能加入。首次保存前完整回读，保存后全量可查询 Dirty=0，再正常重启回载。禁止空列表、Save All、模板/Variant/MCPTest 引用和地图保存。

# v0.1.1 GASToolsets 与默认 GameplayEffect

- UE5.8 GASToolsets 静态源码包含 AttributeSetToolset 2 个只读工具、AbilitySystemInspectorToolset 4 个只读工具和 GameplayCueToolset 8 个工具。GameplayCueToolset 含创建 Cue、增删 GameplayCue Tag 和执行 Cue 等操作，不能把整个插件描述为只读；v0.1.1 只批准调用前两个 Toolset 的 6 个只读工具。
- GASToolsets 不提供 GameplayEffect 创建或 Modifier Authoring。`BlueprintTools.create` 可创建 GameplayEffect Blueprint，ObjectTools Schema 可表达 `FGameplayModifierInfo`、Attribute、Override 和 ScalableFloat。
- GE 创建、11 项嵌套 Modifier 设置和 `get_properties` 精确回读必须放入一次 `ProgrammaticToolset.execute_tool_script`。该入口成功时提交事务，脚本异常/取消时自动撤销自身外层事务；任一回读不一致必须抛错并禁止保存。
- `GE_DefaultAttributes` 的默认精确保存清单只包含自身和 `BP_PlayerState`；`BP_PlayerCharacter` 仅在原生父类变化导致实际 Dirty 时追加，`BP_PlayerController` 永不保存。禁止空列表、Save All 和扩大到整个 Effects/Blueprints 目录。
- Editor 重启前必须完成 GE/三个 Player Blueprint warnings-as-errors 编译、Manifest 精确保存和全量可查询资产 Dirty=0。重启后再用 AttributeSetToolset、AbilitySystemInspectorToolset、ObjectTools 和 BlueprintTools 分别验证 Schema、运行时值、CDO、父类与引用。
- UE5.8 Instant GE 的成功 Handle `IsValid()` 为 false、`WasSuccessfullyApplied()` 为 true；自动化和运行时代码必须使用后者，避免默认 GE 被重复应用。
