# WULP v1.3.15 — two independently filtered profiles

**2026-10-25. Software built and owner-private switching drill passed. Hardware switching NOT YET TESTED.**

## Starting point

The Wii U owner confirmed that **v1.3.14 retains exactly 277 selected games and icon order after THREE full cold boots**, and after a game launch and Menu return. This remains the established hardware-proven baseline. The new switching release must **not overwrite** either existing 277 Barista save, its immutable golden or WLPA.

## Architecture

This release adds a second isolated `original141` profile to the pinned Homebrew-on-Menu v144 base rather than activating the old mutable/unfiltered `legacy` fallback.

| Property | Existing extended277 | New original141 |
| --- | --- | --- |
| Immutable reference | `wiiu/wulp/profiles/extended/reference/BaristaAccountSaveFile.dat` | `wiiu/wulp/profiles/original141/reference/BaristaAccountSaveFile.dat` |
| Writable Barista | `wiiu/wulp/profiles/extended/save/80000001/BaristaAccountSaveFile.dat` | `wiiu/wulp/profiles/original141/save/80000001/BaristaAccountSaveFile.dat` |
| Paired WLPA | Existing, 284 entries | New, 148 entries |
| Selected games | 277, plus seven protected synthetic HB titles | 141, plus the same seven protected synthetic HB titles |
| Selector | Existing v144 extended marker unchanged | `WULP145:1\nprofile=original141\nbarista_sha256=...\nwlpa_sha256=...\n` |
| Game validator | Exactly v144 bounded metadata gate | Independent v145 bounded-metadata validator, preserving first 315 slots and all 60 folder banks |

The additional eight protected system icons remain outside the filter; both profiles preserve all 15 first-page protected icons.

`src/wulp_v145_dual_gate.h` independently verifies the original141 Barista and its signed paired WLPA; the `gWulp145AllowedCount` permits the existing integrated HOMP MCP filter to use either a 284-entry or 148-entry sorted allowlist. The v144 extended277 selector remains unchanged.

**Known limitation:** on invalid private pair, the integrated plugin may fall back to the underlying legacy save. The owner-private controller refuses unexpected markers and pair changes, but this is not a transactional Menu kernel filesystem.

## Code and compilation

- `tools/build_v145_homp.py` patches pinned upstream HOMP `c3b6d06301ba97f479da6227c8dd95bbda01d29f` after applying the proven v144 integration.
- `tests/test_v145_dual_gate.cpp` checks a valid paired original141 golden and writable save, metadata drift, protected icon/flag/folder mutations, missing and duplicate game IDs, malformed marker, and tampered paired WLPA.
- [GitHub Actions build](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/40269127019): **SUCCESS**, Wii U executable **245479 bytes**, SHA-256 `3472722a19caf2a1fc70bb4e5ee38ed7dd9bbd917497570cafebf1ea53652b60`.

## Owner-private switching package

`WULP-v1.3.15-Dual-Profile-Guarded-Switch-PRIVATE.zip` includes the compiled HOMP WUPS, Windows `.cmd` launchers and a standalone host switch controller `wulp145_switch.py`. **No owner-private Barista or game list is included.** Install into the user's original v141d PC deployment directory beside `wulp141_deploy.py`, `tools/wulp_v136.py`, three already-owned private inventory CSVs and `PRIVATE_RECOVERY_KEEP_ON_PC`.

The host's `stage` action requires the exact v144 plugin, exact archived pretest source, verified 277 golden/WLPA with 277 distinct games and bounded metadata drift. It saves the current v144 plugin and selector marker on PC first, adds new independent original141 paired reference/working save/WLPA **without overwriting any existing Barista**, then replaces only the WUPS plugin. **It keeps extended277 selected** to permit an immediate v144-to-v145 regression hardware boot.

The host's `to141` and `to277` actions, with console completely off, independently re-validate **both** profiles and their working saves. They switch only the small profile marker. `rollback` restores the exact hardware-proven v144 plugin and extended277 marker without overwriting either working save. Unknown state refuses.

**Default SD root in all Windows scripts: `E:/`**, user-editable via `SD_ROOT`. The host CLI also accepts `--sd`.

## Offline test and remaining hardware gates

Full PC drill using the user's previously verified private 141-game Barista structure, historical catalog, generated 277 golden/WLPA, v144/v145 compiled binaries, and synthetic removable-SD staging: **PASS**. It covered stage on extended277; switch to141; duplicate switch refused; switch to277; tampered USB record flags refused; switch to141; restore v144 plugin and original marker. Neither underlying Barista was changed.

Next real hardware test is: **stage v145 while keeping 277 selected, boot/cold boot and ensure icon order unchanged, power off and select 141, boot/cold boot, power off and return to 277, verify original order**. Record each state. Do not claim either switch actually works until the owner reports it.

Long-term upgrades still required: safe in-Menu hot switching, support for intentionally moved folders, a semantic map of mutable opaque metadata, and profile presets for additional console systems.
