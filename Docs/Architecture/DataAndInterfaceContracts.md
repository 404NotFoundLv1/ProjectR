---
title: "ProjectR 数据和接口契约"
subtitle: "跨版本稳定 API、事件、Schema、序列化和迁移规则"
version: "v1.0"
date: "2026-07-10"
---

# 契约总则

契约定义模块之间允许共享的最小表面。实现可以替换，但契约进入已验收版本后默认冻结。C++ 类型、Blueprint API、GameplayTag、资产路径和 Save 字段的破坏性变化必须经过 Architecture Decision 与迁移测试。

## 通用规则

- 每个契约有唯一所有者；消费者不能绕过契约直接修改所有者内部状态。
- 事件负载使用值类型 ID、GameplayTag、弱对象引用或不可变快照，避免跨世界保存裸 UObject 指针。
- 失败结果必须显式返回，不以静默空值掩盖错误。
- 结构体用于持久化时必须包含 Schema/SaveVersion 或由所属 SaveGame 统一版本化。
- Blueprint 可调用函数的参数和语义一旦发布，新增参数优先使用新函数或带默认值的兼容入口。

# GameplayTag taxonomy 合同

**所有者**：Core/Tags。

**建立版本**：v0.0.2。

**事实源**：`Config/DefaultGameplayTags.ini`。

**规范入口**：`Docs/Architecture/GameplayTagTaxonomy.md`。

**消费者**：v0.0.3 及之后的 Game Framework、Enhanced Input、GAS、Combat、Companion、QTE、Director、Roguelike、Content 和 Save 合同。

- `Input.*` 是未来 Enhanced Input 到系统/Ability 的语义提交面，不绑定具体 Ability 类。
- `Skill.*` 是稳定技能 ID，并可由未来 GAS 消费为 AbilityTag。
- `Companion.*`、`QTE.*`、`Rule.*` 分别冻结伙伴身份、QTE 类型/结果和本地法令 ID。
- `Chapter.*`、`Room.*`、`Reward.*` 是数据驱动内容分类，本版本不创建对应业务对象或资产。
- 公共 C++ 只通过 `UPRTagLibrary` 的显式 checked getter 读取 16 个高频基础标签；不得添加任意字符串查询或 Blueprint 暴露。
- 后续只能增量添加子标签；重命名或删除必须提供 Redirect、ADR、消费者清单和兼容测试。

# Game Framework 与正式地图旅行合同

**所有者**：Core。

**建立版本**：v0.0.3。

**消费者**：v0.0.4 构建/打包入口、v0.1.0 Enhanced Input 与 2.5D 移动，以及后续 Reality Hub、Network Run、Combat 和 Boss 内容。

- `EPRMapId` 冻结五个正式 ID：`MainMenu`、`RealityHub`、`NetworkPrototype`、`CombatGym`、`BossGym`。
- `UPRGameInstance::OpenMap(EPRMapId)` 是唯一冻结的 Blueprint 地图旅行入口；使用 Soft World 引用，非法 ID 或无 World 时返回 `false` 并记录 `LogProjectR` Error。
- 正式地图路径冻结为 `/Game/ProjectR/Maps/L_MainMenu`、`L_RealityHub`、`L_Network_Prototype`、`L_CombatGym` 和 `L_BossGym`。删除、重命名或改动 ID 映射必须提供 ADR、消费者清单和迁移验证。
- 正式生命周期扩展点为 `UPRGameInstance`、`APRGameModeBase`、`APRRealityGameMode`、`APRNetworkRunGameMode`、`APRPlayerController`、`APRPlayerState` 和 `APRPlayerCharacter`；模板 `AProjectR*` 与 Variant 类型不是正式消费者依赖。
- GameMode Blueprint 统一使用 `BP_PlayerCharacter`、`BP_PlayerController` 和 `BP_PlayerState`；RealityHub 使用 `BP_RealityGameMode`，网络原型与两个测试场使用 `BP_NetworkRunGameMode`。
- v0.0.3 只冻结固定侧视相机脚手架，不冻结输入、移动、GAS、Save 或玩法行为；这些由各自版本增量实现。

# 1. CombatEvent 合同

**所有者**：Combat。  
**建立版本**：v0.1.2。  
**消费者**：HUD、QTE、PlayerProfile、Companion Dialogue、统计、Boss 和自动化测试。

建议负载 `FPRCombatEvent`：

| 字段 | 类型 | 语义 |
|---|---|---|
| EventId | FGuid | 本次事件唯一标识 |
| EventTag | FGameplayTag | Hit、Damage、Death、ShieldBreak、Dodge、Ability 等 |
| SourceId/TargetId | FName | 稳定实体 ID，不用于持久化 UObject |
| AbilityTag | FGameplayTag | 可空；触发能力 |
| Magnitude | float | 已在本地确定的数值 |
| WorldTimeSeconds | double | 本世界内排序时间 |
| Flags | FGameplayTagContainer | Critical、Perfect、Boss、Companion 等 |

兼容要求：新增事件类型使用新 Tag；不得让 QTE、UI 或画像反向调用 Combat 私有实现。

# 2. Ability、AbilitySet 与 InputTag 合同

**所有者**：Abilities。  
**建立版本**：v0.1.3。  
**消费者**：Enhanced Input、六个玩家技能、QTE、奖励构筑、AI 连携、HUD。

- `UPRGameplayAbility` 统一暴露 AbilityTag、InputTag、Cost、Cooldown、CombatEvent 输出和失败原因。
- `UPRAbilitySetDataAsset` 只描述授予内容，不承担运行时状态。
- 输入层只提交 InputTag；不得硬编码具体 Ability 类。
- 强化通过数据参数、GameplayEffect、Fragment/Modifier 接口扩展，不为每个插件复制 Ability 类。

冻结项：InputTag、AbilityTag、失败原因 Tag、AbilitySet Schema 和 Blueprint 事件名。

# 3. SaveGame 合同

**所有者**：Save。  
**建立版本**：v0.1.4。  
**消费者**：关系、账号墓园、任务、Meta、设置、本地化选择、Steam Cloud。

根结构至少包含：

```text
SaveVersion
ProfileId
CompanionRelationships
UnlocksAndProgression
AccountGraveyard
QuestProgress
Settings
AggregateStats
```

- 所有读取先执行 `MigrateSaveIfNeeded`。
- 新字段必须有安全默认值；删除/重命名字段必须保留迁移路径。
- 运行时 UObject、临时 Actor、原始 LLM 响应和 API Key 不进入存档。
- Steam Cloud 包装同一个本地 Save 数据，不创建第二套业务 Schema。

# 4. Companion 与 Relationship 合同

**所有者**：Companions；持久化由 Save。  
**建立版本**：v0.3.0。  
**消费者**：AI 实体、QTE、救援、对话、任务、结局。

- Companion 以稳定 CompanionId/GameplayTag 标识。
- Relationship 持久化 Trust、Affection、Evaluation Tags、Overload 和关键事件 ID。
- 战斗支援读取不可变关系快照，关系变化通过 `FPRRelationshipDelta` 请求并由所有者校验。
- AI 无法以支援伤害完成普通/精英/Boss 终结；阈值属于 Companion Combat Policy 数据。

# 5. QTEResult 合同

**所有者**：QTE。  
**建立版本**：v0.3.2。  
**消费者**：关系、画像、Director Rule Counter、Boss、记忆、HUD。

`FPRQTEResult` 至少包含 QTEId、CompanionId、ResultTag、TriggerEventId、TimingGrade、AppliedEffectIds、RelationshipDeltaRequest 和 ProfileSampleTags。

- QTE 负责候选、请求、响应和结果；长期关系/画像由各自所有者消费结果。
- 失败、拒绝、超时必须是不同 ResultTag。
- QTE DataAsset 的 ID 和触发 Tag 发布后冻结。

# 6. Director 合同

**所有者**：Director。  
**建立版本**：v0.4.0。  
**消费者**：法令、房间、Boss、章节、UI、对话。

数据流：

```text
FPRPlayerProfileSnapshot
  -> FPRDirectorRequest
  -> IPRDirectorProvider (Mock / HTTP)
  -> FPRDirectorResponse
  -> UPRDirectorResponseValidator
  -> Local Rule Registry
  -> Applied Rule Handle
```

Response 只允许 RuleId、Level、ReasonTags、VisibleReason、有限参数和表达文本。Validator 执行 Schema、白名单、Clamp、长度、超时和 fallback。未知 RuleId 永不进入执行层。

# 7. Room 与 Reward 合同

**所有者**：Roguelike。  
**建立版本**：v0.4.2。  
**消费者**：章节、经济、难度、法令、统计。

- RoomData 只描述 RoomId、TypeTag、LevelAsset、EncounterId、RewardPolicyId、Entry/Exit 条件和内容标签。
- RewardData 只描述 RewardId、稀有度、互斥标签、权重条件和应用接口。
- 章节通过内容注册表提供 Room/Enemy/Rule/Reward，不修改 RoomManager 核心流程。
- 经济和难度只能调整权重/约束，不直接生成未知资产类。

# 8. RunSummary 与 AccountRecord 合同

**所有者**：RunState；持久化由 Save。  
**建立版本**：v0.4.3。  
**消费者**：账号墓园、现实据点、长期记忆、任务、成就、Director Profile。

AccountRecord 保存账号 ID、身份、开始/结束时间、结束原因、章节、Boss、主要法令、构筑摘要、主同步 AI、关键 QTE、死因和反证奖励。RunSummary 是适合对话/画像消费的有界摘要，不保存无限战斗日志。

# 9. 契约升级清单

1. 列出所有直接消费者。
2. 判断能否仅新增字段/Tag/可选接口。
3. 若必须破坏，记录 ADR 和兼容期。
4. 更新 SaveVersion/SchemaVersion 和迁移器。
5. 添加旧数据、旧 Blueprint、旧资产引用的失败测试。
6. 运行当前版本和直接下游版本冒烟测试。
7. 更新版本依赖矩阵后才能关闭变更。
