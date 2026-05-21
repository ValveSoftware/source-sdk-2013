# Frog Bomber (Bomberman in TF2)

## Quick start (join any game first)

You can start on OW, stock, or any map — then switch:

```
exec bomber_quickstart
```

Same as `ff_play bomber` (host/listen server only). Reloads **`itemtest`**, runs `mode_bomber.cfg`, builds the sky arena, then tells you to join RED/BLU + Scout.

Confirm the fix loaded: center message should include **`phase5-spawn-fix`**.

## Phase 3 — classic arena (try it now)

1. Rebuild server + client (`build_mod_tf.bat` or VS Release x64).
2. Quit game, deploy DLLs.
3. Console (from any listen-server game):
   ```
   ff_play bomber
   ```
   or `exec bomber_quickstart`
   Default map: **`itemtest`** (sky arena). Fallback: `ff_play bomber koth_badlands`
4. Pick a team. You should get:
   - **Top-down camera**
   - **Scout** with no weapons
   - **WASD** = 4-direction grid movement
   - **MOUSE1 (fire)** = place a bomb on your current cell
   - Bombs **fuse** then explode in a **+** shape; blast stops at walls
   - Getting hit **kills** you; you **respawn** after a short delay
   - **Classic arena**: hard border walls + pillar blocks + soft crates (see `maps/BM_ARENA_MAP.md`)
   - **Cyan grid lines** on the floor (toggle `tf_bm_show_grid 0`)
   - Full **map geometry** visible (roof clip is off by default — old clip showed only skybox)

## Controls

| Input | Action |
|-------|--------|
| WASD | Move one axis at a time (grid snap) |
| MOUSE1 | Place bomb (if you have a slot free) |
| — | Bombs chain-detonate when caught in another blast |

## Tuning

| ConVar | Default | Purpose |
|--------|---------|---------|
| `tf_bm_grid_origin` | auto | Corner of cell (0,0); set manually with `getpos` if needed |
| `tf_bm_cell_size` | `64` | Grid cell size |
| `tf_bm_move_speed` | `320` | Move speed |
| `tf_bm_bomb_fuse` | `2.5` | Seconds until explosion |
| `tf_bm_bomb_range` | `2` | Blast arms length in cells |
| `tf_bm_max_bombs` | `1` | Active bombs per player |
| `tf_bm_respawn_time` | `2` | Respawn wave after death |
| `tf_bm_cam_height` | `900` | Camera height (client) |
| `tf_bm_clip_world` | `0` | Experimental roof clip (leave off) |
| `tf_bm_show_grid` | `1` | Cyan floor grid for reading the arena |
| `tf_bm_show_grid` | `1` | Draw floor grid lines |
| `tf_bm_arena_width` | `15` | Arena cells wide (includes border) |
| `tf_bm_arena_height` | `13` | Arena cells tall |
| `tf_bm_arena_soft_fill` | `0.55` | Chance of soft crate in empty cells |

Stand where you want the grid anchor, note coordinates (`getpos`), set  
`tf_bm_grid_origin "X Y Z"` then `ff_restart`.

## Roadmap

- **Phase 0:** Top-down grid movement — done
- **Phase 1:** Place bomb + cross explosion + death — done
- **Phase 2:** Breakable crates + grid overlay — done
- **Phase 3:** Classic arena (hard border + pillars + soft fill) — done
- **Phase 4:** Dedicated `bm_arena` Hammer map, power-ups, scoring
- **Phase 3:** Power-ups, chain reactions (partial: chain detonate works)
- **Phase 4:** Rounds, scoring, juice

## Mode switch

`ff_mode bomber` — mode `3`  
`ff_play bomber` — cfg + default map  
`ff_mode ow` / `rim` / `stock` — other Frog Fortress modes
