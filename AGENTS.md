# ProjectR Codex Instructions

## Project identity

- Project: 《首脑协议：共鸣搭档》
- Unreal project/module: ProjectR
- Engine: Unreal Engine 5.8
- Type: single-player 2.5D action roguelike
- Stack: C++, GAS, Enhanced Input, GameplayTags, DataAsset, versioned SaveGame, Unreal MCP

## Required reading order

Before any implementation:

1. `AGENTS.md`
2. `CURRENT_VERSION.md`
3. The task file referenced by CURRENT_VERSION
4. `Docs/Architecture/VersionDependencyMap.md` for the current version, its direct upstream, and listed downstream consumers
5. `Docs/Architecture/DataAndInterfaceContracts.md` sections named by the task
6. `Docs/Workflow/CodexWorkflow.md` for every implementation
7. `Docs/Workflow/CodingStandard.md` before C++ or configuration work
8. `Docs/Workflow/AssetNaming.md` before asset work
9. Only the directly relevant product/design sections

Implement only the current version. Do not advance CURRENT_VERSION yourself.

## Naming

- Actor: `APR*`
- UObject/Component/Subsystem: `UPR*`
- Struct: `FPR*`
- Enum: `EPR*`
- Source filenames: `PR*`
- Assets: `BP_`, `WBP_`, `DA_`, `GE_`, `GA_`, `VFX_`, `SFX_`, `L_`, `IA_`, `IMC_`

Do not silently rename public classes, Blueprint APIs, GameplayTags, serialized properties, directories, or asset paths. Destructive changes require an Architecture Decision, migration, compatibility test, and explicit approval.

## Architecture constraints

- Dependencies follow Core -> Infrastructure -> Combat -> Companion/QTE -> Director/Roguelike -> Content -> Platform/Release.
- Reverse collaboration uses interfaces, delegates, subsystem APIs, GameplayTags, or data contracts.
- Combat publishes CombatEvent and must not depend on specific QTE, Director Rule, chapter, dialogue, or UI classes.
- SaveGame is versioned from its first introduction; all loads migrate before use.
- Blueprint layers configure C++ systems; core damage, rule execution, save migration, LLM validation, and account lifecycle do not live in Widget graphs.

## Runtime AI constraints

- Real-time combat never waits for network/LLM.
- LLM providers return only validated structured fields: whitelist IDs, levels, reason tags, limited numeric parameters, and player-facing text.
- Unknown IDs, invalid schemas, out-of-range values, timeout, offline, or provider failure use deterministic fallback.
- Formal API keys never ship in the PC client or repository.

## Automation-first rule

- Codex owns C++, configuration, scripts, builds, tests, and compile-error fixes.
- Unreal Editor mechanical work defaults to Unreal MCP: assets, Blueprint parent/components/CDO/graphs, DataAssets, tables, materials, scene actors, saving, compile, logs, screenshots, and PIE.
- First use existing Toolsets; then selectively enable official specialized Toolsets; then create a minimal ProjectR editor-only Toolset. Human work is the final exception.
- Do not tell the user to manually create an asset until the capability audit and alternatives are recorded.

## Unreal MCP writes

- Never modify `.uasset/.umap` through shell, ordinary file IO, or binary patching.
- Serialize MCP writes to one UnrealEditor instance.
- Before writes: list exact Package paths, check exists/can-edit, stop on collisions, and produce an MCP Operation Manifest.
- Default write root: `/Game/ProjectR/`; test root: `/Game/ProjectR/MCPTest/`.
- Do not overwrite, rename, move, delete, migrate, fix redirectors, or resave all without explicit approval.
- After writes: save exact Packages, list Dirty Packages, compile affected Blueprints, validate references, reload after restart when required, and run relevant PIE/automation.
- Report asset created, Blueprint compiled, restart loaded, and PIE passed as separate states.

## Implementation scope

- No future-version business logic.
- No unrelated refactors.
- Files outside Allowed paths require stopping and explaining the dependency.
- Preserve Blueprint/Save compatibility.
- Prefer events/timers over Tick; delegates/events for UI; data-driven gameplay values; explicit failure logs.

## Required verification

Run every applicable command in the current task. Never claim Build, Blueprint compile, Editor restart, PIE, packaging, or subjective quality passed unless actually verified.

Before completion, run Future Compatibility Review against direct downstream consumers and update DevelopmentLog, KnownIssues, and ArchitectureDecisions as required.

## Completion report

1. Summary
2. Changed files
3. Created/modified UE Packages
4. Public APIs/contracts
5. MCP Operation Manifest results
6. Commands and actual results
7. Acceptance matrix PASS/FAIL/NOT RUN
8. Future Compatibility Review
9. Human judgment/manual steps with exact runbook
10. Known risks
11. Suggested commit message

Do not commit or push unless explicitly instructed.
