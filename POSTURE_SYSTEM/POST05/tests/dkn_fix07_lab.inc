// LIGHT FADES TWICE 0.13.16 DKN01 FIX07: the laboratory section of the FIX07 deltas (included by tests/dkn_lab.cpp, run by its main()). Every check runs the
// production core (src/dkn_core.hpp): the F1-C3 planner, the phase-1 execution total, the offensive leap (GapCloser, leap_pick, leap_ground), the reviewed
// reposition verdict, the trajectory verdict, the thrown-mace verdict and ledger, and that nothing else of the 14 plans moved.

// FNV-1a over every field of a plan (bytes of the values, never padding): computed with the FIX06 0.13.15 core (803a5b1) by
// dkn_fix07_evidence/tools fp.cpp; FIX07 must give the same for every plan but F1-C3.
static std::uint64_t f7_fnv(std::uint64_t h, const void* p, std::size_t n) {
  const unsigned char* b = static_cast<const unsigned char*>(p);
  for (std::size_t i = 0; i < n; ++i) h = (h ^ b[i]) * 1099511628211ull;
  return h;
}
static std::uint64_t f7_plan_fnv(const Plan& p) {
  std::uint64_t h = 1469598103934665603ull;
  h = f7_fnv(h, &p.n, 1), h = f7_fnv(h, &p.scale, 4);
  for (unsigned i = 0; i < 7; ++i) {
    const Seg& s = p.seg[i];
    const unsigned char m = static_cast<unsigned char>(s.m);
    h = f7_fnv(h, &m, 1), h = f7_fnv(h, &s.start, 4), h = f7_fnv(h, &s.cut, 4), h = f7_fnv(h, &s.rate_in, 4), h = f7_fnv(h, &s.rate_mid, 4), h = f7_fnv(h, &s.rate_out, 4),
    h = f7_fnv(h, &s.morph, 4);
  }
  for (unsigned i = 0; i < 8; ++i) {
    const unsigned char c = p.clamped[i];
    h = f7_fnv(h, &p.target_ticks[i], 4), h = f7_fnv(h, &c, 1);
  }
  return h;
}
static const std::uint64_t kFix06PlanFnv[kPlans] = {
    0x9bf684cf81b08cceull, 0xe3f50b5d2b0787eeull, 0x80500b9bf1b30989ull, 0xb40b79c5fe9e101full, 0x0769370a8b9c8150ull, 0x52fccf85888c08e6ull, 0xf9f17ccf8110b565ull,
    0x1d850261938cf742ull, 0x153323e30e4c7422ull, 0x8d64e582cdaa9525ull, 0x60f533bbd9338e0cull, 0x669bf1100be725edull, 0xe57ebdf456850ad6ull, 0x3b428dbef5753c7full};

// FIX08: the FIX07 F1-C3 plan is history (its row and targets were replaced by the approved FIX08 36 / 56 / 57 / 40): rebuilt here from its own row, targets
// and pair-1 exception with the very plan_f1 arithmetic, so every FIX07 check below still pins what FIX07 delivered.
static Plan plan_f1c3_fix07() {
  const Combo& c = kCombos[kF1C3];
  const F1Plan f{256, 0, {{57, 22, 10}, {64, 8, 6}, {64, 7, 6}, {56, 12, 10}}};
  const float T7[4] = {24.0f, 56.0f, 57.0f, 40.0f};
  Plan p = plan_f1(c, f, 99u);
  for (unsigned i = 0; i + 1 < c.n_moves; ++i) {
    const MoveInfo& a = info(c.moves[i]);
    const MoveInfo& b = info(c.moves[i + 1]);
    const float wind = b.impact[0] - f.pair[i].start, rmax = i == 0 ? 1.76f : kF1RecoveryRateMax;
    float r = (f.pair[i].cut - a.impact[0]) / (T7[i] - wind);
    p.clamped[i] = !(T7[i] > wind) || r < kF1RecoveryRateMin || r > rmax;
    p.seg[i].rate_out = clampf(r, kF1RecoveryRateMin, rmax), p.target_ticks[i] = T7[i];
  }
  return p;
}
static void fix07_section() {
  std::printf("\n==== FIX07 (0.13.16) ====\n");
  CHECK(std::string_view(kVersion) == "0.13.17" && std::string_view(kTag) == "DKN01_FIX08", "FIX08 version / tag (FIX08 carries every FIX07 delta below; F1-C3 pinned on its FIX07 row)");
  // ---------------------------------------------------------------- A. F1-C3 only: 24 / 56 / 57 / 40, the other 13 plans bit for bit FIX06
  {
    for (unsigned i = 0; i < kPlans; ++i) {
      const std::uint64_t h = f7_plan_fnv(plan(combo_of(i), i));
      if (i == kF1C3) CHECK(f7_plan_fnv(plan_f1c3_fix07()) != kFix06PlanFnv[i] && h != kFix06PlanFnv[i], "F1-C3 changed (FIX07, again FIX08)");
      else CHECK(h == kFix06PlanFnv[i], "%s plan is FIX06 bit for bit (got %016llx)", combo_of(i).name, static_cast<unsigned long long>(h));
    }
    CHECK(f7_plan_fnv(plan_f1c3_fix06()) == kFix06PlanFnv[kF1C3], "the FIX06 F1-C3 plan is rebuilt from its own row: only the row + the targets changed");
    const Combo& c = kCombos[kF1C3];
    CHECK(c.n_moves == 5 && c.n_impacts == 5 && c.moves[0] == Move::F1A && c.moves[1] == Move::F1B && c.moves[2] == Move::F1B && c.moves[3] == Move::F1B && c.moves[4] == Move::F1A,
          "F1-C3 = LATERAL VERTICAL VERTICAL VERTICAL LATERAL (five impacts, order and identity kept)");
    const Plan p = plan_f1c3_fix07();
    const SimOut o = simulate(c, p);
    CHECK(o.ok && o.n == 5, "F1-C3 simulates its five impacts");
    const double want[4] = {24, 56, 57, 40}, old[4] = {36.10, 86.02, 87.55, 61.82};
    double total = 0, ptotal = 0;
    for (unsigned j = 0; j < 4; ++j) {
      const double sim = o.tick[j + 1] - o.tick[j];
      total += sim, ptotal += p.target_ticks[j];
      CHECK(std::fabs(p.target_ticks[j] - want[j]) < 1e-5 && !p.clamped[j], "F1-C3 pair %u planned %.3f == approved %.0f (no clamp)", j + 1, p.target_ticks[j], want[j]);
      CHECK(std::fabs(sim - want[j]) <= 1.0 + 1e-6, "F1-C3 pair %u simulated %.0f ticks vs approved %.0f (quantization <= 1)", j + 1, sim, want[j]);
      CHECK(sim < old[j] * 0.70 && sim > old[j] * 0.60, "F1-C3 pair %u ~35 %% shorter than FIX06 (%.0f vs %.2f)", j + 1, sim, old[j]);
      std::printf("F1-C3 pair %u-%u: FIX06 %.2f t (%.2f s) -> FIX07 planned %.2f t sim %.0f t (%.3f s)\n", j + 1, j + 2, old[j], old[j] / 30, p.target_ticks[j], sim, sim / 30);
    }
    CHECK(std::fabs(ptotal - 177.0) < 1e-4 && std::fabs(total - 177.0) <= 2.0, "F1-C3 total 177 ticks = 5.90 s (planned %.2f, simulated %.0f)", ptotal, total);
    CHECK(o.tick[2] - o.tick[1] > o.tick[1] - o.tick[0] && o.tick[3] - o.tick[2] > o.tick[4] - o.tick[3] && o.tick[4] - o.tick[3] > o.tick[1] - o.tick[0],
          "F1-C3 keeps its contrast: a quick first pair, two long pauses, a medium close (not uniform)");
    CHECK(p.seg[0].start == 0.0f && p.seg[0].rate_in == 1.0f, "F1-C3 first impact untouched (start 0, native wind-up)");
    CHECK(p.seg[4].cut < 0.0f, "F1-C3 last move ends natively");
    for (unsigned m = 0; m < 5; ++m) CHECK(p.seg[m].rate_in == 1.0f, "F1-C3 seg %u wind-up + swing at the native rate", m);
    CHECK(p.seg[0].cut == 57.0f && p.seg[1].start == 22.0f && std::fabs(p.seg[0].rate_out - 1.75f) < 1e-4, "F1-C3 pair 1: cut 57 / start 22 / recovery 1.75 (the rig exception)");
    CHECK(p.seg[1].cut == 64.0f && p.seg[2].start == 8.0f && p.seg[2].cut == 64.0f && p.seg[3].start == 7.0f && p.seg[3].cut == 56.0f && p.seg[4].start == 12.0f,
          "F1-C3 pairs 2-4: 64/8, 64/7, 56/12 (rig: pose gaps 17.2 / 19.0 / 73.5)");
    for (unsigned m = 1; m < 4; ++m) CHECK(std::fabs(p.seg[m].rate_out - 1.0f) < 1e-4, "F1-C3 pairs 2-4 recover at the native rate (seg %u %.3f)", m, p.seg[m].rate_out);
    CHECK(p.seg[1].morph == 10.0f && p.seg[4].morph == 10.0f && p.seg[2].morph == 6.0f && p.seg[3].morph == 6.0f, "F1-C3 blends: 10 frames over gaps > 60, else 6");
    const OppOut op = opportunities(c, p);
    CHECK(op.n == 5, "F1-C3 five impact windows");
    for (unsigned i = 0; i < op.n; ++i)
      CHECK(op.imp[i].open_ticks >= 5 && op.imp[i].impact_inside && op.imp[i].separated, "F1-C3 impact %u: AT %u ticks, impact inside, own ticket (parry / block / direct each)", i + 1,
            op.imp[i].open_ticks);
    // every other F1 combo keeps its FIX06 rhythm (audio x S), the F2 ones their FIX06 targets
    for (unsigned ci = 0; ci < 8; ++ci) {
      if (ci == kF1C3) continue;
      const Plan q = plan(kCombos[ci], ci);
      for (unsigned j = 0; j + 1 < kCombos[ci].n_impacts; ++j) {
        const double ref = iv_target(ci, j) > 0.0f ? iv_target(ci, j) : (kCombos[ci].peak_ms[j + 1] - kCombos[ci].peak_ms[j]) * 0.03 * q.scale;
        CHECK(std::fabs(q.target_ticks[j] - ref) < 1e-3, "%s pair %u unchanged by FIX07", kCombos[ci].name, j + 1);
      }
    }
  }
  // ---------------------------------------------------------------- B. F1 Mortal Draw / Helm Splitter: 120 TOTAL, saturated, the armour by progress
  {
    CHECK(policy::kF1ExecDamage == 120 && policy::kHp1Max == 600, "F1 executions 120 total of 600 (20 %%)");
    const int cases[][2] = {{600, 480}, {480, 360}, {240, 120}, {121, 1}, {120, 0}, {119, 0}, {40, 0}, {1, 0}};
    for (const auto& h : cases) CHECK(f1_exec_hp(h[0]) == h[1], "F1 execution %d -> %d (got %d)", h[0], h[1], f1_exec_hp(h[0]));
    CHECK(600 - f1_exec_hp(600) == 120 && 40 - f1_exec_hp(40) == 40, "effective damage: 120, or what was left (40 -> 40, never 120 counted)");
    CHECK(armor_progress(600) == 0 && armor_progress(480) == 2 && armor_call(2, 0) == 2, "600 -> 480: two 50-HP thresholds, one native call breaks both");
    unsigned max_cross = 0;
    for (int b = 600; b > 0; --b) {
      const int a = f1_exec_hp(b), pb = armor_progress(b), pa = armor_progress(a);
      const unsigned cross = static_cast<unsigned>(pa - pb);
      if (a > 0 && cross > max_cross) max_cross = cross;
      CHECK(pa >= pb && (a == 0 ? pa == 12 : pa <= 11 && cross >= 2 && cross <= 3), "armour %d -> %d crosses %u (2 or 3: 120 / 50)", b, a, cross);
    }
    CHECK(max_cross == 3, "a 120 contact may cross three thresholds");
    CHECK(exec_verdict_f1(false, Bk::StaggerA, true, false) && exec_verdict_f1(false, Bk::StaggerB, false, true) && !exec_verdict_f1(true, Bk::StaggerA, true, false),
          "phase-1 routes unchanged; phases 2-3 keep their fall (no 120 there)");
  }
  // ---------------------------------------------------------------- C. native leap physics and the offensive leap bands
  {
    const Jump b = leap_model(Leap::Back), f = leap_model(Leap::Fwd), l = leap_model(Leap::FwdLong);
    CHECK(b.dist == jump_model(true).dist && f.dist == jump_model(false).dist, "FIX06 jump model kept for back / forward");
    CHECK(b.dist == 450.0f && f.dist == 245.0f && l.dist == 405.0f && f.air == 7 && l.air == 9, "native leaps: back 450, forward (8) 245, long forward (17) 405");
    CHECK(leap_mode(Leap::Fwd) == 8 && leap_mode(Leap::FwdLong) == 17 && leap_mode(Leap::Back) == 6 && leap_mode(Leap::None) == -1, "native ChaseL modes");
    CHECK(native::jump_mode(17) && native::native_leap(native::kActChaseL, 17), "mode 17 is a jump the reposition / leap lease follows and an unowned native leap");
    for (float d = 300.0f; d < 900.0f; d += 0.5f) {
      const Leap p = leap_pick(d);
      if (p == Leap::None) {
        CHECK(d - f.dist < policy::kLeapLandMin || d - l.dist > policy::kEngage2 || (d - f.dist > policy::kEngage2 && d - l.dist < policy::kLeapLandMin),
              "no leap at %.1f only when neither lands inside [200, 370]", d);
        continue;
      }
      const float land = d - leap_model(p).dist;
      CHECK(land >= policy::kLeapLandMin && land <= policy::kEngage2, "leap at %.1f lands at %.1f inside [200, 370] (never on Link)", d, land);
    }
    CHECK(leap_pick(370.0f) == Leap::None && leap_pick(444.0f) == Leap::None && leap_pick(445.0f) == Leap::Fwd && leap_pick(610.0f) == Leap::Fwd && leap_pick(611.0f) == Leap::FwdLong &&
              leap_pick(775.0f) == Leap::FwdLong && leap_pick(776.0f) == Leap::None,
          "bands: 445..610 forward (8), 611..775 long forward (17) (both valid 605..615: the landing nearer 285); 370..445 and > 775 no leap (normal approach)");
  }
  // ---------------------------------------------------------------- D. the ONE roll per opportunity: 90 / 10, 25 x 4, debounce, episode + cooldown, no reroll
  {
    GapView v;
    v.phase2 = true, v.dist = 520, v.dy = 0;
    std::uint64_t leaps = 0, normals = 0, pick[4] = {}, rolls = 0, wait_ticks = 0;
    for (unsigned run = 0; run < 100000; ++run) {
      GapCloser g;
      g.rng.seed(run * 2654435761ull + 7);
      for (unsigned t = 0; t < 10; ++t) {
        v.now = 1000 + t;
        const GapOut o = g.decide(v);
        if (o.g == Gc::Wait) ++wait_ticks;
        if (o.g != Gc::Roll) continue;
        ++rolls;
        CHECK(t == policy::kLeapDebounceTicks - 1, "the roll waits the debounce (%u ticks)", t + 1);
        if (o.pick == GcPick::LeapCombo) {
          ++leaps, ++pick[o.combo - 4];
          CHECK(o.leap == Leap::Fwd && o.combo >= 4 && o.combo <= 7, "a leap of the band with one of the four F2/F3 combos (never a single / counter / kick)");
        } else {
          ++normals;
          CHECK(o.pick == GcPick::Normal && o.leap == Leap::None && o.combo < 0, "10 %%: the normal approach, nothing scheduled");
        }
        // no reroll while the episode stays open (Link still far): 400 more ticks of opportunities
        for (unsigned u = 1; u < 400; ++u) {
          v.now = 1000 + t + u;
          g.observe(v.now, v.dist);
          CHECK(g.decide(v).g != Gc::Roll || u >= policy::kLeapEpisodeTicks, "no reroll before the episode closes (tick %u)", u);
          if (u >= policy::kLeapEpisodeTicks) break;
        }
        break;
      }
    }
    const double lp = 100.0 * leaps / rolls;
    std::printf("LEAP_ROLL 100000 opportunities: leap+combo %.2f %% normal %.2f %% picks %.2f/%.2f/%.2f/%.2f %%\n", lp, 100.0 * normals / rolls, 100.0 * pick[0] / leaps, 100.0 * pick[1] / leaps,
                100.0 * pick[2] / leaps, 100.0 * pick[3] / leaps);
    CHECK(rolls == 100000 && lp > 89.5 && lp < 90.5, "90 / 10 per opportunity (%.2f %%)", lp);
    for (unsigned k = 0; k < 4; ++k) CHECK(pick[k] * 100.0 / leaps > 24.4 && pick[k] * 100.0 / leaps < 25.6, "combo %u picked 25 %% (%.2f)", k, pick[k] * 100.0 / leaps);
    // eligibility: phase 1, busy, no Link, unsafe, not in front, height, too close / too far, disarmed: never a roll
    GapCloser g;
    g.rng.seed(5);
    GapView w = v;
    auto rolls_in = [&](GapView x) {
      GapCloser h;
      h.rng.seed(9);
      for (unsigned t = 0; t < 20; ++t) {
        x.now = 5000 + t;
        if (h.decide(x).g == Gc::Roll) return true;
      }
      return false;
    };
    CHECK(rolls_in(w), "baseline eligible");
    w = v, w.phase2 = false;
    CHECK(!rolls_in(w), "phase 1: no leap (no native phase-1 leap)");
    w = v, w.free = false;
    CHECK(!rolls_in(w), "busy (lease / sequence / defense / re-entry / reposition): no roll");
    w = v, w.link = false;
    CHECK(!rolls_in(w), "no Link / Link guard-broken: no roll");
    w = v, w.safe = false;
    CHECK(!rolls_in(w), "event / not on the ground: no roll");
    w = v, w.front = false;
    CHECK(!rolls_in(w), "Link not in front: no roll (the native chase turns first)");
    w = v, w.dy = 121;
    CHECK(!rolls_in(w), "height difference > 120: no roll");
    w = v, w.dist = 440;
    CHECK(!rolls_in(w), "370..445: no leap would land without landing on Link: normal approach");
    w = v, w.dist = 900;
    CHECK(!rolls_in(w), "> 775: approach first");
    // a flicker of eligibility (1 tick in, 1 out) never rolls; the debounce restarts
    GapCloser fl;
    fl.rng.seed(3);
    bool flick = false;
    for (unsigned t = 0; t < 200; ++t) {
      GapView x = v;
      x.now = 100 + t, x.dist = t % 2 ? 520.0f : 300.0f;
      flick = flick || fl.decide(x).g == Gc::Roll;
    }
    CHECK(!flick, "an opportunity flickering at the threshold never rolls (debounce)");
    // the episode closes when Link comes close; the next roll needs the cooldown too; the time bound closes it without contact
    GapCloser e;
    e.rng.seed(11);
    GapView x = v;
    std::uint64_t first = 0;
    for (unsigned t = 0; t < 20 && !first; ++t) x.now = 200 + t, first = e.decide(x).g == Gc::Roll ? x.now : 0;
    CHECK(first && !e.armed, "rolled once, disarmed");
    e.observe(first + 10, 300);  // Link came within reach: closed, cooldown still running
    CHECK(e.closed && !e.armed, "closed by contact, cooldown pending");
    e.observe(first + policy::kLeapCooldownTicks - 1, 600);
    CHECK(!e.armed, "not before the cooldown");
    e.observe(first + policy::kLeapCooldownTicks, 600);
    CHECK(e.armed && e.rearmed == 1, "re-armed after contact + cooldown");
    GapCloser t2;
    t2.rng.seed(13);
    x.now = 0;
    std::uint64_t r2 = 0;
    for (unsigned t = 0; t < 20 && !r2; ++t) x.now = 300 + t, r2 = t2.decide(x).g == Gc::Roll ? x.now : 0;
    t2.observe(r2 + policy::kLeapEpisodeTicks - 1, 600);
    CHECK(!t2.armed, "an episode without contact stays open until its time bound");
    t2.observe(r2 + policy::kLeapEpisodeTicks, 600);
    CHECK(t2.armed, "the time bound closes it (cooldown already over)");
    // determinism: same seed, same decisions; the stream is the actor's own (no other RNG moves)
    GapCloser d1, d2;
    d1.rng.seed(77), d2.rng.seed(77);
    bool same = true;
    for (unsigned t = 0; t < 2000; ++t) {
      GapView y = v;
      y.now = t, y.dist = 450.0f + static_cast<float>((t * 37) % 400);
      d1.observe(t, y.dist), d2.observe(t, y.dist);
      const GapOut a1 = d1.decide(y), a2 = d2.decide(y);
      same = same && a1.g == a2.g && a1.pick == a2.pick && a1.combo == a2.combo && a1.leap == a2.leap;
    }
    CHECK(same, "deterministic per seed");
  }
  // ---------------------------------------------------------------- E. the leap phase on the ground: start the combo / wait / cancel (never an attack in the void)
  {
    CHECK(leap_ground(300, true, 1, 30) == LpStep::StartCombo, "landed within reach and in front: the combo starts");
    CHECK(leap_ground(371, true, 1, 30) == LpStep::Wait && leap_ground(300, false, 1, 30) == LpStep::Wait, "out of reach / not in front: no combo yet (no attack in the void)");
    CHECK(leap_ground(500, true, policy::kLeapApproachTicks, 60) == LpStep::CancelTimeout, "Link escaped: bounded wait, then cancel");
    CHECK(leap_ground(701, true, 1, 30) == LpStep::CancelFar, "Link beyond 700: cancel");
    CHECK(leap_ground(300, true, 1, policy::kLeapWatchdogTicks + 1) == LpStep::CancelWatchdog, "watchdog");
    // fairness of the first impact after the landing: the landing clip (7 frames) + the combo's own wind-up (never retimed)
    for (unsigned ci = 4; ci < 8; ++ci) {
      const unsigned fi = first_impact_tick(ci);
      std::printf("LEAP %s: landing clip %u t + first impact %u t after the combo start = %u t after touchdown (%.2f s); from take-off (8): %u t\n", kCombos[ci].name,
                  static_cast<unsigned>(native::kEndTnbJump2), fi, static_cast<unsigned>(native::kEndTnbJump2) + fi, (native::kEndTnbJump2 + fi) / 30.0, leap_model(Leap::Fwd).ticks + fi);
      CHECK(fi >= 8 && native::kEndTnbJump2 + fi >= 15, "%s: never an instant hit hidden by the leap (>= 15 ticks from touchdown)", kCombos[ci].name);
    }
  }
  // ---------------------------------------------------------------- F. the reviewed reposition verdict (bands kept; geometry, native turn, long leap)
  {
    CHECK(repo_verdict7(100, 0, true, true, true, true, true, true) == Rp::Back && repo_verdict7(100, 0, true, true, true, false, true, true) == Rp::GeoUnsafe,
          "< 240: back jump only over safe ground behind (FIX06 never checked it)");
    CHECK(repo_verdict7(100, 0, false, true, true, true, true, true) == Rp::Unsafe && repo_verdict7(100, 130, true, true, true, true, true, true) == Rp::Unsafe, "event / height: no jump");
    CHECK(repo_verdict7(240, 0, true, true, true, true, true, true) == Rp::Hold && repo_verdict7(370, 0, true, true, true, true, true, true) == Rp::Hold, "240..370 hold");
    CHECK(repo_verdict7(395, 0, true, true, true, true, true, true) == Rp::ForwardTooClose && repo_verdict7(484.9f, 0, true, true, true, true, true, true) == Rp::ForwardTooClose,
          "370..485: any native leap would land on Link (log actor 363: 395 -> hold)");
    CHECK(repo_verdict7(490, 0, true, true, true, true, true, true) == Rp::Forward, "log actor 242: 490 -> forward (8)");
    CHECK(repo_verdict7(713, 0, true, true, true, true, true, true) == Rp::ForwardLong, "log actor 326: 713 -> the long native leap (17) lands at 308, nearer the middle");
    CHECK(repo_verdict7(748, -11, true, false, true, true, true, true) == Rp::WaitFront && repo_verdict7(748, -11, true, true, true, true, true, true) == Rp::ForwardLong,
          "log actor 275: 748 not in front -> native turn first, then the long leap (FIX06 held: HOLD_UNSAFE front=0)");
    CHECK(repo_verdict7(748, -11, true, false, false, true, true, true) == Rp::Unsafe, "still not in front after the turn: no jump sideways / backwards");
    CHECK(repo_verdict7(713, 0, true, true, true, true, true, false) == Rp::Forward && repo_verdict7(713, 0, true, true, true, true, false, false) == Rp::GeoUnsafe,
          "geometry: the long leap refused -> the short one; both refused -> hold");
    CHECK(repo_verdict7(500, 0, true, true, true, true, false, true) == Rp::GeoUnsafe, "500: the long leap would land on Link, the short one refused -> hold");
    for (float d = 371; d < 1200; d += 1) {
      const Rp r = repo_verdict7(d, 0, true, true, true, true, true, true);
      const float land = r == Rp::Forward ? d - 245 : r == Rp::ForwardLong ? d - 405 : 999;
      if (r == Rp::Forward || r == Rp::ForwardLong) CHECK(land >= policy::kRepoNear, "reposition at %.0f lands at %.0f >= 240", d, land);
    }
  }
  // ---------------------------------------------------------------- G. the trajectory verdict
  {
    CHECK(geo_sample(false, 0, 0) == Geo::Ok && geo_sample(true, 0, 0) == Geo::NoGround && geo_sample(false, -81, 0) == Geo::Drop && geo_sample(false, -80, 0) == Geo::Ok &&
              geo_sample(false, 51, 0) == Geo::Rise && geo_sample(false, 50, 0) == Geo::Ok,
          "a sample: no ground / drop > 80 / rise > 50");
    Geo s4[4] = {Geo::Ok, Geo::Ok, Geo::Ok, Geo::Ok};
    CHECK(geo_path(s4, 4, false, false) == Geo::Ok && geo_path(s4, 4, true, false) == Geo::Wall && geo_path(s4, 4, false, true) == Geo::Water, "clear / wall / water");
    s4[1] = Geo::NoGround;
    CHECK(geo_path(s4, 4, false, false) == Geo::NoGround, "a pit in the middle of the path refuses the leap even with ground at the landing");
    s4[1] = Geo::Ok, s4[3] = Geo::Drop;
    CHECK(geo_path(s4, 4, false, false) == Geo::Drop, "a ledge right behind the landing (body radius) refuses it");
  }
  // ---------------------------------------------------------------- H. the thrown mace: +50 / +60 / 0 / 0 for Link, 0 for the Darknut, ONE resolution per throw
  {
    CHECK(mace_verdict(false, false, false) == Mace::None && mace_verdict(true, false, true) == Mace::Direct && mace_verdict(true, true, false) == Mace::Block &&
              mace_verdict(true, true, true) == Mace::Parry,
          "verdicts (a window without a shield contact is never a parry)");
    CHECK(mace_link_points(Mace::Block) == 50 && mace_link_points(Mace::Direct) == 60 && mace_link_points(Mace::Parry) == 0 && mace_link_points(Mace::None) == 0,
          "approved: block +50, direct +60, parry 0, miss 0 (a dodge never makes a contact)");
    for (Mace m : {Mace::None, Mace::Block, Mace::Direct, Mace::Parry}) CHECK(mace_darknut_points(m) == 0, "never the Darknut's posture");
    MaceLedger L;
    CHECK(!L.resolve(Mace::Block), "no throw: nothing resolves");
    L.thrown();
    CHECK(L.resolve(Mace::Block) && !L.resolve(Mace::Direct) && !L.resolve(Mace::Parry) && L.blocks == 1 && L.directs == 0 && L.repeats == 2, "ONE resolution per throw");
    L.landed();
    L.thrown();
    CHECK(L.serial == 2 && L.resolve(Mace::Parry) && L.parries == 1, "a new throw (a second Darknut / a later transformation) gets its own");
    CHECK(!L.resolve(Mace::None), "a miss resolves nothing");
    // Link's posture saturates at 100 (lnk01): 0 / 40 / 50 / 99 / 100 + 50 / 60
    for (int b : {0, 40, 50, 99, 100})
      for (Mace m : {Mace::Block, Mace::Direct}) {
        const int after = b + mace_link_points(m) > 100 ? 100 : b + mace_link_points(m);
        CHECK(after <= 100 && after >= b, "Link %d + %d saturates at 100 (%d)", b, mace_link_points(m), after);
      }
  }
}
