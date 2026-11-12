// LIGHT FADES TWICE 0.13.4 CHILFOS FIX04 P0: CPU benchmark of the native BG line the C3 wind-up casts every Execute (fopAcM_otherBgCheck ->
// cBgS::LineCross -> dBgWKCol::LineCheck), with the eye height of 0.13.3 (-1e9) and of 0.13.4 (Link's feet - 60).
// The stride loop is transcribed from TwilitRealm/dusklight@40457c6 src/d/d_bg_w_kcol.cpp (dBgWKCol::LineCheck): 500-unit strides (EXE constant
// 0x141e4e718 = 500.0f), each stride building its AABB against the KCL area with the out-of-line C_VECSubtract/C_VECAdd/C_VECScale calls the 2.0.3 EXE
// makes (llvm-objdump of 0x14080b150), clamped to the area masks, and walking the octree cells of that AABB when it is inside the area. The KCL here is
// SYNTHETIC (a 16384 x 4096 x 16384 area, 512-unit leaf cells, one flat floor prism per cell): it reproduces the cost SHAPE (strides x per-stride work),
// not the user's room. Real numbers on the user's PC are measured in production by CHILFOS_PERF event=THROW_WINDUP (native_ms_max / native_ms_avg).
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "../src/kk_core.hpp"
#if defined(_MSC_VER)
#define NOINL __declspec(noinline)
#else
#define NOINL __attribute__((noinline))
#endif
struct Vec { float x, y, z; };
NOINL void C_VECSubtract(const Vec* a, const Vec* b, Vec* o) { o->x = a->x - b->x, o->y = a->y - b->y, o->z = a->z - b->z; }
NOINL void C_VECAdd(const Vec* a, const Vec* b, Vec* o) { o->x = a->x + b->x, o->y = a->y + b->y, o->z = a->z + b->z; }
NOINL void C_VECScale(const Vec* a, Vec* o, float s) { o->x = a->x * s, o->y = a->y * s, o->z = a->z * s; }
NOINL float C_VECDot(const Vec* a, const Vec* b) { return a->x * b->x + a->y * b->y + a->z * b->z; }
struct Kcl {
  Vec min{-8192, -2048, -8192};
  std::uint32_t xmask = ~0x3FFFu, ymask = ~0xFFFu, zmask = ~0x3FFFu;  // width 16384 / 4096 / 16384
  int shift = 9;                                                         // 512-unit leaf cells
  Vec nrm{0, 1, 0}, pos{0, 0, 0};                                        // one floor plane, y = 0
};
static volatile std::uint64_t g_sink = 0;
// dBgWKCol::LineCheck: returns hit; counts strides and visited cells
static bool line_check(const Kcl& k, Vec start, Vec end, std::uint64_t& strides, std::uint64_t& cells) {
  Vec d;
  C_VECSubtract(&end, &start, &d);
  const float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
  if (len == 0.0f) return false;
  Vec step;
  C_VECScale(&d, &step, 500.0f / len);
  Vec cur = start, prev;
  float walked = 0.0f;
  bool hit = false;
  do {
    ++strides;
    prev = cur;
    if (walked + 500.0f <= len) C_VECAdd(&cur, &step, &cur);
    else cur = end;
    Vec lo, hi, b;
    C_VECSubtract(&prev, &k.min, &lo);
    hi = lo;
    C_VECSubtract(&cur, &k.min, &b);
    if (lo.x > b.x) lo.x = b.x; else hi.x = b.x;
    if (lo.y > b.y) lo.y = b.y; else hi.y = b.y;
    if (lo.z > b.z) lo.z = b.z; else hi.z = b.z;
    lo.x -= 1, lo.y -= 1, lo.z -= 1, hi.x += 1, hi.y += 1, hi.z += 1;
    auto u = [](float f) { return static_cast<int>(static_cast<std::int64_t>(f)); };  // MSVC x64: cvttss2si 64-bit, then the low 32 bits
    int x0 = u(lo.x) < 0 ? 0 : u(lo.x), x1 = u(hi.x) > int(~k.xmask) ? int(~k.xmask) : u(hi.x);
    if (x0 < x1) {
      int y0 = u(lo.y) < 0 ? 0 : u(lo.y), y1 = u(hi.y) > int(~k.ymask) ? int(~k.ymask) : u(hi.y);
      if (y0 < y1) {
        int z0 = u(lo.z) < 0 ? 0 : u(lo.z), z1 = u(hi.z) > int(~k.zmask) ? int(~k.zmask) : u(hi.z);
        if (z0 < z1) {
          const int cs = 1 << k.shift;
          for (int z = z0; z <= z1; z += cs - (z & (cs - 1)))
            for (int y = y0; y <= y1; y += cs - (y & (cs - 1)))
              for (int x = x0; x <= x1; x += cs - (x & (cs - 1))) {
                ++cells;
                Vec a, c;
                C_VECSubtract(&prev, &k.pos, &a);
                C_VECSubtract(&cur, &k.pos, &c);
                const float t0 = C_VECDot(&k.nrm, &a), t1 = C_VECDot(&k.nrm, &c);
                if ((t0 >= 0 && t1 <= 0) || (t0 <= 0 && t1 >= 0)) hit = true;
              }
        }
      }
    }
    walked += 500.0f;
  } while (walked <= len);
  g_sink = g_sink + strides + cells;
  return hit;
}
struct Res { double us_per_call; std::uint64_t strides, cells; bool hit; };
static Res run(float eye_y, int kcl, int calls) {
  std::vector<Kcl> ks(static_cast<unsigned>(kcl));
  const Vec link{0, 0, 300}, kk{0, 0, 0};
  std::uint64_t strides = 0, cells = 0;
  bool hit = false;
  const auto t0 = std::chrono::steady_clock::now();
  for (int c = 0; c < calls; ++c)
    for (const Kcl& k : ks) hit = line_check(k, Vec{kk.x, eye_y, kk.z}, Vec{link.x, link.y + 100.0f, link.z}, strides, cells) || hit;  // cBgS::LineCross: every KCL
  const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
  return {us / calls, strides / std::uint64_t(calls), cells / std::uint64_t(calls), hit};
}
int main() {
  int fails = 0;
  for (int kcl : {1, 4}) {
    const Res old_eye = run(-1.0e9f, kcl, 15);                                    // 0.13.3: ~15 wind-up Execute calls per throw
    const Res new_eye = run(kk::throw_eye(150.0f, 0.0f), kcl, 15 * 1000);         // 0.13.4
    const Res vanilla = run(150.0f, kcl, 15 * 1000);                              // the native eye (no mod)
    std::printf("KK_LOS_BENCH kcl=%d eye=-1e9 strides_per_call=%llu us_per_call=%.1f windup_ms=%.2f | eye=link-60 strides_per_call=%llu us_per_call=%.3f windup_ms=%.4f hit=%d | "
                "vanilla_eye strides_per_call=%llu us_per_call=%.3f | speedup=%.0fx\n",
                kcl, (unsigned long long)old_eye.strides, old_eye.us_per_call, old_eye.us_per_call * 15 / 1000.0, (unsigned long long)new_eye.strides, new_eye.us_per_call,
                new_eye.us_per_call * 15 / 1000.0, new_eye.hit ? 1 : 0, (unsigned long long)vanilla.strides, vanilla.us_per_call, old_eye.us_per_call / (new_eye.us_per_call > 0 ? new_eye.us_per_call : 1e-3));
    if (old_eye.strides < 1900000ull * std::uint64_t(kcl)) ++fails;                   // the 0.13.3 cost is real
    if (new_eye.strides > 3ull * std::uint64_t(kcl)) ++fails;                         // 0.13.4 is vanilla-sized
    if (new_eye.strides > vanilla.strides + std::uint64_t(kcl)) ++fails;
    if (!(old_eye.us_per_call > 100.0 * new_eye.us_per_call)) ++fails;                // at least two orders of magnitude, whatever the CPU
  }
  std::printf("KK_LOS_BENCH version=%s failures=%d RESULT=%s scope=CPU_STRIDE_LOOP_TRANSCRIBED_SYNTHETIC_KCL gpu=NOT_MEASURED runtime=PERF_RUNTIME_PENDING\n", kk::kVersion, fails,
              fails ? "LAB_FAIL" : "LAB_PASS");
  return fails ? 1 : 0;
}
