# Fixed first page, prioritized remaining 19 pages (v0.3.2)

## Accepted UX contract

Stock Nintendo Menu's 300 paired Barista positions form twenty 15-icon
pages (for the specific provided Barista format):

- Page 1 (positions 0-14): mandatory console/system entries, followed by
  user-selected Homebrew and tools, followed by **placeholder folder
  proposals** if fewer than 15 icons are selected. The placeholders are
  NOT actual Barista folder records yet.
- Pages 2 through 20 (positions 15-299): at most 285 game/VC/application
  icons. Only requested platform families enter the list; extra unrelated
  titles **must not** automatically refill blank positions.

Within software pages, selected platforms appear in **explicit priority
order**. A platform gets all available software slots up to the remaining
capacity; later platforms get whatever remains. Excess games are recorded as
*not shown in this profile*, not deleted or silently lost from the inventory.

| Priority | Installed selected games | Visible games | Not visible |
| --- | --- | --- | --- |
| Wii U, Wii | 280 Wii U + 10 Wii | 280 Wii U + 5 Wii | 5 Wii |
| Wii U, Wii, NDS | 270 Wii U + 10 Wii + 10 NDS | 270 + 10 + 5 NDS | 5 NDS |
| Wii U only | 15 Wii U | 15 Wii U on page 2 | all other platforms |
| NES only | 15 NES | 15 NES on page 2 | all other platforms |

The second and subsequent unneeded pages stay empty, rather than showing
unselected games. In particular, the engine **must implement an allowlist** of
visible launchable Title IDs, including both the limited games and selected
Homebrew icons, plus mandatory system entries. Updates/DLC/system metadata
continue to be passed through to MCP but **are not game icons**.

## A previously hidden constraint verified against real Barista saves

The user's two historic 130,116-byte Barista saves contain:
- Barista 1: 8 protected system records at positions 0-7, then 9 Homebrew
  records at positions 8-16. Games start at position 17.
- Barista 2: 8 protected system records at positions 0-7, then 7 Homebrew
  records at positions 8-14. Games start at position 15.

Therefore **15 reserved Menu positions cannot mean 15 discretionary
Homebrew apps on this console**. They include 8 mandatory system entries,
leaving 7 explicitly selected Homebrew/tool slots. Barista 1 requires moving
or excluding 2 existing Homebrew icons outside that first page before we
can claim a first-page-only tools layout. The Menu's Homebrew Loader
shortcut (L+R+D-Pad Down) must remain available for others.

On other hardware/regions, the number, identity and positions of protected
entries must be scanned from that user's Barista save. If protected items
cannot fit in the first fifteen positions, the planner refuses to render.

## Stable refresh and classification

Discover games from read-only MCP/ACP, classify VC platform by recognized
product-code families + persistent exact-ID overrides; retain original
user ordering within each platform. On refresh, preserve known IDs in their
saved relative order, and append newly installed games deterministically
by name. EUR/JPN variants are classified from *their own metadata*, not
an American static Title ID table. New unknown titles stay in a review
category and can be enabled if explicitly requested. Selected Homebrew
has its own dynamic path-based catalog because synthetic 0005000F icons
may be missing from a standalone MCP probe.

If more than available tool slots are selected, do not truncate silently:
the UI must request the user to choose fewer tools. The others remain
available through the standard Homebrew Loader shortcut. If fewer are
selected, propose empty-folder placeholders, but never insert untested
folder records into a live Barista save. Folder IDs/metadata, 60-folder
budget, safe record relocation, and actual first-page UI behavior require
separate hardware validation.

The **offline-only** generic implementation is
`wiiu_library_profiles/menu_policy.py`, with synthetic regression tests
`tests/test_menu_policy.py`. It emits full 300-slot plan, ordered 285-slot
game list, allowlisted Title IDs, hidden ID list, per-platform truncation
counts, spare slots, and folder placeholder proposals. It does not mutate
any Wii U save or title inventory.

## Before console activation

1. Validate Homebrew-on-Menu synthetic Menu title enumeration and stable
   SD-path-to-Title-ID matching.
2. Validate folder placeholders and safe first-page allocation, including
   relocating Homebrew icons from positions 15 and 16 in historic Barista 1.
3. Build Barista files offline against a *same-console, same-region* known
   template, with checksums and explicit protected-field checks.
4. Match the Menu-scoped hook to the **same exact allowlist** used by the
   Barista builder. Never rely on the order of unfiltered MCP items or on
   the saved layout alone. Validate plugin chaining and buffer sizes.
5. Test cold-switch with backup and reliable rollback before enabling
   automatic refresh. Never write Barista while Menu runs.

The two homebrew probe applications created for the project are tool
candidates, not general Wii U games. Their synthetic Homebrew-on-Menu IDs
should be discovered independently from the installed-game MCP catalog.
Do not confuse these synthetic icons with unusual native-app
product-code families in ACP.
