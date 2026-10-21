# WULP v1.3.2: October 20 hardware and offline validation checkpoint

**Status: READ-ONLY DIAGNOSTICS VERIFIED, MULTI-PROFILE DEPLOYMENT NOT AUTHORIZED.**

This document supersedes the outdated `hardware_validation: NOT TESTED ON CONSOLE` field **only for the read-only attestation**. It does not establish that active Wii U Menu redirection or the proposed paired-profile switch works.

## Evidence tested

- User ran the v1.3.2 optional read-only attestation WUPS on real Wii U hardware and provided `v132-barista-attest.csv`, `v132-fixed-icons.csv`, and the previous v0.8 Menu report.
- Report: `V132 / STRUCTURE_OK`, 130,116-byte Barista account save, 15 fixed first-page icons, 60 empty folder banks, 141 current USB games; `redirected_copy_is_active = UNPROVEN`.
- One subsequent powered-off SD backup of the named Barista candidate was verified: entire-file SHA-256 plus 5,040-byte protected prefix and post-9,600-byte preserved suffix **all matched** the Menu-side report.
- The v0.8.1 report indicated that the Wii U filter had enumerated 141/141 selected games and 7/7 homebrew entries. Its Barista hash field was `MISMATCH_OR_UNAVAILABLE`, not independent proof of redirection.
- Re-ran the 33 packaged Python regression tests under `PC_ONLY/tests`: **33/33 PASS**.
- Re-ran the v1.3 offline paired candidate lab against **that exact one attested Barista backup** and the bundled historical v1.0 inventory, v1.1.1 classification audit, and 277-game v1.2 preview. The independent validator succeeded:
  - 511-game unique installed-game reference set.
  - 277 candidate games and seven synthetic homebrew entries; 284 entries total in WLPA.
  - Eight vacant root USB game positions.
  - 277 game-record replacements, involving 822 differing bytes.
  - All 15 protected first-page icons, all 600 type/flag records, metadata suffix and 60 empty folder banks preserved.
  - Paired Barista/WLPA hashes and WLPA CRC verified offline.

## What this DOES NOT prove

1. **The active redirection layer is not proven.** WUPS read a specific SD Barista candidate file by path; it did not prove the running Wii U Menu loaded that same file.
2. **No two independent powered-off backups were supplied.** The package's formal two-backup preflight was not completed. Structural and independent-pair verification above used the one supplied backup, *without claiming a double-read validation*.
3. **No account-save/allowlist deployment has occurred.** The 277-game candidate is an in-memory, quarantined offline test result only; no new console-ready Barista/allowlist files or installer were produced.
4. Existing v0.8.1 plugin accepts `profile=wiiu|nes`, **not `multi`**. A new profile reader and coordinated switching mechanism must be designed and validated before actual profile activation.
5. No active Wii U/NAND files should be replaced, modified or disabled on the strength of this result.

## Next engineering tasks, independent of user action

- [ ] Design a **read-only active-use attestation**, verifying which redirected Barista save the actual Menu consumes (not merely which candidate path is readable). It must fail closed, avoid unsafe writes, and be reversible.
- [ ] Separate and version the existing generator/verifier Python code and tests, preserving independent build reproducibility and user-private game inventories outside Git.
- [ ] Specify a compatible `multi` profile filter, synchronized profile/allowlist transition and rollback transaction. No deployment until the active-source proof and rollback are demonstrated.
- [ ] Prepare an explicit hardware test matrix for read-only attestation, controlled switching, failed-boot rollback, and title/menu persistence.

**Privacy:** Console account identifiers, serial-specific SD paths, Title ID selections, raw Barista saves and personally identifying console hashes are intentionally not included in this public checkpoint. The corresponding reports remain user-private.

**Date:** October 20, 2026.
