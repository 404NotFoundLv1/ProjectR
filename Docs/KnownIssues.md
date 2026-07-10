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
| KI-005 | 通用 MCP 没有直接创建空白关卡 Tool；v0.0.3 已验证官方 World Partition Builder 复制路径 | 新地图仍需批准模板复制或未来专用 Tool | v0.0.5 完成安全资产生产冒烟；必要时创建专用 Tool | Open |
| KI-006 | UE MCP 为 Experimental | Tool Schema/API 可能变化 | 每次引擎升级重新审计；不作为运行时依赖 | Accepted Risk |
| KI-007 | 首次 Build、自动 PIE 与移动/镜头/跳跃人工验收均已记录 | 主观操作门已有可审计结果 | Build 退出 0；自动 PIE 无新增阻断日志；ManualOperationsRunbook 第 6 节三项 PASS | Closed |
| KI-008 | 当前没有版本化 SaveGame | 后续关系/墓园易返工 | v0.1.4 建立合同与迁移测试 | Planned |
| KI-009 | 误暂存的生成物仍作为不可达 loose objects 占用 `.git`，审计时约 1.34 GiB | 本地仓库占用偏大，但不影响工作树或正确索引 | 首次基线提交后另行批准并验证安全 GC；本版本禁止主动清理 | Open |
| KI-010 | 无 HEAD 的首次基线上，`git diff --cached --check` 报告模板 Source/Config 和既有文档中的尾空格/EOF 空行 | 全量 cached whitespace gate 为 FAIL；不影响 Build/PIE，但阻止把静态门写成通过 | 在允许修改各路径且有兼容审查的独立卫生变更中建立格式基线；v0.0.0 不越权批量改写 | Open |

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

# 记录规则

问题必须包含复现/证据、影响版本、临时处理、负责人和可验证关闭条件。不能用删除日志或降低校验来关闭问题。
