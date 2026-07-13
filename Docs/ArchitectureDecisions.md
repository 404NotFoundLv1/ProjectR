---
title: "ProjectR Architecture Decisions"
subtitle: "不可逆或跨版本决策记录"
version: "v1.0"
date: "2026-07-10"
---

# ADR-001 - CodexReady 文档体系

**状态**：Accepted。  
**决策**：设计案为产品事实源；架构图/接口合同管理跨版本关系；路线图管理公共版本；CURRENT_VERSION + 独立任务合同管理当前执行。Markdown 是编辑事实源，PDF 是归档。

# ADR-002 - 类型与资产命名

**状态**：Accepted。  
**决策**：APR/UPR/FPR/EPR；文件 PR；资产 BP/WBP/DA/GE/GA/VFX/SFX/L/IA/IMC。模板类先迁移引用，不直接破坏性重命名。

# ADR-003 - 运行时确定性与 LLM 边界

**状态**：Accepted。  
**决策**：战斗、伤害、敌人、QTE、法令效果和存档本地执行。LLM 仅产生白名单结构化意图和表达，经过 Validator 并有 Mock/离线回退。

# ADR-004 - CombatEvent 作为跨系统事件面

**状态**：Accepted。  
**决策**：QTE、HUD、画像、AI 台词和统计消费 CombatEvent；Combat 不依赖这些具体系统。

# ADR-005 - 版本化 SaveGame

**状态**：Accepted。  
**决策**：从首次实现开始包含 SaveVersion 和迁移器；Steam Cloud 不建立第二套 Schema。

# ADR-006 - Unreal MCP 自动化优先

**状态**：Accepted。  
**决策**：Editor 机械工作默认由 Codex MCP 完成；能力顺序为通用 Toolset、官方专用 Toolset、ProjectR Toolset、人工例外。二进制只由 Editor API 写入，操作串行并完整验证。

# ADR-007 - 公共版本与内部微任务

**状态**：Accepted。  
**决策**：保留 v5 公共版本编号并新增 v0.0.5；过大版本拆为不发布的字母微任务，每个微任务可编译回滚。

# ADR-008 - Git 基线与 UE Package 使用 LFS

**状态**：Accepted。
**上下文**：首次索引误含 Binaries、Intermediate、Saved、DerivedDataCache、IDE 数据库和临时渲染产物；Content 中 749 个 `.uasset` 与 4 个 `.umap` 尚未配置 LFS。
**选项**：继续普通 Git Blob；忽略 Content；只对 UE Package 启用 LFS。
**决策**：忽略所有可再生生成目录和本地客户端状态；保留工程、Source、Content、Config、Docs、BuildScripts、`.mcp.json` 与 `.vsconfig`；仅让 `.uasset`、`.umap` 走 Git LFS。
**后果**：首次基线不携带本机缓存，UE Package 路径与引用不变；远端上传仍需单独验证 LFS 认证和配额。
**影响版本/合同**：v0.0.0 建立仓库存储边界；v0.0.1 消费稳定目录和构建入口。
**迁移/回滚**：清理前用 `git write-tree` 记录索引树；需要时用不带 `-u` 的 `git read-tree <tree>` 只恢复索引。禁止以 `git clean` 或硬重置回滚。
**验证**：保护目录不在索引；全部 753 个 UE Package 由 `git lfs ls-files` 列出；工作树文件零丢失。

# ADR-009 - 文档根目录大小写冻结为 Docs

**状态**：Accepted。
**上下文**：Windows 磁盘目录实际为 `docs`，而 AGENTS、CURRENT_VERSION 和版本合同冻结路径为 `Docs/**`；大小写敏感环境会把二者视为不同目录。
**选项**：保留偏差并记录风险；修改全部合同为小写；在首次提交前只修正目录大小写。
**决策**：经用户明确批准，在首次提交前通过工作区内临时名执行 `docs` → `Docs`，不改变文件内容和内部相对路径。
**后果**：Windows、Linux、CI 和下游 v0.0.1 使用同一公共目录名；不存在已发布 Git 历史迁移。
**影响版本/合同**：v0.0.0 目录稳定合同及 v0.0.1 的 `Docs/**` Allowed paths。
**迁移/回滚**：移动前后核对 22 份文档的聚合 SHA-256；实施中止时可经同样的临时名恢复原大小写。
**验证**：实际目录名严格等于 `Docs`，文档数量和内容哈希不变，Git 索引只出现 `Docs/`。

# ADR-010 - Core DeveloperSettings 使用最小 C++ 配置契约

**状态**：Accepted。
**上下文**：v0.0.1 需要为 Debug、MockDirector 和 Steam 提供长期稳定的功能开关，但对应业务系统尚未进入实现版本；提前暴露 Blueprint API 或引用未来类型会扩大公共合同并造成反向依赖。
**选项**：只写文档；创建 Blueprint 可读设置；创建只由 C++ 和 Project Settings 使用的最小配置类。
**决策**：在 Core 创建 `UPRDeveloperSettings`，使用 `Config=Game, DefaultConfig`，自动注册为 `Game > ProjectR`。三个 Config 布尔值固定为 `bEnableDebugFeatures=false`、`bUseMockDirector=true`、`bEnableSteamFeatures=false`；不暴露 Blueprint API，不实现被开关控制的系统。由于导出的公共 Header 直接继承 `UDeveloperSettings`，`ProjectR.Build.cs` 经用户明确批准在 `PublicDependencyModuleNames` 声明 `DeveloperSettings`。
**后果**：未来 C++ 消费者通过 `GetDefault<UPRDeveloperSettings>()` 读取确定性配置；Mock 默认保持离线回退，Steam 与 Debug 默认关闭。Debug 开关不替代 Shipping 安全边界。
**影响版本/合同**：v0.0.1 建立 Core 配置入口；v0.0.2 可独立接入模块和 GameplayTags，不依赖具体 Director、Steam 或 Debug 类型。
**迁移/回滚**：当前没有 Config、Blueprint、Save 或资产消费者；需要回滚时只删除本轮类和反向撤销文档 diff，不修改 UE Package。
**验证**：ProjectREditor Build 成功；Editor 重启后 MCP 能反射类和 CDO；Project Settings 面板显示准确分组、名称和默认值。

# ADR-011 - GameplayTags 使用单一 ini 事实源与分层模块下限

**状态**：Accepted。

**上下文**：v0.0.2 需要为后续 Game Framework、GAS、Companion、QTE、Director 和 Roguelike 建立稳定标签面，同时不得提前实现 Ability、ASC、AttributeSet、GameplayEffect 或内容资产。重复使用 Native Tag、Blueprint 或多个 ini 定义会造成事实源漂移；把编辑器工具模块带入 Runtime 会污染依赖边界。

**选项**：使用 Native GameplayTag；在 Blueprint/资产中按需创建；以 `DefaultGameplayTags.ini` 为唯一事实源并提供受限 C++ getter。

**决策**：在 `Config/DefaultGameplayTags.ini` 按完整名称字典序定义 53 个带非空注释的标签，开启 Config 导入和无效标签警告，关闭 Fast/Dynamic Replication，不添加 Redirect。`UPRTagLibrary` 只提供 16 个 checked、缓存的 C++ 静态 getter，不暴露 Blueprint API 或通用字符串查询。Runtime 启用 `GameplayAbilities`，Editor-only 启用实验性 `GameplayTagsToolset`，不启用 `GASToolsets`。`GameplayAbilities`、`GameplayTags`、`GameplayTasks`、`EnhancedInput`、`UMG`、`AIModule`、`NavigationSystem` 属于 Public 模块下限；`HTTP`、`Json`、`JsonUtilities`、`AudioMixer` 保持 Private。

**后果**：后续版本可直接消费稳定 Tag 与模块入口，内容标签仍保持数据驱动；Editor Toolset 不成为运行时依赖。实验性 Toolset 的 Schema/API 变化继续按 KI-006 审计。

**影响版本/合同**：v0.0.2 冻结 GameplayTag taxonomy、module dependency floor 和 `UPRTagLibrary`；v0.0.3 可创建 Game Framework，v0.1.1 可接入 ASC/AttributeSet/GAS，无需重写本版本。

**迁移/回滚**：后续只能增量加子标签；重命名或删除必须提供 Redirect、ADR、消费者清单和兼容测试。v0.0.2 回滚只反向撤销 ini/C++/uproject/Build.cs 与文档 diff，不修改 UE Package。

**验证**：静态核对 53 个标签、9 个根、16 个 getter 和模块唯一性；ProjectREditor Build；Editor 重启后使用 GameplayTagsToolset、ObjectTools 与设置反射进行只读验证。

# ADR-012 - 正式 Game Framework、稳定地图 ID 与 World Partition 基线

**状态**：Accepted。

**上下文**：v0.0.3 需要建立可由 v0.0.4 打包和后续玩法版本稳定消费的地图与生命周期入口，同时现有四张模板地图采用 World Partition/External Actors，通用 MCP Duplicate 无法完整迁移其外部 Package。直接继承模板或 Variant Framework 会把演示实现冻结为正式架构依赖。

**选项**：继续使用模板 `AProjectR*`；为每张图各自硬编码 OpenLevel；创建独立 APR/UPR Framework、枚举式旅行入口并通过官方 World Partition Builder 复制灰盒地图。

**决策**：创建独立的正式 APR/UPR Framework 和 `EPRMapId`，由 `UPRGameInstance::OpenMap` 统一映射五张 Soft World；不依赖模板/Variant Framework。五张地图由 UE5.8 `WorldPartitionRenameDuplicateBuilder` 在逐目标碰撞门后串行创建，并允许其准确 ExternalActors/ExternalObjects 子树。`APRPlayerCharacter` 只提供位置 `(0,600,100)`、旋转 `(0,-90,0)`、FOV 60 的固定侧视相机，不提前实现 v0.1.0 输入和移动。

**后果**：v0.0.4 获得稳定地图和默认启动入口；v0.1.0 可在正式 Character 上增量实现 2.5D 操作。模板和 Variant 继续保留为参考，灰盒环境与 Quinn 外观可临时复用，但正式 Framework 不引用其类。

**影响版本/合同**：v0.0.3 冻结 Framework 生命周期、五个地图 ID/路径和 `OpenMap` Blueprint API；v0.0.4 只消费这些入口，不修改其语义。

**迁移/回滚**：文本/C++ 可反向撤销本轮 Patch；DefaultEngine 可恢复模板默认值。新增 World Partition Package 不自动删除，失败时先撤回引用并输出准确 Package 清单，再取得逐项破坏性删除批准。

**验证**：正式 Build；13 个主 Package 与 336 个 External Package 精确落盘；八个 Blueprint warnings-as-errors 编译；Editor 重启回载；五图 GameMode/PlayerStart/Dirty/依赖审计；MainMenu 旅行；四图 PIE；四图人工相机构图 PASS。

# ADR-013 - 统一构建入口、Config 地图事实源与实际结果报告

**状态**：Accepted。

**上下文**：v0.0.4 需要为 v0.0.5 和后续版本提供可从任意工作目录重复调用的 Editor Build、Win64 Package、生成目录清理和 QA 记录入口。当前机器同时存在多个 UE 次版本，UE/VS 工具不在 PATH；真实 Package 还必须与 v0.0.3 冻结的五张正式地图保持一致，并避免把预览或命令生成写成执行通过。

**选项**：只在文档中保存人工命令；在各批处理文件中复制独立逻辑；由稳定 `.bat` 入口调用同一 PowerShell 实现，并使用 Config 地图事实源和统一报告 Schema。

**决策**：公共入口为 `BuildEditor.bat`、`PackageDevelopment.bat`、`CleanGenerated.bat` 和 `AutomationReport.bat`，`PackageDev.bat` 仅作兼容转发。入口要求绝对 Project 路径，按显式参数、环境变量、精确版本自动发现解析 UE。五张 `MapsToCook` 写入 `DefaultGame.ini` 并作为打包唯一事实源。每次运行在被忽略的 `Saved/AutomationReports` 中保存脱敏命令、完整日志和 SchemaVersion 1 实际结果；清理默认只预览，Apply 需双重确认和逐路径保护门。

**后果**：v0.0.5 可原样复用 Build、Package 和独立 check 报告入口；测试资产不会因打包脚本中的第二份地图列表而进入正式包。统一包装器成为兼容表面，后续变更必须保留参数、退出码和报告字段语义。报告和包是本地审计产物，不进入 Git。

**影响版本/合同**：v0.0.4 建立构建、打包、清理和报告合同；v0.0.5 直接消费这些入口记录资产创建、Blueprint compile、精确保存、重启回载和 PIE。工程名、模块名、Targets、GameplayTags、Blueprint API、Save 和 UE Package 不变。

**迁移/回滚**：当前没有既有自动化消费者；回滚只反向撤销本轮 BuildScripts、BuildGuide、报告合同和五条 `MapsToCook`。不得用 `git clean`、硬重置或 CleanGenerated Apply 代替回滚；已生成的忽略目录保留用于审计。

**验证**：公共 CLI、WhatIf、清理预览、报告 Schema、退出码和路径门的完整合同测试及独立复审均已通过。最终 `ProjectREditor Win64 Development` Build `v004-final-build-19f4ed6c55e` 退出 0，模块 DLL 后置条件 PASS；Development Package `v004-actual-package-19f4ece2a19` 的 UAT/包装器均退出 0，五张地图精确匹配，归档 EXE、Pak、UTOC、UCAS 均存在且非空。Shipping、Clean Apply、PIE 和 MCP 写入按本版本边界为 `NOT RUN`，不阻断接受本 ADR。

# ADR-014 - 官方 MCP Toolset、World Partition Builder 与精确保存基线

**状态**：Accepted。

**上下文**：v0.0.5 需要把当前 UE5.8 MCP 连接变为可审计资产生产能力，同时测试地图复制会产生 External Package，且旧草案的 Save All 指令违反工程的精确保存合同。当前 Toolset 必须先证明能覆盖 Blueprint、DataAsset、组件、Graph、Actor、保存、重载和 PIE，缺口被证明前不得创建自定义插件。

**选项**：使用 Save All 和通用 Duplicate；预先创建 ProjectR Authoring Toolset；复用 20/261 个官方工具并对 World Partition 单独使用官方 Builder。

**决策**：复用现有官方 Toolset；World Partition 只使用 `WorldPartitionRenameDuplicateBuilder`。DataAsset 冒烟使用非抽象 `/Script/Engine.PrimaryAssetLabel` 并保持标签规则为空/false；不直接实例化 UE5.8 中标记为 Abstract 的 `/Script/Engine.PrimaryDataAsset`。测试 Package 隔离在 `/Game/ProjectR/MCPTest/**` 及两个批准的对应 External roots。所有保存使用 Manifest 非空精确列表，Blueprint compile 与 Dirty=0 后才重启；`ProjectRAuthoringTools` 仅在可复现的官方能力缺口被记录并另行批准后创建。

**后果**：v0.1.0 可直接复用 Manifest、碰撞、精确保存、Dirty、重载、PIE、BuildEditor 和 AutomationReport，不依赖新的插件或运行时类型。测试资产不进入正式打包地图清单。

**影响版本/合同**：v0.0.5 建立 MCP 安全资产生产合同；v0.1.0 只消费工具和验证入口，不消费测试资产或新增玩法 API。工程名、模块名、Targets、GameplayTags、Save、正式地图和 Framework 不变。

**迁移/回滚**：文档可反向撤销；失败测试 Package 留在隔离根并输出准确路径/哈希，未获批准不自动删除。不得使用硬重置、Git clean、普通文件 IO、Save All、Resave All 或 GC。

**验证**：合同静态门在首次 Package 写入前通过。首次尝试证明抽象 `PrimaryDataAsset` 能创建内存对象但 SavePackage 明确拒绝；经用户批准只丢弃两个未落盘测试对象并改用具体 `PrimaryAssetLabel`。BuildEditor 退出 0；Builder 生成 1 Map、65 External Actor、2 External Object，放置 Actor 后 External Actor 为 66；Blueprint/DataAsset 创建、warnings-as-errors 编译、非空精确保存、默认文化重启回载、引用验证和 PIE 均分别 PASS。PIE 日志出现 `ProjectR MCP smoke PASS`，截图非黑屏且 Cube 可见，停止后 248 个可查询资产 Dirty 为 0；原有 1102 个 Package 的 SHA-256 全部不变。AutomationReport `v005-report-20260711T022722Z` 汇总 14/14 required checks 为 PASS。

# ADR-015 - 语义输入配置、Controller 生命周期与最小 PIE 输入 Toolset

**状态**：Accepted。

**上下文**：v0.1.0 需要建立可由 GAS、QTE 与重绑定系统增量消费的正式 Enhanced Input 层，并客观证明键鼠与手柄路径在真实 PIE 中生效。现有 20/261 个官方 Tool 覆盖 InputAction、IMC、DataAsset、Blueprint、保存、Dirty、重载和 PIE，但没有键、轴或手柄事件注入入口。

**选项**：只做人工输入验收；把 InputAction 直接绑定未来 Ability 类；创建覆盖资产生产的通用插件；保留官方资产工具并只增加一个窄范围 Editor-only 输入冒烟工具。

**决策**：正式输入通过 `UPRInputConfigDataAsset` 的 InputTag→InputAction 数据合同驱动；`APRPlayerController` 幂等拥有 Mapping Context 生命周期，`APRPlayerCharacter` 负责 X/Z 移动、生成 Y 平面约束、Mesh 左右朝向和 protected native 语义钩子。空中反向输入按用户确认采用动作游戏式即时转向：只把当前 X 速度以相同绝对值切换到输入方向，Z/Y、地面参数、最大速度和跳跃参数不变。实际移动与视觉朝向解耦：输入方向立即生效，Mesh 使用无永久 Tick 的 0.12 秒 Timer Ease-In-Out；快速反向从当前 yaw 重新插值。经用户单独批准创建无 Content 的 `ProjectRAuthoringTools`，只注册 `RunPIEInputSmoke`；工具在活跃 PIE 中注入固定键/轴序列、采样客观状态并释放全部输入，不提供资产保存或任意代码执行。

**后果**：v0.1.1 不必改写 Context 生命周期；v0.1.3 可覆写语义钩子转发 InputTag；v0.3.2 与 v0.8.2 可增量扩展配置。插件依赖 ProjectR 仅存在于 Editor 模块，Runtime/Shipping 不反向依赖 ToolsetRegistry。

**影响版本/合同**：冻结 `/Game/ProjectR/Input/` 下八个基线资产路径、`FPRTaggedInputAction`、`UPRInputConfigDataAsset`、Controller 只读获取入口和 Character native 钩子。不新增 GameplayTag、Save 字段、Ability、QTE、Config 或 Blueprint callable 玩法 API。

**迁移/回滚**：文本/C++/插件只反向撤销本轮准确 diff；Controller CDO 可精确恢复空引用。已保存的八个新 Package 未获逐项删除批准时只隔离并列出 Referencer，不自动删除。禁止硬重置、Git clean、Save All、Resave All 和 Fix Redirectors。

**验证**：TDD RED Build 因缺少 InputConfig 合同退出 6；实现后 Build 成功。插件首次在 `Default` 阶段注册时因 ToolsetRegistrySubsystem 尚未初始化而失败，改用 UE 官方 Toolset 相同的 `PostEngineInit` 后自动显示为第 21 个 Toolset/第 262 个 Tool。首次 PIE 注入暴露 `CreateSimulated` 默认空 Viewport 导致的访问冲突；按 UE5.8 API 传入实际 `FSceneViewport` 与默认 InputDevice 后复验 PASS。首次人工手感发现空中反向只改变 Mesh、实际速度未立即反转；新增 PIE 断言在旧实现上得到 Falling 状态但 `airReverseDeltaX≈0.000005 cm` 的预期 RED。修复后同会话两次及新 PIE 会话一次均 PASS：反向前速度约 `+435～469 cm/s`，下一采样变为 `-445～479 cm/s`，0.1 秒实际反向位移约 `-44～47 cm`，Y 漂移 `0 cm`；用户键鼠复验 PASS。平滑朝向先在旧瞬时实现得到 `leftFacingEarlyYaw=90` 的 RED，0.12 秒实现的诊断序列得到中间角 `-83.06`、最终 `90`，快速反向从 `62.22` 连续过渡到 `90` 的 GREEN。最终标准 PIE 为移动、双输入跳跃、平面、最终左右朝向、Actor/相机稳定 PASS；平滑主观手感仍由用户最终确认。

# ADR-016 - PlayerState-owned GAS、一次性默认属性与受控 GE Authoring

**状态**：Accepted。

**上下文**：v0.1.1 需要在 Pawn 替换和重新占有后保留战斗数值，同时为 v0.1.2、v0.1.3 和 v0.2.3 冻结稳定 ASC/AttributeSet 接口。UE5.8 的 Instant GameplayEffect 成功 Handle 不属于 Active Effect，`IsValid()` 为 false；通用 ObjectTools 支持嵌套 Modifier，但普通多次属性写入不具备整体原子性。

**选项**：ASC 由 Character 持有；PlayerState 持有 ASC 但每次 Pawn 初始化重放默认 GE；PlayerState 持有持久 ASC，并仅在 Authority 上成功应用默认 GE 一次。

**决策**：`APRPlayerState` 持有 replicated、Mixed-mode `UPRAbilitySystemComponent` 和 `UPRAttributeSet`，Owner 固定为 PlayerState、Avatar 指向当前 Character。默认 Instant GE 只在 Authority 成功执行一次，并以 `WasSuccessfullyApplied()` 判定；失败保留重试。11 项属性使用 RepNotify Always、统一原生事件、比例式 Max 调整和无业务语义 Clamp。GE 创建、嵌套 Modifier 设置和完整回读在单个 ProgrammaticToolset 事务内执行，之后才允许精确保存。

**后果**：Pawn 替换不丢失当前属性；v0.1.2 可增加 Damage/Death，v0.1.3 可在薄 ASC 上增加 AbilitySet/InputTag，v0.2.3 可单向订阅属性事件。GameplayEffect Authoring 失败不会留下猜测性已保存资产。

**影响版本/合同**：冻结 ASC 具体类型、11 项属性名、Owner/Avatar 生命周期、默认 GE 路径和统一属性事件。本版本不新增 Ability、Damage、CombatEvent、HUD、Save 或 GameplayTag。

**迁移/回滚**：先解除 BP PlayerState 的 GE 引用，再反向撤销 C++/uproject/文档。新 GE 未获逐项删除批准时只隔离并报告 Referencer；禁止硬重置、Git clean、Save All、Resave All 或普通文件 IO。

**验证**：v0.1.1 的 TDD RED 因目标 GAS Header 不存在按预期失败；生产实现后 BuildEditor 通过。`ProjectR.GAS` 四项原生自动化全部成功，覆盖属性 Schema/复制元数据、Clamp/Max 比例、默认 GE 及 Owner/Avatar/Pawn 替换生命周期。GASToolsets 重启后实测为 24 Toolset/276 Tool，11 项 Attribute 可回读。`GE_DefaultAttributes` 在单次 ProgrammaticToolset 事务中配置并断言 11 项 Modifier，随后只精确保存 GE 与 `BP_PlayerState`；三个 Player Blueprint 与 GE 均 warnings-as-errors 编译通过。Editor 重启回载、CombatGym PIE 属性检查、默认 GE 仅应用一次、v0.1.0 输入回归、截图、日志和 PIE 后 Dirty=0 全部取得实际 PASS。

# ADR 模板

```text
ID / Title：
Status：Proposed / Accepted / Superseded
Context：
Options：
Decision：
Consequences：
Affected versions/contracts：
Migration/rollback：
Verification：
```
