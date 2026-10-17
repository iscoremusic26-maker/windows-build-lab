# CP10.2 FIX01 — implementation and evidence

## Where the texture is bound (Dusklight 40457c6, Aurora 3227d76)
1. `J3DModel::createMatPacket` (libs/JSystem/src/J3DGraphAnimator/J3DModel.cpp ~L159-181) allocates `mMatPacket` **per model instance** and calls `matPacket->setTexture(pModelData->getTexture())` once.
2. `J3DMatPacket::draw` (J3DPacket.cpp L213-218, TARGET_PC): `j3dSys.setTexture(mpTexture); mpMaterial->load();`.
3. `J3DMaterial::load` (J3DMaterial.cpp L223-227, TARGET_PC) → `mTevBlock->loadTexture()` → `j3dSys.getTexture()->loadGX(texNo, map)` → `GXLoadTexObj(&mpTexObj[idx])` (J3DTexture.cpp L7-26).
4. Aurora emits the GXTexObj metadata (data pointer, texObjId) and resolves GPU textures by `texObjId`/`texDataVersion`, then by content hash (aurora lib/gx/texture.cpp `resolve_static_texture`). A private GXTexObj over private bytes therefore becomes its own GPU texture.
The texture is chosen **before** `drawFast`, so a drawFast hook cannot change it — which is why CP10.2 could only tint.

## Routes compared
| Route | Isolation | Why chosen / rejected |
|---|---|---|
| A. TextureService / `textures/` overlay replacement (by pointer or source hash) | Global per key: every E_OC2 sharing the atlas changes | Rejected: cannot keep Normal and Black side by side |
| B. Clone J3DModelData / material DL per actor | Strong, but large lifetime surface (materials, shared DLs, anim) | Rejected: more ownership risk than needed |
| C. **Private J3DTexture bound to the actor's own J3DMatPackets 0 and 1** | Per model instance by construction; shared data only read | **Chosen**: uses the exact native path (steps 2-3), no hook in the draw, no global state |

## Mechanism
- At the Black actor's first native `execute` (same exact-record admission and HP write as CP10.2) the Expansion verifies the CP04.4/CP10.2 effective-resource contract (3 materials `oc_body_mat01/02`, `oc_eye_mat`, TEV/colour channels, one 128×128 CMPR atlas shared by the three texture slots, no vertex colours) and that all three packets still point to the model data's texture.
- Private texture (built once per distinct atlas, pointer+FNV hash keyed, max 4): copy 8192 bytes → `devf_tex::recolor_cmpr` → three ResTIMG copies whose `imageOffset` points into the private block → `new J3DTexture(3, res)` (native constructor; `J3DTexture::loadGXTexObj` creates private GXTexObjs).
- Binding: `body0/body1->setTexture(private)`; eye packet, J3DModelData, the shared ResTIMG/bytes/GXTexObjs and every other model are untouched (`texture_binding.hpp`). Packets are owned and freed by the game with the model; re-entry creates new packets and binds again.
- Shutdown: live bound packets are pointed back to the native texture. Private textures are kept for the session (≤4 × ~8.3 KB) so no destroyed-but-unfreed model can reference freed memory.
- Fail-closed: any contract miss → `BLACK_TEXTURE_REJECTED reason=...`; that actor keeps the native look; HP route unaffected.
- Logs (once per event, never per frame): `BLACK_TEXTURE_PREPARED`, `BLACK_TEXTURE_BOUND`, `BLACK_TEXTURE_RELEASED`, `BLACK_TEXTURE_RESTORED`, `BLACK_TEXTURE_REJECTED`, plus `HP_APPLIED`.

## Colour design (expansion/cmpr_recolor.hpp, preset `kCharcoal`)
Measured locally on the user's own `oc2.bmd` atlas (not stored anywhere): 1024 sub-blocks, endpoint hues cluster at 320-360° (skin), 20-40° (leather), 40-80° (cloth), neutral (chain mail/metal). Skin weight = hue band 325°±35° (feather 20°) × chroma ≥4..13; skin → luma×0.80, slate tint (0.95, 1.00, 1.12), 12% chroma kept; others ×0.92. Integer-only math. Local check on that atlas: 928/2048 endpoints classified skin, 0 mode/index violations.

## Tests
`tests/texture_lab.cpp` (20 cases, synthetic): 565 round-trip (all 65536), neutral/eye/leather/cloth/metal keep hue, skin → charcoal with luma ~80% and reduced chroma, hue band monotonic, four-color mode kept with swaps and endpoint collisions, three-color transparency kept, indices never re-quantized, malformed sizes rejected, bit-exact golden hash of a synthetic atlas, original buffer untouched, Normal+Black same resource isolated, two Black + one Normal, fail-closed packet states, cache keyed by pointer+content, re-entry and idempotent restore. Plus CP10.2 labs (66 runtime, 26 policy) and editor self-tests.

## Not verified
In-game appearance, interaction with other texture-replacement mods for E_OC2 (FIX01 recolors whatever atlas is loaded), HP values other than 150.
