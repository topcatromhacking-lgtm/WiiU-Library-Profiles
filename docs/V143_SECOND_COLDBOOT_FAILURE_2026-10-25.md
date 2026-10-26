# WULP cold boot regression persists after v1.3.12a

**2026-10-25 — owner hardware result**

The v1.3.12a 277-game profile **successfully loaded on the Wii U and survived launching a game and returning to the Menu**, but **did not survive a true powered-off cold boot**. The exact post-coldboot layout and Barista bytes for this second incident have **not** been captured yet. Do not claim reboot persistence.

## Distinct hypotheses, pending real SD evidence

1. **Save mutation at cold boot.** The single-byte opaque metadata drift tolerated by `wulp_v142_working_gate.h` may be insufficient if a cold boot changes multiple opaque bytes, any protected root record, or a folder bank. Never widen this acceptance solely from a synthetic test.
2. **Cold-start initialization ordering.** Pinned upstream `homebrew_on_menu_plugin` initializes `sSDIsMounted` during `ON_APPLICATION_START`. Its `SAVEInit` hook returns without redirecting when that flag is false. In the v142 integration, the title filter flag is populated by validation inside `initSaveData()`; if SAVEInit skips or runs before SD init, a later title-list query can receive the native unfiltered library. A warm Menu return can have different SD readiness. This is an evidence-supported source hypothesis, **not yet demonstrated on the owner's hardware**.
3. Different account/SD module load state could also prevent the extended pointer from being used.

## Owner-private evidence request

A host-only read-only package `WULP-v1.3.12b-Failed-Cold-Boot-READ-ONLY-Evidence.zip` was built, consisting of `wulp143_coldboot_capture.py` and `CAPTURE_AFTER_FAILED_COLD_BOOT.cmd`. Run them from the **original v141d PC deployment directory** alongside `PRIVATE_RECOVERY_KEEP_ON_PC` with the console fully powered off and the SD attached to PC.

The script copies the current live legacy Barista, golden, working, WLPA, marker, immutable legacy reference and active HOMP WUPS to *new* owner-private evidence files on PC; reads source SD a second time to detect changes during capture; emits sanitized `wulp143-coldboot-sanitized-report.json`, with no raw IDs, serial, save data, paths or console metadata. It **never writes to SD**.

The verdict discriminates `DISK_PROFILE_VALID_COLD_BOOT_INITIALIZATION_SUSPECT`, `WORKING_SAVE_CHANGED_OUTSIDE_V142_GATE`, `IMMUTABLE_REFERENCE_OR_POINTER_CHANGED` and `INSTALLED_WUPS_NOT_V142`. Host analysis with synthetic one-byte and two-byte metadata changes passed.

## Separate source development

`tools/build_v143_homp.py` adds two conservative cold-start improvements to pinned WULP142 source:

- It rechecks SDUtils mounted state at `SAVEInit` when the originally cached `sSDIsMounted` was false but SDUtils initialization is complete. The prior `SAVEInit` early return is preserved if the SD is genuinely unavailable.
- It logs Menu startup, first SD state, attach events, `SAVEInit` skip reasons and redirection initialization, without writing user data.

This is a **research-only build, not authorized for console installation** before the real postcold SD evidence is examined. Compilation/tests are in the [v143 workflow](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/workflows/wulp-v143-coldboot-trace.yml). Do not upload owner's private Barista/SD evidence to public GitHub.
