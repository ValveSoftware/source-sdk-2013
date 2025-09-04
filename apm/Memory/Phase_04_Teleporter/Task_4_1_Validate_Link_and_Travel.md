Task: Phase 04 – Task 4.1 – Validate entrance/exit link and travel delta (guarded)

Status: Completed — Guarded checks added; parity preserved with seams off; build clean.

Summary:
- Added compile-time-guarded validation for Engineer teleporter link when both entrance and exit exist for the bot.
- Computes travel metric:
  - Prefer TF nav incursion delta (same-team) when both areas are available.
  - Fallback to 3D world distance when areas are missing.
- Calls seam stubs under `TF_BOT_ENGINEER_SEAMS`:
  - `Seam_TeleporterIsValid(entrance, exit)`
  - `Seam_TeleporterShouldRedeploy(entrance, exit, travelDelta)`
- Building state sets local guarded flags (for T4.2 consumption only; no behavior changes now):
  - `seam_TeleporterLinkInvalid`
  - `seam_TeleporterShouldRedeploy`
- Added optional guarded cvar: `tf_bot_engineer_seams_min_teleport_travel` (default 3000) for fallback redeploy guidance when seam returns don’t provide it.
- Lightweight debug logging behind `developer` and `tf_bot_engineer_seams_debug` with marker `[TF-ENG seam]`.

Files/Lines:
- src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_build_teleport_entrance.cpp:66
  - Guarded link/travel check + seam calls + debug log in `Update()`.
- src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_build_teleport_exit.cpp:87
  - Guarded link/travel check + seam calls + debug log in `Update()`.
- src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_building.cpp:33
  - ConVar `tf_bot_engineer_seams_min_teleport_travel` (guarded).
- src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_building.cpp:291
  - Guarded local flags + travel computation and seam calls in `Update()`.
- src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_building.cpp:327
  - Guarded debug log for teleporter link assessment.

Behavioral Notes:
- No changes to baseline logic with seams off.
- With seams on, only local flags and debug logs are set/emitted; no destruction/redeploy yet. T4.2 will consume these flags.

Build Results:
- Command: `cd src && ./buildallprojects release | tee ../.build_phase4_t4_1.out`
- Outcome: Success. `server_tf` objects compiled and `server.so` linked and copied.
- Log: `.build_phase4_t4_1.out`

Verification Tips:
- Enable logging: `developer 1; tf_bot_engineer_seams_debug 1; con_logfile nbdebuglogs/nextbot.debug.txt`
- Grep marker: `[TF-ENG seam] Teleporter link check` to find per-state prints.

