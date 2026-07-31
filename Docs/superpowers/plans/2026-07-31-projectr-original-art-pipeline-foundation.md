# ProjectR Original Art Pipeline Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish and prove a versioned, original-only ProjectR environment-art pipeline from asset brief through Blender/Substance source files to an auditable Unreal Engine 5.8 smoke asset.

**Architecture:** Plain-text briefs and provenance records are the control plane; Blender and Substance files are the authoritative art sources; FBX 2020.2-compatible meshes and packed textures are reproducible derivatives; Unreal Packages are imported only through a serialized MCP manifest. The foundation is accepted only after a 100 cm smoke asset survives source validation, Unreal import, fixed-camera inspection, and a clean-clone rebuild.

**Tech Stack:** Unreal Engine 5.8, Unreal MCP, ProjectRAuthoringTools editor plugin, Blender 4.5.5 LTS with Blender Python, Adobe Substance 3D Painter/Designer or Automation Toolkit, Python 3.13 standard library, PowerShell, Git LFS.

## Global Constraints

- Do not execute this plan while `CURRENT_VERSION.md` points to `v0.7.2` or to any task that does not explicitly allow every file and Package path listed below.
- Do not advance `CURRENT_VERSION.md`; the new art task contract must already be approved through the ProjectR version-handoff process.
- The implementation task must list `BuildScripts/ArtPipeline/`, `Docs/Art/`, `SourceArt/ProjectR/`, the exact documentation files below, `/Game/ProjectR/MCPTest/ArtPipeline/`, and any conditional `ProjectRAuthoringTools` files in Allowed Paths.
- New environment-art payloads contain no third-party final meshes, textures, materials, decals, alphas, HDRIs, animation, VFX, audio, or kits.
- AI output is reference-only; no AI bitmap is imported as a game texture and no generated mesh becomes an authoritative source.
- Blender is the geometry authority; Substance is the material authority; `SourceArt/ProjectR/Exports/` contains reproducible import derivatives.
- Unreal Engine 5.8 FBX compatibility is frozen to FBX 2020.2; export uses Z-up and validates dimensions after import.
- Never write `.uasset/.umap` through shell or ordinary file I/O. Use one Unreal Editor instance and serialized Unreal MCP writes.
- Before every Unreal write, list exact Package paths, check existence/editability, and record an MCP Operation Manifest. Stop on any collision.
- Never overwrite, rename, move, delete, migrate, fix redirectors, or resave all assets without separate explicit approval.
- Human work is limited to Adobe account/license sign-in and subjective G1/G2/G4 approvals after capability alternatives are audited.
- Do not commit unless the user explicitly authorizes commits for the execution session. Each task's final commit step is conditional; otherwise report `NOT RUN — commit not authorized`.
- Read `AGENTS.md`, `CURRENT_VERSION.md`, its task file, direct dependency-map rows, named data contracts, `CodexWorkflow.md`, `CodingStandard.md`, `AssetNaming.md`, `AutomationOwnership.md`, and `UnrealMCPAuthoring.md` before implementation.

---

## Planned File Structure

```text
BuildScripts/ArtPipeline/
  pr_art_contract.py             # brief/ledger data types and validation
  pr_art_cli.py                  # stable validation entry point
  pr_texture_check.py            # exported texture header/name checks
  Tests/
    test_pr_art_contract.py
    test_pr_texture_check.py
    Test-BlenderPipeline.ps1

Docs/Art/
  README.md                      # production rules and commands
  Briefs/
    asset-brief.schema.json
    PR_ART_SMOKE_BlockoutCube.json
  Manifests/
    MCP_ArtPipeline_Smoke.json
  Reviews/
    README.md
    ArtPipelineCapabilityAudit.md
    G2_ArtPipelineSmoke.md
  Runbooks/
    SubstanceSetupAndExport.md

SourceArt/ProjectR/
  Blender/Smoke/SM_PR_Smoke_Blockout.blend
  Concepts/Smoke/README.md
  Exports/Smoke/
    SM_PR_Smoke_Blockout.fbx
    T_PR_SmokeSurface_BC.png
    T_PR_SmokeSurface_N.png
    T_PR_SmokeSurface_ORM.png
  Generators/blender/
    pr_blender_common.py
    create_smoke_asset.py
    export_static_mesh_fbx.py
    validate_blend.py
  Provenance/AssetLedger.csv
  Substance/Painter/Smoke/PR_SmokeSurface.spp
  Substance/ExportPresets/ProjectR_UE58_ORM.spexp

/Game/ProjectR/MCPTest/ArtPipeline/
  Geometry/SM_PR_Smoke_Blockout
  Textures/T_PR_SmokeSurface_BC
  Textures/T_PR_SmokeSurface_N
  Textures/T_PR_SmokeSurface_ORM
  Materials/M_PR_SmokeSurface
  Materials/MI_PR_SmokeSurface
  Maps/L_ArtPipelineSmoke
```

Conditional fallback only when the official MCP capability audit fails:

```text
Plugins/ProjectRAuthoringTools/Source/ProjectRAuthoringTools/Public/PRArtPipelineAuthoringToolset.h
Plugins/ProjectRAuthoringTools/Source/ProjectRAuthoringTools/Private/PRArtPipelineAuthoringToolset.cpp
Plugins/ProjectRAuthoringTools/Source/ProjectRAuthoringTools/Private/Tests/PRArtPipelineAutomationTests.cpp
Plugins/ProjectRAuthoringTools/Source/ProjectRAuthoringTools/Private/ProjectRAuthoringToolsModule.cpp
Plugins/ProjectRAuthoringTools/Source/ProjectRAuthoringTools/ProjectRAuthoringTools.Build.cs
```

### Task 1: Freeze the Art Storage, Naming, and Version Contract

**Files:**
- Modify: `.gitattributes`
- Modify: `.gitignore`
- Modify: `Docs/ArchitectureDecisions.md`
- Modify: `Docs/Workflow/AssetNaming.md`
- Create: `Docs/Art/README.md`
- Modify at closeout only: `Docs/DevelopmentLog.md`
- Modify only if a real problem is found: `Docs/KnownIssues.md`

**Interfaces:**
- Consumes: the approved art-pipeline specification and the approved future art task contract.
- Produces: ADR-041, stable art prefixes, LFS/ignore rules, and the repository roots consumed by every later task.

- [ ] **Step 1: Enforce the execution gate**

Run from `E:/MyWork/ProjectR`:

```powershell
$currentPath = (Get-Content -Raw CURRENT_VERSION.md | Select-String -Pattern 'Docs/Versions/[^`\s]+\.md').Matches.Value
if (-not $currentPath) { throw 'CURRENT_VERSION does not resolve to a task file.' }
$task = Get-Content -Raw $currentPath
$required = @('BuildScripts/ArtPipeline','Docs/Art','SourceArt/ProjectR','/Game/ProjectR/MCPTest/ArtPipeline')
$missing = @($required | Where-Object { $task -notmatch [regex]::Escape($_) })
if ($missing.Count -gt 0) { throw "Art task contract is not active or lacks Allowed Paths: $($missing -join ', ')" }
```

Expected: exit 0 and zero missing paths. If it fails, stop the entire plan without creating files.

- [ ] **Step 2: Append ADR-041**

Append `# ADR-041 - ProjectR art sources are original, reconstructible, and LFS-versioned` to `Docs/ArchitectureDecisions.md`. The decision must freeze:

```text
Blender 4.5.5 LTS
Unreal Engine 5.8 FBX target: FBX 2020.2
Geometry authority: SourceArt/ProjectR/Blender/**/*.blend
Material authority: SourceArt/ProjectR/Substance/**/*.{spp,sbs}
Stable derivatives: SourceArt/ProjectR/Exports/**/*.{fbx,png,tga,exr}
Runtime derivatives: /Game/ProjectR/Art/** and /Game/ProjectR/MCPTest/ArtPipeline/**
Forbidden final inputs: third-party mesh/texture/material/decal/alpha/HDRI/animation/VFX/audio/kit
```

Also state that caches and automatic backups are ignored, generated concepts are reference-only, clean-clone reconstruction is mandatory, and Package deletion remains approval-gated.

- [ ] **Step 3: Extend the asset-prefix table**

Add exactly these rows to `Docs/Workflow/AssetNaming.md` without renaming existing assets:

```markdown
| Static Mesh | `SM_` | `SM_RealityHub_CartridgeSlot` |
| Texture | `T_` | `T_RealityHub_Trim_BC` |
| Material | `M_` | `M_PR_SurfaceOpaque` |
| Material Instance | `MI_` | `MI_RealityHub_CartridgeSlot` |
```

Document suffixes `_BC`, `_N`, `_ORM`, `_E`, and collision prefixes `UBX_`, `UCP_`, `USP_`, `UCX_`. Names remain ASCII and Package paths remain public contracts.

- [ ] **Step 4: Add scoped LFS rules**

Append these exact patterns to `.gitattributes`:

```gitattributes
/SourceArt/**/*.blend filter=lfs diff=lfs merge=lfs -text
/SourceArt/**/*.spp filter=lfs diff=lfs merge=lfs -text
/SourceArt/**/*.sbs filter=lfs diff=lfs merge=lfs -text
/SourceArt/**/*.sbsar filter=lfs diff=lfs merge=lfs -text
/SourceArt/**/*.spexp filter=lfs diff=lfs merge=lfs -text
/SourceArt/**/*.fbx filter=lfs diff=lfs merge=lfs -text
/SourceArt/**/*.png filter=lfs diff=lfs merge=lfs -text
/SourceArt/**/*.tga filter=lfs diff=lfs merge=lfs -text
/SourceArt/**/*.exr filter=lfs diff=lfs merge=lfs -text
```

Do not add an ordinary-Git exception for new art Packages.

- [ ] **Step 5: Add exact ignore rules**

Append these entries to `.gitignore`:

```gitignore
/SourceArt/ProjectR/Cache/
/SourceArt/ProjectR/Temp/
/SourceArt/**/*.blend1
/SourceArt/**/*.blend2
/SourceArt/**/*_autosave.spp
/SourceArt/**/*.sbsprs
```

- [ ] **Step 6: Validate Git and LFS routing**

Run:

```powershell
git lfs env
git check-attr filter diff merge text -- SourceArt/ProjectR/Blender/Smoke/SM_PR_Smoke_Blockout.blend
git check-attr filter diff merge text -- SourceArt/ProjectR/Provenance/AssetLedger.csv
git diff --check
```

Expected: the `.blend` path reports `filter=lfs`, `diff=lfs`, `merge=lfs`, `text=unset`; the CSV does not route to LFS; `git diff --check` exits 0.

- [ ] **Step 7: Record the GitHub LFS capacity gate**

In `Docs/Art/README.md`, require a non-zero GitHub LFS budget and an initial 100 GiB working allowance before content production. Record current local `git lfs env` output and state that exceeding the remote budget blocks pushes but never justifies removing source files.

- [ ] **Step 8: Commit only if authorized**

If the user explicitly authorized commits:

```powershell
git add .gitattributes .gitignore Docs/ArchitectureDecisions.md Docs/Workflow/AssetNaming.md Docs/Art/README.md
git commit -m "docs: freeze original art storage contracts"
```

Otherwise report: `NOT RUN — commit not authorized`.

### Task 2: Implement Asset Brief and Provenance Validation

**Files:**
- Create: `BuildScripts/ArtPipeline/pr_art_contract.py`
- Create: `BuildScripts/ArtPipeline/pr_art_cli.py`
- Create: `BuildScripts/ArtPipeline/Tests/test_pr_art_contract.py`
- Create: `Docs/Art/Briefs/asset-brief.schema.json`
- Create: `Docs/Art/Briefs/PR_ART_SMOKE_BlockoutCube.json`
- Create: `Docs/Art/Reviews/README.md`
- Create: `SourceArt/ProjectR/Provenance/AssetLedger.csv`

**Interfaces:**
- Consumes: ADR-041 roots and naming rules.
- Produces: `AssetBrief`, `LedgerRow`, `load_brief(Path)`, `validate_brief(AssetBrief)`, `validate_ledger(Path)`, and CLI exit codes consumed by all later tasks.

- [ ] **Step 1: Write failing unit tests**

Create `BuildScripts/ArtPipeline/Tests/test_pr_art_contract.py` with these cases:

```python
import csv
import json
import tempfile
import unittest
from pathlib import Path

from BuildScripts.ArtPipeline.pr_art_contract import load_brief, validate_brief, validate_ledger

VALID = {
    "schema_version": 1,
    "asset_id": "PR_ART_SMOKE_BlockoutCube",
    "ue_package": "/Game/ProjectR/MCPTest/ArtPipeline/Geometry/SM_PR_Smoke_Blockout",
    "room": "Smoke",
    "priority": "P0",
    "dimensions_cm": {"x": 100, "y": 100, "z": 100},
    "camera_role": "pipeline scale reference",
    "source_blend": "SourceArt/ProjectR/Blender/Smoke/SM_PR_Smoke_Blockout.blend",
    "export_fbx": "SourceArt/ProjectR/Exports/Smoke/SM_PR_Smoke_Blockout.fbx",
    "collision": "UCX",
    "material_profile": "PR_SmokeSurface",
    "third_party_final_assets": False,
    "required_gates": ["G2", "G3"]
}

class ContractTests(unittest.TestCase):
    def test_valid_brief(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root) / "brief.json"
            path.write_text(json.dumps(VALID), encoding="utf-8")
            self.assertEqual(validate_brief(load_brief(path)), [])

    def test_rejects_external_package_and_third_party_payload(self):
        bad = dict(VALID, ue_package="/Game/Other/SM_Bad", third_party_final_assets=True)
        with tempfile.TemporaryDirectory() as root:
            path = Path(root) / "brief.json"
            path.write_text(json.dumps(bad), encoding="utf-8")
            errors = validate_brief(load_brief(path))
            self.assertIn("ue_package must be under /Game/ProjectR/", errors)
            self.assertIn("third_party_final_assets must be false", errors)

    def test_ledger_requires_unique_asset_id_and_package(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root) / "AssetLedger.csv"
            fields = ["asset_id", "ue_package", "source_path", "export_path", "source_sha256", "export_sha256", "third_party_final_assets", "g1", "g2", "g3", "g4"]
            with path.open("w", newline="", encoding="utf-8") as stream:
                writer = csv.DictWriter(stream, fieldnames=fields)
                writer.writeheader()
                row = {key: "" for key in fields}
                row.update(asset_id="A", ue_package="/Game/ProjectR/A", third_party_final_assets="false")
                writer.writerow(row); writer.writerow(row)
            self.assertIn("duplicate asset_id: A", validate_ledger(path))

if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run the tests to verify RED**

Run:

```powershell
python -m unittest BuildScripts.ArtPipeline.Tests.test_pr_art_contract -v
```

Expected: FAIL because `BuildScripts.ArtPipeline.pr_art_contract` does not exist.

- [ ] **Step 3: Implement the contract module**

Implement `pr_art_contract.py` with immutable dataclasses and these exact validation rules:

```python
from __future__ import annotations
import csv, hashlib, json
from dataclasses import dataclass
from pathlib import Path

@dataclass(frozen=True)
class AssetBrief:
    schema_version: int
    asset_id: str
    ue_package: str
    room: str
    priority: str
    dimensions_cm: dict[str, float]
    camera_role: str
    source_blend: str
    export_fbx: str
    collision: str
    material_profile: str
    third_party_final_assets: bool
    required_gates: tuple[str, ...]

def load_brief(path: Path) -> AssetBrief:
    data = json.loads(path.read_text(encoding="utf-8"))
    data["required_gates"] = tuple(data["required_gates"])
    return AssetBrief(**data)

def validate_brief(brief: AssetBrief) -> list[str]:
    errors: list[str] = []
    if brief.schema_version != 1: errors.append("schema_version must equal 1")
    if not brief.asset_id.startswith("PR_ART_"): errors.append("asset_id must start with PR_ART_")
    if not brief.ue_package.startswith("/Game/ProjectR/"): errors.append("ue_package must be under /Game/ProjectR/")
    if brief.priority not in {"P0", "P1", "P2"}: errors.append("priority must be P0, P1, or P2")
    if set(brief.dimensions_cm) != {"x", "y", "z"} or any(v <= 0 for v in brief.dimensions_cm.values()):
        errors.append("dimensions_cm must contain positive x, y, z")
    if not brief.source_blend.startswith("SourceArt/ProjectR/Blender/") or not brief.source_blend.endswith(".blend"):
        errors.append("source_blend must be a ProjectR .blend path")
    if not brief.export_fbx.startswith("SourceArt/ProjectR/Exports/") or not brief.export_fbx.endswith(".fbx"):
        errors.append("export_fbx must be a ProjectR .fbx path")
    if brief.collision not in {"NONE", "UBX", "UCP", "USP", "UCX"}: errors.append("collision has an unsupported value")
    if brief.third_party_final_assets: errors.append("third_party_final_assets must be false")
    if not set(brief.required_gates).issubset({"G1", "G2", "G3", "G4"}): errors.append("required_gates contains an unknown gate")
    return errors

def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""): digest.update(chunk)
    return digest.hexdigest()

def validate_ledger(path: Path) -> list[str]:
    errors: list[str] = []
    seen_ids: set[str] = set(); seen_packages: set[str] = set()
    with path.open(newline="", encoding="utf-8-sig") as stream:
        for row in csv.DictReader(stream):
            asset_id, package = row["asset_id"], row["ue_package"]
            if asset_id in seen_ids: errors.append(f"duplicate asset_id: {asset_id}")
            if package in seen_packages: errors.append(f"duplicate ue_package: {package}")
            if row["third_party_final_assets"].lower() != "false": errors.append(f"third-party final payload: {asset_id}")
            seen_ids.add(asset_id); seen_packages.add(package)
    return errors
```

- [ ] **Step 4: Implement the CLI**

`pr_art_cli.py` must expose:

```powershell
python BuildScripts/ArtPipeline/pr_art_cli.py validate-brief Docs/Art/Briefs/PR_ART_SMOKE_BlockoutCube.json
python BuildScripts/ArtPipeline/pr_art_cli.py validate-ledger SourceArt/ProjectR/Provenance/AssetLedger.csv
python BuildScripts/ArtPipeline/pr_art_cli.py hash SourceArt/ProjectR/Exports/Smoke/SM_PR_Smoke_Blockout.fbx
```

Exit `0` with `PASS` on success, exit `2` and print one error per line on contract failures, and never mutate input files.

- [ ] **Step 5: Add the JSON schema, smoke brief, and ledger header**

The JSON schema must set `additionalProperties: false`, require every field in `VALID`, constrain the Package root to `^/Game/ProjectR/`, and constrain priorities/gates/collision to the enumerations above. Initialize `AssetLedger.csv` with exactly:

```csv
asset_id,ue_package,source_path,export_path,source_sha256,export_sha256,third_party_final_assets,g1,g2,g3,g4
PR_ART_SMOKE_BlockoutCube,/Game/ProjectR/MCPTest/ArtPipeline/Geometry/SM_PR_Smoke_Blockout,SourceArt/ProjectR/Blender/Smoke/SM_PR_Smoke_Blockout.blend,SourceArt/ProjectR/Exports/Smoke/SM_PR_Smoke_Blockout.fbx,,,false,NOT_REQUIRED,PENDING,PENDING,NOT_REQUIRED
```

- [ ] **Step 6: Run GREEN and contract checks**

```powershell
python -m unittest BuildScripts.ArtPipeline.Tests.test_pr_art_contract -v
python BuildScripts/ArtPipeline/pr_art_cli.py validate-brief Docs/Art/Briefs/PR_ART_SMOKE_BlockoutCube.json
python BuildScripts/ArtPipeline/pr_art_cli.py validate-ledger SourceArt/ProjectR/Provenance/AssetLedger.csv
```

Expected: all tests pass; both CLI calls print `PASS` and exit 0.

- [ ] **Step 7: Commit only if authorized**

```powershell
git add BuildScripts/ArtPipeline Docs/Art SourceArt/ProjectR/Provenance
git commit -m "feat: validate original art contracts"
```

Otherwise report the commit step as not run.

### Task 3: Build and Test the Blender Static-Mesh Pipeline

**Files:**
- Create: `SourceArt/ProjectR/Generators/blender/pr_blender_common.py`
- Create: `SourceArt/ProjectR/Generators/blender/create_smoke_asset.py`
- Create: `SourceArt/ProjectR/Generators/blender/export_static_mesh_fbx.py`
- Create: `SourceArt/ProjectR/Generators/blender/validate_blend.py`
- Create: `BuildScripts/ArtPipeline/Tests/Test-BlenderPipeline.ps1`
- Create through Blender: `SourceArt/ProjectR/Blender/Smoke/SM_PR_Smoke_Blockout.blend`
- Create through Blender: `SourceArt/ProjectR/Exports/Smoke/SM_PR_Smoke_Blockout.fbx`

**Interfaces:**
- Consumes: one validated asset brief.
- Produces: `configure_projectr_scene()`, `validate_scene()`, a 100 cm source cube with `UCX_` collision, and an FBX derivative suitable for the UE 5.8 smoke import.

- [ ] **Step 1: Prove Blender 4.5.5 is absent or correctly installed**

Run:

```powershell
$blender = Get-Command blender -ErrorAction SilentlyContinue
if ($blender) { & $blender.Source --version } else { Write-Output 'RED: Blender not installed' }
```

Expected before provisioning on the current workstation: `RED: Blender not installed`.

- [ ] **Step 2: Install the frozen Blender version**

Run only after the user has authorized external tool installation for plan execution:

```powershell
winget install --id BlenderFoundation.Blender --version 4.5.5 --exact --accept-package-agreements --accept-source-agreements
```

Open a fresh shell and run `blender --version`. Expected first line: `Blender 4.5.5 LTS`.

- [ ] **Step 3: Write the failing PowerShell smoke test**

`Test-BlenderPipeline.ps1` must invoke factory-startup Blender, require exit code 0, require both output files, and reject any source-scene validation message. First run it before the generator scripts exist:

```powershell
WindowsPowerShell -NoProfile -ExecutionPolicy Bypass -File BuildScripts/ArtPipeline/Tests/Test-BlenderPipeline.ps1
```

Expected: FAIL because `create_smoke_asset.py` is missing.

- [ ] **Step 4: Implement common scene rules**

`pr_blender_common.py` must contain:

```python
import bpy

EXPECTED_UNIT_SCALE = 0.01
RENDER_COLLECTION = "PR_RENDER"
COLLISION_COLLECTION = "PR_COLLISION"

def configure_projectr_scene() -> None:
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.length_unit = "CENTIMETERS"
    scene.unit_settings.scale_length = EXPECTED_UNIT_SCALE

def validate_scene() -> list[str]:
    errors: list[str] = []
    scene = bpy.context.scene
    if scene.unit_settings.system != "METRIC" or scene.unit_settings.scale_length != EXPECTED_UNIT_SCALE:
        errors.append("scene units must be metric centimeters at scale 0.01")
    names = {obj.name for obj in scene.objects}
    for required in {"SM_PR_Smoke_Blockout", "UCX_SM_PR_Smoke_Blockout_00"}:
        if required not in names: errors.append(f"missing object: {required}")
    render = scene.objects.get("SM_PR_Smoke_Blockout")
    if render and any(abs(actual - 100.0) > 0.01 for actual in render.dimensions):
        errors.append(f"render dimensions must be 100 cm: {tuple(render.dimensions)}")
    if render and any(abs(value) > 0.0001 for value in render.location):
        errors.append("render origin must be at world origin")
    return errors
```

- [ ] **Step 5: Implement the deterministic smoke generator**

`create_smoke_asset.py` must clear the factory scene, create a 100 × 100 × 100 cm cube named `SM_PR_Smoke_Blockout`, create a slightly inset convex collision cube named `UCX_SM_PR_Smoke_Blockout_00`, add a Triangulate modifier to the render mesh, create one UV map, and save the `.blend` path supplied after `--output`. It must exit non-zero if `validate_scene()` returns errors.

- [ ] **Step 6: Implement the FBX exporter and validator**

`export_static_mesh_fbx.py` must open `--input`, select only the render/collision objects, and call:

```python
bpy.ops.export_scene.fbx(
    filepath=output_path,
    use_selection=True,
    object_types={"MESH"},
    apply_unit_scale=True,
    apply_scale_options="FBX_SCALE_UNITS",
    axis_forward="-Y",
    axis_up="Z",
    use_mesh_modifiers=True,
    add_leaf_bones=False,
    bake_anim=False,
)
```

`validate_blend.py` must open `--input`, print each validation error, and exit 2 on failure.

- [ ] **Step 7: Run the Blender test to GREEN**

```powershell
WindowsPowerShell -NoProfile -ExecutionPolicy Bypass -File BuildScripts/ArtPipeline/Tests/Test-BlenderPipeline.ps1
python BuildScripts/ArtPipeline/pr_art_cli.py hash SourceArt/ProjectR/Blender/Smoke/SM_PR_Smoke_Blockout.blend
python BuildScripts/ArtPipeline/pr_art_cli.py hash SourceArt/ProjectR/Exports/Smoke/SM_PR_Smoke_Blockout.fbx
```

Expected: PowerShell exits 0; both files exist and have non-empty SHA-256 values. Write those hashes into the smoke ledger row.

- [ ] **Step 8: Commit only if authorized**

```powershell
git add BuildScripts/ArtPipeline/Tests SourceArt/ProjectR/Generators/blender SourceArt/ProjectR/Blender/Smoke SourceArt/ProjectR/Exports/Smoke SourceArt/ProjectR/Provenance/AssetLedger.csv
git commit -m "feat: add reproducible Blender art export"
```

Otherwise report the commit step as not run.

### Task 4: Freeze and Validate the Substance Texture Export

**Files:**
- Create: `BuildScripts/ArtPipeline/pr_texture_check.py`
- Create: `BuildScripts/ArtPipeline/Tests/test_pr_texture_check.py`
- Create: `Docs/Art/Runbooks/SubstanceSetupAndExport.md`
- Create through Substance: `SourceArt/ProjectR/Substance/Painter/Smoke/PR_SmokeSurface.spp`
- Create through Substance: `SourceArt/ProjectR/Substance/ExportPresets/ProjectR_UE58_ORM.spexp`
- Create through Substance: three smoke PNGs under `SourceArt/ProjectR/Exports/Smoke/`

**Interfaces:**
- Consumes: the smoke FBX and an authenticated local Substance installation.
- Produces: one authored `.spp`, one reusable export preset, and BaseColor/Normal/ORM derivatives checked before Unreal import.

- [ ] **Step 1: Audit installed Adobe capabilities**

Run read-only checks for Painter, Designer, `sbsrender`, `sbscooker`, and Painter plugin/Python support. Record exact executable paths and versions in `Docs/Art/Reviews/ArtPipelineCapabilityAudit.md`. Do not ask the user to author a texture until this audit is recorded.

- [ ] **Step 2: Complete the only credential-bound human step**

If Adobe tools are missing, the user installs the current licensed Substance 3D Collection through Adobe Creative Cloud and signs in. Codex then re-runs the executable/version audit. Expected: Painter launches and can open the smoke FBX; Designer or SAT exposes `sbsrender` for later procedural materials.

- [ ] **Step 3: Write RED tests for the texture checker**

The tests must require exactly these files, 2048 × 2048 PNG dimensions, and no extra suffixes:

```text
T_PR_SmokeSurface_BC.png
T_PR_SmokeSurface_N.png
T_PR_SmokeSurface_ORM.png
```

Run `python -m unittest BuildScripts.ArtPipeline.Tests.test_pr_texture_check -v`; expect FAIL because the checker does not exist.

- [ ] **Step 4: Implement the PNG header checker**

`pr_texture_check.py` must read the PNG signature and IHDR width/height with `struct.unpack(">II", header[16:24])`, reject non-PNG files, reject dimensions other than 2048 × 2048, and return errors sorted by filename. It must not modify images.

- [ ] **Step 5: Create the ProjectR export preset**

In Painter's Export Textures dialog create and export `ProjectR_UE58_ORM` with:

```text
$textureSet_BC: RGB = Base Color, A = 1.0, 8-bit PNG, sRGB
$textureSet_N: RGB = Normal DirectX, A = 1.0, 8-bit PNG, linear
$textureSet_ORM: R = Ambient Occlusion, G = Roughness, B = Metallic, A = 1.0, 8-bit PNG, linear
Resolution: 2048
Padding: Dilation Infinite
```

No shelf download, stock smart material, external alpha, or scan may be used.

- [ ] **Step 6: Create and export the smoke material**

Create `PR_SmokeSurface.spp` from the smoke FBX. Use only authored fill values: BaseColor `(0.18, 0.21, 0.23)`, Roughness `0.55`, Metallic `0.65`, Normal flat, AO from the mesh bake. Save the `.spp`, export with `ProjectR_UE58_ORM`, and save the `.spexp` under the planned path.

- [ ] **Step 7: Run GREEN checks and update provenance**

```powershell
python -m unittest BuildScripts.ArtPipeline.Tests.test_pr_texture_check -v
python BuildScripts/ArtPipeline/pr_texture_check.py SourceArt/ProjectR/Exports/Smoke PR_SmokeSurface
```

Expected: all tests pass; texture validation prints `PASS`. Add the `.spp` and three texture hashes to the smoke review record.

- [ ] **Step 8: Commit only if authorized**

```powershell
git add BuildScripts/ArtPipeline Docs/Art/Runbooks Docs/Art/Reviews SourceArt/ProjectR/Substance SourceArt/ProjectR/Exports/Smoke
git commit -m "feat: freeze ProjectR Substance export"
```

Otherwise report the commit step as not run.

### Task 5: Audit Unreal MCP and Prepare the Smoke Manifest

**Files:**
- Create: `Docs/Art/Reviews/ArtPipelineCapabilityAudit.md`
- Create: `Docs/Art/Manifests/MCP_ArtPipeline_Smoke.json`
- Conditional create/modify: the five `PRArtPipelineAuthoringToolset` files listed in Planned File Structure.

**Interfaces:**
- Consumes: validated FBX/PNG derivatives and ProjectR Unreal authoring rules.
- Produces: an exact seven-Package write manifest and either an official-tool route or a fixed no-argument ProjectR Toolset route.

- [ ] **Step 1: Start one clean ProjectR Editor and audit existing Toolsets**

Use existing official Toolsets first. Record whether they can, with exact source and destination paths:

1. Import one static mesh without importing automatic materials/textures.
2. Preserve custom `UCX_` collision.
3. Import BaseColor/Normal/ORM with explicit sRGB/compression settings.
4. Create or configure one Material Instance.
5. Create a Level, place one mesh actor, save exact Packages, query references, inspect Dirty state, capture a viewport, and run PIE.

Every item is `PASS` or `FAIL` with tool name, arguments, result, and limitation. “Tool exists” is not a pass.

- [ ] **Step 2: Write the exact manifest before any write**

`MCP_ArtPipeline_Smoke.json` must contain only:

```json
{
  "schema_version": 1,
  "operation_id": "ProjectR-ArtPipeline-Smoke-v1",
  "packages": [
    "/Game/ProjectR/MCPTest/ArtPipeline/Geometry/SM_PR_Smoke_Blockout",
    "/Game/ProjectR/MCPTest/ArtPipeline/Textures/T_PR_SmokeSurface_BC",
    "/Game/ProjectR/MCPTest/ArtPipeline/Textures/T_PR_SmokeSurface_N",
    "/Game/ProjectR/MCPTest/ArtPipeline/Textures/T_PR_SmokeSurface_ORM",
    "/Game/ProjectR/MCPTest/ArtPipeline/Materials/M_PR_SmokeSurface",
    "/Game/ProjectR/MCPTest/ArtPipeline/Materials/MI_PR_SmokeSurface",
    "/Game/ProjectR/MCPTest/ArtPipeline/Maps/L_ArtPipelineSmoke"
  ],
  "overwrite": false,
  "delete": false,
  "move": false,
  "rename": false
}
```

- [ ] **Step 3: Check all seven destinations**

Query each Package individually. Expected: all are absent and their parent roots are editable. Any collision stops the task; do not generate a new suffix.

- [ ] **Step 4: Decide the automation route**

Use the official route only if every capability in Step 1 is `PASS`. If any required capability is `FAIL`, implement the conditional fixed Toolset; it accepts no path, Package, material, numerical, or delete input and exposes exactly:

```cpp
UFUNCTION(meta=(AICallable))
static UToolCallAsyncResultString* CreateArtPipelineSmokeManifest();

UFUNCTION(meta=(AICallable))
static UToolCallAsyncResultString* ValidateArtPipelineSmokeManifest();
```

The Toolset hard-codes the seven manifest Packages, the four source files, a 100 cm bounds assertion, ORM/sRGB settings, and `overwrite=false`. Register/unregister it in `ProjectRAuthoringToolsModule.cpp` and add only the minimum required editor modules in `ProjectRAuthoringTools.Build.cs`.

- [ ] **Step 5: Add RED automation for the conditional Toolset**

Only on the fallback route, create `PRArtPipelineAutomationTests.cpp` with tests that assert:

```text
ProjectR.ArtPipeline.Authoring.FixedManifest
ProjectR.ArtPipeline.Authoring.RejectsExistingPackage
ProjectR.ArtPipeline.Authoring.SourceFilesRequired
```

The tests must fail before implementation, then pass without accepting caller-controlled paths.

- [ ] **Step 6: Build and test the conditional Toolset**

Only on the fallback route:

```powershell
BuildScripts\BuildEditor.bat -Project E:\MyWork\ProjectR\ProjectR.uproject -RunId art-pipeline-toolset
```

Then run the three `ProjectR.ArtPipeline.Authoring` tests through the established Unreal automation runner. Expected: build exit 0 and 3/3 pass.

- [ ] **Step 7: Commit only if authorized**

Commit the capability audit and manifest. Include conditional plugin files only if the audit required them:

```powershell
git add Docs/Art/Reviews/ArtPipelineCapabilityAudit.md Docs/Art/Manifests/MCP_ArtPipeline_Smoke.json Plugins/ProjectRAuthoringTools/Source/ProjectRAuthoringTools
git commit -m "feat: add fixed Unreal art import manifest"
```

Otherwise report the commit step as not run.

### Task 6: Import and Verify the Unreal Smoke Asset

**Files:**
- Create via Unreal MCP: the seven exact Packages in `MCP_ArtPipeline_Smoke.json`
- Update: `Docs/Art/Reviews/G2_ArtPipelineSmoke.md`
- Update: `SourceArt/ProjectR/Provenance/AssetLedger.csv`

**Interfaces:**
- Consumes: the validated smoke manifest and the selected official/fixed Toolset route.
- Produces: a restart-stable Unreal smoke map proving mesh, collision, textures, material, scale, save, and provenance.

- [ ] **Step 1: Re-run pre-write validation**

Run brief, ledger, Blender, texture, source-hash, and seven-Package collision checks immediately before writing. Expected: all pass and all Packages remain absent.

- [ ] **Step 2: Import the mesh with exact options**

Import only `SM_PR_Smoke_Blockout.fbx` to its manifest destination with:

```text
Skeletal Mesh = false
Import Mesh = true
Import Materials = false
Import Textures = false
Combine Meshes = false
Auto Generate Collision = false
Import Mesh LODs = false
Remove Degenerates = true
Vertex Color Import = Replace
```

Expected: one static mesh, one custom convex collision, no surprise materials or textures.

- [ ] **Step 3: Import and configure the textures**

Import the three PNGs to exact destinations. Set BaseColor `sRGB=true`; set Normal and ORM `sRGB=false`; use normal-map compression for `_N` and mask compression for `_ORM` when the official tool exposes it. Read settings back; failure to read back is a G3 failure.

- [ ] **Step 4: Create the smoke master material and Material Instance**

Create `M_PR_SmokeSurface` with exactly three texture parameters named `BaseColorTexture`, `NormalTexture`, and `ORMTexture`. Connect BaseColor RGB to Base Color, Normal RGB to Normal, ORM R/G/B to Ambient Occlusion/Roughness/Metallic, and use the default opaque blend mode. Create `MI_PR_SmokeSurface`, bind the three exact imported textures, and save both material Packages.

- [ ] **Step 5: Create and stage the smoke map**

Create `L_ArtPipelineSmoke`, place the mesh at `(0,0,50)` so its base is at Z=0, apply `MI_PR_SmokeSurface`, add a 180 cm reference capsule or existing read-only player reference, and use a side-on camera. Do not add gameplay logic.

- [ ] **Step 6: Validate geometry and rendering**

Read back:

```text
Static mesh full bounds: 100 ± 0.1 cm on X/Y/Z
Actor scale: 1,1,1
Actor rotation: 0,0,0
Simple collision primitives: exactly 1
Material slots: exactly 1
Missing textures/materials: 0
```

Capture wireframe/collision and lit screenshots to the review record. Mark G2 and G3 `PASS` only with this evidence.

- [ ] **Step 7: Save exact Packages and verify Editor state**

Save only the seven manifest Packages. Query all browsable `/Game/ProjectR` assets for Dirty state; verify no unexpected Dirty Packages. Close the Editor normally without a Save Content prompt, restart, reload all seven Packages, and repeat bounds/material/collision checks.

- [ ] **Step 8: Run the smoke PIE**

Run PIE in `L_ArtPipelineSmoke` long enough to confirm map load, no missing references, no import warnings promoted to runtime errors, and stable side-on rendering. Record `PIE PASS` separately from `asset created` and `restart loaded`.

- [ ] **Step 9: Update provenance**

Fill `source_sha256`, `export_sha256`, `g2=PASS`, `g3=PASS`; leave `g1=NOT_REQUIRED`, `g4=NOT_REQUIRED`. Add screenshot/log evidence paths in `G2_ArtPipelineSmoke.md`.

- [ ] **Step 10: Commit only if authorized**

```powershell
git add Content/ProjectR/MCPTest/ArtPipeline Docs/Art/Reviews/G2_ArtPipelineSmoke.md SourceArt/ProjectR/Provenance/AssetLedger.csv
git commit -m "test: prove Unreal art pipeline smoke asset"
```

Otherwise report the commit step as not run. Do not add ordinary-Git `.gitattributes` exceptions for these Packages.

### Task 7: Prove Clean-Clone Reconstruction and Close the Foundation

**Files:**
- Create under ignored evidence root: `Saved/AutomationReports/art-pipeline-foundation-<timestamp>/`
- Modify: `Docs/DevelopmentLog.md`
- Modify only if needed: `Docs/KnownIssues.md`
- Modify if the accepted decision changed: `Docs/ArchitectureDecisions.md`

**Interfaces:**
- Consumes: every foundation artifact and the seven restart-stable smoke Packages.
- Produces: the foundation acceptance report required by the Tier A content-production plan.

- [ ] **Step 1: Run all source-side checks from the primary workspace**

```powershell
python -m unittest discover BuildScripts/ArtPipeline/Tests -p "test_*.py" -v
WindowsPowerShell -NoProfile -ExecutionPolicy Bypass -File BuildScripts/ArtPipeline/Tests/Test-BlenderPipeline.ps1
python BuildScripts/ArtPipeline/pr_art_cli.py validate-brief Docs/Art/Briefs/PR_ART_SMOKE_BlockoutCube.json
python BuildScripts/ArtPipeline/pr_art_cli.py validate-ledger SourceArt/ProjectR/Provenance/AssetLedger.csv
git lfs fsck
git diff --check
```

Expected: all exit 0.

- [ ] **Step 2: Create a safe local rebuild clone**

Resolve `E:/MyWork/ProjectR/tmp/ArtPipelineRebuild` and verify it is inside `E:/MyWork/ProjectR/tmp` before creation. Clone with LFS enabled into that exact ignored directory; never delete or replace another directory.

- [ ] **Step 3: Rebuild the smoke asset in the clone**

Run the brief/ledger tests and Blender smoke test from the clone. Compare source hashes exactly. Compare FBX semantic metrics—object names, 100 cm dimensions, collision count, UV count, and material-slot count—rather than requiring binary FBX hashes to match if exporter metadata differs.

- [ ] **Step 4: Run Unreal regression gates**

Run the future task's exact BuildEditor, art automation, Data Validation, smoke map restart, smoke PIE, and historical RealityHub/Allocator regression commands. Expected: all required checks pass; unrelated established diagnostics remain unchanged and documented.

- [ ] **Step 5: Complete the acceptance matrix**

The final report must separately record:

```text
Version/Allowed Paths gate
ADR/naming/LFS gate
Brief and ledger tests
Blender source/export test
Substance export check
MCP capability audit
Seven-Package collision check
Asset created
Exact Packages saved
Restart loaded
Bounds/collision/material readback
PIE
Clean-clone rebuild
Git LFS fsck
Future Compatibility Review
```

No unrun item may be reported as passed.

- [ ] **Step 6: Update ProjectR tracking**

Add a DevelopmentLog entry with actual commands/results, Package list, manifest results, human/manual steps, known risks, FCR, and suggested commit message. Add KnownIssues only for a real unresolved defect; do not create speculative issues.

- [ ] **Step 7: Commit only if authorized**

```powershell
git add Docs/DevelopmentLog.md Docs/KnownIssues.md Docs/ArchitectureDecisions.md Saved/AutomationReports
git commit -m "docs: close original art pipeline foundation"
```

Do not add ignored `Saved/AutomationReports` unless the approved task contract explicitly requires tracked evidence. Otherwise stage only authorized documents and report the commit step as not run or adjusted.
