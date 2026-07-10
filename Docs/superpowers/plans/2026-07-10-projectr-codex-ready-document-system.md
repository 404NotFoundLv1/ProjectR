# ProjectR Codex-Ready Document System Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a Codex-ready ProjectR document system in Markdown and PDF, including complete game logic, cross-version dependency contracts, Unreal MCP automation rules, current-version task files, and repository execution copies.

**Architecture:** Treat the original four files as immutable source archives. Generate editable Markdown as the single source of truth under `E:/MyWork/ProjectR_Design/CodexReady_v6`, synchronize operational Markdown into `E:/MyWork/ProjectR`, and render every archival Markdown file to PDF with a reusable ReportLab renderer. Validate content contracts first, then PDF structure, then every rendered page visually.

**Tech Stack:** PowerShell, Python 3, ReportLab, pypdf, pdfplumber, Poppler (`pdfinfo`, `pdftoppm`), Markdown text processing.

## Global Constraints

- Do not modify the four original files in `E:/MyWork/ProjectR_Design`.
- Do not modify `Source/`, `Content/`, `Config/`, or `ProjectR.uproject`.
- Do not invoke Unreal MCP write tools.
- Use UTF-8 for all Markdown.
- Every archival Markdown deliverable must have a same-name PDF.
- Runtime gameplay remains deterministic and locally executable; LLM output is structured, validated, clamped, timeout-protected, and has an offline fallback.
- Unreal Editor mechanical work defaults to Codex through MCP; human steps are exceptions with exact runbooks.
- The version dependency map covers every public version from v0.0.0 through v1.0.4.
- The current task is v0.0.0 because Git and the formal baseline are not established.
- The workspace is not a Git repository; do not claim commits were made.

---

### Task 1: Freeze Inputs and Prepare the Delivery Manifest

**Files:**
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/README.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/build_manifest.json`
- Create: `E:/MyWork/ProjectR/tmp/pdfs/codexready/`

**Interfaces:**
- Consumes: the four immutable source files named in the approved specification.
- Produces: source hashes and the canonical output file list used by all later validation steps.

- [ ] **Step 1: Record source hashes and metadata**

Run:

```powershell
Get-FileHash E:/MyWork/ProjectR_Design/*.md,E:/MyWork/ProjectR_Design/*.pdf -Algorithm SHA256
```

Expected: four hashes matching the audit baseline; no source file is written.

- [ ] **Step 2: Create the delivery README and manifest**

The README must explain the document hierarchy, Markdown-as-source rule, repository-copy rule, current version, and how to advance versions. The manifest must list each Markdown/PDF pair and each operational repository copy.

- [ ] **Step 3: Validate manifest completeness**

Run a script that rejects duplicate output paths, missing Markdown/PDF pairs, or outputs outside the approved roots.

Expected: zero duplicate paths and zero out-of-scope paths.

### Task 2: Generate Product, Architecture, and Roadmap Markdown

**Files:**
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/01_Product/ProjectR_Game_Design_CodexReady_v1.0.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/01_Product/ProjectR_System_Architecture_and_Version_Dependency_Map.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/ProjectR_Codex_Executable_Roadmap_v6.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/ProjectR_Version_Index.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/ProjectR_Roadmap_Errata_and_Decisions.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/ProjectR_Data_and_Interface_Contracts.md`

**Interfaces:**
- Consumes: original design proposal, original roadmap v5, approved specification v1.1, and verified UE/MCP capability audit.
- Produces: the human-readable product north star, complete system dependency graph, full public-version dependency matrix, interface contracts, and corrected roadmap.

- [ ] **Step 1: Generate the revised design document**

Preserve all core creative content while adding product invariants, scope layers, architecture boundaries, product gates, automation implications, and consistent Demo timing.

- [ ] **Step 2: Generate the architecture and dependency map**

Include module ownership, runtime data flow, one-way dependency rules, and one row for every public version from v0.0.0 through v1.0.4 with `Depends on`, `Produces`, `Consumed by`, `Extension points`, and `Must remain compatible`.

- [ ] **Step 3: Generate the data and interface contracts**

Define stable ownership and compatibility rules for CombatEvent, Ability/AbilitySet, SaveGame, Companion/Relationship, QTE result, Director request/response/rule, Room/Reward, and RunSummary/AccountRecord.

- [ ] **Step 4: Generate roadmap v6 and its index/errata**

Correct UPR/USP, introduce v0.0.5, split oversized versions into internal microtasks, defer future business behavior, remove repeated prompt blocks, and add version advancement gates.

- [ ] **Step 5: Run cross-document contract checks**

Expected: every public version appears exactly once in Version Index and dependency matrix; all named stable contracts exist; all current-stage task files are linked.

### Task 3: Generate Workflow, MCP, and Tracking Markdown

**Files:**
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/03_Workflow/ProjectR_Codex_Development_Handbook.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/03_Workflow/ProjectR_Unreal_MCP_Capability_and_Authoring_Guide.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/03_Workflow/ProjectR_Codex_Automation_Ownership_Matrix.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/03_Workflow/ProjectR_Manual_Operations_Runbook.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/03_Workflow/AGENTS.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/03_Workflow/CURRENT_VERSION.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/04_Tracking/DevelopmentLog.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/04_Tracking/KnownIssues.md`
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/04_Tracking/ArchitectureDecisions.md`

**Interfaces:**
- Consumes: system architecture, roadmap, current MCP tool audit, and approved automation-first rules.
- Produces: durable Codex behavior, exact MCP authoring policy, automation ownership, human exception runbooks, and tracking templates.

- [ ] **Step 1: Generate AGENTS and current-version controls**

AGENTS must require reading the dependency map and direct downstream contracts before edits. CURRENT_VERSION must point to `Docs/Versions/v0.0.0.md` and prohibit future-version implementation.

- [ ] **Step 2: Generate the development handbook**

Document the plan/write/verify/review loop, future compatibility review, version advancement, completion report, Git boundaries, and honest reporting requirements.

- [ ] **Step 3: Generate MCP capability and authoring guidance**

Record the verified 19 toolsets/255 tools, capability tiers, package collision checks, serialized writes, Blueprint compile/save/restart/PIE distinctions, and custom Toolset escalation.

- [ ] **Step 4: Generate automation ownership and manual runbooks**

Assign C++, config, assets, Blueprint graphs, validation, subjective review, credentials, destructive confirmation, and external content to explicit owners. Every manual procedure includes why, menu path, asset/property values, expected result, validation, and rollback.

- [ ] **Step 5: Generate tracking documents**

Seed DevelopmentLog with the documentation baseline, KnownIssues with verified current gaps, and ArchitectureDecisions with accepted naming, runtime AI, MCP, and document-system decisions.

### Task 4: Generate v0.0.0-v0.0.5 Task Contracts and Repository Copies

**Files:**
- Create: `E:/MyWork/ProjectR_Design/CodexReady_v6/02_Roadmap/Versions/v0.0.0.md` through `v0.0.5.md`
- Create: `E:/MyWork/ProjectR/AGENTS.md`
- Create: `E:/MyWork/ProjectR/CURRENT_VERSION.md`
- Create operational Markdown under `E:/MyWork/ProjectR/Docs/` as listed in the approved specification.

**Interfaces:**
- Consumes: roadmap v6, data contracts, automation ownership, and current real project baseline.
- Produces: six executable task contracts and the exact Markdown context Codex loads during implementation.

- [ ] **Step 1: Generate six task contracts**

Each contract contains scope, non-goals, allowed/forbidden paths, downstream consumers, frozen contracts, extension points, C++ work, MCP manifest, human judgment, automated verification, PIE checks, rollback conditions, and completion report.

- [ ] **Step 2: Synchronize operational Markdown**

Copy content, adjusting only relative links. Do not copy PDFs into the game repository.

- [ ] **Step 3: Verify current-version resolution**

Expected: `CURRENT_VERSION.md` resolves to an existing v0.0.0 task; all files referenced by v0.0.0 exist.

- [ ] **Step 4: Verify protected paths are untouched**

Compare hashes/timestamps for `Source/`, `Content/`, `Config/`, and `ProjectR.uproject` against the pre-task snapshot.

Expected: no task-authored changes in protected paths.

### Task 5: Implement the ReportLab Markdown-to-PDF Renderer

**Files:**
- Create: `E:/MyWork/ProjectR/tmp/pdfs/codexready/render_markdown_pdf.py`

**Interfaces:**
- Consumes: one UTF-8 Markdown file with YAML metadata.
- Produces: one A4 PDF with cover, separate table of contents, headers, footers, page numbers, CJK text, tables, code blocks, lists, and page-break support.

- [ ] **Step 1: Implement Markdown block parsing**

Support YAML metadata, headings, paragraphs, blockquotes, bullet/number lists, fenced code, pipe tables, horizontal rules, and `<div class="page-break"></div>`.

- [ ] **Step 2: Implement ReportLab layout**

Use a Windows CJK font, repeated table headers, splittable tables, readable code blocks, bookmarks, table of contents, and page templates.

- [ ] **Step 3: Render a short smoke document**

Expected: PDF opens with pypdf, contains Chinese text, has at least three pages, and renders to PNG without black squares.

- [ ] **Step 4: Correct renderer defects before bulk rendering**

Do not continue if cover, TOC, tables, code, or Chinese glyphs are broken.

### Task 6: Render and Validate Every PDF

**Files:**
- Create one PDF beside every archival Markdown file listed in the manifest.
- Create temporary page PNGs and contact sheets under `E:/MyWork/ProjectR/tmp/pdfs/codexready/rendered/`.

**Interfaces:**
- Consumes: final archival Markdown and the renderer.
- Produces: same-name PDF pairs and visual QA evidence.

- [ ] **Step 1: Bulk-render all PDFs**

Expected: renderer exits 0 for every Markdown; no PDF is zero bytes.

- [ ] **Step 2: Run structural PDF checks**

Use `pdfinfo`, pypdf, and pdfplumber to verify page count, metadata, extractable title/version text, and absence of parser errors.

- [ ] **Step 3: Render every page to PNG**

Use Poppler `pdftoppm`; failure of any page is a blocking defect.

- [ ] **Step 4: Inspect all pages through contact sheets and targeted full-size pages**

Check clipping, overlap, table overflow, tiny code, broken CJK, orphan headings, page headers/footers, and section transitions.

- [ ] **Step 5: Iterate renderer/content until zero visual defects remain**

Re-render every affected PDF after each meaningful fix.

### Task 7: Final Consistency Audit and Delivery

**Files:**
- Update: `E:/MyWork/ProjectR_Design/CodexReady_v6/build_manifest.json`
- Update: `E:/MyWork/ProjectR_Design/CodexReady_v6/README.md`

**Interfaces:**
- Consumes: all generated Markdown/PDF artifacts and repository copies.
- Produces: final checksums, validation results, known limitations, and a navigable delivery index.

- [ ] **Step 1: Verify original hashes are unchanged**

Expected: all four source hashes equal Task 1 values.

- [ ] **Step 2: Verify Markdown/PDF pairing and repository-copy consistency**

Expected: zero missing pairs; operational copies match their archival sources apart from approved relative links.

- [ ] **Step 3: Run placeholder, identifier, and version scans**

Reject unintentional `TBD`, `TODO`, `USPDeveloperSettings`, `USPTagLibrary`, unresolved `<...>` placeholders, missing public versions, and broken local links.

- [ ] **Step 4: Record actual validation results in manifest and README**

Include source hashes, output hashes, PDF page counts, visual QA status, protected-path status, and the explicit statement that Git commit was not possible because no repository exists.

- [ ] **Step 5: Deliver clickable paths and the next executable task**

The final handoff identifies `CURRENT_VERSION = v0.0.0` and clearly separates completed documentation work from unexecuted game-development work.

