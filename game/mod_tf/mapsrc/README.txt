Frog Bomber — compile bm_arena.bsp
==================================

This folder contains the Hammer source for a flat Bomberman-only map.

Requirements
------------
- Team Fortress 2 installed (for textures/FGD)
- Source SDK Base 2013 Multiplayer (Hammer + vbsp/vvis/lightrad)

Steps
-----
1. Open Hammer from Source SDK 2013.
2. File → Open → bm_arena.vmf (this folder).
3. File → Map → Map Properties → set game/mod_tf as the mod if prompted.
4. Run F9 (Run Map Normal) OR compile from a shell:

   compile_bm_arena.bat
   powershell -File compile_bm_arena.ps1

5. Copy the output BSP to:

   game/mod_tf/maps/bm_arena.bsp

6. In-game (listen host):

   ff_play bomber

Until bm_arena.bsp exists, ff_play bomber falls back to itemtest with a code-built floor.

Map layout
----------
- Flat floor 2048×1536 units (fits 15×13 cells @ 64 units)
- RED / BLU team spawns at arena corners
- Arena props (walls/crates) are still spawned by the mod on round start

Legacy sky mode (not recommended): tf_bm_sky_arena 1
