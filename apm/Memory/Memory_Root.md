# Memory Root — dynamic‑md

- Strategy: dynamic‑md
- Asset format: md
- Workspace root: .

Purpose
- Track build/run observations and phase/task progress across milestones.
- Keep entries concise; link to code paths when helpful.

Structure
- Phase directories created on demand by the Manager Agent:
  - `apm/Memory/Phase_01_Baseline_Build/`
  - `apm/Memory/Phase_02_Engineer_Seams/`
  - `apm/Memory/Phase_03_Placement_Heuristics/`
  - `apm/Memory/Phase_04_Teleporter/`
  - `apm/Memory/Phase_05_Maintenance/`
  - `apm/Memory/Phase_06_Validation/`
- Each phase contains one file per task (empty at creation time) to accumulate notes.

Conventions
- Date prefix entries `[YYYY-MM-DD]`.
- Keep actionable bullets and links to files (e.g., `src/game/server/tf/...:42`).
- Defer large artifacts to repo docs; keep memory focused on iteration signals.

Index
- Milestone 1 Phases (planned):
  - 01 Baseline Build & Branch Setup
  - 02 Engineer Seams (Header-Only) + Toggle
  - 03 Placement Heuristics (Guarded)
  - 04 Teleporter Validation & Redeploy (Guarded)
  - 05 Upgrade/Repair Policy (Guarded)
  - 06 Validation & Findings

## Phase 01 – Baseline Build & Branch Setup Summary
> Delivered: T1.1 (branch created)
> Outcome: Container toolchain OK; `vpc` `/ninja` segfaults outside script. `src/buildallprojects` generated Ninja graph + compile DB; build failed at HL2MP link. TF-only manual graph generation segfaulted.
> Next: Use generated `sdk_everything_release.ninja` to build TF-only targets for baseline; consider pinning known-good `vpc`.
