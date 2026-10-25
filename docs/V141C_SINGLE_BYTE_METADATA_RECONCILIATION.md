# WULP v1.3.11c — reconciled single-byte root metadata drift

**Date:** 2026-10-24

## Hardware-adjacent result

The owner supplied the sanitized `WULP141B` report from an SD removed while the Wii U was powered off.

- 130116-byte current redirected Barista; the independent historical backup matches the known v1.3.5 hardware read.
- 141/141 original games present, same Title IDs and slots.
- The first fifteen protected icons and all first 315 root records preserved.
- All 600 root record types/flags unchanged; 60 folder banks structurally valid, unoccupied.
- Exactly **one byte** differs overall. Given both buffers individually pass the folder-bank structure validation, and every root record byte is unchanged, this one-byte change must lie in the **1956-byte opaque root metadata range [9600, 11556)** rather than game slots, folder entries, or folder metadata.
- The initial v1.3.11a deployment appropriately **refused** the hash mismatch, with no additional console changes.

## Reconciled private activation controller

An owner-private v1.3.11c distribution was prepared without any original Barista bytes. It permits precisely two verified source identities:

1. The original historical v1.3.5-attested backup, unchanged; OR
2. The exact current-SD Barista SHA-256 that was independently verified in the private v1.3.11b read-only report, provided it differs from the previous backup at **exactly one** offset in [9600, 11556). Both files must independently pass the structural Barista validator.

Any later/current SD change, two-byte change, changed title ID, altered folder or protected records, different historical backup, or invalid checksum **refuses activation**. There is no generic `--force` bypass.

The candidate is generated from the **current SD Barista**, preserving its root metadata byte and all other untouched structure. The separate legacy SD save is never modified. The deployment retains the private original active plugin and current original Barista for rollback on the PC, stages independent golden/working candidate and WLPA, and only then switches the profile marker. The first 141-game profile remains recoverable through the private rollback script.

## Internal reproducibility

A full synthetic SD staging/activation/rollback drill was rerun after this precise change. Result: **PASS**, including:

- Exactly one controlled metadata-byte change accepted only with its matching independently attested current digest.
- Unknown digest, multi-byte difference, protected-icon change, game-slot change and folder change all rejected.
- 141-game SD original remained byte-identical after stage, activation and rollback.
- 277-game candidate, 284-entry allowlist and paired checks generated and verified; selection marker and original plugin restored on rollback.

**Not yet verified on physical Wii U:** 277 displayed icons, title launching/returning, durable Barista write persistence and interrupted SD writes. The next meaningful step is an **owner-run hardware activation**, not another synthetic PC task.

Private checksums, game IDs, user saves, private source bundle and recovery contents are intentionally excluded from the public repository.
