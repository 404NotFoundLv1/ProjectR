# ProjectR BuildScripts

本目录是 ProjectR 自动化构建与验证脚本的稳定入口。

v0.0.0 只建立目录和责任边界，不实现构建、打包或发布脚本。首次 Editor Build 直接调用 UE5.8 的 `Engine/Build/BatchFiles/Build.bat`；可重复构建、打包和 QA 脚本由 v0.0.4 按其任务合同实现。

本目录中的未来脚本必须：

- 接受显式工程路径和构建配置，不依赖个人机器的当前目录。
- 返回真实退出码并保留完整日志，不把未运行或失败写成通过。
- 只生成到已被 `.gitignore` 保护的输出目录。
- 不修改 `Source`、`Content`、`Config` 或 UE Package。
