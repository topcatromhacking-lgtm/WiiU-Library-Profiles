# WULP v1.3.14 — cumulative metadata drift after second cold boot

**2026-10-25. Hardware test pending. v1.3.12a persistence FAILED.**

## Real-world evidence

The owner reported that v1.3.12a restored the correct 277 games, and that this layout survived launching a game and returning to Menu, **but not a full cold boot**. The owner then supplied the private read-only `WULP143` post-cold-boot report (no raw Barista or title IDs).

The report confirmed:
- Golden 277-game Barista and 284-entry WLPA remain unchanged and verified against original PC recovery.
- Immutable pretest legacy reference and extended-selection marker remain valid; the v142 Wii U plugin is installed.
- **277/277 game Title IDs remain present and distinct**, with unchanged protected first 315 root records, all USB root record types and flags, and all 60 folder banks.
- Exactly **two bytes changed**, both in the 1956-byte opaque root metadata range `[9600,11556)`. The previous v142 validator permitted at most **one** such changed byte, rejecting this valid-looking working save and falling back to legacy selection.
- Live legacy Barista also changed, as seen in the previous incident; the fixed v142 plugin no longer trusts that mutable file as its cryptographic reference.
- The SD was stable during the read-only capture and was never written by the capture tool.

This proves an actual **working-gate rejection** after cold boot, but not that SD readiness is irrelevant. Both paths are addressed conservatively in the research build.

## Source fix

- `src/wulp_v144_working_gate.h` compares root slots 0..314 exactly, the complete folder and footer range `[11556,130116)` exactly, and all 285 USB game slots for correct type/flags and precisely the same 277 distinct allowed game IDs.
- It tolerates **at most 32 changed bytes in the isolated opaque root metadata region**; any other unexpected difference is rejected. This deliberately finite bound is a **temporary engineering policy**, **not** an identified semantic map of the metadata. We should map the changing offsets/fields from preserved owner-private captures after verifying the persistent hardware result.
- `tools/build_v144_homp.py` layers the new working validator onto `v143`, retaining the `SAVEInit` SD-mount readiness recheck, boot-time event tracing, immutable pretest legacy reference and the independent paired title filter that remains enabled when the working save is rejected.
- No on-device Barista normalization, writes, reordering, folder edits or unverified Title IDs are introduced by this code.

## Test status

[GitHub Actions](https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/40262140264) **SUCCESS**. Wii U WUPS code-only binary SHA-256 `bcf5e02fa38d7cdda454c2b777b92ef9c88a6e4b5f8aa9da01de9078a53b5f98`, size 241794 bytes.

- Native tests: unchanged working save, cumulative metadata bytes 1..32 accepted, 33rd rejected, valid game rearrangement, missing/duplicate/unauthorized Title IDs, protected slots, flags, folder banks and incorrect file sizes all checked.
- Owner-private host-only deployment: **PASS** staged v141d profile → v142 legacy refusal → v142a upgrade → second cold-boot synthetic two-byte metadata drift → v143 read-only snapshot → v144 WUPS-only guarded upgrade → v141d full rollback.
- Owner-private negative tests: any SD change after the most recent read-only capture is refused with no SD writes; tampered USB record flags refused even if the snapshot is also tampered. No owner data uploaded to GitHub.
- **Real hardware persistence remains untested** for v144. No claim of stable cold-boot operation.

## Private install and rollback

An owner-private ZIP, `WULP-v1.3.14-Guarded-Cold-Boot-Upgrade-PRIVATE.zip`, contains only the compiled `.wps`, host-only `wulp144_upgrade.py`, Windows launcher, instructions and checksums. No owner ID/game list/Barista.

Put the three required program files into the ORIGINAL v141d PC extraction beside `PRIVATE_RECOVERY_KEEP_ON_PC`, with the Wii U fully OFF and SD attached to PC. Run `UPGRADE_V144_FROM_COLD_BOOT_CAPTURE.cmd` and enter SD root (`E:/`). The controller reuses only the newest `coldboot-evidence-v143/capture-*` and requires the current SD be **bit-identical** to its saved snapshot. It regenerates the golden/WLPA from pretest PC materials, independently enforces the exact **two-byte opaque-only** hardware drift, captures the old v142 plugin and current live legacy Barista again on PC, and changes **only the WUPS plugin** on SD. It does not write Barista, WLPA, marker or any Nintendo NAND data. If any precondition fails it stops.

On installation, verify 277 games and protected icons, game launch/return, two full cold boots, no NES reappearance, no extra blank pages. On abnormal behavior **power down**, preserve the PC recovery, and use the original v141d `ROLLBACK.cmd`. Do not repeatedly activate older versions.

## Future

Long-term robustness needs a field-level description of the changing opaque metadata, and boot-time visibility into `SAVEInit` ordering and redirection state. The 32-byte threshold is a bounded hypothesis test, not a formal fix for arbitrary Barista writes.
