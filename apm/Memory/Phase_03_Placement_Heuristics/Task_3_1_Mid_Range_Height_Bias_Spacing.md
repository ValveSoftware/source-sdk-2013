# Task 3.1 — Mid-Range Distance, Light Height Bias, Nest Spacing (Guarded)

- Status: Completed
- Date: 2025-09-03

## Summary
Implemented guarded placement heuristics for Engineer sentry placement behind `TF_BOT_ENGINEER_SEAMS`:
- Mid-range preference: use `Seam_DesiredPlacementRange(...)` when non-zero; fallback to `0.9 × SENTRY_MAX_RANGE` under guard.
- Nest spacing: avoid clustering near existing friendly nests using `Seam_NestSpacingMin(...)`; fallback to `400` units under guard.
- Light height bias: reorder candidates to prefer areas with positive `Seam_HeightBias(...)`; fallback to favor areas above the point centroid under guard.

Default behavior (seams OFF) remains unchanged.

## Files Touched
- `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_move_to_build.cpp`
  - Guarded hooks and fallbacks:
    - Mid-range distance selection inside CP-specific range limit.
    - Nest spacing filter prior to `exposedAreaVector` append.
    - Height bias reordering post `exposedAreaVector.Sort(...)`.
- `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_seams.h`
  - Header-only seam stubs retained (default returns 0/no-op).
  - Temporarily toggled `TF_BOT_ENGINEER_SEAMS` to `1` for compile-only verification; reverted to `0` after build.

## Build Results
- Seams OFF (default): success
  - Command: `cd src && ./buildallprojects release | tee ../buildlog/.build_phase3_t3_1.out`
  - Log: `buildlog/.build_phase3_t3_1.out`
- Seams ON (compile-time verification): success
  - Temporary define: `TF_BOT_ENGINEER_SEAMS=1` (header edit)
  - Command: `cd src && ./buildallprojects release | tee ../buildlog/.build_phase3_t3_1_seams_on.out`
  - Log: `buildlog/.build_phase3_t3_1_seams_on.out`
  - Reverted header to default OFF after build.

## Notes
- Seam stubs will be tuned with CVars and hint-aware logic in Tasks T3.2–T3.3. Current guarded fallbacks provide sensible behavior when seam returns are zero.

---

## Instrumentation Addendum (Traces)
- Status: Added traces
- Date: 2025-09-04

### What was added (guarded by `TF_BOT_ENGINEER_SEAMS` and `developer 1`)
- Range rejects (sampled) and summary counts.
- Nest spacing rejects (sampled) and summary counts.
- Height bias summary (preferred vs others) and kept-areas/total-surface summary.
- Final selection traces (home area, hint, area center + random point).

### Trace locations
- `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_move_to_build.cpp:211`
- `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_move_to_build.cpp:265`
- `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_move_to_build.cpp:315`
- `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_move_to_build.cpp:342`
- `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_move_to_build.cpp:365`
- `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_move_to_build.cpp:399`
- `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_move_to_build.cpp:423`

### Build results (traces)
- Seams ON: success
  - Header temporarily set `TF_BOT_ENGINEER_SEAMS=1`
- Command: `cd src && ./buildallprojects release | tee ../buildlog/.build_phase3_t3_1_traces_seams_on.out`
- Log: `buildlog/.build_phase3_t3_1_traces_seams_on.out`
- Seams OFF: success (reverted header to default `0`)
- Command: `cd src && ./buildallprojects release | tee ../buildlog/.build_phase3_t3_1_traces_seams_off.out`
- Log: `buildlog/.build_phase3_t3_1_traces_seams_off.out`

### How to capture
- Console: `developer 1`, optional `con_timestamp 1`
- Log to file: `con_logfile nbdebuglogs/nextbot.debug.txt`
- Enable NextBot debug (choose one): `nb_debug behavior` | `nb_debug path` | `nb_debug look_at` | `nb_debug vision` | `nb_debug locomotion`
- Disable NextBot debug: `nb_debug` (no argument)
- Optional visual overlay: `tf_bot_debug_sentry_placement 1`
- Stop logging: `con_logfile ""`
