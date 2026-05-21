# Frog Bomber — maps (flat arena)

## Workshop vs this mod

**Steam Workshop maps only work in retail Team Fortress 2**, not inside the Source SDK `mod_tf` project directly.

This mod already mounts TF2 content (`appid 440` in `gameinfo.txt`), so you can use:

- Stock TF maps: `itemtest`, `koth_badlands`, `ctf_2fort`, etc.
- Your own compiled `.bsp` in `game/mod_tf/maps/` (best long-term)
- Custom files in `game/mod_tf/custom/` (VPK or loose files)

There is **no Workshop “subscribe” button** for `mod_tf` — think “pick a flat TF map” or “compile `bm_arena.bsp`”.

## Recommended right now

```
ff_play bomber
```

Default map is `**itemtest**` (TF2’s flattest stock map). The arena is built on a **sky layer** (`tf_bm_sky_height`, default 3072 units above the map) so you never walk on hills or props — only the flat grid.

If `itemtest` fails to load:

```
ff_play bomber koth_badlands
```

You are **warped into the sky arena** on spawn (RED/BLU corners). Terrain below is only scenery.

## What the code builds on any map

On round start the server spawns a **15×13 Bomberman board** centered on team spawns:

- Hard border walls (solid)
- Fixed pillar walls (checkerboard)
- Soft crates (barrels you blow up)
- Cyan/red grid overlay on the client

## Compile your own flat map (`bm_arena`) — later

1. Install/source SDK Hammer (from TF2 SDK or `sourcesdk/content`).
2. New map → big floor brush (`tools/toolsskybox` or `dev/dev_measuregeneric01`).
3. Skybox, `light_environment`, `info_player_teamspawn` (RED + BLU).
4. Floor roughly **1024×832** units or larger (15×13 cells × 64 units).
5. FGD: `tf.fgd`, game config: `mod_tf`.
6. Run BSP compile → copy `bm_arena.bsp` to `game/mod_tf/maps/`.
7. Launch:

```
ff_play bomber bm_arena
```

## Flat maps to try (from installed TF2)


| Map             | Notes                                    |
| --------------- | ---------------------------------------- |
| `itemtest`      | Default for `ff_play bomber` — very flat |
| `koth_badlands` | Open KOTH yard                           |
| `ctf_2fort`     | Courtyard area works; lots of buildings  |


## Custom content folder (like a mini-workshop)

Drop a VPK or folder under:

`game/mod_tf/custom/`

See `gameinfo.txt` — that path is scanned at startup for extra maps/materials.