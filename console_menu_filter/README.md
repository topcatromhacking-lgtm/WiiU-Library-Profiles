# v0.2 five-game Wii U Menu filtering experiment

**EXPERIMENTAL, NOT YET VERIFIED ON HARDWARE.** The plug-in is `.wps` (Aroma WUPS), **not** the existing .wuhb metadata probe. It **does not** increase the native Wii U Menu's 300-icon limit. It removes five entries from the in-memory result of `MCP_TitleList` in the Wii U Menu process only. It never removes titles from storage or touches NAND/USB files or saves. There is **no promise** that filtering the MCP list will change visible icons on stock Menu because Barista's persisted layout and other plugins can affect icon behavior.

## Target set (exact base-game Title IDs)

| Game | Title ID |
|---|---|
| 007 Legends | `000500001010E600` |
| 1001 Spikes | `0005000010176B00` |
| 8Bit Hero | `00050000101F2800` |
| Alice in Wonderland | `0005000010202500` |
| Alphadia Genesis | `000500001018FB00` |

The identifiers came from a real metadata scan, but the full private game catalog is not in this repository.

## Safety features

1. **Disabled by default**. Without an exact opt-in file, the hook calls the original function and returns the unaltered result.
2. The symbol replacement is scoped **only to Nintendo's Wii U Menu process** with `WUPS_FP_TARGET_PROCESS_WII_U_MENU`.
3. The filter matches complete 64-bit Title IDs **plus** `MCP_APP_TYPE_GAME` (`0x80000000`). Updates, DLC, OS titles, Wii VC and Homebrew-on-Menu synthetic IDs pass through.
4. If the returned array, count or byte capacity is suspicious, the plugin does not edit it. It never truncates the entire list to meet 300 or 280 entries.
5. The marker is sampled **once per Menu application session**, not changed in the middle of a Menu enumeration. Restart Aroma after adding or removing it.
6. Remove the marker and reboot to disable. For complete rollback remove the plugin `.wps` and reboot. The original working probes remain installed separately.

## Preparation: backing up the Menu

Before enabling, **back up the SD card** and the current Wii U Menu layout. In particular preserve the original `sd:/wiiu/homebrew_on_menu_plugin/` folder if present. The **Homebrew on Wii U Menu** plugin may redirect Barista Menu save data to `sd:/wiiu/homebrew_on_menu_plugin/[SerialNumber]/save`. Do not edit the redirected save or `titlesmap` files. Backup procedures and side effects of Barista with a shorter MCP list have **not yet been validated on this console**. If you cannot restore the original Menu layout from backup, do **not** enable the experimental filter.

## Stage A: inert-install sanity check (recommended)

1. Copy `wiiu_library_profiles_five_filter.wps` to `sd:/wiiu/environments/aroma/plugins/`.
2. Ensure `sd:/wiiu/library_profiles/enable-five-title-test.txt` **does not exist**.
3. Reboot fully into Aroma. Confirm the Wii U Menu and Homebrew Loader `L + R + D-Pad Down` still work. **No icons should change** at this stage.
4. If Menu startup fails, remove this `.wps` from SD using a PC/card reader, then reboot. Do not uninstall titles.

## Stage B: explicit five-game test

Only after backing up, passing Stage A, and understanding the risk:

1. Create folder `sd:/wiiu/library_profiles/`, if missing.
2. Copy the supplied `enable-five-title-test.txt` containing **exactly** `ENABLE-V0.2-FIVE-TITLE-TEST` followed by a **single LF newline** (no CRLF, BOM or extra content).
3. Reboot into Aroma, preferably cold boot, so Menu reevaluates the marker.
4. Check whether the five named games stop appearing while unrelated games and Homebrew access remain intact. Because your library exceeds the 300-icon limit, confirm these five were **actually visible** in the unfiltered Menu first. Otherwise this test cannot establish that the hook worked.
5. Do not use Data Management, delete/move games, reorganize folders, or change users until you have tested rollback.

## Stage C: rollback validation (mandatory)

1. Delete or rename `sd:/wiiu/library_profiles/enable-five-title-test.txt`.
2. **Fully reboot** into Aroma. Confirm all five icons return to their prior positions and your existing folders and Homebrew-on-Menu setup are intact.
3. If the Menu fails to load or icons persistently misbehave, shut down, remove `wiiu_library_profiles_five_filter.wps` directly from the SD card, then reboot. Restore backed-up redirected Menu saves only if needed, with the console off.
4. Preserve the original v0.1 and v0.1.1 JSON catalogs for before/after comparison.

## Compatibility research still open

[Homebrew on Wii U Menu](https://github.com/wiiu-env/homebrew_on_menu_plugin) also replaces `MCP_TitleList`, appending synthetic Homebrew icons. WUPS hook chaining order, possible title-count/capacity interactions, and Barista's persistence are **not established** by a successful compile or offline test. Do not treat this plugin as production-ready or use it to filter hundreds of entries yet. Keep branch and PR unmerged pending hardware confirmation. See [issue #3](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/issues/3).

## Reproduce build and predicate tests

```sh
gcc -std=c11 -Wall -Wextra -Werror -pedantic tests/filter_logic_test.c -o /tmp/wlp_five_filter_test
/tmp/wlp_five_filter_test

# With a Wii U wut+WUPS devkitPro SDK:
make -C console_menu_filter
# Or use the pinned reproducible Dockerfile (as in GitHub Actions).
```

GitHub CI compiles the WUPS plugin and uploads it as a build artifact with **no opt-in marker included**, so merely extracting the plugin will not activate the filter.
