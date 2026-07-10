---
title: "ProjectR 系统架构与版本依赖图"
subtitle: "整体逻辑、模块所有权、数据流和 v0.0.0-v1.0.4 防返工契约"
version: "v1.0"
date: "2026-07-10"
---

# 系统架构总则
本文件是跨版本关系的事实源。Codex 在修改任何玩法模块前，必须先读取当前版本行、直接上游版本和列出的下游消费者。
## 依赖方向
```text
Core / Tags / Settings
  -> Input / GAS / CombatEvent / Save
  -> Skills / Enemies / Boss Prototype / HUD
  -> Companion / Relationship / QTE / Dialogue Events
  -> PlayerProfile / Director Rules / Rooms / Run State
  -> Account Deletion / Meta Progression / Reality Hub / Memory
  -> Chapter Content
  -> Economy / Accessibility / Localization / Steam
  -> QA / Performance / Release
```
反向协作只能通过 Interface、Delegate、Subsystem API、GameplayTag 或 DataAsset Schema。Combat 不得 include 具体 QTE、Director Rule、章节或 UI 类型；这些系统订阅 CombatEvent。
## 运行时数据流
```text
Player Input -> AbilitySystem -> CombatEvent -> Enemy/Player State
                                      |-> HUD
                                      |-> QTE Trigger Evaluation
                                      |-> PlayerProfile Aggregation
                                      |-> Companion Dialogue Selection

Run Start -> Profile Snapshot -> Director Provider -> Validator -> Local Rule Registry
                                                        |-> Apply deterministic effects
                                                        |-> Display reason/counterplay

Run End -> AccountRecord + RunSummary -> SaveGame
                                      |-> Graveyard
                                      |-> Companion Memory
                                      |-> Quest/Meta/Steam consumers
```
## 模块所有权
| 模块 | 拥有的数据/行为 | 禁止承担 |
|---|---|---|
| Core | 生命周期、DeveloperSettings、日志、全局入口 | 具体技能/法令/章节逻辑 |
| Combat | 伤害、命中、死亡、CombatEvent | UI、对话、画像策略 |
| Abilities | Ability、成本、冷却、输入 Tag | 房间/章节/长期存档 |
| Companions | 队友实体、支援、关系运行时 | 首脑规则执行 |
| QTE | 候选、请求、响应、QTEResult | 直接改长期存档 |
| Director | 画像快照、Provider、Validator、Rule 应用 | 高频战斗网络调用 |
| Roguelike | RunState、Room、Reward、AccountRecord | 队友人设和对话生成 |
| Save | 版本化持久化和迁移 | 决定玩法规则 |
| UI | 展示与输入转发 | 核心规则、伤害、存档迁移 |
| Platform | Steam facade 和降级 | 业务系统中的散落 Steam API |

## 稳定跨版本合同
| 建立版本 | 合同 | 主要消费者 |
|---|---|---|
| v0.1.2 | CombatEvent | 技能、敌人、HUD、QTE、画像、台词、统计 |
| v0.1.3 | Ability/AbilitySet/InputTag | 六技能、构筑、QTE、AI 连携 |
| v0.1.4 | SaveGame/SaveVersion | 关系、墓园、任务、Meta、设置、Steam Cloud |
| v0.3.0 | Companion/Relationship | AI 实体、QTE、救援、任务、结局 |
| v0.3.2 | QTEResult | 关系、画像、法令反制、Boss、记忆 |
| v0.4.0 | Director Request/Response/Rule | 法令、房间、Boss、章节、对话 |
| v0.4.2 | Room/Reward | 章节、经济、难度 |
| v0.4.3 | RunSummary/AccountRecord | 墓园、现实据点、记忆、任务、成就 |

## 公共版本依赖矩阵
| 版本 | 目标/Produces | Depends on | Consumed by | Extension points | Must remain compatible |
|---|---|---|---|---|---|
| v0.0.0 创建 Steam 版 UE 工程与仓库 | 把已经存在的 UE5.8 模板工程转化为可回滚、可审计的正式开发基线。 | UE5.8 模板工程与已连接 MCP | v0.0.1 | 配置、接口骨架、验证脚本、MCP Authoring 入口 | 工程名、模块名、目录、构建入口、GameplayTag 命名 |
| v0.0.1 制定工程规范、命名规则和 Codex 协作规则 | 避免后续多人/多月开发中结构失控。 | v0.0.0 | v0.0.2 | 配置、接口骨架、验证脚本、MCP Authoring 入口 | 工程名、模块名、目录、构建入口、GameplayTag 命名 |
| v0.0.2 接入基础模块依赖和 GameplayTags | 建立完整游戏所需的基础插件和标签体系。 | v0.0.1 | v0.0.3 | 配置、接口骨架、验证脚本、MCP Authoring 入口 | 工程名、模块名、目录、构建入口、GameplayTag 命名 |
| v0.0.3 创建核心地图与 Game Framework | 建立主菜单、现实据点、网络关卡、测试场的基本流转。 | v0.0.2 | v0.0.4 | 配置、接口骨架、验证脚本、MCP Authoring 入口 | 工程名、模块名、目录、构建入口、GameplayTag 命名 |
| v0.0.4 建立自动化构建脚本和日志目录 | 为长期开发准备可重复编译、打包和 QA 记录。 | v0.0.3 | v0.0.5 | 配置、接口骨架、验证脚本、MCP Authoring 入口 | 工程名、模块名、目录、构建入口、GameplayTag 命名 |
| v0.0.5 建立 Unreal MCP 安全资产生产基线 | 把已连接的 UE5.8 Unreal MCP 变成可审计、可回滚、可验证的正式开发能力。 | v0.0.4 | v0.1.0 | 配置、接口骨架、验证脚本、MCP Authoring 入口 | 工程名、模块名、目录、构建入口、GameplayTag 命名 |
| v0.1.0 实现 Enhanced Input 和 2.5D 移动 | 完成基础操作与横版约束。 | v0.0.5 | v0.1.1 | Delegate、Subsystem API、版本化 Schema、Debug handler registry | CombatEvent、Ability 输入、SaveVersion、基础 Blueprint API |
| v0.1.1 接入 GAS：ASC、AttributeSet、默认属性 | 建立完整战斗数值基础。 | v0.1.0 | v0.1.2 | Delegate、Subsystem API、版本化 Schema、Debug handler registry | CombatEvent、Ability 输入、SaveVersion、基础 Blueprint API |
| v0.1.2 统一伤害、死亡、复活和受击表现 | 所有技能、敌人、法令走同一伤害入口。 | v0.1.1 | v0.1.3, v0.2.0, v0.2.1, v0.3.2, v0.3.3, v0.4.0, v0.9.1 | Delegate、Subsystem API、版本化 Schema、Debug handler registry | CombatEvent、Ability 输入、SaveVersion、基础 Blueprint API |
| v0.1.3 技能授予、冷却、资源消耗和输入 Tag | 把所有技能纳入数据驱动 AbilitySet。 | v0.1.2 | v0.1.4, v0.2.0, v0.3.2, v0.4.2, v0.4.4 | Delegate、Subsystem API、版本化 Schema、Debug handler registry | CombatEvent、Ability 输入、SaveVersion、基础 Blueprint API |
| v0.1.4 本地 SaveGame 与版本化存档结构 | 从早期开始避免存档结构混乱。 | v0.1.3 | v0.1.5, v0.3.0, v0.4.3, v0.4.4, v0.5.1, v0.8.2, v0.8.4 | Delegate、Subsystem API、版本化 Schema、Debug handler registry | CombatEvent、Ability 输入、SaveVersion、基础 Blueprint API |
| v0.1.5 Debug 菜单、日志分类和开发命令 | 提高后续系统验证效率。 | v0.1.4 | v0.2.0 | Delegate、Subsystem API、版本化 Schema、Debug handler registry | CombatEvent、Ability 输入、SaveVersion、基础 Blueprint API |
| v0.2.0 实现 6 个 P0 玩家技能 | 完成主角商业版可扩展技能基础。 | v0.1.5 | v0.2.1 | QTE 触发事件、画像采样事件、HUD 数据源 | 技能/敌人 DataAsset、伤害入口、Boss 原型事件 |
| v0.2.1 敌人基类、三种小怪和精英 | 建立可扩展敌人生态。 | v0.2.0 | v0.2.2 | QTE 触发事件、画像采样事件、HUD 数据源 | 技能/敌人 DataAsset、伤害入口、Boss 原型事件 |
| v0.2.2 审计者 Boss 纵切片 | 完成第一个可演示 Boss。 | v0.2.1 | v0.2.3 | QTE 触发事件、画像采样事件、HUD 数据源 | 技能/敌人 DataAsset、伤害入口、Boss 原型事件 |
| v0.2.3 战斗 HUD 与技能反馈 | 完成可读性第一轮。 | v0.2.2 | v0.2.4 | QTE 触发事件、画像采样事件、HUD 数据源 | 技能/敌人 DataAsset、伤害入口、Boss 原型事件 |
| v0.2.4 第一轮手感和平衡 | 让纵切片能玩而不是只有功能。 | v0.2.3 | v0.3.0 | QTE 触发事件、画像采样事件、HUD 数据源 | 技能/敌人 DataAsset、伤害入口、Boss 原型事件 |
| v0.3.0 AI 队友数据、关系和主同步结构 | 创建三名 AI 的长期关系底层。 | v0.2.4 | v0.3.1, v0.3.2, v0.3.4, v0.5.1, v0.7.1 | 救援策略、任务 Hook、Director counter 事件 | Companion、Relationship、QTEResult、DialogueEvent |
| v0.3.1 AI 实体、跟随、支援和压血不击杀 | 让 AI 成为战斗伙伴但不代打。 | v0.3.0 | v0.3.2 | 救援策略、任务 Hook、Director counter 事件 | Companion、Relationship、QTEResult、DialogueEvent |
| v0.3.2 12 个 P0 QTE 完整实现 | 完成三 AI 差异化连携。 | v0.3.1 | v0.3.3, v0.4.0, v0.4.1, v0.5.2, v0.7.2 | 救援策略、任务 Hook、Director counter 事件 | Companion、Relationship、QTEResult、DialogueEvent |
| v0.3.3 动态交流与战斗短句系统 | 让 AI 在战斗和房间间隙有陪伴感。 | v0.3.2 | v0.3.4 | 救援策略、任务 Hook、Director counter 事件 | Companion、Relationship、QTEResult、DialogueEvent |
| v0.3.4 濒死保护与分歧缓存区完整化 | 把死亡惩罚转化为情感选择。 | v0.3.3 | v0.4.0 | 救援策略、任务 Hook、Director counter 事件 | Companion、Relationship、QTEResult、DialogueEvent |
| v0.4.0 玩家画像与首脑法令白名单 | 构建 AI 原生规则肉鸽核心。 | v0.3.4 | v0.4.1, v0.4.2, v0.6.0, v0.6.1, v0.6.2, v0.7.0, v0.7.1 | 章节内容注册、经济权重、Meta 消费接口 | DirectorRule、RunState、Room、Reward、AccountRecord |
| v0.4.1 实现 12 条可执行法令 | 让规则真实改变战斗、奖励和事件。 | v0.4.0 | v0.4.2 | 章节内容注册、经济权重、Meta 消费接口 | DirectorRule、RunState、Room、Reward、AccountRecord |
| v0.4.2 房间系统、事件房和奖励插件 | 完整一轮肉鸽结构。 | v0.4.1 | v0.4.3, v0.6.0, v0.6.1, v0.6.2, v0.7.0, v0.7.1, v0.8.0 | 章节内容注册、经济权重、Meta 消费接口 | DirectorRule、RunState、Room、Reward、AccountRecord |
| v0.4.3 账号创建、死亡删号、通关删号和墓园 | 主题闭环完整化。 | v0.4.2 | v0.4.4, v0.5.0, v0.5.2, v0.8.4, v0.9.3 | 章节内容注册、经济权重、Meta 消费接口 | DirectorRule、RunState、Room、Reward、AccountRecord |
| v0.4.4 Meta 成长：技能树、AI 树、羁绊树 | 建立完整版长期成长。 | v0.4.3 | v0.5.0 | 章节内容注册、经济权重、Meta 消费接口 | DirectorRule、RunState、Room、Reward、AccountRecord |
| v0.5.0 现实据点完整 UI 和交互终端 | 把局外变成长期关系承接空间。 | v0.4.4 | v0.5.1 | 章节剧情、结局变量、长期摘要迁移 | Hub terminal、Quest、MemorySummary、Dialogue Provider |
| v0.5.1 三名 AI 个人任务第一章 | 让队友不只是功能系统。 | v0.5.0 | v0.5.2 | 章节剧情、结局变量、长期摘要迁移 | Hub terminal、Quest、MemorySummary、Dialogue Provider |
| v0.5.2 记忆摘要与 LLM 对话安全层 | 让长期记忆可扩展且不失控。 | v0.5.1 | v0.6.0 | 章节剧情、结局变量、长期摘要迁移 | Hub terminal、Quest、MemorySummary、Dialogue Provider |
| v0.6.0 第一章：配给官内容包 | 完成商业版第一个章节。 | v0.5.2 | v0.6.1 | 后续章节复用的内容包接口 | 章节内容 ID、Boss/Enemy/Rule 注册合同 |
| v0.6.1 第二章：守夜者内容包 | 完成风险预防主题章节。 | v0.6.0 | v0.6.2 | 后续章节复用的内容包接口 | 章节内容 ID、Boss/Enemy/Rule 注册合同 |
| v0.6.2 第三章：抚慰者内容包 | 完成情绪稳定主题章节。 | v0.6.1 | v0.7.0 | 后续章节复用的内容包接口 | 章节内容 ID、Boss/Enemy/Rule 注册合同 |
| v0.7.0 第四章：审计者完整版 | 把 Demo Boss 扩展为正式章节。 | v0.6.2 | v0.7.1 | 成就、统计和发布文案数据 | 终章变量、TripleResonance、结局输入 |
| v0.7.1 第五章：首脑意志投影终章 | 完成 1.0 主线结局。 | v0.7.0 | v0.7.2 | 成就、统计和发布文案数据 | 终章变量、TripleResonance、结局输入 |
| v0.7.2 三重共鸣和后期高光技能 | 补齐宣传片级玩法高潮。 | v0.7.1 | v0.8.0 | 成就、统计和发布文案数据 | 终章变量、TripleResonance、结局输入 |
| v0.8.0 经济、掉落、商店和插件池扩展 | 形成长期可刷的构筑深度。 | v0.7.2 | v0.8.1 | QA 自动化、云存档兼容、发布检查 | Economy、Settings、Localization、Platform facade |
| v0.8.1 难度曲线、辅助模式和新手保护 | 降低商业版流失。 | v0.8.0 | v0.8.2 | QA 自动化、云存档兼容、发布检查 | Economy、Settings、Localization、Platform facade |
| v0.8.2 设置、手柄、键位重绑定和 UI 导航 | 满足 Steam 基础体验。 | v0.8.1 | v0.8.3 | QA 自动化、云存档兼容、发布检查 | Economy、Settings、Localization、Platform facade |
| v0.8.3 本地化管线和文本审查 | 为中英发布做准备。 | v0.8.2 | v0.8.4 | QA 自动化、云存档兼容、发布检查 | Economy、Settings、Localization、Platform facade |
| v0.8.4 Steamworks 接入：成就、云存档、Rich Presence | 完成平台功能第一轮。 | v0.8.3 | v0.9.0 | QA 自动化、云存档兼容、发布检查 | Economy、Settings、Localization、Platform facade |
| v0.9.0 性能预算和第一轮优化 | 确保低配置 PC 可运行。 | v0.8.4 | v0.9.1 | RC/Shipping Gate 和热修输入 | 性能预算、QA Matrix、ReleaseBlocker、Save migration |
| v0.9.1 QA 测试矩阵和自动冒烟测试 | 系统性发现崩溃和流程断点。 | v0.9.0 | v0.9.2 | RC/Shipping Gate 和热修输入 | 性能预算、QA Matrix、ReleaseBlocker、Save migration |
| v0.9.2 Content Complete 冻结和缺口清单 | 停止膨胀，进入修复与打磨。 | v0.9.1 | v0.9.3 | RC/Shipping Gate 和热修输入 | 性能预算、QA Matrix、ReleaseBlocker、Save migration |
| v0.9.3 Beta Bug 修复、存档兼容和崩溃处理 | 让外部测试版本稳定。 | v0.9.2 | v0.9.4 | RC/Shipping Gate 和热修输入 | 性能预算、QA Matrix、ReleaseBlocker、Save migration |
| v0.9.4 音频、视觉、震动和可读性最终打磨 | 提高商业版观感。 | v0.9.3 | v1.0.0 | RC/Shipping Gate 和热修输入 | 性能预算、QA Matrix、ReleaseBlocker、Save migration |
| v1.0.0 Steam 商店页材料、Demo 和 Press Kit | 完成发布前市场材料。 | v0.9.4 | v1.0.1 | 1.1 路线和长期维护接口 | BuildId、发布分支、KnownIssues、Hotfix policy |
| v1.0.1 Release Candidate 打包和安装测试 | 生成候选发布包。 | v1.0.0 | v1.0.2 | 1.1 路线和长期维护接口 | BuildId、发布分支、KnownIssues、Hotfix policy |
| v1.0.2 最终 Bug 修复和 1.0 分支冻结 | 只修阻断，不加功能。 | v1.0.1 | v1.0.3 | 1.1 路线和长期维护接口 | BuildId、发布分支、KnownIssues、Hotfix policy |
| v1.0.3 正式发布和首周热修流程 | 上线后快速响应严重问题。 | v1.0.2 | v1.0.4 | 1.1 路线和长期维护接口 | BuildId、发布分支、KnownIssues、Hotfix policy |
| v1.0.4 发布后 1.1 路线收敛 | 根据数据决定后续扩展，而不是盲目加内容。 | v1.0.3 | 1.1 路线 | 1.1 路线和长期维护接口 | BuildId、发布分支、KnownIssues、Hotfix policy |

## 契约变更流程
1. 在 ArchitectureDecisions 记录原因、选项、影响版本和最终选择。
2. 为旧 Blueprint API、资产路径和序列化字段提供兼容期或迁移器。
3. 更新本矩阵的消费者和兼容列。
4. 增加能在旧数据/旧资产上失败的回归测试，再实施变更。
5. 通过 Build、资产验证、旧 Save 迁移和直接下游冒烟测试后才能推进版本。

## Future Compatibility Review
每个版本完成时检查：下游消费者是否仍能通过稳定契约接入；是否新增了具体未来类型依赖；是否静默更名 GameplayTag/Blueprint API/资产路径；是否修改序列化而未升级 SaveVersion；是否把主观内容写死进基础设施。发现任一问题时，当前版本不得关闭。
