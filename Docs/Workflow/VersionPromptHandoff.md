---
title: "ProjectR 新任务交接与版本 Prompt 工作流"
version: "1.0"
date: "2026-07-26"
---

# 用途

本文件让新的 Codex 任务无需继承旧聊天即可继续 ProjectR 的版本合同与 Prompt 生成工作。仓库、Git、当前任务合同和本文件优先于旧聊天摘要；启动时必须重新审计真实状态。

# 项目与当前快照

- 工程根目录：`E:/MyWork/ProjectR`
- UE：`5.8`；已验证本机 Engine Root：`D:/Unreal Engine 5/UE_5.8`（每次 Build 前仍须重新确认）。
- Unreal MCP：项目根 `.mcp.json` 中的 `http://127.0.0.1:8000/mcp`；只连接同一台机器上唯一的 Unreal Editor。
- 最近实施提交：`5703aaf374a1b0461498bf0afe1013bf5cc815fa`，`v0.3.4 Complete near-death rescue divergence system`。
- 最近收尾提交：`99d51d14e0e8a0bd138e998458b803769763c37a`，`docs: close v0.3.4`。
- 本文件生成时 `main` 与 `origin/main` 同步、工作区干净；新任务不得假定此快照仍成立，必须先运行 Git 审计。
- 当前版本：`v0.3.4`，状态 `Completed`。下一公共版本是 `v0.4.0`，但尚未生成任务合同，除非用户明确要求，否则不得规划或实现。

# 事实源与优先级

发生冲突时按以下顺序处理：当前用户的明确指令 → `AGENTS.md`/当前任务合同/冻结架构 → 真实代码、资产和 Git 状态 → 仓库内路线勘误与依赖图 → CodexReady v6 外部资料 → 旧 v5 历史档案。不能静默选择，应报告差异并通过 RoadmapErrata 或 ADR 解决。

## 外部 CodexReady v6（允许只读访问）

| 用途 | Markdown 事实源 | PDF 归档核对 |
|---|---|---|
| 文档包说明 | `E:/MyWork/ProjectR_Design/CodexReady_v6/README.md` | 无 |
| 产品设计 v1.0 | `E:/MyWork/ProjectR_Design/CodexReady_v6/01_Product/ProjectR_Game_Design_CodexReady_v1.0.md` | `E:/MyWork/ProjectR_Design/CodexReady_v6/01_Product/ProjectR_Game_Design_CodexReady_v1.0.pdf` |
| 系统架构与版本依赖 | `E:/MyWork/ProjectR_Design/CodexReady_v6/01_Product/ProjectR_System_Architecture_and_Version_Dependency_Map.md` | `E:/MyWork/ProjectR_Design/CodexReady_v6/01_Product/ProjectR_System_Architecture_and_Version_Dependency_Map.pdf` |
| 可执行路线图 v6.0 | `E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/ProjectR_Codex_Executable_Roadmap_v6.md` | `E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/ProjectR_Codex_Executable_Roadmap_v6.pdf` |
| 接口合同 | `E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/ProjectR_Data_and_Interface_Contracts.md` | `E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/ProjectR_Data_and_Interface_Contracts.pdf` |
| 路线勘误/决策 | `E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/ProjectR_Roadmap_Errata_and_Decisions.md` | `E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/ProjectR_Roadmap_Errata_and_Decisions.pdf` |
| 版本索引 | `E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/ProjectR_Version_Index.md` | `E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/ProjectR_Version_Index.pdf` |

Markdown 是可执行事实源；PDF 仅用于阅读、归档与视觉核对。旧 `ProjectR_Steam_Full_Codex_Executable_Roadmap_v5.md/.pdf` 已不用于生成新合同，只保留为历史档案。

## 仓库内必须读取的资料

每次接手或生成下一个版本 Prompt，按此顺序读取：

1. `AGENTS.md`
2. `CURRENT_VERSION.md`
3. `Docs/Workflow/VersionPromptHandoff.md`
4. `Docs/Roadmap/VersionIndex.md` 与当前/上一个版本任务合同
5. `Docs/Architecture/VersionDependencyMap.md` 中当前版本、直接上游与所有下游消费者
6. `Docs/Architecture/DataAndInterfaceContracts.md` 的相关章节
7. `Docs/Roadmap/RoadmapErrata.md`
8. `Docs/DevelopmentLog.md`、`Docs/KnownIssues.md`、`Docs/ArchitectureDecisions.md`
9. `Docs/Workflow/CodexWorkflow.md`；C++、资产、MCP 或 Build 工作再读对应专项流程文档
10. 当前真实 Source、Content、Config、Plugin、UE Package、MCP Toolset 和 Git 状态

# 已完成与后续路线

| 范围 | 状态 | 交付内容 |
|---|---|---|
| v0.0.0–v0.0.5 | Completed | 工程基线、规范、Tags、正式地图、构建脚本、MCP 安全资产生产 |
| v0.1.0–v0.1.5 | Completed | 输入/2.5D、GAS、伤害与复活、Ability、Save、非 Shipping Debug 基础设施 |
| v0.2.0–v0.2.4 | Completed | 六项 P0 技能、敌人/精英、Auditor Boss、HUD、首轮手感平衡 |
| v0.3.0–v0.3.4 | Completed | Companion 关系/实体/支援、12 QTE、确定性对话、一次性濒死分歧 |
| v0.4.0 | Planned | 玩家画像与首脑法令白名单；下一个待生成任务合同 |
| v0.4.1–v1.0.4 | Planned | 法令、房间与奖励、账号墓园、Meta、据点/记忆、章节、商业功能、QA 与发布 |

完整名称、前置和状态以 `Docs/Roadmap/VersionIndex.md` 为准。不得因为下游“即将需要”而提前实现 PlayerProfile、Director、Room、Account、Graveyard、Meta 或 LLM 业务。

# 已冻结的关键边界

- 依赖方向：Core → Infrastructure → Combat → Companion/QTE → Director/Roguelike → Content → Platform/Release。
- Combat 只发布 `CombatEvent`，不依赖 QTE、Dialogue、Director、章节或 UI；下游只消费稳定值型事件。
- Save 从 v0.1.4 起版本化；运行时 UObject、Handle、Timer、Delegate、Widget、Target 不得进入 Save。
- 实时战斗不等待网络或 LLM；未来 LLM 只返回经本地白名单、Schema 和数值 Clamp 验证的结构化结果。
- UE 机械工作默认由 Codex 通过 Unreal MCP 完成；禁止通过普通文件 I/O 修改 `.uasset/.umap`。
- 公共版本不得拆成 A/B/C 或中间功能版本；一个版本只在全部完成门通过并获用户授权后创建一个正式实现提交。

# 新任务的职责与固定工作流

本任务链的主要职责是为每个公共版本生成简洁、可直接复制的 Prompt，而不是依赖旧聊天持续记忆。

1. 新任务启动：只读审计上述资料与 Git，报告是否可继续；不得修改或自行推进版本。
2. 用户说“vX 已完成，规划 vY”：生成一段简洁的**计划模式 Prompt**。它只关闭 vX、创建 vY 任务合同并推进 `CURRENT_VERSION`，不实施功能，不拆分版本。
3. 用户完成文档收尾后说“生成 vY 正式实施计划 Prompt”：生成一段简洁的**计划模式 Prompt**。它只产出 vY 的可执行实施计划，不实施。
4. 用户批准实施计划后，才在单独任务中实施 vY；Codex负责 C++、MCP 资产、测试、Build、PIE 与修复。用户仅做主观体验、外部凭据和破坏性确认。
5. 完成后，如实记录实际提交、报告、NOT_RUN 项、Known Issues、ADR 和 Future Compatibility Review；除非用户明确要求，不提交、不推送、不推进下一版本。

# 新任务首条消息

```text
我接手 ProjectR 的版本合同与 Prompt 生成工作。请严格读取
Docs/Workflow/VersionPromptHandoff.md 及其中列出的仓库和 CodexReady v6 事实源，
随后只读审计 Git、CURRENT_VERSION、当前版本合同和 VersionIndex。

不要修改文件、调用 MCP 写操作、运行 Build/PIE、创建提交，
也不要自行规划或实现下一个版本。请仅报告当前状态、发现的冲突和是否已准备好等待我的下一条版本指令。
```

# 刷新规则

本文件是交接快照，不是替代审计的权威。每次版本收尾、路线来源变更、架构决策、Git 分支变化或 MCP 配置变化后，应在用户授权的文档收尾中更新相应快照；新任务仍必须重新检查真实状态。
