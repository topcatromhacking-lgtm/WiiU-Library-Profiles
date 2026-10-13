# Wii U Library Profiles v1.1, Stage B: SD master database read-only Menu integration

**Experimental, cross-compiled in GitHub Actions, not hardware-validated.**
Uses the **previously working** v0.8.1 Menu MCP_TitleList filter and v1.0 passive inventory capture, with one NEW, separately opt-in, pre-filter classification audit. No standalone .wuhb scanner, SD mounts, separate MCP calls, metadata probing, Barista modification, profile generation, or changes to the two allowlists.

This branch includes only the **public code**, not the user's actual installed-title list or 511-game master; the private runtime master is included in the separately distributed ZIP.

## Paths and activation

Database binary:
```
sd:/wiiu/library_profiles/db/master_titles.wlpdb
```
Optional editable CSV (used only offline for compiling, NOT read by Menu):
```
sd:/wiiu/library_profiles/db/master_titles.csv
```
Exact second marker:
```
sd:/wiiu/library_profiles/v08/ENABLE-V11-DB-READ.txt
```
Marker bytes, exactly: `ENABLE-WLP-V11-DB-READ` followed by ONE LF (`0A`). The release ZIP provides these bytes, avoiding Windows CRLF problems.

Possible diagnostic outputs, only when enabled and a complete enumeration is seen:
```
sd:/wiiu/library_profiles/v08/v11-db-status.csv
sd:/wiiu/library_profiles/v08/v11-db-audit.csv
```
The audit is written to `v11-db-audit.csv.part` before rename. The status counts confirmed/review/unknown game titles and communicates unavailable/bad DB errors without changing the Menu filter. The audit contains raw Title IDs and known metadata; it does NOT affect Barista or the chosen profile.

The original `ENABLE-V08-EXPERIMENT.txt` and `active-profile.cfg` must remain correct. `ENABLE-V10-INVENTORY.txt` is independent and should be **absent** for this specific v1.1 test; otherwise v1.0 inventory capture would also be enabled.

## Safest hardware sequence

1. **Power down Wii U.** Back up the currently working v0.8.1 or v1.0 WUPS plugin, redirected Barista DAT, profile config, both allowlists and original marker on PC. Do not disable SDUSB. Do not replace Barista over FTP while Menu is running.
2. **Remove the old WLP filtering plugin from `sd:/wiiu/environments/aroma/plugins/`** (copy it to PC), then install **only** `wiiu_library_profiles_v11_db_readonly.wps`. Do not run two plugins hooking MCP_TitleList simultaneously.
3. **Phase 1, no new marker:** remove `ENABLE-V11-DB-READ.txt` and `ENABLE-V10-INVENTORY.txt` if either is present. Boot and confirm that your Wii U / NES profile still operates as before. Do not proceed if the Menu is not stable.
4. **Phase 2, cautious opt-in:** power down. Copy `master_titles.wlpdb` to the `db/` directory (create the folder offline on SD if absent). Optionally copy `master_titles.csv` alongside it. Copy the supplied **exact** v11 marker into `v08/`. Boot and return normally to Wii U Menu. No standalone diagnostic, buttons or special test actions.
5. Power down normally, then read `v11-db-status.csv` and `v11-db-audit.csv` on PC. Compare against the private Stage A 511-title database baseline. The status should read `DB_READY`, `AUDIT_OK_FILTER_UNCHANGED`, `511` games, `510` confirmed, `1` review, `0` unknown **if the installed library is unchanged**. A real newly installed game not in master should appear as UNKNOWN instead; this is intended.
6. After successful validation, remove only the **v11** marker to disable future audits. Do not overwrite or delete the working v08 config/allowlists.

## Immediate rollback (if any issue)

Power OFF, remove the experimental v1.1 `.wps`, restore the backed-up known-good v0.8.1 `.wps`, and remove the optional v11 marker. The database files can stay on SD because v0.8.1 ignores them. Leave the working Barista, SDUSB installation, first 15 icons, `.ignore` entries, old activation marker and profile manifests unchanged.

## Design and safety details

- Database parser compiled from the Stage A `engine/wlpdb_core.h` C11 reader. Big-endian WLPD v1, fixed 32-byte records, CRC32, sorted unique Title IDs, string offsets and lengths checked before use.
- Operational caps: 4096 records / 256 KiB runtime database. A larger/malformed/unreadable database is not loaded. `db_load_once()` only runs **once**, when the original account and manifest checks have already passed.
- Full title-list enumeration occurs through the existing proven **Menu hook**, never via a newly opened MCP service. Classification uses binary search in memory, no per-title SD reads.
- A complete manifest-ready enumeration is required. The largest observed snapshot replaces an earlier smaller one. The new optional audit is strictly **diagnostic** and can be missing, corrupt or fail to write without changing allowlist filtering or Barista.
- Only `wlp_is_game()` title records are classified; updates, DLC, system and synthetic Homebrew records do NOT consume game slots in this audit.
- `CONFIRMED` means the SD master flags that Title ID as reviewed/confirmed; it is **not** a statement that Wii U-native or Virtual Console type can be independently proven by MCP.
- The known *Wonderful 101* catalog discrepancy remains a REVIEW entry; never silently guess from game name.
- User-specific DB entries are kept out of the public repository. Please do not post the audit with installed paths to a public issue.

## Next step after hardware confirmation

v1.2 implements platform/region profile selection and preview using loaded metadata, without Barista writes. Automatic Barista generation remains a separately gated later milestone.
