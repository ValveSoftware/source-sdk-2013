# Task 2.1 - Header-Only Interfaces for Engineer Seams

- Status: DONE
- Date: 2025-09-04

## Summary
- Objective: Introduce header-only seam interfaces for future guarded Engineer logic without altering behavior or build configuration.
- Result: Added `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_seams.h` with a default-off toggle and inline no-op stubs. No includes added and no wiring performed.

## Output
- File: `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_seams.h`
- Toggle: `TF_BOT_ENGINEER_SEAMS` (default `0`; override via `-D` or local edit)
- Interfaces (inline, conservative return values):
  - `float Seam_DesiredPlacementRange(const CTFPlayer*, const CBaseEntity*)`
  - `float Seam_HeightBias(const CTFPlayer*, const Vector&)`
  - `float Seam_NestSpacingMin(const CTFPlayer*)`
  - `float Seam_HintBias(const CTFPlayer*, const CBaseEntity*)`
  - `bool  Seam_TeleporterIsValid(const CBaseObject*, const CBaseObject*)`
  - `bool  Seam_TeleporterShouldRedeploy(const CBaseObject*, const CBaseObject*, float)`
  - `bool  Seam_ShouldRepairFirst(const CBaseObject*, int)`
  - `int   Seam_TargetUpgradeLevel(const CBaseObject*)`
  - `int   Seam_MetalReserve(const CTFPlayer*)`
- Notes: Forward declarations only; no external includes. Inline stubs return `0.0f`, `0`, or `false` to guarantee zero behavior change if used prematurely.

## Updates
- [2025-09-04] Style alignment with Valve headers:
  - Replace `#pragma once` with include guard `TF_BOT_ENGINEER_SEAMS_H` (with `#endif // TF_BOT_ENGINEER_SEAMS_H`).
  - Pointer/reference spacing to `Type *name`, `Type &name`.
  - Forward-declare `Vector` as `class Vector;`.
  - Tabs for indentation in bodies.

## Next
- T2.2: Keep defaults no-op; finalize toggle policy documentation and examples for local enablement.
- T2.3: Wire minimal guarded call sites within Engineer behaviors behind `TF_BOT_ENGINEER_SEAMS`.
