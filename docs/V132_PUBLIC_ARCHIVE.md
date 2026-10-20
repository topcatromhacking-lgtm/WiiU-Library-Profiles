# Public v1.3.2 archive provenance and integrity

The full user-supplied `WiiU-Library-Profiles-v1.3.2-Barista-Attestation-EXPERIMENTAL.zip` contains three sensitive owner-specific CSVs under `PC_ONLY/PRIVATE/`. Its SHA-256 was `ef6c2ee6eb6836a204df84fa23ab3d3155503ea09fd79df846a5424744a36d3e`. That **exact original ZIP is not distributed publicly**.

The linked `WiiU-Library-Profiles-v1.3.2-Barista-Attestation-EXPERIMENTAL-PUBLIC-Redacted.zip` is a derived **privacy-redacted** archive that:
- Excludes the complete `PC_ONLY/PRIVATE/` subtree.
- Retains the optional compiled WUPS attestation module, enable marker, PC tooling and regression tests, with unchanged binary bytes in those retained original files.
- Replaces README private-data examples with references to locally supplied files in `/PC/user-private/`.
- Includes `PUBLIC-DISTRIBUTION-NOTE.md` and a regenerated internal `SHA256-MANIFEST.json`.
- Was ZIP-structure checked and SHA-256 checked for each internal payload; the source tests `test_v132_active_save_gate.py` (14 tests) and `test_v13_pair_generator.py` (19 tests) passed in a temporary extraction on October 20, 2026.

Public SHA-256: `8e55286a47eea4181527664c72c82a372a08e62c76a32f0b2e0dde140b226933`.
The archive is **experimental and not yet tested on actual Wii U hardware**. This is a Git-tracked archive, not a formal GitHub Release.

Original authorship belongs to TopCat's project. Do not distribute console-specific account saves, Title ID inventories, or owner-selected preview CSVs.
