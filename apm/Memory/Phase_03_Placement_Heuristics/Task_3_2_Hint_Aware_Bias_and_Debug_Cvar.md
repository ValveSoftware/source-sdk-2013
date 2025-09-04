Status: Completed

Summary:
- Added guarded hint-aware placement bias in `CollectBuildAreas` to prefer nav areas near enabled, team-matching `bot_hint_sentrygun` entities. Bias first tries `Seam_HintBias(me, hint)` and falls back to a simple proximity score within a radius. Resulting areas are reordered by preference while keeping original candidate filtering and distance sort intact.
- Introduced guarded debug cvar `tf_bot_engineer_seams_debug` (default 0) for lightweight traces when `developer 1`.
- Added optional traces: counts of hint-biased vs other areas and nearest hint to the final chosen area when selecting a random point.
- All changes are guarded by `#if TF_BOT_ENGINEER_SEAMS` with no build script changes.

Files Updated:
- `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_move_to_build.cpp`

Key Changes:
- ConVar: `tf_bot_engineer_seams_debug` added (guarded).
- Bias logic: collect `bot_hint_sentrygun` via `IsAvailableForSelection(me)`, compute bias per area, and reorder candidate list.
- Debug: gated `DevMsg` logs showing preferred/other counts and nearest hint vs chosen area center.

Build Results:
- Command: `cd src && ./buildallprojects release | tee ../buildlog/.build_phase3_t3_2.out`
- Outcome: Build succeeded. Artifacts updated under `game/mod_tf/bin/linux64/server.so`.
- Seams default: No build script changes; guarded code compiles as before. Debug traces remain off by default.

Notes:
- Baseline selection flow and filters remain unchanged; hint bias only reorders the already filtered/sorted candidates when seams are compiled in. Debug output is additionally gated by `tf_bot_engineer_seams_debug` and `developer`.
