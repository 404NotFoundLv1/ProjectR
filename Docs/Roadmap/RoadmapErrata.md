---
title: "ProjectR 路线勘误与架构决策摘要"
subtitle: "从路线图 v5 到 v6 的冲突修正和不可逆决策"
version: "v1.0"
date: "2026-07-10"
---

当前新任务合同的路线事实源为
`E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/ProjectR_Codex_Executable_Roadmap_v6.md`；
同名 PDF 仅作归档核对。本文保留“v5 → v6”表述用于记录历史修正，不表示继续使用 v5 生成新合同。

# 已修正问题

| 编号 | v5 问题 | v6 决策 | 原因 |
|---|---|---|---|
| E-001 | UObject 类型同时出现 UPR 与 USP | 全部统一为 UPR | 避免反射类型、文件名和文档分裂 |
| E-002 | v0.0.0 假设尚无项目 | 以现有 UE5.8 模板工程为输入 | 与真实仓库状态一致 |
| E-003 | UE 二进制资产全部转人工 | 默认由 Codex 通过 Unreal MCP 创建/配置/保存 | v0.0.2 已验证 20 Toolset/261 Tool |
| E-004 | v0.1.5 提前要求法令/QTE/AI/Boss | 只注册命令框架，未来系统返回 NotAvailable | 不提前实现未来业务 |
| E-005 | v0.2.2 提前执行正式通关删号 | 只广播 OnPrototypeRunCompleted | 正式账号生命周期属于 v0.4.3 |
| E-006 | v0.2.0 一次实现六技能 | 拆为合同、两两技能和回归五个微任务 | 每步可编译和审查 |
| E-007 | v0.3.2 一次实现 12 QTE | 已由 E-024 取代：v0.3.2 作为单一完整版本实施 | 避免将同一 QTE 合同拆成不兼容半成品 |
| E-008 | v0.4.1 一次实现 12 法令 | 接口 + 三批法令 + 回归 | 先稳定效果合同 |
| E-009 | v0.4.2 同时做房间/路径/奖励/30+ 插件 | 五个微任务 | 避免房间和经济同时失控 |
| E-010 | 路线图重复粘贴完整提示词 | 改用 AGENTS + CURRENT_VERSION + 独立合同 | 减少上下文噪声 |
| E-011 | 没有跨版本消费者和冻结规则 | 增加 51 版本依赖矩阵和八项稳定合同 | 防止后续大规模返工 |

# 待 v0.0.5 启动审计落实的勘误

以下项目仅记录已发现的合同冲突，不代表已经修改 v0.0.5 合同或授权对应写入。

| 编号 | 草案缺口 | v0.0.5 启动审计必须落实 | 当前状态 |
|---|---|---|---|
| E-012 | v0.0.5 的 Allowed paths 只列出 `Content/ProjectR/MCPTest/**`；复制 World Partition 测试地图可能同时创建 `Content/__ExternalActors__/ProjectR/MCPTest/**` 和 `Content/__ExternalObjects__/ProjectR/MCPTest/**` | 只读审计确认源图为 65 个 External Actor 与 2 个 External Object；任务合同已增加两个准确 roots、阶段清单冻结、SHA-256 和失败隔离规则 | Resolved by v0.0.5 startup gate |
| E-013 | v0.0.5 的人工步骤写有“Save All 后重启”，与 `AGENTS.md`、CodexWorkflow 和 UnrealMCPAuthoring 的 Manifest 精确保存、禁止 Save All 合同冲突 | 已改为“Blueprint 编译通过、Manifest 非空精确保存成功、意外 Dirty Package 为 0 后重启”；Save All/空数组保存均明确禁止 | Resolved by v0.0.5 startup gate |

# v0.1.0 启动合同勘误

| 编号 | v5 问题 | v6 决策 | 原因 | 当前状态 |
|---|---|---|---|---|
| E-014 | v0.1.0 仍以 v0.0.4 为直接前置，并同时要求技能/QTE 输入绑定和输入调试显示 | 直接前置改为 v0.0.5；v0.1.0 只建立稳定 InputAction→InputTag 语义、2.5D 移动和固定侧视相机；技能输入延期至 v0.1.3，QTE 输入延期至 v0.3.2，调试证据使用日志、Enhanced Input 内置能力和 AutomationReport，不创建 v0.1.5 Debug UI | 避免输入层硬编码未来 Ability/QTE 类，并确保正式资产生产遵守 v0.0.5 MCP 安全合同 | Resolved by v0.1.0 task contract |

# v0.1.1 启动合同勘误

| 编号 | v5 问题 | v6 决策 | 原因 | 当前状态 |
|---|---|---|---|---|
| E-015 | v0.1.1 同时要求伤害/恢复测试和 HUD 委托，基础 ASC 类型却延后到 v0.1.3；属性 `Crit` 语义含糊，默认 GameplayEffect 资产创建仍转交人工 | v0.1.1 立即冻结薄 `UPRAbilitySystemComponent`、11 项属性 Schema 和统一原生属性变化事件，只验证初始化、通用 Clamp、Max 调整、复制元数据与重复初始化；`Crit` 明确为 0–1 的 `CritChance`，默认 GE 由 Codex 通过 Unreal MCP 创建。伤害语义延期至 v0.1.2，Ability 业务延期至 v0.1.3，HUD 消费延期至 v0.2.3 | 避免后续替换 ASC 类型、跨版本职责混入和不必要的人工资产操作，同时为伤害、技能与 HUD 冻结稳定数据合同 | Resolved by v0.1.1 task contract |

# v0.1.4 启动合同勘误

| 编号 | v5 问题 | v6 决策 | 原因 | 当前状态 |
|---|---|---|---|---|
| E-016 | v0.1.4 一次性要求 SaveVersion、关系、解锁、账号墓园、任务、设置、统计、主菜单 New/Continue 与开发清档，但这些业务模型和消费者尚未建立 | v0.1.4 只建立原生版本化 SaveGame、最小 `ProfileId` 分区、严格迁移框架和 A/B 双代 `PRSV` 物理存储；关系数据延后至 v0.3.0，受控开发清档延后至 v0.1.5，Account/Run/Graveyard 延后至 v0.4.3，Meta 解锁与货币延后至 v0.4.4，主菜单 New/Continue 延后至 v0.5.0，Memory/RunSummary 延后至 v0.5.2，设置延后至 v0.8.2，Steam Cloud 延后至 v0.8.4。未来分区只能通过递增 Schema 迁移加入 | 避免在所有者、数据范围和消费者尚未冻结时制造虚假持久化字段、UI 和删除语义，同时从第一版就保证损坏恢复、未来版本拒绝和云存档复用同一格式 | Resolved by v0.1.4 task contract |

# v0.1.5 启动合同勘误

| 编号 | v5 问题 | v6 决策 | 原因 | 当前状态 |
|---|---|---|---|---|
| E-017 | v0.1.5 将手工 `WBP_DebugMenu`、加资源、清 Cooldown、法令/QTE/AI/Boss 命令和开发清档混在同一版本，并只以“Shipping 不显示”描述隔离边界 | 采用用户批准的无 Content `ProjectRDebug` Runtime 插件和原生 Slate 面板；插件引用与模块描述双重 `TargetConfigurationDenyList=[Shipping]`。Save Debug 仅查询脱敏 RuntimeState；资源、Cooldown、Director、QTE、Companion 与 Boss 命令先返回结构化 `NotAvailable`，直到所属版本提供受控 API。现有 `LogProjectR` 保持兼容，只冻结 Core/Input/GAS/Ability/Combat/Save/Debug 类别，不提前创建 Director/Companion 业务或日志类别 | 使 Debug 不能成为第二套 Gameplay/Save 实现，并从 Shipping 构建、收据、模块、资产和快捷键行为上真实隔离；同时避免手工作业和未来类型占位 | Resolved by v0.1.5 task contract |

# v0.2.1 启动合同勘误

| 编号 | v5 问题 | v6 决策 | 原因 | 当前状态 |
|---|---|---|---|---|
| E-018 | v0.2.1 同时要求敌人死亡接入奖励/统计，并让精英硬直直接触发 QTE，但 Reward/房间结算、统计消费者和 QTE 尚未属于该版本 | v0.2.1 只通过既有 CombatEvent 发布伤害、死亡、ShieldBreak 与稳定 Enemy 身份事实；不创建 Reward、统计或 QTE 消费者。奖励/房间结算留给 v0.4.2，QTE 消费留给 v0.3.2；未来消费者只订阅事件与稳定 ID，不反向控制 Enemy 私有实现 | 防止敌人基础版本预建未冻结的奖励、统计和 QTE 业务，同时保留后续版本可验证的统一事件面 | Resolved by v0.2.1 task contract |

# v0.2.2 启动合同勘误

| 编号 | v5 问题 | v6 决策 | 原因 | 当前状态 |
|---|---|---|---|---|
| E-019 | v5 的审计者原型把 P1–P3、完整 HUD、反证碎片和正式通关删号混在同一 Boss 版本，且未区分 Director、QTE、Companion、Save 与章节所有权 | v0.2.2 只交付本地确定性的 Auditor Boss 纵切片：击败后一次性发布 `OnPrototypeRunCompleted` 和运行时 `CounterproofFragmentsAwarded=1`。不写入 Save/背包/货币/账号；正式 Account/Run/Graveyard 删除延后至 v0.4.3。P2 仅消费既有 `Rule.DistanceCorrection`/`Rule.RepetitionPenalty` 作为 Boss 内部确定性 ID，不创建 DirectorSubsystem、动态规则或 LLM。Boss UI 仅显示本战必需状态，不替代 v0.2.3 HUD；不实现 v0.3.1 Companion、v0.3.2 QTE 或 v0.7.0 审计者完整章节 | 保持 Boss 可演示，同时避免把未冻结的持久化、账号、Director、队友、QTE、章节与 HUD 业务伪装为纵切片能力；E-019 细化既有 E-005 的删号延后原则 | Active for v0.2.2 |

# v0.2.3 启动合同勘误

| 编号 | v5 问题 | v6 决策 | 原因 | 当前状态 |
|---|---|---|---|---|
| E-020 | v5 要求 HUD 显示“当前法令”“当前 AI”和 Boss 大招预警，并把手工 `WBP_HUD` 制作与 HUD 首轮混在同一版本；但 Director 属于 v0.4.0，Companion 数据/战斗 AI 属于 v0.3.0/v0.3.1，v0.2.2 没有独立 Boss Ultimate 合同 | v0.2.3 仅建立只读、事件驱动的战斗 HUD：Boss 战可显示既有 `FPRBossRuntimeState.ActiveRuleId`，并固定标注为“审计者本地规则”；显示已有 Attack Windup、PredictionShield 和 `PredictedAbilityTag`，不伪造 Director 法令、Companion/AI 状态或 Boss Ultimate。所有 WBP 由 Codex 通过 Unreal MCP/UMGToolSet 创建、配置、编译和精确保存。本版本为单一完整路线版本，不拆分 A–E、子版本或中间功能提交 | 避免把未来系统的权威状态、业务数据或人工机械操作前置到 HUD；同时让当前 Boss 与战斗事实获得可读、可验证的正式消费者 | Active for v0.2.3 |

# v0.2.4 启动合同勘误

| 编号 | v5 问题 | v6 决策 | 原因 | 当前状态 |
|---|---|---|---|---|
| E-021 | v5 建议建立 `Content/ProjectR/Data/Balance/*`，并以普通房间时长和敌人硬直作为首轮平衡目标；但当前数值已分散在正式 DataAsset、GameplayEffect 与 Character CDO，正式 Room/Encounter 属于 v0.4.2，Poise/Stagger/HitStop 系统尚未冻结 | v0.2.4 只在既有权威资产原位调整数值，不创建第二套运行时 Balance DataAsset 或 Subsystem。`Docs/BalanceNotes.md` 只记录基线、最终值、理由和证据，不是运行时权威。普通战斗 60–120 秒目标固定解释为 `L_CombatGym` 的白名单 2 Melee + 1 Ranged + 1 Shield 遭遇；不创建 Room、Encounter 或 Roguelike 系统。敌人硬直只允许调整既有命中反应或已存在的 Stunned 数据；不新建 Poise、Stagger、HitStop 或韧性系统。所有机械资产修改由 Codex 通过 Unreal MCP 完成。本版本为单一完整版本，不拆分子版本或中间功能提交 | 防止多重数值权威、提前引入未冻结玩法系统或将机械 Editor 工作转给用户，同时保留可追溯、可回滚的首轮平衡基线 | Active for v0.2.4 |

# v0.3.0 启动合同勘误

| 编号 | v5 问题 | v6 决策 | 原因 | 当前状态 |
|---|---|---|---|---|
| E-022 | v5 将 Companion 数据、支援 AI、QTE 和动态台词混入同一阶段，且未区分每轮主同步选择与长期关系持久化 | v0.3.0 只建立三名 Companion 的稳定身份/人设展示元数据、0–100 关系、唯一主同步运行时选择和 Save Schema 1→2 迁移；不创建 Actor、AI、Ability、QTE、对话或动态台词。主同步是每轮运行时状态，不写入 Save；长期关系才持久化。支援 Actor/压血不击杀延后至 v0.3.1，QTE 延后至 v0.3.2，动态台词/对话延后至 v0.3.3，濒死保护/分歧缓存延后至 v0.3.4。所有 DataAsset 机械操作由 Codex 经 Unreal MCP 完成；本版本为单一完整版本，不拆分子版本或中间功能提交 | 防止把没有所有者、输入、Actor 生命周期或持久化语义的未来玩法伪装为数据，保持 Schema 迁移、关系事件和未来消费者的稳定边界 | Resolved by v0.3.0 |
| E-023 | 原始路线把三种支援、压血不击杀、Boss 阶段边界和运行时 Pawn 所有权留为隐含实现细节 | v0.3.1 固定为 WorldSubsystem 拥有唯一 Primary Pawn；Axiom=Shield、Kindle=LightDamage、Null=ControlMark；Normal/Elite 分别保留 20%/35% Health，Auditor 依既有 66%/33% 阶段阈值停止直接支援伤害；SupportEvent 为 v0.3.2 的只读值事件 | 防止 Companion 反向控制 Save、Combat、Boss 私有阶段或形成第二套伤害/生命周期系统 | Active for v0.3.1 |
| E-024 | 原始路线建议将 12 个 QTE 按框架与三名 Companion 拆分；这会让关系、输入、结果与清理合同在同一版本内分裂 | v0.3.2 作为单一完整版本一次实现全部 12 个 P0 QTE；只建立一套确定性 Registry、状态机、有限输入与只读 Prompt UI，不创建中间功能提交 | 先冻结唯一 QTE 事件面和生命周期，避免 v0.3.3 对话、v0.3.4 救援、v0.4.0 Director 依赖多个不兼容的半成品 | Resolved by v0.3.2 |
| E-025 | 原始路线把动态交流、房间间隙和未来 LLM 对话混为同一阶段，并假设手工创建对话资产 | v0.3.3 只建立本地确定性短句与安全状态有限选择：安全状态来自现有战斗清除、Boss 完成或固定 Gym 预览；Room 延后 v0.4.2，LLM/记忆层延后 v0.5.2，资产由 Codex 经 MCP 生产 | 保持实时战斗离线确定性，避免提前创建 Room、LLM、自由文本、长期记忆或人工资产流程 | Active for v0.3.3 |
| E-026 | 原始路线让 v0.3.4 的濒死选择直接写入玩家画像和账号墓园，并假设已有回房、撤离、资源保留、奖励与技能树解锁 | v0.3.4 只在当前 Gym/Prototype 语义中以既有 `Combat.Event.Death` 和 `UPRCombatSubsystem::Revive` 实现一次性分歧；发布值型 `FPRDivergenceResult`，但不实现画像、墓园、Room/Reward、账号删除或解锁。画像留给 v0.4.0，Room/Reward 留给 v0.4.2，墓园/账号留给 v0.4.3，技能树资格留给 v0.4.4 | 防止在没有正式 RunState、房间、奖励和账户所有者时伪造持久化或地图业务，同时为下游提供稳定结果面 | Active for v0.3.4 |

# v0.4.0 启动合同勘误

| 编号 | 路线/接口缺口 | v0.4.0 决策 | 原因 | 当前状态 |
|---|---|---|---|---|
| E-027 | v6 要求 v0.4.0 建立 PlayerProfile、Director Rule 白名单和可替换 HTTP Provider，但正式 RunState/AccountRecord 属于 v0.4.3，十二条可执行法令属于 v0.4.1；仓库也没有正式远程端点、凭据或网络等待合同 | v0.4.0 的 PlayerProfile 只是在 `UGameInstanceSubsystem` 中保存的有界运行会话画像：成功 Create/Load Profile 开始新画像会话，普通地图旅行保持画像，不修改 Save Schema或持久化原始事件。现有 `Rule.CooperationAudit`、`Rule.DistanceCorrection`、`Rule.RepetitionPenalty`、`Rule.SurvivalProtocol` 构成本版本固定白名单；Applied Rule Handle 只表示经过 Validator 的运行时激活记录，不施加 GameplayEffect、Combat、Room、Reward 或资源效果。HTTP 仅保留可替换 Provider/Transport 边界，默认不联网、不配置端点或密钥；不可用、超时或非法响应立即使用确定性 Mock。正式十二条执行策略留给 v0.4.1，Room/Reward 留给 v0.4.2，RunSummary/AccountRecord及画像持久化留给 v0.4.3 | 保持实时玩法离线确定性，避免在所有者尚未建立时提前冻结 Save/Run 业务或把 Provider 输出直接变成执行权限，同时为后续法令、章节和远程服务保留稳定的 Request/Response/Validator 接口 | Active for v0.4.0 |
| E-028 | v6 将 v0.4.1 表述为 a–e 微任务，用户已授权此版本以单一公共合同、单一完成门和唯一功能实现提交交付 | a–e 仅可作为同一 v0.4.1 内部顺序，不创建子版本、子合同或中间功能提交。删除回声、最优路径、资源均衡、风险奖赏与服从测试在 v0.4.1 只能使用明确标注、可逆的会话内降级效果；Room、Reward、Account、经济和持久化仍由 v0.4.2/v0.4.3 所有 | 避免在未建立 Room/Reward/Account 所有者时伪造长期业务，同时保持 RuleId、Validator、Response 与 Applied Handle 对下游稳定 | Active for v0.4.1 |
| E-029 | v6/E-009 将 v0.4.2 表述为 a–e 微任务，并把房间、事件、奖励插件、商店兼容语义与未来 Run/经济扩展并列；用户要求 v0.4.2 以单一公共合同和唯一功能提交交付 | a–e 仅作为同一 v0.4.2 的内部实施顺序，不创建子版本、子合同或中间功能提交。v0.4.2 只建立 `UGameInstanceSubsystem` 所有的会话房间流程，不建立正式 RunState 或修改 Save Schema；RunSummary、Account 与账号生命周期留给 v0.4.3。默认 Registry 只注册 Combat、Elite、Boss、Safe 与 Event 房；MemoryCorridor、CurseProtocol、Commission 以稳定 RoomId 复用 `Room.Type.Event`，`Room.Type.Shop` 只保留既有 Schema/Tag 兼容，不创建商店或经济。当前精确创建 30 个会话内 RewardData 作为“30+ 插件基础”，v0.8.0 再扩充正式经济与 60+ 插件池。事件关系变化只调用既有 `UPRCompanionSubsystem::ApplyRelationshipDelta`，PlayerProfile 通过既有 Relationship 事件自然采样，不新增画像写入口。唯一功能提交为 `v0.4.2 Implement rooms, events, and reward plugins` | 保持 v0.4.1 Director、v0.4.3 Account/Save 与 v0.8.0 经济所有权边界，避免重复持久化、假商店、任意内容生成和多合同漂移 | Active for v0.4.2 |

# v0.4.3 启动合同勘误

| 编号 | 路线/接口缺口 | v0.4.3 决策 | 原因 | 当前状态 |
|---|---|---|---|---|
| E-030 | v6 将账号、运行结算、死亡/通关删号、墓园、反证碎片和未来复盘并列，并给出内部微任务建议；但 Profile/A-B 物理槽、长期关系、Meta、RealityHub UI 和长期记忆各有独立所有者版本 | v0.4.3 使用单一合同、单一完成门和唯一功能提交，不创建子版本、子合同或中间提交。“删号”只删除当前 Profile 内的临时账号记录，绝不删除 Profile、A/B 物理槽或长期关系。墓园、反证碎片和有界 `FPRRunSummary` 持久化；Meta 消费留给 v0.4.4，完整墓园/账号创建 UI 留给 v0.5.0，v0.5.2 只在既有 RunSummary 上增加安全记忆摘要。五种账号身份仅冻结稳定 ID 和展示元数据，不伪造商店价格、额外房间预览或 QTE 窗口效果。`ChallengeContinues` 保留账号和当前运行；`RescueEvacuationRequested` 与 `LeaveRunRequested` 结束临时账号并返回现实。成功加载时若发现无法恢复 Room 私有运行态的 ActiveAccount，则确定性归档为 `InterruptedRecovery`。只有完整 `FPRRoomSequenceCompleted` 结算增加 1 个反证碎片；死亡、撤离和中断不奖励 | 保持 Save 物理安全边界与上游稳定值合同，避免把未来 Meta/UI/Memory 业务提前塞入账号生命周期，同时使异常退出、保存失败和旅行失败均具有可重试、可验证语义 | Active for v0.4.3 |

# v0.4.4 启动合同勘误

| 编号 | 路线/接口缺口 | v0.4.4 决策 | 原因 | 当前状态 |
|---|---|---|---|---|
| E-031 | v6 将 Meta 成长的内部步骤与技能、AI、羁绊效果并列，但当前反证碎片归 v0.4.3 AccountPersistence，记忆碎片没有稳定来源；技能、QTE、濒死分歧、关系、RealityHub UI和长期记忆又各有冻结所有者 | v0.4.4 使用单一合同、单一完成门和唯一功能提交，不创建子版本、检查点或中间功能提交。`CounterproofFragments` 保留在 v0.4.3 的 AccountPersistence；RunState拥有奖励来源，ProgressionSubsystem只消费余额并拥有节点解锁，不迁移或重命名字段。Schema 4新增 `MemoryFragments`余额并安全默认0，但本版本不产出记忆碎片，正式节点的 MemoryFragment Cost均为0；未来来源必须通过独立合同和递增迁移。节点分为“下一轮实际效果”和“稳定资格”：Health、Energy与Companion支援只在下一轮生效；技能槽、技能强化、QTE、濒死保护、过载修复和羁绊节点只持久化资格，不修改当前六技能、既有QTE、濒死分歧或关系行为。v0.5.0负责RealityHub树界面，v0.5.1消费剧情/语音/连携资格，v0.5.2才建立记忆摘要及经批准的记忆碎片来源。活动运行期间的解锁不改变当前RunSnapshot | 保持货币、Save和上游玩法的单一所有权，避免凭空造币或以Meta版本重写已经发布的战斗、QTE、分歧和关系合同，同时为下游UI、剧情及记忆系统提供稳定资格面 | Active for v0.4.4 |
| E-032 | v6 的 v0.5.0 只列出现实据点五终端，当前仓库则没有 Hub UI、手动 Dialogue/Repair、QTE 训练启动或 Director 预报 dry-run API；用户要求一次完整交付且禁止改写冻结上游 | v0.5.0 使用单一合同、单一完成门和唯一功能提交。`UPRRealityHubSubsystem` 只编排既有公开 API：卡带槽经 Save/RunState 创建账号并进入网络；墓园只读；训练只往返固定 CombatGym；预报器使用本地值型画像与固定 Registry、绝不调用 Provider/读取 Response/Apply Rule；成长面板只经 Progression 查询/解锁 API。Dialogue、Repair、Quest、MemorySummary 与 DialogueProvider 仅提供明确不可执行的 typed extension point，个人任务/剧情留给 v0.5.1，记忆/Provider 内容留给 v0.5.2。Hub 不新增地图、Tag、Save Schema、网络或任意输入入口 | 在没有扩大上游所有权的前提下让五个终端均可操作、失败可见、可返回且可验证，并为后续 Quest、MemorySummary 和 Provider 留下稳定边界 | Active for v0.5.0 |
| E-033 | v6/产品设计要求六项个人任务及 QTE/台词奖励，但当前没有“低概率事件概率”“非最优救人路线”“撤离关键资源”“删除回声房取回碎片”等完整权威事实；产品文案中的“不退火线”“删除回声”也不属于现有十二项 P0 QTE。v0.5.0 只留空 Quest/DialogueProvider seam，Hub Root 尚未承载可执行的 Companion Quest 内容 | v0.5.1 使用单一合同、单一完成门和唯一功能提交。六项目标以既有公开值事实作最小稳定代理：Commission/Fulfill、DivergenceEvacuation、EliteAudit+低血+Boss、撤离+RewardId、Rule.DeleteEcho+反证碎片、Hub 实际展示五条墓园记录。QTE 奖励只投影到现有 `Axiom_CooperativeRefutation`、`Kindle_ReverseBurnRescue`、`Null_GarbageCollection` entitlement，不门控或修改 QTE。UI 只原位修改 `WBP_RealityHubRoot` 与 `WBP_RealityHubCompanionTerminal`，不改 `UPRRealityHubSubsystem` 所有权或公共输入。任务以 Schema 5 最小追加持久化，Memory/LLM/新 QTE/路线/删除回声房继续留给所属未来版本 | 在不伪造缺失玩法、不重写冻结上游的前提下完整交付第一章任务，并将产品语义映射为可测试、可迁移、可回滚的稳定事实 | Active for v0.5.1 |

# v0.5.2 启动合同勘误

| 编号 | 路线/接口缺口 | v0.5.2 决策 | 原因 | 当前状态 |
|---|---|---|---|---|
| E-034 | v6 将“每轮生成 RunSummary”、长期记忆、`memory_refs`/带关系 Delta 的对话选项、LLM 安全层和 MemoryFragments 来源并列；但 `FPRRunSummary` 已由 v0.4.3 RunState 所有，v0.5.1 只允许公开 Completed/entitlement/LineId 消费，Progression 是 MemoryFragments 余额所有者，仓库也没有正式网络端点、凭据或远程模型合同。用户要求 v0.5.2 一次完整交付，不拆分子版本或中间功能提交 | v0.5.2 使用单一合同、单一完成门和唯一功能提交。`UPRMemorySubsystem` 只从 `FPRAccountDeletedEvent.Record.Summary` 与公开值事件派生有界 `FPRMemorySummary`，不创建第二套 RunSummary、不修改 RunState。Provider 输出严格只允许 `scene`、`companion_id`、`emotion`、`summary`、`player_options` 五字段；`memory_refs` 留在本地请求上下文，Player options 只能是 Persona Registry 的固定 ID，不携带关系 Delta、Tag、效果或自由指令。每个首次成功持久化、非 `InterruptedRecovery` 且含合法 KeyEventId 的 SummaryId 原子奖励 1 个 MemoryFragment；Progression 继续拥有余额，只增加 MemorySubsystem friend 可调用的私有单枚入账投影，重复/失败/迁移旧档不补发。Schema 6 以同一 A/B 快照原子暂存 Memory 与 Progression，落盘成功前不发布摘要或余额。默认仅实现 typed Provider boundary、Validator、确定性本地 Mock 与离线 fallback，不创建 HTTP Provider、Transport、端点、API Key、远程模型或客户端凭据 | 保持 RunState、Quest、Progression、Save 和 Dialogue 的冻结所有权，避免重复摘要权威、任意经济入口与生成式选项越权，同时满足长期有界、离线可用、可验证和未来 Provider 可替换边界 | Active for v0.5.2 |

# 已接受的架构决策

1. 正式类使用 APR/UPR/FPR/EPR；现有模板类先迁移引用，不直接重命名二进制资产。
2. CombatEvent 是战斗向 UI、QTE、画像、对话和统计输出的唯一通用事件面。
3. SaveGame 从 v0.1.4 开始版本化；Steam Cloud 只包装同一份本地 Schema。
4. 运行时 LLM 只返回白名单结构化意图；发布客户端不保存正式模型密钥。
5. Unreal MCP 写操作串行，先碰撞检查，后精确保存/编译/验证。
6. Codex 自动化优先；人工只承担主观判断、外部凭据和确认性例外。
7. 公共版本保持 v5 主编号；过大范围使用不发布的内部微任务。
8. v0.0.2 只选择性启用 Editor-only `GameplayTagsToolset`；标签仍由 Config 管理，Runtime 不依赖 Editor 工具模块。

# 尚未执行的决策

- 是否启用 GASToolsets、UMGToolSet 等其他专用 Toolset，必须在使用它们的版本前选择性启用并重新审计。
- 是否删除模板 Variant，必须等正式 APR Game Framework 完成引用迁移和回归后决定。
- 是否创建 ProjectRAuthoringTools，必须先证明官方 Toolset 无法幂等完成具体任务。
