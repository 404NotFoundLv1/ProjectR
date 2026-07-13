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
- 公共 C++ 只通过 `UPRTagLibrary` 的显式 checked getter 读取 30 个高频基础标签；不得添加任意字符串查询或 Blueprint 暴露。
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

# 构建、打包与自动化报告合同

**所有者**：Platform/Release Automation。

**建立版本**：v0.0.4。

**消费者**：v0.0.5 Unreal MCP 安全资产生产基线，以及后续 C++ Build、Cook/Package、自动化测试、QA 和发布版本。

- 公共入口冻结为 `BuildScripts/BuildEditor.bat`、`PackageDevelopment.bat`、`CleanGenerated.bat` 和 `AutomationReport.bat`；`PackageDev.bat` 仅为原样转发参数和退出码的兼容别名。
- 所有入口必须接收绝对 `.uproject` 路径，不依赖当前工作目录或 PATH 中的 UE/VS 工具；UE Root 按显式参数、`PROJECTR_UE_ROOT`、精确版本自动发现的优先级解析，零个或多个有效候选均显式失败。
- Development/Shipping 的唯一正式地图清单来自 `Config/DefaultGame.ini` 的五条 `MapsToCook`；打包脚本只读取、验证并转译该清单，不维护第二份可漂移列表。
- 每次入口调用写入 `Saved/AutomationReports/<RunId>/<EntryPoint>-<Configuration>/`，至少包含脱敏 `command.txt`、完整 `run.log` 和 SchemaVersion 1 `result.json`；创建任何文件前必须验证目标严格位于报告根下、既有路径链无 reparse point、Git ignored/untracked 且目标目录不碰撞。
- 自动化状态只允许 `PASS`、`FAIL`、`NOT_RUN`；WhatIf、预览和未执行门不得冒充真实 Build、Package、Clean Apply、Editor restart、Blueprint compile 或 PIE 通过。
- `AutomationReport.bat` 只汇总调用者提交的独立 checks，不执行或推断 MCP 写入；v0.0.5 必须分别记录资产创建、Blueprint compile、精确保存、重启回载和 PIE 的实际状态。
- `CleanGenerated.bat` 默认只预览。实际删除必须通过精确项目根确认、项目内包含关系、Git ignore/untracked、从项目根到候选的祖先链及候选全部后代无 reparse point、删除前重验、活动进程和保护路径检查；`Source`、`Content`、`Config`、`Docs`、`BuildScripts`、`.git`、uproject、AutomationReports、Packages、Autosaves、SaveGames、Logs 和 Screenshots 永久不在清理集合中。
- 公共 CLI、退出码、报告 Schema、磁盘门禁和人工例外的规范说明位于 `Docs/Workflow/BuildGuide.md`；破坏性更改必须新增 ADR、消费者清单和兼容回归。

# Unreal MCP 安全资产生产合同

**所有者**：Content Authoring Automation。

**建立版本**：v0.0.5。

**消费者**：v0.1.0 及之后需要通过 Unreal MCP 创建或修改 UE Package 的版本。

- 每次写入前必须冻结 Operation Manifest：准确 Package/允许的动态 External roots、Tool/参数、碰撞和可编辑性结果、串行顺序、停止点、精确保存范围、重启与 PIE 门。
- World Partition 地图不得使用通用 AssetTools Duplicate；使用 UE5.8 官方 Builder，并在主 World、External Actors 和 External Objects 均无碰撞后串行执行。生成哈希 Package 在每个阶段后枚举并冻结路径与 SHA-256。
- `AssetTools.save_assets` 必须传非空准确列表；空列表、Save All、Resave All、普通文件 IO 和未批准删除/移动/重命名永久禁止。
- 重启前必须满足 Blueprint warnings-as-errors 编译通过、Manifest 目标逐项保存成功、可查询 AssetRegistry inventory 的逐项 `is_dirty=false`；不可浏览的 External Package 通过冻结清单、SHA-256 和无保存提示的正常关闭门补足，不得把 object ref 当作磁盘 Package path 解释。
- 资产创建、Blueprint compile、精确保存、Dirty=0、重启回载、引用验证和 PIE 是独立验收状态；任一状态不能由其他状态推断。
- 测试资产隔离在 `/Game/ProjectR/MCPTest/**` 及任务明确批准的两个对应 External roots，不进入五张正式 `MapsToCook`，正式 Framework 和地图不得引用测试根。

# Enhanced Input 与 2.5D 移动合同

**所有者**：Core/Input。

**建立版本**：v0.1.0。

**消费者**：v0.1.1 GAS 基础、v0.1.3 Ability InputTag 路由、v0.3.2 QTE 输入和 v0.8.2 输入重绑定。

- 正式资产根冻结为 `/Game/ProjectR/Input/`；`IA_Move`、`IA_Jump`、`IA_Attack`、`IA_Dodge`、`IA_Interact`、`IA_Execute`、`IMC_Player` 与 `DA_PlayerInputConfig` 是默认基线。
- `UPRInputConfigDataAsset` 通过 `FPRTaggedInputAction` 把稳定的 `Input.*` GameplayTag 映射到 InputAction；输入层只提交语义 Tag，不绑定具体 GameplayAbility、QTE 或 UI 类型。
- `APRPlayerController` 的 Blueprint CDO 持有 InputConfig；本地 Controller 幂等添加默认 Mapping Context，并只移除自己拥有的 Context。重复 BeginPlay/OnPossess 不得重复注册。
- `APRPlayerCharacter` 在每次 `SetupPlayerInputComponent` 从当前 Controller 解析全部必需 Action，解析完整后才绑定；移动只提交世界 X 输入，跳跃使用 CharacterMovement，语义按钮只调用 protected native Pressed/Released 钩子。
- 空中收到与当前 X 速度反向的有效 Move 输入时，Character 必须立即把 X 速度按原绝对值切换到输入方向；不得改变 Z 速度、生成 Y 平面、地面移动参数、最大速度或跳跃参数。
- 角色在 BeginPlay 以生成时 Y 为平面原点启用 Y 法线约束；Actor/胶囊和固定相机不随左右朝向旋转。实际 X 移动方向立即响应输入，Mesh 视觉朝向才以 0.12 秒 Ease-In-Out 在 `-90/+90` yaw 之间过渡并保持最后方向；快速反向时必须从当前插值角度重新开始，不能先跳到端点。
- 正式默认 IMC 包含 v0.1.0 的 13 条基线 Mapping；永久校验只要求基线存在且有效，允许下游增量扩展。重命名或删除正式资产、Tag 或钩子必须提供 ADR、消费者清单和兼容测试。
- `ProjectRAuthoringTools` 只提供 Editor-only 的真实 PIE 输入注入，不进入 Shipping、不保存 Package，也不成为 ProjectR Runtime 依赖。

# GAS ASC、属性与默认初始化合同

**所有者**：Core/Abilities。

**建立版本**：v0.1.1。

**消费者**：v0.1.2 Damage/Death、v0.1.3 AbilitySet/InputTag、v0.2.3 HUD。

- `APRPlayerState` 永久持有 replicated `UPRAbilitySystemComponent` 与 `UPRAttributeSet`；ASC Owner 固定为 PlayerState，Avatar 为当前 `APRPlayerCharacter`。Pawn 替换只更新 Avatar，不重建 ASC 或 AttributeSet。
- `UPRAbilitySystemComponent` 在本版本保持薄类型，使用 Mixed replication；Ability 授予、Cooldown、Cost 和 InputTag 路由属于 v0.1.3。
- 11 项属性名、类型、范围和 `REPNOTIFY_Always` 复制合同冻结。当前值变化通过统一的 `FPRAttributeChangedNative` 原生事件发布，Abilities 不依赖 HUD。
- MaxHealth、MaxShield、MaxEnergy 变化时保持对应当前值比例；旧 Max 为 0 时只 Clamp、不补满。Clamp 不产生伤害、死亡、恢复或 CombatEvent 语义。
- `/Game/ProjectR/Effects/GE_DefaultAttributes` 是默认值的正式 GameplayEffect。它为 Instant、11 个 Override Modifier；C++ 安全默认值必须与 GE 保持一致，并由自动化防止漂移。
- 默认 GE 只在 Authority 上成功应用一次，成功状态归 PlayerState。缺失 Class、无效 Spec 或应用失败不消费重试；UE5.8 Instant GE 必须用 `FActiveGameplayEffectHandle::WasSuccessfullyApplied()` 判定成功，不能使用 `IsValid()`。
- `APRPlayerCharacter` 在 PossessedBy/OnRep_PlayerState 重新初始化 ActorInfo，幂等绑定 MoveSpeed 并立即同步 CharacterMovement；旧 Pawn 只有仍是 ASC 当前 Avatar 时才可清除 ActorInfo。
- v0.1.0 的输入、空中反向、0.12 秒 Mesh 转向、Y 平面和固定侧视相机继续作为强制回归合同。

# 1. 统一伤害与 CombatEvent 合同

**所有者**：Combat。  
**建立版本**：v0.1.2。  
**消费者**：v0.1.3 Ability、v0.2.0 玩家技能、v0.2.1 敌人、v0.2.3 HUD、v0.3.2 QTE，以及画像、对话、统计和 Boss。

- `UPRCombatSubsystem::ApplyDamage(FPRDamageRequest)` 与 `Revive(FPRReviveRequest)` 是唯一公共 C++ 入口；不暴露 Blueprint API。
- 伤害只沿 `DamageRequest -> GE_Damage -> UPRDamageExecutionCalculation -> IncomingDamage -> UPRAttributeSet` 结算一次。Execution 只输出 Meta 属性，AttributeSet 是唯一 Shield/Health 扣减点，Subsystem 只读取结算前后差值并发布事实。
- `FPRDamageRequest` 冻结 `SourceId`、`DamageSource`、`Instigator`、`Target`、`AbilityTag`、`RawDamage`、`bCritical` 和 `DamageTags`。零、负数、NaN、Inf 或缺失必需引用为 `Invalid`；有限值超过 200000 Clamp 并记录 Warning。
- `FPRCombatEvent` 冻结 `EventId`、`EventTag`、`SourceId`、`TargetId`、`DamageSource`、`Instigator`、`Target`、`AbilityTag`、`RawDamage`、`ShieldAbsorbed`、`HealthDamage`、`RemainingHealth`、`RemainingShield`、`bCritical`、`DamageTags`、`ResponseTags`、`bFatal` 和 `WorldTimeSeconds`。稳定 ID/Tag/数值可持久化；弱 UObject 引用不可持久化。
- 成功伤害广播 `Combat.Event.Damage`；致死顺序严格为 `Damage -> State.Dead -> Combat.Event.Death`。Dead/Invulnerable 拒绝广播 `Combat.Event.DamageRejected`，不会修改属性；Dead 优先于 Invulnerable。
- Shield 吸收、破盾与 Health 扣减只通过 `Combat.Response.*` 写入 `ResponseTags`，不额外创建第二套伤害事件。
- `State.Alive` 与 `State.Dead` 互斥并以 TagOnly loose tag 复制；死亡状态、ASC 与属性归 PlayerState，跨 Pawn 替换保留。复活以快照回滚保证属性和 Tag 原子切换，不重放默认属性 GE。
- `IPRCombatantInterface` 由 ASC Owner 提供稳定 ID 与 DamageEffect；`IPRCombatFeedbackInterface` 由 Avatar 提供受击/生死反馈。Combat 不依赖具体敌人、HUD、QTE、画像、对话或统计类型。
- 新事件类型只能增量增加新的 `Combat.Event.*` Tag；现有字段或 Tag 的重命名/删除必须提供 ADR、消费者清单与兼容迁移。

# 2. Ability、AbilitySet 与 InputTag 合同

**所有者**：Abilities。  
**建立版本**：v0.1.3。  
**消费者**：Enhanced Input、六个玩家技能、QTE、奖励构筑、AI 连携、HUD。

- `UPRGameplayAbility` 固定为 `InstancedPerActor` 的抽象 Blueprint 基类，只保存 `AbilityTag`、`ActivationPolicy`、`bCanActivateWhileDead` 和 `bCancelOnDeath`。InputTag 只存在于 `FGameplayAbilitySpec` 动态源 Tag；基类不创建 `FPRDamageRequest`、CombatEvent、位移或具体技能表现。
- `UPRAbilitySetDataAsset` 是 PrimaryAsset 类型 `ProjectRAbilitySet`，只序列化 `FPRAbilitySetEntry[]`。Entry 冻结 AbilityClass、AbilityLevel、InputTag、初始化授予标记、GrantedSpecTags 与可选 AbilityData；GrantId、SpecHandle、Held、ActiveEffectHandle 和弱 UObject 引用均不得进入 Save。
- `UPRAbilitySystemComponent` 是授予记录、InputTag 路由和生命周期事件的唯一运行时所有者。Authority 授予以内部 GrantId 为事实源，支持 `InitializationOnly` 到 `AllEntries` 的增量补充、准确移除和本次调用失败回滚；PlayerState-owned Spec 跨 Pawn 替换保留。
- 输入层只提交合法 `Input.*` Tag。Pressed 上升沿维护 Held 并按 `OnInputTriggered`/`WhileInputActive` 尝试一次激活；重复 Press 无操作；Release 只处理 Held Spec，Held 策略随后准确取消。合法但无匹配 Spec 的输入静默无操作，重复匹配属于错误。Passive 不持有 InputTag，在授予、ActorInfo 可用或 Revive 后按需恢复一次。
- Dead 进入时 ASC 清 Held，并只取消 `bCancelOnDeath=true` 的 ProjectR Ability；Revive 不重新授予或更换 SpecHandle。Avatar 更换只取消旧 Avatar 的活动实例、清 Held、更新 ActorInfo 并恢复 Passive，不重建 AbilitySet。
- Energy Cost 与 Cooldown 的唯一权威来源是 Ability 继承的 GameplayEffect Class。Commit 顺序固定为 CheckCooldown、CheckCost、应用并验证 Cooldown、应用并验证 Cost、Notify；Cooldown 失败不扣 Cost，Cost 后半段失败只回滚本次 Cooldown。Instant Cost 使用 `WasSuccessfullyApplied()` 判定。
- `FPRAbilityLifecycleEventNative` 由 ASC 单向发布 Granted/Removed/Activated/ActivationFailed/Committed/CommitFailed/Ended/Cancelled；未来 HUD、QTE 和统计只能订阅事件并使用只读 RuntimeState 查询，不能创建第二套生命周期事实。
- 正式 `/Game/ProjectR/Abilities/DA_DefaultAbilitySet` 在 v0.1.3 保持空；验证 GA/GE/AbilitySet 隔离在 `/Game/ProjectR/MCPTest/Abilities/`，不得成为正式技能或正式内容依赖。

冻结项：四个公共枚举、四个公共结构字段顺序、InputTag 动态 Spec Tag 语义、AbilityTag、六个失败 Tag、AbilitySet PrimaryAssetId/Schema、GrantId 幂等语义、Commit 顺序和生命周期事件顺序。

# 3. SaveGame 合同

**所有者**：Save。  
**建立版本**：v0.1.4。  
**消费者**：关系、账号墓园、任务、Meta、设置、本地化选择、Steam Cloud。
**当前状态**：v0.1.4 已建立并验证原生 Schema 1、`PRSV` Envelope、A/B 平台槽、严格迁移注册表和单 active/single trailing 异步保存队列。

- `UPRSaveGame : USaveGame` 只序列化三个 `SaveGame` 属性，顺序为 `SchemaVersion`、`SaveRevision`、`Profile`；`FPRProfileSaveData` 当前只含稳定 `ProfileId`。`CurrentSchemaVersion=1`、`MinimumMigratableVersion=1`，新建内存对象允许 Revision 0，成功落盘对象必须 Revision > 0。
- `UPRSaveSubsystem : UGameInstanceSubsystem` 的唯一产品入口为 `LoadDefaultProfile`、`CreateNewDefaultProfile`、`RequestSaveCurrentProfile`、`GetSaveRuntimeState` 和 `OnSaveOperation`。入口只接受 Game Thread；Initialize/PIE/退出不会自动 Create、Load、Save 或 Delete。
- `FPRSaveRuntimeState` 与 `FPRSaveOperationEvent` 只承载当前进程的只读状态和结果，不被保存或复制。非 Game Thread、Busy、ShuttingDown、合并、失败与取消均有显式枚举结果；所有对外事件只在 Game Thread 广播。
- 物理数据为 `SaveGameToMemory` 产生的 UE Payload 外包固定 16 字节小端 `PRSV` Header：Magic、EnvelopeVersion 1、HeaderSize 16、PayloadSize、CRC32；Payload 上限 16 MiB，长度必须精确，无尾随数据。未来 Envelope/Schema 必须阻断旧客户端覆盖。
- 生产存储只通过 `ISaveGameSystem`；Editor/Development/Shipping 的 Profile 0 使用各自固定 Base 和 `_A`/`_B` 后缀。首次保存写 A，后续写非当前代；写前重验观察快照，写后异步回读并逐项验证 Envelope、CRC、Class、Profile、Schema、Revision 和原始字节，验证通过前不发布新 Revision。
- 加载对 A/B 独立诊断并按 Future barrier、Profile/Revision 冲突、最高 Revision、相同 Payload 确定性选 A 的顺序选择；单代有效可从损坏备用代恢复并设置 `bNeedsResave`，但 Load 不自动写回。无有效代按冻结的错误优先级返回，不静默 Create。
- 保存队列最多一个 active 物理操作和一个 trailing 逻辑请求；trailing 为 last-write-wins 并复用同一 RequestId。active 失败先发布自身失败，再取消 trailing；Deinitialize 拒绝新请求、取消未启动 trailing，active Weak 回调在对象销毁后静默丢弃。
- `FPRSaveMigrationRegistry` 只允许准确 `N -> N+1` 且每个 From 唯一；迁移在副本上顺序执行，任一步失败或版本未精确加一都不替换原对象。v0.1.4 生产注册表为空，不伪造 Schema 0 存档，也不使用 Project CustomVersion 作为业务迁移版本。
- Automation 后端只接受 `ProjectR_Automation_<32位小写十六进制 GUID>`，每次访问写入内存 Ledger；物理测试只能查询、写入、回读并通过平台 API 精确删除当前 GUID 的 `_A`/`_B`，禁止枚举或探测生产槽。
- 未来关系、Account/Run/Graveyard、Meta、Settings 与有界 Memory/RunSummary 只能在对应版本通过新 Schema 和逐版本迁移加入。运行时 UObject、ASC/AttributeSet、Actor、Spec/Effect/Grant Handle、Held Input、Delegate、CombatEvent、原始 LLM 响应和 API Key 永不进入存档。
- Steam Cloud 复用同一 A/B 槽字节、`PRSV` 和业务 Schema，不创建第二套云端格式；任何破坏性格式变化必须新增 ADR、迁移、消费者清单与兼容测试。

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
