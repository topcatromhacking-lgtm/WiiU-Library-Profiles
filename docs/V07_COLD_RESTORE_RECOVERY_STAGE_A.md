# v0.7 Cold Restore Safety: Stage A only

## Scope

The account-specific v0.6 **full 300-icon restoration** is reconstructed and byte-audited offline. Its five restored Wii U games replace two temporary Homebrew probe records and three GBA placement records. The sparse Wii U-only and NES-only layouts are **not** being installed.

v0.7 starts with a **PC-side preflight and off-SD backup of the currently active save**. No console action and no filtering are authorized at this stage. The test account is the owner's primary Wii U Menu account; the other account's Barista remains untouched.

A private standalone Python utility was created and privately distributed. It supports:
- Scan one unique serial-scoped redirected active Barista file under `wiiu/homebrew_on_menu_plugin/<serial>/save/<account>/`;
- Validate exact 130,116-byte known baseline SHA-256 before preparing a restore;
- Validate the proposed full-layout candidate independently by SHA-256 and preserve it in a separate private working package;
- Save a byte-exact recovery backup and typed recovery plan **on the PC, not the SD**. Refuse an existing recovery directory, unknown account, unexpected Barista hash, or SD-resident backup;
- Future *explicitly opted-in* cold install and restore operations, not part of this first user-stage procedure. The implementation requires a Wii U off/SD-in-PC flag, typed consent, and no obsolete Library Profiles `.wps`. Restore records a second copy of any Menu-resaved candidate, and refuses unexpected hashes without extra consent.

A local unit test suite passed **13 checks**, covering read-only preflight, backup refusal cases, account binding, all 26 expected modified-byte offsets, synthetic install/rollback, and preservation of the secondary account.

## Hardware safety constraints

1. There is still **no proof** the Wii U Menu will preserve a restored layout when it starts and re-saves Barista.
2. A ZIP containing a generated `.dat` is **not** permission to install it by hand. The initial command should be **scan**, followed by **prepare** after the active save's hash is checked; then stop and have the recovery plan verified.
3. The first proposed on-console experiment would be **full original-style five-game restoration only**, with all 300 icons occupied and the old filter removed. This remains experimental; it is not sparse-platform switching.
4. The two platform-only snapshots contain intentional blank positions. They require a separately validated **Menu-scoped synchronized strict allowlist** before any on-console use.
5. Do not modify Barista while Wii U Menu is running. Save redirection is per account and per console serial. The plugin may rewrite or migrate its redirected SD save; unknown hashes and mismatched paths are hard stops.

All private Barista source bytes, specific inventory, serial/account data and working pilot tool are distributed only to the owner. They are not uploaded to the public repository.

**Status: Stage A preflight package built locally; console installation not attempted.**
