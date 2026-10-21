# Optional v1.3.4 PC test

1. Install Python 3.10+ if not already present; unzip the PC-test bundle.
2. On Windows, double-click `run_v134_pc_test.cmd`. The script runs synthetic
   unit tests and creates `v134-pc-test-result.json`. No Wii U hardware needed.
3. Optional: to recheck your *previous* attestation without another SD read,
   copy the four files into `PRIVATE_INPUTS/`, using exact names below:
   - `Barista-copy1.dat`
   - `v132-barista-attest.csv`
   - `v132-fixed-icons.csv`
   - `v08-menu-report.csv` (rename any earlier numbered download)
4. Run again after moving the previous result file aside. Upload **only**
   `v134-pc-test-result.json`. Keep all four console-specific files private.

Without a native C compiler, two optional C-integration tests skip on Windows,
which is expected. The GitHub CI workflow executes those tests on Linux.
**Do not install this bundle on the Wii U.** It is not a plugin or an active
Menu-use test, and should not be used to activate the 277-game profile.
