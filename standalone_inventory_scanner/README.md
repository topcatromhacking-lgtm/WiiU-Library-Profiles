# v1.0 Stage A: Read-only standalone scanner (.wuhb)

This is a **standalone Homebrew Launcher application**, not a Menu plugin. It does not replace or interfere with v0.8.1's \`MCP_TitleList\` Menu-only hook. It does not change Barista, title installation, \`.ignore\`, the selected profile, or Menu layout.

The app calls \`MCP_TitleCount\` / \`MCP_TitleList\` from its **own process**, so the Menu-only title filter should not hide titles from its inventory. It scans **installed titles** and SD homebrew apps (up to two directory levels below \`sd:/wiiu/apps\`). It exports:
- \`sd:/wiiu/library_profiles/v10/v10-installed-titles.csv\`
- \`sd:/wiiu/library_profiles/v10/v10-homebrew-apps.csv\`
- \`sd:/wiiu/library_profiles/v10/v10-summary.txt\`

The console MCP API identifies Title IDs, type, version, storage path and indexed device. **It does not provide reliable emulated-console classification for custom VC injections**. Thus, ordinary base games initially report \`WIIU_OR_VIRTUAL_CONSOLE_UNKNOWN\` and \`platform_confidence=UNKNOWN\`, while Wii-type app records may be identified from their app type. The scanner *attempts* to read installed \`meta/meta.xml\` from MCP paths; if inaccessible, \`display_name\` and \`region_inferred\` remain blank/UNKNOWN. It does not claim platform or region solely from guesses. Synthetic HB Title IDs and SD \`.wuhb\` paths are listed separately; correlating them is a **future step**, not a proven mapping. Files inside SD are never modified, only listed.

## Safe installation, especially with Homebrew on Wii U Menu

1. With Wii U off, preserve the working v0.8.1 plugin, Barista and \`.ignore\`.
2. Add this line to \`sd:/wiiu/apps/.ignore\` (append without removing existing lines):
   \`wlp_inventory_scanner/wlp_inventory_scanner.wuhb\`
   This prevents Homebrew on Wii U Menu from offering an unwanted additional Menu shortcut. Homebrew Launcher can still launch the WUHB.
3. Copy \`wlp_inventory_scanner.wuhb\` into \`sd:/wiiu/apps/wlp_inventory_scanner/\`.
4. Boot Aroma normally, use the Homebrew Launcher shortcut (R+L+Down as configured) and launch **WLP Inventory Scanner**. The app exports files automatically then shows a short summary; press **B** to exit.
5. Upload all three output files for reconciliation. If the 511-game count differs, compare exact Title IDs and MCP app types rather than assuming the profile filter removed games.

**If the app fails to mount SD or MCP enumeration:** the on-screen status and \`v10-summary.txt\` (when SD is writable) should disclose the failure. No Barista or game files are edited.

The current stage intentionally does not implement a profile selection UI or on-console Barista writing. Those depend on validated inventory and identification of VC inject metadata.
