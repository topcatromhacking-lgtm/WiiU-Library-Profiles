# WULP v1.3.17 — platform-class checkboxes now drive Menu filtering

**2026-10-27: Wii U binary compiled and offline tests passed. Console test pending.**

## Scope

Aroma Plugin Configuration shows one Boolean for each class, in a **WULP - Console Classes** section: Wii U (140), Wii (32), Nintendo DS (31), Game Boy Advance (74). All four default ON and reproduce the existing 277-game extended profile.

On changing a checkbox, WUPS Storage saves the choice and the plugin requests the normal Wii U Menu restart. During SaveInit, the validated extended277 source remains the trust anchor. When a subset is selected, the plugin attempts to activate a matching private Barista preset **and** matching MCP TitleList allowlist. The 15 protected icons remain. If the subset's hashes, binary class catalog, game IDs or Barista layout do not validate, the original extended277 save and filter remain selected.

The original141 profile and v145a switcher were not modified. After upgrading to v147, first restore the known-good v145a plugin before running older PC marker-switching commands.

## Fifteen new masks

Four ON uses the existing v145a 277 files byte-for-byte. The other 15 masks (00–0E hexadecimal) use isolated snapshots under `wiiu/wulp/profiles/byclass/XX/`, each with:
- Immutable 130116-byte Barista layout reference
- Separately writable Barista layout save
- Sorted paired WLPA (selected Title IDs plus seven protected Homebrew title IDs)
- Immutable hash manifest bound to the **original 277 golden Barista** and a 277-entry, CRC-checked per-class catalog

The catalog and all snapshots are reconstructed locally from the owner's existing private `v12-preview.csv` and current 277 golden. Do not commit owner-specific Title IDs, Barista bytes, serial numbers or inventory to this public repository.

The current counts are: Wii U only 140, Wii only 32, NDS only 31, GBA only 74; no classes selected 0 game icons, 15 protected system/Homebrew icons; all selected 277.

## Tests

- Native host compiled validator against all 15 templates generated from the actual owner-supplied historical 277 selection: **PASS**.
- Negative cases: corrupted manifest, corrupted catalog CRC, modified Title ID/layout, and 33 changed opaque metadata bytes: **REJECTED**.
- PC-powered-off deployment rehearsal: stage all 15 templates, leave extended277 selected, read-only checks, restore exact v145a plugin with no Barista/allowlist overwrites: **PASS**. Two legitimate Menu metadata changes remain accepted.
- [Wii U CI compiled](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/40569130014): **SUCCESS**. WUPS binary 254828 bytes; SHA-256 `0eb03f944e86081ae87f1e02a4105dd9d9165f45699305788414bff6a5fc2ea2`.
- Wii U hardware with mixed-platform checkbox selections: **NOT YET TESTED**.

## Owner-only guarded install

Private distribution: `WULP-v1.3.17-Four-Console-Class-Filter-Guarded-PRIVATE.zip`. It includes the binary, guarded PC stager and v145a rollback, generic source code, CMD launchers and documentation, but **does not** include any private console saves or Title IDs.

Power Wii U completely off, connect SD to Windows PC, then run `STAGE_CLASS_FILTER.cmd` with **`E:/`** as the default SD root (editable). Start from known-good v145a **277-game** selected state, retain original `PRIVATE_RECOVERY_KEEP_ON_PC`. The stager changes only the plugin and creates new files under previously nonexistent byclass subdirectories. It never overwrites existing Barista, WLPA or 141/277 markers.

First hardware validation: verify all ON → 277 games in correct order. In Aroma plugin config, leave only GBA checked → expect 74 games and protected icons after Menu restart; restore all four → expect original 277, then verify cold boot. If anomalous, power down and use `ROLLBACK_TO_V145.cmd`.

This is a **functional hardware candidate**, not a confirmed working on-console filter.
