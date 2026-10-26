# WULP v1.3.12a — legacy-save drift in real post-reboot capture (2026-10-25)

## New physical evidence

The owner's **read-only** post-reboot `WULP142` report establishes:

| Property | Result |
|---|---|
| Live legacy redirected Barista | CHANGED from original 141-game pretest backup |
| Immutable 277-game golden | UNCHANGED |
| Immutable 284-entry WLPA | UNCHANGED |
| Extended marker | VALID |
| Prior v141d WUPS plugin | PRESENT |
| Writable 277-game Barista | EXACT same 277-game set; only ONE changed opaque metadata byte |
| 15 protected icons, 600 record types/flags, 60 folder banks | UNCHANGED |
| Existing v142 working validator | ELIGIBLE |
| Upgrade result | REFUSED solely because `LIVE_LEGACY_SAVE_CHANGED`; NO SD WRITES |

This strongly supports the two boot-fallback mechanisms found during v1.3.12 code review. It does not establish whether the live legacy change is safe by itself, nor prove a second hardware boot.

## Error in v142 PC updater

The plugin itself now uses the **immutable private pretest legacy Barista** to verify the golden/WLPA pairing and does not rely on a mutable legacy save for its extended mode. The original v142 host-only updater, however, retained the old condition:

`if not report['original_legacy_unchanged']: reasons.append('LIVE_LEGACY_SAVE_CHANGED')`

This is too restrictive for the owner-reported case.

## Owner-only v1.3.12a installer correction

A private update kit reuses the **existing read-only evidence directory** `PRIVATE_RECOVERY_KEEP_ON_PC/postboot-evidence-v142`, without asking for a new PC capture. It:

1. Requires the saved v142 report to have **only** the legacy-change refusal and no other gate failures.
2. Compares **every byte** of live SD legacy/golden/working/WLPA/marker/WUPS against the already preserved private postboot evidence before any SD write.
3. Independently regenerates and verifies the immutable 277-game golden + WLPA from the saved *pretest* source `PRIVATE_RECOVERY_KEEP_ON_PC/legacy/Barista.original.private`. The legacy source is never taken from the live mutated SD Barista.
4. Requires an intact first-315 legacy root block, unchanged Barista record type/flag fields, unique game IDs of the expected title kind in legacy USB slots, and structurally sane folder-bank type/flag fields; saves the entire changed legacy unmodified in existing PC evidence.
5. Permits **at most one opaque-root metadata byte drift** in the 277-game working Barista. Preserves the first 15 icons, original other protected records and all 60 folder banks. Rejects game substitution, duplication, type changes, unsupported metadata/folder edits.
6. On success, stages only the immutable pretest reference `LegacyBaristaReference.dat` and swaps the `homebrew_on_menu.wps` binary to the **same already compiled v1.3.12 WUPS**, keeping legacy, golden, working, WLPA and marker untouched. The owner can use their original v141d rollback to restore the prior plugin, disabled filters and selector.
7. On any post-capture difference, malformed live legacy, invalid reference, prior activation, or missing snapshots: REFUSE, **no SD changes**.

The owner-private package contains no console serial, game IDs, Barista data or private inventory and is intentionally **not** committed to public GitHub.

## PC regression tests

Three complete isolated host runs using the original v141d deployment controller, known 141-game Barista shape, previous v139 binary, actual v142 binary, and existing v142 capture logic:

- Valid v141d stage → activate → synthetic one-byte drift + legacy title addition → v142 read-only refusal → v142a conditional upgrade → original v141d rollback: **PASS**, byte-for-byte legacy/working preserved, original HOMP restored.
- SD working Barista mutated again **after** evidence capture: **REFUSED; WUPS and SD untouched**.
- SD legacy protected root record tampered, including updated captured bytes: **REFUSED; WUPS and SD untouched**.

These PC tests are not a physical boot result.

## Still open

Verify the owner-specific SD passes this structural preflight. Then verify real Wii U Menu icon count, no NES reappearance, launch/return and **two cold boots**. A v141d rollback restores the exact previous plugin and pointer, but **does not revert the live legacy Barista that the previous Menu boot already modified**; preserve the pretest and postboot private backups before any further intervention.

**Project status: 277-game first-boot rendering observed; post-reboot persistence not yet demonstrated.**
