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
| `Chapter` | Roguelike/章节生命周期 | `Chapter.State.Active`, `Chapter.State.Available`, `Chapter.State.Completed`, `Chapter.State.Locked` |
| `Companion` | Companion/稳定 AI 身份 | `Companion.Axiom`, `Companion.Kindle`, `Companion.Null` |
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
