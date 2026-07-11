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
| KI-003 | GAS 基础模块下限已接入；ASC、AttributeSet、GameplayAbility/Effect 尚未实现 | GAS 运行时玩法仍不可用 | v0.1.1 实现 ASC/AttributeSet/默认属性并构建通过 | Open |
| KI-004 | GameplayTagsToolset 已启用并验证；专用 GAS/UMG/Automation/Animation 等 Toolset 仍未启用 | Tag 自动化可用，其他对应 Editor 自动化能力未承诺 | 相关版本前选择性启用并重审计 | Open |
| KI-005 | 通用 MCP 没有直接创建空白关卡 Tool；官方 World Partition Builder 已完成受控复制、External Package 枚举、重载和 PIE | 已有安全模板复制路径，不阻断资产生产基线 | v0.0.5 Builder、精确保存、重启回载与 PIE 全部通过 | Closed |
| KI-006 | UE MCP 为 Experimental | Tool Schema/API 可能变化 | 每次引擎升级重新审计；不作为运行时依赖 | Accepted Risk |
| KI-007 | 首次 Build、自动 PIE 与移动/镜头/跳跃人工验收均已记录 | 主观操作门已有可审计结果 | Build 退出 0；自动 PIE 无新增阻断日志；ManualOperationsRunbook 第 6 节三项 PASS | Closed |
| KI-008 | 当前没有版本化 SaveGame | 后续关系/墓园易返工 | v0.1.4 建立合同与迁移测试 | Planned |
| KI-009 | 误暂存的生成物仍作为不可达 loose objects 占用 `.git`，审计时约 1.34 GiB | 本地仓库占用偏大，但不影响工作树或正确索引 | 首次基线提交后另行批准并验证安全 GC；本版本禁止主动清理 | Open |
| KI-010 | 无 HEAD 的首次基线上，`git diff --cached --check` 报告模板 Source/Config 和既有文档中的尾空格/EOF 空行 | 全量 cached whitespace gate 为 FAIL；不影响 Build/PIE，但阻止把静态门写成通过 | 在允许修改各路径且有兼容审查的独立卫生变更中建立格式基线；v0.0.0 不越权批量改写 | Open |
| KI-011 | v0.0.5 草案的 World Partition MCP 冒烟 Allowed paths 未覆盖 External Actors/Objects，且“Save All 后重启”与精确 Package 保存合同冲突 | 合同修正前，v0.0.5 不能安全执行模板地图复制或保存/重启门 | v0.0.5 首次 MCP 写入前，任务合同列出准确 External Package roots，改为 Manifest 精确保存且 Dirty=0 后重启，并完成只读启动审计 | Closed |
| KI-012 | 默认文化下整图 DSL 反编译因本地化 Node Type ID 返回假阴性；node-level 回读证明 Graph 未丢失 | 整图 DSL 结果不能脱离 node-level/PIE 证据单独判定 Graph 持久化 | 默认文化重启后 node-level 连接与 Pin 精确匹配，PIE 输出日志标记并停止成功 | Closed |

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

# 记录规则

问题必须包含复现/证据、影响版本、临时处理、负责人和可验证关闭条件。不能用删除日志或降低校验来关闭问题。
