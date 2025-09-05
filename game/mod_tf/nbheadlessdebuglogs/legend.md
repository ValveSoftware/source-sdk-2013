# Headless NextBot Debugging Legend

This mirrors the structure of `nbdebuglogs/`, but is used by the headless runner (`scripts/headless_test.sh`) and `cfg/headless.cfg` so that ad‑hoc tests don’t mix with your regular logs.

Key points:
- Logs live under `game/mod_tf/nbheadlessdebuglogs`.
- Subfolder `bot-visuals-debug/` contains the heavy visual spam modes (`look_at`, `vision`). Keep bot counts low for these.
- Typical files:
  - `nextbot.debug.txt`: general console output, cvar echoes.
  - `nb.behavior.debug.txt`: `nb_debug behavior` traces.
  - `nb.path.debug.txt`, `nb.locomotion.debug.txt`: path/loco traces.
  - `bot-visuals-debug/nb.lookat.debug.txt`, `bot-visuals-debug/nb.vision.debug.txt`.

NB debug modes (via `headless.cfg` aliases):
- `hd_apply_nb_behavior`, `hd_apply_nb_path`, `hd_apply_nb_loco`, `hd_apply_nb_lookat`, `hd_apply_nb_vision`, `hd_apply_nb_off`.

Tip: The headless script clears `*.txt` under this folder before each run so you always start with fresh logs.

