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
- 公共 C++ 只通过 `UPRTagLibrary` 的显式 checked getter 读取 47 个高频基础标签；不得添加任意字符串查询或 Blueprint 暴露。
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
- v0.2.0-A 使用独立 `EntryPoint=v020A`：37 个 required check 必须来自本轮新鲜 Build、Automation、MCP 与静态审计证据；PIE、Network PIE、物理手柄、人工技能手感和 Authoring Tool 扩展为 optional `NOT_RUN`，不得借用 v0.2.0-E 的完整 58 项报告。
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
- v0.2.0-A 的冻结子 Manifest 为 29 个 Create、2 个 Modify、31 个精确保存 Package；`DA_DefaultAbilitySet` 与三个 Player Blueprint 仅回读/编译，不保存。UE5.8 的 IMC 序列化事实源为 `DefaultKeyMappings.mappings`；inline Fragment 和 GEComponent 必须逐对象回读，官方工具不能可靠表达时停止而不是保存猜测值。
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
- v0.2.0-A 在六项 InputConfig 基线之后追加 ShadowThrust、FireSlash、ThunderDrop、VectorHook、CounterProofWall 五项，在 IMC 索引 13–25 追加 J/K/L 与五组技能键鼠/手柄映射；旧六项配置和旧 13 条 Mapping 的 Action、Key、Modifier、顺序必须逐项不变。AfterimageDodge 继续复用 `Input.Dodge`/`IA_Dodge`；Character 只向 ASC 转发 Pressed/Released Tag，不授予未完成技能。
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
- v0.2.0-A 只为 `State.Stunned` 增加 PlayerSkill 活动 Ability 的释放/取消门；Dead、Revive、Passive、Avatar 替换、Mixed replication、11 项复制属性和 transient `IncomingDamage` 合同不变。

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
- v0.2.0-A 在 `FPRDamageRequest` 末尾追加 `ImpactOrigin` 与 `IncomingDirection`；方向约定为伤害传播方向（ImpactOrigin/Source → Target）。旧非技能调用允许零方向，Skill 请求要求 finite 非零方向，进入 Mitigation 前归一化。
- `EPRCombatRequestStatus` 的旧值 `Applied=0` 至 `Invalid=4` 保持不变，只在末尾追加 `RejectedBlocked=5`。伤害检查顺序冻结为结构/Authority/finite → Target → Dead → `IPRCombatMitigationInterface` → Invulnerable → GAS；合法 Blocked 只接受非空 `Combat.Response.*`，不改属性/Effect/死亡状态，并恰好广播一次 `DamageRejected`。
- `FPRCombatOutcomeRequest` 与 `PublishAbilityOutcome` 只发布零数值 `Combat.Event.AbilityOutcome`；要求 Authority、SourceId、AbilityTag、Instigator及非空纯 `Combat.Response.*`。TargetId 依次由 Combatant、CombatEventSubject 解析；Combat 不 include 具体技能、敌人、Boss、QTE、HUD 或 Director。

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
- v0.2.0-A 新增 `UPRPlayerSkillDataAsset`（PrimaryAsset 类型 `ProjectRPlayerSkill`）、只含数据的 inline Fragment、`UPRPlayerSkillGameplayAbility`、Avatar 局部 `UPRPlayerSkillComponent` 与 World 级 `UPRPlayerSkillSubsystem`。依赖方向固定为 Skill DA → GA Blueprint → Cost/Cooldown GE；GA 从 AbilitySpec SourceObject 读取 DA，禁止 GA → DA 硬引用环。
- ASC 继续独占 Spec、AbilitySet、Held Input、Commit、Cost/Cooldown 与 Ability 生命周期；Component 只持有 Phase/Timer/当前位移，Subsystem 只做确定性目标查询、RootMotionSource 位移注册/清理并把 Outcome 转发 Combat。两者不得形成第二套授予、扣属性、输入或事件事实源。
- 目标资格只通过 native `IPRAbilityTargetInterface::CanBeTargetedByAbility(AbilityTag)`；v0.2.0 不新增 Faction enum/Team Tag。查询和位移使用 Source 当前 Y 平面的 X/Z 投影、1 cm 容差、固定 LOS/排序/重复 TargetId 失败合同；World Cleanup、Subsystem Deinitialize、Avatar 失效、Dead/Stunned 和 EndPlay 必须幂等清理 Timer、delegate 与 RootMotionSource。
- 六个正式 GA/DA/Cost/Cooldown GE 在 A 仅为空壳；六个 GA BP 无业务 Graph，`DA_DefaultAbilitySet` 仍为空。Shadow/Fire/Thunder/Afterimage/Hook/Wall 的具体逻辑、状态 GE、VFX/SFX 与授予均留给 B–E。
- v0.2.0-B 在不扩展上述公共合同的前提下实现 ShadowThrust 与 FireSlash：具体 GA 只通过 Subsystem 私有适配器调用统一 Combat；Shadow 使用现有 ForwardSweep、AvatarComponent RootMotion 和每次激活 TargetId 去重，Fire 使用 ForwardArea 并以 WorldSubsystem 私有注册表维护唯一 Burning 实例。Burning 每 0.5 秒经 Combat 结算一次，共三次；刷新来源与快照但不叠加第二个 Effect/Timer。
- B 的正式 `GE_State_Burning` 只授予 `State.Burning`，1.5 秒、无 Period、AggregateByTarget/StackLimit 1/成功应用刷新 Duration；Fire GA CDO 是该 GE 的唯一具体技能绑定。`DA_DefaultAbilitySet` 在 B 只含 ShadowThrust、FireSlash 两个 startup Entry，ASC 的 GrantId/SpecHandle/Input/Commit/Cost/Cooldown 所有权不变。
- 正式侧视角色以 Mesh 的局部 Right 轴表达当前 X 方向朝向（相对 yaw -90 为 +X、+90 为 -X）；Shadow/Fire 只把该轴投影到 X/Z，禁止使用会落入 Y 轴的 Mesh Forward。受控位移结束、阻挡或取消时，移除 RootMotionSource 后必须清除其残余强制速度，避免 Recovery 漂出已验证终点。Shadow 若把原始 450 cm 路径裁到 WorldStatic 前的安全终点，即使公共位移请求到达该安全终点，具体 GA 仍按技能层 Blocked 处理且不得发布 `DisplacementApplied`。
- v0.2.0-E 为六个已授予正式 Skill DataAsset 绑定安全占位 `VFX`/`SFX` 软引用。`UPRPlayerSkillComponent` 是唯一通用表现入口：在 Avatar 安全阶段异步预加载已授予 Skill DataAsset 的软引用，只在本地非 Dedicated Server 创建附着的 Niagara/Audio 组件；激活不得同步加载。软引用缺失或未加载时只记录一次受控 Warning 并跳过表现，绝不影响 Check/Commit、Combat、位移、Cost、Cooldown、状态或 AbilityOutcome。能力结束、取消、Avatar 替换、World Cleanup、Component EndPlay 与 Owner EndPlay 必须幂等停止/销毁表现组件并释放流式句柄。表现资产不得包含碰撞、伤害、GameplayTag、GameplayCue、执行逻辑或永久 Tick。
- v0.2.0-E 没有新增 GameplayTag、Input、Save、Spec、GrantId、EffectHandle 或公共 PlayerSkill API；`DA_DefaultAbilitySet` 保持六项严格顺序。表现只可由 v0.2.4 通过 DataAsset 软引用替换，不得把运行时 Target、Timer、RootMotion、Held Input、Guarding 或 UObject 引用写入 Save。
- v0.2.4 经用户明确批准的兼容修订只追加 `Skill.BasicAttack`（GameplayTag 总数 127、根 13、checked getter 61）和一个第七 `DA_DefaultAbilitySet` Entry；前六项顺序、输入、GrantId/SpecHandle 语义和六项 PlayerSkill 数据/表现合同不变。`UPRBasicAttackDataAsset`（PrimaryAsset `ProjectRBasicAttack`）与 `UPRGA_BasicAttack` 固定使用 `Input.Attack`、`OnInputTriggered`、150 cm/45 cm ForwardSweep、`10 + 1.0 * AttackPower`、0.08/0.06/0.22 秒与零 Cost/Cooldown。它通过既有 ASC InputTag 路由和 `UPRCombatSubsystem::ApplyDamage`，不进入 `UPRPlayerSkillComponent`、不请求表现资产、不新增 InputAction/GE，也不形成第二套输入、授予、伤害或生命周期系统。
- v0.2.4 的 Combat HUD 继续只展示六项 P0 PlayerSkill；BasicAttack 无 SkillBar Slot。固定 PlayerSkill PIE 工具必须接受 7 个正式 Spec，但只对六项 P0 SkillData 执行表现与既有 B–E 行为回归。

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

# 4. 隔离式 Debug 与日志合同

**所有者**：Core/Logging 与 `ProjectRDebug`。
**建立版本**：v0.1.5。
**消费者**：Development QA、v0.2.0-v0.4.3 的受控调试适配器、v0.9.1 QA，以及 v1.0.1/v1.0.2 发布边界。

- ProjectR 主模块只拥有通用日志类别和脱敏工具，不得 include、链接、动态加载或以软路径引用 `ProjectRDebug`。兼容类别 `LogProjectR` 保留；新增类别固定为 `LogProjectRCore`、`LogProjectRInput`、`LogProjectRGAS`、`LogProjectRAbility`、`LogProjectRCombat`、`LogProjectRSave`。`LogProjectRDebug` 只在 Debug 插件内定义。
- `FPRLogSanitizer::RedactedValue()` 固定返回 `[REDACTED]`；GUID 日志关联只允许输出域分隔 SHA-1 的 `prg_` 加 12 位小写十六进制，非法 GUID 输出 `prg_invalid`。该 token 不持久化、不作为认证或业务身份。
- Debug 命令使用 SchemaVersion 1 的结构化 Descriptor/Argument/Request/Result。CommandId、StableName、参数顺序、类型、范围、Choice 白名单和 `bChangesRuntimeState` 发布后冻结；未来只能追加新 CommandId，不得复用、重排或静默改名。
- Registry 只接受 Game Thread 调用、弱绑定 UObject Provider 和准确 Provider Handle。Provider、CommandId、StableName 三重唯一；同 Provider 的完全相同注册幂等返回原 Handle，失效 Provider 不执行，注销只影响准确 Handle。
- v0.1.5 的可用命令固定为 `Debug.Status`、`Combat.DamageSelf`、`Combat.ReviveSelf`、`Save.RuntimeState` 和 `Travel.CombatGym`。它们只能调用既有 Combat、Save 只读状态和枚举式 Travel API；不得缓存 Pawn/ASC，不得读取 ProfileId，不得形成第二套玩法或存储入口。
- `Ability.GrantResource`、`Ability.ClearCooldown`、`Director.GenerateRule`、`QTE.Spawn`、`Companion.CycleAI`、`Boss.Jump` 在正式白名单 API 建立前必须返回 `NotAvailable`，不能用临时反射、Console、Spawn、对象路径或自由文本绕过。
- `UPRDebugSubsystem` 绑定 GameInstance 生命周期；原生 Slate 面板由 SceneViewport 焦点下的非重复 F1 打开，面板后代焦点只允许 F1 关闭。PreLoadMap、World cleanup、PIE Stop 和 Deinitialize 必须移除准确 Widget、InputProcessor、Provider 与 Delegate，并恢复鼠标和 GameOnly 输入模式。
- `ProjectRDebug` 是无 Content 的 Win64 Runtime 插件，但 `.uplugin` 模块与 `.uproject` 引用同时 `TargetConfigurationDenyList=[Shipping]`，并保留 Shipping compile tripwire。Development Feature Gate 还受 `UPRDeveloperSettings::bEnableDebugFeatures` 控制；Shipping 中该设置不能重新激活不存在的模块。
- `BuildScripts/ValidateDebugBoundary.ps1` 是正式静态/Package 门：Development 必须证明插件和模块存在；Shipping 必须用目标收据、Shipping 编译动作、Stage、Archive 和运行时共同证明插件不存在。共享或陈旧 UHT Manifest、同轮 Development Editor 日志不得作为 Shipping 包含证据。
- 命令审计日志只允许 `Request=<opaque token>`、固定 `StableName`、结果枚举和 `ChangesRuntimeState`；不得记录参数原文、ProfileId、Payload、路径、环境变量、账号或自由文本。
- 本合同不创建 UE Package、输入资产、Widget Blueprint、Save Slot 或 GameplayTag。任何未来 Debug 写操作都必须先有正式白名单业务 API、Allowed-path 修订、独立测试和 Shipping 边界回归。

# 5. Companion 与 Relationship 合同

**所有者**：Companions；持久化由 Save。  
**建立版本**：v0.3.0。  
**消费者**：AI 实体、QTE、救援、对话、任务、结局。

- Companion 以稳定 CompanionId/GameplayTag 标识；v0.3.0 固定 Axiom、Kindle、Null 三项，Registry 顺序不可变。
- `FPRRelationshipState` 只持久化 `Trust`、`Affection`、`Evaluation`、`Overload` 四个 `int32 [0,100]`，默认 `50/50/50/0`。`FPRCompanionRelationshipRecord` 只含 `CompanionId` 与 State，必须为三项 canonical 顺序；不保存关键事件 ID、UObject、Spec、EffectHandle、Timer 或 Delegate。
- `UPRCompanionSubsystem` 是关系与 run-local 主同步的唯一 owner：每轮恰有一个 Primary 和两个按 Registry 派生的 Background；重复选择幂等。没有已加载 Profile 时读 API 返回默认 canonical 快照，但 Delta 必须拒绝且不发起 I/O；成功 Create/Load 才投影 Schema 2 的持久化关系，选择本身不持久化。
- Relationship 变化只由具名 `FPRRelationshipDelta` 经 Subsystem 逐字段 Clamp 后 stage 到已加载 Profile，实际值变化才广播一次 `FPRRelationshipChangedEvent`。SaveSubsystem 只持有值类型，不依赖 Companion 类。
- 战斗支援、QTE、对话和保护系统是后续消费者；v0.3.0 不创建 AI、Ability、QTE、Dialogue、Widget 或 Companion Combat Policy。

# 6. QTEResult 合同

**所有者**：QTE。  
**建立版本**：v0.3.2。  
**消费者**：关系、画像、Director Rule Counter、Boss、记忆、HUD。

`FPRQTEResult` 至少包含 QTEId、CompanionId、ResultTag、TriggerEventId、TimingGrade、AppliedEffectIds、RelationshipDeltaRequest 和 ProfileSampleTags。

- QTE 负责候选、请求、响应和结果；长期关系/画像由各自所有者消费结果。
- 失败、拒绝、超时必须是不同 ResultTag。
- QTE DataAsset 的 ID 和触发 Tag 发布后冻结。

# 7. Director 合同

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

# 8. Room 与 Reward 合同

**所有者**：Roguelike。  
**建立版本**：v0.4.2。  
**消费者**：章节、经济、难度、法令、统计。

- RoomData 只描述 RoomId、TypeTag、LevelAsset、EncounterId、RewardPolicyId、Entry/Exit 条件和内容标签。
- RewardData 只描述 RewardId、稀有度、互斥标签、权重条件和应用接口。
- 章节通过内容注册表提供 Room/Enemy/Rule/Reward，不修改 RoomManager 核心流程。
- 经济和难度只能调整权重/约束，不直接生成未知资产类。

# 9. RunSummary 与 AccountRecord 合同

**所有者**：RunState；持久化由 Save。  
**建立版本**：v0.4.3。  
**消费者**：账号墓园、现实据点、长期记忆、任务、成就、Director Profile。

AccountRecord 保存账号 ID、身份、开始/结束时间、结束原因、章节、Boss、主要法令、构筑摘要、主同步 AI、关键 QTE、死因和反证奖励。RunSummary 是适合对话/画像消费的有界摘要，不保存无限战斗日志。

# v0.2.1-A Enemy 公共基础合同

**所有者**：Enemy；**直接消费者**：v0.2.1-B–E、v0.2.2 Boss、v0.2.3 HUD、v0.3.2 QTE。

- `APREnemyCharacter` 自持 `UPRAbilitySystemComponent` 与 `UPRAttributeSet`，在 Authority 上以 `(this, this)` 初始化 ActorInfo，并使用 Minimal replication；这不改变 PlayerState-owned、Mixed player ASC。初始化顺序为 Prototype/SpawnId 验证、ActorInfo、一次 DefaultAttributes GE、Alive/Dead Tag、幂等 AbilitySet 授予、StateTree 启动。失败保持 Dormant，不能攻击。
- `EPRAbilityActivationPolicy::ServerTriggered=3` 追加在既有枚举末尾。它没有 InputTag，只能由 Authority 的 `TryActivateAbilityByAbilityTag` 以唯一精确 AbilityTag 激活；空 Tag、非 Authority、零或多匹配以及非 ServerTriggered CDO 均失败。
- `UPREnemySubsystem` 只接受 `PrototypeTag + FTransform`，不接受类、路径、字符串或任意 UObject；A 的固定 Registry 只生成 MeleeMinion。`FPREnemyRuntimeState`、SpawnId、目标、Token、Timer、Delegate 和 StateTree 均是运行时数据，不能保存。
- Enemy 只通过已有 Target/Combatant/AttackProxy 窄接口发现玩家和结算伤害；Player pawn 的 Combatant 适配仅转发既有 PlayerState 的稳定 CombatantId 与 Damage GE，不复制 ASC、属性或 Combat 状态。该适配使 Actor Avatar 能满足既有 CombatSubsystem 的目标接口，而不让 Enemy include PlayerCharacter。
- X/Z 受控移动固定 PlaneY，禁止 NavMesh、MoveTo、PathFollowing、BehaviorTree、Blackboard、EQS 和 Actor/Brain 永久 Tick。攻击由 ServerTriggered native GA 进入 Windup/Active/Recovery/Cooldown，以 GUID Token 将每次激活限制为对同一目标最多一次 `UPRCombatSubsystem::ApplyDamage`。

# v0.2.1-B Shared DefaultAttributes 补充合同

**所有者**：Enemy；**直接消费者**：v0.2.1-B–E、v0.2.2 Boss、Enemy 自动化。

- `/Game/ProjectR/Enemies/Effects/GE_Enemy_DefaultAttributes` 是唯一的 Enemy 默认属性 Instant GE。它准确包含 11 个 `Override` / SetByCaller modifier，固定顺序为 `MaxHealth, Health, MaxShield, Shield, MaxEnergy, Energy, AttackPower, MoveSpeed, CritChance, Permission, Resonance`；每项分别使用同名冻结的 `Enemy.Data.*` Tag。
- Max 属性必须先于对应 Current 属性应用。该顺序使非默认 P0 值（特别是 Ranged 的 Health/MaxHealth=80/80）不会在 Current 写入时被旧 Max 值钳制；它不是可忽略的编辑器重序列化。
- GE 固定为 `Instant`、Period=0、无 Granted Tag/Ability、Execution、GameplayCue、GE Component 或 Stacking。后续原型只能提供 SetByCaller 数值，不得重排、复制或为某个原型修改该共享 GE。
- 此补充不变更公共 C++ Schema、GameplayTag、CombatSubsystem、AttributeSet 或 PlayerSkill；它将 B 的共享资产修正确认冻结为 C–E 的只读上游基线。未来正式 Shield 恢复仍必须经 AttributeSet/Combat 合同，不能以修改此 GE 绕过。

# v0.2.1-C ShieldMinion Guarding 合同

**所有者**：Enemy；**直接消费者**：v0.2.1-D、v0.2.2 Boss、v0.2.3 HUD、v0.3.2 QTE。

- `APREnemyCharacter` 对所有已初始化、Alive 且具有 Shield 属性的 Enemy 以 ASC Shield Attribute Change Delegate 维护 `State.Guarding`：当且仅当 `Shield > 0` 时 tag count 为 1；`Shield <= 0`、死亡、EndPlay、Despawn 或初始化失败时为 0。绑定、解绑及重复初始化均幂等；通过正式属性入口将 Shield 恢复为正值会重新添加该状态事实。
- `APREnemyCharacter` 的 Dead 反馈保留 Actor 以支持未来正式 Revive，但在 Dead 的首次反馈中记录当前 Capsule `ECC_Pawn` 响应并设置为 Ignore；复活反馈幂等恢复记录的原始响应。该规则只处理 Pawn 阻挡，不更改 Alive Enemy 碰撞、伤害、属性、AI、Registry 或保存语义。
- 该 tag 不是新的 Mitigation、Guard HP 或阻断结果：伤害仍唯一经 `UPRCombatSubsystem::ApplyDamage` 和既有 AttributeSet spill-over，先 Shield 后 Health。首次 Shield 正值变为零由既有 CombatEvent 产生一次 `ShieldAbsorbed`/`ShieldBroken`（有溢出时同时 `HealthDamaged`）；C 不伪造第二事件，也不施加 `State.Stunned`。
- Heavy 只由稳定 `IPRAbilityTargetInterface::GetAbilityTargetMobility()` 表达。VectorHook 和后续 Boss 只能消费该接口，不能 include 或识别 `ShieldMinion` 具体类型。

# v0.2.1-D Elite ShieldBreak Response 合同

**所有者**：Enemy Prototype / Enemy Runtime；**直接消费者**：v0.2.1-E、v0.2.2 Boss、v0.2.3 HUD、v0.3.2 QTE。

- `UPREnemyPrototypeDataAsset::ShieldBreakEffect` 是受限的可选 `TSubclassOf<UGameplayEffect>` 数据字段：为空时不产生额外响应；非空时只能由已验证 Prototype 在首次 Shield 正值变为零时消费。D 的 Elite 唯一绑定既有 `/Game/ProjectR/Abilities/Effects/GE_State_Stunned`；Melee/Ranged/Shield 保持为空。它不接受字符串、路径或任意运行时类，也不改变既有默认 GE、Combat 或 AttributeSet 合同。
- Enemy 以一次性运行时标记在 Shield Attribute Change Delegate 内先锁定首次响应。若目标已具有 `State.Stunned`，响应仍被消费，但不增加、刷新或复制 Effect；正式属性恢复 Shield 后仍可按 C 合同重新获得 `State.Guarding`，但之后的破盾不再施加该 Prototype 的一次性响应。该标记、EffectHandle、Target、Token、Timer、Delegate 和 Brain 状态均不持久化。
- `State.Stunned` 的敌人语义仅在 Enemy 层消费 ASC Tag Event：添加时幂等取消当前 Enemy Attack GA/Token/攻击 Timer，停止 X/Z 移动并将 Brain 保持为 `Staggered`；存在期间不追击、撤退、HoldRange 或攻击；移除时只触发一次既有 Reevaluate，不重新授予 AbilitySet、不重放 DefaultAttributes、不重置属性或绕过 Cooldown。CombatSubsystem 不依赖 Elite 类型，ShieldBroken 仍是唯一真实 CombatEvent 事实。
- `Anchored` 与 `Heavy` 继续仅由 `GetAbilityTargetMobility()` 区分。VectorHook 对两者都只能拉动玩家至既有停距，不能 include 或识别 Elite；枚举身份保留给 Boss/下游策略消费。

# v0.2.1-E Enemy 整合与表现合同

**所有者**：Enemy Runtime / Editor-only automation；**直接消费者**：v0.2.2 Boss、v0.2.3 HUD、v0.3.2 QTE、QA。

- Enemy 表现是可选、非权威的本地附属物：Registry 安全加载阶段可预加载已验证 Attack Data 的 VFX/SFX 软引用；激活阶段绝不同步加载。未加载、空引用或零时长占位音频只记录受控 Warning 并跳过，不得改变伤害、攻击 Token、移动、Cost、Cooldown、状态、CombatEvent 或生命周期。
- `APREnemyCharacter` 的身份/状态文字、攻击短暂表现和 `APREnemyProjectile` 的无碰撞可见组件都必须在 EndPlay/攻击结束/投射物结束时清理；它们不拥有 GameplayTag、碰撞、伤害、永久 Tick 或保存状态。Enemy 仍只依赖 Target、Combatant、AttackProxy、Mobility、CombatEvent 和 Prototype 窄合同。
- `RunPIEEnemyIntegrationSmoke()` 与 `RunPIEEnemyHumanFeelSequence()` 是固定无参、Editor-only 入口：只在 `L_CombatGym` 的 Authority PlayWorld 使用正式四项 PrototypeTag 和白名单 Spawn API，所有 Actor/Timer/Delegate/Token 在成功、失败、超时与 PIE Stop 路径清理；不接受任意外部输入，不保存地图或 UE Package。人工手感结论只能由用户提供，不能由入口伪造。

# v0.2.2 Auditor Boss 合同

**所有者**：Boss / Enemy Runtime；**建立版本**：v0.2.2；**直接消费者**：v0.2.3 HUD、v0.2.4 平衡、v0.3.2 QTE、v0.4.0 Director、v0.4.3 Run/Account、v0.7.0 审计者章节。

- `EPRAuditorBossPhase` 固定为 `Dormant`、`Sampling`、`RuleAudit`、`PredictionShield`、`Defeated`，只能单向推进。`FPRBossRuntimeState` 只包含 BossId、SpawnId、阶段、当前/最大 Health/Shield、RuleId、PredictedAbilityTag、AttackTag/Phase、PhaseSequence、Defeated 与时间；`FPRPrototypeRunResult` 只包含稳定 Completion/Boss/Spawn ID、固定 1 个 Fragment 和时间。两者不得保存 UObject、ASC、Spec、EffectHandle、Timer 或 Delegate。
- `UPRBossSubsystem` 是 Boss RuntimeState、状态事件和 `OnPrototypeRunCompleted` 的唯一所有者。Defeated 与 completion 各只发布一次；结果不进入 Save、背包、货币、账号或删除流程。`WBP_BossHealth`、`WBP_BossPrototypeResult` 只订阅原生事件/读取 RuntimeState，不能在 Widget Graph 决定阶段、伤害、规则、预测或奖励。
- Auditor 继承 `APREnemyCharacter` 并实现既有 `IPRCombatMitigationInterface`；`UPRAuditorBossComponent` 是其唯一 Boss 行为所有者。它复用 Enemy 自持 ASC/AttributeSet、DefaultAttributes、AbilitySet、AttackData、ServerTriggered Ability、Brain、X/Z 平面、CombatSubsystem 与 CombatEvent，不建立第二套 Boss 伤害、属性、死亡或 Projectile 系统。
- `IPREnemyAttackSelectionInterface` 是可选窄扩展：Enemy Brain 只接受选择器返回的、属于该 Prototype 已验证 AttackDefinitions 的 Tag；没有选择器时必须维持第一个 Attack 的既有四敌行为。Auditor 是 v0.2.2 唯一实现者。Registry 内容是唯一 Spawn 白名单，四个既有条目顺序不变，Auditor 只能是第五项；API 不接受类、对象、路径或字符串 Spawn。
- P1 仅保留最近 12 秒、最多 16 条合法 `Skill.*` 样本；P2 只使用本地确定性 `Rule.DistanceCorrection` 或 `Rule.RepetitionPenalty`，不调用 Director/LLM/JSON。P3 以频次和 Tag 字典序确定 Prediction，使用 240 Shield 的既有 GAS 资产。预测 Skill 通过正式 mitigation 返回 `Blocked`，唯一新增响应为 `Combat.Response.PredictionBlocked`，既有 CombatSubsystem 仍发布一次 `Combat.Event.DamageRejected`；Block 不改变 Health、Shield 或 Prediction GE。
- v0.2.2 不保存 Enemy/Boss Runtime、采样、Timer、Delegate、Effect、Spec、Target 或 Widget 状态；v0.4.3 若要持久化 Run/Account，只能消费 Completion 的稳定 ID/运行结果语义。

# v0.2.3 Combat HUD 合同

**所有者**：Local PlayerController / HUD；**建立版本**：v0.2.3；**直接消费者**：本地玩家；**上游**：Attribute、Ability Runtime/Lifecycle、CombatEvent、Boss Runtime/Completion。

- `APRCombatHUD` 是唯一 HUD Actor 所有者：仅在本地 `APRPlayerController` 上创建一次 `UPRCombatHUDWidget`，不在 Authority/Server 创建，不向 Widget 赋予游戏玩法权威；EndPlay、Widget destruct、Pawn 替换和 World cleanup 必须解除每一个 Delegate 并清理 Widget 自有 Timer。
- `UPRPlayerResourcesWidget` 先读取 AttributeSet 快照，再只订阅属性变更；`UPRSkillBarWidget` 只读取已授予的 `DA_DefaultAbilitySet` 顺序和 `FPRAbilityRuntimeState`，六个显示名由固定 `Skill.*` 映射产生，绝不以 raw tag 文本代替可读名称。
- `UPRCombatFeedbackWidget` 只订阅 `FPRCombatEvent` 和 Ability Lifecycle 失败事件。EventId 一次消费；同一 Source/Target/Ability/Semantic 在 0.2 秒内可合并；最多三项、2.5 秒自有 Timer；死亡和激活/Commit 失败不合并。它不得调用 Damage、Effect、Ability、Spawn、Save、Reward、Director 或其他权威 API。
- Boss 区只复用 `UPRBossHealthWidget` 和 `UPRBossPrototypeResultWidget` 的只读 Boss Runtime/Completion 绑定；`ActiveRuleId` 必须标注为 Auditor local rule，而不是 Director rule。Boss Widget 不能决定阶段、预测、奖励或持久化。
- 运行时 UI、Delegate、Timer、反馈条目、Ability RuntimeState、Boss RuntimeState、Spec/Effect Handle、Target 和 UObject 引用都不得进入 SaveGame。v0.2.4 只能调整既有 DataAsset 数值/表现软引用；v0.3.2、v0.4.0、v0.4.3 只能消费这些稳定事件/状态面，不能反向依赖 HUD。

# 10. 契约升级清单

1. 列出所有直接消费者。
2. 判断能否仅新增字段/Tag/可选接口。
3. 若必须破坏，记录 ADR 和兼容期。
4. 更新 SaveVersion/SchemaVersion 和迁移器。
5. 添加旧数据、旧 Blueprint、旧资产引用的失败测试。
6. 运行当前版本和直接下游版本冒烟测试。
7. 更新版本依赖矩阵后才能关闭变更。
