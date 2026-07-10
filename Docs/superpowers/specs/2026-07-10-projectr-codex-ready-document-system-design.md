# ProjectR Codex-Ready 文档系统设计规格

**状态**：待用户书面规格复核  
**批准方案**：方案 2 - Codex 可执行文档体系  
**规格修订**：v1.1 - 增加跨版本防返工与自动化优先要求  
**规格日期**：2026-07-10  
**源文档目录**：`E:/MyWork/ProjectR_Design/`  
**游戏工程目录**：`E:/MyWork/ProjectR/`

## 1. 目标

在不覆盖原始设计案和路线图的前提下，将现有文档重构为一套同时满足以下需求的文档系统：

1. 人类可以连续阅读并理解产品愿景、玩法、叙事和商业范围。
2. Codex 可以只读取当前版本所需的有限上下文并执行开发任务。
3. Unreal MCP 的真实能力边界、写入保护和验证流程成为永久工程约束。
4. 每个开发版本都具备明确前置状态、范围、非目标、允许路径、资产操作、验证和回滚边界。
5. 所有面向阅读与归档的交付物同时提供 Markdown 和 PDF。
6. 真正供 Codex 自动加载的控制文件以 Markdown 形式同步到 ProjectR 工程根目录和 `Docs/`。
7. Codex 能从文档中理解完整系统架构、数据流、模块所有权、版本依赖和每个版本对后续版本提供的稳定契约。
8. 默认由 Codex 完成 C++、配置、构建、测试以及 Unreal Editor 内可自动化的资产创建、Blueprint 节点配置和验证；人工操作仅作为有明确原因的例外。

## 2. 非目标

- 不改变游戏的核心创意、三名 AI 队友、人类不可预测性主题和账号删除核心循环。
- 不在本任务中实现任何游戏功能、C++ 类、Blueprint、DataAsset 或关卡。
- 不通过文件系统直接创建、修改或伪造 `.uasset`、`.umap`。
- 不启用或禁用任何 UE 插件。
- 不执行 Unreal MCP 写操作。
- 不初始化 Git、不提交、不推送。
- 不覆盖四份原始 PDF/Markdown 文件。

## 3. 已确认的真实工程基线

- 引擎：Unreal Engine 5.8。
- 工程：`ProjectR.uproject` 已存在，当前仍接近官方 Third Person C++ 模板。
- MCP：`http://127.0.0.1:8000/mcp` 已连接至打开 ProjectR 的 UnrealEditor 进程。
- 当前 MCP：19 个 Toolset、255 个底层 Tool。
- 当前关卡：`/Game/ThirdPerson/Lvl_ThirdPerson`。
- 已启用：`ModelContextProtocol`、`EditorToolset`。
- 已安装但未启用：`AllToolsets`、`GASToolsets`、`GameplayTagsToolset`、`UMGToolSet`、`StateTreeToolset`、`AIModuleToolset`、`AutomationTestToolset`、`AnimationAssistantToolset`、`LiveCodingToolset`、`NiagaraToolsets`、`PCGToolset` 等。
- 当前没有 Git 仓库。
- 当前没有根目录 `AGENTS.md` 和 `CURRENT_VERSION.md`。
- 当前 `Build.cs` 有 Enhanced Input、AI、UMG 等依赖，但尚未接入 GAS 模块。
- 工程中存在 Third Person 模板及 Combat、Platforming、SideScrolling Variant 参考代码，不应直接等同于正式 ProjectR 架构。

## 4. 输出架构

### 4.1 归档与阅读交付目录

所有新文档写入：

`E:/MyWork/ProjectR_Design/CodexReady_v6/`

```text
CodexReady_v6/
  README.md
  README.pdf
  01_Product/
    ProjectR_Game_Design_CodexReady_v1.0.md
    ProjectR_Game_Design_CodexReady_v1.0.pdf
    ProjectR_System_Architecture_and_Version_Dependency_Map.md
    ProjectR_System_Architecture_and_Version_Dependency_Map.pdf
  02_Roadmap/
    ProjectR_Codex_Executable_Roadmap_v6.md
    ProjectR_Codex_Executable_Roadmap_v6.pdf
    ProjectR_Version_Index.md
    ProjectR_Version_Index.pdf
    ProjectR_Roadmap_Errata_and_Decisions.md
    ProjectR_Roadmap_Errata_and_Decisions.pdf
    ProjectR_Data_and_Interface_Contracts.md
    ProjectR_Data_and_Interface_Contracts.pdf
    Versions/
      v0.0.0.md
      v0.0.0.pdf
      v0.0.1.md
      v0.0.1.pdf
      v0.0.2.md
      v0.0.2.pdf
      v0.0.3.md
      v0.0.3.pdf
      v0.0.4.md
      v0.0.4.pdf
      v0.0.5.md
      v0.0.5.pdf
  03_Workflow/
    ProjectR_Codex_Development_Handbook.md
    ProjectR_Codex_Development_Handbook.pdf
    ProjectR_Unreal_MCP_Capability_and_Authoring_Guide.md
    ProjectR_Unreal_MCP_Capability_and_Authoring_Guide.pdf
    ProjectR_Codex_Automation_Ownership_Matrix.md
    ProjectR_Codex_Automation_Ownership_Matrix.pdf
    ProjectR_Manual_Operations_Runbook.md
    ProjectR_Manual_Operations_Runbook.pdf
    AGENTS.md
    AGENTS.pdf
    CURRENT_VERSION.md
    CURRENT_VERSION.pdf
  04_Tracking/
    DevelopmentLog.md
    DevelopmentLog.pdf
    KnownIssues.md
    KnownIssues.pdf
    ArchitectureDecisions.md
    ArchitectureDecisions.pdf
```

版本独立任务合同首先只生成当前生产工程阶段 `v0.0.0` 至 `v0.0.5`。后续版本在进入对应阶段前按路线图模板逐个生成，避免一次维护几十份尚未生效的重复文件。

### 4.2 ProjectR 工程内执行副本

以下 Markdown 同步到工程，作为 Codex 真实工作上下文：

```text
ProjectR/
  AGENTS.md
  CURRENT_VERSION.md
  Docs/
    Design/
      ProductPillars.md
      RuntimeAIArchitecture.md
    Architecture/
      SystemArchitecture.md
      VersionDependencyMap.md
      DataAndInterfaceContracts.md
    Roadmap/
      VersionIndex.md
      RoadmapErrata.md
    Versions/
      v0.0.0.md
      v0.0.1.md
      v0.0.2.md
      v0.0.3.md
      v0.0.4.md
      v0.0.5.md
    Workflow/
      CodexWorkflow.md
      UnrealMCPAuthoring.md
      AutomationOwnership.md
      ManualOperationsRunbook.md
    DevelopmentLog.md
    KnownIssues.md
    ArchitectureDecisions.md
```

PDF 仅放在 `ProjectR_Design/CodexReady_v6`，避免在游戏仓库中复制大量不可 diff 的归档文件。

## 5. 两份主文档的重构策略

### 5.1 游戏设计案 CodexReady v1.0

保留：

- 世界观、共鸣卡带、双世界闭环。
- 首脑法令、三名 AI 队友、QTE、肉鸽构筑、账号删除。
- 章节、Boss、现实据点、长期关系和情感主题。
- 设计示例、台词方向与核心宣传表达。

重构：

- 在开头增加产品北极星、核心循环、设计不变量和 MVP/商业版边界。
- 将“30 分钟 Demo”统一为约 35 分钟支持量、30 分钟演示剪辑范围，消除时长表述矛盾。
- 将 P0、Vertical Slice、Commercial 1.0 三层内容范围分开。
- 技术实现建议改为稳定的运行时合同，不再与具体开发版本混排。
- 明确运行时 LLM 只返回白名单结构化意图；战斗、伤害、敌人、QTE 和法令效果由本地确定性逻辑执行。
- 增加服务端密钥边界、超时、Schema 校验、Clamp、Mock 回退与离线可玩要求。
- 增加系统依赖图、内容规模预算和三个产品 Gate。

### 5.2 Codex 可执行路线图 v6

保留原公共阶段编号和 1.0 商业路线，但进行以下修订：

- 统一 `UPR`，废止 `USPDeveloperSettings`、`USPTagLibrary` 写法。
- 将当前 ProjectR 模板工程视作 v0.0.0 的输入，而不是“尚无项目”。
- v0.0.0 聚焦 Git、基线编译、模板资产盘点和回滚边界。
- v0.0.1 生成工程规范、`AGENTS.md`、`CURRENT_VERSION.md` 和命名迁移决策。
- v0.0.2 接入基础模块、GameplayTags 与配置校验。
- v0.0.3 创建正式 Game Framework，同时隔离模板 Variant。
- v0.0.4 建立构建、自动化、日志与验证脚本。
- 新增 v0.0.5：Unreal MCP 安全资产生产基线。
- v0.1.5 对未来系统的 Debug 命令只注册框架和“系统尚不可用”的明确响应。
- v0.2.2 只广播原型 Run 完成事件，不提前实现正式通关删号。
- v0.2.0、v0.3.2、v0.4.1、v0.4.2 使用内部微任务，并要求每个微任务可独立编译。
- 区分“公共版本号”和“内部任务编号”，例如 `v0.3.2-a` 不作为发布 Tag。
- 每个版本采用固定任务合同，不再内嵌大段重复提示词。

每份版本任务合同固定包含：

1. 目标。
2. 当前真实前置状态。
3. 本次范围。
4. Non-goals。
5. Allowed paths。
6. 禁止修改路径。
7. 公共 API/数据合同。
8. Codex 代码工作。
9. Unreal MCP 资产工作。
10. 必须人工判断的内容。
11. 自动验证。
12. PIE 验收。
13. 回滚条件。
14. 完成报告格式。
15. 建议提交名。

### 5.3 跨版本防返工机制

路线图不能只说明“下一步做什么”，还必须说明“当前版本为哪些后续版本提供什么”。因此，v6 必须包含以下机制：

#### 全局系统依赖图

使用稳定的单向依赖表达核心关系：

```text
Core / Tags / Settings
  -> Input / GAS / CombatEvent / Save
  -> Player Skills / Enemies / Boss Prototype / HUD
  -> Companion / Relationship / QTE / Dialogue Events
  -> PlayerProfile / Director Rules / Rooms / Run State
  -> Account Deletion / Meta Progression / Reality Hub / Memory
  -> Chapter Content
  -> Economy / Accessibility / Localization / Steam
  -> QA / Performance / Release
```

任何反向依赖必须通过接口、事件或数据合同解决。例如 Combat 不得直接依赖某个具体 QTE、Director Rule 或章节类；QTE、画像和 UI 订阅 CombatEvent。

#### 全版本依赖矩阵

`ProjectR_System_Architecture_and_Version_Dependency_Map` 必须覆盖 v0.0.0 至 v1.0.4，每个公共版本至少记录：

- `Depends on`：必须已经存在的版本输出。
- `Produces`：本版本新增的稳定能力。
- `Consumed by`：明确列出使用这些输出的后续版本。
- `Extension points`：为后续扩展预留的接口、Delegate、GameplayTag、DataAsset Schema 或 Subsystem API。
- `Must remain compatible`：不能随意破坏的 Blueprint API、序列化字段、资产路径、GameplayTag、SaveVersion 和事件名称。
- `Deferred intentionally`：明确延迟到未来版本的内容，防止当前版本提前实现业务逻辑。

以下跨阶段契约必须被显式记录：

- v0.1.2 `CombatEvent` 被技能、敌人、QTE、玩家画像、HUD、AI 台词和统计共同消费。
- v0.1.3 Ability 输入和 AbilitySet 合同被全部玩家技能、QTE 触发和构筑系统消费。
- v0.1.4 版本化 SaveGame 被队友关系、账号墓园、任务、Meta 成长、设置和 Steam Cloud 消费。
- v0.3.0 Companion/Relationship 合同被 AI 实体、QTE、对话、濒死保护、个人任务和结局变量消费。
- v0.3.2 QTE 结果合同被关系、玩家画像、法令反制、Boss 和记忆摘要消费。
- v0.4.0 Director Provider/Validator/Rule 合同被法令、房间、Boss、对话、章节内容和运行统计消费。
- v0.4.2 Room/Reward 合同被全部章节内容、经济和难度系统消费。
- v0.4.3 RunSummary/AccountRecord 合同被墓园、现实据点、长期记忆、个人任务和 Steam 成就消费。

#### 契约先行与最小实现

- 当前版本只实现本版本需要的最小行为，但可以定义经过下游需求验证的稳定接口。
- 不得为了未来版本提前实现完整业务类；应使用 Interface、Delegate、Subsystem API、GameplayTag 和可版本化 Data Schema 作为扩展缝。
- 每个新公共 API 都必须写明所有者模块、生命周期、线程/世界上下文、失败行为和未来消费者。
- GameplayTag、资产路径、序列化字段和 BlueprintCallable 名称一旦进入已验收版本，默认冻结。
- 需要破坏性变更时，必须新增 Architecture Decision、迁移步骤、兼容期和回归测试，不允许静默重命名。

#### 每版本未来影响审查

每份版本任务合同增加以下必填项：

1. 下游消费者清单。
2. 本版本必须冻结的契约。
3. 本版本明确不实现的未来行为。
4. 对 SaveGame、Blueprint、GameplayTag、DataAsset 和资产引用的兼容风险。
5. 如果以后需要扩展，预定的扩展点。
6. 进入下一版本前必须通过的跨版本回归测试。

Codex 在编辑前必须读取全局依赖图、当前任务合同和当前任务列出的直接下游合同。完成后必须执行一次 Future Compatibility Review，确认没有把未来系统写死在当前实现里。

#### 版本晋级门槛

只有以下项目全部满足，`CURRENT_VERSION.md` 才能指向下一版本：

- 当前版本验收通过。
- 所有公共 API 和数据合同已记录。
- 下游版本所需的扩展点存在，但未来业务未被提前实现。
- 相关自动回归测试通过。
- Blueprint、GameplayTag、SaveGame 和资产路径无未记录破坏。
- ArchitectureDecisions 和 KnownIssues 已更新。

## 6. Codex 自动化优先与 Unreal MCP 永久规则

### 6.1 自动化优先原则

- C++、Build.cs、Target、ini、脚本、Schema、测试和编译错误修复默认全部由 Codex 完成；文档不得把任何编码工作转交给用户。
- UE Editor 内的机械操作默认由 Codex 通过 Unreal MCP 完成，包括资产查询/创建、Blueprint 创建与父类设置、组件添加、默认属性填写、Blueprint Graph 节点和连线配置、DataAsset/DataTable、材质、关卡 Actor 布置、保存、编译、日志读取和 PIE 控制。
- Codex 不得仅因为操作发生在 UE Editor 中就生成“请用户手动创建”的清单。必须先审计当前 Toolset，并按能力层级寻找自动化路径。
- 对同类重复内容，优先生成批量、幂等、可验证的 ProjectR Authoring Tool，而不是让用户重复点击。
- 每个版本任务合同必须有 `Automation Ownership` 表，逐项标记 `Codex C++`、`Codex MCP`、`Codex Custom Toolset`、`Human Judgment` 或 `Human External Credential`。

### 6.2 自动化能力层级

Codex 必须按以下顺序处理 UE Editor 工作：

1. **现有通用 Toolset**：优先使用当前已经验证的 Blueprint、Actor、Asset、Object、Scene、DataAsset、DataTable、Material、Mesh、Texture、EditorApp 和 Logs 工具。
2. **选择性启用官方专用 Toolset**：任务需要时启用并审计 GameplayTags、GAS、UMG、StateTree、AIModule、Automation、Animation、LiveCoding、Niagara 或 PCG Toolset；不默认无差别启用 AllToolsets。
3. **ProjectR 专用 Toolset**：官方工具无法幂等完成任务时，由 Codex编写 editor-only Python/C++ Toolset，重新加载/刷新工具后调用它完成操作。
4. **人工例外**：只有前三层均不可行、操作需要主观判断/外部凭据，或风险需要用户确认时，才转为人工步骤。

如果某专用 Toolset 未启用，文档必须把“启用插件、重启编辑器、重新审计 Toolset”纳入当前版本前置步骤；不能直接把该版本全部资产工作判为人工。

### 6.3 Codex 可自动完成的默认范围

- 创建普通 Blueprint 子类、设置父类、变量、函数、事件、Dispatcher 和复制属性。
- 添加/删除/挂接组件，填写组件和 Blueprint CDO 属性。
- 读取和写入 Blueprint Graph DSL，创建节点、设置 Pin、连接执行流和数据流，并编译 Blueprint。
- 创建和填写 DataAsset、DataTable、CurveTable、StringTable。
- 创建 Material、Material Function、MPC、Material Instance，配置表达式和参数。
- 导入并配置 Texture、StaticMesh、SkeletalMesh，设置材质、LOD、碰撞、Nanite、Socket。
- 复制模板地图，加载地图，放置/组织 Actor，设置 Transform、标签和层级。
- 保存准确 Package、检查 Dirty 状态、读取 Output Log、启动/停止 PIE 和采集验证截图。
- 在专用 Toolset 启用并验证后，自动处理 UMG、GameplayTags、GAS、StateTree/Behavior Tree、动画、Niagara 和自动化测试可覆盖的部分。

### 6.4 允许人工介入的边界

人工操作只允许出现在以下类别：

- 动作手感、镜头舒适度、Boss 前摇可读性、UI 视觉层级、动画观感、音频和美术质量等主观判断。
- Steamworks 后台、API Key、签名、商店后台、受许可约束的外部资源等凭据或外部账户操作。
- 资产覆盖、重命名、移动、删除、批量迁移和 Redirector 修复等破坏性决策的最终确认。
- Editor 重启、硬件实测或当前工具无法安全驱动的系统对话框。
- 原始美术、动画、声音等需要人类创作或合法采购的内容；但其导入、命名、配置和引用仍尽量由 Codex 完成。

### 6.5 人工步骤书写标准

任何确实必须由用户操作的内容，都必须在任务合同和完成报告中给出：

1. 为什么无法自动化，以及已经检查过哪些 Toolset/替代方案。
2. 操作前置条件和风险。
3. 精确菜单路径或外部后台入口。
4. 精确资产路径、资产名、类名、属性名和目标值。
5. 逐步编号操作，不使用“配置好”“适当设置”等模糊措辞。
6. 每一步完成后应看到的结果。
7. 验证方法、失败表现和回滚方法。
8. 用户完成后需要返回给 Codex 的日志、截图或状态信息。

### 6.6 Unreal MCP 写入安全规则

- 禁止通过 shell、普通 Python 文件 IO、十六进制或外部二进制工具直接修改 `.uasset/.umap`。
- UE 二进制资产只能通过 Unreal MCP、UE Editor Python Toolset、Editor-only C++ Toolset 或批准的 Editor Utility 创建和修改。
- MCP 写入调用必须串行；禁止多个 Agent 同时写同一个 UnrealEditor 实例。
- 写入前列出全部目标 Package，并对每个目标执行存在性和命名冲突检查。
- 默认只允许在 `/Game/ProjectR/` 写入；测试资产限制在 `/Game/ProjectR/MCPTest/`。
- 未经明确批准不得覆盖、重命名、移动、删除现有资产，不得批量修复 Redirector 或 Resave All。
- 写入后必须保存准确 Package、列出 Dirty Package、编译受影响 Blueprint、运行资产校验和相关冒烟测试。
- Blueprint 创建成功、Blueprint 编译通过、编辑器重启后可加载、PIE 通过是四个独立状态。
- 当前通用 Toolset 可创建普通 Blueprint、DataAsset、材质和场景 Actor；UMG、GAS、StateTree、Behavior Tree、动画、Niagara 和自动化能力必须在专用 Toolset 启用并重新审计后才能承诺。
- ProjectR 专用 Authoring Toolset 仅在通用工具无法提供幂等批量生产、项目 Schema 校验或专用资产类型支持时创建。

### 6.7 每版本 MCP 操作清单

每个涉及 UE 资产的版本必须在执行前生成 MCP Operation Manifest：

- 预计创建、修改和只读访问的 Package。
- 每个操作使用的 Toolset/Tool。
- 碰撞检查和可编辑性检查结果。
- 是否需要专用 Toolset、Editor 重启或用户批准。
- 操作顺序和失败时停止点。
- 保存、Blueprint 编译、资产验证、编辑器重启和 PIE 验证状态。

执行后 Manifest 更新为实际结果，禁止用“创建成功”代替完整验证。

## 7. 命名与模板迁移决策

- Actor 类：`APR*`。
- UObject、Component、Subsystem：`UPR*`。
- Struct：`FPR*`。
- Enum：`EPR*`。
- 文件名：`PR*.h/.cpp`。
- 资产前缀：`BP_`、`WBP_`、`DA_`、`GE_`、`GA_`、`VFX_`、`SFX_`、`L_`、`IA_`、`IMC_`。
- 现有 `AProjectRCharacter` 等模板类不在文档阶段直接重命名。
- v0.0.3 创建正式 `APRPlayerCharacter` 等类并完成引用迁移；确认无引用后再决定是否删除模板类。
- `Variant_Combat`、`Variant_Platforming`、`Variant_SideScrolling` 在迁移完成前仅作参考，禁止新正式系统依赖其类型。

## 8. 当前版本

`CURRENT_VERSION.md` 初始指向 `v0.0.0`，状态为 `In Progress`。

理由：项目文件已经存在，但以下 v0.0.0 验收条件尚未满足：

- 未初始化 Git。
- 未建立 `.gitignore` 和回滚基线。
- 未记录首次正式 Build/PIE 结果。
- 未完成模板 Variant 与 Content 盘点。
- 未建立 DevelopmentLog、KnownIssues 和 ArchitectureDecisions。

## 9. PDF 生成与视觉标准

- A4 纵向。
- 中文正文使用可嵌入的 CJK 字体；代码使用等宽字体。
- 独立封面，不在封面堆放完整目录。
- 目录单独分页。
- 页眉显示文档短名和版本，页脚显示页码。
- 标题层级、表格、提示框和代码块保持一致。
- 长表格允许跨页并重复表头。
- 代码块自动换行或缩小到可读下限，不允许裁切。
- 不出现孤立标题、重叠、黑块、乱码或不可读的小字。
- 所有 PDF 从最终 Markdown 生成；生成后提取文字检查关键章节，并逐页渲染为 PNG 做视觉 QA。

## 10. 一致性与验证

### Markdown

- UTF-8 编码。
- 无 `TBD`、`TODO`、占位符或工具引用 token。
- 所有内部相对链接可解析。
- 所有版本号、类名、GameplayTag 和资产路径在文档之间一致。
- `CURRENT_VERSION.md` 指向真实存在的任务文件。
- 路线图中的 v0.0.0-v0.0.5 与独立任务合同逐项一致。
- v0.0.0-v1.0.4 的每个公共版本都出现在依赖矩阵中，并包含 Depends on、Produces、Consumed by、Extension points 和 Must remain compatible。
- 每个版本的 Automation Ownership 覆盖代码、配置、资产、Blueprint、验证和人工例外。
- 每个必须人工操作的条目均符合人工步骤书写标准。

### PDF

- 每个 Markdown 有且仅有一个同名 PDF。
- PDF 页数大于 0，可由 `pdfinfo` 和 `pypdf` 打开。
- PDF 文本包含对应 Markdown 的标题、版本和关键章节。
- 最新渲染 PNG 无裁切、重叠、乱码或表格溢出。

### 工程副本

- 工程内 Markdown 与交付包中相应 Markdown 内容一致，允许路径链接因目录位置不同而调整。
- 不修改 `Source/`、`Content/`、`Config/` 和 `.uproject`。
- 不执行 Unreal MCP 写操作。

## 11. 完成定义

只有以下条件全部满足才可以声明完成：

1. 所有批准的 Markdown 和 PDF 文件存在。
2. 原始四份文件的 SHA-256 未变化。
3. 工程内执行副本存在且相互引用有效。
4. Markdown 一致性检查通过。
5. 每份 PDF 的结构检查通过。
6. 所有最终 PDF 已渲染并完成视觉检查。
7. 最终报告列出创建文件、验证结果、已知限制和下一步当前版本。
8. 完整系统依赖图和 v0.0.0-v1.0.4 版本依赖矩阵存在，且与路线图一致。
9. 数据/接口契约文档覆盖 SaveGame、CombatEvent、Ability、Companion、QTE、Director、Room/Reward、RunSummary/AccountRecord。
10. 自动化责任矩阵证明 UE Editor 机械工作默认归 Codex，人工任务均有原因和逐步 Runbook。

## 12. 明确假设

- 原始文档作为不可变源档案保留。
- 本轮只为 v0.0.0-v0.0.5 生成独立版本任务合同；后续路线保留在总路线图中。
- 面向 Codex 的工程执行文件以中文为主，代码标识符和路径保持英文。
- PDF 是阅读与归档产物；Markdown 是唯一可编辑事实源。
- 当前不启用 AllToolsets；路线文档只记录启用和审计步骤。
