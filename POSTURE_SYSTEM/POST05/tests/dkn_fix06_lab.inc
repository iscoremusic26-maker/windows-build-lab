// LIGHT FADES TWICE 0.13.15 DKN01 FIX06: the laboratory section of the FIX06 deltas (included by tests/dkn_lab.cpp, run by its main()). Every check runs the
// production core (src/dkn_core.hpp): the planner, Defense::confirm with routes, Seq, seq_gate, the sway, the phase-1 verdict, the reposition verdict.
// The sequence session below replays the adapter's PRE order tick by tick (Defense::due first, then seq_gate) with the move durations of the real plans.

// ---------------------------------------------------------------- a confirmed parry, exactly as the adapter reaches it (rebound -> SEEN -> CONFIRMED)
static bool fix06_confirm(Defense& d, std::uint64_t parry, bool p2, bool shield) {
  d.cp = Cp::Await, d.parry_tick = parry, d.counter_by = parry + policy::kCounterWaitTicks, d.counter_idx = -1, d.reverse_seen = false;
  if (d.observe_rebound(parry, true, 1.0f, p2, shield) != Rb::Seen) return false;
  return d.observe_rebound(parry + 1, true, 2.0f, p2, shield) == Rb::Confirmed;
}
// the move model of the adapter: an owned move started at tick s ends (NATIVE_END) at s + ceil(T[n]); its impacts land at s + the simulated impact ticks
struct F6Move {
  bool on = false;
  unsigned idx = 0;
  std::uint64_t start = 0, end = 0, impacts[9] = {};
  unsigned n = 0;
};
static F6Move fix06_move(unsigned idx, std::uint64_t start) {
  F6Move m;
  const Plan p = plan(combo_of(idx), idx);
  const SimOut o = simulate(combo_of(idx), p);
  m.on = true, m.idx = idx, m.start = start, m.n = o.n;
  for (unsigned i = 0; i < o.n && i < 9; ++i) m.impacts[i] = start + static_cast<std::uint64_t>(o.tick[i]);
  m.end = start + static_cast<std::uint64_t>(std::ceil(timeline(p).T[p.n]));
  return m;
}
// Link's ordinary input on one tick (what the adapter sees of him). Only `held` (a forced state) exists in the gate's inputs.
enum class F6Link : unsigned char { Idle = 0, BSpam, Block, Parry, ShieldAttack, Jump, Roll, Held, Count };
struct F6Event { std::uint64_t tick; int kind; unsigned idx; std::uint64_t ref; };  // kind 0 start, 1 end, 2 seq end, 3 continuation roll
struct F6Run {
  F6Event ev[32];
  unsigned n = 0;
  std::uint64_t first_move_impact = 0, last_counter_impact = 0, counter_end = 0, combo_start = 0, combo_first_impact = 0;
  bool direct = false, continued = false, ended = false, late_move = false;
  SqEnd why = SqEnd::None;
};
// One sequence from an authentic parry at tick P, Link doing `profile` every tick (or `held_from..held_to` in a forced state); `exc` = a legitimate exception
// at tick exc_tick (Break / Phase / Defeat / NoTarget...). After the end, 600 more ticks: nothing of the sequence may start (no late attack).
static F6Run fix06_session(std::uint64_t seed, bool p2, F6Link profile, std::uint64_t held_from = 0, std::uint64_t held_to = 0, SqEnd exc = SqEnd::None, std::uint64_t exc_tick = 0) {
  F6Run r;
  Defense d;
  Seq q;
  d.routes = true, d.rng.seed(seed * 3 + 1), d.route.seed(seed * 5 + 2), q.rng.seed(seed * 7 + 3);
  const std::uint64_t P = 1000;
  if (!fix06_confirm(d, P, p2, true)) return r;
  if (!q.begin(P + 1, P, d.route_combo, d.counter_idx)) return r;
  r.direct = d.route_combo;
  F6Move mv;
  for (std::uint64_t t = P + 2; t < P + 1200; ++t) {
    const bool held = t >= held_from && t < held_to;
    (void)profile;  // the profile is NOT an input of anything below: that is the property under test
    if (exc != SqEnd::None && t == exc_tick && (q.owns() || mv.on)) {  // the adapter: dkn_begin_break / phase / defeat ... -> move_end(cancel) + seq_end + Defense::cancel
      mv.on = false;
      if (d.cp == Cp::Scheduled) d.cp = Cp::None;
      d.cancel();
      q.finish(exc);
      r.ended = true, r.why = exc;
      if (r.n < 32) r.ev[r.n++] = {t, 2, 0, 0};
      continue;
    }
    if (mv.on) {  // POST: the owned move ends naturally at its end tick
      if (t >= mv.end) {
        mv.on = false;
        if (r.n < 32) r.ev[r.n++] = {t, 1, mv.idx, 0};
        if (q.st == Sq::Counter) {
          r.last_counter_impact = mv.n ? mv.impacts[mv.n - 1] : t, r.counter_end = t;
          r.continued = q.counter_done(t, r.last_counter_impact, p2);
          if (r.n < 32) r.ev[r.n++] = {t, 3, r.continued ? 1u : 0u, 0};
          if (!r.continued) r.ended = true, r.why = q.why;
        } else if (q.st == Sq::Combo) {
          q.combo_done(), r.ended = true, r.why = q.why;
        }
      }
      continue;
    }
    // PRE (lease None): the scheduled start first, then the gate
    const int c = d.due(t);
    if (c >= 0) {
      if (r.ended) r.late_move = true;
      mv = fix06_move(static_cast<unsigned>(c), t);
      const bool combo = !is_single(static_cast<unsigned>(c));
      q.started(combo);
      if (!r.first_move_impact && mv.n) r.first_move_impact = mv.impacts[0];
      if (combo && !r.combo_start) r.combo_start = t, r.combo_first_impact = mv.n ? mv.impacts[0] : 0;
      if (r.n < 32) r.ev[r.n++] = {t, 0, static_cast<unsigned>(c), 0};
      continue;
    }
    GateIn gi;
    gi.now = t, gi.cp = d.cp, gi.link_held = held, gi.dist = 250.0f;
    const GateOut go = seq_gate(q, gi);
    if (go.g == Gate::End) r.ended = true, r.why = go.why, q.finish(go.why);
    else if (go.g == Gate::StartContinuation) {
      if (r.ended) r.late_move = true;
      mv = fix06_move(static_cast<unsigned>(q.idx), t);
      q.started(true);
      r.combo_start = t, r.combo_first_impact = mv.n ? mv.impacts[0] : 0;
      if (r.n < 32) r.ev[r.n++] = {t, 0, static_cast<unsigned>(q.idx), 0};
    }
  }
  return r;
}
static bool fix06_same_schedule(const F6Run& a, const F6Run& b) {
  if (a.n != b.n) return false;
  for (unsigned i = 0; i < a.n; ++i)
    if (a.ev[i].tick != b.ev[i].tick || a.ev[i].kind != b.ev[i].kind || a.ev[i].idx != b.ev[i].idx) return false;
  return true;
}

static void fix06_section() {
  std::printf("\n==== FIX06 (0.13.15) ====\n");
  // ---------------------------------------------------------------- A. selective retiming: five final targets, everything else of the eight plans untouched
  {
    CHECK(std::string_view(kVersion) == "0.13.15" && std::string_view(kTag) == "DKN01_FIX06", "version / tag");
    unsigned n = 0;
    for (const IvTarget& t : kIvTargets) {
      ++n;
      CHECK((t.combo == 5 || t.combo == 7) && t.ticks >= 25.0f && t.ticks <= 30.0f, "target %u of F2-C2 / F2-C4 only", n);
    }
    CHECK(n == 5 && iv_target(5, 0) == 27.0f && iv_target(5, 2) == 30.0f && iv_target(7, 0) == 27.0f && iv_target(7, 1) == 25.5f && iv_target(7, 3) == 30.0f,
          "the approved table: C2 1-2 27, C2 3-4 30, C4 1-2 27, C4 2-3 25.5, C4 4-5 30 (final ticks after x1.40)");
    for (unsigned j = 0; j < 9; ++j) CHECK(iv_target(4, j) < 0 && iv_target(6, j) < 0 && iv_target(0, j) < 0 && iv_target(1, j) < 0 && iv_target(2, j) < 0 && iv_target(3, j) < 0,
                                           "F2-C1 / F2-C3 / phase 1: no target (pair %u)", j);
    CHECK(iv_target(5, 1) < 0 && iv_target(5, 3) < 0 && iv_target(5, 4) < 0 && iv_target(5, 5) < 0 && iv_target(7, 2) < 0, "the doubles' inner cuts and C2 2-3 are protected");
    std::printf("RETIME combo pair   FIX05_plan   FIX06_target  sim_ticks  quantization\n");
    const int log_fix05[2][6] = {{37, 9, 47, 12, 12, 13}, {37, 34, 8, 43, 0, 0}};  // the user's log dusklight-20261121-234432 (COMBO_SWING ticks)
    for (unsigned ci : {5u, 7u}) {
      const Plan p6 = plan(kCombos[ci], ci), p5 = plan(kCombos[ci], ci, policy::kF2ComboScale);  // p5 = the FIX05 plan, bit for bit (3e)
      const SimOut o6 = simulate(kCombos[ci], p6), o5 = simulate(kCombos[ci], p5);
      CHECK(o6.ok && o5.ok, "%s simulates", kCombos[ci].name);
      for (unsigned j = 0; j + 1 < kCombos[ci].n_impacts; ++j) {
        const bool rt = iv_target(ci, j) > 0;
        const double s6 = o6.tick[j + 1] - o6.tick[j], s5 = o5.tick[j + 1] - o5.tick[j];
        std::printf("RETIME %s %u->%u  %6.2f (sim %2.0f, log %2d)  %6.2f%s  %3.0f  %+.2f\n", kCombos[ci].name, j + 1, j + 2, p5.target_ticks[j], s5, log_fix05[ci == 5 ? 0 : 1][j], p6.target_ticks[j],
                    rt ? " *" : "  ", s6, s6 - p6.target_ticks[j]);
        if (rt) CHECK(std::fabs(s6 - iv_target(ci, j)) <= 0.5 + 1e-6 && std::fabs(p6.target_ticks[j] - iv_target(ci, j)) < 1e-5, "%s pair %u reaches its target %.1f (sim %.0f)", kCombos[ci].name, j + 1,
                      iv_target(ci, j), s6);
        else CHECK(p6.target_ticks[j] == p5.target_ticks[j] && std::fabs(s6 - p6.target_ticks[j]) < 1.0, "%s protected pair %u: planned %.2f unchanged (sim %.0f, FIX05 sim %.0f)", kCombos[ci].name,
                   j + 1, p6.target_ticks[j], s6, s5);
      }
      // the protected pairs keep their exact plan parameters: inner doubles (rate_mid) and the boundary pairs that are not retimed (cut / start / rates)
      unsigned imp = 0;
      for (unsigned m = 0; m < p6.n; ++m) {
        const MoveInfo& mi = info(p6.seg[m].m);
        if (mi.impacts == 2) CHECK(p6.seg[m].rate_mid == p5.seg[m].rate_mid, "%s seg %u: the double's inner cut interval untouched (%.4f / %.4f)", kCombos[ci].name, m, p6.seg[m].rate_mid,
                                   p5.seg[m].rate_mid);
        imp += mi.impacts;
        if (m + 1 < p6.n && !(iv_target(ci, imp - 1) > 0))
          CHECK(p6.seg[m].cut == p5.seg[m].cut && p6.seg[m].rate_out == p5.seg[m].rate_out && p6.seg[m + 1].start == p5.seg[m + 1].start && p6.seg[m + 1].rate_in == p5.seg[m + 1].rate_in,
                "%s boundary %u (protected pair %u): cut / start / rates untouched", kCombos[ci].name, m, imp);
        CHECK(p6.seg[m].m == p5.seg[m].m && p6.seg[m].morph == p5.seg[m].morph, "%s seg %u: same move, same morph", kCombos[ci].name, m);
        CHECK(p6.seg[m].rate_in >= 0.999f && p6.seg[m].rate_in <= 1.6f && p6.seg[m].rate_out >= 0.999f && p6.seg[m].rate_out <= 1.6f, "%s seg %u: natural rates (no slow motion, no rush)",
              kCombos[ci].name, m);
      }
      CHECK(p6.seg[0].start == p5.seg[0].start && p6.seg[0].rate_in == p5.seg[0].rate_in, "%s: the opening wind-up untouched", kCombos[ci].name);
      CHECK(p6.seg[p6.n - 1].cut == p5.seg[p6.n - 1].cut && p6.seg[p6.n - 1].rate_out == p5.seg[p6.n - 1].rate_out, "%s: the final recovery untouched", kCombos[ci].name);
      CHECK(o6.tick[0] == o5.tick[0], "%s: the first impact at the same tick (%.0f)", kCombos[ci].name, o6.tick[0]);
    }
    for (unsigned ci : {0u, 1u, 2u, 3u, 4u, 6u})
      CHECK(plan_fp(ci, plan(kCombos[ci], ci)) == plan_fp(ci, plan(kCombos[ci], ci, kCombos[ci].phase2 ? policy::kF2ComboScale : 0.0f)), "%s: untouched plan", kCombos[ci].name);
    for (unsigned pi = 8; pi < kPlans; ++pi) CHECK(plan_fp(pi, plan(combo_of(pi), pi)) == plan_fp(pi, plan(combo_of(pi), 99)) || pi == kIdxF2Double, "single / counter %u untouched", pi);
    CHECK(plan(kCombos[5], 99).target_ticks[0] != 27.0f, "the targets need the combo's own index (the default 99 = the FIX05 plan)");
  }
  // ---------------------------------------------------------------- B. routes: ONE roll per confirmed parry (60 / 40), ONE per concluded counter (80 / 20)
  {
    const unsigned N = 200000;
    unsigned combo = 0, counters = 0, pick[2][4] = {}, ctr[2][3] = {}, wrong = 0;
    for (unsigned i = 0; i < N; ++i) {
      const bool p2 = i & 1;
      Defense d;
      d.routes = true, d.rng.seed(i * 13 + 7), d.route.seed(i * 17 + 9);
      if (!fix06_confirm(d, 500, p2, true)) {
        ++wrong;
        continue;
      }
      wrong += d.route_rolls != 1 || d.confirmed != 1;
      CHECK(d.observe_rebound(503, true, 4.0f, p2, true) == Rb::None && d.route_rolls == 1, "a later read never rolls again");
      if (d.route_combo) {
        ++combo;
        const unsigned k = d.counter_idx >= 0 ? static_cast<unsigned>(d.counter_idx) - (p2 ? 4u : 0u) : 99u;
        wrong += k > 3 || d.counter_idx < 0 || d.counter_idx > 7 || combo_of(static_cast<unsigned>(d.counter_idx) & 7u).phase2 != p2;
        ++pick[p2][k & 3];
        const unsigned land = static_cast<unsigned>(d.counter_at - d.parry_tick) + first_impact_tick(static_cast<unsigned>(d.counter_idx));
        wrong += land < policy::kParryComboLandMin || land > policy::kParryComboLandMax;
      } else {
        ++counters;
        const int c = d.counter_idx;
        wrong += !is_single(static_cast<unsigned>(c));
        ++ctr[p2][p2 ? (c == int(kIdxF2Kick) ? 0 : c == int(kIdxF2Thrust) ? 1 : 2) : (c == int(kIdxF1Bash) ? 0 : c == int(kIdxF1SingleA) ? 1 : 2)];
        const unsigned land = static_cast<unsigned>(d.counter_at - d.parry_tick) + first_impact_tick(static_cast<unsigned>(c));
        wrong += land < policy::kCounterLandTicks;
      }
    }
    const double pc = 100.0 * combo / N;
    std::printf("ROUTE parries=%u direct_combo=%.2f%% counter=%.2f%% F1 picks %.1f/%.1f/%.1f/%.1f F2 picks %.1f/%.1f/%.1f/%.1f F1 counters %.1f/%.1f/%.1f F2 counters %.1f/%.1f/%.1f\n", N,
                pc, 100.0 * counters / N, 100.0 * pick[0][0] / (combo / 2.0), 100.0 * pick[0][1] / (combo / 2.0), 100.0 * pick[0][2] / (combo / 2.0), 100.0 * pick[0][3] / (combo / 2.0),
                100.0 * pick[1][0] / (combo / 2.0), 100.0 * pick[1][1] / (combo / 2.0), 100.0 * pick[1][2] / (combo / 2.0), 100.0 * pick[1][3] / (combo / 2.0),
                100.0 * ctr[0][0] / (counters / 2.0), 100.0 * ctr[0][1] / (counters / 2.0), 100.0 * ctr[0][2] / (counters / 2.0), 100.0 * ctr[1][0] / (counters / 2.0),
                100.0 * ctr[1][1] / (counters / 2.0), 100.0 * ctr[1][2] / (counters / 2.0));
    CHECK(wrong == 0, "every confirmed parry: one roll, a combo of its own phase landing 27..36 or a counter landing >= 24 (%u wrong)", wrong);
    CHECK(std::fabs(pc - 60.0) < 0.5, "60 / 40 (%.2f %%)", pc);
    for (unsigned ph = 0; ph < 2; ++ph)
      for (unsigned k = 0; k < 4; ++k) CHECK(std::fabs(100.0 * pick[ph][k] / (combo / 2.0) - 25.0) < 1.0, "F%u combo %u picked %.2f %% (25)", ph + 1, k, 100.0 * pick[ph][k] / (combo / 2.0));
    for (unsigned ph = 0; ph < 2; ++ph)
      for (unsigned k = 0; k < 3; ++k) CHECK(std::fabs(100.0 * ctr[ph][k] / (counters / 2.0) - 33.33) < 1.5, "F%u counter thirds kept (%u: %.2f %%)", ph + 1, k, 100.0 * ctr[ph][k] / (counters / 2.0));
    // the continuation: 80 / 20, one roll per concluded counter, equal picks among the phase's four combos
    unsigned cont = 0, cpick[2][4] = {}, cw = 0;
    for (unsigned i = 0; i < N; ++i) {
      const bool p2 = i & 1;
      Seq q;
      q.rng.seed(i * 29 + 5);
      q.begin(10, 9, false, p2 ? int(kIdxF2Thrust) : int(kIdxF1SingleA)), q.started(false);
      const bool c = q.counter_done(60, 40, p2);
      cw += q.cont_rolls != 1 || q.counter_done(61, 40, p2) || q.cont_rolls != 1;
      const unsigned ki = q.idx >= 0 ? static_cast<unsigned>(q.idx) - (p2 ? 4u : 0u) : 99u;  // bounds first: a broken rule is a FAIL, never a crash
      if (c && ki < 4) ++cont, ++cpick[p2][ki], cw += q.st != Sq::Bridge || combo_of(static_cast<unsigned>(q.idx)).phase2 != p2;
      else if (c) ++cw;
      else cw += q.st != Sq::None || q.why != SqEnd::CounterOnly;
    }
    const double pk = 100.0 * cont / N;
    std::printf("CONTINUATION counters=%u combo=%.2f%% exit=%.2f%% theoretical_sequences_with_a_combo=%.1f%% (60 + 40 x 80)\n", N, pk, 100.0 - pk,
                policy::kParryComboPct + (100 - policy::kParryComboPct) * policy::kCounterComboPct / 100.0);
    CHECK(cw == 0 && std::fabs(pk - 80.0) < 0.5, "80 / 20 (%.2f %%), one roll per counter (%u wrong)", pk, cw);
    for (unsigned ph = 0; ph < 2; ++ph)
      for (unsigned k = 0; k < 4; ++k) CHECK(std::fabs(100.0 * cpick[ph][k] / (cont / 2.0) - 25.0) < 1.0, "F%u continuation combo %u: %.2f %%", ph + 1, k, 100.0 * cpick[ph][k] / (cont / 2.0));
    // routes off = FIX05 (counter only): the earlier labs keep their meaning
    Defense legacy;
    legacy.rng.seed(3);
    CHECK(fix06_confirm(legacy, 100, true, true) && is_single(static_cast<unsigned>(legacy.counter_idx)) && legacy.route_rolls == 0, "routes off: the FIX05 counter-only rule");
    // credit only (a break preempted the confirmation): no route, no sequence
    Defense co;
    co.routes = true, co.route.seed(1), co.cp = Cp::CreditOnly, co.parry_tick = 50, co.counter_by = 60;
    CHECK(co.confirm(51, true, true, true) && co.cp == Cp::None && co.route_rolls == 0, "a credit-only confirmation never rolls a route");
    // a sequence never stacks over a live one
    Seq q;
    CHECK(q.begin(5, 4, true, 4) && !q.begin(6, 5, false, 10) && q.begun == 1, "never two sequences at once");
    CHECK(seq_end_of("NATIVE_END") == SqEnd::None && seq_end_of("B1_RECOVERY") == SqEnd::None && seq_end_of("POSTURE_BREAK") == SqEnd::Break && seq_end_of("PHASE_CHANGE") == SqEnd::Phase &&
              seq_end_of("DEFEAT") == SqEnd::Defeat && seq_end_of("NO_TARGET") == SqEnd::NoTarget && seq_end_of("EVENT") == SqEnd::Event && seq_end_of("LINK_GUARD_BROKEN") == SqEnd::LinkBroken &&
              seq_end_of("LINK_FAR") == SqEnd::LinkFar && seq_end_of("WATCHDOG") == SqEnd::Watchdog && seq_end_of("ACTION_LOST") == SqEnd::Refused,
          "owned-move end reasons: natural (NATIVE_END / B1_RECOVERY) or the legitimate exceptions");
  }
  // ---------------------------------------------------------------- C. the sequence owns the initiative: Link's ordinary actions change NOTHING of its schedule
  {
    unsigned sessions = 0, combos_direct = 0, counters_only = 0, continued = 0, mismatch = 0, late = 0;
    unsigned min_direct = 999, max_direct = 0, min_counter = 999, min_bridge = 999, max_bridge = 0, min_settle = 999, max_settle = 0;
    for (unsigned seed = 1; seed <= 4000; ++seed)
      for (int p2 = 0; p2 < 2; ++p2) {
        const F6Run base = fix06_session(seed, p2 != 0, F6Link::Idle);
        ++sessions;
        if (base.n == 0) {  // no confirmed parry / no sequence at all: a failure, never an out-of-range read below
          CHECK(false, "seed %u F%d: no sequence was opened by the confirmed parry", seed, p2 + 1);
          continue;
        }
        CHECK(base.n > 0 && base.ended && (base.why == SqEnd::Done || base.why == SqEnd::CounterOnly), "seed %u F%d: the sequence ends naturally (%s)", seed, p2 + 1, name(base.why));
        late += base.late_move;
        for (unsigned pr = 1; pr < unsigned(F6Link::Held); ++pr) mismatch += !fix06_same_schedule(base, fix06_session(seed, p2 != 0, F6Link(pr)));
        if (base.direct) {
          ++combos_direct;
          const unsigned land = static_cast<unsigned>(base.first_move_impact - 1000);
          min_direct = land < min_direct ? land : min_direct, max_direct = land > max_direct ? land : max_direct;
        } else {
          const unsigned land = static_cast<unsigned>(base.first_move_impact - 1000);
          min_counter = land < min_counter ? land : min_counter;
          if (base.continued) {
            ++continued;
            const unsigned gap = static_cast<unsigned>(base.combo_first_impact - base.last_counter_impact), settle = static_cast<unsigned>(base.combo_start - base.counter_end);
            min_bridge = gap < min_bridge ? gap : min_bridge, max_bridge = gap > max_bridge ? gap : max_bridge;
            min_settle = settle < min_settle ? settle : min_settle, max_settle = settle > max_settle ? settle : max_settle;
          } else {
            ++counters_only;
          }
        }
      }
    std::printf("SEQUENCE sessions=%u direct=%u (rebound->first impact %u..%u) counter=%u (rebound->first impact >= %u) continued=%u (last counter impact -> first combo impact %u..%u, "
                "counter end -> combo start %u..%u) exit20=%u spam_profiles_changing_the_schedule=%u late_moves=%u\n",
                sessions, combos_direct, min_direct, max_direct, sessions - combos_direct, min_counter, continued, min_bridge, max_bridge, min_settle, max_settle, counters_only, mismatch, late);
    CHECK(mismatch == 0, "B spam / block / parry / Shield Attack / jump / roll never move, cancel, reorder or delay the sequence (%u mismatches)", mismatch);
    CHECK(late == 0, "nothing of a sequence starts after its end");
    CHECK(min_direct >= policy::kParryComboLandMin && max_direct <= policy::kParryComboLandMax, "direct combo lands %u..%u ticks after the rebound (approved 27..36)", min_direct, max_direct);
    CHECK(min_counter >= policy::kCounterLandTicks, "counter lands >= 24 ticks after the rebound (%u)", min_counter);
    CHECK(min_bridge >= policy::kBridgeLandTicks && min_settle >= policy::kBridgeSettleTicks, "continuation: first impact >= 24 after the last counter impact (%u), start >= 3 after the counter end (%u)",
          min_bridge, min_settle);
    CHECK(max_settle <= policy::kBridgeSettleTicks + 12, "no second long wait over a long counter recovery (counter end -> combo start <= %u, got %u)", policy::kBridgeSettleTicks + 12, max_settle);
    // per counter kind and phase: the measured bridge (last impact -> counter end -> combo start -> first impact), every continuation combo
    std::printf("BRIDGE counter              last_impact->end  end->start  start->1st_impact  last_impact->1st_impact\n");
    for (unsigned ci : {kIdxF2Kick, kIdxF2Thrust, kIdxF2Double, kIdxF1Bash, kIdxF1SingleA, kIdxF1SingleB}) {
      const F6Move m = fix06_move(ci, 0);
      const bool p2 = combo_of(ci).phase2;
      if (m.n == 0 || m.n > 9) {  // a counter without its impacts: a failure, never an out-of-range read
        CHECK(false, "%s simulates no impact", combo_of(ci).name);
        continue;
      }
      for (unsigned k = 0; k < 4; ++k) {
        const unsigned cidx = (p2 ? 4u : 0u) + k;
        const std::uint64_t due = bridge_due(m.end, m.impacts[m.n - 1], cidx);
        const unsigned fi = first_impact_tick(cidx);
        std::printf("BRIDGE %-16s -> %-6s %4llu %10llu %12u %18llu\n", combo_of(ci).name, combo_of(cidx).name, static_cast<unsigned long long>(m.end - m.impacts[m.n - 1]),
                    static_cast<unsigned long long>(due - m.end), fi, static_cast<unsigned long long>(due + fi - m.impacts[m.n - 1]));
        CHECK(due + fi >= m.impacts[m.n - 1] + policy::kBridgeLandTicks && due >= m.end + policy::kBridgeSettleTicks, "%s -> %s bridge fair", combo_of(ci).name, combo_of(cidx).name);
      }
      CHECK(ci != kIdxF2Double || m.n == 2, "the counter double keeps two impacts");
    }
    // a Link held by a forced state after the counter: waited for, then kBridgeFreeTicks; never forever
    {
      unsigned waited = 0, dropped = 0, checked = 0;
      for (unsigned seed = 1; seed <= 3000 && checked < 200; ++seed) {
        const F6Run base = fix06_session(seed, true, F6Link::Idle);
        if (base.direct || !base.continued) continue;
        ++checked;
        const F6Run h = fix06_session(seed, true, F6Link::Idle, base.counter_end, base.counter_end + 20);
        CHECK(h.continued && h.combo_start >= base.counter_end + 20 + policy::kBridgeFreeTicks && h.combo_start >= base.combo_start, "held 20 ticks: the combo waits until he is free + 6");
        waited += h.combo_start > base.combo_start;
        const F6Run x = fix06_session(seed, true, F6Link::Idle, base.counter_end, base.counter_end + 400);
        CHECK(x.ended && x.why == SqEnd::Incapacitated && x.combo_start == 0 && !x.late_move, "held beyond the bridge: the continuation is dropped once (%s)", name(x.why));
        dropped += x.why == SqEnd::Incapacitated;
      }
      CHECK(checked > 50 && waited == checked && dropped == checked, "forced-state bridge cases %u / %u / %u", checked, waited, dropped);
    }
    // legitimate exceptions at every stage of the sequence: it ends ONCE and nothing it scheduled ever starts afterwards
    unsigned exc_cases = 0, exc_ok = 0;
    for (unsigned seed = 1; seed <= 300; ++seed)
      for (int p2 = 0; p2 < 2; ++p2) {
        const F6Run base = fix06_session(seed, p2 != 0, F6Link::Idle);
        if (base.n == 0) {
          CHECK(false, "exceptions seed %u: no sequence", seed);
          continue;
        }
        const std::uint64_t last = base.ev[base.n - 1].tick;
        for (SqEnd e : {SqEnd::Break, SqEnd::Phase, SqEnd::Defeat, SqEnd::NoTarget, SqEnd::Event, SqEnd::LinkBroken})
          for (std::uint64_t t = 1002; t <= last; t += 3) {
            const F6Run x = fix06_session(seed, p2 != 0, F6Link::BSpam, 0, 0, e, t);
            ++exc_cases;
            bool after = false;
            for (unsigned i = 0; i < x.n; ++i) after = after || (x.ev[i].kind == 0 && x.ev[i].tick > t);
            exc_ok += x.ended && x.why == e && !after && !x.late_move;
          }
      }
    std::printf("SEQUENCE_EXCEPTIONS cases=%u ok=%u (break / phase / defeat / no target / event / Link guard broken at every 3rd tick of every stage)\n", exc_cases, exc_ok);
    CHECK(exc_cases > 1000 && exc_ok == exc_cases, "every exception ends the sequence once, no late move (%u / %u)", exc_ok, exc_cases);
    // the gate itself: Prep without its schedule, an orphan Counter / Combo, the watchdog, the context
    {
      Seq g;
      GateIn in;
      in.now = 10, in.cp = Cp::Scheduled;
      CHECK(seq_gate(g, in).g == Gate::Open, "no sequence: the common combat");
      g.begin(10, 9, true, 4);
      CHECK(seq_gate(g, in).g == Gate::Own, "Prep with its schedule: owned (nothing else may start)");
      in.cp = Cp::None;
      GateOut o = seq_gate(g, in);
      CHECK(o.g == Gate::End && o.why == SqEnd::Dropped, "Prep whose schedule was dropped ends");
      Seq h;
      h.begin(10, 9, false, 10), h.started(false);
      o = seq_gate(h, in);
      CHECK(o.g == Gate::End && o.why == SqEnd::Refused, "a Counter state without its owned move ends (never an orphan owner)");
      Seq w;
      w.begin(10, 9, true, 4);
      in.cp = Cp::Scheduled, in.now = 10 + policy::kSeqWatchdogTicks + 1;
      CHECK(seq_gate(w, in).why == SqEnd::Watchdog, "watchdog");
      Seq b;
      b.rng.seed(4);
      b.begin(10, 9, false, 10), b.started(false);
      while (!b.counter_done(60, 40, true)) b = Seq{}, b.rng.seed(b.rng.next() + 1), b.begin(10, 9, false, 10), b.started(false);
      GateIn c;
      c.now = 61, c.dist = 250;
      c.link_ok = false;
      Seq b1 = b, b2 = b, b3 = b, b4 = b;
      CHECK(seq_gate(b1, c).why == SqEnd::NoTarget, "bridge: no target");
      c.link_ok = true, c.link_broken = true;
      CHECK(seq_gate(b2, c).why == SqEnd::LinkBroken, "bridge: Link guard broken");
      c.link_broken = false, c.safe = false;
      CHECK(seq_gate(b3, c).why == SqEnd::Event, "bridge: event");
      c.safe = true, c.dist = 701;
      CHECK(seq_gate(b4, c).why == SqEnd::LinkFar, "bridge: Link beyond 700");
    }
  }
  // ---------------------------------------------------------------- D. phase 1: Mortal Draw / Helm Splitter, 100 HP TOTAL, natural recovery, transformation first
  {
    CHECK(exec_verdict_f1(false, Bk::StaggerA, true, false) && exec_verdict_f1(false, Bk::StaggerB, false, true), "F1: Mortal Draw in the first stagger, Helm Splitter in the second");
    CHECK(!exec_verdict_f1(false, Bk::StaggerB, true, false) && !exec_verdict_f1(false, Bk::StaggerA, false, true) && !exec_verdict_f1(false, Bk::Combat, true, true) &&
              !exec_verdict_f1(false, Bk::Recover, true, true) && !exec_verdict_f1(false, Bk::Pending, true, true) && !exec_verdict_f1(false, Bk::StaggerA, false, false),
          "F1: no punishment outside its stagger, never by an ordinary cut, never twice (Recover after the first)");
    CHECK(!exec_verdict_f1(true, Bk::StaggerA, true, false) && !exec_verdict_f1(true, Bk::StaggerB, false, true), "phases 2-3 keep their own execution FALL (exec_verdict)");
    CHECK(exec_verdict(true, Bk::StaggerA, true, false, false) == Ex::Fall && exec_verdict(false, Bk::StaggerA, true, false, false) == Ex::None, "the F2/F3 verdict is unchanged");
    struct Hp { int before, after; } hp[] = {{600, 500}, {450, 350}, {101, 1}, {100, 0}, {99, 0}, {1, 0}, {0, 0}};
    for (const Hp& h : hp) CHECK(f1_exec_hp(h.before) == h.after, "100 HP total: %d -> %d (got %d)", h.before, h.after, f1_exec_hp(h.before));
    CHECK(policy::kF1ExecDamage == 100, "approved 100");
    // armour: the special crossing one or two 50-HP thresholds advances exactly the progress of the remaining HP (never a double credit, never below)
    for (int b = 600; b > 0; b -= 7) {
      const int a = f1_exec_hp(b), pb = armor_progress(b), pa = armor_progress(a);
      CHECK(pa >= pb && pa - pb <= 2 && (a > 0 || pa == 12) && (a == 0 || pa <= 11), "armour %d -> %d: progress %d -> %d", b, a, pb, pa);
      CHECK(armor_call(pa, pb) == (pa > pb && pa <= 11 ? pa : -1), "armour call %d over %d broken", pa, pb);
    }
    Break f;
    f.latch(), f.begin(0, false);
    CHECK(f.helm_prep(10) && f.st == Bk::StaggerB && !f.helm_prep(11) && f.exec_f1() && f.st == Bk::Recover && !f.exec_f1() && !f.helm_prep(12), "F1: one preparation, one punishment, then recovery");
    Break g;
    g.latch(), g.begin(0, false);
    CHECK(g.exec_f1() && g.st == Bk::Recover && !g.helm_prep(5), "F1 Mortal Draw: the Helm Splitter preparation is no longer possible in the same break");
    CHECK(sa_verdict(false, Bk::StaggerA, true) == Sa::HelmPrep && sa_verdict(false, Bk::Recover, true) == Sa::Reject && sa_verdict(false, Bk::Combat, true) == Sa::Reject,
          "F1 Helm Splitter preparation only by a NEW Shield Attack in the first stagger");
  }
  // ---------------------------------------------------------------- E. reposition after the rise / getup: one native jump or none
  {
    const Jump b = jump_model(true), f = jump_model(false);
    std::printf("REPOSITION native back jump %.0f units / %u air ticks / %u total; forward %.0f / %u / %u; forward needs distance >= %.0f to land >= %.0f\n", b.dist, b.air, b.ticks, f.dist, f.air,
                f.ticks, policy::kRepoNear + f.dist, policy::kRepoNear);
    CHECK(b.dist == 450.0f && b.air == 9 && f.dist == 245.0f && f.air == 7, "native jump physics (back 50 x 9, forward 35 x 7)");
    CHECK(repo_verdict(0, 0, true, false, true) == Rp::Back && repo_verdict(239.9f, 0, true, false, true) == Rp::Back && repo_verdict(240.0f, 0, true, false, true) == Rp::Hold &&
              repo_verdict(370.0f, 0, true, false, true) == Rp::Hold && repo_verdict(370.1f, 0, true, false, true) == Rp::ForwardTooClose &&
              repo_verdict(484.9f, 0, true, false, true) == Rp::ForwardTooClose && repo_verdict(485.0f, 0, true, false, true) == Rp::Forward && repo_verdict(900.0f, 0, true, false, true) == Rp::Forward,
          "bands: < 240 back, 240..370 hold, > 370 forward only when it lands >= 240");
    CHECK(repo_verdict(100, 0, false, false, true) == Rp::Unsafe && repo_verdict(100, 0, true, true, true) == Rp::Unsafe && repo_verdict(100, 130, true, false, true) == Rp::Unsafe &&
              repo_verdict(600, 0, true, false, false) == Rp::Unsafe && repo_verdict(600, -130, true, false, true) == Rp::Unsafe && repo_verdict(600, 0, false, false, true) == Rp::Unsafe &&
              repo_verdict(300, 0, false, true, false) == Rp::Hold,
          "unsafe (event, wall behind, height, Link not in front): no jump; the middle band never jumps");
    CHECK(native::jump_mode(6) && native::jump_mode(7) && native::jump_mode(8) && native::jump_mode(9) && !native::jump_mode(17) && !native::jump_mode(1), "the two native jumps and their landings");
    CHECK(native::native_leap(native::kActChaseL, 8) && native::native_attack_entry(native::kActAttackL, 10) && native::native_attack_entry(native::kActAttackShieldL, 0),
          "the leap suppression without a lease and the landing attack entries stay");
  }
  // ---------------------------------------------------------------- F. sword darkening: only the definitive death, the body's own value
  {
    CHECK(!sword_tint(false, -50.0f) && !sword_tint(true, 0.0f) && sword_tint(true, -0.3f) && sword_tint(true, -50.0f), "definitive death AND a darkening body only");
    CHECK(tint_value(-49.7f) == -49 && tint_value(-0.3f) == 0 && tint_value(-50.0f) == -50, "the body's float -> s16 conversion");
  }
}
