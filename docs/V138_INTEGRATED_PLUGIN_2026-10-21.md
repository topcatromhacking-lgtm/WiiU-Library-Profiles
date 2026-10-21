# WULP v1.3.8: compiled integrated Wii U plugin — 2026-10-21

**Build: PASS. Hardware activation: NOT YET TESTED.**
GitHub Actions: https://github.com/topcatromhacking-lgtm/WiiU-Library-Profiles/actions/runs/39681354740

## What exists

- A genuine **238664-byte Wii U PowerPC `.wps`** produced with pinned
  `wiiu-env/homebrew_on_menu_plugin` commit
  `c3b6d06301ba97f479da6227c8dd95bbda01d29f`.
- The build is reproducible from `tools/build_v138_homp.py` and
  `.github/workflows/wulp-v138-integrated-beta.yml`.
- The patch extends the **existing** SaveRedirection init rather than adding a competing SAVEInit hook.
- Only account `80000001` can opt in. The default is the original Homebrew-on-Menu
  `homp_save_redirection` path.
- A bounded file reader and portable SHA-256/C++ gate validate the marker, full
  130116-byte candidate, 2320-byte paired WLPA, 277 distinct game records,
  seven first-page homebrew records, root preservation, CRC and content hashes.
- When verified, the selected directory controls the save redirect, and the
  paired WLPA Title IDs filter `MCP_TitleList` **in the same plugin**.
  Non-game/system title categories pass through.
- Missing marker, incomplete profile, wrong account or mismatched data select
  the **legacy path**. There is no installation code or save overwriter.

## Build-only verification

GitHub runner native SHA-256 vectors passed, source anchors were checked against
the pinned upstream checkout, source was patched, Docker devkitPPC/WUT/WUPS
build completed, and the ELF-derived WUPS artifact was uploaded.
Downloaded `.wps` passed SHA256SUMS and identified as **32-bit big-endian
PowerPC Cafe OS**.

Previous hardware result: v1.3.5 redirected read of all 130116 bytes matched
the user's private Barista snapshot.

## Original 277-title dataset privately recovered

The user's private v1.3.2 archive (not committed publicly) contains three
historical CSV inputs. Recovered them within the private TopCat archive context.
Local cross-reconciliation showed: 610 total inventory rows, 511 distinct GAME
Title IDs, 511 audit records (510 verified), 277 unique selected games,
and **zero** ordering, installed-set or classification mismatches.
Platform counts: 140 Wii U, 32 Wii, 31 NDS and 74 GBA.
The exact Title IDs, private CSVs and Barista save have **not** been placed in this
public repository. A new console-specific Barista/WLPA pair cannot be built
without an independently verified private baseline copy.

## Remaining hard gates

1. **MCP enumeration completeness/capacity:** real `MCP_TitleList` must yield all
   selected titles. The current hook filters its returned list; it does not
   demonstrate that 511 installed apps can all be enumerated into the buffer
   passed by Menu. Test on hardware before enabling the extended profile.
2. **Plugin conflicts:** the previous v0.8 WULP filter may also hook title
   enumeration. Do not simultaneously activate conflicting filters until
   hooks are reconciled.
3. **Persistence:** the Menu may write to the selected Barista file. That changes
   its hash, potentially causing next-boot legacy fallback. v1.3.8 does
   not implement the required durable commit/journal for normal Menu writes.
4. **Hardware:** No test of actual extended rendering, cold-boot recovery,
   title launch/return or 277-game profile switching has occurred.
5. **Reversibility:** save a backed-up original
   `sd:/wiiu/environments/aroma/plugins/homebrew_on_menu.wps` before any
   future manual beta replacement. Restore that exact original to remove this
   experiment. **Do not replace original Barista or WLPA.**

The compiled artifact is a valuable **integration alpha**, not an approved
production profile switcher. No additional PC-only user tests are requested.
