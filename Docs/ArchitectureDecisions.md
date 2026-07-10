---
title: "ProjectR Architecture Decisions"
subtitle: "不可逆或跨版本决策记录"
version: "v1.0"
date: "2026-07-10"
---

# ADR-001 - CodexReady 文档体系

**状态**：Accepted。  
**决策**：设计案为产品事实源；架构图/接口合同管理跨版本关系；路线图管理公共版本；CURRENT_VERSION + 独立任务合同管理当前执行。Markdown 是编辑事实源，PDF 是归档。

# ADR-002 - 类型与资产命名

**状态**：Accepted。  
**决策**：APR/UPR/FPR/EPR；文件 PR；资产 BP/WBP/DA/GE/GA/VFX/SFX/L/IA/IMC。模板类先迁移引用，不直接破坏性重命名。

# ADR-003 - 运行时确定性与 LLM 边界

**状态**：Accepted。  
**决策**：战斗、伤害、敌人、QTE、法令效果和存档本地执行。LLM 仅产生白名单结构化意图和表达，经过 Validator 并有 Mock/离线回退。

# ADR-004 - CombatEvent 作为跨系统事件面

**状态**：Accepted。  
**决策**：QTE、HUD、画像、AI 台词和统计消费 CombatEvent；Combat 不依赖这些具体系统。

# ADR-005 - 版本化 SaveGame

**状态**：Accepted。  
**决策**：从首次实现开始包含 SaveVersion 和迁移器；Steam Cloud 不建立第二套 Schema。

# ADR-006 - Unreal MCP 自动化优先

**状态**：Accepted。  
**决策**：Editor 机械工作默认由 Codex MCP 完成；能力顺序为通用 Toolset、官方专用 Toolset、ProjectR Toolset、人工例外。二进制只由 Editor API 写入，操作串行并完整验证。

# ADR-007 - 公共版本与内部微任务

**状态**：Accepted。  
**决策**：保留 v5 公共版本编号并新增 v0.0.5；过大版本拆为不发布的字母微任务，每个微任务可编译回滚。

# ADR-008 - Git 基线与 UE Package 使用 LFS

**状态**：Accepted。
**上下文**：首次索引误含 Binaries、Intermediate、Saved、DerivedDataCache、IDE 数据库和临时渲染产物；Content 中 749 个 `.uasset` 与 4 个 `.umap` 尚未配置 LFS。
**选项**：继续普通 Git Blob；忽略 Content；只对 UE Package 启用 LFS。
**决策**：忽略所有可再生生成目录和本地客户端状态；保留工程、Source、Content、Config、Docs、BuildScripts、`.mcp.json` 与 `.vsconfig`；仅让 `.uasset`、`.umap` 走 Git LFS。
**后果**：首次基线不携带本机缓存，UE Package 路径与引用不变；远端上传仍需单独验证 LFS 认证和配额。
**影响版本/合同**：v0.0.0 建立仓库存储边界；v0.0.1 消费稳定目录和构建入口。
**迁移/回滚**：清理前用 `git write-tree` 记录索引树；需要时用不带 `-u` 的 `git read-tree <tree>` 只恢复索引。禁止以 `git clean` 或硬重置回滚。
**验证**：保护目录不在索引；全部 753 个 UE Package 由 `git lfs ls-files` 列出；工作树文件零丢失。

# ADR-009 - 文档根目录大小写冻结为 Docs

**状态**：Accepted。
**上下文**：Windows 磁盘目录实际为 `docs`，而 AGENTS、CURRENT_VERSION 和版本合同冻结路径为 `Docs/**`；大小写敏感环境会把二者视为不同目录。
**选项**：保留偏差并记录风险；修改全部合同为小写；在首次提交前只修正目录大小写。
**决策**：经用户明确批准，在首次提交前通过工作区内临时名执行 `docs` → `Docs`，不改变文件内容和内部相对路径。
**后果**：Windows、Linux、CI 和下游 v0.0.1 使用同一公共目录名；不存在已发布 Git 历史迁移。
**影响版本/合同**：v0.0.0 目录稳定合同及 v0.0.1 的 `Docs/**` Allowed paths。
**迁移/回滚**：移动前后核对 22 份文档的聚合 SHA-256；实施中止时可经同样的临时名恢复原大小写。
**验证**：实际目录名严格等于 `Docs`，文档数量和内容哈希不变，Git 索引只出现 `Docs/`。

# ADR-010 - Core DeveloperSettings 使用最小 C++ 配置契约

**状态**：Accepted。
**上下文**：v0.0.1 需要为 Debug、MockDirector 和 Steam 提供长期稳定的功能开关，但对应业务系统尚未进入实现版本；提前暴露 Blueprint API 或引用未来类型会扩大公共合同并造成反向依赖。
**选项**：只写文档；创建 Blueprint 可读设置；创建只由 C++ 和 Project Settings 使用的最小配置类。
**决策**：在 Core 创建 `UPRDeveloperSettings`，使用 `Config=Game, DefaultConfig`，自动注册为 `Game > ProjectR`。三个 Config 布尔值固定为 `bEnableDebugFeatures=false`、`bUseMockDirector=true`、`bEnableSteamFeatures=false`；不暴露 Blueprint API，不实现被开关控制的系统。由于导出的公共 Header 直接继承 `UDeveloperSettings`，`ProjectR.Build.cs` 经用户明确批准在 `PublicDependencyModuleNames` 声明 `DeveloperSettings`。
**后果**：未来 C++ 消费者通过 `GetDefault<UPRDeveloperSettings>()` 读取确定性配置；Mock 默认保持离线回退，Steam 与 Debug 默认关闭。Debug 开关不替代 Shipping 安全边界。
**影响版本/合同**：v0.0.1 建立 Core 配置入口；v0.0.2 可独立接入模块和 GameplayTags，不依赖具体 Director、Steam 或 Debug 类型。
**迁移/回滚**：当前没有 Config、Blueprint、Save 或资产消费者；需要回滚时只删除本轮类和反向撤销文档 diff，不修改 UE Package。
**验证**：ProjectREditor Build 成功；Editor 重启后 MCP 能反射类和 CDO；Project Settings 面板显示准确分组、名称和默认值。

# ADR 模板

```text
ID / Title：
Status：Proposed / Accepted / Superseded
Context：
Options：
Decision：
Consequences：
Affected versions/contracts：
Migration/rollback：
Verification：
```
