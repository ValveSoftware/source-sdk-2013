---
agent_type: Manager
agent_id: Manager_1
handover_number: 1
current_phase: Phase 04: Teleporter Validation & Redeploy
active_agents: [Agent_Implementation]
branch: dev/bot/overhaul
build_policy: src/buildallprojects
---

# Manager Agent Handover File - TF2 Engineer Bot Behavior Overhaul (Milestone 1)

## Active Memory Context
**User Directives:**
- MUST use `src/buildallprojects` for graph + builds (containerized).
- Preserve Valve coding style; minimal, surgical diffs.
- Guard all new Engineer logic behind compile-time macro/cvars; default OFF.
- Do not break non-Engineer classes; no build system changes.
- Conventional Commits; findings in `apm/findings.md`.

**Decisions:**
- Added header-only seams (`tf_bot_engineer_seams.h`) default-off.
- Placement heuristics under guard (mid-range, light height bias, nest spacing) with tuning cvars.
- Hint-aware bias and lightweight debug cvar (`tf_bot_engineer_seams_debug`).
- Teleporter link validation and optional redeploy guarded with cooldown and cvars.
- Parity maintained with seams OFF; compile-time only toggles validated.

## Coordination Status
**Producer-Consumer Dependencies:**
- T2.x (Seams) → T3.x (Placement hooks, cvars) and T4.x (Teleporter checks).
- T3.1 hooks → T3.3 cvars (runtime tuning via ConVars when seams ON).
- T4.1 validation (flags, min travel) → T4.2 redeploy (consumes flags with cooldown).
- T5.x (Maintenance policy) will consume seams; keep guarded and default-off.

**Coordination Insights:**
- Direct `devtools/bin/vpc /ninja` segfaults; always use `src/buildallprojects`.
- HL2MP targets fail linking in this branch; stick to TF module targets.
- Debug capture: `developer 1; tf_bot_engineer_seams_debug 1; con_logfile nbdebuglogs/nextbot.debug.txt`.

## Next Actions
**Ready Assignments:**
- Task 5.1 – Upgrade/Repair Policy (Guarded) → Agent_Implementation
  - Objective: Prefer repair under health fraction; keep metal reserve; cap upgrade level.
  - Scope: Guarded decisions only; default-off parity; add minimal cvars.
  - Memory Log: `apm/Memory/Phase_05_Maintenance/Task_5_1_Repair_Priority.md` (create if missing).
- Task 5.2 – Metal Reserve & Upgrade Cap (Guarded) → Agent_Implementation
  - Objective: Introduce metal reserve and max upgrade level logic.
  - Memory Log: `apm/Memory/Phase_05_Maintenance/Task_5_2_Metal_Reserve_and_Upgrade_Cap.md` (create if missing).

**Blocked Items:**
- None critical. Tooling note: VPC /ninja segfault outside `buildallprojects`.

**Phase Transition:**
- Phase 04 complete (2/2). Summarize Phase 04 in `Memory_Root` after Phase 05 kickoff.

## Working Notes
**File Patterns:**
- Seams header: `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_seams.h`
- Placement logic: `.../engineer/tf_bot_engineer_move_to_build.cpp`
- Teleporter/building: `.../engineer/tf_bot_engineer_building.cpp` (+ `.h`)

**Tuning CVars (guarded):**
- `tf_bot_engineer_seams_desired_range`, `tf_bot_engineer_seams_nest_spacing_min`, `tf_bot_engineer_seams_height_bias_weight`
- `tf_bot_engineer_seams_hint_radius`, `tf_bot_engineer_seams_hint_bias_weight`
- `tf_bot_engineer_seams_min_teleport_travel`, `tf_bot_engineer_seams_teleport_redeploy`, `tf_bot_engineer_seams_teleport_redeploy_cooldown`

**Coordination Strategies:**
- Keep hooks minimal and compile-guarded; add cvars before changing defaults.
- Validate seams OFF parity via `src/buildallprojects release` after wiring.

**User Preferences:**
- Concise, actionable prompts; document findings; maintain branch hygiene.

