# v1.1.1 audit snapshot fix and v1.2 offline profile planner

**v1.1.1 WUPS experimental build passed portable C, callback-regression and pinned Wii U WUPS crossbuild. Not yet hardware-validated.**

User's v1.1 console reports showed `v11-db-audit.csv` complete (511 unique game IDs: 510 confirmed, one review, zero unknown), but later `v11-db-status.csv` recorded `AUDIT_WRITE_FAILED_FILTER_UNCHANGED` at 641 raw MCP records. Earlier v1.0 raw list had 610 records and the same 511 games; the additional 31 entries were non-game records. The exact underlying write/close/rename failure was **not** identified by v1.1's combined error test. Rename-over-existing is one possible hypothesis, not proven.

## v1.1.1 changes, separately gated

- Identical game sets produce the same 32-byte SHA-256 after canonical sorting of only GAME Title IDs (big-endian eight-byte IDs). Changes in raw MCP ordering or updates/DLC/system counts are ignored.
- Capture is still gated behind the *existing* v1.1 optional exact marker `sd:/wiiu/library_profiles/v08/ENABLE-V11-DB-READ.txt`, and the original account/config/manifest readiness gates.
- New CSV is immutable and never replaces a completed file: `v111-db-audit-<SHA256>-<master-CRC32>.csv`. It uses the matching `.part` file, checks whether the target already exists, and does not rename over an existing target. A changed database CRC produces a new export.
- The append-only `v111-db-events.csv` records the attempted operation, OS `errno`, raw/game counts and fingerprint. Failures distinguished: destination lookup, partial open/write/close, final rename, allocation, hash, and existing-file close.
- An unchanged previously exported snapshot is reused across boots; the status counts are still calculated accurately. No repetitive 610/641 raw count exports.
- The original v0.8.1 Menu filtering function was regression-tested unchanged (except the optional read-only DB audit call already added in v1.1). Barista, allowlists and installed software are never written. Missing/corrupt databases fail open for the filter.
- The original v1.1 output `v11-db-audit.csv` is preserved; the new file naming is different to avoid overwriting or confusing older reports.

**Hardware safety:** install **only one** Library Profiles MCP TitleList WUPS plugin at a time; back up the proven v0.8.1 before trying v1.1.1. First boot without v1.1 marker and verify Menu. Then opt in from powered-off SD. SDUSB must remain enabled. Original v1.1 database path still applies. Roll back by restoring the original .wps and removing optional v11 marker.

## v1.2: Offline profile engine

Separate user-private ZIP contains `engine/wlp_v12.py`, five explicit JSON profiles, 511-title SD master seed and binary, local inventory, v1.1 hardware audit, and generated reports. It **never writes Barista or deployed allowlists**.

- Named JSON profile defines platforms, regions, individually included and excluded 16-digit Title IDs, capacity policy `reject|preview|acknowledge`.
- Exact installed Title ID is the join key. New IDs without database metadata are flagged UNKNOWN, never included silently. Unconfirmed records (currently The Wonderful 101) are held for review.
- Default priority WIIU, WII, NDS, GBA, N64, SNES, PCE, MSX, NES. Within each platform use original master order.
- Protects first 15 fixed positions (8 system and 7 selected homebrew); positions 16-300 permit at most 285 game previews. No automatic truncation application.
- Region bitmask union prevents duplicates. Individual include can override selected platform/region only for reviewed/installed games; explicit excludes always override.
- Stable installed-game-set fingerprints and a 511 vs 511 no-change test compare the v1.0 and v1.1 captures.
- Regression outcomes with private baseline: WIIU=140; NES=95; FOUR=277; FIVE=298 eligible, 13 N64 overflow; ALL=510 eligible, 225 overflow. One review item remains.
- Offline tests: 10 v1.1 tests + 10 v1.2 tests passed.

**Privacy:** user installed titles and SD master metadata are supplied in the user's private ZIP, not committed to the public repo.

The **next engineering milestone** after optional hardware validation is separate, gated v1.2 WUPS selection and its output preview, still without Barista changes. Actual synchronized Barista/allowlist generation and on-console user interface remain later milestones.
