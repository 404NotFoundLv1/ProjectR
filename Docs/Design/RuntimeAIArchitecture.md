# Runtime AI Architecture

```text
Deterministic Combat / QTE / Rule Execution
  <- validated local whitelist rule
  <- DirectorResponseValidator
  <- Mock or HTTP Director Provider
  <- bounded PlayerProfileSnapshot
```

- LLM 仅返回 whitelist ID、等级、原因标签、有限参数和表达文本。
- 超时、离线、解析失败、未知 ID、越界数值均回退 Mock/模板。
- 发布客户端不保存正式 API Key。
- MCP/Codex 只用于开发，不进入运行时依赖。
