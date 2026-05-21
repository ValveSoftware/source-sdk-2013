# Frog Fortress — choose your plan later

Both **Overwatch-style heroes** and **Rainbow Is Magic (RIM)** stay in the mod. Nothing is deleted — you pick which ruleset is active.

## Quick switch

| What you want | Console | Config |
|---------------|---------|--------|
| OW heroes (default) | `ff_mode ow` | `exec mode_ow.cfg` |
| RIM hostages | `ff_mode rim` | `exec mode_rim.cfg` |
| Stock mod_tf | `ff_mode stock` | `exec mode_stock.cfg` |

Run `ff_mode` with no args to see the current mode.

After switching, **reload the map** (`changelevel <map>`) so spawn logic, objectives, and bots match the new mode.

## Canonical ConVar

`tf_ff_game_mode`:

- `0` — stock mod_tf (OW and RIM off)
- `1` — Overwatch-style heroes (`tf_ow_mode` synced on)
- `2` — Rainbow Is Magic hostages (`tf_rim_mode` synced on)

Legacy `tf_ow_mode` / `tf_rim_mode` still work; the server keeps them mutually exclusive and updates `tf_ff_game_mode`.

## Suggested maps

- **OW:** `koth_badlands` (then `exec ow_quickstart` for 6v6 bots)
- **RIM:** `plr_hightower` or other Payload maps with hostage setup

## Code

Mode checks live in `CTFGameRules::IsOverwatchMode()` / `IsRainbowIsMagicMode()` in `tf_gamerules.cpp`. RIM entities and win logic remain compiled under `#ifdef SOURCESDK` and only run when mode `2` is active.
