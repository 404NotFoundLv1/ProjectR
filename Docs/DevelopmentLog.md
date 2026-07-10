---
title: "ProjectR Development Log"
subtitle: "版本实施、验证、MCP 操作和兼容审查记录"
version: "v1.0"
date: "2026-07-10"
---

# 记录规则

每个公共版本追加一条记录，不重写历史。自动结果、人工判断、未运行项和已知风险分开记录。

# 2026-07-10 - 文档与工程基线审计

- UE5.8 ProjectR C++ 模板工程存在。
- Unreal MCP 已连接至 ProjectR Editor，读取当前关卡成功。
- 当前 MCP 为 19 Toolset、255 底层 Tool。
- ProjectR 尚无 Git、AGENTS.md、CURRENT_VERSION.md 和正式版本合同。
- 生成 CodexReady v6 文档体系；未实现任何游戏功能，未调用 MCP 写工具。
- 当前版本设为 v0.0.0。

# 2026-07-10 - v0.0.0 仓库基线实施（Completed）

- 清理前索引为 1299 个路径、未跟踪 293 个；生成/本地目录占暂存工作树约 4.77 GB。
- 用树 `e32a2786e2a870f622f5e715c6e502d2c179cea0` 保留索引回滚点；只清索引，工作树缺失文件 0。
- 经明确批准将 `docs` 只修正大小写为 `Docs`；22 份原文档的归一化聚合 SHA-256 前后一致。
- 创建 `.gitignore`、`.gitattributes`、`BuildScripts/README.md` 和详细仓库/UE 基线；UE Package 修改为 0。
- Git LFS 本地 hook 已安装；白名单索引为 876 个路径，753 个 UE Package 全部由 LFS 管理，保护目录索引项为 0。
- 第一次 Build 因 ProjectR Live Coding 在 UHT 后退出 1；用户授权正常关闭 ProjectR Editor 后，相同命令完成 48/48 actions，退出码 0，`Result: Succeeded`。
- ProjectR Editor 以 PID 39504 重启并重新持有 8000 端口；关卡和 `/Script/ProjectR.*` 身份验证通过。
- 自动 PIE 能启动、停止并返回有效非黑屏 PNG；三次日志增量比较新增阻断问题均为 0。
- MCP 只执行状态读取、PIE Start/Stop 和截图；没有资产保存、Blueprint 修改或 Package 写入。
- `git diff --check` 通过；全量 `git diff --cached --check` 因无 HEAD 而检查全部首次导入文件，发现模板 Source/Config 与既有文档的历史空白问题，按 KI-010 记录且不越权格式化。
- 用户按 ManualOperationsRunbook 第 6 节确认移动 PASS、镜头 PASS、跳跃 PASS；未报告失败复现。
- v0.0.0 基线提交已创建：`2bad8adb7fc2a38d27453bb07a4c91bac76e1726`（`v0.0.0 Init Steam production project`）；版本转换前工作区干净。
- push 和 GC 均未执行；`CURRENT_VERSION` 的推进由独立版本转换提交完成。
- Future Compatibility Review：未修改工程名、模块名、Targets、GameplayTag、Blueprint API、Save、Config 或资产路径；v0.0.1 可从稳定 `Docs`、构建入口和 `Source/ProjectR/Core` 扩展。

# 2026-07-10 - v0.0.1 工程规范与 DeveloperSettings（Completed）

- 创建 `CodingStandard.md` 和 `AssetNaming.md`；完善 AGENTS 必读顺序、CodexWorkflow、ManualOperationsRunbook 和 ADR-010。
- 在 `Source/ProjectR/Core` 创建 `UPRDeveloperSettings`，三个 Config 开关默认值为 Debug=false、MockDirector=true、Steam=false；未暴露 Blueprint API，未实现对应业务系统。
- 首次 Build 被运行中 Editor 的 Live Coding 阻断；确认无 PIE、无打开资产且地图未脏后正常关闭 ProjectR Editor。
- 第二次 Build 完成 UHT 和 C++ 编译，但因缺少 `DeveloperSettings` 直接模块依赖产生 11 个链接错误；按计划停止并取得用户对 `ProjectR.Build.cs` 唯一 Allowed-path 例外的明确批准。
- 在 `PublicDependencyModuleNames` 增加 `DeveloperSettings` 后，正式 `ProjectREditor Win64 Development` Build 退出 0，日志为 `Result: Succeeded`。
- ProjectR Editor 重启并重新持有 `127.0.0.1:8000`；MCP 反射类 `/Script/ProjectR.PRDeveloperSettings`，CDO 返回 `false/true/false`。
- MCP 仅执行状态、日志、类和属性读取；没有 set/save/create/move/delete、Blueprint 修改或 UE Package 写入。
- 用户按 ManualOperationsRunbook 第 7 节确认 `Game > ProjectR` 面板、`Feature Flags` 和三个默认值均为 PASS，未修改配置。
- `git diff --check`、Allowed-path、文件计数和保护路径检查通过；Source/Content/Config/地图为 89/753/5/4，CURRENT_VERSION、Targets、uproject、Config、Content 和资产路径不变。
- PIE 不属于本版本验收范围，标记 `NOT RUN`。
- v0.0.1 正式提交已创建：`cb6c22feb6e87c41630cfb880c5cd20ec68279bf`（`v0.0.1 Add coding standards and Codex workflow`）；版本转换前工作区干净。
- push 和 GC 均未执行；`CURRENT_VERSION` 的推进由独立版本转换提交完成。
- Future Compatibility Review：Core 设置类不依赖 GameplayTags、GAS、Director、Steam 或 Debug 具体类型；v0.0.2 可直接接入基础模块与 Tag Library，无需改写本版本公共设置契约。

# 版本记录模板

```text
Date / Version：
Goal：
Changed files：
UE Packages：
Public contracts：
Commands and results：
Acceptance：
Future Compatibility Review：
Human judgment：
Known risks：
Commit：
```
