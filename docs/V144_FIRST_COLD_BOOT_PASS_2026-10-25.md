# WULP v1.3.14 — first cold boot PASSED on real Wii U

Date: 2026-10-25. **Hardware-reported result**, not a simulated PC test.

## Observed results

The owner reports:
- First launch after the v1.3.14 guarded upgrade: **277 selected games**.
- First full power-off and cold boot: **277 selected games remain**.

This is the first reported cold-boot selection persistence success in the v141d→v142a→v144 series. Earlier builds displayed 277 on initial boot and survived game launch/return but reloaded an unexpected library after a cold boot. The v144 candidate retains the immutable reference, the independent paired 284-entry allowlist, narrow 277-title working-save constraints and the fallback filter, and adds bounded cumulative opaque-metadata drift plus an SD-ready recheck.

**Not yet established in v144:** a second cold boot; a game-launch-and-return cycle on this exact version; icon ordering across reboots; absence of unauthorized NES icons or blank white pages (the owner reported the 277 title count only); durable switching from extended profile to legacy and back. Do not label as stable v1.0 or complete.

## Owner preference for next installers and diagnostics

The user's SD root is **`E:/` by default**. Future Windows scripts must default to this root without requiring repeated prompt input. Provide an obvious manual override at the script's top or accept an explicit `--sd` CLI option; user will edit the path if the drive letter changes. Normalize `E:/` to `E:\\` only for display; Python `pathlib.Path('E:/')` is an acceptable Windows root. Never hardcode a console serial or upload private Barista material to public GitHub.

## Next test

Before issuing another build, use the installed v144:
1. Verify selected platforms are still 140 Wii U, 32 Wii, 31 NDS, 74 GBA, with fifteen protected icons.
2. Launch a representative Wii U game and return to Menu; check for NES additions or extra blank pages.
3. Cold boot once more (second cold boot) and verify total selection, protected icons, layout and absence of unexpected titles/pages.
4. Only then perform profile switching and longer-term persistence tests with backed-up SD data.

No reinstall, new PC-only synthetic test or Barista modification is needed for this next milestone.

## Later owner update — third cold boot and game return PASS

The owner subsequently reported **277 games after the third full cold boot, with no change in icon order**. The owner also launched a game and returned to the Wii U Menu: **277 games remained, with no change in icon order**.

Accordingly, v1.3.14 **passes the single-profile hardware persistence milestone** for the observed 277-game selection. This is grounded in on-console owner testing, not PC simulation. Other behaviors such as changing between profiles, Menu folder editing, title deletion, altered SD media, multiple accounts, and long-term metadata drift remain untested. The report did not explicitly enumerate white pages or unauthorized NES titles; do not claim individual visual checks that weren't reported.

### Next engineering target: profile switching without undoing the proven state

**Do not implement the legacy switch as a bare marker flip.** In v144 the integrated MCP filter is explicitly active only when the extended marker + paired immutable reference validate; a legacy marker removes that filter and risks exposing the full unselected game library. The live legacy Barista was also mutated during earlier fallback boots and must not be assumed to equal the original 141-icon layout.

Safe switching must keep the current v144 277-game golden/working/allowlist unchanged, reconstruct the legacy profile from the existing PC pretest recovery, pair it with a verified 141-game title allowlist, and pre-validate both destination saves before changing one selector. Restore the old plugin/filters only through the existing protected rollback workflow. New Windows tools should default to `E:/`, with a user-editable SD root override.
