# v1.3.2: Read-only account save attestation

**Experimental, host regressions and Wii U cross-compilation passed. Hardware validation pending.**

This builds on the *hardware-confirmed* v1.2 Menu profile preview (511 installed games; 277 selected, no overflow), v1.1.1 database matching and v0.8.1 filtering. The installed-title filter and Menu callback are identical to v1.2 except for one independently enabled read-only attestation call. No Barista save or allowlist is modified.

The optional plugin reads the Homebrew-on-Menu *candidate* redirected account save at:

```
fs:/vol/external01/wiiu/homebrew_on_menu_plugin/<cfg-serial>/save/80000001/BaristaAccountSaveFile.dat
```

It obtains the active account with the existing `nn::act::GetPersistentId()` gate, explicitly restricts v1.3.2 attestation to `80000001`, requires the original validated v08 manifest and Menu title scan, and reads the 130,116-byte file **once**. Strict C11 parser checks 600 main slots, all 15 protected first-page entries, shadow USB slots, unique installed game IDs and 60 completely empty folder banks. Missing/corrupt/unexpected saves produce a failed status **without any effect on the original Menu filter**.

The file path is constructed using the previous `active-profile.cfg` serial, but the SD copy **does not itself prove that the Homebrew-on-Menu redirection layer was active when the Menu accessed it**. The output explicitly states `redirected_copy_is_active=UNPROVEN`. The plugin never mounts MLC/NAND, scans another account, copies/mirrors saves or invokes Menu Sort Aroma. Any write operations are limited to the two new diagnostic CSVs under the already-established v08 diagnostics directory.

## Hardware test: two phases

1. Power Wii U **OFF** and back up your current proven WUPS, Barista, both original allowlists, `active-profile.cfg` and markers to PC. Do **not** install two Library Profiles `.wps` files simultaneously.
2. Install only `wiiu_library_profiles_v132_save_attest.wps` under `sd:/wiiu/environments/aroma/plugins/`. Do **not** add the v1.3.2 marker yet. Existing `ENABLE-V08-EXPERIMENT.txt`, the profile and allowlists stay unchanged. Temporarily remove independent v10/v11/v12 diagnostic markers. Boot and confirm your original Wii U/NES profile works.
3. Power **OFF**. Copy the exact LF marker `ENABLE-V132-ATTEST.txt` to `sd:/wiiu/library_profiles/v08/` (file contains `ENABLE-WLP-V132-ATTEST\n`). Boot normally once; then power off and retrieve `v132-barista-attest.csv` and `v132-fixed-icons.csv`.
4. Successful structure result: `V132,STRUCTURE_OK,...,account=80000001,bytes=130116,...,folders_empty=60,fixed_icons=15,redirected_copy_is_active=UNPROVEN`. A file hash and full source path are present. This **does not** authorize replacement.
5. After a **complete power-off**, independently copy the candidate Barista file TWICE from the precise SD path in the report to the PC. The user's private ZIP includes a PC-only gate that checks both copies, exact hashes, protected prefix, suffix, folder layout, the 511-game fingerprint and the 277-game candidate/WLPA; it generates only non-installable JSON/text.
6. For immediate rollback, power off; remove the v132 marker and restore the old WUPS. No Barista, WLPA, cfg, SDUSB or NAND changes are needed.

## Evidence and remaining gate

- Portable C11 checker verifies the known root format and 60 folders; repository tests include corrupt/truncated saves, abnormal flags, duplicate IDs and occupied folder.
- GitHub Actions workflow `.github/workflows/v132-save-attestation.yml`: host regression test confirms v1.2 callback and all other filtering dependencies are unchanged; pinned Wii U cross-build passed.
- The private offline bundle includes 33 passed Python regression tests and an independently verified 277+7 Barista/WLPA proposal bound to an exact source.
- An actual redirection-layer confirmation, backup provenance and safe multi-file logical transaction still need separate hardware validation. **There is no deployable Barista DAT, WLPA allowlist, multi-profile activation or save writer.**

Related upstream: [Homebrew-on-Menu SaveRedirection.cpp](https://github.com/wiiu-env/homebrew_on_menu_plugin/blob/main/src/SaveRedirection.cpp) establishes the current-user serial-scoped SD save copy and copies from original only if missing; [Menu Sort Aroma](https://github.com/Yirr777/menu_sort-aroma/blob/main/src/main.c) has root/folder structure and may mirror redirected save changes back to system storage, so it must not run during future controlled testing.

Private source file listings, actual 511 installed Title IDs, active account report and historical Barista binaries are **not** committed to GitHub.
