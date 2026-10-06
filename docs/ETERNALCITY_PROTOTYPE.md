# Eternal City Prototype

## Status

The client-side source slice and mod metadata are in this worktree on the
Valve `singleplayer` source branch. It has **not been built**: this machine
has the Source SDK Base 2013 Singleplayer runtime (Steam app 243730) and the
repository's VPC generator, but no compatible Visual C++ compiler or
MSBuild installation. No executable, DLL, launch, or in-game UI result is
claimed. No SDK components were installed.
The targeted VPC client-project check completed successfully and included
the new HUD translation unit; that validates project wiring only, not
compilation or runtime compatibility.

The prototype uses the existing HL2 player, weapon, and mounted game content.
The camera and interface are client-only. Health and ammunition are read
from the live HL2 player and active weapon. Level, experience, credits,
equipment, and medkits are mock values held only in client memory; the
Equip and Use actions do not affect server gameplay and nothing is saved.

## Controls

| Input | Action |
| --- | --- |
| WASD, Shift, Space | Existing Source movement, sprint, and jump |
| Mouse and fire controls | Existing HL2 weapon input |
| E | Existing HL2 Use action; it is not rebound |
| Ctrl+E | Character panel |
| I | Equipment panel |
| J | Inventory panel |
| Escape | Close the open panel |
| Enter | Not implemented; stock SDK chat is disabled in singleplayer |

E remains the normal HL2 Use action; the Character panel is Ctrl+E so context
interaction is not silently rebound.

While a panel is open, the VGUI panel is app-modal, movement and action
commands are suppressed, and key releases still reach the engine to avoid
sticking held movement or attack inputs. The game simulation is not paused.
The minimap, map key, and other EternalCity keymap entries are not
implemented.

The HUD releases its modal focus when a level ends. Modal commands also
clear impulses and pending weapon selections, not only movement/buttons.
The elevated camera enters Source's third-person rendering state and
releases only the state it acquired when returning to an ordinary view.
All of this remains untested in the engine.

## Remaining acceptance gaps

The toolchain blocker prevents compilation, DLL ABI verification, launch,
safe runtime staging, and real resolution/input tests. The Python checks
inspect source contracts and calculated bounds; they do not execute the
C++ implementation.

There are also known scope gaps independent of the missing compiler:
Ctrl+E is a documented deviation from the requested bare-E Character
shortcut; modal UI suppresses player commands but does not pause the world;
Enter chat has no prototype implementation (the stock
`ClientModeShared::StartMessageMode` rejects `maxClients == 1`);
Equipment/Inventory currently show text and action buttons, not an item
grid. Fixed camera yaw does not remap movement or aiming to screen space:
mouse input retains ordinary HL2 view-angle semantics, so crosshair/aim
alignment must be assessed in the engine. These are not completed
acceptance criteria and must not be reported as playable features.

## Source changes

- `src/game/client/hl2/clientmode_hlnormal.cpp` applies the elevated fixed-yaw
  camera to live player views, traces a hull against solid geometry, and
  retains the engine view for dead, observer, and vehicle views.
- `src/game/client/hl2/eternalcity_hud.cpp` draws the live vitals/ammunition
  and mock progression strip and implements the modal panels as a registered
  `CHudElement`, respecting the standard HUD visibility lifecycle.
- `src/game/client/client_hl2.vpc` adds the HUD translation unit to the HL2
  client project.
- `game/mod_hl2/gameinfo.txt` identifies the mod and keeps app ID 243730.

## Build and stage

Do not install a compiler or SDK as part of this task. When a compatible
32-bit Windows C++ toolchain and MSBuild are available:

1. Open a Developer Command Prompt with Visual Studio 2013 C++ support
   (`v120_xp`, Win32). This exact toolchain is not installed here.
2. From `src\game\client`, regenerate the targeted client project:

   ```bat
   ..\..\devtools\bin\vpc.exe /checkfiles +client /hl2
   ```

   This was run successfully in this worktree: VPC reported Visual Studio
   2013 generation and produced `client_hl2.vcxproj` with both HUD files.
   The generated project targets `v120_xp`, `Win32`, and `client.dll`.
3. Do not build with the generated project defaults. Its post-build event
   copies `client.dll` and `client.pdb` into the source tree's
   `game\mod_hl2\bin`. Once the compatible toolchain is available, build
   with distinct, newly-created empty output and intermediate directories
   and disable that event, for example:

   ```bat
   msbuild client_hl2.vcxproj /p:Configuration=Release /p:Platform=Win32 /p:OutDir=<new-build-root>\bin /p:IntDir=<new-build-root>\obj /p:PostBuildEventUseInBuild=false
   ```

   Replace `<new-build-root>` with an absolute path that does not already
   exist. This override procedure has not been exercised; confirm the DLL
   and intermediate files stay in those directories and that
   `game\mod_hl2\bin` remains untouched before proceeding.
4. Keep the staged runtime separate from the source mod and build outputs.
   Copy `game\mod_hl2` to a new staging directory named
   `EternalCityPrototype`, then add the successfully built client DLL at
   `EternalCityPrototype\bin\client.dll`. Do not overwrite an existing
   staging directory. Do not copy Valve VPKs or other Valve game assets.
5. After the staged directory passes the runtime smoke test below, copy it
   to `C:\Program Files (x86)\Steam\steamapps\sourcemods\EternalCityPrototype`.
   If that target already exists, stop and preserve its contents.

The project settings and default publish event above were inspected in the
generated project and `src/vpc_scripts/source_dll_win32_base.vpc`. No
compiler build has been run; the VPC generator alone does not produce a game
DLL.

## Runtime smoke test

Only after a successful build and staging:

1. Start Source SDK Base 2013 Singleplayer with the staged directory as the
   game (`hl2.exe -game <absolute-staging-path> -console -windowed -w 1280
   -h 720`).
2. Load an installed HL2 map from the console, for example
   `map d1_trainstation_01`. Confirm the local player is visible and the
   camera follows from above.
3. Walk toward a wall and confirm the camera shortens its distance before
   clipping. Verify WASD, Shift, Space, weapon fire, and E Use.
4. Check health and active-weapon clip/reserve updates. Open Ctrl+E, I, and
   J; confirm Escape closes panels and no movement, attack, impulse, or
   weapon selection occurs while a panel is open. Change/disconnect the
   level with a panel open and confirm modal focus is released.
5. Click Equip and Use and confirm the mock equipment/medkit display
   changes without changing live health or server inventory.
6. Repeat at 1920x1080. These resolution checks and all runtime checks
   remain unverified until a DLL can be built and launched.

Confirm the installed SDK Base's `hl2_mod\bin` server DLL can be loaded
through the existing gameinfo search path. Do not assume a client-only
build proves a runnable mod; server compatibility and mounted HL2 content
must also pass the smoke test. If incompatible, build the matching HL2
server from this checkout with the same output/staging separation before
repeating the test.

`tests\test_eternalcity_prototype.py` runs standard-library static contract
checks only. Passing it verifies source/project wiring and the modal panel's
calculated bounds, not compilation, Source ABI compatibility, or runtime
behavior.
