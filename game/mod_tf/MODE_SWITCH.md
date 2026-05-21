# Frog Fortress — choose your plan later

Both **Overwatch-style heroes** and **Rainbow Is Magic (RIM)** stay in the mod. Nothing is deleted — you pick which ruleset is active.

## Quick switch

| What you want | Console | Config |
|---------------|---------|--------|
| OW heroes (default) | `ff_mode ow` | `exec mode_ow.cfg` |
| RIM hostages | `ff_mode rim` | `exec mode_rim.cfg` |
| Frog Bomber (grid) | `ff_mode bomber` | `exec mode_bomber.cfg` |
| Stock mod_tf | `ff_mode stock` | `exec mode_stock.cfg` |

Run `ff_mode` with no args to see the current mode.

`ff_mode <name>` now also runs the matching `mode_*.cfg`. For a full switch (cfg + map), use `ff_play` below.

## One-command launch (recommended)

Use `ff_play` to apply mode, execute the right cfg, and switch map in one step:

Or load shortcuts once: `exec ff_aliases.cfg`, then `ff_ow`, `ff_rim`, `ff_bomber`, or `ff_stock`.

- `ff_play ow` -> defaults to `koth_badlands`
- `ff_play rim` -> defaults to `plr_hightower`
- `ff_play bomber` -> defaults to `koth_badlands` (grid auto-aligned to team spawns)
- `ff_play stock` -> defaults to `ctf_2fort`

After any map load, the server runs **post-map setup** automatically (~4s): kicks old bots, applies mode cfg, restarts the match, then spawns OW/RIM bots or respawns bomber players on the grid.

Same map, no reload: `ff_restart`

You can override the map:

- `ff_play ow cp_foundry`
- `ff_play rim plr_hightower`

## Canonical ConVar

`tf_ff_game_mode`:

- `0` — stock mod_tf (OW and RIM off)
- `1` — Overwatch-style heroes (`tf_ow_mode` synced on)
- `2` — Rainbow Is Magic hostages (`tf_rim_mode` synced on)
- `3` — Frog Bomber (top-down grid; see `BOMBERMAN.md`)

Legacy `tf_ow_mode` / `tf_rim_mode` still work; the server keeps them mutually exclusive and updates `tf_ff_game_mode`.

## Suggested maps

- **OW:** `koth_badlands` (then `exec ow_quickstart` for 6v6 bots)
- **RIM:** `plr_hightower` or other Payload maps with hostage setup

## Code

Mode checks live in `CTFGameRules::IsOverwatchMode()` / `IsRainbowIsMagicMode()` / `IsBombermanMode()` in `tf_gamerules.cpp`. Custom logic only runs for the active `tf_ff_game_mode` value.

**RIM bots:** after the round is running, `rim_add_bots` or `exec rim_quickstart`.

**Bomber:** see `BOMBERMAN.md`; align grid with `getpos` then `tf_bm_grid_origin`.
