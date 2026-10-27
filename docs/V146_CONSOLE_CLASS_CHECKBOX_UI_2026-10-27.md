# WULP v1.3.16 — native console-class selector UI

## Requirement correction, October 27, 2026

The owner explicitly clarified that the selection should be **per console/platform class, not per individual game**. Each class gets one checkbox in the Wii U plugin's own WUPS/Aroma configuration interface:

| Checkbox | Existing games | Initial setting |
| -- | --: | -- |
| Wii U | 140 | Enabled |
| Wii | 32 | Enabled |
| Nintendo DS | 31 | Enabled |
| Game Boy Advance | 74 | Enabled |
| Total | **277** | All enabled |

15 protected Homebrew/system icons are not subject to these toggles. The original141/extended277 on-console switching validated on real hardware in v1.3.15a must remain intact while developing this UI. The previously identified one missing Wii U title can be reconciled separately.

## Initial native UI implementation

`tools/build_v146_class_ui.py` patches pinned upstream Homebrew-on-Menu `c3b6d06301ba97f479da6227c8dd95bbda01d29f` after the working `tools/build_v145_homp.py` integration:

- Registers the **WULP - Console Classes** category in the existing WUPS plugin configuration menu.
- Four native `WUPSConfigItemBoolean` entries with platform name and expected title count. This is a real WUPS configuration screen, not a PC mockup, web visualization, or game-per-row UI.
- All four default to true, matching the 277-title selection.
- Each checkbox stores its own `wulp_ui_...` value through the plugin's existing WUPS storage and SaveStorage mechanism.
- Existing v145 dynamic profile selection, v144 bounded drift, MCP title-list filter, Menu launch/reload and Barista account save paths remain untouched. No SD save or marker writes are performed when toggling options.
- This initial source/UI milestone **does not apply the checkbox values to the active Wii U Menu game list**. Connecting class masks to title discovery and icon layouts is the next implementation layer, not a claimed current result.

## Next stage

Build a title-ID-to-class map for the exact 277 allowed titles. Compose the enabled console classes into a selection mask, recompute the filtered title list while retaining the 15 protected icons, and integrate a coherent Menu reload/profile save strategy. The UI should ultimately show current selected game count and apply changes via a confirmed action.

**SD default root: `E:/`** for any PC deployment helpers, user can manually change when necessary. Do not publish private save backups, serials, full owner-specific inventory or title lists into the public repository. Preserve a byte-exact v1.3.15a rollback before any new on-device beta.

## Test scope

The automated WUPS native compile and source checks verify the UI and that existing selection hooks remain untouched. Actual Wii U UI navigation and appearance require a hardware test; UI-driven game filtering is explicitly not implemented or claimed for this version.

## Compilation outcome (2026-10-27)

**SUCCESS** — [pinned Wii U native plugin CI](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/40565050946), compiled executable `homebrew_on_menu-WULP-v146-CLASS-UI.wps`, **249,139 bytes**, SHA-256 `aa74beed789aa765729c5b7da4f781f0a04a507e2ac61ae27b781f281e04320f`. This is a **code-only experimental build**; it has not been installed or navigated on owner hardware, and a guarded replacement of v145a is not included in the code-only artifact. The four persistent UI toggles are implemented and the 141/277 runtime filter has not been changed. Source patch `tools/build_v146_class_ui.py` and native compile passed.
