# Frog Fortress OW — Hero Shooter Mode

## Quick start

1. Launch via `game\Deploy and Launch Frog Fortress.bat` or Steam (see `STEAM_NONSTEAM_GAME.txt`).
2. Default: OW mode, map `koth_badlands`, **5 bots per team** (enemy team gets **+1 bot** when you pick RED/BLU).
3. Pick RED or BLU → class menu shows **6 heroes** (Scout/Sniper/Medic/Heavy/Pyro/Engineer).
4. Abilities: **Q** slot3, **E** slot2, **Shift** slot1, **R** ultimate.
5. F2–F7 quick hero pick in spawn: `ow_hero skirmisher`, etc.

## Switching modes (choose plan later)

See **`MODE_SWITCH.md`** for full detail.

- `ff_play ow` / `ff_play rim` / `ff_play bomber` / `ff_play stock` — one command (cfg + map)
- `ff_mode ow` — sets mode + runs cfg (reload map or use `ff_play` for full switch)
- Canonical: `tf_ff_game_mode` — `0` stock, `1` OW, `2` RIM, `3` Frog Bomber

## ConVars

| ConVar | Default | Purpose |
|--------|---------|---------|
| `tf_ff_game_mode` | 1 | Mode switch: 0=stock, 1=OW, 2=RIM, 3=Bomber |
| `tf_ow_mode` | 1 | OW hero shooter (synced from `tf_ff_game_mode`) |
| `tf_rim_mode` | 0 | RIM hostage mode (synced when mode 2) |
| `tf_ow_bots_per_team` | 5 | Bots on each team at round start |
| `tf_ow_enemy_bonus_on_join` | 1 | Extra bots on the team you didn't pick |
| `tf_ow_respawn_time` | 6 | Respawn wave seconds |
| `tf_ow_hero_lock` | 1 | Hero change only in spawn room |

## Data files

- `scripts/ow_heroes.txt` — hero stats, weapons, abilities
- `scripts/ow_modes.txt` — map prefix → escort/control/assault

## Console

- `ow_roster` — list heroes
- `ow_add_bots` — top up bots to the 5v5 (+ bonus) targets

## Bots (OW mode)

OW uses **stock TF bot AI** (payload push, KOTH cap, CTF, etc.) — not seek-and-destroy wander. Bots spawn after map setup ends, staggered. RIM mode still uses custom bear-hunt behavior.
- `ow_hero <name>` — pick hero (spawn room only if locked)

## Archived RIM

`ff_mode rim` or `exec mode_rim.cfg`, then `changelevel plr_hightower` (or your RIM map).
