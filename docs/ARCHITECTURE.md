# Architecture, compatibility research and safety rules

## Why this project exists

Stock Wii U Menu uses a 300-visible-title cap, distinct from the number of installed titles. Huge libraries should be navigable by platform and origin (native, Virtual Console, injected game), using **one console user** and the stock Nintendo Menu.

## v0.1: read-only foundation

- `console_probe/`: experimental Aroma **homebrew application** (`.wuhb`, not a WUPS plugin) that reads `MCP_TitleCount` / `MCP_TitleList` and writes a local SD-root JSON catalog. It does not modify system storage, menus, profiles, installs, title records, or Barista layouts. **Not compiled or tested on hardware yet.**
- `wiiu_library_profiles/`: portable Python model, conservative classification, per-title overrides and read-only profile preview.
- `tests/`: synthetic cases including catalogs of >300 and 1,000 installed records, duplicate IDs across devices, profile-size overflow, system app protection and manual VC/injection classification. These are **not** hardware tests.

The catalog may include system titles, updates and DLC, some of which have no Menu icons. Conversely, Homebrew on Menu synthesizes titles inside its Menu-scoped MCP hook, so a scanner run as a separate homebrew app may not enumerate synthetic WUHB entries. Treat these counts as different until reconciled experimentally.

## v0.2 planned: Menu-only interception

[Homebrew on Wii U Menu](https://github.com/wiiu-env/homebrew_on_menu_plugin) patches `MCP_TitleList` with `WUPS_MUST_REPLACE_FOR_PROCESS(... WUPS_FP_TARGET_PROCESS_WII_U_MENU)` and *appends* synthetic records. We propose investigating a compatible, composable **filter** of the resulting in-memory list, never changing installed-title records or Data Management. Work out hook ordering and buffer-size / return-count semantics before shipping. Changing lists may not be sufficient: stock Barista layouts may retain stale saved records.

## v0.3 planned: profile switching

Aroma configuration chooses a platform/VC origin filter, stores selection to SD and reloads Menu only after a validated transition. Preserve a recovery profile and maintain access to Settings and Homebrew tools. If separate layouts turn out to be required, implement carefully versioned Barista snapshots on SD; never live-swap NAND saves. Homebrew on Menu redirects Barista save operations to SD when installed: don't bypass or clobber its live copy.

## Classification model

`platform`: `wiiu`, `wii`, `nes`, `snes`, `n64`, `nds`, `gba`, `gb`, `gbc`, `tg16`, `other`, `homebrew`, `system`, `unclassified`.

`origin`: `native`, `official_vc`, `injection`, `homebrew`, `system`, `unknown`.

Native/VC/injection **cannot reliably be distinguished from full Title ID alone**, particularly VC injections using donor identities. Treat all `00050000...` launchable entries as unclassified until explicit override or verified metadata identifies them. Wii-title MCP app type gives a provisional `wii` identification only. Allow the user to attach `tags` for Favorites, and explicitly select both platform AND origin when desired.

Profiles cap visible records at **280 by default** (engineering headroom, not a verified Nintendo slot count). Overflow produces an error; the software never silently truncates. System and injected Homebrew records cannot be excluded by the offline engine. Menu/Disc/folder count reconciliation is still a hardware research task.

## Relevant upstream projects

- [Menu Sort Aroma](https://github.com/Yirr777/menu_sort-aroma): grouping, per-user titlemaps, Barista backups. Titlemaps **sort**; they do not filter installed titles.
- [Homebrew on Wii U Menu](https://github.com/wiiu-env/homebrew_on_menu_plugin): Menu-scoped `MCP_TitleList` hook, synthetic entries and SD Barista save redirection. GPL-3.0 licensed; consider license compatibility before reusing source.
- [WiiU-300-Limit-Circumvention](https://github.com/haimbilia/WiiU-300-Limit-Circumvention): USA Menu v277 810-slot experimental patch; extends capacities but not required for a sub-300 title filter.
- [Launchiine](https://github.com/wiiu-env/launchiine): replacement Menu; not our target because keeping stock Nintendo Menu is a requirement.

## Safe testing ladder

1. Verify probe builds for installed devkitPro/WUT toolchain and exports JSON of full installed inventory (including >300).
2. Offline mark five safe, known game Title IDs for hide/show; validate profile preview.
3. Hook only the Wii U Menu, never global MCP services. Confirm original + Homebrew-on-Menu plugin coexistence and hook order.
4. Verify five titles disappear; no deleted games; restoring the other profile brings them back.
5. Verify folders/icons across Menu restart, user switch, install/delete, reboot, Quick Start and Data Management.
6. Only after those succeed test near 300 entries and expanded real libraries.

Before any live build, document one-button rollback via SD/plugin disable, saved-layout backups and known-good Menu state. No unreviewed NAND write code in this project.
