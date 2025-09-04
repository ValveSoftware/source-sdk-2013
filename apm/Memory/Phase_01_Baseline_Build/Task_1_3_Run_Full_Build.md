# Task 1.3 - Run Full Build (TF-only, Linux64)

- Status: FAIL
- Date: 2025-09-04

## Summary
- Objective: Generate TF-only Ninja graph (Release) and run full build; record results.
- Result: Blocked at graph generation. `devtools/bin/vpc` segfaults when invoked with TF/Linux64/Ninja flags, preventing build.

## Output
- Branch: `dev/bot/overhaul`
- Tree: stashed local changes to clean state
- Tools: `ninja 1.11.1`; `podman 4.9.3`; `vpc (Build: Feb 05 2025)`
- Command attempted:
  - `cd src && ./devtools/bin/vpc /tf /linux64 /ninja /define:SOURCESDK +everything /mksln _vpc_/ninja/tf_release`
- Error observed:
  - `Segmentation fault (core dumped)` (see `.vpc_tf_release.out` at repo root)
- Validation:
  - Graph: `src/_vpc_/ninja/tf_release.ninja` MISSING
  - Artifacts: `game/mod_tf/bin/linux64/client.so` MISSING; `game/mod_tf/bin/linux64/server_srv` MISSING
  - Build log: `src/../.build_tf_release.out` MISSING (build not started)
  - Compile DB: `src/compile_commands.json` NOT GENERATED

## Issues
- `devtools/bin/vpc` crashes on solution generation with TF/Linux64/Ninja settings. Help output works, but `/mksln` with targets triggers a segfault.
- Likely environment/toolchain mismatch for provided Linux `vpc` binary or VPC data in this branch.

## Next
- Re-run VPC inside the validated container toolchain from Task 1.2, then reattempt Ninja generation and build.
- If crash persists in container, capture minimal repro (exact args) and investigate known VPC issues for this branch; consider pinning to a known-good VPC binary.
- Once Ninja graph is generated, proceed with compile DB generation and full TF-only build; update findings.

