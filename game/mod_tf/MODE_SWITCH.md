# Frog Fortress — choose your plan later

**Frog Bomber** is the default at launch (`autoexec.cfg` → `mode_bomber.cfg`, `tf_ff_game_mode 3`). Overwatch-style heroes and Rainbow Is Magic (RIM) stay in the mod — switch anytime.

## Quick switch

| What you want | Console | Config |
|---------------|---------|--------|
| Frog Bomber (default) | `ff_mode bomber` | `exec mode_bomber.cfg` |
| OW heroes | `ff_mode ow` | `exec mode_ow.cfg` |
| RIM hostages | `ff_mode rim` | `exec mode_rim.cfg` |
| Stock mod_tf | `ff_mode stock` | `exec mode_stock.cfg` |

Run `ff_mode` with no args to see the current mode.

`ff_mode <name>` runs the matching `mode_*.cfg`. **`ff_mode bomber`** (and `ff_play bomber`) **changelevel** to **`itemtest`** when you are on a non-bomber map.

## One-command launch (recommended)

Use `ff_play` to apply mode, execute the right cfg, and switch map in one step:

Or load shortcuts once: `exec ff_aliases.cfg`, then `ff_bomber`, `ff_ow`, `ff_rim`, or `ff_stock`.

- `ff_play bomber` -> **`itemtest`** (code-built grid; default on fresh launch)
- `ff_play ow` -> defaults to `koth_badlands`
- `ff_play rim` -> defaults to `plr_hightower`
- `ff_play stock` -> defaults to `ctf_2fort`

### Join any game first, then switch mode

1. Launch **mod_tf** (Frog Fortress DLLs).
2. **Create server** or **join** any listen match — default mode is **bomber**; wrong map auto-loads **itemtest**.
3. To switch when you are **host** or admin:
   - `ff_play ow` / `ff_play rim` / `ff_play bomber`
4. Bomber on a stock map: server **changelevels** to **`itemtest`**, runs `mode_bomber.cfg`, builds the grid (~4–8s).
5. **Join a team**, pick **Scout** — grid spawn (`tf_bm_build_id` = **`reset-v20-bomber-default`**).

Leaving OW for bomber clears hero state and kicks OW fill bots so OW and bomber do not run together.

**Requirements:** `ff_play` only works on a **listen host** or **dedicated server you control**.

After any map load, the server runs **post-map setup** automatically. **Bomber** kicks bots, builds arena on **itemtest**; OW/RIM use their own setup.

Same map, no reload: `ff_restart`

Override map: `ff_play ow cp_foundry`, `ff_play rim plr_hightower`, etc.

## Canonical ConVar

`tf_ff_game_mode` (default **3**):

- `0` — stock mod_tf (OW and RIM off)
- `1` — Overwatch-style heroes (`tf_ow_mode` synced on)
- `2` — Rainbow Is Magic hostages (`tf_rim_mode` synced on)
- `3` — **Frog Bomber** (grid on itemtest; see `BOMBERMAN.md`)

Legacy `tf_ow_mode` / `tf_rim_mode` still work; the server keeps them mutually exclusive and updates `tf_ff_game_mode`.

## Suggested maps

- **Bomber:** `itemtest` (only supported arena map)
- **OW:** `koth_badlands` (optional bots via `tf_bot_quota` / `tf_bot_add`)
- **RIM:** `plr_hightower` or other Payload maps with hostage setup

## Code

Mode checks live in `CTFGameRules::IsOverwatchMode()` / `IsRainbowIsMagicMode()` / `IsBombermanMode()` in `tf_gamerules.cpp`. Custom logic only runs for the active `tf_ff_game_mode` value.

**Bomber:** see `BOMBERMAN.md`. Humans only (`tf_bot_quota 0`). Stuck? `bm_fix` then `kill`. Debug grid: `tf_bm_show_grid 1`.
