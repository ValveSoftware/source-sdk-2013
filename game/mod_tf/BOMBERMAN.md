# Frog Bomber (default mode — itemtest only)

**Frog Bomber** is the mod default (`autoexec.cfg` → `mode_bomber.cfg`, `tf_ff_game_mode 3`). Dedicated Hammer map work was removed; the arena is a code-spawned grid on **`itemtest`**.

## Quick start

Launch mod_tf (or `+map itemtest`) — autoexec applies bomber. Or explicitly:

```
ff_play bomber
```

1. Server loads **`itemtest`** (automatic if you were on another map).
2. Join a team, pick **Scout** (FFA: one player per corner slot).
3. **WASD** = grid move, **MOUSE1** = bomb.
4. Confirm build: `tf_bm_build_id` = **`reset-v21-free-roam`**. **Free walk** in the Hammer room (`tf_bm_grid_move 0`). Spawn log: **`BM spawn: ... (grid cell X,Y)`** at Z ~ **-127**.

## Arena pipeline (no stacked rebuilds)

1. Post-map setup → **exec `mode_bomber.cfg`** then **one** `BM_BuildArena`
2. **Spawn once** in `GetPlayerSpawnSpot` (no PostSpawnThink re-teleport)
3. FFA: spawns at grid corner cells (NE/NW/SW/SE by player slot)
4. `bm_fix`: force rebuild + respawn

## itemtest play room (from `getpos` corners)

| Corner | X | Y | Z |
|--------|---|---|---|
| NE | 1304.03 | -280.81 | -126.97 |
| NW | 2023.97 | -280.04 | -126.97 |
| SW | 2023.99 | -2535.97 | -126.97 |
| SE | 1304.03 | -2535.87 | -126.97 |

AABB **1304.03,-2535.97** → **2023.97,-280.04**, center **~1664,-1408**, floor **-135** (~feet -127). Grid auto-fits room (**11×35** @ 64 typical). After `bm_letgo`, **`kill`** or **`bm_lock`** returns you to grid spawn.

## Defaults (reset)

| Setting | Value | Why |
|--------|-------|-----|
| `tf_ff_game_mode` | `3` | Bomber is default (DLL + cfg) |
| `tf_bm_ffa` | `1` | Free-for-all; `mp_friendlyfire 1` |
| `tf_bm_grid_move` | `0` | Normal WASD in room (set `1` for classic grid steps) |
| `tf_bm_arena_lock` | `0` | No spawn warp when touching walls |
| `tf_bm_show_grid` | `0` | 3D debug grid (off in FP — avoids blinking blue bar; use `tf_bm_camera_mode 1`) |
| `tf_bm_draw_floor` | `0` | Blue floor tint (off in FP) |
| `tf_bm_camera_mode` | `0` | Normal first-person |
| `tf_bm_void_arena` | `0` | Arena on itemtest floor |
| `tf_bm_render_props` | `0` | Invisible wall collision (no NULL materials) |
| `tf_bm_bomb_scale` | `0.18` | Mini soldier statue bombs |

Stuck? `bm_fix` then `kill`. Switch to OW: `ff_ow`.

## Not in this reset

- `bm_arena.bsp` / Hammer compile pipeline
- Dedicated map anchors

## Roadmap

- Visible crate/wall props
- Scoring and power-ups
