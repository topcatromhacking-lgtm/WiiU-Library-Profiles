# ACP product-code platform hints

## Purpose

The v0.1.1 probe calls `ACPGetTitleMetaXml` to read installed Wii U title metadata. The offline preview may use the **product-code family** as a preliminary platform tag. This does **not** prove what ROM or executable is inside a modified/injected title.

The `product_code_platform_hint` function recognizes the following patterns for **base** installed titles (`00050000`). The letters below are the first one or two characters of the four-character product identifier after `WUP-N-`.

| ACP product code pattern | Platform hint |
|---|---|
| `WUP-N-F...` | NES / Famicom |
| `WUP-N-J...` | SNES / Super Famicom |
| `WUP-N-N...` | Nintendo 64 |
| `WUP-N-D...` | Nintendo DS |
| `WUP-N-PA..`, `PB..`, `PC..`, `PD..` | Game Boy Advance |
| `WUP-N-PN..`, `PP..` | PC Engine / TurboGrafx-16 |
| `WUP-N-MN..` | MSX |
| `WUP-N-V...` | Wii software wrappers |
| `WUP-P-....` or `WUP-U-....` | Wii U native application family |
| Other/unrecognized | Unclassified (no automatic guess) |

The `MCP_APP_TYPE_GAME_WII` app type separately signals a Wii software wrapper. In all cases, the **origin** field stays `unknown` until a user classifies it. The original injected ROM can differ from these wrapper hints. Explicit full Title ID overrides take precedence over a product-code hint, so SNES-donor Game Boy Color injection can be corrected as `gbc + injection`.

**Unknown is preferable to misclassification.** Do not infer that `WUP-N-` is always Virtual Console: some standalone apps use the same code class. No injected-game ROMs, titlemap files or NAND data are scanned.

## Scope of verification

An initial real-console MCP catalog exported 660 unique records from a >300 installed-title library. The v0.1.1 ACP test returned 511/511 successful base-title metadata lookups and 149 intentionally skipped non-game records. The test inventory remains private and is **not** part of the repository.

In that diagnostic inventory, product-code family hints classified most base titles, with two unrecognized code families. This is an offline metadata classification success, **not** verification of the Wii U Menu enumeration hook, real game content or all possible donor/injection types.

## Offline usage

```sh
python -m wiiu_library_profiles wiiu-library-profiles-metadata-v011.json
python -m wiiu_library_profiles wiiu-library-profiles-metadata-v011.json \
  --profile examples/snes-profile.json
```

Platform family selectors fit within the v0.1 preview budget for the tested console, but **Homebrew on Menu adds synthetic icons inside the Wii U Menu process**. An offline ACP inventory can omit those synthetic icons. Always account for them and keep recovery paths accessible before enabling any future Menu filter.

Source background: [WiiUBrew Title](https://wiiubrew.org/wiki/Title), [Nintendo Wii U VC systems](https://www.nintendo.co.jp/wiiu/software/vc/about/index.html), [Cemu Wiki VC](https://wiki.cemu.info/wiki/Virtual_Console).
