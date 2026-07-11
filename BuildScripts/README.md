# ProjectR BuildScripts

本目录是 ProjectR 自动化构建与验证的稳定入口。v0.0.4 冻结以下公共入口：

- `BuildEditor.bat`：构建 `ProjectREditor Win64`。
- `PackageDevelopment.bat`：Cook/Stage/Package/Archive Win64 客户端；`PackageDev.bat` 是无逻辑兼容转发器。
- `CleanGenerated.bat`：默认只预览可再生目录；实际删除需要双重显式确认和全部路径门禁。
- `AutomationReport.bat`：将外部自动化的独立 check 写入 SchemaVersion 1 报告。

入口不依赖当前目录或 PATH 中的 UE/VS 工具，必须接受显式 `-Project`，返回真实退出码。完整命令、日志和 `result.json` 写入：

```text
Saved/AutomationReports/<RunId>/<EntryPoint>-<Configuration>/
```

构建、Cook、Stage、Package 和报告只生成在 `.gitignore` 保护的目录。脚本不修改 `Source`、`Content`、UE Package 或未批准的 Config。详细参数、退出码和回滚见 `Docs/Workflow/BuildGuide.md`。
