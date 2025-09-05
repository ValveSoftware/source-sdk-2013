APM Memory Log — Task 5.1: Repair Priority (Guarded)

Status: Completed

Summary:
- Added a guarded policy to prefer repairing damaged buildings over upgrading when below a configurable health fraction threshold. Integrated into `CTFBotEngineerBuilding::UpgradeAndMaintainBuildings` with strict parity when seams are off or the threshold is disabled.
- Honors seam advisory via `Seam_ShouldRepairFirst(...)` first; if that returns false and the cvar threshold is set (> 0), overrides baseline upgrade choices to repair first. Sapper/plasma-disabled and in-progress building states remain highest priority and unchanged.

Introduced CVar (default):
- `tf_bot_engineer_seams_repair_health_frac` ("0"): Health fraction below which repair takes priority (0=disabled)

Decision Logic (guarded):
- Compute health fractions for own sentry and dispenser and the Engineer’s metal count.
- Run baseline selection chain to choose a `workTarget`, tracking whether the choice was due to upgrade preference.
- If chosen by upgrade and either:
  - `Seam_ShouldRepairFirst(workTarget, engMetal)` is true; or
  - `tf_bot_engineer_seams_repair_health_frac > 0` and any building is below the threshold;
  then pick a repair target:
  - If both under threshold, prefer sentry; otherwise pick the one under threshold.
  - If only seam requested repair (no threshold), repair the more damaged building, preferring sentry on ties.
- Emit optional debug when this repair-priority decision engages:
  - `[TF-ENG seam] Repair priority: sentry=<frac> disp=<frac> chosen=<type> cvar=<val> seam=<0|1>`

Code References:
- src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_building.cpp
  - New guarded cvar declaration under `#if TF_BOT_ENGINEER_SEAMS` near other seam cvars.
  - Repair-first override integrated inside `UpgradeAndMaintainBuildings(CTFBot *me)` after baseline candidate selection.

Behavior/Parity:
- Seams OFF: No changes; code gated by `#if TF_BOT_ENGINEER_SEAMS`.
- Seams ON with `tf_bot_engineer_seams_repair_health_frac=0`: Baseline-equivalent behavior (no repair-priority override unless seam hook requests it).
- Seams ON with threshold > 0: When baseline would upgrade, bots repair first if a building is below threshold, preferring the sentry when both are affected.

Build Results:
- Command: `cd src && ./buildallprojects release | tee ../buildlog/.build_phase5_t5_1.out`
- Result: Successful compile/link; `server.so` copied. Parity maintained per guards.

Notes:
- Debug prints are gated by `developer` and `tf_bot_engineer_seams_debug`.
- No unrelated behavior was modified; sapper/plasma-disabled and building-in-progress priorities are unchanged.

Validation (Headless):
- Tools: `scripts/headless_test.sh` (cleans `game/mod_tf/nbheadlessdebuglogs/` per run), `cfg/headless.cfg`, `cfg/listenserver.cfg` (deferred nb_debug apply at map load).
- Run: `scripts/headless_test.sh pl_upward 8 behavior 180`
- Evidence: `game/mod_tf/nbdebuglogs/nextbot.debug.txt` shows repeated lines:
  - `[TF-ENG seam] Repair priority: sentry=1.00 disp=1.00 chosen=sentry cvar=1.50 seam=0` when `tf_bot_engineer_seams_repair_health_frac=1.5` (forced threshold), confirming repair-first engagement. With cvar=0, behavior remained baseline.

Auxiliary Config Refactor:
- `cfg/autoexec.cfg` reduced to universal settings (sv_cheats, developer, timestamps).
- `cfg/nbdebug.cfg` holds interactive NextBot debug aliases and logging routes; `cfg/nb_visual_debug.cfg` presets look_at/vision runs; `cfg/headless.cfg` routes to `nbheadlessdebuglogs/` and defers `nb_debug` to map load; `cfg/listenserver.cfg` applies any pending debug aliases after map init.
