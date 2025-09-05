APM Test Configs
=================

- Purpose: keep APM test/overlay cfg files organized under one folder.
- Usage examples:
  - exec in console or launch args: `+exec apm/apm_t5_4_test.cfg`
  - headless: `HEADLESS_EXTRA_ARGS='+exec apm/apm_t5_4_test.cfg' scripts/headless_test.sh ...`

Files:
- `apm_t5_3_attack.cfg`: enables T5.3 attack burst overlay.
- `apm_t5_4_test.cfg`: config for T5.4 weapon prioritization testing.
- `_template_test.cfg`: starter template for new APM task tests.

Convention:
- Name files `apm_<task>_<slug>.cfg` and place them here.
- Keep command‑line short by favoring `+exec apm/<file>.cfg` over many inline `+cvar` args.
