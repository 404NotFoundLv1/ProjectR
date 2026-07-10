---
title: "ProjectR C++ 与 Blueprint 编码规范"
subtitle: "类型命名、目录依赖、注释、配置和 Blueprint 暴露边界"
version: "v1.0"
date: "2026-07-10"
---

# 适用范围

本规范适用于 ProjectR 自有 C++、配置和 Blueprint API。现有 Unreal 模板 Variant 在迁移版本到来前作为参考保留；不得为了符合本规范而批量重命名、移动或格式化模板代码与资产。

# 类型与文件命名

| 类型 | 前缀 | 示例 |
|---|---|---|
| Actor | `APR` | `APRPlayerCharacter` |
| UObject、Component、Subsystem、DeveloperSettings | `UPR` | `UPRDeveloperSettings` |
| Struct | `FPR` | `FPRCombatEvent` |
| Enum | `EPR` | `EPRRunState` |
| ProjectR 源文件 | `PR` | `PRDeveloperSettings.h/.cpp` |

- 公共类型、Blueprint API、GameplayTag、序列化字段、目录和资产路径一旦发布，不得静默改名。
- 破坏性命名变更必须先写 Architecture Decision，并提供迁移、兼容测试和明确批准。
- 布尔成员使用 `b` 前缀；名称描述能力或状态，不使用含糊缩写。

# 目录与依赖

依赖只沿以下方向扩展：

```text
Core -> Infrastructure -> Combat -> Companion/QTE -> Director/Roguelike -> Content -> Platform/Release
```

- `Core` 只拥有生命周期、DeveloperSettings、日志和全局入口，不包含具体技能、法令、章节或平台实现。
- 反向协作使用 Interface、Delegate、Subsystem API、GameplayTag 或数据合同，禁止直接 include 下游具体类型。
- 新文件放入其所有权目录；不得为整理目录而移动现有公开文件或模板 Variant。

# C++ 文件结构

- Header 使用 `#pragma once`；对应的 `*.generated.h` 必须是最后一个 include。
- `.cpp` 首先 include 自己的 Header，再 include 直接依赖；不依赖偶然的传递 include。
- 只在公共签名需要时把 include 放进 Header，其余使用前置声明或放入 `.cpp`。
- UObject 引用必须遵守反射和 GC 规则；需要持有的 UObject 指针使用适当的 `UPROPERTY`/`TObjectPtr`。
- 优先使用 Delegate、事件和 Timer；只有经测量且具有明确生命周期时才使用 Tick。

# 注释、日志与失败处理

- 注释说明意图、所有权、边界和非显然原因，不逐字复述代码。
- 公共 API 说明输入、输出、失败语义和兼容约束。
- 失败路径使用明确的 ProjectR 日志类别和可定位上下文；不得吞掉错误或仅依赖屏幕提示。
- 临时兼容措施必须关联 KnownIssues 或 Architecture Decision，并写明关闭条件。

# Blueprint 暴露规则

- 只暴露稳定、确有内容配置需要的最小 API；默认保持 C++ only。
- 暴露时必须提供稳定的 Category、DisplayName、空引用行为和失败语义。
- 核心伤害、规则执行、Save 迁移、LLM 校验和账号生命周期不得放入 Widget Graph。
- 不为调试便利提前冻结 Blueprint API；未来需要暴露时按合同变更流程审查。

# 配置与默认值

- DeveloperSettings 属于 Core，只保存配置意图，不实现被开关控制的业务系统。
- 默认值必须确定、可审计并适合离线运行；构建和反射验证必须检查实际 CDO 值。
- Debug 配置开关不构成 Shipping 安全边界；敏感调试功能仍必须由编译和运行时策略限制。
- 修改 Project Settings 产生的 ini 变更必须位于当前版本 Allowed paths，否则停止并更新合同。

# 验证与兼容

每次修改至少执行当前合同要求的静态检查和 Build。涉及反射时重启 Editor 并读取实际类/CDO；涉及资产时遵守 `AssetNaming.md` 与 Unreal MCP Manifest。未运行的检查必须标记 `NOT RUN`。
