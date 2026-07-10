---
title: "ProjectR UE 资产命名规范"
subtitle: "Package 根目录、类型前缀、语义命名和破坏性操作边界"
version: "v1.0"
date: "2026-07-10"
---

# 适用范围

本规范约束后续创建或正式迁移到 ProjectR 的 UE 资产，不授权重命名现有 ThirdPerson、Combat、Platforming 或 SideScrolling 模板资产。

# Package 根目录

- 正式资产默认创建在 `/Game/ProjectR/`，再按系统或内容域分目录。
- MCP 能力测试只使用 `/Game/ProjectR/MCPTest/`，验证后也不得未经批准删除。
- 资产路径是公共合同；大小写、目录和 Package 名必须稳定。

# 类型前缀

| 资产类型 | 前缀 | 示例 |
|---|---|---|
| Actor/Object Blueprint | `BP_` | `BP_PlayerCharacter` |
| Widget Blueprint | `WBP_` | `WBP_CombatHUD` |
| DataAsset | `DA_` | `DA_PlayerAbilitySet` |
| GameplayEffect | `GE_` | `GE_Damage_Fire` |
| GameplayAbility | `GA_` | `GA_Dash` |
| Niagara/VFX | `VFX_` | `VFX_HitImpact` |
| Sound/SoundCue/MetaSound | `SFX_` | `SFX_Player_Hit` |
| Level/Map | `L_` | `L_CombatGym` |
| InputAction | `IA_` | `IA_Jump` |
| InputMappingContext | `IMC_` | `IMC_Player` |

# 命名形式

- 使用 `前缀 + PascalCase 语义名`；需要修饰时用下划线分隔稳定维度，例如 `GE_Damage_Fire`。
- 使用 ASCII 字母、数字和下划线；不使用空格、连字符、临时编号或人员姓名。
- 名称描述资产职责，不描述当前存放位置或易变化的实现细节。
- 同一语义不得以多个近义名称重复创建；碰撞时停止并检查现有资产和 Referencers。

# MCP 创建和修改规则

1. 写入前列出准确 Package 路径，检查存在、可编辑和碰撞状态。
2. 使用最小 MCP Operation Manifest，所有写操作串行发送到同一 ProjectR Editor。
3. Blueprint 修改后单独报告创建、编译、保存、重启加载和 PIE 状态。
4. 只保存 Manifest 中的准确 Package，并复查 Dirty Package 与引用。
5. 不通过 Shell、普通文件 IO 或二进制补丁修改 `.uasset/.umap`。

# 移动、重命名与删除

覆盖、移动、重命名、删除、批量迁移和 Redirector 修复必须先提供旧/新路径、Referencers、兼容影响和回滚方案，并获得逐项明确批准。不得以“整理命名”为由破坏模板引用或未来版本合同。
