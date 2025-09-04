APM Memory Log — Task 3.3: Expose Tuning CVars (Guarded)

Status: Completed

Summary:
- Added guarded tuning cvars for Engineer placement heuristics and lightly integrated them behind `TF_BOT_ENGINEER_SEAMS` in `tf_bot_engineer_move_to_build.cpp`. Defaults preserve parity when seams are off; when seams are on, cvars allow iterative tuning without code edits.

Introduced CVars (defaults):
- `tf_bot_engineer_seams_desired_range` ("0"): Desired max sentry placement range (0=default)
- `tf_bot_engineer_seams_nest_spacing_min` ("0"): Minimum distance between friendly sentry nests (0=default)
- `tf_bot_engineer_seams_height_bias_weight` ("0"): Weight to prefer areas above the objective (0=disabled)
- `tf_bot_engineer_seams_hint_radius` ("900"): Proximity radius for hint-aware bias
- `tf_bot_engineer_seams_hint_bias_weight` ("0"): Additional bias weight for hint proximity (0=none)

Code References:
- src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_move_to_build.cpp
  - Guarded cvar declarations colocated with `tf_bot_engineer_seams_debug` under `#if TF_BOT_ENGINEER_SEAMS`.
  - CP-specific desired range block: honors `Seam_DesiredPlacementRange`; if zero, uses `tf_bot_engineer_seams_desired_range` (>0) with tolerance; else falls back to mid-range bias (0.9x).
  - Nest spacing filter: when `Seam_NestSpacingMin(me) <= 0`, reads `tf_bot_engineer_seams_nest_spacing_min`; if still <= 0, defaults to `400.0f`.
  - Height bias: after `Seam_HeightBias(...)`, if 0 then applies `tf_bot_engineer_seams_height_bias_weight` when > 0; otherwise parity 0/1 fallback.
  - Hint-aware proximity: `kHintBiasRadius` replaced by `tf_bot_engineer_seams_hint_radius` (fallback 900 if <= 0); on proximity-fallback path, adds `tf_bot_engineer_seams_hint_bias_weight` to `hintBias` before thresholding.

Build Results:
- Command: `cd src && ./buildallprojects release | tee ../.build_phase3_t3_3.out`
- Seams OFF parity: Preserved by guarding all changes under `TF_BOT_ENGINEER_SEAMS` and using no-op seam stubs; build succeeded.
- Seams ON compile path: New cvars and usages compiled successfully (see build log).
- Log: `.build_phase3_t3_3.out` indicates successful compile/link and copy of `server.so`.

Notes:
- No behavior changes when seams are disabled at compile time.
- With seams enabled, tunables are available for iteration without code changes.
