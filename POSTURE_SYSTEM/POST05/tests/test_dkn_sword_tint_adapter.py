"""DKN01 FIX07 (0.13.16; FIX06 0.13.15 history below): execute the PRODUCTION sword darkening of src/dkn_adapter.inc (dkn_native_sword_color, on_dkn_draw_pre,
on_dkn_morf_entry_pre, dkn_tint_restore, on_dkn_morf_entry_post, on_dkn_draw_post) inside a model of the NATIVE daB_TN_c::draw() order of the 2.0.3 EXE, against
J3D material doubles that share ONE sword model data between Darknuts.

The draw() model is the measured call order (dkn_fix07_evidence/tools/dis_fn.py on dusklight.exe 2.0.3): settingTevStruct; setLightTevColorType_MAJI(body)
(0xcc866b: TEV color 0 of every body material = tevStr.TevColor); the body's own mMaterialColor writes (0xcc86d7..0xcc870c); mpModelMorf2->entryDL() (0xcc8731);
...; setLightTevColorType_MAJI(sword) (0xcc882d: TEV color 0 of the sword = tevStr.TevColor); mpModelMorf1->entryDL() (0xcc8839). What the GPU consumes is what
mDoExt_modelEntryDL diffs into the instance display list INSIDE entryDL: the double captures the material there (between the entryDL PRE and POST hooks).

Checks: the FIX06 route (write in the draw PRE, restore in the draw POST) is REPRODUCED as the cause of the human report (the native MAJI of the sword overwrites
it before the diff: the captured colour is tevStr.TevColor, never dark); the FIX07 route captures the body's own value in the sword's diff for the definitive
death only, frame after frame (progressive, no accumulation), only TEV color 0 (texture / lighting route untouched); every other instance / stage / the body /
every other morf of the game captures what the native gives; the shared data is exactly what MAJI left once entryDL returns; a lost POST never leaves a write
past the draw. Six mutants must be detected (FIX08: + the FIX07 raw body value); a compile error is never a detection. Execution with doubles: never a render or a visual certification."""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def body(text, signature):
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 0
    for end in range(opening, len(text)):
        depth += text[end] == '{'
        depth -= text[end] == '}'
        if depth == 0:
            return text[start:end + 1]
    raise ValueError(signature)


PREFIX = r'''
#include <cstdint>
#include <cstdio>
#include <cmath>
#include "dkn_core.hpp"
#define DKN_NOINLINE
typedef std::uint16_t u16;
struct ModContext {};
enum HookAction { HOOK_CONTINUE = 0 };
namespace mods { template <class T> T arg(void* args, int i) { return static_cast<T>(static_cast<void**>(args)[i]); } }
struct J3DGXColorS10 { short r = 0, g = 0, b = 0, a = 255; };
struct J3DMaterial { J3DGXColorS10 c; bool has = true; unsigned writes = 0; J3DGXColorS10* getTevColor(unsigned) { return has ? &c : nullptr; } };
struct J3DModelData { J3DMaterial m[3]; unsigned n = 1; u16 getMaterialNum() const { return static_cast<u16>(n); } J3DMaterial* getMaterialNodePointer(u16 i) { return &m[i]; } };
struct J3DModel { J3DModelData* d = nullptr; J3DModelData* getModelData() { return d; } };
struct mDoExt_McaMorfSO { J3DModel* model = nullptr; J3DModel* getModel() { return model; } };
struct GXColorS10 { short r = 0, g = 0, b = 0, a = 0; };
struct dKy_tevstr_c { GXColorS10 TevColor; };
struct fopAc_ac_c { std::uint32_t id = 0; };
struct daB_TN_c : fopAc_ac_c { int mType = 1, mActionMode2 = 2; float mMaterialColor = 0; bool mDisappear = false, mInactive = false; std::uint16_t mStates[16] = {};
                                mDoExt_McaMorfSO* mpModelMorf1 = nullptr; mDoExt_McaMorfSO* mpModelMorf2 = nullptr; dKy_tevstr_c tevStr; };
enum { fpcNm_B_TN_e = 531 };
std::uint32_t fopAcM_GetID(const fopAc_ac_c* a) { return a->id; }
daB_TN_c* g_actors[8] = {};
bool fopAcM_SearchByID(std::uint32_t id, fopAc_ac_c** p) { for (daB_TN_c* a : g_actors) if (a && a->id == id) { *p = a; return true; } *p = nullptr; return false; }
int fopAcM_GetName(const fopAc_ac_c*) { return fpcNm_B_TN_e; }
struct DknSlot { bool on = false; std::uint32_t id = 0; dkn::Ph ph = dkn::Ph::Light; std::uint64_t st_sword_tint_draws = 0, st_sword_tint_entries = 0; short tint_body_last = 1; unsigned tint_samples = 0; };
float dkn_frame(const daB_TN_c*) { return 79.0f; }  // FIX08: the death clip frame in the sampled SWORD_DARKENING_STEP line (telemetry only)
struct DknState {
  bool enabled = true;
  DknSlot s[8];
  J3DModelData* tint_data = nullptr;
  short tint_saved[8][3] = {};
  unsigned tint_n = 0;
  std::uint64_t tint_draws = 0, tint_restores = 0;
  const mDoExt_McaMorfSO* tint_morf = nullptr;
  short tint_value = 0;
  DknSlot* tint_slot = nullptr;
  std::uint64_t tint_armed = 0, tint_entry_writes = 0, tint_entry_restores = 0, tint_overwritten_by_maji = 0, tint_unrestored = 0;
} dn;
DknSlot* dkn_find(std::uint32_t id) { for (DknSlot& s : dn.s) if (s.on && s.id == id) return &s; return nullptr; }
void dkn_log(const char*, const DknSlot&, const daB_TN_c*, const char*) {}
HookAction on_dkn_morf_entry_pre(ModContext*, void* args, void*, void*) noexcept;
void on_dkn_morf_entry_post(ModContext*, void* args, void*, void*) noexcept;
'''

MAIN = r'''
unsigned checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { std::printf("FAIL line=%d: %s\n", __LINE__, #c); return 1; } } while (0)
void maji(J3DModelData& d, const dKy_tevstr_c& t) {  // setLightTevColorType_MAJI_sub: material->setTevColor(0, &tevstr->TevColor) (lightType 1)
  for (unsigned i = 0; i < d.n; ++i) if (d.m[i].has) d.m[i].c.r = t.TevColor.r, d.m[i].c.g = t.TevColor.g, d.m[i].c.b = t.TevColor.b;
}
J3DGXColorS10 entry_dl(mDoExt_McaMorfSO& mf, J3DModelData& d) {  // the hooked out-of-line entryDL: PRE, mDoExt_modelEntryDL's diff (captured), POST
  void* args[1] = {&mf};
  on_dkn_morf_entry_pre(nullptr, args, nullptr, nullptr);
  const J3DGXColorS10 diffed = d.m[0].c;
  on_dkn_morf_entry_post(nullptr, args, nullptr, nullptr);
  return diffed;
}
struct Drawn { J3DGXColorS10 body, sword; bool sword_drawn = false; };
// daB_TN_c::draw() of the 2.0.3 EXE, with its PRE / POST hooks; `fix06` = the FIX06 route (sword write before draw(), restore after it), for the reproduction
Drawn native_draw(daB_TN_c& a, J3DModelData& bodyd, J3DModelData& swordd, bool fix06 = false) {
  Drawn r;
  void* args[1] = {&a};
  on_dkn_draw_pre(nullptr, args, nullptr, nullptr);
  short f6[3] = {};
  bool f6on = false;
  if (fix06 && dn.tint_morf) f6on = true, f6[0] = swordd.m[0].c.r, f6[1] = swordd.m[0].c.g, f6[2] = swordd.m[0].c.b, swordd.m[0].c.r = swordd.m[0].c.g = swordd.m[0].c.b = dn.tint_value;
  if (fix06) dn.tint_morf = nullptr;  // FIX06 had no entryDL hook
  if (!a.mDisappear && !a.mInactive) {
    maji(bodyd, a.tevStr);
    if (a.mMaterialColor != 0.0f) for (unsigned i = 0; i < bodyd.n; ++i) bodyd.m[i].c.r = bodyd.m[i].c.g = bodyd.m[i].c.b = static_cast<short>(a.mMaterialColor);
    r.body = entry_dl(*a.mpModelMorf2, bodyd);
    if (a.mStates[15] != 7) {
      maji(swordd, a.tevStr);
      r.sword = entry_dl(*a.mpModelMorf1, swordd), r.sword_drawn = true;
    }
  }
  on_dkn_draw_post(nullptr, args, nullptr, nullptr);
  if (f6on) swordd.m[0].c.r = f6[0], swordd.m[0].c.g = f6[1], swordd.m[0].c.b = f6[2];
  return r;
}
bool same(const J3DGXColorS10& x, short r, short g, short b) { return x.r == r && x.g == g && x.b == b; }
int main() {
  J3DModelData body, sword, other_body;  // ONE sword model data for every mType-1 Darknut (B_tnp2 tn2_sword_b); body data shared too
  J3DModel mbA{&body}, mbB{&body}, mbC{&body}, mbD{&body}, msA{&sword}, msB{&sword}, msC{&sword}, msD{&sword}, mx{&other_body};
  mDoExt_McaMorfSO bA{&mbA}, bB{&mbB}, bC{&mbC}, bD{&mbD}, sA{&msA}, sB{&msB}, sC{&msC}, sD{&msD}, foreign{&mx};
  daB_TN_c A, B, C, D;
  A.id = 11, B.id = 12, C.id = 13, D.id = 99;
  A.mpModelMorf1 = &sA, B.mpModelMorf1 = &sB, C.mpModelMorf1 = &sC, D.mpModelMorf1 = &sD;
  A.mpModelMorf2 = &bA, B.mpModelMorf2 = &bB, C.mpModelMorf2 = &bC, D.mpModelMorf2 = &bD;
  g_actors[0] = &A, g_actors[1] = &B, g_actors[2] = &C, g_actors[3] = &D;
  A.mStates[15] = 1, B.mStates[15] = 1, C.mStates[15] = 2, D.mStates[15] = 1;
  dn.s[0] = DknSlot{true, 11, dkn::Ph::Dead, 0, 0};     // A: definitive death (F3 Finish / F3 HP0)
  dn.s[1] = DknSlot{true, 12, dkn::Ph::Revived, 0, 0};  // B: alive (F3)
  dn.s[2] = DknSlot{true, 13, dkn::Ph::Dkr, 0, 0};      // C: first defeat / recoverable fall (DKR01 owns it)
  // D: never admitted (ineligible Darknut)
  // 0) REPRODUCTION of the human report: the FIX06 route never reaches the consumed state (MAJI of the sword re-sets TEV color 0 from tevStr before the diff)
  A.mMaterialColor = -20.0f;
  {
    const Drawn d6 = native_draw(A, body, sword, true);
    CHECK(same(d6.body, -20, -20, -20));
    CHECK(same(d6.sword, 0, 0, 0));  // FIX06: the sword's diff got tevStr.TevColor (0), never the dark value: the cause of "the sword does not darken"
  }
  dn.tint_morf = nullptr;
  // 1) FIX07: progressive darkening, frame after frame, alternating instances; the sword's diff = the body's diff for A only
  float m = 0;
  for (int f = 0; f < 200; ++f) {
    m = m - 0.3f < -50.0f ? -50.0f : m - 0.3f;
    A.mMaterialColor = m, C.mMaterialColor = m, D.mMaterialColor = m;  // C / D: a darkening body must still not tint their swords
    const short v = static_cast<short>(m), w = dkn::sword_tint_value(m);  // FIX08: the sword's perceptual value for the body's v (same relative darkening after the clamp)
    const Drawn db = native_draw(B, body, sword), da = native_draw(A, body, sword), dc = native_draw(C, body, sword), dd = native_draw(D, body, sword);
    CHECK(same(da.sword, w, w, w) && same(da.body, v, v, v) && (v == 0 ? w == 0 : w < v));
    CHECK(same(db.sword, 0, 0, 0) && same(dc.sword, 0, 0, 0) && same(dd.sword, 0, 0, 0));
    CHECK(same(sword.m[0].c, 0, 0, 0) && dn.tint_data == nullptr && dn.tint_morf == nullptr);  // the shared data is what MAJI left, once entryDL returned
  }
  CHECK(dn.s[0].st_sword_tint_entries == 200 && dn.s[1].st_sword_tint_entries == 0 && dn.s[2].st_sword_tint_entries == 0);
  CHECK(body.m[0].writes == 0);  // never a write into the body (its own native write darkens it)
  // 2) a definitive death whose body is not darkening yet (mMaterialColor 0): nothing armed
  A.mMaterialColor = 0;
  const std::uint64_t armed0 = dn.tint_armed;
  CHECK(same(native_draw(A, body, sword).sword, 0, 0, 0) && dn.tint_armed == armed0);
  // 3) a non-zero tevStr colour (another room's light colour), several materials, one without TEV: the dark value captured, MAJI's values back exactly
  A.mMaterialColor = -33.9f, A.tevStr.TevColor = {5, 6, 7, 0};
  sword.n = 3, sword.m[2].has = false;
  {
    const Drawn d = native_draw(A, body, sword);
    CHECK(same(d.sword, -78, -78, -78));  // FIX08: body -33 -> sword kSwordTint[33] = -78 (FIX07 wrote -33)
    CHECK(same(sword.m[0].c, 5, 6, 7) && same(sword.m[1].c, 5, 6, 7));
  }
  // 4) two dead Darknuts with different values: each sword its own value
  dn.s[1].ph = dkn::Ph::Dead, B.mMaterialColor = -10.0f, B.tevStr.TevColor = {5, 6, 7, 0};
  CHECK(same(native_draw(A, body, sword).sword, -78, -78, -78) && same(native_draw(B, body, sword).sword, -25, -25, -25));
  // 5) the sword dropped by the death clip (mStates[15] 2): still the same morf, still darkened; state 7 / disappearing / inactive / disabled: nothing
  A.mStates[15] = 2;
  CHECK(same(native_draw(A, body, sword).sword, -78, -78, -78));
  A.mStates[15] = 7;
  CHECK(!native_draw(A, body, sword).sword_drawn && dn.tint_morf == nullptr);
  const std::uint64_t writes0 = dn.tint_entry_writes;
  A.mStates[15] = 1, A.mDisappear = true;
  native_draw(A, body, sword);
  CHECK(dn.tint_entry_writes == writes0 && same(sword.m[0].c, 5, 6, 7));
  A.mDisappear = false, dn.enabled = false;
  CHECK(same(native_draw(A, body, sword).sword, 5, 6, 7) && dn.tint_entry_writes == writes0);
  dn.enabled = true;
  // 6) every other morf of the game (another actor's entryDL while nothing / something is armed) is never written
  {
    other_body.m[0].c = {9, 9, 9, 255};
    void* args[1] = {&foreign};
    on_dkn_morf_entry_pre(nullptr, args, nullptr, nullptr);
    on_dkn_morf_entry_post(nullptr, args, nullptr, nullptr);
    void* aa[1] = {&A};
    on_dkn_draw_pre(nullptr, aa, nullptr, nullptr);  // A armed (sA)
    on_dkn_morf_entry_pre(nullptr, args, nullptr, nullptr);
    on_dkn_morf_entry_post(nullptr, args, nullptr, nullptr);
    CHECK(other_body.m[0].writes == 0 && same(other_body.m[0].c, 9, 9, 9));
    on_dkn_draw_post(nullptr, aa, nullptr, nullptr);
    CHECK(dn.tint_morf == nullptr);
  }
  // 7) an entryDL POST that never came (abnormal): the draw POST restores, nothing stays written past the draw; a later POST of another morf writes nothing
  {
    void* aa[1] = {&A};
    on_dkn_draw_pre(nullptr, aa, nullptr, nullptr);
    void* sa[1] = {&sA};
    on_dkn_morf_entry_pre(nullptr, sa, nullptr, nullptr);
    CHECK(same(sword.m[0].c, -78, -78, -78));
    on_dkn_draw_post(nullptr, aa, nullptr, nullptr);
    CHECK(same(sword.m[0].c, 5, 6, 7) && dn.tint_unrestored == 1 && dn.tint_data == nullptr);
    void* sb[1] = {&sB};
    on_dkn_morf_entry_post(nullptr, sb, nullptr, nullptr);
    CHECK(same(sword.m[0].c, 5, 6, 7));
  }
  std::printf("DKN_SWORD_TINT_ADAPTER_PASS checks=%u entry_writes=%llu restores=%llu maji_overwrites_seen=%llu fix06_route_reproduced=1 runtime=HUMAN_RUNTIME_PENDING\n", checks,
              static_cast<unsigned long long>(dn.tint_entry_writes), static_cast<unsigned long long>(dn.tint_restores), static_cast<unsigned long long>(dn.tint_overwritten_by_maji));
  return 0;
}
'''

FUNCS = ['DKN_NOINLINE void dkn_native_sword_color(', 'HookAction on_dkn_draw_pre(', 'HookAction on_dkn_morf_entry_pre(', 'void dkn_tint_restore(',
         'void on_dkn_morf_entry_post(', 'void on_dkn_draw_post(']
WRITER_STORE = '  J3DGXColorS10* c = m->getTevColor(0);\n  if (!c) return;'


def run_variant(source, directory, label):
    parts = [body(source, f) for f in FUNCS]
    cpp = directory / (label + '.cpp')
    # the double counts every store of the production writer (its body is unchanged: the counter lives on the material double)
    text = PREFIX + '\n' + '\n'.join(parts).replace(WRITER_STORE, WRITER_STORE + '\n  ++m->writes;') + '\n' + MAIN
    cpp.write_text(text)
    exe = directory / (label + ('.exe' if os.name == 'nt' else ''))
    inc = str(ROOT / 'src')
    if os.name == 'nt':
        cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/I' + inc, str(cpp), '/Fe' + str(exe), '/Fo' + str(directory / (label + '.obj'))]
    else:
        cmd = [shutil.which('g++') or 'clang++', '-std=c++20', '-O2', '-I' + inc, str(cpp), '-o', str(exe)]
    built = subprocess.run(cmd, capture_output=True, text=True, cwd=directory, timeout=120)
    if built.returncode:
        raise RuntimeError('compile failure never counts as detection: ' + built.stdout + built.stderr)
    return subprocess.run([str(exe)], capture_output=True, text=True, timeout=20)


def main():
    source = (ROOT / 'src/dkn_adapter.inc').read_text()
    for f in FUNCS:
        assert source.count(f) == 1, f
    assert source.count(WRITER_STORE) == 1
    with tempfile.TemporaryDirectory() as td:
        directory = Path(td)
        positive = run_variant(source, directory, 'positive')
        print(positive.stdout, end='')
        if positive.returncode:
            print(positive.stderr)
            return 1
        mutations = [
            # the FIX06 route again: the darkening written in the draw PRE (before the native MAJI of the sword) instead of inside its entryDL
            ('write_in_draw_pre_fix06', '  dn.tint_morf = a->mpModelMorf1, dn.tint_value = dkn::sword_tint_value(a->mMaterialColor), dn.tint_slot = s, ++dn.tint_armed;',
             '  dn.tint_morf = a->mpModelMorf1, dn.tint_value = dkn::sword_tint_value(a->mMaterialColor), dn.tint_slot = s, ++dn.tint_armed;\n'
             '  { void* x[1] = {a->mpModelMorf1}; on_dkn_morf_entry_pre(nullptr, x, nullptr, nullptr); on_dkn_morf_entry_post(nullptr, x, nullptr, nullptr); }\n  dn.tint_morf = nullptr;'),
            ('restore_removed', '    if (mat && mat->getTevColor(0)) dkn_native_sword_color(mat, dn.tint_saved[i][0], dn.tint_saved[i][1], dn.tint_saved[i][2]);', '    (void)mat;'),
            ('any_stage_darkens', '  if (!s || !dkn::sword_tint(s->ph == dkn::Ph::Dead, a->mMaterialColor)) return HOOK_CONTINUE;',
             '  if (!s || !dkn::sword_tint(true, a->mMaterialColor)) return HOOK_CONTINUE;'),
            ('any_morf_written', '  if (!dn.tint_morf || !args || mods::arg<const mDoExt_McaMorfSO*>(args, 0) != dn.tint_morf) return HOOK_CONTINUE;',
             '  if (!dn.tint_morf || !args) return HOOK_CONTINUE;'),
            ('accumulates', '  c->r = r, c->g = g, c->b = b;', '  c->r = static_cast<short>(c->r + r), c->g = static_cast<short>(c->g + g), c->b = static_cast<short>(c->b + b);'),
            # FIX08: the body's raw value on the sword again (the FIX07 value: written and diffed, but hidden by the TEV clamp of the brighter sword material)
            ('fix07_raw_body_value', '  dn.tint_morf = a->mpModelMorf1, dn.tint_value = dkn::sword_tint_value(a->mMaterialColor), dn.tint_slot = s, ++dn.tint_armed;', '  dn.tint_morf = a->mpModelMorf1, dn.tint_value = dkn::tint_value(a->mMaterialColor), dn.tint_slot = s, ++dn.tint_armed;'),
        ]
        for label, old, new in mutations:
            assert source.count(old) == 1, label
            result = run_variant(source.replace(old, new, 1), directory, label)
            if result.returncode != 1 or 'FAIL line=' not in result.stdout:
                raise RuntimeError('mutation survived: ' + label + result.stdout + result.stderr)
            print(label + ': DETECTED ' + result.stdout.strip())
    print(json.dumps({'status': 'DKN_SWORD_TINT_ADAPTER_PASS', 'mutants_detected': 6, 'mutants_total': 6, 'fix06_route_reproduced': True,
                      'scope': 'production draw PRE / entryDL PRE / entryDL POST / draw POST + writer inside the measured 2.0.3 draw() order (MAJI before each entryDL), '
                               'J3D material doubles sharing one model data; no render / visual certification'}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
