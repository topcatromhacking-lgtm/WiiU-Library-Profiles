# v0.6: Per-account five-position Barista reconstruction (offline only)

## Status

Private **offline** data repair and two sparse-profile variants complete.
**No live Menu filters, save switching or SD writes performed.**

This follows the successful v0.4.2 hardware inventory visibility diagnostics
and the v0.5 audit of separate redirected Barista saves. The affected primary
account's layout was changed by the earlier v0.2 five-title hide experiment:
two probe NAND-side records and three unrelated software USB-side records
occupied the first five software positions.

## Corrective technique

The known format has 300 paired Menu positions, each with a 16-byte
NAND-side and 16-byte USB-side record, plus opaque trailing data.

For the affected original save, restore only positions **16–20
(1-based)** in an offline copy:

- Positions 16 and 17: convert two temporary Homebrew probe NAND-side
  records back to the already-observed empty NAND-side record format.
- Positions 16–20: place five validated installed Wii U game Title IDs
  in the standard USB-side game records. The identities and proposed order
  derive from the historical v0.2 excluded-title list and curated game order,
  not an untouched pre-v0.2 Barista snapshot.
- Preserve positions 1–15, both banks for positions 21–300, all USB
  record flags, and the complete opaque suffix.

A **private** exact-hash source check rejects wrong-account backups and
unexpected original records. A private independent byte diff showed changes
in exactly two NAND records and five USB records, only **26 byte positions**
different in this specific original file. The repaired full view contains
15 fixed first-page items and 285 installed games; neither probe remains on
the first software page. Account-specific original system/tool order and
unknown save data are not borrowed from the other user account.

## Three private output pairs

1. Full restoration: 15 system/tool + 285 game icons, paired with a full
   300-title WLPA manifest; all other software positions retained.
2. Wii U only: 15 + 141, 144 intentionally blank software slots, matching
   156-title WLPA manifest.
3. NES only: 15 + 95, 190 blank software slots, matching
   110-title WLPA manifest.

Every WLPA has CRC32 integrity checks and a SHA-256 binding to its *own*
130,116-byte candidate Barista. The Wii U/NES software USB slot bytes
match the prior verified v0.3.5 examples, while the protected system and
tool data now come from the affected primary account rather than the
unaffected secondary account.

## Validation

A self-contained **private** package was generated with source hash checks,
the original private snapshot, installed inventory and curated selection,
all 3 Barista candidates, all 3 WLPA manifests, audited byte offsets and
reproducible offline builder. **20 local tests passed**, including
modified-byte whitelisting, title ID uniqueness, refusal of a wrong or
mutated source save, tool page preservation, manifest binding and byte-exact
independent regeneration after extracting the ZIP.

**No private installed Title IDs, original Barista binaries, SD account
folders, game inventory, user-specific hashes, or executable restoration
artifact are included in this public repository.** The private v0.6 ZIP
was delivered directly in conversation.

## Not yet safe for hardware installation

Previous v0.2 hardware rollback showed that merely removing a Menu title
filter does not restore persisted Barista icon positions. Never copy these
offline `.dat` candidates onto an active redirected save, even with the
console powered off, before implementing a **safe account-specific cold
transaction and rollback**. Each proposed layout must be loaded alongside
its exact matching active Menu title-list allowlist, with safeguards for
empty pages, synthetic Homebrew entries and update/DLC passthrough.

Hardware deployment and reversibility remain **unverified**.
