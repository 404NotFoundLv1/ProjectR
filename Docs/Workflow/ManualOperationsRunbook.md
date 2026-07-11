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

# 7. v0.0.1 DeveloperSettings 面板验收

**原因**：当前 19 个 MCP Toolset 可以反射 `UPRDeveloperSettings` 类和 CDO，但没有打开或导航 Project Settings 的专用 Tool；本步骤只确认自动注册后的可见界面，不代替 Build 和反射验证。

**前置**：Codex 已完成 `ProjectREditor Win64 Development` Build、重启 ProjectR Editor，并通过只读 MCP 确认三个属性的默认值为 `false/true/false`。

1. 在 ProjectR Editor 打开 `Edit -> Project Settings`。
2. 在左侧展开 `Game`，选择 `ProjectR`。
3. 确认页面包含 `Feature Flags`，并显示 `Enable Debug Features`、`Use Mock Director`、`Enable Steam Features`。
4. 确认三个复选框依次为关闭、开启、关闭。
5. 不切换任何复选框，不点击会写入配置的操作，直接关闭 Project Settings。
6. 返回 Codex：`DeveloperSettings 面板：PASS`；失败时返回缺失项、实际默认值和截图。

**期望**：`Game > ProjectR` 可见，三个显示名和默认值与 CDO 一致。
**回滚**：本步骤不修改配置或资产；若误改值，在保存前关闭设置并选择不保存。若已经产生 Config diff，立即通知 Codex，不手工编辑 ini。

# 8. v0.0.2 Gameplay Tags 面板只读验收（自动化失败时备用）

**原因**：优先由 GameplayTagsToolset 和设置反射自动验证。只有 Toolset 加载失败或无法证明 Project Settings 语义时，才执行本步骤。

**前置**：Codex 已完成 `ProjectREditor Win64 Development` Build、重启 ProjectR Editor，并记录自动验证缺口；当前无 PIE、无打开资产且地图未 Dirty。

1. 在 ProjectR Editor 打开 `Edit -> Project Settings`。
2. 搜索并打开 `Gameplay Tags` 设置页。
3. 只读确认 `Import Tags From Config` 与 `Warn On Invalid Tags` 为开启。
4. 只读确认 `Fast Replication` 与 `Dynamic Replication` 为关闭。
5. 确认标签列表中可见 `Input.Move`、`State.Alive`、`QTE.Result.Success`，且来源显示为 `DefaultGameplayTags.ini`。
6. 不切换复选框、不添加/重命名/删除 Tag、不点击会写入配置的操作，直接关闭设置页。
7. 返回 Codex：`Gameplay Tags 面板：PASS`；失败时报告缺失项、实际值和截图。

**期望**：四个设置值与配置合同一致，配置 Tag 可见且来源正确。

**回滚**：本步骤不应产生 Config 或 Package 变化；若误改，保存前关闭并选择不保存。若已产生 diff，立即通知 Codex，不手工编辑 ini。

# 9. v0.0.3 正式地图固定侧视相机验收

**原因**：MCP 已验证正式玩家生成、相机 CDO、PIE、日志和非空画面，但侧视构图是否舒适、角色和地面是否清晰仍需要人工视觉判断。

**前置**：Codex 已完成五张正式地图的创建和回载、八个 Blueprint 的 warnings-as-errors 编译、MainMenu 到 RealityHub 的自动旅行测试，以及四张玩法地图的自动 PIE；当前 PIE 已停止。

依次对以下地图执行步骤 1-6：

- `/Game/ProjectR/Maps/L_RealityHub`
- `/Game/ProjectR/Maps/L_Network_Prototype`
- `/Game/ProjectR/Maps/L_CombatGym`
- `/Game/ProjectR/Maps/L_BossGym`

1. 在 ProjectR Editor 中打开指定地图。
2. 选择 `Selected Viewport`、单玩家并启动 PIE。
3. 本版本没有移动输入属于预期；不要以无法移动判定失败。
4. 确认角色可见、视角明确为侧视、地面和环境轮廓可读，没有严重裁切、黑屏或持续抖动/漂移。
5. 按 `Esc` 停止 PIE；不要修改 Blueprint、地图、Project Settings 或相机数值。
6. 分别记录每张地图为 PASS/FAIL。失败时附截图和最短描述，例如“角色在画面外”或“地面被完全裁切”。

**期望**：四张地图均能稳定显示固定侧视构图。相机原生基线为角色相对位置 `(0, 600, 100)`、旋转 `(0, -90, 0)`、FOV 60；本步骤只判断结果，不调整数值。

**返回模板**：

```text
RealityHub：PASS / FAIL
Network Prototype：PASS / FAIL
CombatGym：PASS / FAIL
BossGym：PASS / FAIL
失败说明/截图：无 / <内容>
```

**2026-07-10 实际结果**：RealityHub、Network Prototype、CombatGym、BossGym 均由用户确认 PASS；失败说明/截图为无。

**回滚**：本步骤不应产生资产或配置变更。若误改或出现保存提示，不保存并立即通知 Codex；v0.0.3 保持 In Progress。

# 10. v0.0.4 Build/Package 前置门

**原因**：BuildEditor 和 PackageDevelopment 由 Codex 自动执行，但真实打包前需要足够的系统卷空间；运行中的同工程 Editor 若出现未保存提示，也不能由自动化猜测处理。

## 10.1 释放系统卷空间

**触发条件**：Codex 报告 `SystemVolumeSpace` 少于 10 GiB。真实入口报告 `v004-package-gate-19f4d96e020` 记录 C: 3.18 GiB，并在启动 UAT 前以退出码 65 停止，不满足 PackageDevelopment 门禁。

**2026-07-11 实际结果**：用户将 C: 释放到 12.47 GiB；随后 `v004-actual-package-19f4ece2a19` 的四个磁盘门、UAT、五地图和 EXE/Pak/IoStore 产物均 PASS。完成后 C: 仍为 12.10 GiB。

1. 打开 Windows `设置 -> 系统 -> 存储 -> 临时文件`。
2. 只选择本人确认可删除的系统临时项；不要选择 Downloads，不删除 ProjectR、Unreal Engine、Visual Studio 或个人目录。
3. 也可由用户自行把明确识别的大文件移至其他磁盘；Codex 不猜测或批量删除系统/用户文件。
4. 完成后把下列只读检查结果返回 Codex：

   ```powershell
   [math]::Round((Get-PSDrive C).Free / 1GB, 2)
   ```

5. 返回值至少为 `10.00` 后，由 Codex 运行 PackageDevelopment；用户不手动执行 UAT、修改 Config 或删除生成目录。

**回滚**：本步骤不修改仓库。若不确定某项能否删除，取消并保留该项。

## 10.2 Editor 保存提示

**触发条件**：Codex 正常关闭指向 `E:/MyWork/ProjectR/ProjectR.uproject` 的 Editor 时出现保存提示。

1. 不选择 `Save` 或 `Don't Save`，先选择 `Cancel`。
2. 记录提示列出的准确 Dirty Package/文件并返回 Codex；不要手工打开、修改或保存这些目标。
3. Codex 根据当前版本 Allowed paths、Manifest 和哈希判断是否可保存；未获得准确结论前保持 Editor 打开。
4. 禁止通过任务管理器、`Stop-Process -Force` 或重启系统绕过提示。

**期望**：只有在无 Dirty 或准确保存范围另行批准后才正常退出 Editor；Build/Package 不与同工程 Editor、Live Coding、UBT 或 UAT 并发。

## 10.3 UE 5.8 路径歧义

**触发条件**：脚本退出码为 66，并在日志中列出多个有效 UE 5.8 候选。

1. 只核对 Codex 提供的候选路径及其 `Engine/Build/Build.version`。
2. 确认 Major/Minor/Patch 为 5.8.0、Changelist 为 55116800 的目标根目录。
3. 把准确 EngineRoot 返回 Codex；由 Codex 通过 `-EngineRoot` 重跑。
4. 不修改 `.uproject` 的 EngineAssociation，不卸载其他引擎，不手工编辑脚本。

**当前审计**：只有 `D:/Unreal Engine 5/UE_5.8` 精确匹配，因此本步骤当前不触发。

# 11. 报告模板

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
