# Findings Log

- Use this file to record observations per build/run.
- Keep items short and actionable. Examples:
  - [YYYY-MM-DD] Build failed: missing VPC. Action: restore devtools/bin/vpc.
  - [YYYY-MM-DD] Engineer bots idle near spawn. Action: verify class action wiring.

## Entries

- [2025-09-04] Branch setup: SUCCESS — `dev/bot/overhaul` created from `origin/master`, upstream set.
- [2025-09-04] Toolchain validate: PARTIAL — Sniper SDK container OK; `vpc` `/ninja` segfaults outside script. `src/buildallprojects release` generated Ninja graph + compile DB but build failed linking HL2MP. Action: build TF-only targets to achieve clean baseline.
- [2025-09-04] Baseline build (TF-only): FAIL — Manual TF Ninja graph generation segfaulted in `vpc`. Action: reuse `sdk_everything_release.ninja` from build script and invoke `ninja` for TF-only targets (`game/mod_tf/bin/linux64/client.so`, `game/mod_tf/bin/linux64/server_srv`); investigate pinning a known-good `vpc`.
- [2025-09-04] Build policy: MUST use `src/buildallprojects` (containerized) for graph generation and builds; avoid invoking `devtools/bin/vpc /ninja` directly due to segfault in this branch.
- [2025-09-04] T3.1 placement heuristics (guarded): PARITY — seams OFF identical to baseline; seams ON compiles via `src/buildallprojects`. Hooks added (mid-range, light height bias, nest spacing) under `TF_BOT_ENGINEER_SEAMS`. Debug trace notes in `apm/Memory/Phase_03_Placement_Heuristics/Task_3_1_Mid_Range_Height_Bias_Spacing.md`.
- [2025-09-04] T3.3 tuning cvars (guarded): PARITY — exposed `tf_bot_engineer_seams_*` cvars for desired range, nest spacing, height bias weight, hint radius, hint bias weight. All guarded; seams OFF unchanged. See memory log for usage notes.

- [2025-09-04] Engineer placement instrumentation: ADDED — Guarded DevMsg traces for mid-range, height bias, and nest spacing in `src/game/server/tf/bot/behavior/engineer/tf_bot_engineer_move_to_build.cpp` (see Task 3.1 memory). Enable with compile-time `TF_BOT_ENGINEER_SEAMS=1` and runtime `developer 1`; capture via `con_logfile nbdebuglogs/nextbot.debug.txt`. Grep marker: "[TF-ENG seam]".

- [2025-09-04] NextBot debug logs: ADDED — Created `game/mod_tf/nbdebuglogs/` to organize captures:
  - Behavior/Path cases: `game/mod_tf/nbdebuglogs/valve-maps/pl_upward/case-all-engi-4/` (e.g., `nb.behavior.debug.txt`, `nb.path.debug.txt`)
  - Visual spam (look_at/vision): `game/mod_tf/nbdebuglogs/valve-maps/pl_upward/bot-visuals-debug/` (e.g., `nb.lookat.debug.txt`, `nb.vision.debug.txt`)
  - General notes/legend: `game/mod_tf/nbdebuglogs/notes.txt`, `game/mod_tf/nbdebuglogs/legend.md`
  - Tip: enable with `developer 1`, `nb_debug 1`, and `con_logfile <path>`; grep markers include gameplay actions (e.g., "EngineerMoveToBuild") and our seam traces "[TF-ENG seam]" when seams are ON.
  - nb_debug usage: valid types are `behavior`, `look_at`, `vision`, `path`, `locomotion`. Use `nb_debug <type>` to enable and `nb_debug` (no args) to disable. Invalid forms like `nb_debug 1` print `Invalid debug type 'x'`.

- [2025-09-04] Baseline build (TF-only, release): FAIL
- Env: ninja 1.11.1; podman 4.9.3; vpc build Feb 05 2025
- Graph: src/_vpc_/ninja/tf_release.ninja missing
- Artifacts: mod_tf client.so/server_srv missing
- Compile DB: src/compile_commands.json not generated
- Notes: VPC segfault on graph generation (`devtools/bin/vpc /tf /linux64 /ninja /define:SOURCESDK +everything /mksln _vpc_/ninja/tf_release`). See `.vpc_tf_release.out` for stderr.
