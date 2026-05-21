# Frog Bomber (Bomberman in TF2)

## Phase 0 — try it now

1. Rebuild server + client (`build_mod_tf.bat` or VS Release x64).
2. Quit game, deploy DLLs.
3. Console:
   ```
   ff_mode bomber
   changelevel ctf_2fort
   ```
   Or: `exec mode_bomber.cfg` then `changelevel ctf_2fort`
4. Pick a team. You should get:
   - **Top-down camera**
   - **Scout** with no weapons
   - **WASD** = 4-direction grid movement (snaps to 64-unit cells)

## Tuning

| ConVar | Default | Purpose |
|--------|---------|---------|
| `tf_bm_grid_origin` | `0 0 0` | Corner of cell (0,0) on the map |
| `tf_bm_cell_size` | `64` | Grid cell size |
| `tf_bm_move_speed` | `320` | Move speed |
| `tf_bm_cam_height` | `720` | Camera height (client) |

Stand where you want the grid center, note coordinates (`getpos`), set  
`tf_bm_grid_origin "X Y Z"` then respawn.

## Roadmap

- **Phase 1:** Place bomb + cross explosion + death
- **Phase 2:** Breakable crates, `bm_arena` map
- **Phase 3:** Power-ups, chain reactions
- **Phase 4:** Rounds, scoring, juice

## Mode switch

`ff_mode bomber` — mode `3`  
`ff_mode ow` / `rim` / `stock` — other Frog Fortress modes
