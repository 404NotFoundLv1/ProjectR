---
title: "ProjectR Unreal MCP 能力与资产生产指南"
subtitle: "UE5.8 实测 Toolset、自动化分层、写入安全与验证"
version: "v1.0"
date: "2026-07-10"
---

# 已验证连接
ProjectR 的 Codex 配置指向 `http://127.0.0.1:8000/mcp`。审计时该端口由打开 `ProjectR.uproject` 的 UnrealEditor 进程监听，读取调用返回当前关卡 `/Game/ThirdPerson/Lvl_ThirdPerson`。
## 当前暴露能力
Tool Search 模式对 Codex 暴露 list_toolsets、describe_toolset、call_tool 三个元工具；v0.0.2 重启实测其后是 20 个 Toolset、261 个底层 Tool。
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
GAS/GameplayEffect、完整 UMG Designer Tree、StateTree/BehaviorTree、AnimBlueprint 状态机、Niagara 和 Automation 的专用 Toolset 当前未启用。进入相关版本时先启用并重审计；重审计后仍有缺口才创建专用 Toolset。
# MCP Operation Manifest
执行前记录目标 Package、只读/创建/修改类型、Toolset/Tool、碰撞与可编辑性结果、操作顺序、失败停止点、是否需要重启/批准。执行后记录实际 Package、保存、Dirty、Blueprint compile、引用验证、重启加载和 PIE。
# 安全规则
- 不通过 shell 或普通文件 IO 修改 `.uasset/.umap`。
- 写入串行；不让多个 Agent 同时写同一 Editor。
- 默认仅写 `/Game/ProjectR/`，测试写 `/Game/ProjectR/MCPTest/`。
- 未批准不覆盖、重命名、移动、删除、批量迁移或 Resave All。
- 写入前检查 exists/can_edit；写入后精确 save_assets。
- 不因 create 返回成功就声称资产有效。

# v0.0.4 自动化报告衔接

- v0.0.4 不启用 AutomationTestToolset、不依赖 8000 端口，也不执行 MCP 写入、资产保存、Editor restart 或 PIE；Build/Package 证据由 Shell/UAT 和 `Saved/AutomationReports` 提供。
- 后续 MCP 任务使用 `BuildScripts/AutomationReport.bat` 汇总已经实际取得的独立 check。报告入口不调用 MCP，也不把 create 返回值推断为 Blueprint compile、Package save、restart load 或 PIE 通过。
- v0.0.5 至少为资产创建、Blueprint compile、Manifest 精确保存、意外 Dirty=0、重启回载和 PIE 分别提供 check；未执行项必须写 `NOT_RUN`。
- `Saved/AutomationReports/<RunId>/<EntryPoint>-<Configuration>/` 是被忽略的本地证据目录，不是 UE Package 根，也不得作为 CleanGenerated Apply 的候选。
- v0.0.5 草案尚有 KI-011/E-012/E-013：World Partition 测试地图的 External Package roots 未进入 Allowed paths，“Save All 后重启”与精确保存合同冲突。首次 MCP 写入前必须在 v0.0.5 启动审计修正并取得批准；v0.0.4 不提前修改该任务页或创建测试资产。

# 建议 v0.0.5 冒烟测试
1. 查询目标路径不存在。
2. 创建测试 DataAsset 并设置属性。
3. 创建 C++ 子类 Blueprint、添加组件、设置 CDO。
4. 写入简单 EventGraph，编译并保存。
5. 复制模板地图、放置 Actor、保存。
6. 检查 Dirty/日志/引用。
7. 重启 Editor 后重新加载。
8. PIE 并采集日志/截图。
