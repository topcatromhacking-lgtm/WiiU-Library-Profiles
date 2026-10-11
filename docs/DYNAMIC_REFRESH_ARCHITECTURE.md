# v0.3.1: Dynamic title discovery and Barista order reconciliation

## Architecture decision

**Do not hardcode the installed games in Barista or ship US-only Title ID allowlists.** The two historic Barista files are templates/snapshots. Treat the current hardware title inventory as the source of which games exist, and generate profile selections from it. The historical Excel file provides an initial user-curated *classification and order*; the full 3,621-entry CSV is a **reference lookup, not an installed-title list**.

### Source precedence

1. **MCP_TitleList / MCP_TitleCount**: detect installed titles (game IDs + device + app type, including updates/DLC/system records that must not be hidden by game-only filtering). Newly installed or removed games appear in the next scan.
2. **ACPGetTitleMetaXml**: prefer actual installed names and product codes, independent of whether the title is USA, JPN or EUR. For an injected title these may be donor metadata, so platform is a wrapper **hint** and origin remains unknown.
3. **A regional Title ID reference registry**, e.g. the user's 3,621-entry CSV (columns Title ID, Name, Region, Type): supply known names and region where ACP is unavailable, but do not use `eShop/Application` as a Wii U-vs-VC discriminator. Regional catalog records are optional enrichment; unknown IDs still appear in the current hardware inventory.
4. **Explicit user overrides keyed by full 64-bit Title ID**: authoritative for injection corrections or nonstandard `WUP-N-` native apps. Keep overrides separate from installed snapshots. Never misinterpret donor platform as the actual ROM.
5. **Homebrew on Menu**: synthetic records `0005000F...` are appended from SD apps by the Homebrew on Menu WUPS hook *inside* the Menu and are **not guaranteed to appear in standalone MCP scans**. Scan those independently (Menu-scoped inventory or SD `wiiu/apps` enumeration), preserve Homebrew access and use a stable SD path as identity alongside generated synthetic Title ID. Installed forwarder channels require user classification; they may use ordinary `00050000` IDs.

### User-visible behavior

- On startup / refresh, merge current console inventory, reference names/regions, ACP platform hints, and saved overrides.
- Compute a diff from the previous refresh (added/removed/reclassified/changed names).
- Classify and sort each console-family collection with a stable policy. Use the historical Excel order when explicitly selected, or alphabetical title + full Title ID as a deterministic default.
- **Do not silently discard unknown games.** Show `unclassified` for manual correction. Wii U homebrew is a distinct category.
- Validate that *every currently installed launchable game* is assigned to at least one profile, that essential Menu and synthetic-homebrew access is preserved, and that no layout exceeds available slots. Split oversized collections into extra profiles/pages rather than truncating.
- Rebuild prepared Barista layouts programmatically (no manual hex editing) from validated templates, protecting non-game slots and copying unknown save data exactly. Do **not** write live Barista while Menu runs. Menu-only MCP filtering must agree with the same profile selection to prevent unwanted icon refilling.
- Region support: recognize Japanese/European Title IDs, but never assume region from a final letter in `product_code`. Different console firmware Menu IDs need separate hardware validation and binary Barista version checks; never copy an American template blindly to EUR/JPN.

### Findings from private hardware data, *not committed*

The reference CSV has 3,621 unique IDs (2,802 eShop/application, 418 patches, 141 DLC, 130 demos, 79 system apps, 51 unknown). All **511/511 installed base-game IDs** matched. Across 660 MCP records, 657 matched reference entries; three missing were non-game system/update records. The 511 installed games were 481 USA, 24 JPN and 6 EUR per reference.

The user's curated Excel classification plus ACP product families allocated the full 511-game collection into: Wii U 141, Wii 32, NDS 31, N64 21, TurboGrafx/PC Engine 40, GBA 74, SNES 54, NES 95, MSX 23. No current game was unclassified **with the curated overrides**. Without overrides, two unusual `WUP-N-` applications remain unclassified, correctly prompting review.

An **offline private** Python refresh prototype and 11 synthetic unit tests are supplied directly to the user, not committed with raw user data. It emits `inventory.json`, `inventory.csv`, `report.json`, `changes.json`, `platform-profiles.json`. The current platform profiles have exact coverage 511/511 within a conservative 280-game profile planning budget. Homebrew icon accounting remains incomplete until a Menu/SD app enumeration is implemented.

### Important distinction

This v0.3.1 refresh planner **does not switch profiles on Wii U hardware, reconstruct a fresh real Menu save, or guarantee no stale Barista icons**. It is an offline validation layer to develop safe Menu/Barista synchronization next. The v0.2 five-game test demonstrated that removing filtered records can persist replacement slots in the redirected Barista layout. Both enumeration and layout state must therefore be reconciled before claiming reversible profiles.

## Next implementation sequence

1. Land generic refresh code and synthetic regression tests, excluding private catalog data.
2. Create an updated read-only Wii U probe for Menu-scoped synthetic homebrew plus existing MCP/ACP inventory, verifying SD paths and device.
3. Test an install/uninstall/rename change matrix: games, updates, DLC, forwarders, Homebrew, and JPN/EUR games. Avoid changing real Menu saved order.
4. Make a deterministic Barista layout planner with protected-slot reservation, user order preservation, capacity and byte-diff auditing.
5. Develop **cold-switch only**, with offline SD saves, backup/recovery and rollback tested on hardware before enabling automatic activation.
