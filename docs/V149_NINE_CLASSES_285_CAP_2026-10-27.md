# WULP v1.3.19 — Nine console classes, original 300-icon limit

**Date:** October 27, 2026. Native Wii U build and offline stage/validation/rollback passed. **Hardware confirmation pending.**

## Requirements fulfilled

The owner requested that the platform filter remain a **single checkbox per console class**, with priority order:

1. Wii U — **141** games
2. Wii — **32**
3. Nintendo DS — **31**
4. Game Boy Advance — **74**
5. Nintendo 64 — **21**
6. Super Nintendo — **54**
7. NES — **95**
8. PC Engine — **40**
9. MSX — **23**

There are **511 distinct titles** in the verified historical inventory, including *The Wonderful 101*. Sort alphabetically **#–Z inside each console class**. Priority and display order follow the nine classes above, with PC Engine before MSX.

Keep the **300-icon architecture**: 15 fixed system/Homebrew icons + **at most 285 game icons**, preserving each class checkbox even if some games are hidden. The visible games are the *first* 285 enabled titles in console priority order. Example with the previous four classes plus N64 checked: **299 selected, 285 displayed, 14 hidden**; N64 shows **7/21**. With all nine: **511 selected, 285 displayed, 226 hidden**.

## Implementation

- Native Aroma configuration in [`build_v149_nine.py`](../tools/build_v149_nine.py): four existing WUPS booleans retain their stored settings and five new independent booleans start OFF. Native configuration gives class-level visible/total counts and a selected/displayed/hidden summary when opened; when over capacity, it displays **"Only 285 games will be shown"**. On closing the settings, it requests a notification with the current overflow count. Existing behavior applies updates on the normal Wii U Menu restart.
- [`wulp_v149_nine.h`](../src/wulp_v149_nine.h): bounded 511-entry, nine-bit class catalog; strict 512-mask selection; protected icon and Barista flag checks; sorted WLPA; per-mask manifest hash binding; 32-byte bounded Menu metadata tolerance.
- The private `wulp149_layout.py` generator uses owner-controlled historical inventory and the exact v1.3.18 immutable all-on Barista as its source. It creates **512** independent game layouts, each with its own immutable Barista, writable Barista, paired WLPA and manifest under `wiiu/wulp/profiles/nineclass/000` through `1FF`. Adds a new 511-class catalog and complete 511-title alphabetical order file; **does not replace** the existing 141, 277, v147 or v148 files.
- The native selector loads the matching saved profile and filters the MCP title list with the matching allowed IDs. If validation fails, it retains the earlier v1.3.18 alphabetical profile instead of applying unverified data. The maximum per-profile WLPA has 285 games plus seven synthetic protected Homebrew Title IDs; the other eight protected icons remain outside the allowlist.

## Tested

- [Wii U native CI](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/40626018416): **PASS**. Binary: `homebrew_on_menu-WULP-v149-NINE-CLASS.wps`, **268,069 bytes**, SHA-256 `46b30f6106aeb2d1b33d15927d75743d67e53bd92a44966e441ab13ad02ecb8d`.
- Historical source catalog cross-checked: **511 games**, nine class counts as above, no duplicate Title IDs. Sorted v1.3.18 278 titles remain the first four classes.
- Offline host test compiled C++ validator and verified **all 512** generated Barista/WLPA/manifest combinations: **PASS**.
- Mock SD powered-off installer tested **stage → check → corruption refused → rollback**: **PASS**. Exact v1.3.18 plugin restored and prior profiles unchanged.
- **Real Wii U hardware v1.3.19 is not yet tested.**

## Owner private package

**`WULP-v1.3.19-Nine-Classes-285-Game-Limit-Guarded-PRIVATE.zip`**

The package includes native compiled plugin, private PC generator and guarded stage/check/rollback scripts. It excludes private Barista backups, installed title lists, console identifiers and game content. Existing private PC recovery and historical inventory must be retained in their original locations.

1. Fully power off Wii U. Insert SD into PC. Extract private package beside the original WULP v141d PC deployment folder. SD default **`E:/`** is editable through the launcher or `WULP_SD_ROOT`.
2. Run `STAGE_NINE_CLASSES.cmd`. Expected status **`WULP149_STAGED_NINE_CLASSES_285_CAP_READY`**. The generator creates 512 isolated profiles, approximately 67 MB total.
3. Boot Wii U. The existing four classes should still show **278 alphabetical games**; the five new classes are unchecked initially.
4. Enable **N64** as fifth class. Expect 299 selected, 285 shown, 14 hidden and N64 7/21 displayed. Return N64 to unchecked: original 278, including *The Wonderful 101*.
5. Optionally enable all nine: expect 511 selected, 285 displayed, 226 hidden; protected icons remain.
6. For a powered-off read-only audit, run `CHECK_NINE_CLASSES_READONLY.cmd`. For exact plugin rollback, run `ROLLBACK_TO_V148.cmd` while the console is fully powered off.

Freeze **v1.3.18 as the hardware-proven baseline** until the owner confirms v1.3.19 boots and filtering persists.
