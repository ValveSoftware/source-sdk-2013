APM Memory Log — Task 5.3: Threat-Aware Repair Interrupt (Guarded)

Status: Completed

Summary:
- Added guarded logic to briefly interrupt repair/upgrade actions when the Engineer comes under immediate threat near the nest. When enabled, if the bot was injured within a recent, configurable window, it either pauses wrenching for a tick or temporarily switches to the attack behavior, then resumes maintenance after a short cooldown. All changes are fully guarded and default off, preserving baseline parity.

Introduced CVars (default):
- `tf_bot_engineer_seams_repair_interrupt` ("0"): Enable repair/upgrade interrupt under nearby threat
- `tf_bot_engineer_seams_repair_interrupt_range` ("600"): Threat radius for repair interrupt (currently informational; injury window is primary trigger)
- `tf_bot_engineer_seams_repair_interrupt_window` ("1.5"): Recent injury window (sec) to consider interrupting
- `tf_bot_engineer_seams_repair_interrupt_attack` ("0"): If 1, briefly switch to attack behavior; else just pause wrenching
- `tf_bot_engineer_seams_repair_interrupt_cooldown` ("2.0"): Cooldown between interrupts (sec)

Class State:
- `CountdownTimer m_repairInterruptCooldown;` with `Invalidate()` in `OnStart` to ensure immediate eligibility.

Integration (guarded):
- Location: `CTFBotEngineerBuilding::Update()`
  - Added checks immediately before each call to `UpgradeAndMaintainBuildings(me)`:
    - If `tf_bot_engineer_seams_repair_interrupt=1` and `m_repairInterruptCooldown.IsElapsed()` and `me->GetTimeSinceLastInjury() < window`, then:
      - Start cooldown (`repair_interrupt_cooldown`).
      - If `repair_interrupt_attack=1`: `SuspendFor(new CTFBotAttack, ...)` to defend nest.
      - Else: `return Continue();` this tick to pause wrenching and allow attack layers to act.
- Optional debug (gated by `developer` and `tf_bot_engineer_seams_debug`):
  - `[TF-ENG seam] Repair interrupt: injury=%d range=%.0f attack=%d`

Code References:
- src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_building.h
- src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_building.cpp
  - New guarded CVars declared near existing seam CVars.
  - `m_repairInterruptCooldown` added and invalidated in `OnStart`.
  - Interrupt checks added in `Update()` before maintenance calls.

Behavior/Parity:
- Seams OFF: No changes.
- Seams ON with `tf_bot_engineer_seams_repair_interrupt=0`: Identical to Tasks 5.1/5.2 behavior.
- Seams ON with interrupt enabled: Bot momentarily defends on recent injury, then resumes maintenance after cooldown.

Build Results:
- Command: `cd src && ./buildallprojects release`
- Result: Successful compile/link; server binary updated.

Validation Plan (Headless/Interactive):
- Enable seams + debug: `sv_cheats 1; developer 1; tf_bot_engineer_seams_debug 1`.
- Set: `tf_bot_engineer_seams_repair_interrupt 1; tf_bot_engineer_seams_repair_interrupt_window 1.5; tf_bot_engineer_seams_repair_interrupt_cooldown 2.0`.
- Optional: `tf_bot_engineer_seams_repair_interrupt_attack 1` to force a brief `CTFBotAttack` suspend.
- Observe in logs around active engineer nests:
  - `[TF-ENG seam] Repair interrupt: injury=1 range=600 attack=0|1`
- Confirm parity by disabling the seam CVar: no interrupts and baseline maintenance.

Notes:
- Proximity CVar is present for future extension; current threat trigger is the recent injury window to keep the check cheap and low-noise.
- All prints are gated by `developer` and the seam debug CVar.
