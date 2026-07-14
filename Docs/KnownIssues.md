---
title: "ProjectR Known Issues"
subtitle: "已验证缺口、风险、影响版本和关闭条件"
version: "v1.0"
date: "2026-07-10"
---

# 当前已知问题

| ID | 问题 | 影响 | 关闭条件 | 状态 |
|---|---|---|---|---|
| KI-001 | v0.0.0 首次基线提交已创建 | 正式、可共享的回滚边界已经建立 | 提交 `2bad8adb7fc2a38d27453bb07a4c91bac76e1726` 存在且版本转换前工作区干净 | Closed |
| KI-002 | 正式 APR/UPR Framework 已建立；模板与 Variant 仅保留为参考 | 正式系统不再依赖模板 Framework，后续仍需避免重新引入 | v0.0.3 父类、地图依赖和 Source 审计均无模板/Variant Framework 引用 | Closed |
| KI-003 | GAS 基础模块、PlayerState-owned ASC、AttributeSet 与默认属性 GE 已接入 | v0.1.1 基础合同已可供后续伤害和 Ability 系统消费 | v0.1.1 Build、自动化、资产回载和 PIE 全部通过 | Closed |
| KI-004 | GameplayTagsToolset 与 GASToolsets 已启用并验证；UMG/Automation/Animation 等其他专用 Toolset 尚未启用 | Tag/GAS 审计可用，其他对应 Editor 自动化能力未承诺 | 相关版本前选择性启用并重审计 | Open |
| KI-005 | 通用 MCP 没有直接创建空白关卡 Tool；官方 World Partition Builder 已完成受控复制、External Package 枚举、重载和 PIE | 已有安全模板复制路径，不阻断资产生产基线 | v0.0.5 Builder、精确保存、重启回载与 PIE 全部通过 | Closed |
| KI-006 | UE MCP 为 Experimental | Tool Schema/API 可能变化 | 每次引擎升级重新审计；不作为运行时依赖 | Accepted Risk |
| KI-007 | 首次 Build、自动 PIE 与移动/镜头/跳跃人工验收均已记录 | 主观操作门已有可审计结果 | Build 退出 0；自动 PIE 无新增阻断日志；ManualOperationsRunbook 第 6 节三项 PASS | Closed |
| KI-008 | 版本化 SaveGame、PRSV、A/B 平台槽和迁移基础已建立 | 后续关系/墓园可以增量扩展同一 Schema，不再需要重建物理格式 | v0.1.4 Build、五项 Save 自动化、平台测试槽隔离/清理、历史回归和 PIE 全部通过 | Closed |
| KI-009 | 误暂存的生成物仍作为不可达 loose objects 占用 `.git`，审计时约 1.34 GiB | 本地仓库占用偏大，但不影响工作树或正确索引 | 首次基线提交后另行批准并验证安全 GC；本版本禁止主动清理 | Open |
| KI-010 | 无 HEAD 的首次基线上，`git diff --cached --check` 报告模板 Source/Config 和既有文档中的尾空格/EOF 空行 | 全量 cached whitespace gate 为 FAIL；不影响 Build/PIE，但阻止把静态门写成通过 | 在允许修改各路径且有兼容审查的独立卫生变更中建立格式基线；v0.0.0 不越权批量改写 | Open |
| KI-011 | v0.0.5 草案的 World Partition MCP 冒烟 Allowed paths 未覆盖 External Actors/Objects，且“Save All 后重启”与精确 Package 保存合同冲突 | 合同修正前，v0.0.5 不能安全执行模板地图复制或保存/重启门 | v0.0.5 首次 MCP 写入前，任务合同列出准确 External Package roots，改为 Manifest 精确保存且 Dirty=0 后重启，并完成只读启动审计 | Closed |
| KI-012 | 默认文化下整图 DSL 反编译因本地化 Node Type ID 返回假阴性；node-level 回读证明 Graph 未丢失 | 整图 DSL 结果不能脱离 node-level/PIE 证据单独判定 Graph 持久化 | 默认文化重启后 node-level 连接与 Pin 精确匹配，PIE 输出日志标记并停止成功 | Closed |
| KI-013 | 官方 20/261 Toolset 没有键鼠/手柄 PIE 输入注入；首版自定义调用未给 `CreateSimulated` 提供实际 Viewport 而崩溃 | 无法自动证明真实 Enhanced Input 路径；错误实现会使 Editor 崩溃 | 用户批准最小 Editor-only Toolset；传入实际 `FSceneViewport`/默认 InputDevice 后键鼠和手柄序列、释放、移动、跳跃、平面、朝向和语义日志全部 PASS | Closed |
| KI-014 | 首次人工验收发现跳跃中反向输入只改变 Mesh 朝向，实际 X 速度未立即反转 | 2.5D 动作移动在空中换向时不符合已确认手感 | PIE 回归断言在旧实现 RED；同幅翻转 X 速度后自动序列连续三次 PASS；用户键鼠复验 PASS | Closed |
| KI-015 | 即时 Mesh yaw 切换满足方向正确性，但用户认为视觉转向过于生硬 | 左右快速换向的视觉手感不够自然 | 0.12 秒 Ease-In-Out 的 RED→GREEN 诊断、Build、标准 PIE 与最终用户手感全部 PASS | Closed |
| KI-016 | 官方 24/276 Tool 无法在活动 PIE 调用 C++-only CombatSubsystem；后台 Editor 约 3 FPS 会把 Timer 恢复采样量化为约 0.33 秒 | 无法客观执行统一伤害闭环，后台墙钟采样会误判 0.10 秒硬直 | 用户批准窄范围 Combat Toolset；25/277 反射、固定事件序列、World 时间采样和 Editor 前台复验得到 0.115/0.100 秒，PIE/Dirty 门通过 | Closed |
| KI-017 | 官方 ObjectTools 无法可靠写入嵌套 `FPRAbilitySetEntry[]`，官方工具也不能在 PIE 调用 C++-only AbilitySet/ASC 接口 | 无法仅靠通用 Tool 完成验证 AbilitySet配置和真实生命周期闭环 | 用户分别批准固定无参数资产配置 fallback 与 Ability PIE smoke；七资产回读、精确保存、重启和固定运行时序列全部 PASS | Closed |
| KI-018 | `-culture=en` 下官方 `read_graph_dsl` 只读调用会把已保存的验证 GA 标为 Dirty | 只读 Graph 审计可能制造意外保存提示或掩盖真实 Dirty 来源 | 引擎/Tool 修复后重审；修复前必须前后检查 Dirty，并仅在完整 node/Pin/连接回读和编译通过后精确单项保存 | Accepted Risk |
| KI-019 | `ProjectRAuthoringTools.Build.cs` 已依赖 GameplayAbilities/GameplayTags，但 `.uplugin` 因 v0.1.3 Forbidden path 未同步插件依赖声明 | UBT 持续输出插件元数据警告；当前 Editor-only 模块仍能构建和加载 | 后续明确允许 `.uplugin` 时补充准确插件依赖，并通过 Build、Editor 重启、26/279 Tool 反射及 Shipping Runtime 不加载验证 | Open |
| KI-020 | 既有 Combat PIE smoke 在严格 Timer 失败路径不能完整恢复临时属性/状态，且后台节流会放大采样 | 失败诊断后的同一 PIE 会话不能继续作为可靠 Combat 基线 | 在允许修改既有 Combat Tool 的维护版本补齐所有失败路径回滚；当前必须 StopPIE、新会话、前台/浮动 PIE 后复测 | Open |
| KI-021 | 本机旧 NVIDIA 566.26 驱动在打包程序默认 D3D12/Ray Tracing 启动时于 `nvgpucomp64.dll` 崩溃，Windows 异常 `0xc0000409`；一次运行还记录 D3D12 PSO `0x8007000e` | 旧驱动曾阻断 Development/Shipping 默认图形路径验收 | 用户更新到 NVIDIA 610.47 后，在无其他 UE 进程的干净条件下重新运行默认 D3D12 Development/Shipping 顶层入口、F1/输入烟测和新时间窗日志，均未复现崩溃 | Closed |

# 2026-07-10 - v0.0.1 Known Issues Review

- 未发现由 `UPRDeveloperSettings`、规范文档或模块依赖引入的持续性问题。
- 首次正式 Build 的 Live Coding 阻断通过正常关闭 ProjectR Editor 消除；随后发现的 `UDeveloperSettings` 链接错误由经批准的直接 `DeveloperSettings` 公共模块依赖修复，最终 Build 退出 0，因此不登记为未关闭 KI。
- KI-002、KI-003、KI-004、KI-006、KI-008、KI-009、KI-010 的范围和状态不变；本版本未提前处理模板迁移、GAS、专用 Toolset、SaveGame 或 Git GC。

# 2026-07-10 - v0.0.2 Known Issues Review

- 未发现由模块下限、53 个 GameplayTags、`UPRTagLibrary` 或 GameplayTagsToolset 引入的持续性阻断问题，因此不虚构新 KI。
- KI-003 更新为基础模块已经接入且 Build 通过；ASC、AttributeSet、GameplayAbility/Effect 仍按 v0.1.1 完成，保持 Open。
- KI-004 更新为 GameplayTagsToolset 已启用并以 6 个工具完成只读验证；其他专用 Toolset 继续按相关版本选择性启用，保持 Open。
- KI-006 保持 Accepted Risk；本轮观察到一次 Editor 重启后的旧 MCP session-id 错误，客户端重新初始化后所有只读调用成功，不构成新问题。
- KI-002、KI-005、KI-008、KI-009、KI-010 的范围和状态不变；未删除模板 Variant、创建地图/SaveGame、运行 GC 或执行无关格式清理。

# 2026-07-10 - v0.0.3 Known Issues Review

- KI-002 关闭：七个正式原生类、八个 Blueprint 父类、五张地图 GameMode 和依赖审计均未引用模板/Variant Framework；模板资产与 Variant 文件原样保留。
- KI-005 保持 Open：UE5.8 官方 `WorldPartitionRenameDuplicateBuilder` 已安全创建并验证五张地图，但通用 MCP 仍没有空白关卡创建 Tool，v0.0.5 继续完成通用资产生产基线。
- KI-006 保持 Accepted Risk：中文文化下 Graph DSL 的英文事件 ID 不匹配、`CaptureViewport` 的可选参数实际需显式传 null，以及新 External Actor 首次保存的 AssetRegistry 时序差异均已记录；这些工具问题不进入运行时或 UE Package 公共合同。
- 未发现由正式 Framework、地图旅行、固定相机或 DefaultEngine 设置引入的持续性阻断问题，因此不虚构新 KI。
- KI-003、KI-004、KI-008、KI-009、KI-010 的状态不变；本版本未提前实现 GAS、Save、专用未来 Toolset、打包脚本或 GC。

# 2026-07-11 - v0.0.4 Known Issues Review（Completed）

- 新增 KI-011：v0.0.5 草案的 World Partition 测试地图可能产生 `__ExternalActors__`/`__ExternalObjects__` Package，但当前 Allowed paths 未覆盖；同时“Save All 后重启”与工程的 Manifest 精确保存、禁止 Save All 合同冲突。两项都留到 v0.0.5 启动审计修正，本版本不改任务页、不创建 MCPTest 资产。
- 初次磁盘门禁 65 已由用户释放系统卷空间后消除；真实 BuildEditor `v004-final-build-19f4ed6c55e` 与 Development Package `v004-actual-package-19f4ece2a19` 均 PASS，未形成长期问题。
- 最终脚本快照的完整合同测试与独立复审均实际退出 0，Spec compliance PASS、Code quality Approved；真实 Build/Package 及产物后置条件也已通过，v0.0.4 可关闭并接受 ADR-013。
- KI-003、KI-004、KI-005、KI-006、KI-008、KI-009、KI-010 的状态不变；本版本不实现 GAS、Save、MCP Toolset、资产写入或 Git GC。

# 2026-07-11 - v0.0.5 Known Issues Review（Completed）

- KI-011 关闭：只读审计确认源 World Partition 地图包含 65 个 External Actor 与 2 个 External Object Package；v0.0.5 Allowed paths 已增加两个准确 External roots，并把 Save All 改为 Blueprint 编译、Manifest 非空精确保存、全量意外 Dirty=0 后重启。合同静态门在首次 Package 写入前单独执行。
- KI-004 保持 Open：当前 20/261 个官方工具已覆盖本版本需要的资产能力，无需启用其他专用 Toolset。
- KI-005 关闭：官方 World Partition Builder、External Package 数量/哈希、精确保存、默认文化重启回载和 PIE 均取得实际 PASS；本版本不需要空白地图 Tool 或自定义插件。
- KI-006 保持 Accepted Risk：Graph DSL 文化差异、External Actor 首次保存时序和 Experimental Schema 均通过已记录的停止点与回读门约束。
- 启动实施发现 UE5.8 的 `UPrimaryDataAsset` 为 Abstract：工具可创建内存对象，但 SavePackage 明确拒绝。用户批准只丢弃本轮两个未落盘测试对象；合同改用具体 `UPrimaryAssetLabel`。该失败未删除或覆盖磁盘文件，属于已定位并当期修正的类型选择错误，不新增长期 KI。
- KI-012 关闭：默认文化下整图 DSL 的空结果是本地化 Type ID 引起的反编译假阴性。经用户批准的一次官方 node-level 复核确认 BeginPlay、PrintString、执行连接和 Pin 值均在重启后存在；warnings-as-errors 编译、精确保存、再次默认文化重载与 PIE 日志标记全部通过。
- KI-004 保持 Open，KI-006 保持 Accepted Risk；当前 20/261 个官方工具与 Builder 已覆盖本版本，不启用额外 Toolset，也不创建 `ProjectRAuthoringTools`。

# 2026-07-11 - v0.1.0 Known Issues Review（Completed）

- KI-013 关闭：官方 Toolset 的输入注入缺口已由用户批准的单工具 Editor-only 插件补足。首次崩溃根因是 `FInputKeyEventArgs::CreateSimulated` 默认 Viewport 为 null，而 `UGameViewportClient::RemapControllerInput` 在 UE5.8 会解引用 Viewport；修复后真实 PIE 自动序列完整 PASS，崩溃未保存或修改任何 Package。
- KI-004 保持 Open：Automation、GAS、UMG、Animation 等其他专用 Toolset 仍按相关版本选择性审计；本版本不启用它们。
- KI-006 保持 Accepted Risk：ProjectR Toolset 使用实际 Schema、`PostEngineInit` 注册、严格参数和重启/PIE 回归约束 Experimental API 风险，不成为 Runtime 依赖。
- KI-003、KI-008、KI-009、KI-010 状态不变；本版本不提前实现 GAS、Save、Git GC 或全仓格式卫生。
- KI-014 已关闭：自动化证明空中 D→A 在 0.1 秒内产生约 `-44～47 cm` 实际反向位移且 Y 漂移为 0，用户键鼠复验返回 PASS。
- KI-015 已关闭：0.12 秒 Timer Ease-In-Out 完成 RED→GREEN 诊断，Build、标准 PIE、最终朝向和相机稳定通过，用户平滑转向手感返回 PASS。
- v0.1.0 没有未关闭的当前版本阻断；最终 AutomationReport 为 22/22 required PASS、1 optional NOT_RUN。任务页与 VersionIndex 已标记 Completed，`CURRENT_VERSION.md` 不自动推进。

# 2026-07-13 - v0.1.1 Known Issues Review（Completed）

- KI-003 关闭：`UPRAbilitySystemComponent`、11 项 replicated Attribute、默认属性 GE、PlayerState Owner/Character Avatar 生命周期和 Pawn 替换持久性均经 Build、原生自动化、重启回载与 PIE 验证。
- KI-004 保持 Open，但范围更新为 GASToolsets 已启用并审计：AttributeSetToolset 与 AbilitySystemInspectorToolset 的 6 个只读工具用于本版本；GameplayCueToolset 的 8 个写入/运行时工具未调用。UMG、Automation 和 Animation 等能力继续按版本启用。
- KI-006 保持 Accepted Risk：本轮通过 Tool Schema、事务化 GE 配置、完整回读、精确保存、重启回载与 Dirty 门约束 Experimental MCP 风险。
- UE5.8 在禁止修改 Config 的测试环境会输出既有 `No GameplayCueNotifyPaths were specified` 警告；原生 Clamp 测试对这一条准确已知引擎警告设置预期匹配，PIE 新时间窗没有 ProjectR/GAS 阻断错误。不新增 KI，也不降低其他错误校验。
- 本版本没有新增未关闭阻断；NetworkPIEReplication 按单机版本合同记录为 optional `NOT_RUN`。

# 2026-07-13 - v0.1.2 Known Issues Review（Completed）

- KI-016 关闭：官方能力缺口由用户批准的独立 `UPRCombatAutomationToolset` 补足；工具不保存 Package、不接受任意 Target/数值/Tag/代码。Editor 后台节流造成首次 0.33 秒采样假失败，改用 PIE World 时间并将 Editor 置前台后，两次受击恢复分别为 0.115 秒和 0.100 秒，固定七事件序列 PASS。
- Pawn 替换自动化首次发现死亡锁在 `PossessedBy` 后被 UE5.8 `DispatchRestart -> ACharacter::Restart` 恢复默认 MovementMode。正式 Character 在 `Restart` 完成后依据持续存在的 `State.Dead` 重应用锁定，原失败测试转为 GREEN；未新增第二套状态。
- KI-004 保持 Open；本版本使用既有 GASToolsets 的只读 Inspector，不调用 GameplayCue 写工具，也不启用 UMG/Automation/Animation Toolset。
- KI-006 保持 Accepted Risk；Schema 回读、事务、精确保存、重启、前台 PIE、日志和 Dirty 门约束 Experimental MCP 风险。
- 用户在固定伤害序列中完成受击手感验收并返回 PASS；最终 AutomationReport 为 29/29 required PASS。当前没有未关闭的 v0.1.2 阻断，NetworkPIEReplication 保持 optional `NOT_RUN`。

# 2026-07-13 - v0.1.3 Known Issues Review（Completed）

- KI-017 关闭：官方嵌套 AbilitySet 写入与 C++-only PIE 调用缺口由用户逐项批准的两个固定无参数工具补足；工具只操作准确验证路径/固定序列、不保存、不提供任意执行。七个资产回读、八 Package精确保存、默认文化重启和 Ability smoke 均 PASS。
- KI-018 记录为 Accepted Risk：英文文化 `read_graph_dsl` 意外标脏验证 GA。发现后先报告，再以 node/Pin/连接、warnings-as-errors 和准确单项保存闭合；没有调用 Save All，也没有修改其他 Package。
- KI-019 保持 Open：为编译 Ability Tool 经批准只修改插件 Build.cs；`.uplugin` 仍是 Forbidden path，最终 Build 成功但保留准确依赖警告。运行时 ProjectR 模块和 Shipping 依赖未改变。
- KI-020 保持 Open：后台/视口 PIE 的 0.10 秒 Combat Timer 因低至约 3 FPS 出现严格采样失败，失败路径会留下仅限 PIE 的临时状态。停止该会话后，前台 Floating PIE 严格回归为 0.1041/0.1046 秒、固定七事件和最终状态 PASS。
- KI-004 保持 Open，KI-006 保持 Accepted Risk；本版本没有启用新的官方插件或专用 Toolset。当前 26/279 来自既有 GASToolsets 和三个已批准 ProjectR Toolset。
- 截图可读且三项 PIE 冒烟通过，但 Editor 显示约 945.8 MiB 显存超预算警告；这是当前机器环境性能风险，不是 Package/玩法合同变化。新时间窗无 ProjectR Error、Ensure 或 Blueprint Runtime Error。
- 当前没有未关闭的 v0.1.3 功能阻断；KI-018/KI-019/KI-020 均有明确隔离流程，不改变本版本运行时产品合同。NetworkPIEReplication 保持 optional `NOT_RUN`。
- 最终 AutomationReport `v013-final-report-20260713a` 为 36/36 required PASS；ProjectRAuthoringToolExtension optional PASS，NetworkPIEReplication optional `NOT_RUN`。

# 2026-07-13 - v0.1.4 Known Issues Review（Completed）

- KI-008 关闭：Schema 1、严格迁移注册表、16 字节 `PRSV`/CRC、A/B 加载与异步保存、写前冲突、写后回读、active/trailing、退出弱回调和平台测试槽白名单均已实现；`ProjectR.Save` 5/5，唯一物理测试 GUID 的 `_A/_B` 已由平台 API 精确清理，Access Ledger 未出现生产槽。独立审查发现并修复了测试宏下 Production Storage 与删除入口可组合误用的风险：Production Storage 现在无清理能力，删除前还会二次匹配准确 automation generation Slot，回归证明在平台访问前拒绝生产槽，因此不保留为 Open KI。
- `ProjectR.Build.cs` 计划中的 `PlatformFeatures` 私有模块依赖在 UE5.8 首次 GREEN Build 被 UBT 以 RulesError 8 拒绝。源码核对确认 `IPlatformFeaturesModule` 位于既有 `Engine` 模块公开头，移除无效依赖后 Build 成功；最终 Build.cs 无 diff，不登记为持续 KI。
- KI-019 保持 Open：最终 Build 成功但仍输出既有 `.uplugin` 未声明 GameplayAbilities 依赖警告，本版本没有修改插件。
- KI-020 保持 Open：首次后台 In-Viewport Combat smoke 按既有模式得到 `0.3333s` 严格采样失败，已立即 StopPIE 且未沿用受污染会话。新前台 Floating PIE 为 `0.1049s/0.1004s`、固定七事件和最终 100/50/Alive PASS；该事实再次验证既有隔离流程，不修改 Combat Tool。
- KI-018 保持 Accepted Risk，KI-004 保持 Open，KI-006 保持 Accepted Risk；本轮未调用 Graph DSL、未修改或启用插件、未新增 Toolset。
- 没有创建或修改 UE Package；265 个可查询资产 Dirty=0，1190 个 Package 的路径、长度和基线 SHA-256 不变。新鲜 PIE 只记录 Save Subsystem “initialized without storage access”，未调用产品 Save API，也未访问真实用户槽。
- 本版本没有新增未关闭功能阻断；NetworkPIEReplication 与 ProjectRAuthoringToolExtension 均为 optional `NOT_RUN`。

# 2026-07-14 - v0.1.5 Known Issues Review（Completed）

- KI-021 关闭：旧 NVIDIA 566.26 下默认 D3D12/Ray Tracing 的 `nvgpucomp64.dll`/`0xc0000409` 崩溃，在用户清洁更新到 610.47 后不再复现。默认 D3D12 Development 稳定进入 CombatGym并完成完整 Debug 固定序列；默认 D3D12 Shipping 两次 F1 无面板、正式 D 键移动有效，两个新时间窗均无 Windows 崩溃事件。
- Development/Shipping Package 本身均退出 0，目标感知边界验证均退出 0；Shipping receipt、Shipping 编译动作、Stage 与 Archive 均不含 ProjectRDebug。共享/陈旧 UHT Manifest 和同轮 Development Editor 日志不再被误当作 Shipping 包含证据。
- Development Client 的首次编译失败来自既有 Editor-only Input validation 测试在 `WITH_EDITOR=0` 下仍调用 `IsDataValid`。经用户准确批准，仅为该测试增加 `WITH_EDITOR` 保护；Editor `ProjectR.Input` 仍为 3/3，Development Client 随后打包成功，运行时输入实现和资产未变。
- KI-019 保持 Open；最终 BuildEditor 仍只有既有 ProjectRAuthoringTools 插件元数据警告。KI-020 保持 Open；本轮遵守失败后 StopPIE、新前台 Floating 会话复测流程并取得 Combat PASS。Windows UI Automation 未暴露 Slate 子控件，DPI-aware Win32 注入作为计划内机械回退完成，没有转交用户。
- ADR-020 与 Debug/日志接口合同已补齐；最终 AutomationReport 以驱动更新后的默认 D3D12 新鲜证据替换旧 FAIL/NOT_RUN。v0.1.5 没有未关闭的当前版本功能阻断，KI-019/KI-020 继续按既有隔离流程保持 Open。

# 记录规则

问题必须包含复现/证据、影响版本、临时处理、负责人和可验证关闭条件。不能用删除日志或降低校验来关闭问题。
