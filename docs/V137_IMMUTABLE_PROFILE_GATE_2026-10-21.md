# WULP v1.3.7: immutable-profile switching architecture — 2026-10-21

## Implemented and verified locally (host-only)

A v1.3.7 **PC-only** source package has been built with `tools/wulp_v136.py`, `tools/wulp_v137.py`, `src/wulp_v137_gate.h`, tests, and Windows test launcher. **36/36 tests passed** with a native C compiler; the 7 native-C integration tests intentionally skip on Windows when no `cc` is installed.

The design replaces unsafe two-file live overwrites with immutable per-profile account save + WLPA pairs, and one exact-format selection marker. The original Homebrew-on-Menu account-save redirect path remains the fallback, and changing back to `legacy` never requires the candidate to be healthy.

The native C gate supports exact marker parsing, SHA-256 callback verification, full candidate length, Barista protected-section and record-flag preservation, 277 distinct game IDs, eight vacant slots, WLPA v1's paired SHA-256 and CRC32, and all 284 IDs (277 games + seven homebrew). Host tests also cover damaged candidate and allowlist, wrong descriptor, truncated marker, wrong account and rollback. A PC-only staging prototype can create an immutable candidate from a **private, verified v1.3.6 stage**. The host lab starts in legacy mode.

The one-boot **v1.3.5 matching hardware redirected read (130116/130116)** remains our last confirmed console result.

## Explicit limits

- No v1.3.7 Wii U `.wps` module built or run.
- No Wii U Menu 277-game rendering, save persistence or title launch demonstrated.
- No atomicity claim about FAT32 or real power failures.
- The portable C gate requires an actual pinned, integrated Wii U platform SHA-256 provider and filesystem adapter, which are not yet implemented.
- Existing v0.8.1 WULP title filtering may conflict and must be integrated/replaced, not layered blindly.
- Private console Barista backups, Title ID inventories and proposed profile files remain off GitHub.

## Next engineering milestone

Patch a pinned `wiiu-env/homebrew_on_menu_plugin` so that during `SAVEInit` / account change it (a) verifies the immutable candidate directory, (b) selects its *save redirection path*, and (c) uses that same selection's WLPA in `MCP_TitleList`. Keep the legacy redirect unchanged if any verification fails; never call `CopyExistingFiles` on a partial candidate. Package a reversible Wii U `.wps` beta and validate backup/rollback before actual 277-game profile activation.

This document reports the host prototype only and does not authorize copying any test data to the Wii U SD card.
