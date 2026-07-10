---
title: "ProjectR 人工操作 Runbook"
subtitle: "只有无法安全自动化、需要主观判断或外部凭据时使用"
version: "v1.0"
date: "2026-07-10"
---

# 使用要求

Codex 只有在记录已审计的 Toolset 与替代方案后，才能引用本 Runbook。完成步骤后，用户应把指定日志、截图或结论返回给 Codex；Codex继续完成验证和记录。

每次引用本 Runbook 时，Codex 必须在版本完成报告中补全精确资产路径、资产名、类名、属性名、目标值和期望结果，不能让用户自行猜测配置。

# 1. 启用 UE 专用 Toolset 并重启

**原因**：新增插件模块通常需要 Editor 重启；自动终止有未保存资产的 Editor 风险较高。  
**前置**：Codex 已列出需要启用的插件和当前 Dirty Package。

1. 在 UE 中打开 `Edit -> Plugins`。
2. 搜索 Codex 指定的准确插件名，例如 `GASToolsets` 或 `UMG ToolSet`。
3. 核对插件位于 Engine/Experimental Toolsets，勾选 Enabled。
4. 如果弹出 Experimental 警告，确认本项目接受实验性开发工具风险。
5. 执行 `File -> Save All`，确认 Codex 报告中的 Dirty Package 均已保存。
6. 关闭并重新打开 `E:/MyWork/ProjectR/ProjectR.uproject`。
7. 在 Output Log 确认 MCP 监听 `127.0.0.1:8000/mcp`。
8. 返回 Codex：“插件已启用、Editor 已重启、MCP 已监听”。

**验证**：Codex重新调用 list_toolsets/describe_toolset。  
**回滚**：插件导致启动失败时，在 `.uproject` 中将对应插件 Enabled 改回 false；此文件修改由 Codex执行。

# 2. 动作手感与镜头验收

**原因**：数值和日志不能代替人体操作感受。

1. 打开 Codex 指定测试地图，例如 `/Game/ProjectR/Maps/L_CombatGym`。
2. 以 Selected Viewport、单玩家启动 PIE。
3. 连续完成 10 次左右移动换向、跳跃、落地和闪避。
4. 分别记录：输入延迟感、镜头不适、是否离开 2.5D 平面、闪避距离是否可控。
5. 按当前版本验收表执行技能/敌人/Boss 场景。
6. 返回 Codex 每项 PASS/FAIL、复现步骤和期望感受；不要自行改 C++ 或 Blueprint 数值。

**回滚**：Codex根据 DataAsset/配置 diff 回退到上一个已验收数值。

# 3. UI、动画、VFX 和音频主观验收

1. 打开 Codex 指定资产/地图和测试状态。
2. 检查信息层级、字号、手柄焦点、动画过渡、前摇可读性、特效遮挡和音量层级。
3. 在 1080p 和目标窗口比例各检查一次。
4. 对失败项提供截图/视频时间点、期望和严重度。
5. Codex负责修改可自动化配置；原始美术/动画/声音需要重新创作时，由用户提供合法源文件。

# 4. 破坏性资产操作批准

**适用**：覆盖、重命名、移动、删除、批量迁移、Redirector 修复。

1. Codex先提供旧路径、新路径、Referencers、Source Control 状态、备份/分支和回滚方案。
2. 用户逐项明确批准，不使用“全部随便处理”的宽泛授权。
3. Codex通过 Unreal MCP执行并保存准确 Package。
4. Codex重新扫描 Referencers、编译 Blueprint、加载相关地图并运行回归。

# 5. Steamworks 与外部凭据

1. 用户登录 Steamworks 合作伙伴后台并选择 ProjectR 对应应用。
2. 用户创建或确认 App ID、成就、Depot、测试分支和云存档配置；密钥不粘贴到对话或仓库。
3. 用户只把非秘密标识和值写入本机受控配置或环境变量。
4. Codex读取非秘密配置、编写 Platform facade、构建并验证降级路径。
5. 用户在 Steam 测试账号确认成就、Rich Presence、Cloud Save 和安装结果。

# 6. v0.0.0 默认模板 PIE 操作验收

**原因**：当前 MCP 可以启动/停止 PIE、读取日志和截取画面，但 19 个已启用 Toolset 没有玩家按键注入能力，不能代替移动、镜头和跳跃的人工输入确认。

**前置**：Codex 已完成 `ProjectREditor Win64 Development` Build、MCP 自动 PIE 冒烟和日志检查；当前工程路径是 `E:/MyWork/ProjectR/ProjectR.uproject`。

1. 在 ProjectR Editor 确认当前地图为 `/Game/ThirdPerson/Lvl_ThirdPerson`。
2. 选择 `Selected Viewport`、单玩家并启动 PIE。
3. 连续约 10 秒使用 `W/A/S/D` 移动，确认角色可开始、停止和改变方向，没有卡死或异常瞬移。
4. 移动鼠标观察镜头，确认镜头跟随角色且没有黑屏、持续抖动或失控旋转。
5. 按 `Space` 至少三次，确认角色能跳起、落地并可继续移动。
6. 按 `Esc` 停止 PIE，不修改 Blueprint、Project Settings、源码或资产。
7. 按下方模板返回：移动、镜头、跳跃分别为 PASS/FAIL；失败时附最短复现和实际表现。

**期望**：默认模板角色生成，移动、镜头和跳跃均可用，无阻断性视觉问题。
**回滚**：本步骤不修改资产；失败时保持 v0.0.0 In Progress，由 Codex根据 Build/PIE 日志定位，不让用户手动改代码或配置。

# 7. 报告模板

```text
Runbook：
版本：
步骤结果：PASS / FAIL
失败步骤编号：
复现：
预期：
实际：
日志/截图位置：
是否允许 Codex 继续修改：是 / 否
```
