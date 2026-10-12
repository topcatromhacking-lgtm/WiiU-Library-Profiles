# v0.4.2: Missing-game investigation, no Menu changes

The v0.4.1 read-only hardware CSV returned:

| Metric | v0.4 hook test | v0.4.1 allowlist test |
|---|---:|---:|
| Valid Menu MCP records | 586 | 556 |
| Base-game records | 511 | 492 |
| Synthetic Homebrew | 9 | 9 |

The Wii U profile declared 141 games and found 131 (**10 missing**);
the NES profile declared 95 and found all 95. Each selected the same
7 of 9 Homebrew candidates. Both saw 6 of 8 fixed Barista system IDs
(the other 2 are not necessarily exposed by `MCP_TitleList`).

**Why v0.4.2 exists:** v0.4.1 logged the **first successful Menu
MCP_TitleList call only**. The 19 fewer games vs v0.4 may be a
snapshot/timing/device-state difference. We must not assume these
games were uninstalled, and must not activate the sparse Barista saves
or hide titles before resolving this.

## Behavior

This separate, inert-by-default Aroma WUPS plugin:

1. Calls original `MCP_TitleList` once per intercept and leaves
   the original return code, data buffer, count and game visibility
   **entirely unchanged**.
2. Reads both existing v0.4.1 WLPA manifest inputs from
   `sd:/wiiu/library_profiles/v041-wiiu.allowlist` and
   `sd:/wiiu/library_profiles/v041-nes.allowlist`.
3. Observes up to 64 successful, bounds-checked Menu calls after
   one explicit opt-in marker, recording at most 20 **changed**
   inventory summaries in `v042-call-history.csv`.
4. When a snapshot has better expected game/homebrew coverage
   (or the same coverage with more games), writes an updated
   **private** `v042-missing-ids.csv` with the exact *expected*
   game/homebrew/system Title IDs missing from the best snapshot.
5. Never overwrites Barista, alters installed titles, modifies
   Menu folders or game saves, nor filters any icon.

All data remains on your SD until you privately upload the CSV.
Do not publish v042-missing-ids.csv: it includes private installed
Title IDs from your allowlist, not a general dump.

## Hardware test

0. Back up the SD and redirected Homebrew-on-Menu save as usual.
1. Remove the old `wiiu_library_profiles_v041_dry_run.wps`
   (and confirm older v0.2/v0.4 diagnostic WUPS modules are absent).
2. Copy the new `wiiu_library_profiles_v042_inventory_diagnostic.wps`
   to `sd:/wiiu/environments/aroma/plugins/`. Leave your
   existing `v041-wiiu.allowlist` and `v041-nes.allowlist` in place.
3. Stage A: with activation file absent, cold-boot and verify
   Menu icons and Homebrew Loader remain normal.
4. Stage B: place **exact ASCII bytes**
   `ENABLE-V0.4.2-READONLY\n` (single LF, no BOM or CRLF)
   in `sd:/wiiu/library_profiles/enable-v042-diagnostic.txt`.
   Cold-boot Aroma into Wii U Menu. No app launch is necessary.
5. Upload **both**:
   - `sd:/wiiu/library_profiles/v042-call-history.csv`
   - `sd:/wiiu/library_profiles/v042-missing-ids.csv`

If there is only one row, it means only one distinct suitable
snapshot was observed. If no CSV is generated, first confirm the
marker and the manifests. No output is generated with the marker
absent. Do not copy any Barista `.dat` candidate onto the SD.

**Interpretation:** if the count increases to 511 in a later
call and all expected IDs become visible, v0.4.1 captured an
incomplete snapshot. Otherwise, inspect the exact missing IDs
and compare them with the MCP catalog and storage/device status.
Any risk of unwanted icon layout persistence still blocks
switching, regardless of this diagnostic passing.
