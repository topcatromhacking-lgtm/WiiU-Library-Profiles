# v0.4: paired Menu/Barista allowlist preflight + Homebrew hook-order probe

## Status on 2026-10-11

**Offline manifest generation/validation passed; read-only console plugin compiled and CI passed; on-hardware Homebrew-on-Menu hook visibility is NOT YET TESTED.**

This project has 511 installed base games and nine SD Homebrew-on-Menu candidates. The fixed Menu layout policy is 15 positions on page 1 (eight observed system items, seven selected tools) plus 285 prioritized software positions. The two historical Baristas and their v0.3.5 sparse candidate layouts are user-private and are not committed to GitHub.

### Paired manifest format (no live implementation yet)

The generic format implemented in `wiiu_library_profiles/allowlist_binary.py` is:

| Offset | Length | Encoding |
|---|---:|---|
| 0 | 4 | ASCII `WLPA` |
| 4 | 2 | Version 1, big endian |
| 6 | 2 | Reserved, must be 0 |
| 8 | 4 | Count, 1 to 300 |
| 12 | 32 | SHA-256 bytes of the exact candidate Barista save |
| 44 | 8 × count | Sorted distinct full 64-bit Title IDs, big endian |
| End | 4 | CRC32 (IEEE) of all earlier bytes, big endian |

A manifest is **bound to the binary Barista candidate**. On the host, we validate the exact original-template SHA-256, candidate SHA-256, unchanged protected first 15 slots, unchanged NAND bank, unchanged suffix beyond 9600 bytes, game slots in recorded order, remaining slots empty, and equality of physical Barista icons and manifest ID set. A CRC only catches accidental corruption, not tampering.

The corresponding C predicate is tested offline against both private candidate manifests but **is not active on the console**. It excludes only nonallowlisted `00050000` base-game records with recognized game/Wii app types, and nonallowlisted synthetic `0005000F` Homebrew entries of any app type. It passes through updates, DLC, system components and unknown record types unchanged.

For the user's already-generated offline example profiles:
- Wii U-only: **141 games + 15 first-page icons = 156 allowlisted**; 144 game slots empty; binary manifest 1296 bytes.
- NES-only: **95 games + 15 first-page icons = 110 allowlisted**; 190 game slots empty; binary manifest 928 bytes.
- Both actual profile manifest bytes, Barista files, and candidate lists are held in the user-private downloadable v0.4 offline ZIP, not this repository.

### Read-only hardware diagnostic: Menu hook order

`console_menu_allowlist_probe/` is a separate WUPS plugin that calls `real_MCP_TitleList` once and returns its **unchanged** return code and title array; it NEVER filters, reorders, edits Barista, alters titles or scans full Title IDs. With an exact SD opt-in text marker it records only counts to a CSV file.

Inert install:
- First back up SD and `sd:/wiiu/homebrew_on_menu_plugin/`.
- Ensure the old `wiiu_library_profiles_five_filter.wps` is removed.
- Copy `wiiu_library_profiles_v04_readonly_probe.wps` to `sd:/wiiu/environments/aroma/plugins/`.
- Do not create the opt-in marker. Cold-boot, verify Menu folders and `L + R + D-Pad Down` work.

Counts-only stage:
- Create `sd:/wiiu/library_profiles/enable-v04-probe.txt` containing **exactly** `ENABLE-V0.4-READONLY-PROBE\n` (LF).
- Cold-boot into Aroma, then retrieve `sd:/wiiu/library_profiles/v04-hook-report.csv`.
- Report records Menu-scoped `MCP_TitleList` count, size/sizeof(MCPTitleListType), base-game count, synthetic homebrew count, updates/DLC count. It logs at most two successful records and does **not** log individual Title IDs.
- **Nine synthetic Homebrew candidates** were discovered in the latest SD inventory. Seeing nine in the hook strongly suggests our hook runs after Homebrew on Wii U Menu's synthetic records have been appended and can distinguish them. Zero/missing log is inconclusive and needs investigation.
- Delete marker (or remove .wps with console off) to stop the diagnostic.

**Do not install v0.3.5 candidate .dat files or v0.4 .bin manifests on the console.** This diagnostic requires no Barista changes. Its WUPS hook, although nonmutating, must still be treated as experimental.

### What remains before v0.4 becomes a profile switcher

1. Hardware report validates Homebrew synthetic entries are visible to our `MCP_TitleList` hook with correct chain order and safe capacity.
2. Implement cold boot/profile selection using exact Barista SHA binding and backup strategy; ensure the Menu isn't executing while save changes happen. Refuse mismatched, unknown-region, or unavailable template.
3. Implement synchronized allowlist and verify that sparse pages **stay empty** across cold boots. The previous v0.2 five-game test did NOT automatically restore the saved Barista order on plugin removal.
4. Test first-page tool selection, new game refresh, EUR/JPN and forwarders, then test full rollback.
5. Only then expose a user-facing Aroma selection UI and consider auto-refresh; avoid writes while the Menu is running.

All GitHub code and examples are generic; user-private Title IDs, original backups, and manifests stay private.
