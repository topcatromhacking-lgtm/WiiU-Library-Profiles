# WULP v1.3.10 — immutable reference and writable working profile

**Date:** 2026-10-21

## Verified inputs

- v1.3.5 physical Wii U: verified full 130116-byte redirected Barista Menu read.
- v1.3.9 physical Wii U: 277/277 selected games observed in native enumeration; 3 scan events; peak native title count 660. This is *enumeration evidence*, not icon rendering.
- Historical 277-game private preview, 511-game inventory and audit recovered from private TopCat GitHub archive and cross-reconciled. They remain private.

## Engineering delivery

- New portable guard: `src/wulp_v140_working_gate.h`.
- New pinned Homebrew-on-Menu source patcher: `tools/build_v140_homp.py`.
- Native C++ regression: `tests/test_v140_working.cpp`.
- CI [build **PASS**](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/39687608333).
- Compiled experimental Wii U PowerPC `.wps`: 240705 bytes, SHA-256 `1e495313f7db2092cfff20c0bb8813daa1fcb718d63bbb516dc1960e6472546f`.

## What changes from v1.3.8

A valid profile has three distinct data objects:

1. The existing **legacy** Homebrew-on-Menu Barista save is not overwritten.
2. The **golden candidate** is immutable at `sd:/wiiu/wulp/profiles/extended/reference/BaristaAccountSaveFile.dat`. Its SHA-256 and paired WLPA are verified against the selector at startup by the existing v1.3.8 gate.
3. The **working candidate** at `sd:/wiiu/wulp/profiles/extended/save/80000001/BaristaAccountSaveFile.dat` is the only profile Barista path given to Menu redirection. It may legitimately change while Menu runs.

On subsequent startup, the working candidate is compared against golden for exact protected first 315 record bytes, type/flag records, opaque metadata and all 60 folder banks. It may reorder the *same* 277 selected USB-root game IDs within slots 315–599 with eight vacancies; no extra/missing/duplicate IDs are allowed. The immutable WLPA still filters the matching 284 IDs. Any mismatch falls back to the existing legacy redirect **without overwriting either save**.

This is a deliberately restricted persistence experiment: saving a new folder placement or other metadata changes currently causes a fail-closed legacy fallback, but the working file is retained for investigation.

## Important limitations

**NOT a hardware-approved profile installation.** The compiled WUPS plugin has not yet been tested on console. A current owner's Barista and current active allowlist, an independent verified second backup, and an integrated profile-pair staging/rollback process are still required to prepare valid console-specific golden and working saves. The beta must not activate any profile just from the public code.

FAT32 persistence and interrupted-write recovery have not been proven. The game Title IDs are observed as available in hardware enumeration, but 277 actual rendered icons and game launch/return are not yet verified.

No additional PC-only user verification task is needed; test scripts run in GitHub CI. The remaining necessary owner activity will be to supply or locally stage console-specific originals once the reversible profile package is ready.
