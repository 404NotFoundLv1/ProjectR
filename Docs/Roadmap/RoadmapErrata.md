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
| E-012 | v0.0.5 的 Allowed paths 只列出 `Content/ProjectR/MCPTest/**`；复制 World Partition 测试地图可能同时创建 `Content/__ExternalActors__/ProjectR/MCPTest/**` 和 `Content/__ExternalObjects__/ProjectR/MCPTest/**` | 在任何地图写入前完成只读碰撞与能力审计；若确定使用 World Partition Builder，先把准确 External Package roots、逐图清单、保存范围和失败隔离规则写入当期任务合同并重新批准 | Pending v0.0.5 startup audit |
| E-013 | v0.0.5 的人工步骤写有“Save All 后重启”，与 `AGENTS.md`、CodexWorkflow 和 UnrealMCPAuthoring 的 Manifest 精确保存、禁止 Save All 合同冲突 | 改为“Manifest 中所有准确 Package 分别保存、Blueprint 编译通过、意外 Dirty Package 为 0 后重启”；不得以 Save All 扩大写入范围 | Pending v0.0.5 startup audit |

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
