"""FIX05: execute the production draw adapter/writer against native collider doubles.

This checks AT admission, not a game collision or a visual test. The native
setSwordAtBit and cc_set contracts are independently pinned by dkn_native_audit.
The old whole-weapon suppression must fail the near-contact assertion.
"""
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
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#define DKN_NOINLINE
struct cXyz { float x=0,y=0,z=0; };
struct Collider {
  bool at=true, hit=true;
  bool ChkAtSet() const { return at; }
  void OffAtSetBit() { at=false; }
  void OnAtSetBit() { at=true; }
  void ClrAtHit() { hit=false; }
};
struct daB_TN_c {
  int mActionMode1=7, mActionMode2=5, mStates[16]={};
  struct { cXyz pos; } current;
  cXyz mCpsPos{1200,0,0};
  Collider mSwordSphs[4], mCps, mSphC;
  unsigned native_calls=0;
  void setSwordAtBit(int want) {
    ++native_calls;
    for (auto& s:mSwordSphs) { s.at=want!=0; if (!want) s.ClrAtHit(); }
    mCps.at=want!=0; if (!want) mCps.ClrAtHit();
  }
};
struct daAlink_c { struct { cXyz pos{600,0,0}; } current; } link;
struct DknSlot {
  bool tf_reanchored=false, tf_restore=false;
  std::uint64_t st_tf_restore=0, st_tf_reanchor=0;
};
namespace dkn { namespace native { constexpr int kActChangeDemo=7; } }
namespace kk { float fsqrt(float f) { return std::sqrt(f); } }
daAlink_c* dknb_link() { return &link; }
float dkn_dist(const daB_TN_c* a, const daAlink_c* k) {
  return std::hypot(k->current.pos.x-a->current.pos.x,k->current.pos.z-a->current.pos.z);
}
float dkn_frame(daB_TN_c*) { return 10; }
void dkn_log(const char*,const DknSlot&,daB_TN_c*,const char*) {}
'''

MAIN = r'''
unsigned checks=0;
#define CHECK(c) do { ++checks; if (!(c)) { std::printf("FAIL line=%d: %s\n",__LINE__,#c); return 1; } } while(0)
// Explicit geometry: the four physical sword spheres at x=10,50,90,130,
// radius 45; stale capsule [1200,130]. The draw moves the anchor even if AT off.
bool sphere_hit(const daB_TN_c& a,float x,float z) {
  for (unsigned i=0;i<4;++i)
    if (a.mSwordSphs[i].at && std::hypot(x-(10.f+40.f*i),z)<=45.f) return true;
  return false;
}
bool capsule_hit(const daB_TN_c& a,float x,float z) {
  float lo=std::fmin(a.mCpsPos.x,130.f),hi=std::fmax(a.mCpsPos.x,130.f);
  float q=std::fmax(lo,std::fmin(x,hi));
  return a.mCps.at && std::hypot(x-q,z)<=45.f;
}
int main() {
  daB_TN_c a; DknSlot s; a.mStates[15]=1; a.mStates[13]=7;
  CHECK(sphere_hit(a,90,0) && capsule_hit(a,600,0));
  dkn_transform_post(s,&a);
  CHECK(s.tf_reanchored && s.tf_restore && s.st_tf_reanchor==1);
  CHECK(!a.mCps.at && !capsule_hit(a,600,0));
  // A real nearby sphere contact must remain admitted on the re-anchor tick,
  // including its pending receipt. The old setSwordAtBit(0) fails here.
  CHECK(sphere_hit(a,90,0));
  CHECK(sphere_hit(a,90,45) && !sphere_hit(a,90,45.01f));
  for (const auto& c:a.mSwordSphs) CHECK(c.at && c.hit);
  CHECK(a.native_calls==0 && !a.mCps.hit);
  a.mCpsPos={130,0,0}; // cc_set, independently pinned native unconditional write
  dkn_transform_post(s,&a);
  CHECK(a.mCps.at && !s.tf_restore && s.st_tf_restore==1 && a.native_calls==1);
  CHECK(sphere_hit(a,90,0) && !capsule_hit(a,600,0));
  CHECK(capsule_hit(a,130,45) && !capsule_hit(a,130,45.01f));
  dkn_transform_post(s,&a);
  CHECK(s.st_tf_reanchor==1 && s.st_tf_restore==1 && a.native_calls==1);
  // Flying mace and earlier extraction modes stay native. Separate actor slots.
  for (int mode=0;mode<6;++mode) {
    daB_TN_c b; DknSlot t; b.mActionMode2=mode; b.mStates[13]=3;
    dkn_transform_post(t,&b);
    CHECK(b.mCps.at && b.mSwordSphs[0].at && !t.tf_reanchored && !t.tf_restore);
    CHECK(s.st_tf_reanchor==1 && s.st_tf_restore==1);
  }
  for (int action:{0,6,8,9}) {
    daB_TN_c b; DknSlot t; b.mActionMode1=action; b.mStates[15]=1;
    dkn_transform_post(t,&b);
    CHECK(b.mCps.at && b.mSwordSphs[0].at && !t.tf_reanchored);
  }
  { // A vanished sword never gets rearmed by the pending restoration.
    daB_TN_c b; DknSlot t; b.mStates[15]=1;
    dkn_transform_post(t,&b); b.mStates[15]=0;
    dkn_transform_post(t,&b);
    CHECK(!b.mCps.at && !t.tf_restore && t.st_tf_restore==0 && b.native_calls==0);
  }
  { // Native restoration by another legitimate route is not repeated.
    daB_TN_c b; DknSlot t; b.mStates[15]=1;
    dkn_transform_post(t,&b); b.mCps.at=true;
    dkn_transform_post(t,&b);
    CHECK(!t.tf_restore && t.st_tf_restore==0 && b.native_calls==0);
  }
  { // Whole-weapon AT authority used by existing moves is unchanged.
    daB_TN_c b; dkn_native_at(&b,true,0);
    CHECK(!b.mSwordSphs[0].at && !b.mCps.at);
    dkn_native_at(&b,true,1);
    CHECK(b.mSwordSphs[0].at && b.mCps.at);
    dkn_native_at(&b,false,0); CHECK(!b.mSphC.at && b.mSwordSphs[0].at);
    dkn_native_at(&b,false,1); CHECK(b.mSphC.at);
  }
  DknSlot cleared{}; CHECK(!cleared.tf_restore && !cleared.tf_reanchored);
  std::printf("DKN_TRANSFORM_ADAPTER_PASS checks=%u runtime=HUMAN_RUNTIME_PENDING\n",checks);
}
'''


def run_variant(source, directory, label):
    writer = body(source, 'DKN_NOINLINE void dkn_native_at(')
    adapter = body(source, 'void dkn_transform_post(')
    cpp = directory / (label + '.cpp')
    cpp.write_text(PREFIX + '\n' + writer + '\n' + adapter + '\n' + MAIN)
    exe = directory / (label + ('.exe' if os.name == 'nt' else ''))
    if os.name == 'nt':
        cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', str(cpp), '/Fe' + str(exe), '/Fo' + str(directory / (label + '.obj'))]
    else:
        cmd = [shutil.which('g++') or 'clang++', '-std=c++20', '-O2', str(cpp), '-o', str(exe)]
    built = subprocess.run(cmd, capture_output=True, text=True, cwd=directory, timeout=60)
    if built.returncode:
        raise RuntimeError('compile failure never counts as detection: ' + built.stdout + built.stderr)
    return subprocess.run([str(exe)], capture_output=True, text=True, timeout=10)


def main():
    source = (ROOT / 'src/dkn_adapter.inc').read_text()
    with tempfile.TemporaryDirectory() as td:
        directory = Path(td)
        positive = run_variant(source, directory, 'positive')
        print(positive.stdout, end='')
        if positive.returncode:
            return 1
        mutations = [
            ('whole_weapon_off', '  dkn_native_at(a, true, 0, true);', '  dkn_native_at(a, true, 0);'),
            ('capsule_left_on', '  dkn_native_at(a, true, 0, true);', '  (void)0;'),
            ('restore_reads_sphere', 'if (in_hand && !a->mCps.ChkAtSet())', 'if (in_hand && !at_on)'),
            ('restore_removed', '      dkn_native_at(a, true, 1);', '      (void)0;'),
        ]
        for label, old, new in mutations:
            assert source.count(old) == 1, label
            result = run_variant(source.replace(old, new, 1), directory, label)
            if result.returncode != 1 or 'FAIL line=' not in result.stdout:
                raise RuntimeError('mutation survived: ' + label + result.stdout + result.stderr)
            print(label + ': DETECTED ' + result.stdout.strip())
    print(json.dumps({'status': 'DKN_TRANSFORM_ADAPTER_PASS', 'mutants_detected': 4, 'mutants_total': 4,
                      'scope': 'production writer/adapter; collider doubles and explicit geometry; no runtime/visual certification'}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
