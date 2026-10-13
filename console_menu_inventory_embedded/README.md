# v1.0 passive Menu inventory (EXPERIMENTAL)

## Why the architecture changed
Four standalone .wuhb diagnostics failed or hung on a Wii U using SDUSB; one froze while mounting SD and one on shutdown. The already-verified v0.8.1 WUPS plugin sees the unfiltered TitleList inside the Wii U Menu before compacting it and already writes an SD report via fs:/vol/external01. This v1.0 variant uses that existing hook and filesystem mapping. No new WHB SD mount, MCP_Open, MCP_TitleCount, or per-game meta.xml reads.

## Proven vs experimental
- v0.8.1 is hardware-proven. This new v1.0 capture is not.
- The original v0.8.1 source and binary filename remain unchanged; this variant is built separately.
- A regression test compares the complete original hook callback with the new callback, after removing exactly one inserted optional capture call.
- Existing v0.8 marker, account binding, profile config, manifest checks, hash-as-advisory and fail-open filter remain the same.
- Capture is disabled unless a SECOND marker exists, exactly sd:/wiiu/library_profiles/v08/ENABLE-V10-INVENTORY.txt containing ENABLE-WLP-V10-INVENTORY followed by LF.
- When enabled, capture occurs before in-place filtering, only for complete manifest observations. Bigger complete observations replace previous CSV; no hard-coded 511-game or 623-record assumption.
- Hard safety bound: 4096 raw records. Diagnostic ceiling, not the Wii U Menu icon ceiling.
- Only writes new v10-inventory.csv.part and v10-inventory.csv under existing v08 directory. Failure does NOT disable the working title filter.
- Platform fields are evidence only; injection platform and region detection are not yet implemented.

## Cautious hardware procedure
1. With the Wii U OFF, preserve a PC copy of the working v0.8.1 .wps, Barista, manifest, config and .ignore. Do not change SDUSB.
2. Remove leftover standalone experimental scanners (.wuhb) if present, but preserve original .ignore exclusions for probes.
3. In sd:/wiiu/environments/aroma/plugins/, REMOVE the old v0.8.1 WUPS plugin (save it on PC), and add wiiu_library_profiles_v10_passive_inventory.wps. NEVER load both together; they hook the same function.
4. First boot WITHOUT the second v10 inventory marker. Confirm the current selected profile behaves exactly as v0.8.1. If it doesn't, rollback immediately.
5. Power off, copy the included marker file to sd:/wiiu/library_profiles/v08/ENABLE-V10-INVENTORY.txt; it must be exact ASCII with LF, not CRLF.
6. Boot normally to Wii U Menu, let it populate as usual, then power off normally. No standalone app or test buttons.
7. Read sd:/wiiu/library_profiles/v08/v10-inventory.csv and v08-menu-report.csv on the PC; if final CSV is absent, inspect v10-inventory.csv.part for failed rename/write. Do not run repeated risky experiments.

## Rollback
With console OFF, remove the experimental plugin and restore the backed-up original v0.8.1 .wps. Remove the optional ENABLE-V10-INVENTORY.txt. Leave the original ENABLE-V08-EXPERIMENT.txt, Barista, active profile and .ignore as-is.

Do not post private title inventories or device paths in a public GitHub issue. This stage exports raw installed-title metadata only; it does not yet classify every VC injection or produce dynamic Barista profiles.
