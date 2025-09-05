---
agent: Agent_Implementation
task_ref: Task 5.4 – Combat weapon prioritization while maintaining nest
status: Completed
ad_hoc_delegation: false
compatibility_issues: false
important_findings: true
---

# Task Log: T5.4 – Combat Weapon Prioritization While Maintaining Nest (Guarded)

## Summary
Added seam-guarded weapon prioritization for Engineers while maintaining the nest. When enabled, the bot prefers shotgun/pistol based on nearby visible threats or recent injury, with low-noise debug and thrash avoidance. Defaults keep parity (no behavior change).

## Details
- Implemented under `#if TF_BOT_ENGINEER_SEAMS` following existing seam patterns.
- New CVars (default off / conservative):
  - `tf_bot_engineer_seams_weapon_prioritize "0"`
  - `tf_bot_engineer_seams_weapon_shotgun_range "450"`
  - `tf_bot_engineer_seams_weapon_pistol_range "900"`
- Integration point: `CTFBotEngineerBuilding::UpgradeAndMaintainBuildings(CTFBot *me)`
  - Early decision to optionally hold a combat weapon when:
    - A primary known threat is visible recently and within range, or
    - Engineer was injured within T5.3’s `tf_bot_engineer_seams_repair_interrupt_window`.
  - Chooses shotgun if `dist <= shotgun_range`, else pistol if `dist <= pistol_range`.
  - Applies a small cooldown to avoid frequent weapon swaps.
  - Presses fire only when the wrench is active to prevent unintended shooting while holding firearms.
- Honored T5.3 behavior precedence: if repair interrupt is enabled and triggers, it supersedes this selection (existing logic unchanged).
- Added debug trace when a weapon selection occurs, gated by `developer` and `tf_bot_engineer_seams_debug`:
  - `[TF-ENG seam] Weapon prioritize: dist=%.0f choose=%s inj_recent=%d srg=%.0f prg=%.0f`

## Output
- Modified: `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_building.cpp`
  - Added new seam CVars for weapon prioritization.
  - Added guarded decision block in `UpgradeAndMaintainBuildings()` for selecting shotgun/pistol.
  - Ensured `PressFireButton()` only executes when the wrench is active under seams.
- Modified: `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_building.h`
  - Added `CountdownTimer m_weaponPrioritizeCooldown;` to limit swap thrash.

## Issues
None.

## Important Findings
- Seam compilation is enabled via `TF_BOT_ENGINEER_SEAMS` from `tf_bot_engineer_seams.h`. Runtime remains parity-safe since new behavior is additionally guarded by CVars defaulting to 0.

## Next Steps
- Optional tuning and validation in combat-heavy scenarios:
  - Test with: `developer 1; tf_bot_engineer_seams_debug 1; tf_bot_engineer_seams_weapon_prioritize 1; tf_bot_engineer_seams_weapon_shotgun_range 800; tf_bot_engineer_seams_weapon_pistol_range 1200`.
  - Combine with T5.3 to confirm precedence: `tf_bot_engineer_seams_repair_interrupt 1`.
  - Headless: `scripts/headless_test.sh pl_upward 8 behavior 180` with the above CVars and confirm `[TF-ENG seam] Weapon prioritize:` lines in logs.
