# WULP hardware checkpoint and v1.3.6 staging gate (2026-10-21)

## Verified v1.3.5 hardware milestone

Owner ran the pinned v1.3.5 diagnostic ContentRedirectionModule `.wms` on physical Wii U and captured successful redirected account-save OPEN, a complete 130116-byte READ, and CLOSE. The local sanitized independent-backup check returned:

- `verdict: MATCHING_REDIRECTED_MENU_READ_OBSERVED`
- `matching_unique_bytes: 130116` of `barista_bytes: 130116`
- `matched_read_events: 1`; `closed_handles_ok: 1`
- Zero rejected sources, mismatches, invalid reads, unknown offsets, sequence gaps or duplicate sequences.

This closes the **one-boot hardware redirected-read evidence** gate. It does **not** establish the 277-game profile, multi-profile plugin support, or hardware rollback. FNV64 is noncryptographic. No private console data is included here.

## v1.3.6 host-only proof of concept

A standalone local prototype was built and passed **15 synthetic automated tests** including full-size 277-game candidate formation, 284-title WLPA pairing, first-page/folder/flags preservation, two distinct matching source backups, source and game-set hashes, fail-closed proof check, private PC staging and simulated two-file failure recovery.

It produces a candidate pair and rollback pair in private PC staging only, and refuses to write to expected SD/console paths. Tests include simulated faults after each candidate write and verification of complete simulated restoration. It does **not** implement an atomic FAT32 live transaction or a console installation. The owner-specific inventory, v1.2 preview, audit, raw saves and active allowlist are intentionally not published. Consequently **no actual owner-data 277-game profile was generated or deployed**.

## Engineering sequence to v1.4

1. Recover owner-private v1.2 277-game preview, v1.0 inventory, v1.1.1 audit and original active allowlist; reconcile with current inventory and source backup. Existing generator in `src/v13_pair_generator.py` and independent verifier in `src/v13_independent_verifier.py` remain canonical offline references.
2. Add a pinned, multi-profile-compatible Menu filter that verifies the paired Barista / allowlist state before consumption. The existing v0.8.x WUPS single-platform filter is not a demonstrated `multi` implementation.
3. Implement cold-boot recovery and a device-level rollback protocol before ever authorizing writing either active file.
4. Only then proceed to controlled hardware activation, game-launch/return testing and cold-boot persistence with all 15 protected icons intact.

No further read-only v1.3.5 test is needed; the bottleneck is safe, compatible **runtime paired-switch implementation**.
