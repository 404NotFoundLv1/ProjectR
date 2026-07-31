# ProjectR Tier A Original Environment Slice Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce the Tier A original-art vertical slice consisting of the side-on Reality Hub backroom and the Allocator Ration Line combat room, using 34 ProjectR-owned source assets and the proven art pipeline.

**Architecture:** A single catalog generates 34 validated asset briefs and the exact Unreal Package manifest. Human approval freezes design at G1 and room graybox at G2; Blender/Substance batches then create authoritative source assets; Unreal MCP imports and assembles only two isolated art maps; G3/G4, gameplay regression, provenance, and clean-clone reconstruction close the slice.

**Tech Stack:** ProjectR art-pipeline foundation, Unreal Engine 5.8, Unreal MCP, Blender 4.5.5 LTS/Python/Geometry Nodes, Adobe Substance 3D Painter/Designer/SAT, Python 3.13, PowerShell, Git LFS.

## Global Constraints

- This plan depends on every required check in `2026-07-31-projectr-original-art-pipeline-foundation.md` passing, including the seven-Package Unreal smoke test and clean-clone reconstruction.
- Do not execute while `CURRENT_VERSION.md` points to `v0.7.2` or any task that does not explicitly authorize this plan's exact paths and Packages.
- Do not advance `CURRENT_VERSION.md`; the approved environment-art version task must already be active.
- Implement only two isolated maps: Reality Hub art slice and Allocator Ration Line art slice. Do not modify `L_RealityHub`, `L_Network_Prototype`, the room registry, or `DA_Room_Allocator_Combat_RationLine`.
- The slice is horizontal 2.5D. Gameplay occupies one continuous X/Z plane; decorative depth along Y has no player collision.
- Reality Hub is a semi-realistic abandoned game-store backroom/anti-Headmind hideout. Warmth is limited to cartridge, AI, repair, and relationship traces. It must not become cartoon low-poly.
- Ration Line is an institutional allocation pressure-test room built around identity reading, queuing, measurement, and adjudication. It must not become a generic neon cyber corridor.
- The newly produced environment payload contains no third-party final mesh, texture, material, decal, alpha, HDRI, animation, VFX, audio, or kit.
- AI output is exploration-only. Final geometry, UVs, materials, decals, and symbols are rebuilt in ProjectR-owned Blender/Substance sources.
- Asset source cap is exactly 34 base assets/generators: 6 shared, 10 Reality, 10 Allocator, 8 props. Additions require explicit scope approval.
- Tier A includes exactly 3 trim families, 2 decal atlases, 5 master-material responsibilities, 4 independent 2K hero texture sets, and 16 named material instances.
- Target performance is 1080p/60 fps on the `TargetHardwareId` frozen in the active art task. If that contract field is missing, performance is `NOT RUN` and closeout is blocked.
- Use one Unreal Editor instance. Serialize MCP writes. Check every Package before creation and stop on collisions.
- Do not overwrite, rename, move, delete, migrate, fix redirectors, or resave all without separate explicit approval. Reimport updates must be explicitly listed by Package in the active task and Operation Manifest.
- Do not commit unless the user explicitly authorizes commits for the execution session. Each commit step is conditional; otherwise report `NOT RUN — commit not authorized`.

---

## 12–18 Day Execution Cadence

This schedule is cumulative with the foundation plan; the foundation is D1–D2, not an extra project before the 12–18 day estimate.

| Workdays | Tasks on the critical path |
|---|---|
| D1–D2 | Foundation Tasks 1–7: contract/LFS, validators, Blender/Substance smoke, Unreal seven-Package smoke |
| D2–D5 | Tier A Tasks 1–2: exact catalog, 34 briefs, G1 room/asset boards |
| D4–D9 | Tier A Tasks 3–4: two G2 maps, shared generators and prop bases |
| D6–D12 | Tier A Tasks 5–6: Reality and Allocator final geometry |
| D6–D13 | Tier A Task 7: trims, decal atlases and four hero texture sets |
| D8–D16 | Tier A Tasks 8–10: bounded Unreal import, map assembly, lighting and fixed-camera checks |
| D14–D18 | Tier A Task 11: G3/G4, performance, clean-clone rebuild, regressions and closeout |

No Unreal import batch may exceed eight Packages without an intermediate save/readback/Dirty gate. No human review batch may mix G1, G2 and G4 decisions.

---

## Exact Asset Catalog

| Group | Asset ID | Unreal mesh name | Nominal dimensions cm | Priority | Construction contract |
|---|---|---|---:|---|---|
| Shared | `PR_ART_SHARED_WallPanel` | `SM_ArtShared_WallPanel` | 400×30×300 | P0 | Beveled framed plate, inset service seam, four authored fasteners, trim UV |
| Shared | `PR_ART_SHARED_DoorFrame` | `SM_ArtShared_DoorFrame` | 250×50×320 | P0 | Three-piece rectangular frame, replaceable top sensor block, clean 250 cm opening |
| Shared | `PR_ART_SHARED_FloorPanel` | `SM_ArtShared_FloorPanel` | 400×400×30 | P0 | Tiled slab with edge tongue, walkable top plane, separate non-colliding inset strips |
| Shared | `PR_ART_SHARED_SupportFrame` | `SM_ArtShared_SupportFrame` | 60×60×320 | P1 | Box-section column, two bracket collars, one cable pass-through |
| Shared | `PR_ART_SHARED_CableTray` | `SM_ArtShared_CableTray` | 300×35×25 | P1 | U-channel tray with repeated authored rung geometry and capped ends |
| Shared | `PR_ART_SHARED_DeviceShell` | `SM_ArtShared_DeviceShell` | 160×70×220 | P1 | Parametric equipment carcass with door/panel/screen sockets, no final identity markings |
| Reality | `PR_ART_RH_CartridgeSlot` | `SM_RealityHub_CartridgeSlot` | 120×60×180 | P0 | Heavy repaired pedestal, protected cartridge mouth, asymmetric cable repair, hero texture |
| Reality | `PR_ART_RH_Graveyard` | `SM_RealityHub_Graveyard` | 160×60×140 | P0 | Quiet bank of five dark account markers and one readable interaction ledge; no cute memorial props |
| Reality | `PR_ART_RH_CompanionTerminal` | `SM_RealityHub_CompanionTerminal` | 140×70×220 | P0 | Rebuilt arcade/server hybrid, narrow portrait display, physical maintenance doors, hero texture |
| Reality | `PR_ART_RH_TrainingSimulator` | `SM_RealityHub_TrainingSimulator` | 180×90×180 | P0 | Folded diagnostic rig with safe standing outline and restrained warning light |
| Reality | `PR_ART_RH_DirectorForecaster` | `SM_RealityHub_DirectorForecaster` | 140×70×190 | P0 | Analog prediction plotter, paper/trace output slot, shielded electronics body |
| Reality | `PR_ART_RH_MemoryWall` | `SM_RealityHub_MemoryWall` | 500×20×250 | P1 | Modular wall rails, six bounded record slots, repair clips, no photo collage import |
| Reality | `PR_ART_RH_EchoCartridge` | `SM_RealityHub_EchoCartridge` | 15×3×10 | P1 | Original retro cartridge relic with ProjectR silhouette and authored label geometry |
| Reality | `PR_ART_RH_BackroomStorage` | `SM_RealityHub_BackroomStorage` | 240×80×240 | P1 | Old retail-stock shelving frame with sparse authored shelf dividers |
| Reality | `PR_ART_RH_RepairWorkbench` | `SM_RealityHub_RepairWorkbench` | 200×80×100 | P1 | Reinforced bench, clamped repair surface, two drawers and cable-safe rear rail |
| Reality | `PR_ART_RH_ServerMainframe` | `SM_RealityHub_ServerMainframe` | 300×90×260 | P1 | Three-bay low server wall for non-walkable background; doors and vents create slow rhythm |
| Allocator | `PR_ART_ARL_IdentityGate` | `SM_AllocatorRationLine_IdentityGate` | 250×80×350 | P0 | Entry arch with face/body scan plane and three institutional status bars |
| Allocator | `PR_ART_ARL_QuotaGate` | `SM_AllocatorRationLine_QuotaGate` | 250×80×350 | P0 | Exit arch with descending adjudication shutter and hard quota indicator |
| Allocator | `PR_ART_ARL_RationCounter` | `SM_AllocatorRationLine_RationCounter` | 300×100×140 | P0 | Long allocation counter, controlled opening, integrated resource scale, hero texture |
| Allocator | `PR_ART_ARL_QuotaMeter` | `SM_AllocatorRationLine_QuotaMeter` | 160×60×240 | P1 | Tall physical meter with segmented numeric bands and mechanical pointer, hero texture |
| Allocator | `PR_ART_ARL_ObservationWindow` | `SM_AllocatorRationLine_ObservationWindow` | 600×40×180 | P1 | Recessed one-way institutional window with repeated observer bays |
| Allocator | `PR_ART_ARL_ResourceConduit` | `SM_AllocatorRationLine_ResourceConduit` | 400×60×60 | P1 | Rectangular measured-flow conduit with spaced inspection collars |
| Allocator | `PR_ART_ARL_QueueBarrier` | `SM_AllocatorRationLine_QueueBarrier` | 250×40×110 | P0 | Low retractable rail whose silhouette defines queue pressure without obscuring combat |
| Allocator | `PR_ART_ARL_AllocationDisplay` | `SM_AllocatorRationLine_AllocationDisplay` | 200×40×120 | P1 | Wide low-emission board for iconographic allocation values, no language-dependent stock UI |
| Allocator | `PR_ART_ARL_Conveyor` | `SM_AllocatorRationLine_Conveyor` | 400×160×90 | P1 | Sealed resource belt with sparse trays; animation-ready but static in Tier A |
| Allocator | `PR_ART_ARL_PressureFrame` | `SM_AllocatorRationLine_PressureFrame` | 400×100×360 | P1 | Structural measurement arch spanning background, with three calibration planes |
| Props | `PR_ART_PROP_Crate` | `SM_ArtProp_Crate` | 90×70×60 | P2 | Stack-safe ribbed box; parameters create 0.75×/1×/1.25× variants |
| Props | `PR_ART_PROP_Tray` | `SM_ArtProp_Tray` | 70×50×8 | P2 | Shallow lipped allocation/repair tray with two corner grips |
| Props | `PR_ART_PROP_Bracket` | `SM_ArtProp_Bracket` | 40×20×50 | P2 | L bracket with two holes and one gusset; mirrored variant allowed |
| Props | `PR_ART_PROP_CableBundle` | `SM_ArtProp_CableBundle` | 200×20×20 | P2 | Four authored curves converted to mesh, restrained sag, two clamp points |
| Props | `PR_ART_PROP_TerminalBlock` | `SM_ArtProp_TerminalBlock` | 35×15×20 | P2 | Six-contact electrical block with protective edge |
| Props | `PR_ART_PROP_ToolSet` | `SM_ArtProp_ToolSet` | 80×40×15 | P2 | Three original simple tools on one shadow-board footprint; no branded silhouette |
| Props | `PR_ART_PROP_Seal` | `SM_ArtProp_Seal` | 25×4×35 | P2 | Tamper seal plate and wire loop; reality/network material variants |
| Props | `PR_ART_PROP_MaintenancePanel` | `SM_ArtProp_MaintenancePanel` | 90×10×120 | P2 | Flush access panel, hinge edge, latch and bounded warning-symbol field |

## Exact Runtime Material Catalog

Master materials:

```text
/Game/ProjectR/Art/Shared/Materials/M_PR_SurfaceOpaque
/Game/ProjectR/Art/Shared/Materials/M_PR_DisplayEmissive
/Game/ProjectR/Art/Shared/Materials/M_PR_Glass
/Game/ProjectR/Art/Shared/Materials/M_PR_Decal
/Game/ProjectR/Art/Shared/Materials/M_PR_CableFabric
```

Material instances:

```text
/Game/ProjectR/Art/RealityHub/Materials/MI_RealityHub_Structure
/Game/ProjectR/Art/RealityHub/Materials/MI_RealityHub_MetalPaint
/Game/ProjectR/Art/RealityHub/Materials/MI_RealityHub_WornPlastic
/Game/ProjectR/Art/RealityHub/Materials/MI_RealityHub_Screen
/Game/ProjectR/Art/RealityHub/Materials/MI_RealityHub_Glass
/Game/ProjectR/Art/RealityHub/Materials/MI_RealityHub_Cartridge
/Game/ProjectR/Art/RealityHub/Materials/MI_RealityHub_MemoryWall
/Game/ProjectR/Art/RealityHub/Materials/MI_RealityHub_Cable
/Game/ProjectR/Art/Allocator/RationLine/Materials/MI_Allocator_Structure
/Game/ProjectR/Art/Allocator/RationLine/Materials/MI_Allocator_SterileMetal
/Game/ProjectR/Art/Allocator/RationLine/Materials/MI_Allocator_Queue
/Game/ProjectR/Art/Allocator/RationLine/Materials/MI_Allocator_Display
/Game/ProjectR/Art/Allocator/RationLine/Materials/MI_Allocator_Glass
/Game/ProjectR/Art/Allocator/RationLine/Materials/MI_Allocator_Conduit
/Game/ProjectR/Art/Allocator/RationLine/Materials/MI_Allocator_Decal
/Game/ProjectR/Art/Allocator/RationLine/Materials/MI_Allocator_Cable
```

### Task 1: Generate and Validate the Exact Tier A Catalog

**Files:**
- Create: `Docs/Art/Manifests/TierAAssetCatalog.json`
- Create: `BuildScripts/ArtPipeline/generate_tier_a_briefs.py`
- Create: `BuildScripts/ArtPipeline/Tests/test_tier_a_catalog.py`
- Create: 34 JSON files under `Docs/Art/Briefs/TierA/`
- Update: `SourceArt/ProjectR/Provenance/AssetLedger.csv`

**Interfaces:**
- Consumes: the table above and foundation `AssetBrief` validation.
- Produces: exactly 34 validated brief files and 34 unique mesh Package rows used by DCC generation and the MCP manifest.

- [ ] **Step 1: Write the RED catalog test**

Create a test that loads `TierAAssetCatalog.json` and asserts:

```python
self.assertEqual(len(catalog["assets"]), 34)
self.assertEqual(Counter(a["group"] for a in catalog["assets"]), {
    "Shared": 6, "RealityHub": 10, "AllocatorRationLine": 10, "Props": 8
})
self.assertEqual(len({a["asset_id"] for a in catalog["assets"]}), 34)
self.assertEqual(len({a["ue_package"] for a in catalog["assets"]}), 34)
self.assertTrue(all(a["third_party_final_assets"] is False for a in catalog["assets"]))
```

Run `python -m unittest BuildScripts.ArtPipeline.Tests.test_tier_a_catalog -v`. Expected: FAIL because the catalog is absent.

- [ ] **Step 2: Create the catalog with the exact 34 rows**

Encode every row in the Exact Asset Catalog table with `asset_id`, `group`, `mesh_name`, `dimensions_cm`, `priority`, `construction_contract`, `source_blend`, `export_fbx`, `ue_package`, `collision`, `material_profile`, and `third_party_final_assets=false`.

Package roots are exact:

```text
Shared -> /Game/ProjectR/Art/Shared/Geometry/
RealityHub -> /Game/ProjectR/Art/RealityHub/Geometry/
AllocatorRationLine -> /Game/ProjectR/Art/Allocator/RationLine/Geometry/
Props -> /Game/ProjectR/Art/Shared/Props/
```

- [ ] **Step 3: Implement deterministic brief generation**

`generate_tier_a_briefs.py` must sort by `asset_id`, render one foundation-schema JSON per asset, use UTF-8 with a final newline, refuse duplicate output paths, and refuse to overwrite a file whose parsed content differs unless `--replace-approved` is supplied by an explicitly approved migration task.

- [ ] **Step 4: Generate and validate all briefs**

```powershell
python BuildScripts/ArtPipeline/generate_tier_a_briefs.py Docs/Art/Manifests/TierAAssetCatalog.json Docs/Art/Briefs/TierA
Get-ChildItem Docs/Art/Briefs/TierA/*.json | ForEach-Object { python BuildScripts/ArtPipeline/pr_art_cli.py validate-brief $_.FullName; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE } }
python -m unittest BuildScripts.ArtPipeline.Tests.test_tier_a_catalog -v
```

Expected: exactly 34 files and all tests pass.

- [ ] **Step 5: Seed the 34 provenance rows**

Append one row per asset with source/export paths and `g1=PENDING,g2=PENDING,g3=PENDING,g4=PENDING`. Validate the ledger. Expected: no duplicate ID/Package and no third-party final payload.

- [ ] **Step 6: Commit only if authorized**

```powershell
git add Docs/Art/Manifests/TierAAssetCatalog.json Docs/Art/Briefs/TierA BuildScripts/ArtPipeline SourceArt/ProjectR/Provenance/AssetLedger.csv
git commit -m "feat: freeze Tier A environment catalog"
```

Otherwise report the commit step as not run.

### Task 2: Produce and Approve G1 Room/Asset Design Boards

**Files:**
- Create: `Docs/Art/Reviews/G1_RealityHub.md`
- Create: `Docs/Art/Reviews/G1_AllocatorRationLine.md`
- Create: `Docs/Art/Reviews/G1_AssetFamilies.md`
- Create: `SourceArt/ProjectR/Concepts/RealityHub/RealityHub_DesignBoard_v001.png`
- Create: `SourceArt/ProjectR/Concepts/AllocatorRationLine/AllocatorRationLine_DesignBoard_v001.png`
- Create: `SourceArt/ProjectR/Concepts/Shared/AssetFamilies_DesignBoard_v001.png`
- Create: generation/provenance JSON beside every AI-assisted concept image.

**Interfaces:**
- Consumes: 34 approved briefs and the ProjectR product/design sections cited by the specification.
- Produces: three frozen G1 boards with consistent side/face/back views, dimensions, material zones, and explicit user approval.

- [ ] **Step 1: Build the reference-only constraint sheet**

Record the required traits and prohibitions:

```text
Required: horizontal side view, semi-realistic stylized PBR, restrained institutional shapes, mature value structure.
Reality required: abandoned retail backroom, repair scars, bounded warm accents, retro cartridge as relic.
Allocator required: queue, measurement, adjudication, resource flow, low-saturation cyan/white system light.
Forbidden: isometric room, chibi/cartoon proportions, generic neon alley, purchased-kit silhouette, copied branding, imported reference collage.
```

- [ ] **Step 2: Generate exploration only when image generation is available**

Generate silhouette/material candidates in batches; save provider, model/version, prompt, task ID/seed, date, and selection result in sidecar JSON. If the service fails, continue with Blender blockout/Grease Pencil; do not block G1 on provider recovery.

- [ ] **Step 3: Rebuild the three unified boards**

Each board must show one coherent design language, a 180 cm person, target side camera, front/back or sectional views, nominal dimensions, material zones, collision intent, and every relevant catalog Asset ID. AI images may sit in a clearly marked exploration strip but may not substitute for coherent multi-view construction.

- [ ] **Step 4: Run the machine checks**

Require three non-empty PNGs, three provenance records, all 34 Asset IDs mentioned exactly once across the asset-family index, and no untracked external reference file under `SourceArt/`.

- [ ] **Step 5: Conduct G1 human review**

The user reviews Reality, Allocator, and family boards as one batch. Record `PASS` only when side-on composition, semi-realistic tone, world distinction, and every P0 silhouette are accepted. If rejected, increment the filename (`v002`, `v003`); never overwrite an approved board.

- [ ] **Step 6: Update ledger and commit only if authorized**

Set `g1=PASS` for all 34 rows only after written approval. Commit with `art: freeze Tier A environment design boards` if authorized; otherwise report not run.

### Task 3: Assemble and Approve the Two G2 Graybox Maps

**Files:**
- Create through Blender: `SourceArt/ProjectR/Blender/Rooms/L_ArtSlice_RealityHub_Blockout.blend`
- Create through Blender: `SourceArt/ProjectR/Blender/Rooms/L_ArtSlice_AllocatorRationLine_Blockout.blend`
- Create via Unreal MCP: `/Game/ProjectR/Art/VerticalSlice/Maps/L_ArtSlice_RealityHub`
- Create via Unreal MCP: `/Game/ProjectR/Art/VerticalSlice/Maps/L_ArtSlice_AllocatorRationLine`
- Create: `Docs/Art/Manifests/MCP_TierA_G2_Maps.json`
- Create: `Docs/Art/Reviews/G2_RealityHub.md`
- Create: `Docs/Art/Reviews/G2_AllocatorRationLine.md`

**Interfaces:**
- Consumes: G1 boards and existing read-only player/enemy/interaction actors.
- Produces: two isolated side-on graybox maps with accepted dimensions, camera, playable floor, and station/combat clearances.

- [ ] **Step 1: Write and collision-check the two-Package map manifest**

The manifest contains only the two exact map Packages, `overwrite=false`, and no other write operation. Both must be absent and editable before creation.

- [ ] **Step 2: Build the Reality Hub Blender blockout**

Use X from `-1200` to `+1200`, Z floor at `0`, and a single gameplay lane centered at Y=`0`. Place nominal station volumes at:

```text
Graveyard X=-1020
MemoryWall X=-720
CartridgeSlot X=-260
CompanionTerminal X=260
DirectorForecaster X=690
TrainingSimulator X=1010
ServerMainframe background X=620, Y=260
```

Keep the underfloor/server bay visual-only. Validate 2400 cm visible width and 650 cm clear height.

- [ ] **Step 3: Build the Ration Line Blender blockout**

Use X from `-1400` to `+1400`, Z floor at `0`, and the same gameplay Y plane. Place:

```text
IdentityGate X=-1180
RationCounter X=-520
QueueBarrier span X=-650..+650
QuotaMeter X=520
QuotaGate X=1180
ObservationWindow background X=0, Y=280, Z=300
PressureFrame background X=0, Y=220
```

Validate 2800 cm combat width, 700 cm clear height, and no platforming requirement.

- [ ] **Step 4: Create the Unreal maps with test-only primitives**

Use engine primitives only as clearly documented non-final graybox actors. Reference existing ProjectR gameplay actors read-only; do not modify registries or public APIs. Decorative graybox actors have collision disabled. Save only the two maps.

- [ ] **Step 5: Run Reality G2 checks**

Verify one continuous walkable floor, all six affordances visible across two smooth horizontal framings, no interaction volume obstruction, no second-level navigation, and no foreground occlusion of the player.

- [ ] **Step 6: Run Ration Line G2 checks**

Load the existing Ration Line encounter through an isolated fixture that does not modify `DA_Room_Allocator_Combat_RationLine`. Verify enemy movement, attacks, projectiles, gates and encounter completion across the flat 26–28 m lane.

- [ ] **Step 7: Conduct G2 human review**

Capture fixed camera screenshots and short gameplay evidence. The user approves spatial rhythm, station visibility, battle readability, and world identity. On failure, modify only the isolated maps/blockout sources through a new exact manifest.

- [ ] **Step 8: Remove final-map dependency on test-only primitives later, not now**

Record every primitive actor in the G2 review. They must all be replaced before G4; G2 does not claim final originality.

- [ ] **Step 9: Commit only if authorized**

Commit blockout sources, map Packages, manifests, and review records with `art: approve Tier A room grayboxes`; otherwise report not run.

### Task 4: Produce the Six Shared Generators and Eight Prop Bases

**Files:**
- Create: `SourceArt/ProjectR/Generators/blender/shared_structure.py`
- Create: `SourceArt/ProjectR/Generators/blender/prop_assets.py`
- Create: `BuildScripts/ArtPipeline/Tests/Test-TierASharedAssets.ps1`
- Create through Blender: 14 `.blend` files matching the catalog source paths.
- Create through Blender: 14 `.fbx` files matching the catalog export paths.

**Interfaces:**
- Consumes: approved G1 design board and 14 exact briefs.
- Produces: reusable ProjectR-authored structure/prop sources with validated dimensions, pivots, UVs, collisions, and semantic export metrics.

- [ ] **Step 1: Write RED batch validation**

The PowerShell test must require all 14 Asset IDs, exact render object names, catalog dimensions within 1%, origin/pivot policy, UV0, triangulation modifier, material-slot count ≤2, and only approved collision prefixes. Run before generators exist; expect FAIL.

- [ ] **Step 2: Implement the six shared constructors**

Expose exactly:

```python
build_wall_panel(spec)
build_door_frame(spec)
build_floor_panel(spec)
build_support_frame(spec)
build_cable_tray(spec)
build_device_shell(spec)
```

Each function follows its construction contract in the catalog table, uses bevel widths proportional to the brief, produces trim-ready UV islands, and adds collision only to the structural silhouette—not vents, fasteners, or inset strips.

- [ ] **Step 3: Implement the eight prop constructors**

Expose exactly:

```python
build_crate(spec); build_tray(spec); build_bracket(spec); build_cable_bundle(spec)
build_terminal_block(spec); build_tool_set(spec); build_seal(spec); build_maintenance_panel(spec)
```

Only crate/bracket/cable-bundle parameter variants are emitted; the eight authoritative sources remain the catalog bases.

- [ ] **Step 4: Generate sources and exports**

Run Blender factory-startup per asset so a failure cannot contaminate later sources. Save `.blend`, validate, then export FBX. Do not continue after the first invalid asset.

- [ ] **Step 5: Run GREEN and scene-fit checks**

Run the headless batch test and place all 14 untextured meshes in both blockout scenes. Check modular seams at 1 cm tolerance, pivots at the intended snap corner, and no gameplay-lane collision from decorative props.

- [ ] **Step 6: Update provenance and commit only if authorized**

Write source/export hashes and generator version to the 14 ledger rows. Commit `art: build shared environment generators` if authorized; otherwise report not run.

### Task 5: Produce the Ten Reality Hub Assets

**Files:**
- Create: `SourceArt/ProjectR/Generators/blender/reality_hub_assets.py`
- Create: `BuildScripts/ArtPipeline/Tests/Test-TierARealityAssets.ps1`
- Create through Blender: 10 Reality `.blend` files and 10 Reality `.fbx` exports.

**Interfaces:**
- Consumes: G1 Reality board, G2 Reality blockout, shared/prop sources, and ten exact Reality briefs.
- Produces: ten final-geometry Reality sources ready for texturing and Unreal import.

- [ ] **Step 1: Write RED tests for all ten Reality assets**

Assert exact object names, dimensions, pivots, UV0, collision policy, triangle counts within the specification's provisional bands, and zero dependency on files outside `SourceArt/ProjectR/`.

- [ ] **Step 2: Build the five device stations and memory wall**

Use `SM_ArtShared_DeviceShell` only as ProjectR-linked geometry logic, then create distinct silhouettes:

```text
CartridgeSlot: low heavy pedestal + protected central mouth + asymmetrical repaired cable.
Graveyard: horizontal quiet marker bank + one interaction ledge; no animated/cute elements.
CompanionTerminal: tall arcade/server hybrid + portrait display + service doors.
TrainingSimulator: wide diagnostic rig + clear standing zone + folded side modules.
DirectorForecaster: narrow plotter + output slot + shielded analog/digital stack.
MemoryWall: long rail surface + six bounded record bays + repair clips.
```

- [ ] **Step 3: Build the four supporting Reality assets**

Build the Echo Cartridge, Backroom Storage, Repair Workbench, and Server Mainframe exactly as the catalog contracts. The cartridge uses an original unbranded silhouette; the server mainframe is optimized for background viewing and has no player collision.

- [ ] **Step 4: Generate, validate, and export the ten sources**

Use one clean Blender process per asset. Fail fast on naming, scale, pivot, UV, collision, missing linked generator version, or out-of-root dependency.

- [ ] **Step 5: Replace Reality blockout volumes in Blender**

Link all ten final meshes into the Reality room source. Compare fixed side-view screenshots to G2: stations may change local detail but not approved centers, floor clearance, or camera framing without a new G2 review.

- [ ] **Step 6: Update provenance and commit only if authorized**

Record source/export hashes for ten rows. Commit `art: build Reality Hub source assets` if authorized; otherwise report not run.

### Task 6: Produce the Ten Allocator Ration Line Assets

**Files:**
- Create: `SourceArt/ProjectR/Generators/blender/allocator_ration_line_assets.py`
- Create: `BuildScripts/ArtPipeline/Tests/Test-TierAAllocatorAssets.ps1`
- Create through Blender: 10 Allocator `.blend` files and 10 Allocator `.fbx` exports.

**Interfaces:**
- Consumes: G1 Allocator board, G2 combat blockout, shared/prop sources, and ten exact Allocator briefs.
- Produces: ten final-geometry Allocator sources ready for texturing and Unreal import.

- [ ] **Step 1: Write RED tests for all ten Allocator assets**

Assert exact names/dimensions/pivots/UV/collision/triangle bands and zero external source dependency. Additionally require QueueBarrier collision height ≤110 cm and both gates to preserve a 250 cm clear opening.

- [ ] **Step 2: Build the three gameplay-silhouette assets**

Build IdentityGate, QuotaGate, and QueueBarrier first. Test them with existing player/enemy silhouettes; the rail cannot hide lower-body attack anticipation and the gates cannot create unintended ledges.

- [ ] **Step 3: Build the two hero measurement assets**

Build RationCounter and QuotaMeter with restrained institutional segmentation, original icon fields, and dedicated hero-texture UVs. No generic sci-fi keyboard wall or imported glyph sheet.

- [ ] **Step 4: Build the five background/system assets**

Build ObservationWindow, ResourceConduit, AllocationDisplay, Conveyor, and PressureFrame. The conveyor is static in Tier A; observation silhouettes are geometry/light rhythm only, not characters.

- [ ] **Step 5: Generate, validate, export, and refit the room source**

Run one clean Blender process per asset, then link all ten into the Ration Line room source. Preserve the G2 combat lane, spawn clearance, gate centers, and fixed camera.

- [ ] **Step 6: Update provenance and commit only if authorized**

Record ten source/export hashes. Commit `art: build Allocator Ration Line assets` if authorized; otherwise report not run.

### Task 7: Author the Tier A Materials and Texture Sets

**Files:**
- Create through Substance Designer: three `.sbs` trim sources under `SourceArt/ProjectR/Substance/Designer/Trims/`
- Create through Substance Designer: two `.sbs` decal-atlas sources under `SourceArt/ProjectR/Substance/Designer/Decals/`
- Create through Substance Painter: four `.spp` hero sources under `SourceArt/ProjectR/Substance/Painter/Heroes/`
- Create: `Docs/Art/Manifests/TierATextureCatalog.json`
- Create: `BuildScripts/ArtPipeline/Tests/test_tier_a_textures.py`
- Create: 27 exported texture files under `SourceArt/ProjectR/Exports/Textures/TierA/`

**Interfaces:**
- Consumes: the final geometry and foundation `ProjectR_UE58_ORM` preset.
- Produces: 3 trim × 3 maps, 2 decal atlases × 3 maps, and 4 hero sets × 3 maps = exactly 27 validated 2K textures.

- [ ] **Step 1: Write the exact texture catalog and RED tests**

The families are:

```text
Trim_RealityStructure: BC, N, ORM
Trim_AllocatorStructure: BC, N, ORM
Trim_SharedProps: BC, N, ORM
Decal_RealityRepair: BC, N, ORM
Decal_AllocatorQuota: BC, N, ORM
Hero_RealityHub_CartridgeSlot: BC, N, ORM
Hero_RealityHub_CompanionTerminal: BC, N, ORM
Hero_AllocatorRationLine_RationCounter: BC, N, ORM
Hero_AllocatorRationLine_QuotaMeter: BC, N, ORM
```

Tests assert 27 unique Package names, 2048² PNGs, `_BC/_N/_ORM` triplets, `third_party_final_assets=false`, and no unlisted export file.

- [ ] **Step 2: Author the three trim graphs**

Use only Designer primitive nodes, ProjectR-authored vector shapes, noises, gradients, and parameters. Reality trim exposes old paint, repaired metal, dark polymer and restrained warm insert values. Allocator trim exposes sterile coated metal, measured seams, off-white composite and desaturated cyan system strips. Shared-prop trim exposes neutral metal, rubber, cable fabric and unpainted fasteners.

- [ ] **Step 3: Author the two decal atlases**

Reality atlas contains original repair arrows, cable labels, bounded warning fields and non-linguistic maintenance marks. Allocator atlas contains original queue numbers, quota ticks, allocation icons and adjudication bars. Do not use copied fonts/logos or downloaded alphas.

- [ ] **Step 4: Author the four hero Painter projects**

Use baked maps plus ProjectR-authored fill/mask logic. Each hero keeps local wear subordinate to its silhouette; no stock smart material is accepted as final appearance.

- [ ] **Step 5: Export and validate all 27 textures**

Use `ProjectR_UE58_ORM`, 2048 resolution, DirectX normal, and exact catalog filenames. Run texture tests and inspect representative channel contact sheets: `_BC` sRGB; `_N` and `_ORM` linear; ORM R=AO/G=Roughness/B=Metallic.

- [ ] **Step 6: Update provenance and commit only if authorized**

Record each source/derivative relationship and hash. Commit `art: author Tier A environment materials` if authorized; otherwise report not run.

### Task 8: Create the Exact Unreal Material and Import Manifest

**Files:**
- Create: `Docs/Art/Manifests/MCP_TierA_Import.json`
- Create: `BuildScripts/ArtPipeline/Tests/test_tier_a_mcp_manifest.py`
- Conditional modify: `PRArtPipelineAuthoringToolset.h/.cpp`, its tests, module registration, and Build.cs.
- Create via Unreal MCP: 34 mesh Packages, 27 texture Packages, 5 master materials, and 16 material instances.

**Interfaces:**
- Consumes: all validated DCC derivatives and the foundation MCP capability route.
- Produces: exactly 82 non-map runtime art Packages, configured and restart-stable.

- [ ] **Step 1: Generate the 84-Package full manifest**

The complete Tier A manifest contains:

```text
34 static meshes
27 textures
5 master materials
16 material instances
2 maps
= 84 Packages
```

`MCP_TierA_Import.json` is the 82-package non-map subset; the two maps remain in the already approved map manifest. Set overwrite/delete/move/rename to false.

- [ ] **Step 2: Write RED manifest tests**

Assert exact counts, unique paths, `/Game/ProjectR/Art/` root, catalog closure, no current frozen-map path, and no Package outside the approved task. Run before manifest generation; expect FAIL.

- [ ] **Step 3: Check all 82 destinations before write**

Query each Package individually. Any existing path is a collision and blocks the batch. Do not suffix or overwrite.

- [ ] **Step 4: Create the five master materials**

Use official Material tools when the foundation audit proved exact graph authoring/readback. Otherwise extend the fixed no-argument Toolset with `CreateTierAMaterialManifest()` hard-coded to the five paths and parameter contracts:

```text
M_PR_SurfaceOpaque: BC texture, N texture, ORM texture, tint, roughness scalar, emissive mask scalar, vertex-color variation.
M_PR_DisplayEmissive: display texture, color, intensity, scanline scalar, no time animation by default.
M_PR_Glass: tint, roughness, opacity; Tier A uses restrained translucency.
M_PR_Decal: BC/N/ORM atlas inputs, atlas UV scale/offset, opacity.
M_PR_CableFabric: BC/N/ORM inputs, tint, roughness, vertex-color variation.
```

- [ ] **Step 5: Import one representative asset per category**

Import one shared, one Reality, one Allocator and one prop mesh plus one texture triplet. Read back scale, collision, slots, sRGB/compression and parent parameters before approving batch import.

- [ ] **Step 6: Import the remaining assets in bounded batches**

Use batches of at most eight Packages. Save exact batch Packages, read back settings, check references and Dirty state, then continue. A batch failure stops later batches.

- [ ] **Step 7: Create the 16 exact material instances**

Bind only catalog textures and the five approved parents. Reality instances use restrained warm accents; Allocator instances use low-saturation institutional values. No instance uses time-driven emissive motion by default.

- [ ] **Step 8: Restart and re-read all 82 Packages**

Close normally, verify no save prompt, restart, load every manifest Package, confirm Dirty=0, missing reference count=0, source-file links are inside `SourceArt/ProjectR/Exports/`, and material parents/parameters match the manifest.

- [ ] **Step 9: Run GREEN tests and commit only if authorized**

Run manifest tests, BuildEditor if the Toolset changed, Data Validation, and exact asset checks. Commit `art: import Tier A environment package manifest` if authorized; otherwise report not run.

### Task 9: Finish and Accept the Reality Hub Art-Slice Map

**Files:**
- Modify via Unreal MCP: `/Game/ProjectR/Art/VerticalSlice/Maps/L_ArtSlice_RealityHub`
- Create: `Docs/Art/Reviews/G4_RealityHub.md`
- Update: `SourceArt/ProjectR/Provenance/AssetLedger.csv`

**Interfaces:**
- Consumes: G2 map, Reality/shared/prop assets, material instances, and existing read-only RealityHub interaction actors.
- Produces: the final Tier A Reality Hub isolated environment map and G4 acceptance evidence.

- [ ] **Step 1: Write a map-update manifest**

List only `L_ArtSlice_RealityHub` as modified and every art Package as a read-only dependency. Explicitly list replacement of all G2 primitive actors. The active task must authorize this exact map modification.

- [ ] **Step 2: Replace every graybox primitive**

Place final meshes at the G2-approved centers. Keep gameplay plane Y=0. Background assets use Y≥200 and collision disabled; sparse foreground frames use Y≤-180 and must not overlap the player silhouette in either approved framing.

- [ ] **Step 3: Bind art to existing interaction affordances without changing logic**

Place the five device shells and memory wall adjacent to existing read-only interaction volumes/actors. Do not reparent, rename, or modify their public Blueprint/API contracts. Verify each interaction still fires at the same gameplay position.

- [ ] **Step 4: Establish the Reality lighting/value hierarchy**

Use cool low ambient light, bounded warm accents at cartridge/companion/repair points, and low-contrast server background. Avoid broad orange fill, saturated arcade colors, or emissive walls.

- [ ] **Step 5: Run machine checks**

Require zero G2 primitive references, zero unexpected collision in decorative depth, six visible affordances, no missing materials/textures, no out-of-root art reference, map Dirty=false after save, and successful restart reload.

- [ ] **Step 6: Run interaction and camera PIE**

Walk the full 22–24 m lane, exercise all available existing interactions, transition between the two horizontal framings, and confirm no camera pop, collision snag, or foreground occlusion.

- [ ] **Step 7: Conduct G4 human review**

Capture fixed 1080p images for left framing, central cartridge/companion transition, and right system framing. User acceptance covers semi-realistic tone, restrained warmth, readable affordances, and absence of cartoon/kit appearance.

- [ ] **Step 8: Update ledger and commit only if authorized**

Set `g3=PASS,g4=PASS` for Reality and shared/prop rows used by the map. Commit `art: finish Reality Hub vertical slice` if authorized; otherwise report not run.

### Task 10: Finish and Accept the Allocator Ration Line Art-Slice Map

**Files:**
- Modify via Unreal MCP: `/Game/ProjectR/Art/VerticalSlice/Maps/L_ArtSlice_AllocatorRationLine`
- Create: `Docs/Art/Reviews/G4_AllocatorRationLine.md`
- Update: `SourceArt/ProjectR/Provenance/AssetLedger.csv`

**Interfaces:**
- Consumes: G2 combat map, Allocator/shared/prop assets, material instances, and the existing read-only Ration Line encounter path.
- Produces: the final Tier A Allocator combat environment map and G4 gameplay/art acceptance.

- [ ] **Step 1: Write a map-update manifest**

List only `L_ArtSlice_AllocatorRationLine` as modified and all art/gameplay dependencies as read-only. Explicitly list replacement of every G2 primitive.

- [ ] **Step 2: Replace every graybox primitive**

Preserve the approved X positions and continuous combat floor. Gates and QueueBarrier retain P0 collision; all observation windows, conduits, displays and pressure frames are background/non-colliding.

- [ ] **Step 3: Establish institutional material/light hierarchy**

Use off-white/dark structural fields, desaturated cyan measurement light, sparse warning accents, and low-frequency background values. No neon alley palette, animated billboard wall, or high-contrast observer silhouettes.

- [ ] **Step 4: Run full encounter gameplay**

Use the isolated fixture to run the existing `DA_Room_Allocator_Combat_RationLine` encounter without editing the DataAsset. Verify enemy spawn/movement, attacks, projectiles, damage feedback, gate closure/opening, and encounter completion.

- [ ] **Step 5: Run combat readability checks**

At fixed 1080p capture idle, maximum enemy count, projectile-heavy, hit-impact, and gate-transition frames. Player/enemy/projectile silhouettes must outrank background displays and decals. Lower background contrast/motion before changing gameplay assets.

- [ ] **Step 6: Run machine checks**

Require zero G2 primitives, no collision snag or unintended ledge, no missing references, no out-of-root art source, restart-stable map, and Dirty=false after exact save.

- [ ] **Step 7: Conduct G4 human review**

User acceptance covers horizontal battle readability, clear “queue/measurement/adjudication” identity, visual distinction from Reality, and absence of generic neon cyber styling.

- [ ] **Step 8: Update ledger and commit only if authorized**

Set `g3=PASS,g4=PASS` for Allocator and remaining shared/prop rows. Commit `art: finish Allocator Ration Line vertical slice` if authorized; otherwise report not run.

### Task 11: Performance, Reconstruction, Regression, and Closeout

**Files:**
- Create under ignored root: `Saved/AutomationReports/tier-a-art-slice-<timestamp>/`
- Modify: `Docs/DevelopmentLog.md`
- Modify: active art version task file as allowed by its closeout contract
- Modify if needed: `Docs/KnownIssues.md`
- Modify if a decision changed: `Docs/ArchitectureDecisions.md`
- Update: `Docs/Art/Reviews/G4_RealityHub.md`
- Update: `Docs/Art/Reviews/G4_AllocatorRationLine.md`
- Update: `SourceArt/ProjectR/Provenance/AssetLedger.csv`

**Interfaces:**
- Consumes: all 84 Packages, 34 briefs/sources, 27 textures, 16 instances, two G4 approvals, and active-task performance target.
- Produces: the auditable Tier A completion report and future-compatibility review.

- [ ] **Step 1: Run all source validation**

Run Python tests, all Blender batch tests, 34 brief validations, ledger validation, 27 texture checks, manifest tests, `git lfs fsck`, and `git diff --check`. Expected: all exit 0.

- [ ] **Step 2: Run Unreal technical validation**

Run the active task's exact BuildEditor, art automation, Data Validation, Package inventory, reference validation, restart readback, Dirty gate, and both isolated-map PIE suites. Report build, asset creation, restart load, map load, PIE and human review separately.

- [ ] **Step 3: Measure performance on the frozen target**

At 1080p and the task's target quality setting, capture frame-time evidence for:

```text
Reality left framing
Reality right/system framing
Ration Line idle
Ration Line maximum expected enemy/projectile load
```

Required result: sustained target 60 fps under the task contract's sampling rule, with no new blocking shader-compilation hitch in a warmed run. If `TargetHardwareId` or sampling rule is absent, report `NOT RUN` and block closeout.

- [ ] **Step 4: Prove clean-clone reconstruction**

In a validated directory under `tmp/`, clone with LFS, run all brief/source/texture tests, regenerate one shared, one Reality, one Allocator and one prop export, compare semantic metrics, and verify Unreal source links remain resolvable. Do not delete the clone without explicit approval.

- [ ] **Step 5: Run required gameplay regressions**

Run direct downstream RealityHub and Allocator suites plus current historical regression commands. Confirm no new Save schema, registry, GameplayTag, combat authority, room logic, input, or public API change.

- [ ] **Step 6: Perform Future Compatibility Review**

Review:

```text
L_RealityHub and its five device interactions plus memory wall presentation
L_Network_Prototype and DA_Room_Allocator_Combat_RationLine
existing side-scroller camera/movement/collision
Room/Encounter/Enemy registries as read-only consumers
future room reuse of shared generators/material contracts
LFS growth and clean-clone recovery
```

Record exact compatible/at-risk conclusions and no silent path changes.

- [ ] **Step 7: Complete the acceptance matrix**

Require `PASS` for G1, G2, G3, G4, 34/34 source assets, 27/27 textures, 84/84 Package readback, both map PIE runs, target performance, clean-clone rebuild, originality ledger, direct regressions, and FCR. Any unrun check is `NOT RUN`, not pass.

- [ ] **Step 8: Update tracking and prepare the completion report**

Use the ProjectR completion-report order:

1. Summary.
2. Changed files.
3. Created/modified UE Packages.
4. Public APIs/contracts.
5. MCP Operation Manifest results.
6. Commands and actual results.
7. Acceptance matrix.
8. Future Compatibility Review.
9. Human judgment/manual steps with exact runbook.
10. Known risks.
11. Suggested commit message.

- [ ] **Step 9: Commit only if authorized**

If the user authorized commits, stage only the exact approved Tier A files and Packages and commit with:

```powershell
git commit -m "art: deliver original Tier A environment slice"
```

Do not push. If commits were not authorized, report `NOT RUN — commit not authorized` and leave the worktree ready for user review.
