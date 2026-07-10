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
| KI-002 | 当前仍接近官方模板，存在多个 Variant | 正式架构和命名易混用 | v0.0.3 完成 APR Framework 迁移与依赖审计 | Open |
| KI-003 | GAS 模块尚未接入 | GameplayAbility/Effect 类当前不可用 | v0.0.2/v0.1.1 接入并构建通过 | Open |
| KI-004 | 专用 GAS/UMG/Automation/Animation 等 Toolset 未启用 | 对应 Editor 自动化能力未承诺 | 相关版本前选择性启用并重审计 | Open |
| KI-005 | 通用 MCP 没有直接创建空白关卡 Tool | 新地图需模板复制或专用 Tool | v0.0.5 冒烟验证模板复制；必要时专用 Tool | Open |
| KI-006 | UE MCP 为 Experimental | Tool Schema/API 可能变化 | 每次引擎升级重新审计；不作为运行时依赖 | Accepted Risk |
| KI-007 | 首次 Build、自动 PIE 与移动/镜头/跳跃人工验收均已记录 | 主观操作门已有可审计结果 | Build 退出 0；自动 PIE 无新增阻断日志；ManualOperationsRunbook 第 6 节三项 PASS | Closed |
| KI-008 | 当前没有版本化 SaveGame | 后续关系/墓园易返工 | v0.1.4 建立合同与迁移测试 | Planned |
| KI-009 | 误暂存的生成物仍作为不可达 loose objects 占用 `.git`，审计时约 1.34 GiB | 本地仓库占用偏大，但不影响工作树或正确索引 | 首次基线提交后另行批准并验证安全 GC；本版本禁止主动清理 | Open |
| KI-010 | 无 HEAD 的首次基线上，`git diff --cached --check` 报告模板 Source/Config 和既有文档中的尾空格/EOF 空行 | 全量 cached whitespace gate 为 FAIL；不影响 Build/PIE，但阻止把静态门写成通过 | 在允许修改各路径且有兼容审查的独立卫生变更中建立格式基线；v0.0.0 不越权批量改写 | Open |

# 2026-07-10 - v0.0.1 Known Issues Review

- 未发现由 `UPRDeveloperSettings`、规范文档或模块依赖引入的持续性问题。
- 首次正式 Build 的 Live Coding 阻断通过正常关闭 ProjectR Editor 消除；随后发现的 `UDeveloperSettings` 链接错误由经批准的直接 `DeveloperSettings` 公共模块依赖修复，最终 Build 退出 0，因此不登记为未关闭 KI。
- KI-002、KI-003、KI-004、KI-006、KI-008、KI-009、KI-010 的范围和状态不变；本版本未提前处理模板迁移、GAS、专用 Toolset、SaveGame 或 Git GC。

# 记录规则

问题必须包含复现/证据、影响版本、临时处理、负责人和可验证关闭条件。不能用删除日志或降低校验来关闭问题。
