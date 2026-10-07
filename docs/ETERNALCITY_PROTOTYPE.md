# Eternal City Prototype

## Status

The client-side source slice and mod metadata are in this worktree on the
Valve `singleplayer` source branch. Both Win32 Release DLLs now build with
the user's Visual Studio Community 2026 installation (MSVC 14.51.36231,
`v145`) using the experimental recipe below. A fresh client/server build
and both VPC CRC checks exited successfully. This is **runtime-unverified**
as a playable prototype: no loaded-map gameplay, camera/input behavior, or
resolution acceptance test has passed. The agent installed no components
and did not publish into the source mod or Steam sourcemods.

An isolated Release/Win32 build was attempted using the existing MSBuild
bundled with SQL Server Management Studio 22. It exited with code 1 and
MSB4019 because its `Microsoft\VC\v180\Microsoft.Cpp.Default.props` import
is missing. Compilation did not start and no DLL was produced. SSMS's
MSBuild alone is not a Visual C++ toolchain; the required `v120_xp`
compiler, C++ targets, and compatible SDK still need to be provided.

After the Community installation, a fresh VPC generation and isolated
Release/Win32 build were attempted with command-line overrides
`PlatformToolset=v145`, `WindowsTargetPlatformVersion=10.0`, and
`PostBuildEventUseInBuild=false`. Output/intermediate paths were
`build\ec-v145-probe\bin\` and `build\ec-v145-probe\obj\`, respectively.
Compilation now starts, but exits with code 1 at `particlemgr.h:123`
(C1083: missing `typeinfo.h`). The installed compiler provides `typeinfo`,
not the legacy `typeinfo.h`. The old custom assembly rule also reports
MSB8065: it expects `$(VCInstallDir)bin\ml.exe` and does not produce
`pointeroverride.obj`; modern MASM is under
`VC\Tools\MSVC\14.51.36231\bin\Hostx64\x86\ml.exe`.
That initial probe produced no DLL. Subsequent fixes use standard
`<typeinfo>`, Developer Command Prompt's `ml.exe`, an accessible VGUI
virtual base, and version-gated UCRT allocation signatures/CRT declarations.
The allocator changes follow the existing Valve `origin/master` migration
patterns without changing the engine allocator interface.

Actual compilation also exposed prototype defects that static text tests
missed: client health is `GetHealth()`, button constants are not in the
`vgui` namespace, cursor coordinates come from VGUI input and need
`ScreenToLocal`, camera input must be qualified as `::input`, and the
base chat member is private. These are corrected using existing SDK APIs.

The latest fresh successful build is in
`build\ec-v145-7793afd1362944de9ebd6e6d4f3b20cd\bin\`.
Legacy warnings were explicitly allowed and remain in `client.log` and
`server.log`; this is not a warning-clean production build. The link uses
Microsoft's `legacy_stdio_definitions.lib` plus the dynamic UCRT import
library, avoiding the duplicate `hypot` definition in the shipped old
`particles.lib`. No `/FORCE:MULTIPLE` or allocator-disabling workaround is
used. Cross-toolset/prebuilt-library and allocator runtime compatibility
remain unverified.

An initial bounded engine launch from worktree-local staging exited 0 and
produced a console log, including registration of the new HUD, but the map
failed to load because the SDK Base does not include
`maps/d1_trainstation_01.bsp`. The installed Half-Life 2 content contains
that map. A new staging helper mounts that directory read-only, without
copying VPKs. Subsequent launches exited 1 before producing a console log,
including a comparison launch of the previously working staging directory.
No Windows Application crash event or launcher stdout/stderr explained
the early exit. This startup failure is unresolved; it must not be
described as a successful map or gameplay smoke test. Logs are retained
under `build\ec-v145-probe\` and the staging directories. The missing HUD
layout/localization startup resources reported by the initial launch were
also added, but have not yet been confirmed by a successful repeat launch.

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
| Ctrl+E | Character panel, if E is unbound or bound to `+use` |
| I | Equipment panel, if I is unbound |
| J | Inventory panel, if J is unbound |
| Escape | Close the open panel |
| Enter | Not implemented; stock SDK chat is disabled in singleplayer |

E remains the normal HL2 Use action; the Character panel is Ctrl+E so context
interaction is not silently rebound.

The prototype registers four local client commands: `ec_character`,
`ec_equipment`, `ec_inventory`, and `ec_close`. The first three toggle
their panel (or switch from another prototype panel); `ec_close` only
closes it. They can be invoked from the console in a loaded game or
assigned with Source's normal `bind` command. No binds are written or
changed automatically. Existing bindings take precedence over fallback
shortcuts, including bindings on I/J; bare E remains `+use`.

For example, after checking your current binds, explicitly opt into:

```text
bind i ec_equipment
bind j ec_inventory
```

These commands replace bindings on those keys only when you enter them
yourself. Assign `ec_character` and `ec_close` to free keys of your choice.
The same shortcut handler is used in the game and in the focused modal,
so bound panel keys can switch tabs or close the current panel there too.
Use single-command binds; compound binds/aliases are not intercepted by
the modal shortcut handler. Commands warn instead of opening in the main
menu, demo playback, or while chat/GameUI/another app-modal owns focus.
Console invocation may require closing the console to interact with the
panel. All command/focus behavior remains runtime-unverified.

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

The unresolved early runtime startup failure prevents loaded-map DLL ABI
verification and real resolution/input tests. Isolated build and staging
are exercised, but not a playable runtime. The Python checks
inspect source contracts and calculated bounds; they do not execute the
C++ implementation.

There are also known scope gaps independent of the startup blocker:
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

### Exercised experimental VS2026 path

From the worktree root, with Python and VS2026 desktop C++ installed:

```powershell
python -B scripts\build_eternalcity.py --allow-legacy-warnings
```

The script detects VS with `vswhere`, requires `v145`, generates and
CRC-checks both projects, and builds into a new GUID-named directory under
`build`. It runs `vcvarsall.bat x64_x86` and MSBuild in the same process.
`PostBuildEventUseInBuild=false` prevents publication. It changes no global
environment, execution policy, installed tools, or Steam files.

The diagnostic compiler options are
`/source-charset:windows-1252 /Zc:sizedDealloc- /Zc:threadSafeInit- /WX-`.
The first preserves the SDK's legacy source encoding; the next two retain
legacy allocation/static-initialization conventions. `/WX-` is opt-in:
omit `--allow-legacy-warnings` to retain warnings-as-errors. The linker
options are `legacy_stdio_definitions.lib /NODEFAULTLIB:libucrt.lib ucrt.lib`.
This configuration no longer targets XP and is not a substitute for
runtime ABI and allocator verification.

Create separate staging from the script's reported build directory:

```powershell
python -B scripts\stage_eternalcity.py --build <reported-build-directory> --hl2-content "C:\Program Files (x86)\Steam\steamapps\common\Half-Life 2\hl2"
```

The helper requires both DLLs and the installed smoke-test map, creates
a new directory inside this worktree's `build` directory, and never
overwrites existing staging. The external HL2 search path is only written
to staged gameinfo, not repository metadata or Steam files. The staging
helper also removes the sample's writable `hl2` search-path designation,
keeping mod output directed at staging rather than the installed game.
Staging does
not constitute acceptance or installation. The installed SDK Base has
no loose `hl2_mod\bin\server.dll`; use the matching built server DLL.

### Original VS2013 path (not exercised here)

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
   exist. These overrides were used in the failed probes above, but a
   successful build has not validated the complete procedure. Confirm the
   DLL and intermediate files stay in those directories and that
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
generated project and `src/vpc_scripts/source_dll_win32_base.vpc`.
The original toolset remains unavailable; only the experimental modern
recipe above has produced both DLLs. VPC alone does not produce a DLL.

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
   Test all four `ec_*` commands, bind panel commands to spare keys, and
   verify same-panel toggling, cross-panel switching, and `ec_close`.
   Temporarily bind I/J to other commands and confirm the fallback does
   not swallow them; restore those binds afterward. Check held/repeated
   keys for double toggles and test rejection from demo/GameUI/foreign
   modal contexts. This does not validate Enter chat, which is still absent.
5. Click Equip and Use and confirm the mock equipment/medkit display
   changes without changing live health or server inventory.
6. Repeat at 1920x1080. These resolution checks and all runtime checks
   remain unverified until both DLLs run in a playable loaded map.

Use both matching DLLs from the isolated build. Do not assume compilation
proves a runnable mod; server compatibility, allocator behavior, and mounted
HL2 content must also pass the smoke test.

`tests\test_eternalcity_prototype.py` runs standard-library static contract
checks only. Passing it verifies source/project wiring and the modal panel's
calculated bounds, not compilation, Source ABI compatibility, or runtime
behavior.
