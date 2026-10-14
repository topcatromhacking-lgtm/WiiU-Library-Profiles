# Wii U Library Profiles v1.2: Read-only Menu profile preview

**Status: GitHub Actions host tests and WUT/WUPS cross-build passed. Hardware test NOT YET completed.**

This is a **separately named** Aroma WUPS plugin. It forks the tested v1.1.1 Menu hook and adds exactly one optional preview operation before the old TitleList compaction. It never edits Barista, the verified first 15 icon positions, `active-profile.cfg`, the v0.8 allowlists, game installations, or any other Wii U account. The existing v0.8.1 Wii U/NES filter is unchanged.

## Architecture

- Reads the validated `WLPD` v1 master database from `sd:/wiiu/library_profiles/db/master_titles.wlpdb`.
- Reads the separately compiled `WLPS` v1 selection from `sd:/wiiu/library_profiles/profiles/active.wlps`. This is a **32-byte header** plus sorted optional include/exclude Title IDs. Includes platform mask (WIIU,WII,NDS,GBA,N64,SNES,PCE,MSX,NES), JPN/USA/EUR region mask and a capacity policy.
- No JSON parser, standalone WUHB or new SD mount. Both database and profile are loaded at most once after the original account/config/manifest checks, using already-proven SDUSB-visible `fs:/vol/external01`.
- The pre-filter MCP list supplies currently installed GAME Title IDs; the plugin performs C binary searches against the master. The preview sorts by **platform priority, then ascending Title ID**, as verified by host C/Python parity tests. This differs from v1.2's original offline row-order sort; the release includes console-compatible expected reports.
- Unknown/unreviewed game IDs are never silently selected. Individually included IDs can override platform/region, but only when installed and confirmed. Individual excludes always win. Strict 285 game preview positions 16 through 300; positions 1-15 are not touched.
- A profile with `reject` policy and overflow reports `policy_blocked=1`; **no deployment occurs for any policy** in this release.
- Uses SHA-256 of only the canonical installed game-set IDs and CRC32 of both loaded DB and profile to identify immutable preview CSV filenames; 610/613/641 raw MCP count fluctuations alone cannot cause re-exports. Diagnostics only, no Barista.
- A missing, corrupt or oversized profile/DB preserves the original allowlist filtering. The 1.1.1 database audit and 1.0 raw inventory export remain separately opt-in.

## Console safety test: TWO PHASES

1. Power Wii U **OFF**. Back up the current, known-working `.wps` and Barista DAT on PC. Leave SDUSB and existing profile/allowlists alone. Never install two MCP_TitleList Library Profiles plugins simultaneously.
2. Remove the previous Library Profiles `.wps` from `sd:/wiiu/environments/aroma/plugins/` (retain a backed-up copy); copy `wiiu_library_profiles_v12_menu_preview.wps` to that directory. **Do not yet add v12 activation marker.**
3. Keep `ENABLE-V08-EXPERIMENT.txt`, `active-profile.cfg`, `wiiu.allowlist`, `nes.allowlist` untouched. Remove optional `ENABLE-V10-INVENTORY.txt` and `ENABLE-V11-DB-READ.txt` for this isolated test. Boot the console and ensure the **original Wii U or NES Menu profile behaves normally**. If not, stop and restore original `.wps`.
4. Power off. Ensure the master DB exists at `sd:/wiiu/library_profiles/db/master_titles.wlpdb`. Copy the supplied console-ready `active.wlps` (default: WIIU + WII + NDS + GBA) to `sd:/wiiu/library_profiles/profiles/active.wlps`. The package also includes editable PC-side `MULTI.json` and the Python compiler for later profile changes.
5. Add the supplied exact LF marker to `sd:/wiiu/library_profiles/v08/ENABLE-V12-PREVIEW.txt`: bytes `ENABLE-WLP-V12-PREVIEW\n`.
6. Boot normally, let Menu settle; power off normally. The visual Wii U Menu **WILL STILL BE FILTERED BY THE EXISTING v08 profile**, not the 277-game preview.
7. Retrieve `v12-preview-status.csv`, `v12-preview-events.csv`, and `v12-preview-<gamesSHA256>-<profileCRC32>-<dbCRC32>.csv` from **v08/**.
8. With unchanged private inventory and default MULTI selector: expected `DB_READY`, `PROFILE_READY`, `PREVIEW_OK_NO_BARISTA_WRITES`, games=511, joined=511, confirmed=510, review=1, unknown=0, eligible=277, shown=277, overflow=0, policy_blocked=0. Expected fingerprint begins `5EA6DCCD`, profile CRC `F8CD6F78`. Compare CSV Title IDs and positions with the supplied `expected-preview.csv`.
9. Remove the v12 marker to disable further previews after testing.

## Rollback

Power console OFF; remove v1.2 .wps, restore saved v0.8.1 or previously known-good v1.1.1 .wps; remove **only** optional v12 activation marker. All Barista, original v08 cfg/allowlists, accounts, SDUSB, first-page icons, .ignore, installed games stay unchanged.

## Later milestones

**v1.2 is read-only.** v1.3 will need a separately reviewed generator of coupled Barista layout + MCP allowlist with safe atomic replacement and actual slot/order validation. A Wii U GamePad profile selector is later still. Do not claim these are implemented.

The full 511-game private master, local expected report, sample profiles and compiled profile compiler are provided **only in the user-side ZIP**, not committed to public GitHub.
