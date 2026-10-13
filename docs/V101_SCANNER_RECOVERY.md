# v1.0.1: isolate a frozen v1.0 scanner

The initial v1.0 standalone WUHB froze on actual Wii U hardware: a persistent
red screen and no HOME response for ten minutes. **Treat that binary as
failed; don't relaunch it.** The root cause is not proven. The first v1.0
program performed `MCP_TitleList`, metadata file reads, directory enumeration
and CSV output *automatically* before the user had any exit option. It also
passed a null error pointer to `VPADRead`. Either hardware launch behavior
or one of those operations could be involved.

This recovery diagnostic changes the launch sequence:

1. Main starts a normal WHB ProcUI/console loop and initializes GamePad input.
2. **No SD mount or file I/O, MCP enumeration or homebrew scan occurs on
   launch.** The startup screen explicitly instructs B to exit.
3. The first, controlled hardware test is to launch and press **B only**.
4. Only after we see a clean open/exit should a separate later run press
   **A** to perform the original read-only scan. It produces the same CSV
   filenames in `sd:/wiiu/library_profiles/v10/`.
5. `VPADRead` now receives a valid `VPADReadError` pointer.

This build is **diagnostic and unproven on hardware**; it does not establish
that the previously crashing operation has been found or fixed.

Do not touch the validated `wiiu_library_profiles_v081_hash_advisory.wps`,
Barista, `.ignore` probe exclusions, or profile allowlist. Keep the
scanner itself excluded by appending `wlp_inventory_scanner_v101/wlp_inventory_scanner_v101.wuhb`
to `sd:/wiiu/apps/.ignore` before using Aroma.

When the console has recovered, remove
`sd:/wiiu/apps/wlp_inventory_scanner/wlp_inventory_scanner.wuhb` (the faulty
v1.0 scanner). Before testing any replacement, note whether the frozen screen
was uniformly red, showed an error code, or had any console text. These are
different failure modes.
