[2025-09-04] Task 1.2 — Validate container toolchain and generate Ninja solution

Outcome
- Podman available locally; Sniper SDK image runs (smoke test printed "ok").
- VPC runs on host for Makefile generator (validated by generating `src/_vpc_/mk/test.mak`).
- VPC Ninja generator segfaults on both host and inside Sniper SDK container; Ninja graph not produced.
- Created output dirs `src/_vpc_/ninja` and `src/_vpc_/mk` for consistency.

Notes
- Host env: `podman 4.9.3`, `ninja 1.11.1` present.
- Container: `registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest` pulled and ran successfully.
- `src/devtools/bin/vpc` exists and is executable (Build: Feb  5 2025 02:20:23).
- VPC with `/ninja` crashes early even on minimal targets (e.g., `+tier1`), while Makefile generator succeeds.
- Container run emitted: "WARNING: Duplicate $CustomBuildStep For 'proto' - Ignoring." immediately before VPC segfault.

Commands Run
- `command -v podman && podman --version`
- `podman run --rm registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest echo ok`
- `ls -la src/devtools/bin && chmod +x src/devtools/bin/vpc`
- `mkdir -p src/_vpc_/ninja src/_vpc_/mk`
- Container VPC (attempt):
  - `cd src && podman run --rm --userns=keep-id ... /bin/bash -lc "cd /my_mod/src && devtools/bin/vpc /hl2mp /tf /linux64 /ninja /define:SOURCESDK +everything /mksln _vpc_/ninja/sdk_everything_release && ninja -f _vpc_/ninja/sdk_everything_release.ninja -t compdb > compile_commands.json && sed -i 's/-fpredictive-commoning//g; s/-fvar-tracking-assignments//g' compile_commands.json"`
  - Result: Segmentation fault during VPC Ninja generation.
- Host VPC (attempts):
  - `cd src && ./devtools/bin/vpc /hl2mp /tf /linux64 /ninja /define:SOURCESDK +everything /mksln _vpc_/ninja/sdk_everything_release` → Segmentation fault (exit 139).
  - `cd src && ./devtools/bin/vpc +tier1 /linux64 /ninja /define:SOURCESDK /mksln _vpc_/ninja/test_tier1` → Segmentation fault (exit 139).
  - `cd src && ./devtools/bin/vpc +tier1 /linux64 /define:SOURCESDK /mksln _vpc_/mk/test` → Succeeded (Makefile path).
- Validation checks:
  - `test -f src/_vpc_/ninja/sdk_everything_release.ninja` → missing.
  - `ls -la src/_vpc_/mk/test.mak` → exists.

Status
- Container toolchain validated (podman + image OK).
- Blocker: VPC Ninja generator crashes on both host and container; cannot produce `_vpc_/ninja/sdk_everything_release.ninja` or `compile_commands.json` in this state.
- Partial success: Verified VPC functionality with Makefile generator; created `src/_vpc_/mk/test.mak` to confirm parser/generator viability.

Follow-ups / Next Steps
- Acquire a known-good VPC build where `/ninja` is stable, or patch/replace `src/devtools/bin/vpc` (current build: 2025-02-05) to fix Ninja generator segfault.
- As a temporary workaround, proceed with Makefile generator for Linux64, or generate compile database via alternate tooling (e.g., intercept build with `bear`/`compiledb`) if compilation is permitted in a later task.
- Re-run Ninja generation once a fixed VPC is available; then generate and clean `compile_commands.json`.

Additional Action (per README)
- Operator executed `src/buildallprojects release` as the recommended path. This script runs inside the Sniper SDK container, generates the Ninja graph, and compiles.
- Artifacts verified post-run:
  - `src/_vpc_/ninja/sdk_everything_release.ninja` present (header OK).
  - `src/compile_commands.json` present (19,412 lines; flags already cleaned by script).
  - Build log saved at `.buildallprojects.out`.
- Note: This exceeds the “no compilation” scope for Task 1.2 but is recorded here for traceability and to document the working, README-aligned flow.
