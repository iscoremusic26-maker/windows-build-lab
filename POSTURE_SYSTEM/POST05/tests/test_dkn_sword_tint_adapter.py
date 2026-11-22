"""DKN01 FIX06: execute the PRODUCTION draw PRE / POST of the sword darkening (src/dkn_adapter.inc: dkn_native_sword_color, on_dkn_draw_pre,
on_dkn_draw_post) and the pure decision of src/dkn_core.hpp against J3D material doubles that share ONE model data between Darknuts.

What a draw sees is taken where the native mpModelMorf1->entryDL() diffs the material into the instance: between the PRE and the POST of that draw.
Checks: only the definitive death darkens, with the body's own value (no accumulation, frame after frame); every other instance drawn before or after
(alive, recoverable fall, ineligible, another dead one with its own value) sees the original colour; the POST restores the exact saved values (also a
non-zero original); a POST without its PRE changes nothing. This is the adapter's execution with doubles, never a render or a visual certification.
Four mutants must be detected (the restore removed, the stage ignored, an accumulating write, a stale save); a compile error is never a detection."""
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
struct J3DMaterial { J3DGXColorS10 c; bool has = true; J3DGXColorS10* getTevColor(unsigned) { return has ? &c : nullptr; } };
struct J3DModelData { J3DMaterial m[3]; unsigned n = 1; u16 getMaterialNum() const { return static_cast<u16>(n); } J3DMaterial* getMaterialNodePointer(u16 i) { return &m[i]; } };
struct J3DModel { J3DModelData* d = nullptr; J3DModelData* getModelData() { return d; } };
struct mDoExt_McaMorfSO { J3DModel* model = nullptr; J3DModel* getModel() { return model; } };
struct daB_TN_c { int mType = 1; float mMaterialColor = 0; bool mDisappear = false, mInactive = false; std::uint16_t mStates[16] = {}; mDoExt_McaMorfSO* mpModelMorf1 = nullptr; std::uint32_t id = 0; };
std::uint32_t fopAcM_GetID(const daB_TN_c* a) { return a->id; }
struct DknSlot { bool on = false; std::uint32_t id = 0; dkn::Ph ph = dkn::Ph::Light; std::uint64_t st_sword_tint_draws = 0; };
struct DknState {
  bool enabled = true;
  DknSlot s[8];
  J3DModelData* tint_data = nullptr;
  short tint_saved[8][3] = {};
  unsigned tint_n = 0;
  std::uint64_t tint_draws = 0, tint_restores = 0;
} dn;
DknSlot* dkn_find(std::uint32_t id) { for (DknSlot& s : dn.s) if (s.on && s.id == id) return &s; return nullptr; }
void dkn_log(const char*, const DknSlot&, const daB_TN_c*, const char*) {}
'''

MAIN = r'''
unsigned checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { std::printf("FAIL line=%d: %s\n", __LINE__, #c); return 1; } } while (0)
// one draw() of actor a: PRE, what entryDL captures (material 0 of the shared sword data), POST
J3DGXColorS10 draw(daB_TN_c& a, J3DModelData& d) {
  void* args[1] = {&a};
  on_dkn_draw_pre(nullptr, args, nullptr, nullptr);
  const J3DGXColorS10 seen = d.m[0].c;
  on_dkn_draw_post(nullptr, args, nullptr, nullptr);
  return seen;
}
bool same(const J3DGXColorS10& x, short r, short g, short b) { return x.r == r && x.g == g && x.b == b; }
int main() {
  J3DModelData sword;  // ONE model data: every mType-1 Darknut's sword (B_tnp2 tn2_sword_b)
  J3DModel ma{&sword}, mb{&sword}, mc{&sword}, md{&sword};
  mDoExt_McaMorfSO fa{&ma}, fb{&mb}, fc{&mc}, fdd{&md};
  daB_TN_c A, B, C, D;
  A.id = 11, B.id = 12, C.id = 13, D.id = 99;
  A.mpModelMorf1 = &fa, B.mpModelMorf1 = &fb, C.mpModelMorf1 = &fc, D.mpModelMorf1 = &fdd;
  A.mStates[15] = B.mStates[15] = C.mStates[15] = D.mStates[15] = 2;  // dropped / in hand: drawn
  dn.s[0] = DknSlot{true, 11, dkn::Ph::Dead, 0};     // A: definitive death
  dn.s[1] = DknSlot{true, 12, dkn::Ph::Revived, 0};  // B: alive (F3)
  dn.s[2] = DknSlot{true, 13, dkn::Ph::Dkr, 0};      // C: recoverable fall / first defeat (DKR01 owns it)
  // D: never admitted (ineligible Darknut)
  // 1) alternating draws while A darkens: A sees the body's value each frame (never accumulated), the others the original colour
  float m = 0;
  for (int f = 0; f < 200; ++f) {
    m = m - 0.3f < -50.0f ? -50.0f : m - 0.3f;
    A.mMaterialColor = m, C.mMaterialColor = m, D.mMaterialColor = m;  // C / D: a non-zero colour must still not tint them
    const short v = static_cast<short>(m);
    CHECK(same(draw(B, sword), 0, 0, 0));
    CHECK(same(draw(A, sword), v, v, v));
    CHECK(same(draw(C, sword), 0, 0, 0));
    CHECK(same(draw(D, sword), 0, 0, 0));
    CHECK(same(sword.m[0].c, 0, 0, 0) && dn.tint_data == nullptr);
  }
  CHECK(dn.s[0].st_sword_tint_draws == 200 && dn.s[1].st_sword_tint_draws == 0 && dn.s[2].st_sword_tint_draws == 0);
  // 2) a definitive death whose body is not darkening yet (mMaterialColor 0) draws the sword unchanged
  A.mMaterialColor = 0;
  CHECK(same(draw(A, sword), 0, 0, 0));
  // 3) a non-zero original colour and several materials come back exactly; a material without a TEV block is skipped
  sword.n = 3, sword.m[0].c = {5, 6, 7, 255}, sword.m[1].c = {-1, 2, -3, 255}, sword.m[2].has = false;
  A.mMaterialColor = -33.9f;
  {
    void* args[1] = {&A};
    on_dkn_draw_pre(nullptr, args, nullptr, nullptr);
    CHECK(same(sword.m[0].c, -33, -33, -33) && same(sword.m[1].c, -33, -33, -33));
    on_dkn_draw_post(nullptr, args, nullptr, nullptr);
  }
  CHECK(same(sword.m[0].c, 5, 6, 7) && same(sword.m[1].c, -1, 2, -3));
  // 4) two dead Darknuts with different values: each its own, the shared data restored after each
  dn.s[1].ph = dkn::Ph::Dead, B.mMaterialColor = -10.0f;
  CHECK(same(draw(A, sword), -33, -33, -33) && same(draw(B, sword), -10, -10, -10) && same(sword.m[0].c, 5, 6, 7));
  // 5) no sword drawn (state 7), disappearing / inactive actor, module disabled: untouched
  A.mStates[15] = 7;
  CHECK(same(draw(A, sword), 5, 6, 7));
  A.mStates[15] = 2, A.mDisappear = true;
  CHECK(same(draw(A, sword), 5, 6, 7));
  A.mDisappear = false, dn.enabled = false;
  CHECK(same(draw(A, sword), 5, 6, 7));
  dn.enabled = true;
  // 6) a POST without its PRE (another actor's draw) changes nothing
  {
    void* args[1] = {&B};
    sword.m[0].c = {1, 1, 1, 255};
    on_dkn_draw_post(nullptr, args, nullptr, nullptr);
    CHECK(same(sword.m[0].c, 1, 1, 1));
  }
  // 7) a tinting PRE whose POST never came (abnormal): the NEXT draw's PRE forgets that save, so no later POST ever writes into model data that may be
  // gone (scene change) or belong to another model (here: B draws with another sword model data; A's stays exactly as it was left)
  {
    J3DModelData other;
    J3DModel mo{&other};
    mDoExt_McaMorfSO fo{&mo};
    sword.m[0].c = {5, 6, 7, 255}, sword.n = 1;
    A.mMaterialColor = -40.0f, dn.s[1].ph = dkn::Ph::Revived, B.mpModelMorf1 = &fo;
    void* aa[1] = {&A};
    on_dkn_draw_pre(nullptr, aa, nullptr, nullptr);  // A's POST is lost
    CHECK(same(sword.m[0].c, -40, -40, -40));
    void* bb[1] = {&B};
    on_dkn_draw_pre(nullptr, bb, nullptr, nullptr);
    on_dkn_draw_post(nullptr, bb, nullptr, nullptr);
    CHECK(same(sword.m[0].c, -40, -40, -40) && same(other.m[0].c, 0, 0, 0) && dn.tint_data == nullptr);
    sword.m[0].c = {5, 6, 7, 255}, B.mpModelMorf1 = &fb;
  }
  std::printf("DKN_SWORD_TINT_ADAPTER_PASS checks=%u draws=%llu restores=%llu runtime=HUMAN_RUNTIME_PENDING\n", checks,
              static_cast<unsigned long long>(dn.tint_draws), static_cast<unsigned long long>(dn.tint_restores));
  return 0;
}
'''


def run_variant(source, directory, label):
    parts = [body(source, 'DKN_NOINLINE void dkn_native_sword_color('), body(source, 'HookAction on_dkn_draw_pre('), body(source, 'void on_dkn_draw_post(')]
    cpp = directory / (label + '.cpp')
    cpp.write_text(PREFIX + '\n' + '\n'.join(parts) + '\n' + MAIN)
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
    with tempfile.TemporaryDirectory() as td:
        directory = Path(td)
        positive = run_variant(source, directory, 'positive')
        print(positive.stdout, end='')
        if positive.returncode:
            return 1
        mutations = [
            ('restore_removed', '    if (mat && mat->getTevColor(0)) dkn_native_sword_color(mat, dn.tint_saved[i][0], dn.tint_saved[i][1], dn.tint_saved[i][2]);', '    (void)mat;'),
            ('any_stage_darkens', '  if (!s || !dkn::sword_tint(s->ph == dkn::Ph::Dead, a->mMaterialColor)) return HOOK_CONTINUE;',
             '  if (!s || !dkn::sword_tint(true, a->mMaterialColor)) return HOOK_CONTINUE;'),
            ('accumulates', '  c->r = r, c->g = g, c->b = b;', '  c->r = static_cast<short>(c->r + r), c->g = static_cast<short>(c->g + g), c->b = static_cast<short>(c->b + b);'),
            ('stale_save_restored', '  dn.tint_data = nullptr, dn.tint_n = 0;\n  if (!dn.enabled || !args) return HOOK_CONTINUE;', '  if (!dn.enabled || !args) return HOOK_CONTINUE;'),
        ]
        for label, old, new in mutations:
            assert source.count(old) == 1, label
            result = run_variant(source.replace(old, new, 1), directory, label)
            if result.returncode != 1 or 'FAIL line=' not in result.stdout:
                raise RuntimeError('mutation survived: ' + label + result.stdout + result.stderr)
            print(label + ': DETECTED ' + result.stdout.strip())
    print(json.dumps({'status': 'DKN_SWORD_TINT_ADAPTER_PASS', 'mutants_detected': 4, 'mutants_total': 4,
                      'scope': 'production draw PRE/POST + writer against J3D material doubles sharing one model data; no render / visual certification'}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
