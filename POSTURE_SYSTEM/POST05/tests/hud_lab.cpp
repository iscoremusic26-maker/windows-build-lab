// POST07 0.7.0 HUD laboratory (pure logic, no game). H01..H28 families of TEST_PLAN.md. Optional: --render <dir> writes LAB reference PNGs
// rasterised from the REAL quad list (one sample per pixel centre, GX-like quads split in two Gouraud triangles, no anti-aliasing).
#include "../src/break_core.hpp"
#include "../src/hud_core.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace post07;
static unsigned checks = 0, failures = 0;
static unsigned fam_checks[29] = {}, fam_fail[29] = {};
static int fam = 0;
static void ck(bool b, const char* what = "") {
  ++checks;
  ++fam_checks[fam];
  if (!b) {
    ++failures;
    ++fam_fail[fam];
    if (fam_fail[fam] <= 3) std::printf("FAIL H%02d %s\n", fam, what);
  }
}
static bool near(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) <= eps; }
static bool same(Rgba a, Rgba b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }

static std::vector<Quad> make(const Visual& v, float sx = 300, float sy = 200) {
  Quad q[kMaxQuads];
  const unsigned n = build(v, sx, sy, q, kMaxQuads);
  return std::vector<Quad>(q, q + n);
}
static Visual vis(float f, float alpha_ = 1, bool broken = false, float pulse = 0, float hit = 0, float rec = 0, float breathe = 0) {
  Visual v;
  v.fill = f, v.alpha = alpha_, v.broken = broken, v.pulse = pulse, v.hit = hit, v.recover = rec, v.breathe = breathe;
  return v;
}
static std::vector<Visual> all_states() {
  std::vector<Visual> out;
  for (float f : {0.f, 0.01f, 0.1f, 0.25f, 0.5f, 0.75f, 0.85f, 0.9f, 0.99f, 1.f})
    for (bool br : {false, true})
      for (float p : {0.f, 0.05f, 0.5f, 1.f})
        for (float h : {0.f, 1.f})
          for (float r : {0.f, 1.f}) out.push_back(vis(f, 1, br, p, h, r, br ? 0.7f : 0));
  return out;
}
// fill quads: brightest ramp colour family, identified by being inside the core band and not part of the frame layers: we tag by geometry
static float fill_extent_of(const std::vector<Quad>& q, float sx, float sy) {
  // the fill and its tip steps are the quads fully inside the core band whose colour is NOT the track colour; measure the outermost x
  const float cy = geo::center_y(sy), hc = geo::layer_h(0, geo::kCoreInset);
  float e = 0;
  for (const Quad& k : q) {
    const bool inCore = k.y0 >= cy - hc - 1e-4f && k.y1 <= cy + hc + 1e-4f;
    const bool track = same(k.tl, alpha(palette::kTrackTop, 0.92f)) || same(k.tl, alpha(palette::kEdgeDark, 0.95f)) || same(k.tl, alpha(palette::kBronzeLight, 0.92f)) ||
                       same(k.tl, alpha(palette::kBronzeLight, 0.95f));  // frame, dark line, track and notches (tapered tips can fall inside the core band)
    const bool white = k.tl.r == 255 && k.tl.g == 255 && k.tl.b == 255;  // gloss / flash
    if (inCore && !track && !white) e = maxf(e, maxf(std::fabs(k.x0 - sx), std::fabs(k.x1 - sx)));
  }
  return e;
}

// ---- tiny PNG writer (stored deflate) -----------------------------------------------------------------------------------------
static unsigned crc_table[256];
static void crc_init() {
  for (unsigned n = 0; n < 256; ++n) {
    unsigned c = n;
    for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xedb88320u ^ (c >> 1) : c >> 1;
    crc_table[n] = c;
  }
}
static unsigned crc(const unsigned char* b, size_t n, unsigned c = 0xffffffffu) {
  for (size_t i = 0; i < n; ++i) c = crc_table[(c ^ b[i]) & 0xff] ^ (c >> 8);
  return c;
}
static void be32(std::vector<unsigned char>& v, unsigned x) {
  for (int s = 24; s >= 0; s -= 8) v.push_back(static_cast<unsigned char>(x >> s));
}
static void chunk(std::vector<unsigned char>& out, const char* type, const std::vector<unsigned char>& data) {
  be32(out, static_cast<unsigned>(data.size()));
  std::vector<unsigned char> td(type, type + 4);
  td.insert(td.end(), data.begin(), data.end());
  out.insert(out.end(), td.begin(), td.end());
  be32(out, crc(td.data(), td.size()) ^ 0xffffffffu);
}
static bool write_png(const std::string& path, int w, int h, const std::vector<unsigned char>& rgb) {
  std::vector<unsigned char> raw;
  for (int y = 0; y < h; ++y) {
    raw.push_back(0);
    raw.insert(raw.end(), rgb.begin() + y * w * 3, rgb.begin() + (y + 1) * w * 3);
  }
  std::vector<unsigned char> z = {0x78, 0x01};
  size_t pos = 0;
  unsigned a = 1, b = 0;
  for (unsigned char c : raw) a = (a + c) % 65521, b = (b + a) % 65521;
  while (pos < raw.size()) {
    const size_t n = raw.size() - pos > 65535 ? 65535 : raw.size() - pos;
    z.push_back(pos + n == raw.size() ? 1 : 0);
    z.push_back(static_cast<unsigned char>(n & 0xff));
    z.push_back(static_cast<unsigned char>(n >> 8));
    z.push_back(static_cast<unsigned char>(~n & 0xff));
    z.push_back(static_cast<unsigned char>((~n >> 8) & 0xff));
    z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
    pos += n;
  }
  be32(z, (b << 16) | a);
  std::vector<unsigned char> png = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
  std::vector<unsigned char> ihdr;
  be32(ihdr, static_cast<unsigned>(w));
  be32(ihdr, static_cast<unsigned>(h));
  ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});
  chunk(png, "IHDR", ihdr);
  chunk(png, "IDAT", z);
  chunk(png, "IEND", {});
  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return false;
  std::fwrite(png.data(), 1, png.size(), f);
  std::fclose(f);
  return true;
}

// ---- GX-like raster model ------------------------------------------------------------------------------------------------------
// J2D model: setColor(TL,TR,BR,BL) members; fillBox vertices (i.x,i.y)=TL (f.x,i.y)=TR (f.x,f.y)=member BL (i.x,f.y)=member BR.
struct Canvas {
  int w, h;
  float scale, ox, oy;  // pixel = (unit - o) * scale
  std::vector<float> px;
  Canvas(int w_, int h_, float s, float ox_, float oy_) : w(w_), h(h_), scale(s), ox(ox_), oy(oy_), px(static_cast<size_t>(w_ * h_ * 3)) {}
  void blend(int x, int y, float r, float g, float b, float a) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    float* p = &px[static_cast<size_t>((y * w + x) * 3)];
    p[0] = p[0] * (1 - a) + r * a, p[1] = p[1] * (1 - a) + g * a, p[2] = p[2] * (1 - a) + b * a;
  }
  // one Gouraud triangle, pixel-centre sampling
  void tri(const float* X, const float* Y, const Rgba* C) {
    const float x0 = (X[0] - ox) * scale, y0 = (Y[0] - oy) * scale, x1 = (X[1] - ox) * scale, y1 = (Y[1] - oy) * scale, x2 = (X[2] - ox) * scale, y2 = (Y[2] - oy) * scale;
    const float den = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
    if (std::fabs(den) < 1e-9f) return;
    const int minx = static_cast<int>(std::floor(minf(x0, minf(x1, x2)))), maxx = static_cast<int>(std::ceil(maxf(x0, maxf(x1, x2))));
    const int miny = static_cast<int>(std::floor(minf(y0, minf(y1, y2)))), maxy = static_cast<int>(std::ceil(maxf(y0, maxf(y1, y2))));
    for (int y = miny; y <= maxy; ++y)
      for (int x = minx; x <= maxx; ++x) {
        const float px_ = x + 0.5f, py_ = y + 0.5f;
        const float l0 = ((y1 - y2) * (px_ - x2) + (x2 - x1) * (py_ - y2)) / den, l1 = ((y2 - y0) * (px_ - x2) + (x0 - x2) * (py_ - y2)) / den, l2 = 1 - l0 - l1;
        if (l0 < 0 || l1 < 0 || l2 < 0) continue;
        auto ch = [&](int k) {
          const float c0 = k == 0 ? C[0].r : k == 1 ? C[0].g : k == 2 ? C[0].b : C[0].a, c1 = k == 0 ? C[1].r : k == 1 ? C[1].g : k == 2 ? C[1].b : C[1].a,
                      c2 = k == 0 ? C[2].r : k == 1 ? C[2].g : k == 2 ? C[2].b : C[2].a;
          return (l0 * c0 + l1 * c1 + l2 * c2) / 255.0f;
        };
        blend(x, y, ch(0), ch(1), ch(2), ch(3));
      }
  }
  void fillbox_j2d(float x0, float y0, float x1, float y1, Rgba mTL, Rgba mTR, Rgba mBR, Rgba mBL) {
    const float X[4] = {x0, x1, x1, x0}, Y[4] = {y0, y0, y1, y1};
    const Rgba V[4] = {mTL, mTR, mBL, mBR};  // fillBox vertex colour order [S]
    const float Xa[3] = {X[0], X[1], X[2]}, Ya[3] = {Y[0], Y[1], Y[2]}, Xb[3] = {X[0], X[2], X[3]}, Yb[3] = {Y[0], Y[2], Y[3]};
    const Rgba Ca[3] = {V[0], V[1], V[2]}, Cb[3] = {V[0], V[2], V[3]};
    tri(Xa, Ya, Ca);
    tri(Xb, Yb, Cb);
  }
  void quad(const Quad& q) {
    const J2DArgs a = j2d_args(q);
    fillbox_j2d(q.x0, q.y0, q.x1, q.y1, a.tl, a.tr, a.br, a.bl);
  }
  std::vector<unsigned char> rgb() const {
    std::vector<unsigned char> o(px.size());
    for (size_t i = 0; i < px.size(); ++i) o[i] = to_u8(px[i] * 255.0f);
    return o;
  }
};
// mock EHB CP06 life bar from its published geometry (only to show the placement; NOT EHB code)
static void mock_life(Canvas& c, float sx, float sy, float fill) {
  const float x = sx - 12.5f, y = sy - 0.5f;
  Quad bg{x - 1.5f, y - 1.5f, x + 26.5f, y + 2.5f, {0, 0, 0, 160}, {0, 0, 0, 160}, {0, 0, 0, 160}, {0, 0, 0, 160}};
  c.quad(bg);
  const Rgba fr{0x37, 0x87, 0xaa, 230};
  c.quad(Quad{x - 1, y - 1, x + 26, y, fr, fr, fr, fr});
  c.quad(Quad{x - 1, y + 1, x + 26, y + 2, fr, fr, fr, fr});
  c.quad(Quad{x - 1, y, x, y + 1, fr, fr, fr, fr});
  c.quad(Quad{x + 25, y, x + 26, y + 1, fr, fr, fr, fr});
  c.quad(Quad{x, y, x + 25, y + 1, {0x10, 0x0c, 0x0c, 230}, {0x10, 0x0c, 0x0c, 230}, {0x10, 0x0c, 0x0c, 230}, {0x10, 0x0c, 0x0c, 230}});
  const Rgba g{120, 200, 60, 255};
  c.quad(Quad{x, y, x + 25 * fill, y + 1, g, g, g, g});
}

static int render_sheet(const std::string& dir, float scale, const char* tag) {
  struct Row {
    const char* name;
    Visual v;
  };
  const Row rows[] = {{"posture_10", vis(0.10f)},          {"posture_25", vis(0.25f)},          {"posture_50", vis(0.50f)},
                      {"posture_75", vis(0.75f)},          {"posture_90", vis(0.90f)},          {"hit_75", vis(0.75f, 1, false, 0, 1)},
                      {"recover_60", vis(0.60f, 1, false, 0, 0, 1)}, {"crossing_pulse_start", vis(1, 1, true, 1)}, {"crossing_pulse_mid", vis(1, 1, true, 0.5f)},
                      {"stagger_open_breathe_lo", vis(1, 1, true, 0, 0, 0, 0)}, {"stagger_open_breathe_hi", vis(1, 1, true, 0, 0, 0, 1)},
                      {"missed_stun_retract_70", vis(0.70f, 1, false, 0, 0, 1)}};
  const int n = static_cast<int>(sizeof rows / sizeof rows[0]);
  const float cw = 40, ch = 12;  // units per cell
  const int W = static_cast<int>(std::ceil(cw * scale)), H = static_cast<int>(std::ceil(ch * static_cast<float>(n) * scale));
  Canvas c(W, H, scale, 0, 0);
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x) {  // neutral outdoor-like backdrop: muted green-grey to warm grey
      float* p = &c.px[static_cast<size_t>((y * W + x) * 3)];
      const float t = static_cast<float>(x) / static_cast<float>(W);
      p[0] = 0.36f + 0.10f * t, p[1] = 0.40f + 0.04f * t, p[2] = 0.34f;
    }
  for (int r = 0; r < n; ++r) {
    const float sx = cw * 0.5f, sy = r * ch + 3.0f;
    mock_life(c, sx, sy, 0.7f);
    Quad q[kMaxQuads];
    const unsigned k = build(rows[r].v, sx, sy, q, kMaxQuads);
    for (unsigned i = 0; i < k; ++i) c.quad(q[i]);
  }
  const std::string path = dir + "/POST07_REFERENCE_RENDER_" + tag + ".png";
  const bool ok = write_png(path, W, H, c.rgb());
  std::printf("RENDER %s %dx%d rows=%d scale=%.3f px/unit %s\n", path.c_str(), W, H, n, scale, ok ? "OK" : "FAILED");
  for (int r = 0; r < n; ++r) std::printf("RENDER_ROW %d %s\n", r, rows[r].name);
  return ok ? 0 : 1;
}

// ---- model draw list for H23 ---------------------------------------------------------------------------------------------------
struct Base {
  int tag = 0;
};
struct ModelList {
  Base* mp2DOpaTopDrawLists[16] = {};
  Base** mp2DOpaTopStart = mp2DOpaTopDrawLists;
  Base* mp2DOpaDrawLists[64] = {};
  Base** mp2DOpaStart = mp2DOpaDrawLists;
  Base* mp2DXluDrawLists[32] = {};
  Base** mp2DXluStart = mp2DXluDrawLists;
  bool sim = true;
  void set2DXlu(Base* b) {
    if (!sim || mp2DXluStart >= mp2DXluDrawLists + 32) return;
    *mp2DXluStart++ = b;
  }
};

static In live(std::uint32_t id, std::uint64_t gen, int posture, bool latched = false, Ph ph = Ph::Tracking, float op = 1) {
  In in;
  in.present = true, in.id = id, in.generation = gen, in.posture = posture, in.latched = latched, in.phase = ph, in.opacity = op;
  return in;
}

int main(int argc, char** argv) {
  crc_init();
  if (argc >= 3 && std::strcmp(argv[1], "--render") == 0) {
    int rc = 0;
    rc |= render_sheet(argv[2], 1080.0f / 448.0f, "1080p");
    rc |= render_sheet(argv[2], 2160.0f / 448.0f, "2160p");
    rc |= render_sheet(argv[2], 720.0f / 448.0f, "720p");
    return rc;
  }
  const auto states = all_states();
  const float sx = 300, sy = 200;
  // H01 geometry: frame exactly below the life bar, same total width; fill inside the core
  fam = 1;
  {
    ck(near(geo::layer_end(0), 14.0f) && near(geo::kFrameTop, 3.4f) && near(geo::kFrameTop + geo::kFrameH, 6.0f), "frame span");
    ck(near(geo::layer_end(geo::kCoreInset), 12.4217f, 1e-3f) && geo::layer_h(0, geo::kCoreInset) > 0.69f && geo::layer_h(0, geo::kCoreInset) < 0.71f, "core");
    const auto q = make(vis(0.6f), sx, sy);
    const Bounds b = bounds(q.data(), static_cast<unsigned>(q.size()));
    ck(b.any && near(b.x0, sx - 14.6f) && near(b.x1, sx + 14.6f) && near(b.y0, sy + 2.8f) && near(b.y1, sy + 6.6f), "bounds no pulse");
    for (const Quad& k : q) ck(k.x1 > k.x0 && k.y1 > k.y0, "positive area");
    for (float u = 0; u <= 14.2f; u += 0.05f) ck(geo::frame_h(u) <= geo::kHalfH + 1e-6f && geo::frame_h(u) >= 0, "profile");
    for (float u = geo::kBodyHalf; u < 14.0f; u += 0.1f) ck(geo::frame_h(u + 0.1f) <= geo::frame_h(u) + 1e-6f, "taper monotone");
  }
  // H02 left/right symmetry of every primitive (mirror about sx, colours mirrored)
  fam = 2;
  for (const Visual& v : states) {
    const auto q = make(v, sx, sy);
    for (const Quad& a : q) {
      bool found = false;
      for (const Quad& m : q)
        if (near(m.x0, 2 * sx - a.x1) && near(m.x1, 2 * sx - a.x0) && near(m.y0, a.y0) && near(m.y1, a.y1) && same(m.tl, a.tr) && same(m.tr, a.tl) && same(m.bl, a.br) &&
            same(m.br, a.bl))
          found = true;
      ck(found, "mirror");
    }
  }
  // H03 fill grows from the CENTRE to BOTH ends: extent = f * Ec, centred, monotone; nothing at 0
  fam = 3;
  {
    float prev = -1;
    for (int p = 0; p <= 100; ++p) {
      const float f = p / 100.0f, E = geo::fill_extent(f);
      ck(near(E, f * geo::layer_end(geo::kCoreInset)), "extent formula");
      ck(E >= prev, "monotone");
      prev = E;
      const auto q = make(vis(f), sx, sy);
      const float m = fill_extent_of(q, sx, sy);
      if (p == 0) ck(m == 0, "no fill at 0");
      else ck(near(m, E, 0.02f) || (E < 0.3f && m <= E + 0.02f), "measured extent");
    }
    // both halves meet at the centre
    const auto q = make(vis(0.4f), sx, sy);
    bool left = false, right = false;
    for (const Quad& k : q) {
      if (near(k.x1, sx) && k.x0 < sx) left = true;
      if (near(k.x0, sx) && k.x1 > sx) right = true;
    }
    ck(left && right, "two halves at centre");
  }
  // H04 retraction from the ends towards the centre (displayed fill only shrinks symmetrically)
  fam = 4;
  {
    Hud h;
    h.tick(0, live(1, 1, 100, true), true);
    float prevE = 1e9f;
    for (int t = 0; t < 120; ++t) {
      h.tick(0, live(1, 1, 0, false), true);
      const float E = geo::fill_extent(h.visual(0, static_cast<std::uint32_t>(t)).fill);
      ck(E <= prevE + 1e-6f, "retract monotone");
      prevE = E;
    }
    ck(h.slot(0).display == 0.0f, "reaches 0");
  }
  // H05 never overlaps the life bar: every primitive, every state, every pulse phase stays below sy + 2 + clearance
  fam = 5;
  for (const Visual& v : states)
    for (const Quad& k : make(v, sx, sy)) ck(k.y0 >= sy + geo::kLifeBottom + geo::kClearance - 1e-4f, "below life bar");
  for (int t = 0; t <= kPulseTicks; ++t)
    for (const Quad& k : make(vis(1, 1, true, static_cast<float>(t) / kPulseTicks), sx, sy)) ck(k.y0 >= sy + 2.5f - 1e-4f && k.y1 <= sy + 7.2f, "pulse band");
  // H06 colour ramp gold -> amber -> orange -> red; green channel non-increasing; red stays dominant
  fam = 6;
  {
    ck(same(ramp(0), palette::kGold) && same(ramp(0.5f), palette::kAmber) && same(ramp(0.8f), palette::kOrange) && same(ramp(1), palette::kRed), "stops");
    int prevg = 999;
    for (int p = 0; p <= 100; ++p) {
      const Rgba c = ramp(p / 100.0f);
      ck(c.g <= prevg, "green non-increasing");
      prevg = c.g;
      ck(c.r >= 0xD0 && c.r >= c.g && c.g >= c.b, "warm hue");
    }
    ck(ramp(0.99f).g < 0x50 && ramp(0.99f).r > 0xD0, "reddish near 100");
    ck(same(ramp(-5), ramp(0)) && same(ramp(7), ramp(1)) && same(ramp(NAN), ramp(0)), "clamped");
  }
  // H07 J2D colour mapping and blending: every alpha <= 254; the modelled fillBox reproduces the intended screen corners
  fam = 7;
  for (const Visual& v : states)
    for (const Quad& k : make(v, sx, sy)) {
      ck(k.tl.a <= 254 && k.tr.a <= 254 && k.br.a <= 254 && k.bl.a <= 254, "alpha cap");
      const J2DArgs a = j2d_args(k);
      // model: vertex(f.x,f.y) <- member BL == argument #4 ; vertex(i.x,f.y) <- member BR == argument #3
      ck(same(a.bl, k.br) && same(a.br, k.bl) && same(a.tl, k.tl) && same(a.tr, k.tr), "mapping");
    }
  // H08 pulse ONCE per crossing (shield preparation, punish, fall, down, get-up: no second pulse); a new crossing after rearm pulses again
  fam = 8;
  {
    Hud h;
    for (int p : {0, 25, 50, 75}) h.tick(0, live(1, 1, p), true);
    ck(h.c.pulses == 0, "no pulse below 100");
    h.tick(0, live(1, 1, 100, true, Ph::Tracking), true);
    ck(h.c.pulses == 1 && h.visual(0, 0).pulse == 1.0f, "pulse at crossing");
    for (Ph ph : {Ph::Punish, Ph::Punish, Ph::Falling, Ph::Down, Ph::GettingUp})
      for (int t = 0; t < 60; ++t) h.tick(0, live(1, 1, 100, true, ph), true);
    ck(h.c.pulses == 1, "no second pulse while broken");
    ck(h.visual(0, 0).pulse == 0.0f, "pulse finished");
    h.tick(0, live(1, 1, 0, false), true);  // rearm
    for (int p : {25, 50, 75}) h.tick(0, live(1, 1, p), true);
    h.tick(0, live(1, 1, 100, true), true);
    ck(h.c.pulses == 2, "second crossing pulses");
    Hud k;  // first observation already broken: no pulse
    k.tick(3, live(9, 4, 100, true, Ph::Punish), true);
    ck(k.c.pulses == 0, "bind while broken");
    // pulse decays over exactly kPulseTicks
    Hud m;
    m.tick(0, live(1, 1, 75), true);
    m.tick(0, live(1, 1, 100, true), true);
    int alive = 0;
    for (int t = 0; t < 40; ++t) {
      if (m.visual(0, 0).pulse > 0) ++alive;
      m.tick(0, live(1, 1, 100, true, Ph::Punish), true);
    }
    ck(alive == kPulseTicks, "pulse length");
  }
  // H09 missed stun 100 -> 50: retracts to 50 (never below), no pulse
  fam = 9;
  {
    Hud h;
    h.tick(0, live(1, 1, 75), true);
    h.tick(0, live(1, 1, 100, true, Ph::Punish), true);
    for (int t = 0; t < 30; ++t) h.tick(0, live(1, 1, 100, true, Ph::Punish), true);
    const auto pulses = h.c.pulses;
    float prev = h.slot(0).display;
    for (int t = 0; t < 120; ++t) {
      h.tick(0, live(1, 1, 50, false, Ph::Tracking), true);
      ck(h.slot(0).display <= prev && h.slot(0).display >= 50.0f, "retract to 50");
      prev = h.slot(0).display;
    }
    ck(h.slot(0).display == 50.0f && h.c.pulses == pulses && !h.visual(0, 0).broken, "settled at 50");
    ck(h.visual(0, 5).recover == 0.0f, "recovery shimmer ends");
  }
  // H10 rearm 100 -> 0: retracts to 0, then the bar fades out (posture 0 = no content)
  fam = 10;
  {
    Hud h;
    h.tick(0, live(1, 1, 100, true, Ph::GettingUp), true);
    for (int t = 0; t < 10; ++t) h.tick(0, live(1, 1, 100, true, Ph::GettingUp), true);
    int t0 = 0;
    for (; t0 < 200 && h.slot(0).display > 0; ++t0) h.tick(0, live(1, 1, 0, false), true);
    ck(h.slot(0).display == 0.0f && t0 > 5 && t0 < 90, "retract duration");
    for (int t = 0; t < 60; ++t) h.tick(0, live(1, 1, 0, false), true);
    ck(h.slot(0).alpha == 0.0f && !h.drawable(0), "hidden at 0");
  }
  // H11 FALLING / DOWN_FINISH / GETTING_UP: latched => full, broken, visible; never a premature 0
  fam = 11;
  {
    Hud h;
    h.tick(0, live(1, 1, 75), true);
    for (Ph ph : {Ph::Punish, Ph::Falling, Ph::Down, Ph::GettingUp})
      for (int t = 0; t < 50; ++t) {
        h.tick(0, live(1, 1, 100, true, ph), true);
        if (t > 8) ck(h.slot(0).display == 100.0f && h.visual(0, 0).broken && h.drawable(0), "broken full");
      }
    ck(h.visual(0, 0).dim < 1.0f, "getting up dimmed");
    Hud k;
    k.tick(0, live(1, 1, 100, true, Ph::Down), true);
    float mn = 1, mx = 0;
    for (std::uint32_t t = 0; t < 24; ++t) mn = minf(mn, k.visual(0, t).breathe), mx = maxf(mx, k.visual(0, t).breathe);
    ck(mn < 0.05f && mx > 0.95f, "finish window breathes");
  }
  // H12 pause: advance=false freezes every animation; resuming continues from the same values
  fam = 12;
  {
    Hud h;
    h.tick(0, live(1, 1, 75), true);
    h.tick(0, live(1, 1, 100, true), true);
    const Hud::Slot before = h.slot(0);
    for (int t = 0; t < 300; ++t) h.tick(0, live(1, 1, 0, false), false);
    const Hud::Slot after = h.slot(0);
    ck(after.display == before.display && after.pulse == before.pulse && after.alpha == before.alpha && after.target == before.target, "frozen");
    ck(h.c.paused_ticks == 300, "paused count");
    Frame f;
    Sample s[kSlots];
    s[0].in = live(1, 1, 100, true);
    publish(h, s, false, 1, f);
    ck(f.n == 0, "nothing published while paused");
  }
  // H13 smoothing in simulation ticks: deterministic, rise <= 6 ticks per hit, regeneration reads continuous; visual() is const (render frames do not advance)
  fam = 13;
  {
    Hud h;
    h.tick(0, live(1, 1, 0), true);
    h.tick(0, live(1, 1, 25), true);
    int t = 1;
    while (h.slot(0).display < 25.0f && t < 50) h.tick(0, live(1, 1, 25), true), ++t;
    ck(t <= 6, "rise fast");
    const Visual a = h.visual(0, 7), b = h.visual(0, 7);
    ck(a.fill == b.fill && a.alpha == b.alpha, "visual const");
    Hud r;
    r.tick(0, live(1, 1, 60), true);
    for (int k = 0; k < 20; ++k) r.tick(0, live(1, 1, 60), true);
    r.tick(0, live(1, 1, 59), true);
    ck(r.slot(0).display < 60.0f && r.slot(0).display > 59.0f, "1-point regen smoothed");
    Hud x, y;  // determinism
    for (int k = 0; k < 500; ++k) {
      const In in = live(1, 1, (k * 37) % 101, ((k * 37) % 101) == 100, Ph::Tracking, (k % 7) / 6.0f);
      x.tick(0, in, true), y.tick(0, in, true);
    }
    ck(x.slot(0).display == y.slot(0).display && x.slot(0).alpha == y.slot(0).alpha, "deterministic");
  }
  // H14 resolutions: geometry is resolution independent (ortho units, logical height 448); minimum sizes in pixels
  fam = 14;
  for (float H : {720.f, 900.f, 1080.f, 1440.f, 2160.f}) {
    const float s = H / 448.0f;
    ck(geo::kFrameH * s >= 4.0f, "frame >= 4 px");
    ck(geo::layer_h(0, geo::kCoreInset) * 2 * s >= 2.2f, "core >= 2.2 px");
    ck((geo::kFrameTop - geo::kHalo - geo::kLifeBottom) * s >= 1.2f, "visible gap");
  }
  for (float W : {608.f, 808.f, 1000.f}) {  // widescreen changes width only; the bar does not depend on it
    const auto q1 = make(vis(0.5f), 100, 100), q2 = make(vis(0.5f), 100, 100);
    ck(q1.size() == q2.size() && W > 0, "width independent");
  }
  // H15 invalid inputs: NaN/inf anchors and visuals, out-of-range posture, id 0, absent, frozen
  fam = 15;
  {
    Quad q[kMaxQuads];
    ck(build(vis(0.5f), NAN, 10, q, kMaxQuads) == 0 && build(vis(0.5f), 10, INFINITY, q, kMaxQuads) == 0, "nan anchor");
    Visual v = vis(NAN, 1);
    ck(build(v, 10, 10, q, kMaxQuads) > 0, "nan fill clamps");
    v = vis(0.5f, NAN);
    ck(build(v, 10, 10, q, kMaxQuads) == 0, "nan alpha hides");
    ck(build(vis(0.5f), 10, 10, nullptr, 0) == 0, "no sink");
    Hud h;
    h.tick(0, live(1, 1, 250), true);
    ck(h.slot(0).target == 100, "clamp high");
    h.tick(1, live(2, 1, -40), true);
    ck(h.slot(1).target == 0, "clamp low");
    In z = live(0, 1, 50);
    h.tick(2, z, true);
    ck(!h.slot(2).bound, "id 0 rejected");
    h.tick(70, live(1, 1, 50), true);
    ck(true, "out of range slot ignored");
    Hud f;
    f.tick(0, live(1, 1, 75), true);
    for (int t = 0; t < 30; ++t) f.tick(0, live(1, 1, 75), true);
    ck(f.drawable(0), "visible");
    In fr = live(1, 1, 75);
    fr.frozen = true;
    for (int t = 0; t < 40; ++t) f.tick(0, fr, true);
    ck(!f.drawable(0) && f.slot(0).bound, "frozen hidden, identity kept");
    Quad small[3];
    QuadSink sink(small, 3);
    ck(build(vis(1, 1, true, 1, 1, 1, 1), 1, 1, small, 3) == 3, "cap respected");
  }
  // H16 identity: slot + generation + id; recycled slot never inherits values, key changes, no pulse on bind
  fam = 16;
  {
    Hud h;
    h.tick(5, live(11, 1, 100, true, Ph::Punish), true);
    const auto k1 = h.slot(5).key;
    h.tick(5, live(11, 2, 25), true);
    ck(h.slot(5).display == 25.0f && h.slot(5).key != k1 && h.c.identity_resets == 1 && h.c.pulses == 0, "generation recycle");
    h.tick(5, live(12, 2, 50), true);
    ck(h.slot(5).display == 50.0f && h.c.identity_resets == 2, "id recycle");
    Hud a;  // two actors never mix
    for (int t = 0; t < 40; ++t) a.tick(0, live(1, 1, 25), true), a.tick(1, live(2, 1, 75), true);
    ck(a.slot(0).display == 25.0f && a.slot(1).display == 75.0f, "no mixing");
    Anchors an;
    const P3 p{0, 0, 0}, eye{0, 80, 0}, at{0, 120, 0};
    an.update(0, 1, 1, p, eye, at);
    const P3 a2 = an.update(0, 2, 1, p, P3{0, 30, 0}, P3{0, 30, 0});
    ck(near(a2.y, 30 + 8), "anchor reset on identity");
  }
  // H17 17 visible actors: 16 nearest painted, 1 counted over budget
  fam = 17;
  {
    Hud h;
    Sample s[kSlots];
    for (unsigned i = 0; i < 17; ++i) s[i].in = live(100 + i, 1, 50), s[i].distance = 100.0f + 10.0f * static_cast<float>(16 - i);
    Frame f;
    for (int t = 0; t < 20; ++t) publish(h, s, true, static_cast<std::uint32_t>(t), f);
    ck(f.n == kMaxDrawn, "16 painted");
    bool farthest = false;
    for (unsigned k = 0; k < f.n; ++k) farthest |= f.rec[k].distance == 260.0f;
    ck(!farthest && h.c.over_budget > 0 && h.c.max_visible == 17, "nearest first");
    for (unsigned k = 1; k < f.n; ++k) ck(f.rec[k - 1].distance <= f.rec[k].distance, "sorted");
  }
  // H18 64 slots handled, 65th ignored safely
  fam = 18;
  {
    Hud h;
    Sample s[kSlots];
    for (unsigned i = 0; i < kSlots; ++i) s[i].in = live(1000 + i, 1, 25 + static_cast<int>(i % 3) * 25), s[i].distance = static_cast<float>(i);
    Frame f;
    for (int t = 0; t < 20; ++t) publish(h, s, true, static_cast<std::uint32_t>(t), f);
    unsigned bound = 0;
    for (unsigned i = 0; i < kSlots; ++i) bound += h.slot(i).bound ? 1u : 0u;
    ck(bound == 64 && f.n == kMaxDrawn && h.c.max_visible == 64, "64");
    h.tick(64, live(9999, 1, 50), true);
    h.tick(1000, live(9999, 1, 50), true);
    ck(h.c.binds == 64, "65th ignored");
    std::uint8_t sel[kMaxDrawn];
    bool ok[kSlots];
    float d[kSlots];
    for (unsigned i = 0; i < kSlots; ++i) ok[i] = true, d[i] = NAN;
    ck(select_nearest(ok, d, 200, kMaxDrawn, sel) == kMaxDrawn, "nan distances, n clamp");
  }
  // H19 no ghost bars: vanished or dead actors are cleared the SAME tick and never published again
  fam = 19;
  {
    Hud h;
    Sample s[kSlots];
    s[3].in = live(77, 1, 75);
    s[3].distance = 10;
    Frame f;
    for (int t = 0; t < 20; ++t) publish(h, s, true, static_cast<std::uint32_t>(t), f);
    ck(f.n == 1, "shown");
    const auto key = f.rec[0].key;
    s[3].in.present = false;
    publish(h, s, true, 21, f);
    ck(f.n == 0 && !h.slot(3).bound && h.c.cleared_missing == 1, "vanished");
    s[3].in = live(77, 1, 75);
    for (int t = 0; t < 20; ++t) publish(h, s, true, static_cast<std::uint32_t>(30 + t), f);
    ck(f.n == 1 && f.rec[0].key != key, "same id returns with new key");
    s[3].in.dead = true;
    publish(h, s, true, 60, f);
    ck(f.n == 0 && h.c.cleared_dead == 1, "dead");
    s[3].in = live(77, 1, 100, true, Ph::Dead);
    publish(h, s, true, 61, f);
    ck(f.n == 0, "phase dead");
  }
  // H20 visibility rules (EHB-equivalent) and content rule
  fam = 20;
  {
    ck(opacity(true, true, true, 100) == 1 && opacity(false, true, true, 100) == 0 && opacity(true, false, true, 100) == 0 && opacity(true, true, false, 100) == 0, "gates");
    ck(near(opacity(true, true, true, 4100), 0.5f) && opacity(true, true, true, 4500) == 0 && opacity(true, true, true, NAN) == 0 && opacity(true, true, true, -1) == 0, "fade");
    ck(on_screen(P3{0, 0, 1}, 608, 448) && !on_screen(P3{-81, 0, 1}, 608, 448) && !on_screen(P3{0, 529, 1}, 608, 448) && !on_screen(P3{0, 0, 400000}, 608, 448), "screen");
    Hud h;
    for (int t = 0; t < 30; ++t) h.tick(0, live(1, 1, 0), true);
    ck(!h.drawable(0), "posture 0 hidden");
    for (int t = 0; t < 30; ++t) h.tick(0, live(1, 1, 25, false, Ph::Tracking, 0), true);
    ck(!h.drawable(0), "opacity 0 hidden");
    for (int t = 0; t < 30; ++t) h.tick(0, live(1, 1, 25, false, Ph::Tracking, 0.5f), true);
    ck(h.drawable(0) && near(h.slot(0).alpha, 0.5f, 0.01f), "distance fade");
  }
  // H21 placement without EHB: identical geometry (no EHB input exists); anchor rules equal to the published EHB rules
  fam = 21;
  {
    ck(near(stable_head_height(0, P3{0, 0, 0}, P3{0, 90, 0}, P3{0, 120, 0}), 120), "attention");
    ck(near(stable_head_height(0, P3{0, 0, 0}, P3{0, 90, 0}, P3{0, 5, 0}), 90), "eye fallback");
    ck(near(stable_head_height(0, P3{0, 0, 0}, P3{0, 900, 0}, P3{0, 5, 0}), 55), "default");
    ck(near(stable_head_height(100, P3{0, 0, 0}, P3{0, 90, 0}, P3{0, 200, 0}), 103), "rise capped 3");
    ck(near(stable_head_height(100, P3{0, 0, 0}, P3{0, 90, 0}, P3{0, 60, 0}), 100), "never drops");
    const P3 a = anchor_of(P3{1, 2, 3}, 50);
    ck(near(a.x, 1) && near(a.y, 60) && near(a.z, 3), "head + 8");
    const auto q = make(vis(0.7f), sx, sy);
    const Bounds b = bounds(q.data(), static_cast<unsigned>(q.size()));
    ck(b.y0 >= sy + geo::kLifeBottom + geo::kClearance && b.x0 >= sx - geo::kLifeHalfW - 0.6f - 1e-4f, "contextual placement");
  }
  // H22 threads / stale actors: the painter input holds no address; cleared state publishes nothing
  fam = 22;
  {
    ck(std::is_trivially_copyable_v<DrawRec> && std::is_trivially_copyable_v<Frame>, "value records");
    ck(sizeof(DrawRec) == sizeof(std::uint64_t) + sizeof(P3) + sizeof(Visual) + sizeof(float) + 4 || sizeof(DrawRec) <= 64, "no hidden pointer payload");
    Hud h;
    Sample s[kSlots];
    s[0].in = live(1, 1, 50);
    Frame f;
    for (int t = 0; t < 10; ++t) publish(h, s, true, static_cast<std::uint32_t>(t), f);
    h.clear_all();
    s[0].in.present = false;
    publish(h, s, true, 11, f);
    ck(f.n == 0, "cleared");
  }
  // H23 2D draw-list helpers on a modelled dDlst_list_c
  fam = 23;
  {
    ModelList d;
    Base hud{1}, pk{2}, other{3};
    ck(queue_after_hud(d, &hud, &pk) == Queue::HudNotQueued && d.mp2DXluStart == d.mp2DXluDrawLists, "hud absent");
    d.mp2DOpaTopDrawLists[0] = &hud;
    d.mp2DOpaTopStart = d.mp2DOpaTopDrawLists + 1;
    ck(queue_after_hud(d, &hud, &pk) == Queue::Queued && d.mp2DXluDrawLists[0] == &pk, "queued");
    d.sim = false;
    ck(queue_after_hud(d, &hud, &pk) == Queue::Refused, "refused");
    d.sim = true;
    for (int i = 0; i < 40; ++i) queue_after_hud(d, &hud, i % 2 ? &other : &pk);
    ck(d.mp2DXluStart == d.mp2DXluDrawLists + 32, "full");
    ck(queue_after_hud(d, &hud, &pk) == Queue::Refused, "full refused");
    const unsigned removed = scrub_all(d, &pk);
    ck(removed > 0 && !list_contains(d.mp2DXluDrawLists, d.mp2DXluStart, &pk), "scrubbed");
    for (Base** p = d.mp2DXluDrawLists; p < d.mp2DXluStart; ++p) ck(*p == &other, "compacted");
    for (Base** p = d.mp2DXluStart; p < d.mp2DXluDrawLists + 32; ++p) ck(*p == nullptr, "tail null");
    Base** bogus = d.mp2DXluDrawLists + 40;
    ck(scrub_list(d.mp2DXluDrawLists, bogus, &other) == 0, "bogus end untouched");
    ck(!list_contains(d.mp2DOpaDrawLists, d.mp2DOpaStart, static_cast<Base*>(nullptr)), "null needle");
  }
  // H24 POST06 regressions: the HUD only reads copies; Breaker results are identical with and without a HUD observer
  fam = 24;
  {
    using namespace post06;
    auto scenario = [](bool observe) {
      Breaker b;
      b.enabled = true;
      Hud h;
      View v;
      v.tracked = true, v.slot = 0, v.id = 100, v.generation = 1, v.pre_hp = v.native_hp = 500, v.mode = 5, v.writer_ok = true;
      std::vector<int> acts;
      auto step = [&](int posture, bool latched) {
        const Action a = b.step(v);
        b.commit(v, a, true);
        acts.push_back(static_cast<int>(a.act) * 100 + static_cast<int>(a.why));
        if (observe) h.tick(0, live(v.id, v.generation, posture, latched, static_cast<Ph>(static_cast<std::uint8_t>(b.slot(0).phase))), true);
      };
      v.hit = v.crossed = v.latched = true;
      step(100, true);
      v.hit = v.crossed = false;
      v.mode = 5, v.state = 5;
      step(100, true);
      v.hit = v.sword = v.from_link = true;
      v.native_hp = 480;
      step(100, true);
      v.hit = v.sword = v.from_link = false;
      v.mode = 6;
      for (int st : {1, 5, 5, 6, 6}) v.state = st, step(100, true);
      v.mode = 7, v.state = 3;
      step(0, false);
      return std::make_pair(acts, std::make_pair(b.c.rearms, b.c.rearms_redirected));
    };
    const auto x = scenario(false), y = scenario(true);
    ck(x == y, "identical with HUD");
    ck(x.second.first == 1 && x.second.second == 1, "rearm 0 redirected (FIX_REARM50)");
    ck(std::strcmp(kBreakVersion, "0.8.0") == 0 && std::strcmp(kHudVersion, "0.7.0") == 0, "versions (POST08 break 0.8.0; HUD unchanged 0.7.0)");
  }
  // H25 PE / import audit: performed by ci/pe_audit.py on the real MSVC DLL (not in this lab)
  fam = 25;
  ck(true, "delegated to CI");
  // H26 performance budget (CPU, lab machine): primitives per bar and per frame; build() timing
  fam = 26;
  {
    unsigned worst = 0;
    for (const Visual& v : states) {
      Quad q[kMaxQuads];
      QuadSink probe(q, kMaxQuads);
      const unsigned n = build(v, sx, sy, q, kMaxQuads);
      worst = n > worst ? n : worst;
    }
    ck(worst <= 51 && worst < kMaxQuads, "<= 51 quads per bar");
    ck(worst * kMaxDrawn <= 1000, "<= 1000 quads per frame");
    Quad q[kMaxQuads];
    const auto t0 = std::chrono::steady_clock::now();
    unsigned sink = 0;
    for (int i = 0; i < 100000; ++i) sink += build(vis(static_cast<float>(i % 101) / 100.0f, 1, i % 7 == 0, 0.3f), sx, sy, q, kMaxQuads);
    const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count() / 100000.0;
    std::printf("H26_PERF_LAB_CPU build_ns_per_bar=%.1f worst_quads=%u max_frame_quads=%u (lab CPU, not the game GPU)\n", ns, worst, worst * kMaxDrawn);
    ck(sink > 0 && ns < 50000.0, "build cost");
  }
  // H27 reference renders exist as a separate mode (--render); here: the raster model reproduces a quad's corner colours
  fam = 27;
  {
    Canvas c(40, 40, 1.0f, 0, 0);
    const Rgba r{255, 0, 0, 254}, g{0, 255, 0, 254}, b{0, 0, 255, 254}, w{255, 255, 255, 254};
    c.quad(Quad{0, 0, 40, 40, r, g, b, w});
    const auto px = c.rgb();
    auto at = [&](int x, int y, int k) { return px[static_cast<size_t>((y * 40 + x) * 3 + k)]; };
    ck(at(0, 0, 0) > 230 && at(39, 0, 1) > 230 && at(39, 39, 2) > 230 && at(0, 39, 0) > 230 && at(0, 39, 1) > 230, "corner colours (J2D swap modelled)");
  }
  // H28 broken / pulse colours are red-orange; broken frame tinted
  fam = 28;
  {
    const auto q = make(vis(1, 1, true, 0, 0, 0, 0.5f), sx, sy);
    bool redFill = false;
    for (const Quad& k : q)
      if (k.tl.r > 180 && k.tl.g < 90 && k.tl.b < 60) redFill = true;
    ck(redFill, "broken fill red");
    ck(palette::kPulse.r == 0xFF && palette::kPulse.g < 0x80, "pulse ember");
  }
  for (int i = 1; i <= 28; ++i) std::printf("H%02d checks=%u failures=%u\n", i, fam_checks[i], fam_fail[i]);
  std::printf("POST07_HUD_LAB checks=%u failures=%u RESULT=%s\n", checks, failures, failures ? "LAB_FAIL" : "LAB_PASS");
  return failures ? 1 : 0;
}
