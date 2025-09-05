Task: Phase 04 – Task 4.2 – Teardown/Redeploy When Invalid/Suboptimal (guarded)

Status: Completed — Guarded redeploy wired; defaults preserve parity; build clean.

Summary:
- Added guarded control cvars (default OFF) to enable safe teleporter exit redeploy when the link is invalid or travel is suboptimal.
  - `tf_bot_engineer_seams_teleport_redeploy` (0/1)
  - `tf_bot_engineer_seams_teleport_redeploy_cooldown` (seconds; default 15)
- Introduced a cooldown timer on the Engineer building action to prevent oscillation: `m_teleportRedeployCooldown`.
- In `CTFBotEngineerBuilding::Update()`, under `TF_BOT_ENGINEER_SEAMS` and with the above cvar enabled:
  - Compute seam flags and travel delta (from Task 4.1).
  - If both entrance and exit exist, cooldown elapsed, and the bot is not under immediate attack, detonate the teleporter exit when:
    - `seam_TeleporterLinkInvalid` OR `seam_TeleporterShouldRedeploy` evaluates true.
  - Start redeploy cooldown; existing `m_teleportExitRetryTimer` logic naturally staggers rebuild.
- Added debug emit on action with `[TF-ENG seam]` marker.

Files/Lines:
- src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_building.h:31
  - Added `CountdownTimer m_teleportRedeployCooldown;` to private section.
- src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_building.cpp:36
  - New cvars: `tf_bot_engineer_seams_teleport_redeploy`, `tf_bot_engineer_seams_teleport_redeploy_cooldown` (guarded).
- src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_building.cpp:327
  - Guarded redeploy logic and debug emit following seam flag computation.

Behavioral Notes:
- With seams OFF or `tf_bot_engineer_seams_teleport_redeploy=0`: No behavior change.
- With seams ON and cvar enabled: Engineer detonates teleporter exit when link invalid or travel is below threshold/guidance; rebuild leverages existing hint/near-sentry paths; cooldown prevents oscillation.

Build Results:
- Command: `cd src && ./buildallprojects release | tee ../buildlog/.build_phase4_t4_2.out`
- Outcome: Success. `server.so` linked and copied into `game/mod_tf/bin/linux64/server.so`.
- Log: `buildlog/.build_phase4_t4_2.out`

Verification Tips:
- Enable logging: `developer 1; tf_bot_engineer_seams_debug 1; tf_bot_engineer_seams_teleport_redeploy 1; con_logfile nbdebuglogs/nextbot.debug.txt`
- Expect marker on redeploy: `[TF-ENG seam] Teleporter redeploy: detonate exit (invalid=... redeploy=... travelDelta=...)`
- Sanity: Confirm redeploys do not repeat rapidly; adjust `tf_bot_engineer_seams_teleport_redeploy_cooldown` as needed.

