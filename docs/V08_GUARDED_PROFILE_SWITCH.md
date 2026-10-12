# v0.8: Guarded title allowlist and account-scoped cold switch (experimental)

**Build status: COMPILING successfully with pinned devkitPPC/WUPS toolchain.
Hardware status: only INERT installation is authorized initially.
Live sparse-profile filtering and persistence have NOT been validated.**

The primary account's five original games were restored in v0.7. With two
experimental project Homebrew apps excluded by `sd:/wiiu/apps/.ignore`, their
positions display correctly, persist after a cold reboot, and remain correct
after launching and exiting a game. That working configuration is the rollback
baseline for v0.8. Do not modify `BaristaIconDataBase.dat`, `layout.dat`
or `BaristaCommonSaveFile.dat`.

## Code

`console_menu_profile_switch/` provides the experimental guarded
`MCP_TitleList` Menu hook. It DOES NOT write Barista or installed titles.
It stays inert without an exact `ENABLE-V08-EXPERIMENT.txt` marker in
`sd:/wiiu/library_profiles/v08`.

For any active filter, it requires all of these checks:

1. Valid exact text config: account, console serial, and profile choice
   (`wiiu` or `nes` only); no arbitrary path input or profile wildcard.
2. Current Wii U Menu account matches the selected account via
   `nn::act::GetPersistentId`.
3. Profile's `WLPA` manifest passes length, version, count, sorted unique
   64-bit Title IDs, and CRC32 checks.
4. The EXACT current account's redirected, on-SD 130,116-byte Barista file
   has SHA-256 matching the manifest's embedded digest.
5. This particular `MCP_TitleList` result has every selected base game and
   selected synthetic Homebrew Title ID, with a plausible output-buffer
   capacity and successful result.

Only then may the hook compact unwanted `00050000` game type entries and
`0005000F` synthetic Homebrew from the Menu's in-memory MCP list. It
passes through system, update, DLC and unknown records unchanged.
If any guard fails, the original list is returned unchanged (fail-open).
A short counts-only diagnostic `v08-menu-report.csv` is written on an
enabled Menu session; no private Title IDs are logged.

**Fail-open is NOT a proof of safe sparse layouts:** an invalid config,
partial title enumeration, hot account switch or Barista checksum mismatch
could allow the stock Menu to refill intentionally empty positions. Hence
the first actual sparse-profile hardware test requires a physical SD
recovery backup and explicit power-off cold-switch procedure.

## Private data

The account-specific 141-game Wii U and 95-game NES Barista/allowlist pairs
were rebuilt offline in v0.6 and revalidated for v0.8. They preserve the 15
fixed tools/system positions from the working account and leave 144/190 game
positions empty, respectively. Every manifest is SHA-256 bound to its
specific Barista image. The user's private save files, real serial, inventory
and game-title IDs are distributed **only in a private conversation ZIP**,
not in this public repository.

A private stdlib Python cold-switch preflight/rollback tool:
- Checks the running source is the exact known working Barista and both
  `.ignore` probe exclusions are present;
- Refuses older Library Profiles hooks in the plugins directory;
- Creates a verified **off-SD PC recovery backup**;
- Has separately consent-gated power-off `arm`/`rollback` logic for
  later controlled hardware testing, not this initial Stage A.

## Validation

- 13 local Python cold-switch/recovery tests passed (both profiles, wrong
  hashes/manifest, two-app ignore, old WUPS conflict, off-SD backup,
  missing consent and post-boot overwrite recovery).
- Portable C SHA-256 published empty-string/`abc` vectors and game/synthetic
  classification passed with GCC.
- Independent private inventory/manifest matching: 511 installed games,
  141 Wii U allowed, 95 NES allowed, seven retained Homebrew with both project
  probes excluded by `.ignore`. Metadata is never intentionally filtered.
- GitHub Actions builds `wiiu_library_profiles_v08_guarded.wps` from this
  branch; the CI artifact intentionally omits marker and private manifests.

**First console gate:** copy only the compiled WUPS `.wps` with marker absent,
cold boot working restored account, and verify all 15 fixed icons and five
original games survive. Do not activate a sparse profile until this stage
passes and a recovery backup is verified. Do not merge as production software.
