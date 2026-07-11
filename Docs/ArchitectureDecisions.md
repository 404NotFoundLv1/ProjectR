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
