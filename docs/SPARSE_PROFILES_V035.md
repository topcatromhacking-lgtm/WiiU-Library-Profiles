# v0.3.5 offline sparse layouts: Wii U only and NES only

## Hardware owner approved examples

Build two **complete offline candidate** Barista save snapshots from the same original Barista 2 template, preserving the entire 15-slot system/tools first page, with only one software platform on pages 2 through 20. These candidates are **not safe to install yet**, because saved Menu position state and enumeration must be filtered/synchronized.

The 130,116-byte `BaristaAccountSaveFile2.dat` supplied privately is the validated baseline. Its first 15 protected positions (8 system records and 7 selected Homebrew) are byte-for-byte retained in both candidates. Only the 285 corresponding 16-byte USB game slot records can change; the entire 300-record NAND bank, fixed positions in the USB bank, all record flags and the opaque data after offset `0x2580` must remain identical.

## Observed results on the user's installed-title inventory

| Metric | Wii U only | NES only |
|---|---:|---:|
| First-page system + tools | 15 | 15 |
| Installed software Title IDs selected | 141 | 95 |
| Software pages occupied | 10 | 7 |
| Empty game slots | 144 | 190 |
| Software icons withheld by active view | 370 | 416 |
| Launchable-icon allowlist (system/tools included) | 156 | 110 |
| Target Barista length (bytes) | 130,116 | 130,116 |
| Candidate SHA-256 | `c65de6f97204dd8cc2608918c01459887304baa574e762bfd167f1abf9334082` | `fcac843d44e0e4c98a6fd10466f7943a929cf19e15974fe486251aad9855de45` |

Both candidates passed full binary audit: **the 4,800-byte NAND bank, the initial 15 USB records and all bytes after the paired banks remain unchanged**. There were 285 modified game-slot records in each output, and 847 changed bytes in Wii U only vs 942 changed bytes in NES only. Every included game ID matches the current installed MCP/ACP-derived game inventory, platform classification and unique-ID requirement; no update or DLC is used as an icon.

The historical Excel order drives the sorting. The one stale *The Wonderful 101* entry in Excel (reference `000500001011F300`) is linked to the actual installed Title ID `000500001012DC00` at the same original row 121. An explicit **private** order-correction mapping records this name-supported repair, rather than hardcoding it in the public library. All five v0.2 filtered Wii U titles are in the Wii U-only candidate.

The **private** downloadable package includes both offline .dat candidates, two complete machine-readable game allowlists, selected-game CSVs, byte-diff audit reports, an original Barista 2 baseline, a 660-record inventory snapshot, seven selected SD homebrew tools, an offline stdlib generator and 11 passing source-data test cases. An independent ZIP extraction, local regeneration of both binary outputs, byte-for-byte comparisons and per-file SHA-256 manifest check all passed. The private binaries, metadata and Excel content are not committed to GitHub.

## Crucial remaining gates

1. **Do not install either candidate save** until source-level Menu-only `MCP_TitleList` allowlist filtering and Homebrew-on-Menu chaining have been verified with the *same* selected 141/95 title IDs.
2. **Do not hot-swap Barista while the Wii U Menu is active.** Determine correct offline/boot-time selection and recovery against the plugin's redirected per-user SD save. Preserve backups of the Menu layout and original system save.
3. Test Menu boot with 144/190 intentionally blank USB game positions. Prior v0.2 hardware filtering experiment proves free slots may be refilled by unrelated titles, so the Barista save by itself does not promise empty subsequent pages.
4. Test installation/removal of a new game, stable category sorting, full installed inventory refresh, region-awareness and optional Homebrew tool selection. One synthetic newly installed NES game was appended by the offline builder without manual hex edits.
5. Verify each region's system Menu and user Barista format independently before generating a cross-region save.

Development branch: `feature/v0.3-offline-barista-builder`. Draft PR #7 remains experimental; do not merge/deploy until on-hardware recovery validation.
