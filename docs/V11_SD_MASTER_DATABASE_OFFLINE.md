# Wii U Library Profiles v1.1: SD master database (offline stage)

**Experimental offline implementation, no Wii U plugin changed.** This branch establishes the Title-ID-based metadata database design. The console currently has hardware-verified v0.8.1 filtering and v1.0 pre-filter inventory capture. The purpose of v1.1 Stage A is to validate the join and game-capacity policies off console before adding file access to the Menu hook.

## Runtime format

The editable, UTF-8 CSV master is **compiled offline** into a deterministic `WLPD` v1 binary. The CSV cannot safely be parsed with a simple line-based scanner because Nintendo game names in it include quoted commas and embedded newlines.

The binary is designed for one-time loading into memory, followed by O(log N) Title-ID lookups **without further filesystem calls from MCP_TitleList**.

Format (all integer fields big-endian): `WLPD` magic (4 bytes), version u16 = 1, record size u16 = 32, record count u32, string table length u32, CRC32 of record and string payload u32. Records are sorted ascending by 64-bit Title ID. Each 32-byte record has ID u64; platform enum u8 (0 unknown; 1 WIIU, 2 WII, 3 NDS, 4 GBA, 5 N64, 6 SNES, 7 PCE, 8 MSX, 9 NES); region mask u8 (JPN=1, USA=2, EUR=4); verified flag u8; reserved byte; name-offset u32 and name-length u16; reserved u16; product-code-offset u32 and length u16; reserved u16 and u32. Offsets refer to the UTF-8 string table after all records.

The SD-side proposed location is `sd:/wiiu/library_profiles/db/master_titles.wlpdb`. The user-readable CSV is `sd:/wiiu/library_profiles/db/master_titles.csv`. The live installed-title list is authoritative for installation status, and the local master is authoritative for known metadata.

## Proven offline outcomes

From a **private** local 511-game installed inventory and 511-row provisional master seed, 511 Title IDs join exactly. 510 entries have a confirmed platform. One Title ID remains unresolved and must be reviewed, not silently assigned a console. Selecting Wii U, Wii, NDS and GBA gives 277 confirmed eligible games. Adding N64 gives 298, and the 285-game preview omits 13 N64 entries and raises an explicit truncation warning. A complete platform selection finds 510 confirmed eligible games with 225 beyond the preview capacity.

Reserved first-page icons (8 system + up to 7 selected homebrew) are out of scope for the game selector. Only positions 16–300 are allocated (285 games). No Barista/allowlist files are written by this offline stage. Platform priority is WIIU, WII, NDS, GBA, N64, SNES, PCE, MSX, NES; within platforms retain original master row order pending a separate sorting phase. Multi-region selection uses a union of region masks; duplicate regional matches do not duplicate Title IDs.

## Next integration

Port the verified offline compiler/decoder to a separately named WUPS v1.1 experimental variant. Read the binary **once at init** from the already-working SDUSB-visible filesystem path and cache in memory. The hook should do lookups only and emit a compact diagnostic summary; a malformed/missing database must not disable the validated v0.8.1 allowlist filter. **Do not enable direct Barista rewriting yet.**

The user-specific game inventory, seed master CSV, generated binary and diagnostic reports are distributed privately to the user and **are not committed to the public repository**.

The full tested offline code, portable C11 decoder, synthetic tests and private test reports are supplied separately as a versioned ZIP.
