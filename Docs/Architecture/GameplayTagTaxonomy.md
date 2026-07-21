---
title: "ProjectR GameplayTag Taxonomy"
subtitle: "v0.0.2 冻结的标签命名、语义与演进规则"
version: "v1.0"
date: "2026-07-10"
---

# 事实源与所有权

`Config/DefaultGameplayTags.ini` 是 ProjectR GameplayTag 的唯一事实源，所有标签必须包含非空 `DevComment`。Core/Tags 拥有 taxonomy 与通用读取入口；业务模块只能消费或按本文件规则增量扩展，不能在 C++、Blueprint、DataAsset 或其他 ini 中复制一套同名定义。

`UPRTagLibrary` 只为高频基础设施标签提供 checked C++ getter。内容 ID 继续以 `FGameplayTag` 数据驱动，不为每个技能、伙伴、房间或奖励复制公共函数。

# 根命名空间

| Root | 所有者/用途 | v0.0.2 标签 |
|---|---|---|
| `Ability` | GAS 激活失败分类与隔离验证 Ability ID | `Ability.ActivateFail.CanActivate`, `Ability.ActivateFail.Cooldown`, `Ability.ActivateFail.Cost`, `Ability.ActivateFail.Networking`, `Ability.ActivateFail.TagsBlocked`, `Ability.ActivateFail.TagsMissing`, `Ability.Validation.OnInputTriggered`, `Ability.Validation.Passive`, `Ability.Validation.WhileInputActive` |
| `Chapter` | Roguelike/章节生命周期 | `Chapter.State.Active`, `Chapter.State.Available`, `Chapter.State.Completed`, `Chapter.State.Locked` |
| `Companion` | Companion/稳定 AI 身份 | `Companion.Axiom`, `Companion.Kindle`, `Companion.Null` |
| `Cooldown` | GAS Cooldown 阻断 Tag；当前仅含隔离验证项 | `Cooldown.Validation` |
| `Input` | Enhanced Input 到系统/能力的语义输入面 | `Input.Attack`, `Input.Dodge`, `Input.Execute`, `Input.Interact`, `Input.Jump`, `Input.Look`, `Input.Move` |
| `QTE` | QTE 类型和互斥结果 | `QTE.Result.Failure`, `QTE.Result.Rejected`, `QTE.Result.Success`, `QTE.Result.Timeout`, `QTE.Type.Attack`, `QTE.Type.Control`, `QTE.Type.Defense`, `QTE.Type.Emotional`, `QTE.Type.Rescue`, `QTE.Type.RuleCounter` |
| `Reward` | 奖励类型和稀有度 | `Reward.Rarity.Common`, `Reward.Rarity.Epic`, `Reward.Rarity.Legendary`, `Reward.Rarity.Rare`, `Reward.Type.Memory`, `Reward.Type.Relationship`, `Reward.Type.Resource`, `Reward.Type.SkillPlugin` |
| `Room` | Roguelike 房间分类 | `Room.Type.Boss`, `Room.Type.Combat`, `Room.Type.Elite`, `Room.Type.Event`, `Room.Type.Safe`, `Room.Type.Shop` |
| `Rule` | Director 可验证的本地法令 ID | `Rule.CooperationAudit`, `Rule.DistanceCorrection`, `Rule.RepetitionPenalty`, `Rule.SurvivalProtocol` |
| `Skill` | 技能稳定 ID，可作为未来 AbilityTag | `Skill.AfterimageDodge`, `Skill.CounterProofWall`, `Skill.FireSlash`, `Skill.ShadowThrust`, `Skill.ThunderDrop`, `Skill.VectorHook` |
| `State` | 通用角色状态 | `State.Alive`, `State.Dead`, `State.InCombat`, `State.Invulnerable`, `State.Stunned` |

# 语义约束

- `Input.*` 表示玩家意图，不硬编码具体 Ability 类；未来输入层只提交 InputTag。
- `Skill.*` 是稳定内容 ID，可作为未来 GAS AbilityTag，但 v0.0.2 不创建 Ability、ASC、AttributeSet 或 GameplayEffect。
- `Companion.*` 表示产品中的 AI 伙伴身份，不表示 Unreal AIController 类型或运行时网络 AI Provider。
- `QTE.Result.Failure` 表示已判定失败；`Rejected` 表示请求被策略拒绝；`Timeout` 表示响应窗口结束；三者不得折叠。
- `Rule.*` 只能引用本地注册且可验证的规则；LLM 未知 ID 不进入执行层。
- `Chapter.State.*`、`Room.Type.*`、`Reward.*` 是数据合同，不在本版本引入具体章节、房间或奖励资产。

# 演进与兼容

1. 后续版本只能增量添加有所有者、有消费者、有非空注释的子标签。
2. 发布后的标签不得静默重命名或删除。必要变更必须同时提供 GameplayTag Redirect、ADR、直接消费者清单和旧名称兼容测试。
3. 新根命名空间需要 Architecture Decision；不得用近义根复制既有语义。
4. TagLibrary 只在多个基础设施消费者确有需要时增加显式 getter；不得提供接收任意字符串的公共查询 API。
5. Blueprint API、Save 字段或资产引用一旦消费标签，迁移必须纳入相应版本合同，不能仅修改 ini。

# v0.0.2 验证基线

- 标签总数：53；根命名空间：9。
- `ImportTagsFromConfig=True`、`WarnOnInvalidTags=True`。
- `FastReplication=False`、`bDynamicReplication=False`。
- 无 Redirect；完整名称按字典序存储。
- `UPRTagLibrary` 暴露 16 个 C++ 静态 getter，不暴露 Blueprint API。

# v0.1.2 Combat 增量

- 显式标签总数由 53 增至 61；根命名空间由 9 增至 10。既有 53 个名称和 `DevComment` 不变，仍无 Redirect。
- 新根 `Combat` 由 Combat 所有，固定包含：
  - `Combat.Data.Damage`：`GE_Damage` 的唯一 SetByCaller 数值键。
  - `Combat.Event.Damage`、`Combat.Event.DamageRejected`、`Combat.Event.Death`、`Combat.Event.Revive`：统一事件类型。
  - `Combat.Response.HealthDamaged`、`Combat.Response.ShieldAbsorbed`、`Combat.Response.ShieldBroken`：单次事件的响应事实。
- `State.Alive`、`State.Dead`、`State.Invulnerable` 继续表达生命周期/拒绝状态，不复制到 `Combat.*`。
- 本版本不增加 Skill、QTE、Critical、DamageType、Heal 或测试 Tag。后续伤害/Ability 消费者使用现有字段和标签增量扩展，不能建立第二套同义 taxonomy。
- `UPRTagLibrary` 新增上述 8 个 Combat checked getter，总计 24 个公开 C++ 静态 getter；仍不提供任意字符串查询或 Blueprint API。

# v0.1.3 Ability 增量

- 显式标签总数由 61 增至 71；根命名空间由 10 增至 12。既有 61 个名称和 `DevComment` 不变，仍无 Redirect。
- 新根 `Ability` 固定包含六个 `Ability.ActivateFail.*` 与三个 `Ability.Validation.*`。失败 Tag 由 GAS DeveloperSettings、ASC 和生命周期事件消费；Validation Tag 只属于隔离验证 GA，不代表 v0.2.0 正式技能。
- 新根 `Cooldown` 当前只包含 `Cooldown.Validation`，由验证 Cooldown GE 授予；v0.2.0 的正式技能只能增量添加 `Cooldown.Skill.*`，不得复用验证 Tag。
- 本版本不新增 Input、Skill、QTE 或正式技能 Tag。正式 AbilitySpec 继续使用既有 `Input.Attack`/`Input.Dodge` 做验证路由，正式空 AbilitySet 不授予任何验证 Ability。
- `UPRTagLibrary` 仅为六个通用激活失败 Tag 新增 checked getter，总计 30 个；Validation/Cooldown 内容 Tag 保持数据驱动。

# v0.2.0-A PlayerSkill 公共合同增量

- 显式标签总数由 71 增至 94；根命名空间保持 12。既有 71 个名称、顺序和 `DevComment` 不变，仍无 Redirect。
- 新增 23 个稳定标签：五个 `Input.Skill.*`、六个 `Cooldown.Skill.*`、`Ability.ActivateFail.{NoTarget,Obstructed,InvalidMovement}`、`Ability.State.PlayerSkillActive`、`Combat.Event.AbilityOutcome`、五个 `Combat.Response.*` 以及 `State.{Burning,Guarding}`。
- `UPRTagLibrary` 为五个 Input、三个目标/位移失败、PlayerSkillActive、AbilityOutcome、五个 Response、Burning 与 Guarding 增加 17 个 checked getter，总计 47；六个 Cooldown Skill 标签保持数据驱动，不增加 getter。
- 五个新 Input Tag 只用于正式 Enhanced Input 到 ASC 的语义路由；AfterimageDodge 固定复用 `Input.Dodge`。`State.Burning`、`State.Guarding` 和 Decoy/Guard/PerfectTiming Response 在 A 只冻结名称，不实现对应 B–D 业务。
- `Combat.Response.*` 是 AbilityOutcome/Mitigation 的唯一响应标签范围；`Ability.ActivateFail.*` 是目标、LOS 与位移失败面。不得新增同义 Team/Faction、技能事件或测试标签。

# v0.2.1-A Enemy 基础合同增量

- 显式标签总数由 94 增至 118；根命名空间由 12 增至 13。新增根 `Enemy`，既有名称、顺序和 DevComment 不变，无 Redirect。
- 一次冻结 v0.2.1 的 24 个稳定 Tag：四个 `Enemy.Type.*`、四个 `Enemy.Attack.*`、四个 `Cooldown.Enemy.*`、11 个 `Enemy.Data.*` SetByCaller 键和 `Enemy.AI.Event.Reevaluate`。A 仅实际消费 MeleeMinion/MeleeStrike；其余原型身份、攻击和冷却 Tag 保持 DataAsset 驱动，以避免 B–D 重新打开 Tag 合同。
- `UPRTagLibrary` 仅为 11 个 Enemy Data 键与 Reevaluate 事件增加 checked getter，总计 59。原型、攻击及冷却 Tag 没有任意字符串公共查询器，也不增加临时测试、Boss、HUD、QTE、Reward 或 Faction Tag。

# v0.2.2 Auditor Boss 增量

- 显式 Tag 从 118 增至 126，根命名空间保持 13，`UPRTagLibrary` checked getter 从 59 增至 60。既有 118 项名称、顺序和 DevComment 均不变，无 Redirect。
- 新增且仅新增八项：`Enemy.Type.AuditorBoss`、`Enemy.Attack.AuditorSweep`、`Enemy.Attack.AuditorLockShot`、`Enemy.Attack.AuditorCounter`、`Cooldown.Enemy.AuditorSweep`、`Cooldown.Enemy.AuditorLockShot`、`Cooldown.Enemy.AuditorCounter`、`Combat.Response.PredictionBlocked`。
- `Enemy.*` 与 `Cooldown.Enemy.*` 继续由 Boss Prototype/Attack DataAsset 驱动，不增加 getter；仅 `Combat.Response.PredictionBlocked` 获得 checked getter，因为它是 Combat response 的稳定消费者事实。复用 `Rule.DistanceCorrection` 与 `Rule.RepetitionPenalty`，不创建 `Boss.Phase.*`、QTE、Director、Account/Delete、Reward 或 Inventory 占位 Tag。
