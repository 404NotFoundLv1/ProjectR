---
title: "ProjectR 路线勘误与架构决策摘要"
subtitle: "从路线图 v5 到 v6 的冲突修正和不可逆决策"
version: "v1.0"
date: "2026-07-10"
---

# 已修正问题

| 编号 | v5 问题 | v6 决策 | 原因 |
|---|---|---|---|
| E-001 | UObject 类型同时出现 UPR 与 USP | 全部统一为 UPR | 避免反射类型、文件名和文档分裂 |
| E-002 | v0.0.0 假设尚无项目 | 以现有 UE5.8 模板工程为输入 | 与真实仓库状态一致 |
| E-003 | UE 二进制资产全部转人工 | 默认由 Codex 通过 Unreal MCP 创建/配置/保存 | v0.0.2 已验证 20 Toolset/261 Tool |
| E-004 | v0.1.5 提前要求法令/QTE/AI/Boss | 只注册命令框架，未来系统返回 NotAvailable | 不提前实现未来业务 |
| E-005 | v0.2.2 提前执行正式通关删号 | 只广播 OnPrototypeRunCompleted | 正式账号生命周期属于 v0.4.3 |
| E-006 | v0.2.0 一次实现六技能 | 拆为合同、两两技能和回归五个微任务 | 每步可编译和审查 |
| E-007 | v0.3.2 一次实现 12 QTE | 框架、三名 AI 分批、回归 | 降低关系/画像耦合风险 |
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
