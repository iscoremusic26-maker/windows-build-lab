# CP21 private installation candidate

For Dusklight 2.0.3, Windows x64, DirectX 12 only. Not a public release.

With Dusklight closed, install exactly these two files in `%APPDATA%\TwilitRealm\Dusklight\mods` and enable both in the mod menu:

- `EnemyVariantFramework_0.15.0.dusk` — **Enemy Variant Framework**
- `BokoblinExpansion_0.17.0.dusk` — **Bokoblin Expansion**

Keep one installed bundle for each technical ID. Move older bundles with the same IDs to a backup outside the mods folder. No editor or encounter file is needed. CP21 ignores existing `placements.json` and `encounters.json`; it does not change them. The Framework has no development menu. Existing native, V1 and V1.1 service IDs and ABI remain unchanged.

The Expansion embeds the user's 32 enabled encounters, in their original order: 15 Blue/E_OC, 7 Red/E_OC2 and 10 Black/E_OC2 with 440 HP, F_SP121 layer 6. Every coordinate, yaw, ID, resource, parameter and authorization comes from the frozen user document. Its GitHub LF form is 18,449 bytes, SHA256 `786afe55dfadd0811cb53012aec7417f0886d2874cab63b0f9ae87ee01487e53`; restoring CRLF exactly reproduces the 19,164-byte original, SHA256 `67a1d7109949e9a18071ff06592f24c2cf0d54093cf76cc07efd6dad730e74aa`.

Original icons and banners are copied byte-for-byte into each bundle's `res/` directory. Compilation rejects changes to the frozen JSON or images. The official manifest/RES_DIR interface is checked against Dusklight commit `40457c6adb381928e4b5fef6ed459ed291edd5e2`.

`TEST_RESULTS.json`, `VERIFICATION_MANIFEST.json`, `SOURCE_PIN.json` and `SHA256SUMS.txt` identify the exact compiled candidate. `SOURCE_COMPILED.zip` includes CP21 sources and the separate, unchanged CP20 compiled regression snapshot. Historical editor sources are regression evidence only; no editor executable is distributed or required.

Automatic laboratory and Windows checks are not gameplay observations. **HUMAN_RUNTIME_PENDING**. In particular, room reentry, actual actor appearance, combat and native audio still require the attached human test. The historical Red `LINK_LAYOUT_CANARY_FAILED_LIFESTEAL_DISABLED` remains a known pending risk; CP21 does not claim to fix lifesteal or change combat.

Both PRs stay DRAFT. No merge or release is authorized. Public release requires successful human testing followed by separate approval.
