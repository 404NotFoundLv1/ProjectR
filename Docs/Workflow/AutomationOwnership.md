---
title: "ProjectR Codex 自动化责任矩阵"
subtitle: "代码、配置、资产、Blueprint、验证、主观判断和外部凭据的默认所有者"
version: "v1.0"
date: "2026-07-10"
---

# 核心原则

只要 Codex 能通过工作区工具、构建工具或 Unreal MCP 安全完成，就不转交用户。用户不是机械操作员；用户负责创意判断、体验评价、凭据和高风险确认。

| 工作项 | 默认所有者 | 自动化路径 | 用户介入条件 |
|---|---|---|---|
| C++/Build.cs/Target/ini | Codex | apply_patch + Build | 无手动编码 |
| 编译错误修复 | Codex | Build 日志 -> 根因 -> 修复 -> 重跑 | 工具链缺失或系统级安装 |
| GameplayTag 配置 | Codex | ini / GameplayTagsToolset | 插件重启确认 |
| 普通 Blueprint | Codex MCP | BlueprintTools | 破坏性覆盖确认 |
| Blueprint Graph | Codex MCP | DSL/节点/Pin/连线/compile | 最终设计可读性审查 |
| Actor 组件/CDO | Codex MCP | ActorTools + ObjectTools | 无法反射的专用编辑器 |
| DataAsset/DataTable | Codex MCP | DataAsset/DataTable/ObjectTools | Schema 尚未编译 |
| 地图灰盒/Actor 布置 | Codex MCP | 复制模板 + SceneTools | 空间手感与美术判断 |
| Material/Instance | Codex MCP | Material/MI Tools | 最终美术质量 |
| Texture/Mesh 导入配置 | Codex MCP | Import + Mesh/Texture Tools | 原始资源创作/授权 |
| GAS/UMG/StateTree/动画/Niagara | Codex MCP/Custom Toolset | 选择性专用 Toolset | 工具确认无法覆盖的 UI |
| Blueprint compile/资产保存 | Codex MCP | compile_blueprint/save_assets | 无 |
| C++ Build/自动测试/PIE | Codex | Shell + Automation + EditorApp | 主观游玩反馈 |
| 手感/镜头/UI层级/动画/音频 | Human Judgment | Codex 提供测试场和记录表 | 始终需要人类判断 |
| Steamworks/API Key/签名 | Human Credential | Codex 准备配置与验证 | 输入凭据、外部后台操作 |
| 删除/移动/覆盖/Redirector | Human Approval + Codex | 用户批准后由 Codex 执行 | 每次明确批准 |

# 能力缺口处理

1. 描述缺少的具体操作，不笼统说“UE 资产需要手工”。
2. 审计当前 Toolset 和输入 Schema。
3. 检查已安装官方专用 Toolset。
4. 必要时由 Codex 编写最小、单职责、幂等的 ProjectR Tool。
5. 只有仍无法安全自动化时生成 Manual Runbook。

# 完成报告要求

所有人工项必须标记原因、已尝试的自动化路径、精确步骤和回滚；所有 Codex 项必须报告实际命令/Tool、结果和未运行项。自动化成功不替代手感或视觉验收。
