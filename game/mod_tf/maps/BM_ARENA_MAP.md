# Frog Bomber — real map (no sky hack)

The old **sky layer** (`tf_bm_sky_arena 1`) floated an arena above stock maps and caused spawn/camera bugs. **Default is now ground play.**

## Play now (no compile)

```
ff_play bomber
```

- Tries **`bm_arena`** if `game/mod_tf/maps/bm_arena.bsp` exists.
- Otherwise **`itemtest`** with a **solid code-built floor** aligned to team spawns.

Build tag: **`phase28-hud-floor`** (green top HUD bar when client DLL is current).

## Proper map (recommended)

1. Open `game/mod_tf/mapsrc/bm_arena.vmf` in Hammer (Source SDK 2013 Multiplayer).
2. Run `game/mod_tf/mapsrc/compile_bm_arena.bat` (or `compile_bm_arena.ps1`).
3. Confirm `game/mod_tf/maps/bm_arena.bsp` exists.
4. `ff_play bomber` — flat BSP with RED/BLU spawns; mod still spawns walls/crates on the grid.

Hammer **GameDir** must be `game/mod_tf` (folder with `gameinfo.txt`). **BSPDir** must be `game/mod_tf/maps` (folder must exist).

Map size: **2048×1536** floor, sized for a **15×13** cell arena at **64** units per cell.

## What the mod still builds each round

- Border walls + soft crates (server entities, invisible models — collision only)
- Invisible **solid play platform** (`tf_bm_floor`)
- Visible **deck markers** (`prop_dynamic_override` when `tf_bm_deck_visible 1`)
- **HUD minimap** (client `CHudBomberArena` — not debug overlays)

## Legacy sky mode

Only if you really want the old behavior:

```
tf_bm_sky_arena 1
ff_play bomber itemtest
```

## Other flat stock maps

```
ff_play bomber koth_badlands
```

Arena uses offset platform beside spawns when not on `bm_arena` (`tf_bm_arena_offset`, `tf_bm_platform_height`).
