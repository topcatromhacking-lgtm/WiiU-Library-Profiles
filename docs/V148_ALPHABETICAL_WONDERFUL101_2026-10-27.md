# WULP v1.3.18 — Alphabetical console classes and The Wonderful 101

**Build and offline verification PASS; Wii U hardware test pending.**

The owner has verified v1.3.17's native four-class filtering on Wii U hardware. Version 1.3.18 alphabetizes games inside each selected class in this order: Wii U, Wii, Nintendo DS, GBA. Each class sorts #–Z using case-insensitive, punctuation-insensitive title comparison. The four checkboxes remain unchanged in purpose.

The Wonderful 101 (`000500001012DC00`) was in the original 141-title Barista backup but omitted from the earlier 277-game preview because its historical catalog classification was REVIEW/UNKNOWN. The new selection contains 141 Wii U, 32 Wii, 31 Nintendo DS, 74 GBA: **278 games**, plus 15 fixed icons, **293 total**.

New class arrangements use 16 separate Barista/allowlist pairs. The earlier 141, 277 and v147 layouts remain intact. The new validator checks the 277-title source, original 141 backup, both class catalogs, order manifest, matched game positions and paired allowlist. Invalid new profiles leave the proven 277-title selection active.

[Native Wii U build](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/40611556943): PASS. Compiled plugin SHA-256: `3840d393eec675cf8a513330716bc8798f82b36f0e0c8aed5781150f2e52b428`.

Offline checks passed on all 16 generated layouts using the historical private inventory and both Barista references, including verification that The Wonderful 101 was restored. A guarded PC rehearsal passed staging, two-byte metadata tolerance, corrupted-save refusal, duplicate-stage refusal, and exact plugin rollback to v147.

The private package is `WULP-v1.3.18-Alphabetical-Wonderful101-Guarded-PRIVATE.zip`. Run `STAGE_ALPHABETICAL_278.cmd` with the Wii U fully powered off, and SD drive `E:/` by default. On-console, all four classes enabled should show 278 sorted games, with The Wonderful 101 under T. The exact previously installed v147 plugin can be restored by `ROLLBACK_TO_V147.cmd` while powered off.

**New sorting has not yet been hardware-confirmed.**
