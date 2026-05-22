# Frog Bomber (Bomberman in TF2)

## Quick start

```
ff_play bomber
```

Or from any listen game: `exec bomber_quickstart`

1. **Compile the map once** (recommended): `game/mod_tf/mapsrc/compile_bm_arena.bat` → `maps/bm_arena.bsp`
2. Join **RED** or **BLU**, pick **Scout** (spectators see the grid only — no body on the arena)
3. **WASD** = grid move, **MOUSE1** = bomb (no bat swing)
4. Confirm build: center HUD shows **`phase15-playable-map`**

## Humans only

Bomber mode sets `tf_bot_quota 0`. Valve bots are not wired for grid play — add humans on a listen server.

## Ground vs sky

| Setting | Behavior |
|--------|----------|
| `tf_bm_sky_arena 0` (default) | Flat floor on `bm_arena` or offset platform beside stock maps |
| `tf_bm_sky_arena 1` | Legacy floating arena (not recommended) |

## Controls

| Input | Action |
|-------|--------|
| WASD | Move one axis at a time (grid snap) |
| MOUSE1 | Place bomb |

## Tuning

| ConVar | Default | Purpose |
|--------|---------|---------|
| `tf_bm_grid_origin` | auto | Cell (0,0) corner (set by arena build) |
| `tf_bm_cell_size` | `64` | Grid cell size |
| `tf_bm_arena_width` / `height` | `15` / `13` | Arena cells (fits `bm_arena` map) |
| `tf_bm_show_grid` | `0` | Debug overlay (`1` = draw grid lines) |
| `tf_bm_client_snap` | `0` | Client origin fix (`0` = server warp only) |
| `tf_bm_arena_offset` | `2048 2048` | Stock maps only — XY offset from spawns |
| `tf_bm_platform_height` | `512` | Stock maps only — Z above spawn cluster |

Stuck or wrong layer? Console: `bm_fix` then `kill`.

See `maps/BM_ARENA_MAP.md` and `mapsrc/README.txt` for Hammer compile steps.

## Roadmap

- Power-ups, scoring — later
