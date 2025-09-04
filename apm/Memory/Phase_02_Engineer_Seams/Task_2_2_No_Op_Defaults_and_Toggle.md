# Task 2.2 - No-Op Defaults and Toggle Policy

- Status: DONE
- Date: 2025-09-04

## Summary
- Objective: Ensure header-only seams remain no-op by default and clearly document how to enable them locally via build flags without touching build scripts or wiring.
- Result: Updated `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_seams.h` with concise toggle usage docs; macro remains default-off (`TF_BOT_ENGINEER_SEAMS=0`). No functional changes.

## Output
- Updated: `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_seams.h`
  - Added "Toggle Policy" usage block with examples:
    - Example 1 (preferred): `-DTF_BOT_ENGINEER_SEAMS=1`
    - Example 2 (temporary): edit header to set `#define TF_BOT_ENGINEER_SEAMS 1` (not for commits)
  - Note included: runtime CVars for tuning to arrive in later phases; wiring will happen in T2.3.
  - Confirmed guard pattern: `#ifndef TF_BOT_ENGINEER_SEAMS` / `#define TF_BOT_ENGINEER_SEAMS 0` remains intact.
  - Confirmed all inline stubs return conservative values (0.0f/0/false).

## Behavior
- No includes added, no wiring performed, no build scripts modified.
- Default-off policy unchanged; behavior parity guaranteed.

## Next
- T2.3: Wire minimal guarded call sites in Engineer behaviors behind `TF_BOT_ENGINEER_SEAMS`.

