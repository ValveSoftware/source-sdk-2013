# Frog Bomber (Bomberman in TF2)

## Quick start

```
ff_play bomber
```

Or from any listen game: `exec bomber_quickstart`

1. **Compile the map once** (recommended): `game/mod_tf/mapsrc/compile_bm_arena.bat` → `game/mod_tf/maps/bm_arena.bsp`
2. Join **RED** or **BLU**, pick **Scout**
3. **WASD** = grid move, **MOUSE1** = bomb (no bat swing)
4. Confirm build: green top banner **`Frog Bomber phase28 — HUD OK`** and `tf_bm_build_id phase28-hud-floor`

## Humans only

Bomber mode sets `tf_bot_quota 0`. Valve bots are not wired for grid play — add humans on a listen server.

## Ground vs sky

| Setting | Behavior |
|--------|----------|
| `tf_bm_void_arena 0` (default) | Arena on map floor (`itemtest` or `bm_arena`) |
| `tf_bm_sky_arena 1` | Legacy floating arena (not recommended) |

## Controls

| Input | Action |
|-------|--------|
| WASD | Move one axis at a time (grid snap) |
| MOUSE1 | Place bomb |

## Tuning (after `mode_bomber.cfg`)

| ConVar | `mode_bomber.cfg` | Purpose |
|--------|-------------------|---------|
| `tf_bm_build_id` | `phase28-hud-floor` | DLL build tag |
| `tf_bm_camera_mode` | `1` | `0`=vanilla FP, `1`=top-down, `2`=ortho (experimental) |
| `tf_bm_cell_size` | `64` | Grid cell size |
| `tf_bm_arena_width` / `height` | `15` / `13` | Arena cells |
| `tf_bm_show_grid` | `0` | Debug overlay (`1` = NDebug lines; HUD minimap always on) |
| `tf_bm_deck_visible` | `1` | Server `prop_dynamic_override` deck markers |
| `tf_bm_render_props` | `0` | Wall/crate models off server (avoids NULL material) |
| `mat_fullbright` | `1` | Brighter 3D on itemtest until dedicated map is lit |

Stuck or wrong layer? Console: `bm_fix` then `kill`. Black 3D but HUD visible? `tf_bm_camera_mode 0`

See `maps/BM_ARENA_MAP.md` and `mapsrc/README.txt` for Hammer compile steps.

## Roadmap

- Power-ups, scoring — later
