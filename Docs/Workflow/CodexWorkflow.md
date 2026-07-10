---
title: "ProjectR Codex 开发手册"
subtitle: "从当前版本合同到 C++/MCP 实施、验证、审查和版本晋级"
version: "v1.0"
date: "2026-07-10"
---

# 开发模型

```text
设计案 = 产品北极星
架构/依赖图 = 跨版本合同
路线图 = 公共版本顺序
AGENTS.md = 永久工程规则
CURRENT_VERSION.md = 唯一当前任务
版本任务合同 = 本轮开发合同
Codex + Unreal MCP = 默认实施者
用户 = 主观判断、外部凭据和破坏性确认
Git = 回滚边界
```

# 每版本工作循环

## 1. 读取

Codex 必须依次读取 AGENTS、CURRENT_VERSION、当前任务合同、系统依赖图中直接上游与下游合同，再检查真实代码/资产状态。

## 2. 计划

输出当前前置是否满足、准确修改文件/Package、公共 API、MCP Operation Manifest、自动/人工验证和风险。发现范围冲突时停止，不通过“顺手实现未来系统”解决。

## 3. C++ 与配置实施

Codex 编写代码、Build.cs、ini、Schema、脚本和测试。每个微任务保持可编译；不把核心规则放进 Widget Blueprint，不用 Tick 代替事件/Timer。

## 4. Unreal MCP 实施

先碰撞/可编辑性检查，再按 Manifest 串行创建/修改资产。Blueprint 修改后编译；只保存准确 Package；检查 Dirty、引用和日志。

## 5. 自动验证

按适用范围运行 `git diff --check`、Editor Build、自动化测试、Schema/Tag/资产验证、Blueprint compile、Editor 重启复查和 PIE。没有实际运行的项目标为 NOT RUN。

## 6. Future Compatibility Review

检查直接下游消费者、冻结 API、SaveVersion、GameplayTag、Blueprint API、资产路径和扩展点。任何未记录破坏都会阻止版本关闭。

## 7. 人工判断

用户只执行 Runbook 中的主观体验、凭据或确认项。Codex 接收结果后继续修复当前版本，不扩大范围。

## 8. 审查和提交

审查当前 diff/Package 清单、生命周期、UObject/GC、Delegate、GAS 初始化、空引用、存档迁移、Tick、Blueprint API 和 LLM 边界。全部通过后由用户明确授权提交。

# 完成报告格式

1. Summary。
2. Changed files。
3. Created/modified UE Package。
4. Public API and contracts。
5. MCP Operation Manifest actual results。
6. Commands and actual results。
7. Acceptance matrix：PASS/FAIL/NOT RUN。
8. Future Compatibility Review。
9. Human judgment/manual steps。
10. Known risks。
11. Suggested commit message。

# 错误修复规则

- 从第一条错误开始读取完整日志。
- 只修当前版本引入的问题。
- 不新增功能、不大规模重构、不删除校验、不用硬编码绕过。
- 修复后说明根因、兼容影响和必须重跑的测试。

# 版本晋级

只有当前任务验收、自动验证、人工判断、Future Compatibility Review、KnownIssues 和 ArchitectureDecisions 均完成，才能更新 CURRENT_VERSION。更新后开启新会话，防止旧上下文继续驱动下一版本。
