---
Use: github
Memory_strategy: dynamic-md
Asset_format: md
Branch: dev/bot/overhaul
Workspace_root: .
---

# Implementation Plan — Milestone 1 (Engineer Behaviors)

Overview
- Keep to Valve coding style; apply minimal, surgical diffs.
- Acceptance: Clean builds and concise findings in `apm/findings.md` (e.g., "Engineer bots stand still…", "Engineer bots attack properly…").

Goals
- Make Engineer bot behavior more human-like and less static while preserving stability.
- Introduce modular seams that allow incremental improvements behind toggles.

Constraints
- Preserve baseline behavior by default (guard experimental logic behind a macro/cvars).
- Avoid build system changes and keep existing VPC/Ninja flow.
- Do not break non-Engineer classes.

Phases and Tasks
- Phase 1 — Baseline Build & Branch Setup
  - T1.1 Create branch `dev/bot/overhaul` off `master`.
  - T1.2 Validate container toolchain (Sniper SDK) and generate Ninja solution.
  - T1.3 Run full build; capture results in `apm/findings.md`.
- Phase 2 — Engineer Seams (Header-Only) + Toggle
  - T2.1 Add header-only interfaces for placement, relocation, teleporter, maintenance decisions.
  - T2.2 Provide no-op defaults; add local config macro (disabled by default).
  - T2.3 Wire minimal guarded call sites.
- Phase 3 — Placement Heuristics (Guarded)
  - T3.1 Add mid-range distance, light height bias, nest spacing.
  - T3.2 Add hint-aware bias and debug cvar.
  - T3.3 Expose tuning cvars.
- Phase 4 — Teleporter Validation & Redeploy (Guarded)
  - T4.1 Validate entrance/exit link and travel delta.
  - T4.2 Teardown/redeploy when invalid/suboptimal.
- Phase 5 — Upgrade/Repair Policy (Guarded)
  - T5.1 Prefer repair under health fraction threshold.
  - T5.2 Keep metal reserve; cap upgrade target level.
  - T5.3 Threat-aware repair interrupt and combat response.
  - T5.4 Combat weapon prioritization while maintaining nest (shotgun/pistol vs wrench).
  - T5.5 Sapper triage: remove sapper vs attack spy (situational).
  - T5.6 Cover micro while repairing (crouch/strafe bias, guarded).
- Phase 6 — Validation & Findings
  - T6.1 Build with seams disabled (parity) and enabled (experiment).
  - T6.2 Smoke test on CP/KotH/Escort maps; log observations and next actions.
- Phase 7 — Documentation
  - T7.1 Update `apm/arch_map.md` with integration points and cvars.
  - T7.2 Document toggle instructions and build/run notes.

Deliverables
- Guarded code paths for Engineer (placement, teleporter, maintenance).
- Tuning cvars and a single header toggle.
- Updated APM docs and findings with clear, reproducible notes.

Preconditions & Dependencies
- Sniper SDK container available and working; VPC present (vendored at `src/devtools/bin/vpc` or available in image).
- Build graph can be generated and compiled.
- Test maps: CP, KotH, Escort for quick validation.
- Toggle default: experimental seams disabled by default; enable locally for testing.
- Canonical build path: use `src/buildallprojects` (containerized) to generate Ninja graph, compile DB, and build. Avoid direct `devtools/bin/vpc /ninja` calls (segfault observed in this branch).

Success Metrics (Milestone 1)
- Baseline parity with seams disabled (no behavioral regressions; builds pass).
- With seams enabled: human-like improvements:
  - Uses mid-range placement relative to objective, slight height bias.
  - Avoids clustering nests; respects hints when near objective.
  - Valid teleporter link and minimum travel; redeploys when suboptimal.
  - Prioritizes repair vs. upgrade according to health and metal reserve.
- Findings captured in `apm/findings.md` with concise entries and next actions.

Risks & Backout Plan
- If regressions occur: disable seams macro to restore baseline instantly.
- Keep guarded changes minimal and isolated to Engineer call sites.

Commit Policy
- Branch: `dev/bot/overhaul` (rebased on `master`).
- Conventional Commits (scope: engineer, docs, chore):
  - `feat(engineer): …` for new guarded behavior
  - `docs(arch): …` for `apm/arch_map.md` updates
  - `chore(memory): …` for findings and logs
