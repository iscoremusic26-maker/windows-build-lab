// LNK01 laboratory, kept for 0.10.0 (LAB_MODEL_ONLY: pure logic, no game). Families L01..L19 of POSTURE_SYSTEM/LNK01/IMPLEMENTATION.md.
// The block scenarios run the REAL post08::Parry / ArmTrack / LinkTrack (parry_core.hpp) and the REAL lnk01 cores on a tick model that reproduces the
// host's collision-record semantics (only ARMED At spheres are registered and cleared; E_OC arms in clip frames [14,22]; native rebound when Link guards;
// a block puts Link in GUARD_SLIP, every 4th consecutive block GUARD_BREAK). Nothing here receives a precomputed "blocked=true": contacts come from the
// modelled collision pass. Optional: --render <dir> writes LNK01 reference PNGs rasterised from the REAL quad list (not game screenshots).
#include "../src/hud_core.hpp"
#include "../src/lnk_core.hpp"
#include "../src/parry_core.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using post07::Quad;
using post07::Rgba;
using post08::Result;
static unsigned checks = 0, failures = 0;
static unsigned fam_checks[20] = {}, fam_fail[20] = {};
static int fam = 0;
static void ck(bool b, const char* what = "") {
  ++checks;
  ++fam_checks[fam];
  if (!b) {
    ++failures;
    ++fam_fail[fam];
    if (fam_fail[fam] <= 4) std::printf("FAIL L%02d %s\n", fam, what);
  }
}
static bool near(float a, float b, float eps = 1e-3f) { return std::fabs(a - b) <= eps; }

// ============================== block / parry tick model ===================================================================================
enum LinkProc { kIdle = 0, kGuard = 1, kGuardAttack = 2, kGuardSlip = 3, kGuardBreak = 4, kDamaged = 5 };
struct Foe {
  std::uint32_t id = 100;
  std::uint64_t gen = 1;
  bool e_oc2 = false;
  int atp = 1;
  bool front = true;      // Link faces it (shield covers it)
  bool reach = true;      // its blade reaches Link
  int state = 0;          // 0 prepare, 1 swing, 3 rebound, 9 find
  float frame = 0, rate = 1.0f, base_rate = 1.0f;
  int find_ticks = 0, first_delay = 0;
  bool hit[2] = {false, false}, shield[2] = {false, false};  // persistent At records
  bool overlap_used = false, armed_this_tick = false, swing_parried = false;
  int swings = 0;
};
struct SimParams {
  int ticks = 300;
  bool link_first = true;
  bool hold_guard = true;
  bool both_spheres = false;  // both At spheres of the weapon register the same overlap in the same pass
  std::vector<int> sa_ticks;  // Shield Attack presses
  std::vector<Foe> foes = {Foe{}};
  bool wolf = false;
  int event_from = -1, event_to = -1;
  int pause_from = -1, pause_to = -1;
  bool double_callback = false;  // the adapter receives the same verdict twice (repeated callback)
  bool fresh_gate = true;        // FIX02 gate on (always in 0.9.0); off = model of a stale reader
  int link_change_at = -1;       // Link actor replaced (new id) at this tick
};
struct SimOut {
  int native_blocks = 0, native_hits = 0, guard_breaks = 0, parries = 0, link_damage = 0;
  std::vector<int> posture_trace;  // milli per tick (after mod_update)
  lnk01::PCounters pc;
  post08::PCounters parry_c;
  int final_milli = 0, peak = 0;
  std::vector<int> credited_ticks;
};
static SimOut run(const SimParams& sp, bool attach_lnk = true) {
  SimOut out;
  post08::Parry p;
  p.enabled = true;
  post08::LinkTrack track;
  post08::ArmTrack arm;
  lnk01::Posture lp;
  std::vector<Foe> foes = sp.foes;
  for (size_t i = 0; i < foes.size(); ++i) foes[i].find_ticks = foes[i].first_delay, foes[i].state = foes[i].first_delay ? 9 : 0;
  int proc = sp.hold_guard ? kGuard : kIdle, slip = 0, guard_counter = 0;  // guard_counter models the native field_0x2fd5 (never read by lnk01)
  float lframe = 0;
  bool pend_block = false, pend_hit = false;
  std::uint32_t link_id = 7;
  auto paused = [&](int t) { return t >= sp.pause_from && t < sp.pause_to; };
  auto event = [&](int t) { return t >= sp.event_from && t < sp.event_to; };
  auto link_exec = [&](int t) {
    if (paused(t)) return;
    if (pend_block) {
      pend_block = false;
      ++out.native_blocks;
      if (++guard_counter == 4) guard_counter = 0, proc = kGuardBreak, slip = 20, ++out.guard_breaks;  // d_a_alink_guard.inc:244-246
      else proc = kGuardSlip, slip = 8;
    }
    if (pend_hit) pend_hit = false, ++out.link_damage, proc = kDamaged, slip = 10;
    if (proc == kGuardSlip || proc == kDamaged || proc == kGuardBreak) {
      if (--slip <= 0) proc = sp.hold_guard ? kGuard : kIdle;
    } else if (std::find(sp.sa_ticks.begin(), sp.sa_ticks.end(), t) != sp.sa_ticks.end() && (proc == kIdle || proc == kGuard)) {
      proc = kGuardAttack, lframe = 1.0f;
    } else if (proc == kGuardAttack) {
      lframe += 1.0f;
      if (lframe > 18.0f) proc = sp.hold_guard ? kGuard : kIdle;
    }
  };
  auto view_of = [&] {
    post08::LinkView v;
    v.valid = true, v.proc = proc == kGuardAttack ? 0x1e : (proc == kGuardSlip ? 0x1d : (proc == kGuardBreak ? 0x1f : proc));
    v.guard_attack = proc == kGuardAttack, v.frame = lframe, v.start = 1, v.end = 12;
    return v;
  };
  auto foe_exec = [&](size_t i, int t) {
    if (paused(t)) return;
    Foe& f = foes[i];
    const int slot = static_cast<int>(i);
    // ---- damage_check PRE: the adapter's reads ----
    post08::Input in;
    in.tracked = true, in.slot = slot, in.generation = f.gen, in.id = f.id, in.hp = 50;
    in.mode = (f.state == 0 || f.state == 1 || f.state == 3) ? 4 : 3;
    in.state = f.state == 9 ? 4 : f.state;
    in.anim_ok = true, in.anim_frame = f.frame, in.anim_rate = f.rate, in.e_oc2 = f.e_oc2, in.atp = f.atp;
    for (int s = 0; s < 2; ++s) in.at_hit[s] = in.at_on_link[s] = f.hit[s], in.at_shield[s] = f.hit[s] && f.shield[s];
    in.link_ok = true;
    in.now = view_of();
    in.pass = track.pass_for(static_cast<std::uint64_t>(t), true, link_id);
    in.fresh = sp.fresh_gate ? arm.fresh(slot, f.id, f.gen, static_cast<std::uint64_t>(t)) : true;
    in.event_running = event(t);
    const post08::Verdict v = p.judge(in);
    // ---- damage_check POST: POST08 step (parry commit; native deflection) ----
    if (v.result == Result::Confirmed) {
      ++out.parries;
      f.swing_parried = true;
      p.commit(slot, f.gen, f.id, v.token, true);
    }
    // ---- LNK01 contact: the SAME verdict, after the POST08 step (mirror of lnk01_contact in mod.cpp) ----
    if (attach_lnk && v.result != Result::None) {
      lnk01::Contact k;
      k.result = v.result, k.blocked = v.blocked, k.token = v.token, k.slot = in.slot, k.id = in.id, k.generation = in.generation;
      const post08::Parry::Episode& e = p.episode(static_cast<unsigned>(slot));
      k.episode_parried = e.bound && e.id == in.id && e.generation == in.generation && e.token == v.token && e.parried;
      k.e_oc2 = in.e_oc2, k.atp = in.atp, k.link_ok = true, k.link_human = !sp.wolf, k.link_id = link_id, k.event_running = in.event_running;
      k.tick = static_cast<std::uint64_t>(t);
      for (int rep = 0; rep < (sp.double_callback ? 2 : 1); ++rep)
        if (lp.on_contact(k).v == lnk01::Verdict::Confirmed) out.credited_ticks.push_back(t);
    }
    arm.note_post(slot, f.id, f.gen, static_cast<std::uint64_t>(t), post08::ArmTrack::armed_now(in.mode == 4 && f.state == 1 ? 4 : 0, f.state == 1 ? 1 : 0, true, f.frame));
    // ---- executeAttack / FIND (native) ----
    f.armed_this_tick = false;
    const bool link_guard = proc == kGuard || proc == kGuardSlip;  // checkPlayerGuard()
    switch (f.state) {
      case 0: f.state = 1, f.frame = 0, f.rate = f.base_rate, f.overlap_used = false, f.swing_parried = false, ++f.swings; break;
      case 1:
        if (f.frame >= 14.0f && f.frame <= 22.0f) f.armed_this_tick = true;
        if ((f.hit[0] || f.hit[1]) && link_guard) {
          f.rate = -1.0f, f.state = 3;
          break;
        }
        f.frame += f.rate;
        if (f.frame >= 30.0f) f.state = 9, f.find_ticks = 8;
        break;
      case 3:
        f.frame += f.rate;
        if ((f.rate < 0 && f.frame <= 0.0f) || (f.rate > 0 && f.frame >= 30.0f)) f.state = 9, f.find_ticks = 8, f.rate = f.base_rate;
        break;
      case 9:
        if (--f.find_ticks <= 0) f.state = 0;
        break;
    }
  };
  for (int t = 1; t <= sp.ticks; ++t) {
    if (t == sp.link_change_at) link_id = 8;
    // ---- mod_update (start of the simulation tick) ----
    track.observe(static_cast<std::uint64_t>(t), true, link_id, view_of().proc, proc == kGuardAttack, lframe, 1, 12);
    if (attach_lnk) {
      lp.observe_link(true, link_id);
      lp.tick(!paused(t) && !event(t));
    }
    if (sp.link_first) link_exec(t);
    for (size_t i = 0; i < foes.size(); ++i) foe_exec(i, t);
    if (!sp.link_first) link_exec(t);
    // ---- collision pass (dScnPly_Draw -> Ccsp()->Move): only ARMED spheres are registered/cleared ----
    if (!paused(t))
      for (Foe& f : foes) {
        if (!f.armed_this_tick) continue;
        f.hit[0] = f.hit[1] = f.shield[0] = f.shield[1] = false;  // ClrAtHitInf of the registered spheres
        if (!f.overlap_used && f.frame >= 15.0f && f.reach) {
          f.overlap_used = true;
          const bool guarded = (proc == kGuardAttack || proc == kGuard || proc == kGuardSlip) && f.front;
          for (int s = 0; s < (sp.both_spheres ? 2 : 1); ++s) f.hit[s] = true, f.shield[s] = guarded;
          if (guarded) pend_block = true;
          else pend_hit = true;
        }
      }
    out.posture_trace.push_back(lp.milli());
  }
  out.pc = lp.c;
  out.parry_c = p.c;
  out.final_milli = lp.milli();
  for (int v : out.posture_trace) out.peak = std::max(out.peak, v);
  return out;
}
static int verdicts(const SimOut& o, lnk01::Verdict v) { return static_cast<int>(o.pc.verdict[static_cast<unsigned>(v)]); }

// ============================== bottom-centre prompt model (dMeterButton_c) ===============================================================
// Native behaviour modelled from d_meter_button.cpp: the root screen draws icons + action texts when visible; the floating message (waku + text) draws
// from OTHER screens; sparkles (drawPikari) draw at an icon position when its slot type is a known button and its sparkle timer > 0 (set by
// pikariCheck in _execute) or by the A-blink / A-repeat-hit paths inside draw itself. _execute never shows the ROOT (only sub-panes).
struct Prompt {
  bool exists = false;
  std::uint64_t key = 0;
  bool root_visible = true;
  bool screen_ok = true, aligned = true, vptr_ok = true;
  std::uint8_t slot[2] = {22, 22};
  float sparkle[2] = {0, 0};
  bool blink_a = false;
  bool floating = false;
};
struct Hud {  // everything else of the native HUD: the mod has NO handle to any of it
  bool hearts = true, rupees = true, cluster_top_right = true, minimap = true, enemy_bars = true, dialogue = true, meter_root = true;
  int do_status = 0, a_status = 0, inputs = 0;
};
struct Render {
  int icons = 0, texts = 0, sparkles = 0, messages = 0, other_hud = 0;
};
static bool known_button(std::uint8_t s) { return s <= 8 || s == 13 || s == 21; }
static void native_draw(const Prompt& m, const Hud& h, Render& r) {
  if (!m.exists) return;
  if (m.root_visible) r.icons += 1, r.texts += 1;
  if (m.floating) ++r.messages;
  for (int i = 0; i < 2; ++i) {
    const bool blink = m.slot[i] == 0 && m.blink_a;
    if (known_button(m.slot[i]) && (m.sparkle[i] > 0 || blink)) ++r.sparkles;
  }
  r.other_hud += (h.hearts && h.rupees && h.cluster_top_right && h.minimap && h.enemy_bars && h.meter_root) ? 1 : 0;
}
// The adapter (mirror of mod.cpp: action_ui_read / action_ui_step / prompt draw PRE+POST), driving the REAL lnk01::ActionUi and SparkleMask.
struct Glue {
  lnk01::ActionUi ui;
  lnk01::SparkleMask mask;
  bool on = true;
  int hide_writes = 0, slot_writes = 0;
  Glue() { ui.enabled = true; }
  lnk01::UiInput read(const Prompt& m, bool meter = true, std::uint64_t meter_key = 1) const {
    lnk01::UiInput in;
    if (!meter) return in;
    in.meter = true, in.meter_key = meter_key;
    if (!m.exists) return in;
    in.prompt = true, in.prompt_key = m.key;
    in.screen = m.screen_ok, in.aligned = m.screen_ok && m.aligned, in.vptr_ok = in.aligned && m.vptr_ok;
    if (in.vptr_ok) in.visible = m.root_visible;
    return in;
  }
  void step(Prompt& m, bool meter = true, std::uint64_t meter_key = 1) {
    if (!on) return;
    const lnk01::UiDecision d = ui.step(read(m, meter, meter_key));
    if (d.hide) m.root_visible = false, ++hide_writes;
  }
  void draw(Prompt& m, const Hud& h, Render& r, bool live = true) {
    if (on && live) step(m);
    lnk01::SparkleMask::Begin b{};
    if (on) b = mask.begin(live, live && ui.layout_ok_for(m.key), m.key, m.slot[0], m.slot[1], ui.c);
    if (b.apply) m.slot[0] = b.s0, m.slot[1] = b.s1, ++slot_writes;
    native_draw(m, h, r);
    if (on) {
      const lnk01::SparkleMask::End e = mask.end(m.key, ui.c);
      if (e.restore) m.slot[0] = e.s0, m.slot[1] = e.s1, ++slot_writes;
    }
  }
};
// Native _execute for one tick of a scripted action list. Recreates the prompt object when the emphasis family starts again (check2DContents).
static void native_execute(Prompt& m, Hud& h, int t, std::uint64_t& next_key) {
  static const std::uint8_t actions[] = {0, 1, 3, 2, 7, 8, 4, 5, 6, 13, 21};
  const int phase = (t / 15) % 12;  // every 0.5 s a new emphasised action; phase 11 = family closed (object deleted)
  if (phase == 11) {
    m.exists = false;
    return;
  }
  if (!m.exists) m = Prompt{}, m.exists = true, m.key = ++next_key;  // JKR_NEW dMeterButton_c: root visible again
  const std::uint8_t a = actions[phase % 11];
  if (m.slot[0] != a) m.slot[0] = a, m.sparkle[0] = 18.0f;  // setString + pikariCheck
  m.slot[1] = (t % 45 < 20) ? 1 : 22;
  for (float& s : m.sparkle) s = s > 0 ? s - 1.0f : 0.0f;
  m.blink_a = (t % 30) < 4;
  m.floating = (t % 90) < 30;
  h.do_status = a, h.a_status = (t / 7) % 5, h.inputs += 1;
}

// ============================== raster (reference renders) ================================================================================
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
struct Canvas {
  int w, h;
  float scale, ox, oy;
  std::vector<float> px;
  Canvas(int w_, int h_, float s, float ox_, float oy_) : w(w_), h(h_), scale(s), ox(ox_), oy(oy_), px(static_cast<size_t>(w_ * h_ * 3)) {}
  void blend(int x, int y, float r, float g, float b, float a) {
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    float* p = &px[static_cast<size_t>((y * w + x) * 3)];
    p[0] = p[0] * (1 - a) + r * a, p[1] = p[1] * (1 - a) + g * a, p[2] = p[2] * (1 - a) + b * a;
  }
  void tri(const float* X, const float* Y, const Rgba* C) {
    const float x0 = (X[0] - ox) * scale, y0 = (Y[0] - oy) * scale, x1 = (X[1] - ox) * scale, y1 = (Y[1] - oy) * scale, x2 = (X[2] - ox) * scale, y2 = (Y[2] - oy) * scale;
    const float den = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
    if (std::fabs(den) < 1e-9f) return;
    const int minx = static_cast<int>(std::floor(std::min(x0, std::min(x1, x2)))), maxx = static_cast<int>(std::ceil(std::max(x0, std::max(x1, x2))));
    const int miny = static_cast<int>(std::floor(std::min(y0, std::min(y1, y2)))), maxy = static_cast<int>(std::ceil(std::max(y0, std::max(y1, y2))));
    for (int y = std::max(miny, 0); y <= std::min(maxy, h - 1); ++y)
      for (int x = std::max(minx, 0); x <= std::min(maxx, w - 1); ++x) {
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
  void quad(const Quad& q) {
    const post07::J2DArgs a = post07::j2d_args(q);
    const float X[4] = {q.x0, q.x1, q.x1, q.x0}, Y[4] = {q.y0, q.y0, q.y1, q.y1};
    const Rgba V[4] = {a.tl, a.tr, a.bl, a.br};  // fillBox vertex colour order [S]
    const float Xa[3] = {X[0], X[1], X[2]}, Ya[3] = {Y[0], Y[1], Y[2]}, Xb[3] = {X[0], X[2], X[3]}, Yb[3] = {Y[0], Y[2], Y[3]};
    const Rgba Ca[3] = {V[0], V[1], V[2]}, Cb[3] = {V[0], V[2], V[3]};
    tri(Xa, Ya, Ca);
    tri(Xb, Yb, Cb);
  }
  std::vector<unsigned char> rgb() const {
    std::vector<unsigned char> o(px.size());
    for (size_t i = 0; i < px.size(); ++i) o[i] = post07::to_u8(px[i] * 255.0f);
    return o;
  }
};
static lnk01::Visual vis(float f, float a = 1, float pulse = 0, float hit = 0, float rec = 0, bool sat = false) {
  lnk01::Visual v;
  v.fill = f, v.alpha = a, v.pulse = pulse, v.hit = hit, v.recover = rec, v.saturated = sat;
  return v;
}
struct Res {
  const char* tag;
  int pw, ph;
};
static void ortho_for(int pw, int ph, float& minx, float& miny, float& w, float& h) {  // painter's logical 2D space: height 448, centred widescreen
  h = 448.0f, w = 448.0f * static_cast<float>(pw) / static_cast<float>(ph), minx = 304.0f - w * 0.5f, miny = 0.0f;
}
// Sheet: crops of the bottom band (rows 76..94 % of the screen) at the true physical resolution, one row per state; an enemy (POST07) bar is drawn
// at its real scale on the left of each row for size comparison.
static int render_sheet(const std::string& dir, Res r) {
  float minx, miny, w, h;
  ortho_for(r.pw, r.ph, minx, miny, w, h);
  const float scale = static_cast<float>(r.ph) / h;
  const lnk01::Layout L = lnk01::layout(minx, miny, w, h);
  struct Row {
    const char* name;
    lnk01::Visual v;
  };
  const Row rows[] = {{"posture_20", vis(0.20f)}, {"posture_40_hit", vis(0.40f, 1, 0, 1)}, {"posture_75", vis(0.75f)},
                      {"posture_100_pulse", vis(1.0f, 1, 0.6f, 0, 0, true)}, {"recover_45", vis(0.45f, 1, 0, 0, 1)}, {"fading_0", vis(0.0f, 0.5f)}};
  const int n = static_cast<int>(sizeof rows / sizeof rows[0]);
  const float y0 = h * 0.80f, y1 = h * 0.94f;  // band shown
  const int RH = static_cast<int>(std::ceil((y1 - y0) * scale));
  const int W = r.pw, H = RH * n;
  Canvas c(W, H, scale, minx, 0);
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x) {
      float* p = &c.px[static_cast<size_t>((y * W + x) * 3)];
      const float t = static_cast<float>(x) / static_cast<float>(W);
      p[0] = 0.40f + 0.12f * t, p[1] = 0.44f + 0.05f * t, p[2] = 0.36f;
    }
  for (int k = 0; k < n; ++k) {
    c.oy = y0 - static_cast<float>(k) * (y1 - y0);
    Quad q[lnk01::kMaxQuads];
    const unsigned m = lnk01::build_bar(rows[k].v, L, q, lnk01::kMaxQuads);
    for (unsigned i = 0; i < m; ++i) c.quad(q[i]);
    post07::Visual ev;
    ev.fill = 0.5f, ev.alpha = 1;
    Quad eq[post07::kMaxQuads];
    const unsigned em = post07::build(ev, minx + w * 0.12f, y0 + (y1 - y0) * 0.35f, eq, post07::kMaxQuads);
    for (unsigned i = 0; i < em; ++i) c.quad(eq[i]);
  }
  const std::string path = dir + "/LNK01_REFERENCE_RENDER_" + r.tag + ".png";
  const bool ok = write_png(path, W, H, c.rgb());
  std::printf("RENDER %s %dx%d rows=%d px_per_unit=%.3f bar_px=%.0fx%.0f\n", path.c_str(), W, H, n, scale, L.half_w * 2 * scale, L.frame_h * 2 * scale);
  for (int k = 0; k < n; ++k) std::printf("RENDER_ROW %d %s\n", k, rows[k].name);
  return ok ? 0 : 1;
}

static std::vector<Quad> bar(const lnk01::Visual& v, const lnk01::Layout& L) {
  Quad q[lnk01::kMaxQuads];
  const unsigned n = lnk01::build_bar(v, L, q, lnk01::kMaxQuads);
  return std::vector<Quad>(q, q + n);
}

int main(int argc, char** argv) {
  crc_init();
  if (argc >= 3 && std::strcmp(argv[1], "--render") == 0) {
    int rc = 0;
    rc |= render_sheet(argv[2], Res{"1080p", 1920, 1080});
    rc |= render_sheet(argv[2], Res{"720p", 1280, 720});
    rc |= render_sheet(argv[2], Res{"ultrawide_2560x1080", 2560, 1080});
    return rc;
  }
  float minx, miny, w, h;
  ortho_for(1920, 1080, minx, miny, w, h);
  const lnk01::Layout L1080 = lnk01::layout(minx, miny, w, h);

  // ---- L01 HUD values 0/1/20/40/75/99/100: symmetric centre->ends fill, saturation, recovery, fade, no ghost after a Link change ----
  fam = 1;
  {
    float prevE = -1;
    for (float f : {0.0f, 0.01f, 0.20f, 0.40f, 0.75f, 0.99f, 1.0f}) {
      const auto q = bar(vis(f), L1080);
      ck(!q.empty() && q.size() <= lnk01::kMaxQuads, "quads within budget");
      float lo = 1e9f, hi = -1e9f;
      for (const Quad& k : q) lo = std::min(lo, k.x0), hi = std::max(hi, k.x1);
      ck(near(lo - L1080.cx, -(hi - L1080.cx), 0.05f), "symmetric bounds");
      // fill extent = widest quad whose colour is the posture ramp (bright, non-frame); measured through the leading edge sparks
      float E = 0;
      const Rgba base = post07::ramp(f);
      for (const Quad& k : q)
        if (k.y0 >= L1080.cy - L1080.fill_h - 0.01f && k.y1 <= L1080.cy + L1080.fill_h + 0.01f && std::fabs(k.x1 - L1080.cx) > 0.01f && k.tl.a > 200 && k.x0 >= L1080.cx - 0.01f &&
            (k.tl.r > 150 || k.tr.r > 150))
          E = std::max(E, k.x1 - L1080.cx);
      (void)base;
      float El = 0;  // left half measured independently
      for (const Quad& k : q)
        if (k.y0 >= L1080.cy - L1080.fill_h - 0.01f && k.y1 <= L1080.cy + L1080.fill_h + 0.01f && std::fabs(L1080.cx - k.x0) > 0.01f && k.tl.a > 200 && k.x1 <= L1080.cx + 0.01f &&
            (k.tl.r > 150 || k.tr.r > 150))
          El = std::max(El, L1080.cx - k.x0);
      ck(near(E, El, 0.05f), "fill symmetric: left extent == right extent");
      ck(E <= L1080.half_w + 0.01f, "fill inside the track");
      ck(f == 0.0f ? E <= 0.0f : E > prevE, "fill grows with posture");
      if (f > 0.0f) ck(near(E, f * L1080.half_w, 0.05f), "fill extent = posture * half width (centre -> each end)");
      prevE = E;
    }
    // ramp: gold at low values, red at 100
    const Rgba lo = post07::ramp(0.2f), hi = post07::ramp(1.0f);
    ck(lo.g > hi.g && hi.r >= 0xD0, "colour warms towards red");
    // hidden when alpha 0
    ck(bar(vis(0.5f, 0.0f), L1080).empty(), "alpha 0 draws nothing");
    // PlayerHud: shows at the first block, holds 0.7 s at 0, then fades; a reset removes it at once
    lnk01::PlayerHud hud;
    ck(!hud.drawable(), "starts hidden");
    hud.tick(20000, true, true);
    ck(hud.drawable() && hud.visual().hit > 0, "first valid contact shows the bar");
    for (int i = 0; i < 6; ++i) hud.tick(20000, true, true);
    ck(near(hud.visual().fill, 0.20f, 0.01f), "display reaches 20/100");
    hud.tick(40000, true, true);
    for (int i = 0; i < 6; ++i) hud.tick(40000, true, true);
    ck(near(hud.visual().fill, 0.40f, 0.01f), "display reaches 40/100");
    int t0 = 0;
    while (hud.display() > 0.0f && t0 < 400) hud.tick(0, true, true), ++t0;
    ck(hud.drawable(), "still visible when the value just reached 0");
    int held = 0;
    while (hud.drawable() && hud.visual().alpha >= 0.999f && held < 100) hud.tick(0, true, true), ++held;
    ck(held >= 18 && held <= 24, "~0.7 s (21 ticks at 30 Hz) hold at 0 before fading");
    int fade = 0;
    while (hud.drawable() && fade < 100) hud.tick(0, true, true), ++fade;
    ck(fade >= 6 && fade <= 12, "gradual fade (~0.3 s), not a pop");
    ck(hud.c.fade_outs == 1, "one fade-out");
    hud.tick(100000, true, true);
    for (int i = 0; i < 4; ++i) hud.tick(100000, true, true);
    ck(hud.c.pulses == 1, "one pulse at 100");
    for (int i = 0; i < 30; ++i) hud.tick(100000, true, true);
    ck(hud.c.pulses == 1 && hud.visual().saturated, "no second pulse while staying at 100");
    hud.tick(95000, true, true), hud.tick(100000, true, true);
    ck(hud.c.pulses == 1, "no pulse re-arm above 90");
    hud.tick(80000, true, true), hud.tick(100000, true, true);
    ck(hud.c.pulses == 2, "re-armed below 90");
    hud.reset();
    ck(!hud.drawable() && hud.display() == 0.0f && hud.c.resets == 1, "reset: no ghost bar");
  }

  // ---- L02 layout at 720p/1080p/1440p/ultrawide/4:3: centred, ~87 % height, 30-34 % of the 16:9 safe width, far wider than an enemy bar ----
  fam = 2;
  {
    struct R {
      int pw, ph;
    };
    for (R r : {R{1280, 720}, R{1920, 1080}, R{2560, 1440}, R{3840, 2160}, R{2560, 1080}, R{3440, 1440}, R{5120, 1440}, R{1024, 768}, R{1600, 900}}) {
      float mx, my, ww, hh;
      ortho_for(r.pw, r.ph, mx, my, ww, hh);
      const lnk01::Layout L = lnk01::layout(mx, my, ww, hh);
      ck(L.ok, "layout ok");
      ck(near(L.cx, mx + ww * 0.5f, 1e-3f), "horizontally centred");
      ck(near((L.cy - my) / hh, 0.87f, 1e-3f), "87 % from the top");
      const float safe = std::min(ww, hh * 16.0f / 9.0f), frac = (2.0f * L.half_w) / safe;
      ck(frac >= 0.30f && frac <= 0.34f, "30-34 % of the 16:9 safe width");
      ck(2.0f * L.half_w > 6.0f * lnk01::policy::kEnemyBarWidthUnits, "much wider than an enemy bar (>6x even at 4:3)");
      ck(L.cx - L.half_w - 30 * L.u > mx && L.cx + L.half_w + 30 * L.u < mx + ww, "inside the viewport, with caps");
      ck(L.cy + L.frame_h + 6 * L.u < my + hh, "above the bottom edge");
      const float px_per_unit = static_cast<float>(r.ph) / hh;
      const float fill_px = 2.0f * L.fill_h * px_per_unit, frame_px = 2.0f * L.frame_h * px_per_unit;
      ck(fill_px / static_cast<float>(r.ph) * 1080.0f >= 9.0f && fill_px / static_cast<float>(r.ph) * 1080.0f <= 14.0f, "fill 9-14 px at 1080p equivalent");
      ck(frame_px > fill_px, "frame slightly taller than the fill");
      if (r.pw == 1920 && r.ph == 1080) {
        const float bar_px = 2.0f * L.half_w * px_per_unit;
        ck(bar_px >= 576.0f && bar_px <= 653.0f, "1080p: 576-653 px wide");
      }
      for (const Quad& k : bar(vis(1.0f, 1, 1.0f, 1, 0, true), L)) ck(k.x0 >= mx - 1 && k.x1 <= mx + ww + 1 && k.y1 <= my + hh, "every primitive on screen");
    }
    ck(!lnk01::layout(0, 0, 0, 448).ok && !lnk01::layout(0, 0, 800, NAN).ok, "degenerate viewport refused");
  }

  // ---- L03 lifecycle: one packet per pass, also when only the Link bar is visible; nothing when not allowed ----
  fam = 3;
  {
    ck(lnk01::packet_needed(true, 0, true), "Link bar alone queues the packet");
    ck(lnk01::packet_needed(true, 3, false), "enemy bars alone queue the packet");
    ck(!lnk01::packet_needed(true, 0, false), "nothing to draw: no packet");
    ck(!lnk01::packet_needed(false, 3, true) && !lnk01::packet_needed(false, 0, true), "HUD not allowed (pause/menu/event): no packet");
    // HUD meter destroyed/recreated: ActionUi forgets, the next HUD is resolved again from scratch
    Glue gl;
    Prompt m;
    m.exists = true, m.key = 10;
    gl.step(m, true, 1);
    ck(!m.root_visible && gl.ui.c.prompts_seen == 1, "resolved on the first HUD");
    gl.ui.forget();
    Prompt m2;
    m2.exists = true, m2.key = 10;  // same address reused after a delete
    gl.step(m2, true, 2);
    ck(!m2.root_visible && gl.ui.c.prompts_seen == 2 && gl.ui.c.meters_seen == 2, "recreated HUD resolved again (no stale binding)");
  }

  // ---- L04/L05/L06/L07 prompt suppression: 600 ticks x presentation frames, action changes, object recreation, floating messages ----
  {
    Hud hud_on, hud_off;
    Prompt m_on, m_off;
    std::uint64_t k_on = 0, k_off = 0;
    Glue on, off;
    off.on = false;
    Render r_on, r_off;
    bool slots_preserved = true, status_identical = true, never_visible_at_draw = true;
    int recreated = 0;
    for (int t = 1; t <= 720; ++t) {
      const std::uint64_t before = m_on.key;
      native_execute(m_on, hud_on, t, k_on);
      native_execute(m_off, hud_off, t, k_off);
      if (m_on.key != before && m_on.exists) ++recreated;
      on.step(m_on);  // dMeter2_c::_execute POST
      for (int frame = 0; frame < 2; ++frame) {  // 60 Hz presentation over a 30 Hz simulation: draw twice per tick
        on.step(m_on);                           // dMeter2_c::_draw POST
        const std::uint8_t s0 = m_on.slot[0], s1 = m_on.slot[1];
        Render tmp;
        on.draw(m_on, hud_on, tmp);
        if (tmp.icons || tmp.texts) never_visible_at_draw = false;
        r_on.icons += tmp.icons, r_on.texts += tmp.texts, r_on.sparkles += tmp.sparkles, r_on.messages += tmp.messages, r_on.other_hud += tmp.other_hud;
        if (m_on.slot[0] != s0 || m_on.slot[1] != s1) slots_preserved = false;
        off.draw(m_off, hud_off, r_off);
      }
      if (hud_on.do_status != hud_off.do_status || hud_on.a_status != hud_off.a_status || hud_on.inputs != hud_off.inputs) status_identical = false;
      if (m_on.exists && m_off.exists && (m_on.slot[0] != m_off.slot[0] || m_on.slot[1] != m_off.slot[1])) slots_preserved = false;
    }
    fam = 4;  // permanent hiding at any posture, 100+ cycles
    ck(r_off.icons > 600 && r_off.texts > 600, "control: without the mod the prompts draw");
    ck(r_on.icons == 0 && r_on.texts == 0 && never_visible_at_draw, "with the mod no icon/text is ever drawn");
    ck(on.ui.c.hidden_checks > 1000 && on.ui.c.refused == 0, "hidden on every check");
    ck(on.hide_writes == recreated + 1 || on.hide_writes == recreated, "one write per new prompt object (idempotent, no alpha oscillation)");
    fam = 5;  // action changes, sparkles, blink paths
    ck(r_off.sparkles > 50, "control: sparkles/blinks happen natively");
    ck(r_on.sparkles == 0, "no orphan sparkle (pikari / blink) with the mod");
    ck(recreated >= 5, "prompt object recreated many times");
    fam = 6;  // preservation: floating message and the rest of the HUD
    ck(r_on.messages == r_off.messages && r_on.messages > 100, "floating messages kept");
    ck(r_on.other_hud == r_off.other_hud, "hearts/rupees/top-right cluster/minimap/enemy bars/meter root untouched");
    ck(hud_on.meter_root && hud_on.hearts && hud_on.cluster_top_right && hud_on.dialogue, "no global HUD flag touched");
    fam = 7;  // controls: logic state identical with and without the mod
    ck(status_identical, "Do/A status and inputs identical");
    ck(slots_preserved, "slot state after every draw identical to native (mask restored)");
    ck(on.ui.c.masks == on.ui.c.mask_restores && on.ui.c.mask_unpaired == 0, "every mask restored");
  }

  // ---- L08 safety: missing/odd layout, foreign prompt, unpaired PRE, HUD gone mid-frame ----
  fam = 8;
  {
    Glue gl;
    Prompt m;
    m.exists = true, m.key = 5, m.vptr_ok = false;
    gl.step(m);
    ck(m.root_visible && gl.ui.c.refused == 1 && gl.hide_writes == 0, "vptr mismatch: FAIL_CLOSED, no write");
    Render r;
    Hud h;
    m.slot[0] = 0, m.sparkle[0] = 10;
    gl.draw(m, h, r);
    ck(gl.slot_writes == 0 && gl.ui.c.mask_refused == 1, "no sparkle mask on a refused layout");
    Prompt a;
    a.exists = true, a.key = 6, a.screen_ok = false;
    const auto ref0 = gl.ui.c.refused;
    gl.step(a);
    ck(a.root_visible && gl.ui.c.refused == ref0 + 1, "missing screen refused");
    Prompt b;
    b.exists = true, b.key = 7, b.aligned = false;
    gl.step(b);
    ck(b.root_visible && gl.ui.c.refused == ref0 + 2, "misaligned refused");
    gl.step(b, false);
    ck(gl.ui.c.no_meter == 1, "no meter: nothing");
    Prompt skip;  // the cutscene-skip prompt (another dMeterButton_c): never masked, never hidden
    skip.exists = true, skip.key = 99, skip.slot[0] = 0, skip.sparkle[0] = 5;
    Render rs;
    gl.draw(skip, h, rs, false);
    ck(skip.root_visible && rs.icons == 1 && rs.sparkles == 1 && gl.ui.c.mask_foreign == 1, "foreign prompt untouched");
    lnk01::SparkleMask mk;
    lnk01::UiCounters uc;
    auto b1 = mk.begin(true, true, 1, 0, 1, uc);
    ck(b1.apply && b1.s0 == lnk01::kButtonNone && b1.s1 == lnk01::kButtonNone, "mask writes NONE");
    auto b2 = mk.begin(true, true, 2, 3, 4, uc);  // PRE without POST (abnormal)
    ck(uc.mask_unpaired == 1 && b2.apply, "unpaired PRE counted");
    auto e1 = mk.end(1, uc);
    ck(!e1.restore, "never restores a different object blindly");
    auto e2 = mk.end(2, uc);
    ck(e2.restore && e2.s0 == 3 && e2.s1 == 4, "restores the exact original slots");
    ck(!mk.end(2, uc).restore, "no double restore");
    lnk01::ActionUi off;
    ck(!off.step(Glue().read(m)).hide && off.c.checks == 0, "disabled: inert");
  }

  // ---- L09 genuine blocks: E_OC and E_OC2 contact the shield (+20 each), no contact (+0), unblocked hit from behind (+0) ----
  fam = 9;
  {
    SimParams sp;
    sp.ticks = 40;
    const SimOut o = run(sp);
    ck(o.native_blocks == 1 && verdicts(o, lnk01::Verdict::Confirmed) == 1 && o.peak == 20000, "Blue normal guard: one block, +20");
    SimParams red = sp;
    red.foes[0].e_oc2 = true, red.foes[0].atp = 2;
    const SimOut r = run(red);
    ck(r.peak == 25000 && r.pc.block_by_variant[1] == 1, "Red (E_OC2 2/2): +25 since LNK07 (was +20 in 0.9.0), variant RED");
    SimParams blk = sp;
    blk.foes[0].e_oc2 = true, blk.foes[0].atp = 3;
    ck(run(blk).pc.block_by_variant[2] == 1 && run(blk).peak == 30000, "Black (E_OC2 atp 3): +30 since LNK07, variant BLACK");
    SimParams miss = sp;
    miss.foes[0].reach = false;
    const SimOut mo = run(miss);
    ck(mo.native_blocks == 0 && mo.peak == 0 && mo.pc.contacts == 0, "swing that never reaches Link: nothing");
    SimParams back = sp;
    back.foes[0].front = false;
    const SimOut bo = run(back);
    ck(bo.link_damage == 1 && bo.peak == 0 && verdicts(bo, lnk01::Verdict::RejectedNonBlock) + verdicts(bo, lnk01::Verdict::RejectedDirectNotAttributed) >= 1,
       "hit from behind: damage, no BLOCK posture (direct hits need the Tg attribution modelled in lnks1_lab)");
    SimParams noguard = sp;
    noguard.hold_guard = false;
    const SimOut no = run(noguard);
    ck(no.link_damage == 1 && no.peak == 0, "no guard: no block posture (LNK02 owns direct hits)");
  }

  // ---- L10 POST08 parry vs ordinary guard ----
  fam = 10;
  {
    // the swing reaches frame 15 at tick 17 (prepare 1 tick + 15 frames); a Shield Attack 4 ticks earlier lands inside the window
    SimParams sp;
    sp.ticks = 40;
    sp.sa_ticks = {13};
    const SimOut o = run(sp);
    ck(o.parries == 1, "timed Shield Attack: parry confirmed (+40 enemy in POST08)");
    ck(o.peak == 0 && verdicts(o, lnk01::Verdict::RejectedParry) >= 1 && verdicts(o, lnk01::Verdict::Confirmed) == 0, "parry gives +0 to Link");
    SimParams g = sp;
    g.sa_ticks.clear();
    const SimOut go = run(g);
    ck(go.parries == 0 && go.peak == 20000, "ordinary guard: +20 Link, 0 parry");
    SimParams early = sp;
    early.sa_ticks = {1};  // ends long before the blow: Link back in normal guard
    const SimOut eo = run(early);
    ck(eo.parries == 0 && eo.peak == 20000, "too early: ordinary block, no gift parry");
    SimParams edge = sp;
    edge.sa_ticks = {3};  // still in Shield Attack, outside the window: MISSED_EARLY = native block, not a parry
    const SimOut ed = run(edge);
    ck(ed.parries == 0 && ed.peak == 20000 && ed.parry_c.result[static_cast<unsigned>(Result::MissedEarly)] == 1, "late window edge: MISSED_EARLY counted once as a block");
    // window sweep: every press either parries (+0 Link) or blocks (+20 Link), never both, never twice
    int sweep_parries = 0, sweep_blocks = 0;
    for (int s = 1; s <= 18; ++s) {
      SimParams w = sp;
      w.sa_ticks = {s};
      const SimOut x = run(w);
      sweep_parries += x.parries, sweep_blocks += verdicts(x, lnk01::Verdict::Confirmed);
      ck((x.parries == 1 && x.peak == 0) || (x.parries == 0 && x.peak == 20000), "parry XOR block for one blow");
      ck(verdicts(x, lnk01::Verdict::Confirmed) + x.parries == 1, "exactly one outcome per blow");
    }
    std::printf("L10 sweep parries=%d blocks=%d\n", sweep_parries, sweep_blocks);
    ck(sweep_parries >= 5 && sweep_blocks >= 5, "the sweep crosses the window (both outcomes present)");
  }

  // ---- L11 duplicates: two spheres, repeated callbacks, stale records, rebound passes, parry then guard ----
  fam = 11;
  {
    SimParams sp;
    sp.ticks = 40;
    sp.both_spheres = true;
    const SimOut o = run(sp);
    ck(o.peak == 20000 && verdicts(o, lnk01::Verdict::Confirmed) == 1, "two spheres of one blade: one block");
    SimParams cb = sp;
    cb.double_callback = true;
    const SimOut c2 = run(cb);
    ck(c2.peak == 20000 && verdicts(c2, lnk01::Verdict::Duplicate) >= 1, "repeated callback: DUPLICATE, +20 once");
    // a stale reader (no FIX02 gate) would see old records again; the real gate turns them into STALE (never credited)
    SimParams long_guard;
    long_guard.ticks = 400;
    const SimOut lg = run(long_guard);
    const int blocks = lg.native_blocks;
    ck(blocks >= 4 && verdicts(lg, lnk01::Verdict::Confirmed) == blocks, "one credit per native block over a long guard");
    ck(lg.pc.contacts >= static_cast<std::uint64_t>(blocks), "rebound passes produce extra contacts");
    ck(verdicts(lg, lnk01::Verdict::Confirmed) + verdicts(lg, lnk01::Verdict::RejectedNonBlock) + verdicts(lg, lnk01::Verdict::Duplicate) +
               verdicts(lg, lnk01::Verdict::RejectedStale) + verdicts(lg, lnk01::Verdict::RejectedDirectNotAttributed) + verdicts(lg, lnk01::Verdict::DirectPending) ==
           static_cast<int>(lg.pc.contacts),
       "every contact has exactly one verdict");
    // direct core checks: stale record, parry then guard in the same attack, guard then a parry in a NEW attack
    lnk01::Posture p;
    p.observe_link(true, 7);
    lnk01::Contact k;
    k.result = Result::StaleContact, k.blocked = true, k.token = 5, k.slot = 0, k.id = 100, k.generation = 1, k.link_ok = true, k.link_human = true, k.link_id = 7;
    ck(p.on_contact(k).v == lnk01::Verdict::RejectedStale && p.milli() == 0, "stale At record: nothing");
    k.result = Result::Confirmed;
    ck(p.on_contact(k).v == lnk01::Verdict::RejectedParry, "parry: rejected for Link");
    k.result = Result::NormalGuard;
    ck(p.on_contact(k).v == lnk01::Verdict::RejectedParry && p.milli() == 0, "guard after a parry in the same attack: no +20");
    k.token = 6;
    ck(p.on_contact(k).v == lnk01::Verdict::Confirmed && p.milli() == 20000, "next attack: ordinary block");
    k.result = Result::MissedLate;
    ck(p.on_contact(k).v == lnk01::Verdict::Duplicate && p.milli() == 20000, "late shield attack in a resolved attack: duplicate");
    k.token = 7, k.result = Result::NormalGuard, k.episode_parried = true;
    ck(p.on_contact(k).v == lnk01::Verdict::RejectedParry, "episode already deflected (POST08 commit): +0");
  }

  // ---- L12 phase skew: Link before/after the Bokoblin in the tick; GUARD_ATTACK -> GUARD_SLIP between pass and read ----
  fam = 12;
  for (bool first : {true, false}) {
    SimParams sp;
    sp.ticks = 40;
    sp.link_first = first;
    const SimOut g = run(sp);
    ck(g.peak == 20000, "guard: +20 in both execution orders");
    sp.sa_ticks = {13};
    const SimOut p = run(sp);
    ck(p.parries == 1 && p.peak == 0, "parry assigned to the impact pass in both orders, no extra +20");
    ck(p.credited_ticks.empty(), "no Link credit at all for a parried blow");
  }

  // ---- L13 several attackers: isolation, A/B alternation, recycled identities, 2/4/8 actors ----
  fam = 13;
  {
    SimParams sp;
    sp.ticks = 40;
    Foe a, b;
    a.id = 100, b.id = 101, b.reach = false, b.first_delay = 3;
    sp.foes = {a, b};
    const SimOut o = run(sp);
    ck(o.peak == 20000 && verdicts(o, lnk01::Verdict::Confirmed) == 1, "A blocked, B whiffs: only A");
    for (int n : {2, 4, 8}) {
      SimParams m;
      m.ticks = 120;
      m.foes.clear();
      for (int i = 0; i < n; ++i) {
        Foe f;
        f.id = 200 + static_cast<std::uint32_t>(i), f.first_delay = i * 5, f.e_oc2 = (i % 3) != 0, f.atp = (i % 3) == 1 ? 2 : 3;
        m.foes.push_back(f);
      }
      const SimOut x = run(m);
      ck(verdicts(x, lnk01::Verdict::Confirmed) == x.native_blocks, "one credit per native block, every attacker isolated");
      ck(x.final_milli <= lnk01::policy::kMaxMilli && x.final_milli >= 0, "bounded");
    }
    lnk01::Posture p;
    p.observe_link(true, 7);
    lnk01::Contact k;
    k.result = Result::NormalGuard, k.blocked = true, k.token = 9, k.slot = 3, k.id = 300, k.generation = 1, k.link_ok = true, k.link_human = true, k.link_id = 7;
    ck(p.on_contact(k).v == lnk01::Verdict::Confirmed, "A credited");
    lnk01::Contact kb = k;
    kb.id = 301, kb.generation = 2;  // slot recycled by another actor, token reuse impossible in POST08 but tested anyway
    ck(p.on_contact(kb).v == lnk01::Verdict::Confirmed, "recycled slot: new identity, nothing inherited");
    ck(p.on_contact(kb).v == lnk01::Verdict::Duplicate, "and deduplicated");
    k.slot = -1;
    ck(p.on_contact(k).v == lnk01::Verdict::RejectedUntracked, "untracked attacker refused");
    k.slot = 3, k.token = 0;
    ck(p.on_contact(k).v == lnk01::Verdict::RejectedUntracked, "no attack episode refused");
  }

  // ---- L14 regeneration: 30-tick delay, 10 pts/s, sim clock only, pause freeze, no catch-up, bounds ----
  fam = 14;
  {
    lnk01::Posture p;
    p.observe_link(true, 7);
    lnk01::Contact k;
    k.result = Result::NormalGuard, k.blocked = true, k.slot = 0, k.id = 100, k.generation = 1, k.link_ok = true, k.link_human = true, k.link_id = 7;
    k.token = 1;
    p.on_contact(k);
    ck(p.milli() == 20000, "+20");
    for (int t = 1; t <= 30; ++t) p.tick(true);
    ck(p.milli() == 20000, "no recovery during the 30-tick delay");
    p.tick(true);
    ck(p.milli() < 20000 && p.milli() > 19500, "recovery starts on tick 31");
    int ticks = 1;
    while (p.milli() > 0 && ticks < 1000) p.tick(true), ++ticks;
    ck(ticks == 60, "20 points in exactly 60 ticks (10 pts/s at 30 Hz)");
    ck(p.c.zeros == 1 && p.milli() == 0, "reaches 0 exactly, never negative");
    for (int i = 0; i < 100; ++i) p.tick(true);
    ck(p.milli() == 0, "stays at 0");
    // a value that is not a multiple of the per-3-tick step (block during recovery) still lands EXACTLY on 0, never below
    {
      lnk01::Posture r;
      r.observe_link(true, 7);
      k.token = 900, r.on_contact(k);
      for (int t = 0; t < 31; ++t) r.tick(true);  // one regeneration step: 19.667
      k.token = 901, r.on_contact(k);             // 39.667
      int lowest = r.milli(), steps = 0;
      while (r.milli() > 0 && steps < 5000) r.tick(true), lowest = std::min(lowest, r.milli()), ++steps;
      for (int t = 0; t < 50; ++t) r.tick(true), lowest = std::min(lowest, r.milli());
      ck(lowest == 0 && r.milli() == 0 && r.c.zeros == 1, "odd remainder: exactly 0, never negative");
      ck(r.conserved(), "conservation with an odd remainder");
    }
    // pause: nothing moves, nothing accumulates, no catch-up after a long pause
    k.token = 2;
    p.on_contact(k);
    for (int t = 0; t < 40; ++t) p.tick(true);
    const int before = p.milli();
    for (int t = 0; t < 100000; ++t) p.tick(false);
    ck(p.milli() == before, "frozen while paused / in events / menus");
    p.tick(true);
    ck(before - p.milli() <= 334 && before - p.milli() >= 333, "one step only after the pause (no dt burst)");
    // render frames never advance the counter: visual() is const, the counter only moves in tick()
    lnk01::PlayerHud hud;
    hud.tick(p.milli(), true, true);
    const int m0 = p.milli();
    for (int f = 0; f < 120; ++f) (void)hud.visual();
    ck(p.milli() == m0, "presentation frames (30/60/144 Hz) do not regenerate");
    // saturation
    lnk01::Posture s;
    s.observe_link(true, 7);
    for (int i = 1; i <= 7; ++i) k.token = 10 + static_cast<std::uint64_t>(i), s.on_contact(k);
    ck(s.milli() == lnk01::policy::kMaxMilli && s.c.saturations == 1, "max 100, one saturation");
    ck(s.conserved() && p.conserved(), "conservation: gained = regenerated + current + lost on reset");
    // a block during recovery restarts the delay
    lnk01::Posture d;
    d.observe_link(true, 7);
    k.token = 50, d.on_contact(k);
    for (int t = 0; t < 45; ++t) d.tick(true);
    const int mid = d.milli();
    k.token = 51, d.on_contact(k);
    for (int t = 0; t < 30; ++t) d.tick(true);
    ck(d.milli() == mid + 20000, "a new block restarts the delay");
    // model run: pause window in the middle freezes the trace
    SimParams sp;
    sp.ticks = 200;
    sp.pause_from = 60, sp.pause_to = 140;
    const SimOut o = run(sp);
    ck(o.posture_trace[70] == o.posture_trace[130], "model: paused ticks keep the value");
  }

  // ---- L15 native guard break: every 4th block breaks natively; LNK01 never reads or writes the native counter, adds no break of its own ----
  fam = 15;
  {
    SimParams sp;
    sp.ticks = 900;
    const SimOut with = run(sp, true), without = run(sp, false);
    ck(with.native_blocks >= 8, "many blocks");
    ck(with.guard_breaks == with.native_blocks / 4, "native break on every 4th block");
    ck(with.guard_breaks == without.guard_breaks && with.native_blocks == without.native_blocks && with.link_damage == without.link_damage,
       "identical native guard behaviour with and without LNK01");
    ck(with.parries == without.parries && with.parry_c.confirmed == without.parry_c.confirmed, "POST08 unchanged by LNK01");
    bool reached100 = false;
    for (int v : with.posture_trace) reached100 |= v >= lnk01::policy::kMaxMilli;
    SimParams fast = sp;
    fast.ticks = 200;
    Foe a, b, c;
    a.id = 1, b.id = 2, b.first_delay = 4, c.id = 3, c.first_delay = 8;
    fast.foes = {a, b, c};
    const SimOut f = run(fast);
    for (int v : f.posture_trace) reached100 |= v >= lnk01::policy::kMaxMilli;
    ck(f.guard_breaks == run(fast, false).guard_breaks, "group pressure: same native breaks; 100 is visual only");
    (void)reached100;
  }

  // ---- L16 Link state: wolf, events, Link replaced, Link absent ----
  fam = 16;
  {
    SimParams sp;
    sp.ticks = 40;
    sp.wolf = true;
    const SimOut w = run(sp);
    ck(w.peak == 0 && verdicts(w, lnk01::Verdict::Confirmed) == 0, "wolf / not human: no block posture");
    SimParams ev;
    ev.ticks = 60;
    ev.event_from = 1, ev.event_to = 60;
    const SimOut e = run(ev);
    ck(e.peak == 0, "event / cutscene: nothing");
    SimParams ch;
    ch.ticks = 120;
    ch.link_change_at = 40;
    const SimOut c = run(ch);
    ck(c.posture_trace[38] > 0 && c.posture_trace[40] == 0, "Link replaced (room change, death/continue): deterministic reset to 0");
    ck(c.pc.link_changes == 1, "change counted");
    lnk01::Posture p;
    p.observe_link(true, 7);
    lnk01::Contact k;
    k.result = Result::NormalGuard, k.blocked = true, k.token = 1, k.slot = 0, k.id = 9, k.generation = 1, k.link_ok = true, k.link_human = true, k.link_id = 7;
    p.on_contact(k);
    p.observe_link(false, 0);
    ck(p.milli() == 0 && !p.bound(), "Link absent: reset");
    k.token = 2;
    ck(p.on_contact(k).v == lnk01::Verdict::RejectedLink, "no bound Link: refused");
    p.observe_link(true, 7);
    k.link_id = 8;
    ck(p.on_contact(k).v == lnk01::Verdict::RejectedLink, "contact for another Link id: refused");
    k.link_id = 7, k.blocked = false;
    ck(p.on_contact(k).v == lnk01::Verdict::RejectedNonBlock, "no native shield flag: refused");
    lnk01::PlayerHud h;
    h.tick(40000, true, true);
    ck(h.drawable(), "human: shown");
    h.tick(40000, true, false), h.tick(40000, true, false), h.tick(40000, true, false);
    ck(!h.drawable(), "wolf: hidden");
    lnk01::PlayerHud h2;
    h2.tick(40000, false, true);
    ck(!h2.drawable() && h2.c.paused == 1, "HUD not allowed (menu/pause/dialogue): nothing animates or shows");
  }

  // ---- L17 regression guards on inherited cores (full inherited labs run separately in CI) ----
  fam = 17;
  {
    ck(std::strcmp(post08::kParryVersion, "0.8.2") == 0, "POST08 core unchanged (0.8.2)");
    ck(std::strcmp(post07::kHudVersion, "0.7.0") == 0, "POST07 core unchanged (0.7.0)");
    ck(post08::policy::kWindowFrames == 9.0f && post08::policy::kWindowTicksCap == 11, "parry window unchanged");
    SimParams sp;
    sp.ticks = 400;
    sp.sa_ticks = {13, 63, 113, 163};
    const SimOut a = run(sp, true), b = run(sp, false);
    ck(a.parries == b.parries && a.parry_c.contacts == b.parry_c.contacts && a.parry_c.stale == b.parry_c.stale, "POST08 verdict stream identical with LNK01 attached");
  }

  // ---- L18 negatives / constants ----
  fam = 18;
  {
    ck(lnk01::kButtonNone == 22, "BUTTON_NONE");
    ck(lnk01::policy::kMaxPoints == 100 && lnk01::policy::kWeights[0].block == 20 && lnk01::policy::kWeights[1].block == 25 && lnk01::policy::kWeights[2].block == 30,
       "LNK07 block weights Blue/Red/Black 20/25/30, max 100");
    ck(lnk01::policy::kParryCostPoints == 0, "parry costs Link nothing in LNK01");
    ck(lnk01::policy::kRegenDelayTicks == 30 && lnk01::policy::kRegenPointsPerSecond == 10 && lnk01::policy::kTicksPerSecond == 30, "regen policy");
    ck(std::strcmp(lnk01::kVersion, "0.10.1") == 0 && std::strcmp(lnk01::kLnk01Version, "0.9.0") == 0, "version 0.10.0 (LNK01 HUD/UI 0.9.0)");
    ck(lnk01::name(lnk01::Verdict::Confirmed) == std::string("CONFIRMED_NATIVE_BLOCK") && lnk01::name(lnk01::UiEvent::VisualHidden) == std::string("VISUAL_HIDDEN"),
       "log vocabulary");
    char line[1024];
    lnk01::Contact k;
    k.result = Result::NormalGuard, k.id = 4294967295u, k.generation = 18446744073709551615ull, k.token = 18446744073709551615ull, k.tick = 18446744073709551615ull;
    lnk01::Outcome o;
    o.v = lnk01::Verdict::RejectedUntracked, o.before = 100000, o.after = 100000;
    const int n = lnk01::fmt_block(line, sizeof line, k, o, 31, true);
    ck(n > 0 && n < 512, "worst-case LNK01_BLOCK line fits the log limit");
    ck(std::strstr(line, "0x") == nullptr, "no address-like hex in the line");
  }

  // ---- L19 performance: fixed memory, O(1) per contact, bounded quads ----
  fam = 19;
  {
    ck(sizeof(lnk01::Posture) < 16384, "posture state < 16 KB (static: 64-slot ledger with one pending direct hit each + 64-slot variant history)");
    ck(sizeof(lnk01::ActionUi) < 512 && sizeof(lnk01::PlayerHud) < 256, "UI state tiny");
    unsigned worst = 0;
    for (float f = 0; f <= 1.0f; f += 0.05f)
      for (float pl : {0.0f, 0.5f, 1.0f})
        for (float hi : {0.0f, 1.0f}) {
          Quad q[lnk01::kMaxQuads + 8];
          post07::QuadSink sink(q, lnk01::kMaxQuads);
          const unsigned n = lnk01::build_bar(vis(f, 1, pl, hi, 1 - hi, f >= 1.0f), L1080, q, lnk01::kMaxQuads);
          worst = std::max(worst, n);
        }
    ck(worst < lnk01::kMaxQuads, "worst bar fits the quad budget without drops");
    std::printf("L19 worst_quads=%u\n", worst);
    lnk01::Posture p;
    p.observe_link(true, 7);
    lnk01::Contact k;
    k.result = Result::NormalGuard, k.blocked = true, k.slot = 0, k.id = 9, k.generation = 1, k.link_ok = true, k.link_human = true, k.link_id = 7;
    for (std::uint64_t i = 1; i <= 200000; ++i) k.token = i, k.slot = static_cast<int>(i % 64), p.on_contact(k), p.tick(true);
    ck(p.c.contacts == 200000 && p.conserved(), "200k contacts handled, conservation holds");
  }

  for (int i = 1; i <= 19; ++i) std::printf("L%02d checks=%u failures=%u\n", i, fam_checks[i], fam_fail[i]);
  std::printf("LNK01_LAB checks=%u failures=%u RESULT=%s LAB_MODEL_ONLY\n", checks, failures, failures ? "LAB_FAIL" : "LAB_PASS");
  return failures ? 1 : 0;
}
