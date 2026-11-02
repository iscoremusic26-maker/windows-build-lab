#pragma once
// POST07 0.7.0: posture HUD, PURE core (standard library only; no game/SDK header, no address, no allocation).
// DISPLAY / READ-ONLY: the glue feeds it copies of the posture sidecar (POST04), the POST06 phase and an EHB-style visibility; nothing computed here
// is ever written back to the posture, POST05, POST06, the actor or the game. All geometry is in the painter's ORTHO units (logical height 448),
// relative to the projected anchor (sx, sy) that Enemy Health Bars CP06 uses for its life bar: the life bar occupies x in [sx-14, sx+14],
// y in [sy-2, sy+2]; this bar lives strictly BELOW it (y >= sy+2.5 for every primitive, every state).
#include <cstddef>
#include <cstdint>

namespace post07 {
inline constexpr const char* kHudVersion = "0.7.0";
inline constexpr unsigned kSlots = 64;     // == post03::kCapacity (static_assert in mod.cpp): one HUD slot per ledger slot, never more
inline constexpr unsigned kMaxQuads = 64;  // primitives per bar (worst case measured in the lab: 51)
inline constexpr unsigned kMaxDrawn = 16;  // bars painted per frame (nearest first); the rest stay animated but unpainted (perf budget, Intel UHD)

struct Rgba {
  std::uint8_t r = 0, g = 0, b = 0, a = 0;
};
struct Quad {  // axis-aligned box in ortho units, colours per SCREEN corner (top-left, top-right, bottom-right, bottom-left)
  float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  Rgba tl, tr, br, bl;
};

inline constexpr bool isfin(float x) noexcept { return x == x && x <= 3.0e38f && x >= -3.0e38f; }
inline constexpr float clamp01(float x) noexcept { return !isfin(x) ? 0.0f : (x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x)); }
inline constexpr float maxf(float a, float b) noexcept { return a > b ? a : b; }
inline constexpr float minf(float a, float b) noexcept { return a < b ? a : b; }
inline constexpr std::uint8_t to_u8(float v) noexcept {
  if (!(v == v) || v <= 0.0f) return 0;  // NaN and negatives -> 0
  const float r = v + 0.5f;
  return r >= 255.0f ? std::uint8_t{255} : static_cast<std::uint8_t>(r);
}
inline constexpr Rgba rgb(std::uint32_t hex) noexcept {
  return Rgba{static_cast<std::uint8_t>(hex >> 16), static_cast<std::uint8_t>(hex >> 8), static_cast<std::uint8_t>(hex), 255};
}
inline constexpr Rgba mix(Rgba a, Rgba b, float t) noexcept {
  const float k = clamp01(t);
  return Rgba{to_u8(a.r + (b.r - a.r) * k), to_u8(a.g + (b.g - a.g) * k), to_u8(a.b + (b.b - a.b) * k), to_u8(a.a + (b.a - a.a) * k)};
}
// Alpha is capped at 254: J2DGrafContext::setColor turns blending OFF when the top-left and bottom-right alphas are both 255 [S J2DGrafContext.cpp:113-123],
// which would make a partly transparent corner opaque. 254 keeps every primitive blended, uniformly.
inline constexpr Rgba alpha(Rgba c, float a01) noexcept { return Rgba{c.r, c.g, c.b, to_u8(254.0f * clamp01(a01))}; }

namespace palette {  // original palette (no third-party asset): warm gold -> amber -> orange -> red, dark bronze frame
inline constexpr Rgba kGold = rgb(0xF2C85A), kAmber = rgb(0xF09E2A), kOrange = rgb(0xF0661E), kRed = rgb(0xD82E14);
inline constexpr Rgba kBrokenA = rgb(0xC0200E), kBrokenB = rgb(0xFF5A26), kBrokenFrame = rgb(0x8A2E16);
inline constexpr Rgba kBronzeLight = rgb(0x8C6A36), kBronzeDark = rgb(0x4A361C), kEdgeDark = rgb(0x080604);
inline constexpr Rgba kTrackTop = rgb(0x241C14), kTrackBottom = rgb(0x0C0906), kBlack = rgb(0x000000), kWhite = rgb(0xFFFFFF);
inline constexpr Rgba kPulse = rgb(0xFF6A24), kRecover = rgb(0xFFE8B0);
}  // namespace palette

// Posture colour: gold (0) -> amber (0.5) -> orange (0.8) -> red (1.0). Monotone in the red-ish direction (green never rises past 0.5).
inline constexpr Rgba ramp(float f) noexcept {
  const float x = clamp01(f);
  if (x <= 0.5f) return mix(palette::kGold, palette::kAmber, x / 0.5f);
  if (x <= 0.8f) return mix(palette::kAmber, palette::kOrange, (x - 0.5f) / 0.3f);
  return mix(palette::kOrange, palette::kRed, (x - 0.8f) / 0.2f);
}

namespace geo {
inline constexpr float kLifeHalfW = 14.0f, kLifeTop = -2.0f, kLifeBottom = 2.0f;  // EHB CP06: solid(x-1.5, y-1.5, 28, 4), x = sx-12.5, y = sy-0.5
inline constexpr float kClearance = 0.5f;                                       // minimum free band between the life bar and any posture primitive
inline constexpr float kFrameTop = 3.4f, kFrameH = 2.6f;                        // frame: sy+3.4 .. sy+6.0, centre line sy+4.7
inline constexpr float kHalfH = kFrameH * 0.5f;
inline constexpr float kBodyHalf = 11.5f, kTipLen = 2.5f, kTipEnd = 0.15f;      // body + tapered tips: total half width 14 == life bar
inline constexpr unsigned kTipSteps = 4;
inline constexpr float kBronze = 0.4f, kDark = 0.2f, kCoreInset = kBronze + kDark;  // subtle double border: bronze 0.4 + dark 0.2, core 1.4 high
inline constexpr float kHalo = 0.6f, kBloom = 0.45f, kNotch = 0.5f;
inline constexpr float kPulseGrowX = 3.0f, kPulseGrowY = 0.6f, kPulsePad = 0.1f;
inline constexpr float center_y(float sy) noexcept { return sy + kFrameTop + kHalfH; }
// Half height of the outer frame at horizontal distance u from the centre (body flat, tips taper linearly to kTipEnd, then nothing).
inline constexpr float frame_h(float u) noexcept {
  if (u < 0.0f) u = -u;
  if (u <= kBodyHalf) return kHalfH;
  const float t = (u - kBodyHalf) / kTipLen;
  return t > 1.0f ? 0.0f : kHalfH + (kTipEnd - kHalfH) * t;
}
// A layer inset by d (vertically and horizontally): half height and extent.
inline constexpr float layer_h(float u, float d) noexcept { return maxf(0.0f, frame_h(u + d) - d); }
inline constexpr float layer_end(float d) noexcept {
  const float t = (kHalfH - d) / (kHalfH - kTipEnd);
  return (t >= 1.0f ? kBodyHalf + kTipLen : kBodyHalf + kTipLen * t) - d;
}
inline constexpr float fill_extent(float f) noexcept { return clamp01(f) * layer_end(kCoreInset); }  // centre -> both ends, symmetric
}  // namespace geo

// What the painter needs for one bar (all already smoothed; no identity, no address).
struct Visual {
  float fill = 0;     // displayed posture / 100
  float alpha = 0;    // global opacity
  float hit = 0;      // 1 -> 0 hit flash
  float pulse = 0;    // 1 -> 0 break pulse (once per crossing)
  float recover = 0;  // 1 -> 0 recovery shimmer at the receding ends
  float breathe = 0;  // 0..1 broken-state breathing
  float dim = 1;      // GETTING_UP is shown slightly dimmed
  bool broken = false;
};

class QuadSink {
 public:
  QuadSink(Quad* out, unsigned cap) noexcept : out_(out), cap_(cap) {}
  void add(float x0, float y0, float x1, float y1, Rgba tl, Rgba tr, Rgba br, Rgba bl) noexcept {
    if (!(isfin(x0) && isfin(y0) && isfin(x1) && isfin(y1)) || x1 - x0 <= 0.0f || y1 - y0 <= 0.0f) return;
    if ((tl.a | tr.a | br.a | bl.a) == 0) return;
    if (n_ >= cap_ || !out_) {
      ++dropped_;
      return;
    }
    out_[n_++] = Quad{x0, y0, x1, y1, tl, tr, br, bl};
  }
  void vgrad(float x0, float y0, float x1, float y1, Rgba top, Rgba bottom) noexcept { add(x0, y0, x1, y1, top, top, bottom, bottom); }
  void hgrad(float x0, float y0, float x1, float y1, Rgba left, Rgba right) noexcept { add(x0, y0, x1, y1, left, right, right, left); }
  unsigned count() const noexcept { return n_; }
  unsigned dropped() const noexcept { return dropped_; }

 private:
  Quad* out_;
  unsigned cap_, n_ = 0, dropped_ = 0;
};

// A symmetric tapered layer inset by d: flat body + kTipSteps steps per tip (vertical gradient top -> bottom).
inline void layer(QuadSink& q, float cx, float cy, float d, Rgba top, Rgba bottom) noexcept {
  const float b = geo::kBodyHalf - d, e = geo::layer_end(d), h0 = geo::layer_h(0.0f, d);
  q.vgrad(cx - b, cy - h0, cx + b, cy + h0, top, bottom);
  for (unsigned k = 0; k < geo::kTipSteps; ++k) {
    const float ua = b + (e - b) * static_cast<float>(k) / geo::kTipSteps, ub = b + (e - b) * static_cast<float>(k + 1) / geo::kTipSteps;
    const float h = geo::layer_h((ua + ub) * 0.5f, d);
    q.vgrad(cx + ua, cy - h, cx + ub, cy + h, top, bottom);
    q.vgrad(cx - ub, cy - h, cx - ua, cy + h, top, bottom);
  }
}

// Builds the whole bar. Order = painter order (back to front). Returns the number of quads.
inline unsigned build(const Visual& v, float sx, float sy, Quad* out, unsigned cap) noexcept {
  QuadSink q(out, cap);
  const float a = clamp01(v.alpha) * clamp01(v.dim);
  if (!isfin(sx) || !isfin(sy) || a < 0.01f) return 0;
  using namespace palette;
  const float cx = sx, cy = geo::center_y(sy), H = geo::kHalfH;
  const float f = clamp01(v.fill), hit = clamp01(v.hit), pulse = clamp01(v.pulse), rec = clamp01(v.recover), br = clamp01(v.breathe);
  // 1. soft dark halo (contrast on bright scenes), tapered by a horizontal fade
  {
    const float w = geo::kLifeHalfW - 0.6f, hh = H + geo::kHalo;
    q.vgrad(cx - w, cy - hh, cx + w, cy + hh, alpha(kBlack, 0.30f * a), alpha(kBlack, 0.42f * a));
    q.hgrad(cx + w, cy - H, cx + w + 1.2f, cy + H, alpha(kBlack, 0.36f * a), alpha(kBlack, 0.0f));
    q.hgrad(cx - w - 1.2f, cy - H, cx - w, cy + H, alpha(kBlack, 0.0f), alpha(kBlack, 0.36f * a));
  }
  Rgba base = v.broken ? mix(kBrokenA, kBrokenB, 0.65f * br) : ramp(f);
  base = mix(base, kWhite, 0.35f * hit);
  // 2. break pulse: one expanding ember ring behind the frame (vertical growth bounded so it never reaches the life bar)
  if (pulse > 0.0f) {
    const float g = 1.0f - pulse, wx = geo::kLifeHalfW + geo::kPulseGrowX * g, hy = H + geo::kPulsePad + geo::kPulseGrowY * g;
    const Rgba c = alpha(kPulse, 0.75f * pulse * a), z = alpha(kPulse, 0.0f);
    q.hgrad(cx - wx, cy - hy, cx, cy + hy, z, c);
    q.hgrad(cx, cy - hy, cx + wx, cy + hy, c, z);
  }
  // 3. warm bloom of the filled span, peeking out of the frame edges
  const float E = geo::fill_extent(f);
  if (E > 0.05f) {
    const float bl = (v.broken ? 0.30f + 0.20f * br : 0.16f) + 0.20f * hit;
    q.vgrad(cx - E, cy - H - geo::kBloom, cx + E, cy + H + geo::kBloom, alpha(base, bl * a), alpha(base, bl * a));
  }
  // 4. subtle double border (bronze, then a dark line) and the dark track; tips are tapered
  const Rgba frameTop = v.broken ? mix(kBronzeLight, kBrokenFrame, 0.7f) : kBronzeLight, frameBottom = v.broken ? mix(kBronzeDark, kBrokenFrame, 0.5f) : kBronzeDark;
  layer(q, cx, cy, 0.0f, alpha(frameTop, 0.92f * a), alpha(frameBottom, 0.92f * a));
  layer(q, cx, cy, geo::kBronze, alpha(kEdgeDark, 0.95f * a), alpha(kEdgeDark, 0.95f * a));
  layer(q, cx, cy, geo::kCoreInset, alpha(kTrackTop, 0.92f * a), alpha(kTrackBottom, 0.92f * a));
  // 5. the fill grows from the CENTRE to BOTH ends: centre slightly deeper, ends brighter; top lit, bottom shaded
  if (E > 0.0f) {
    const float d = geo::kCoreInset, bcore = geo::kBodyHalf - d, hc = geo::layer_h(0.0f, d), Eb = minf(E, bcore);
    const Rgba edge = mix(base, kWhite, 0.10f), centre = mix(base, kBlack, 0.22f);
    const Rgba eT = alpha(mix(edge, kWhite, 0.18f), a), eB = alpha(mix(edge, kBlack, 0.25f), a), cT = alpha(mix(centre, kWhite, 0.18f), a),
               cB = alpha(mix(centre, kBlack, 0.25f), a);
    q.add(cx - Eb, cy - hc, cx, cy + hc, eT, cT, cB, eB);
    q.add(cx, cy - hc, cx + Eb, cy + hc, cT, eT, eB, cB);
    if (E > bcore) {
      const float e = geo::layer_end(d);
      for (unsigned k = 0; k < geo::kTipSteps; ++k) {
        const float ua = bcore + (e - bcore) * static_cast<float>(k) / geo::kTipSteps, ub = minf(E, bcore + (e - bcore) * static_cast<float>(k + 1) / geo::kTipSteps);
        if (ub <= ua) break;
        const float h = geo::layer_h((ua + ub) * 0.5f, d);
        q.vgrad(cx + ua, cy - h, cx + ub, cy + h, eT, eB);
        q.vgrad(cx - ub, cy - h, cx - ua, cy + h, eT, eB);
      }
    }
    // gloss line on the upper third of the core
    q.vgrad(cx - Eb, cy - hc, cx + Eb, cy - hc * 0.35f, alpha(kWhite, 0.22f * a), alpha(kWhite, 0.04f * a));
    // white flash over the fill at the crossing (pulse) -- once
    if (pulse > 0.0f) q.vgrad(cx - E, cy - hc, cx + E, cy + hc, alpha(kWhite, 0.80f * pulse * pulse * a), alpha(kWhite, 0.55f * pulse * pulse * a));
    // leading edges: hot on hits, pale and cool while recovering
    if (E > 0.3f) {
      const float w = 0.5f, hs = geo::layer_h(E, d) + 0.2f;
      Rgba spark = alpha(mix(edge, kWhite, 0.45f + 0.45f * hit), (0.50f + 0.45f * hit) * a);
      if (rec > 0.0f && hit <= 0.0f) spark = alpha(mix(edge, kRecover, 0.4f + 0.6f * rec), (0.35f + 0.55f * rec) * a);
      q.add(cx + E - w, cy - hs, cx + E, cy + hs, alpha(spark, 0.0f), spark, spark, alpha(spark, 0.0f));
      q.add(cx - E, cy - hs, cx - E + w, cy + hs, spark, alpha(spark, 0.0f), alpha(spark, 0.0f), spark);
    }
  }
  // 6. centre notches (top and bottom) -- the bar's origin
  {
    const float top = cy - H, bot = cy + H, n = geo::kNotch;
    const Rgba c = alpha(frameTop, 0.95f * a);
    q.vgrad(cx - 0.6f, top - n * 0.5f, cx + 0.6f, top, c, c);
    q.vgrad(cx - 0.3f, top - n, cx + 0.3f, top - n * 0.5f, c, c);
    q.vgrad(cx - 0.6f, bot, cx + 0.6f, bot + n * 0.5f, c, c);
    q.vgrad(cx - 0.3f, bot + n * 0.5f, cx + 0.3f, bot + n, c, c);
  }
  return q.count();
}

// Bounding box of a built bar (lab and layout tests).
struct Bounds {
  float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  bool any = false;
};
inline Bounds bounds(const Quad* q, unsigned n) noexcept {
  Bounds b;
  for (unsigned i = 0; i < n; ++i) {
    if (!b.any) b = Bounds{q[i].x0, q[i].y0, q[i].x1, q[i].y1, true};
    b.x0 = minf(b.x0, q[i].x0), b.y0 = minf(b.y0, q[i].y0), b.x1 = maxf(b.x1, q[i].x1), b.y1 = maxf(b.y1, q[i].y1);
  }
  return b;
}

// ---- per-actor display state -----------------------------------------------------------------------------------------------
// POST06 phase numbers as published by break_core.hpp (Tracking, Punish = STAGGER_OPEN, Falling, Down = DOWN_FINISH, GettingUp, Dead): checked in mod.cpp.
enum class Ph : std::uint8_t { Tracking = 0, Punish = 1, Falling = 2, Down = 3, GettingUp = 4, Dead = 5, Unknown = 255 };

struct In {
  bool present = false;  // ledger slot valid AND the actor is in this tick's live queue with the same address and id
  std::uint32_t id = 0;
  std::uint64_t generation = 0;
  int posture = 0;  // POST04 sidecar value 0..100
  bool latched = false, dead = false, frozen = false;
  Ph phase = Ph::Unknown;  // POST06 phase of the SAME identity, Unknown otherwise
  float opacity = 0;       // EHB visibility rules 0..1 (on screen, targetable, line of sight, distance fade)
};

inline constexpr std::uint8_t kHitTicks = 6, kPulseTicks = 20, kRecoverTicks = 15;  // 30 Hz simulation ticks

struct HudCounters {
  std::uint64_t ticks = 0, binds = 0, identity_resets = 0, cleared_missing = 0, cleared_dead = 0, pulses = 0, hits = 0, recoveries = 0, frozen_hidden = 0,
                paused_ticks = 0, published = 0, painted = 0, over_budget = 0, quads = 0, quad_drops = 0, queued = 0, refused = 0, hud_not_queued = 0,
                packet_draws = 0, draw_skipped = 0, interp_hits = 0, interp_misses = 0, scrubbed = 0, max_visible = 0;
};

class Hud {
 public:
  struct Slot {
    bool bound = false, last_latched = false, broken = false;
    std::uint32_t id = 0;
    std::uint64_t generation = 0, key = 0;
    float display = 0, alpha = 0;
    int target = 0;
    std::uint8_t hit = 0, pulse = 0, recover = 0;
    Ph phase = Ph::Unknown;
  };
  HudCounters c{};

  const Slot& slot(unsigned i) const noexcept { return s_[i < kSlots ? i : 0]; }
  void clear(unsigned i) noexcept {
    if (i < kSlots) s_[i] = Slot{};
  }
  void clear_all() noexcept {
    for (unsigned i = 0; i < kSlots; ++i) s_[i] = Slot{};
  }

  // One simulation tick for slot i. `advance` = false while paused / HUD suppressed: nothing moves, nothing pulses.
  void tick(unsigned i, const In& in, bool advance) noexcept {
    if (i >= kSlots) return;
    Slot& s = s_[i];
    const bool dead = in.dead || in.phase == Ph::Dead;
    if (!in.present || in.id == 0 || dead) {  // gone, retired or dead: hidden NOW (no ghost bar, no fade on a vanished anchor)
      if (s.bound) ++(dead ? c.cleared_dead : c.cleared_missing);
      s = Slot{};
      return;
    }
    const int t = in.posture < 0 ? 0 : (in.posture > 100 ? 100 : in.posture);
    if (!s.bound || s.id != in.id || s.generation != in.generation) {  // new or recycled identity: never inherit the previous actor's values
      if (s.bound) ++c.identity_resets;
      s = Slot{};
      s.bound = true;
      s.id = in.id;
      s.generation = in.generation;
      s.key = ++next_key_;
      s.display = static_cast<float>(t);
      s.target = t;
      s.last_latched = in.latched;  // first observation: an already broken actor does not pulse
      ++c.binds;
    }
    if (!advance) {
      ++c.paused_ticks;
      return;
    }
    if (s.hit) --s.hit;
    if (s.pulse) --s.pulse;
    if (s.recover) --s.recover;
    if (t > s.target) {
      s.hit = kHitTicks;
      ++c.hits;
    } else if (t < s.target) {
      s.recover = kRecoverTicks;
      ++c.recoveries;
    }
    if (in.latched && !s.last_latched) {  // the crossing to 100: ONE pulse (shield preparation, punish hits and later ticks keep latched => no new pulse)
      s.pulse = kPulseTicks;
      ++c.pulses;
    }
    s.last_latched = in.latched;
    s.target = t;
    s.broken = in.latched;
    s.phase = in.phase;
    // display smoothing in simulation time: fast rise, slower retraction (regeneration reads as a continuous retreat towards the centre)
    const float T = static_cast<float>(t);
    if (T > s.display) {
      s.display += maxf((T - s.display) * 0.5f, 2.0f);
      if (s.display > T) s.display = T;
    } else if (T < s.display) {
      s.display -= maxf((s.display - T) * ((s.display - T) > 25.0f ? 0.16f : 0.12f), 0.2f);
      if (s.display < T) s.display = T;
    }
    // visibility: only with content (posture above 0, an animation running, or broken); fades in fast, out slower
    const bool content = s.display > 0.5f || t > 0 || s.pulse || s.hit || in.latched;
    if (in.frozen) ++c.frozen_hidden;
    const float want = (!in.frozen && content) ? clamp01(in.opacity) : 0.0f;
    s.alpha += (want - s.alpha) * (want > s.alpha ? 0.35f : 0.20f);
    if (want <= 0.0f && s.alpha < 0.01f) s.alpha = 0.0f;
  }

  bool drawable(unsigned i) const noexcept { return i < kSlots && s_[i].bound && s_[i].alpha >= 0.01f; }

  Visual visual(unsigned i, std::uint32_t sim_tick) const noexcept {
    Visual v;
    if (i >= kSlots || !s_[i].bound) return v;
    const Slot& s = s_[i];
    v.fill = clamp01(s.display / 100.0f);
    v.alpha = s.alpha;
    v.hit = static_cast<float>(s.hit) / kHitTicks;
    v.pulse = static_cast<float>(s.pulse) / kPulseTicks;
    v.recover = static_cast<float>(s.recover) / kRecoverTicks;
    v.broken = s.broken;
    v.dim = s.broken && s.phase == Ph::GettingUp ? 0.8f : 1.0f;
    if (s.broken) {
      const unsigned period = (s.phase == Ph::Falling || s.phase == Ph::Down) ? 24u : (s.phase == Ph::GettingUp ? 0u : 40u);
      if (period) {
        const float x = static_cast<float>(sim_tick % period) / static_cast<float>(period), tri = 1.0f - (x > 0.5f ? 2.0f * x - 1.0f : 1.0f - 2.0f * x);
        v.breathe = tri * tri * (3.0f - 2.0f * tri);
      }
    }
    return v;
  }

 private:
  Slot s_[kSlots]{};
  std::uint64_t next_key_ = 0;
};

// Nearest-first selection of at most `cap` drawable bars (stable for equal distances). Returns the count written to `out`.
inline unsigned select_nearest(const bool* ok, const float* dist, unsigned n, unsigned cap, std::uint8_t* out) noexcept {
  unsigned m = 0;
  bool taken[kSlots] = {};
  if (n > kSlots) n = kSlots;
  while (m < cap) {
    int best = -1;
    for (unsigned i = 0; i < n; ++i) {
      if (!ok[i] || taken[i]) continue;
      const float d = isfin(dist[i]) ? dist[i] : 3.0e38f;
      if (best < 0 || d < (isfin(dist[best]) ? dist[best] : 3.0e38f)) best = static_cast<int>(i);
    }
    if (best < 0) break;
    taken[best] = true;
    out[m++] = static_cast<std::uint8_t>(best);
  }
  return m;
}

// ---- EHB-equivalent anchor and visibility (re-implemented independently from the published rules of Enemy Health Bars CP06) -------------
struct P3 {
  float x = 0, y = 0, z = 0;
};
// stable head height: attention point height, else eye height, else 55; it only rises (max 3 units per tick, 20 % per tick), never drops
inline float stable_head_height(float previous, P3 pos, P3 eye, P3 attention) noexcept {
  float h = attention.y - pos.y;
  if (!isfin(h) || h < 20.0f || h > 600.0f) h = eye.y - pos.y;
  if (!isfin(h) || h < 20.0f || h > 600.0f) h = 55.0f;
  if (!(previous > 0.0f)) return h;
  if (h > previous + 3.0f) {
    const float step = (h - previous) * 0.2f;
    return previous + (step > 3.0f ? 3.0f : step);
  }
  return previous;
}
inline P3 anchor_of(P3 pos, float head) noexcept { return P3{pos.x, pos.y + head + 8.0f, pos.z}; }
inline bool on_screen(P3 o, float width, float height) noexcept {  // o: ortho point minus (minX, minY)
  if (!isfin(o.x) || !isfin(o.y) || !isfin(o.z)) return false;
  const float right = width > 0.0f ? width + 80.0f : 720.0f, bottom = height > 0.0f ? height + 80.0f : 560.0f;
  return o.z < 400000.0f && o.x >= -80.0f && o.x <= right && o.y >= -80.0f && o.y <= bottom;
}
inline float opacity(bool onscreen, bool targetable, bool clear, float distance) noexcept {
  if (!onscreen || !targetable || !clear || !isfin(distance) || distance < 0.0f || distance >= 4500.0f) return 0.0f;
  return distance <= 3700.0f ? 1.0f : (4500.0f - distance) / 800.0f;
}

// ---- what the packet paints: VALUE copies made during the simulation tick (no actor address, no game pointer) -------------
struct DrawRec {
  std::uint64_t key = 0;  // HUD identity key (interpolation key); never an address
  P3 anchor;              // world anchor sampled this tick from the live actor
  Visual vis;
  float distance = 0;
};
struct Frame {
  DrawRec rec[kMaxDrawn]{};
  unsigned n = 0;
  void clear() noexcept { n = 0; }
};

// Per-identity stable anchor (head height only rises), reset when the slot's id/generation changes. Kept apart from Hud::Slot because it is
// computed BEFORE the visibility (which needs the projected anchor) and therefore before Hud::tick.
class Anchors {
 public:
  P3 update(unsigned i, std::uint32_t id, std::uint64_t generation, P3 pos, P3 eye, P3 attention) noexcept {
    if (i >= kSlots) return P3{};
    if (id_[i] != id || gen_[i] != generation) {
      id_[i] = id;
      gen_[i] = generation;
      head_[i] = 0.0f;
    }
    head_[i] = stable_head_height(head_[i], pos, eye, attention);
    return anchor_of(pos, head_[i]);
  }
  float head(unsigned i) const noexcept { return i < kSlots ? head_[i] : 0.0f; }
  void clear_all() noexcept {
    for (unsigned i = 0; i < kSlots; ++i) id_[i] = 0, gen_[i] = 0, head_[i] = 0.0f;
  }

 private:
  std::uint32_t id_[kSlots] = {};
  std::uint64_t gen_[kSlots] = {};
  float head_[kSlots] = {};
};

// One simulation pass over the 64 slots: animate every slot, then publish at most kMaxDrawn value records, nearest first.
// `advance` = false (pause, menu, event, HUD hidden): nothing animates and nothing is published.
struct Sample {
  In in;
  P3 anchor;
  float distance = 4500.0f;
};
inline void publish(Hud& h, const Sample* s, bool advance, std::uint32_t sim_tick, Frame& f) noexcept {
  f.clear();
  ++h.c.ticks;
  bool ok[kSlots] = {};
  float dist[kSlots] = {};
  unsigned visible = 0;
  for (unsigned i = 0; i < kSlots; ++i) {
    h.tick(i, s[i].in, advance);
    ok[i] = advance && h.drawable(i);
    dist[i] = s[i].distance;
    visible += ok[i] ? 1u : 0u;
  }
  std::uint8_t sel[kMaxDrawn] = {};
  const unsigned n = select_nearest(ok, dist, kSlots, kMaxDrawn, sel);
  if (visible > n) h.c.over_budget += visible - n;
  if (visible > h.c.max_visible) h.c.max_visible = visible;
  for (unsigned k = 0; k < n; ++k) {
    const unsigned i = sel[k];
    f.rec[f.n++] = DrawRec{h.slot(i).key, s[i].anchor, h.visual(i, sim_tick), s[i].distance};
  }
  h.c.published += f.n;
}

// ---- J2D colour mapping ------------------------------------------------------------------------------------------------------
// J2DGrafContext::fillBox emits (i.x,i.y)<-mColorTL, (f.x,i.y)<-mColorTR, (f.x,f.y)<-mColorBL, (i.x,f.y)<-mColorBR [S J2DGrafContext.cpp:147-156]:
// the two BOTTOM corners take the opposite member. To obtain the screen colours of a Quad, setColor(TL, TR, BR, BL) receives (tl, tr, bl, br).
struct J2DArgs {
  Rgba tl, tr, br, bl;  // arguments in setColor(colorTL, colorTR, colorBR, colorBL) order
};
inline constexpr J2DArgs j2d_args(const Quad& q) noexcept { return J2DArgs{q.tl, q.tr, q.bl, q.br}; }

// ---- 2D draw-list helpers (pure templates over a list type with the dDlst_list_c member names; tested on a model in the lab) -------
template <class T, unsigned N>
bool list_contains(T* const (&list)[N], T* const* end, const T* needle) noexcept {
  if (!needle || end < list || end > list + N) return false;
  for (T* const* i = list; i < end; ++i)
    if (*i == needle) return true;
  return false;
}
// Removes every entry equal to `packet`, compacting the rest and nulling the freed tail; untouched when `end` is not inside the list.
template <class T, unsigned N>
unsigned scrub_list(T* (&list)[N], T**& end, const T* packet) noexcept {
  if (end < list || end > list + N) return 0;
  T** dst = list;
  unsigned removed = 0;
  for (T** p = list; p < end; ++p) {
    if (*p != packet) *dst++ = *p;
    else ++removed;
  }
  for (T** p = dst; p < end; ++p) *p = nullptr;
  end = dst;
  return removed;
}
enum class Queue : std::uint8_t { HudNotQueued, Queued, Refused };
// Queue the packet in the 2D XLU list only when the native HUD meter is queued this tick (it honours pause menus, minimal HUD, cutscene camera...).
// A refused set() (list full / not a simulation frame) is detected by the list pointer not advancing.
template <class D, class B>
Queue queue_after_hud(D& d, const B* hud, B* packet) noexcept {
  const bool found = hud && (list_contains(d.mp2DOpaTopDrawLists, d.mp2DOpaTopStart, hud) || list_contains(d.mp2DOpaDrawLists, d.mp2DOpaStart, hud));
  if (!found) return Queue::HudNotQueued;
  auto** before = d.mp2DXluStart;
  d.set2DXlu(packet);
  return d.mp2DXluStart != before ? Queue::Queued : Queue::Refused;
}
template <class D, class B>
unsigned scrub_all(D& d, const B* packet) noexcept {
  return scrub_list(d.mp2DOpaTopDrawLists, d.mp2DOpaTopStart, packet) + scrub_list(d.mp2DOpaDrawLists, d.mp2DOpaStart, packet) +
         scrub_list(d.mp2DXluDrawLists, d.mp2DXluStart, packet);
}
}  // namespace post07
