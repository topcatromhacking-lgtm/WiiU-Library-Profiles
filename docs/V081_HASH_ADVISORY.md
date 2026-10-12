# v0.8.1: Barista SHA-256 is advisory, not an activation gate

**Experimental firmware plugin, not yet verified after game exit or cold boot.**

## Why this change

On the real Wii U, the Wii U-only profile initially succeeded: the game
list ended at ZombiU, fifteen fixed icons were preserved, empty positions
remained empty, and `v08-menu-report.csv` showed `FILTER_APPLIED` with all
141 selected Wii U base games and 7 homebrew records present.

However, after launching a game and returning to the Menu, the Menu refilled
empty slots with unwanted NES and Wii icons. The next cold boot did not restore
Wii U-only. The original v0.8 plugin required the on-SD
`BaristaAccountSaveFile.dat` to have a single exact SHA-256; the Menu can
modify the Barista file during normal operations. In v0.8 that deactivated
filtering on later Menu starts. It is **plausible**, not yet proven, that this
caused the post-exit refill.

v0.8.1 removes that dependency. Any valid/mismatched/unreadable Barista save
does NOT disable filtering. The SHA comparison remains as an OPTIONAL
diagnostic field, marked `MATCH` or `MISMATCH_OR_UNAVAILABLE` in the report.

## Still mandatory

- `sd:/wiiu/library_profiles/v08/ENABLE-V08-EXPERIMENT.txt` must contain
  exactly `ENABLE-WLP-V08-EXPERIMENT\n`.
- Valid `active-profile.cfg` with account, console serial and profile.
- Valid corresponding `wiiu.allowlist` or `nes.allowlist` manifest,
  including its CRC and sorted unique IDs.
- Running account must match config; account is also rechecked for each
  MCP enumeration call.
- Title enumeration must contain every selected game and fixed homebrew
  Title ID before filtering. Non-base-game system, DLC and updates pass.

## Key limits

This plugin only changes the Menu's observed `MCP_TitleList`; it does NOT
rewrite positions in Barista. The Menu may still refill positions from
previous cached state, reorder them, or persist unintended items despite
the hook. Therefore: **this revision tests a specific hypothesis, not a
proven final solution**. A subsequent `FILTER_APPLIED` CSV alongside visible
unwanted titles would show filtering alone isn't sufficient.

The diagnostic path remains
`sd:/wiiu/library_profiles/v08/v08-menu-report.csv`.
It now starts with `plugin_version,barista_hash` columns that distinguish
v0.8.1 from earlier reports.

## Upgrade from v0.8 on an SD card

1. Copy current Barista/allowlist/activation files to your PC.
2. With Wii U off, remove the former
   `wiiu_library_profiles_v08_guarded.wps` from
   `sd:/wiiu/environments/aroma/plugins/`.
3. Install **only** `wiiu_library_profiles_v081_hash_advisory.wps`.
   Do not run multiple Library Profiles hooks simultaneously.
4. Leave the working `v08/` manifest, account config, and marker in place.
   Keep the two project probes excluded in `sd:/wiiu/apps/.ignore`.
5. If Barista has already refilled, restore the 141-game Wii U-only Barista
   **on the powered-off SD card** from the PC. The v0.8.1 plugin no longer
   requires this for checksum validation, but Barista still governs
   layout/positions; an existing refilled layout may need restoration.
6. Cold boot. Verify `FILTER_APPLIED`, `plugin_version=0.8.1`, and the
   advisory `barista_hash` column. Verify all 141 base games and the
   fifteen fixed icons, empty positions, game-exit behavior, and reboot.

**Recovery**: remove the activation marker while Wii U is powered off and
restore the verified full-layout Barista from the PC. Preserve your `.bak`
and full-layout backups; no need to edit icon database.
