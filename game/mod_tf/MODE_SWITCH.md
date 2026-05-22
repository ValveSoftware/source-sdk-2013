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

`ff_mode <name>` runs the matching `mode_*.cfg`. **`ff_mode bomber`** (and `ff_play bomber`) also **changelevel** to `itemtest` / `bm_arena` when you are on a stock OW map — do not use `exec mode_bomber.cfg` alone on `koth_badlands`.

## One-command launch (recommended)

Use `ff_play` to apply mode, execute the right cfg, and switch map in one step:

Or load shortcuts once: `exec ff_aliases.cfg`, then `ff_ow`, `ff_rim`, `ff_bomber`, or `ff_stock`.

- `ff_play ow` -> defaults to `koth_badlands`
- `ff_play rim` -> defaults to `plr_hightower`
- `ff_play bomber` -> defaults to `bm_arena` (compile `mapsrc/bm_arena.vmf`; else `itemtest` + flat floor)
- `ff_play stock` -> defaults to `ctf_2fort`

### Join any game first, then switch mode

You do **not** need to start in bomber. This is the normal workflow:

1. Launch **mod_tf** (your build with Frog Fortress DLLs).
2. **Create server** or **join** any listen match (menu, `map`, `connect`, etc.) — OW is fine; you can land on `koth_badlands` or whatever.
3. When you are **host** (listen server) or server admin, run:
   - `ff_play bomber` or `exec bomber_quickstart` or `ff_bomber` (after `exec ff_aliases.cfg`)
4. The mod **changelevels** to `bm_arena` (or `itemtest` if the BSP is not compiled yet), runs `mode_bomber.cfg`, builds walls/crates on a **flat floor**, and finishes post-map setup (~4–8s).
5. **Join RED or BLU**, pick **Scout** — you spawn on the arena floor (console: `tf_bm_build_id` should show **`phase28-hud-floor`**; green top HUD bar when client DLL is current).

Leaving OW for bomber clears hero state and kicks OW fill bots so OW and bomber do not run together.

**Requirements:** `ff_play` only works on a **listen host** or **dedicated server you control**. It does not work on Valve matchmaking / remote servers you do not admin. You must run **mod_tf**, not retail TF2.

After any map load, the server runs **post-map setup** automatically: OW/RIM kick bots and restart; **bomber** skips bot kick/restart and builds the arena when the round is running. **Bots use Valve stock AI** — add them yourself if you want (`tf_bot_add`, `tf_bot_quota`).

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

- **OW:** `koth_badlands` (optional: `tf_bot_quota 6` + `tf_bot_quota_mode fill`, or `tf_bot_add scout red` etc.)
- **RIM:** `plr_hightower` or other Payload maps with hostage setup

## Code

Mode checks live in `CTFGameRules::IsOverwatchMode()` / `IsRainbowIsMagicMode()` / `IsBombermanMode()` in `tf_gamerules.cpp`. Custom logic only runs for the active `tf_ff_game_mode` value.

**RIM fill bots:** after the round is running, use stock `tf_bot_add` / `tf_bot_quota` (no custom spawn queue).

**Bomber:** see `BOMBERMAN.md`. Humans only (`tf_bot_quota 0`). Stuck? `bm_fix` then `kill`. Debug grid: `tf_bm_show_grid 1`.
