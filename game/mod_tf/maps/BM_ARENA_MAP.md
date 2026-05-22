# Frog Bomber — real map (no sky hack)

The old **sky layer** (`tf_bm_sky_arena 1`) floated an arena above stock maps and caused spawn/camera bugs. **Default is now ground play.**

## Play now (no compile)

```
ff_play bomber
```

- Tries **`bm_arena`** if `game/mod_tf/maps/bm_arena.bsp` exists.
- Otherwise **`itemtest`** with a **solid code-built floor** and optional offset platform beside the map.

Build tag: **`phase15-playable-map`**

## Proper map (recommended)

1. Open `game/mod_tf/mapsrc/bm_arena.vmf` in Hammer (Source SDK 2013).
2. Run `game/mod_tf/mapsrc/compile_bm_arena.bat` (or F9 in Hammer).
3. Confirm `game/mod_tf/maps/bm_arena.bsp` exists.
4. `ff_play bomber` — you spawn on the flat map with RED/BLU team spawns.

Map size: **2048×1536** floor, sized for a **15×13** cell arena at **64** units per cell.

## What the mod still builds

Each round the server spawns on the floor:

- Border walls + pillar blocks + soft crates
- Invisible **solid play platform** (`tf_bm_floor`) + visible deck props
- Optional debug grid overlay (`tf_bm_show_grid 1`)

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
