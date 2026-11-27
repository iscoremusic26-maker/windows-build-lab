// LIGHT FADES TWICE 0.13.17 DKN01 FIX08: the dkn_lab section of the FIX08 deltas (included by tests/dkn_lab.cpp after the FIX07 section; same CHECK).
// Pure core only: Table B, F1-C3 1->2, the F1 Helm recovery look, the defensive evasion, the swing threat, the neutral receipt, the B1 recovery constants and the
// sword's darkening value. What a player sees / hears is HUMAN_RUNTIME_PENDING.
static void fix08_section() {
  std::printf("\n==== FIX08 (0.13.17) ====\n");
  CHECK(std::string_view(kVersion) == "0.13.17" && std::string_view(kTag) == "DKN01_FIX08", "FIX08 version / tag");
  // ---------------------------------------------------------------- A. Table B: exactly the six approved F1 values changed, every other value identical
  {
    CHECK(link_gain(Move::F1A, Outcome::Block) == 24 && link_gain(Move::F1A, Outcome::Direct) == 36, "F1 lateral 20/30 -> 24/36");
    CHECK(link_gain(Move::F1B, Outcome::Block) == 31 && link_gain(Move::F1B, Outcome::Direct) == 43, "F1 vertical 26/36 -> 31/43");
    CHECK(link_gain(Move::F1Shield, Outcome::Block) == 50 && link_gain(Move::F1Shield, Outcome::Direct) == 60, "F1 shield bash 42/50 -> 50/60");
    for (Move m : {Move::F1A, Move::F1B, Move::F1Shield, Move::F2A, Move::F2B, Move::F2B1, Move::F2C}) CHECK(link_gain(m, Outcome::Parry) == 0, "%s: a parry gives Link +0", name(m));
    CHECK(link_gain(Move::F2A, Outcome::Block) == 23 && link_gain(Move::F2A, Outcome::Direct) == 33 && link_gain(Move::F2B, Outcome::Block) == 12 && link_gain(Move::F2B, Outcome::Direct) == 22 &&
              link_gain(Move::F2B1, Outcome::Block) == 23 && link_gain(Move::F2B1, Outcome::Direct) == 33 && link_gain(Move::F2C, Outcome::Block) == 38 && link_gain(Move::F2C, Outcome::Direct) == 45,
          "F2/F3 Table B unchanged (thrust 23/33, each double cut 12/22, B1 23/33, kick 38/45)");
    CHECK(mace_link_points(Mace::Block) == 50 && mace_link_points(Mace::Direct) == 60 && mace_link_points(Mace::Parry) == 0, "thrown mace unchanged 50 / 60 / 0");
    CHECK(policy::kLinkEnemyParry1 == 20 && policy::kLinkEnemyParry2 == 25, "Link's posture on an authentic Darknut parry unchanged (+20 F1 / +25 F2-F3)");
    CHECK(dk_parry_gain(Move::F1A, 0) == 12 && dk_parry_gain(Move::F1B, 0) == 14 && dk_parry_gain(Move::F1Shield, 0) == 14 && policy::kGainBlocked1 == 4 && policy::kGainSwordHp1 == 6 &&
              policy::kGainIron1 == 25, "posture inflicted ON the Darknut unchanged (F1 +4 / +6, parries 12 / 14 / 14, iron 25)");
    // saturation at 100 (Link's max): one credit per impact, never above the max
    const int sat[][3] = {{76, 24, 100}, {69, 31, 100}, {50, 50, 100}, {64, 36, 100}, {57, 43, 100}, {40, 60, 100}, {77, 24, 100}, {99, 60, 100}, {0, 24, 24}};
    for (const auto& x : sat) {
      const int after = x[0] + x[1] > policy::kLinkMax ? policy::kLinkMax : x[0] + x[1];
      CHECK(after == x[2], "Link %d + %d = %d (saturated at 100)", x[0], x[1], after);
    }
    // every F1 combo, all blocked, from 0: the impact that breaks Link (posture 100) with the new table
    for (unsigned ci = 0; ci < 4; ++ci) {
      const Combo& c = kCombos[ci];
      int lp = 0, br = -1;
      for (unsigned m = 0; m < c.n_moves; ++m) {
        lp += link_gain(c.moves[m], Outcome::Block);
        if (lp >= 100 && br < 0) br = static_cast<int>(m) + 1;
        if (lp > 100) lp = 100;
      }
      std::printf("FIX08 %s all-blocked from 0: Link breaks at impact %d\n", c.name, br);
      CHECK(br == (ci == 1 ? -1 : 4), "%s all blocked: Link's posture saturates at impact %d (FIX07 never / 5)", c.name, br);
    }
  }
  // ---------------------------------------------------------------- B. F1-C3 36 / 56 / 57 / 40, the 1.76 exception retired, the other plans untouched
  {
    const Combo& c = kCombos[kF1C3];
    const Plan p = plan(c, kF1C3), p7 = plan_f1c3_fix07();
    const SimOut o = simulate(c, p);
    CHECK(o.ok && o.n == 5, "F1-C3 simulates its five impacts");
    const double want[4] = {36, 56, 57, 40};
    double total = 0, ptotal = 0;
    for (unsigned j = 0; j < 4; ++j) {
      const double sim = o.tick[j + 1] - o.tick[j];
      total += sim, ptotal += p.target_ticks[j];
      CHECK(std::fabs(p.target_ticks[j] - want[j]) < 1e-5 && !p.clamped[j], "F1-C3 pair %u planned %.3f == approved %.0f, no clamp", j + 1, p.target_ticks[j], want[j]);
      CHECK(std::fabs(sim - want[j]) <= 1.0 + 1e-6, "F1-C3 pair %u simulated %.0f vs %.0f (quantization <= 1)", j + 1, sim, want[j]);
      std::printf("FIX08 F1-C3 pair %u-%u: planned %.2f t, simulated %.0f t (%.3f s)\n", j + 1, j + 2, p.target_ticks[j], sim, sim / 30);
    }
    CHECK(std::fabs(ptotal - 189.0) < 1e-4 && std::fabs(total - 189.0) <= 2.0, "F1-C3 total 189 ticks = 6.30 s (planned %.2f, simulated %.0f)", ptotal, total);
    CHECK(p.seg[0].cut == 61.0f && p.seg[1].start == 16.0f && p.seg[1].morph == 8.0f && std::fabs(p.seg[0].rate_out - 1.5f) < 1e-4,
          "pair 1: lateral cut 61 recovered at 1.50, vertical from 16 (10 frames of lift + smash at 1.0), morph 8 (got %.1f / %.1f / %.1f / %.3f)", p.seg[0].cut, p.seg[1].start,
          p.seg[1].morph, p.seg[0].rate_out);
    CHECK(kF1C3Pair1RateMax == kF1RecoveryRateMax && p.seg[0].rate_out <= kF1RecoveryRateMax, "no recovery-rate exception any more (FIX07 1.76)");
    CHECK(p.seg[1].rate_in == 1.0f && info(Move::F1B).impact[0] - p.seg[1].start == 10.0f && p.seg[1].morph < info(Move::F1B).impact[0] - p.seg[1].start,
          "the vertical's wind-up and swing at the native rate; the blend (8) ends before the contact (10 frames): pure native frames at the impact");
    for (unsigned m = 1; m < 5; ++m)
      CHECK((m == 1 || (p.seg[m].start == p7.seg[m].start && p.seg[m].morph == p7.seg[m].morph)) && p.seg[m].cut == p7.seg[m].cut && p.seg[m].rate_in == p7.seg[m].rate_in &&
                std::fabs(p.seg[m].rate_out - p7.seg[m].rate_out) < 1e-6,
            "F1-C3 segment %u: everything but the first pair's start / morph identical to FIX07", m);
    CHECK(p.seg[0].start == 0.0f && p.seg[0].rate_in == 1.0f && p.seg[4].cut < 0.0f, "first impact and the natural end untouched");
    for (unsigned i = 0; i < kPlans; ++i)
      if (i != kF1C3) CHECK(f7_plan_fnv(plan(combo_of(i), i)) == kFix06PlanFnv[i], "%s untouched by FIX08", combo_of(i).name);
    const OppOut op = opportunities(c, p);
    CHECK(op.n == 5, "F1-C3 five impact windows");
    for (unsigned i = 0; i < op.n; ++i) CHECK(op.imp[i].open_ticks >= 5 && op.imp[i].impact_inside && op.imp[i].separated, "F1-C3 impact %u: its own AT window / ticket", i + 1);
  }
  // ---------------------------------------------------------------- C. FIX08-01: the phase-1 Helm Splitter recovery looks for Link
  {
    // cLib_addCalcAngleS(4, 0x400, 16) replicated, wrap-around included
    CHECK(turn_step(0, 0x8000 - 1) == 0x400 && turn_step(0, -0x7FFF) == -0x400 && turn_step(0, 0x40) == 16 && turn_step(0, 8) == 8 && turn_step(0, -8) == -8 && turn_step(100, 100) == 100,
          "turn step: capped 0x400, divided by 4 in between, the 16 minimum, never past the target");
    CHECK(turn_step(0x7F00, -0x7F00) == static_cast<std::int16_t>(0x7F00 + 0x80) && turn_step(-0x7F00, 0x7F00) == static_cast<std::int16_t>(-0x7F00 - 0x80), "turn across +-180: the short way");
    for (int start : {0x7FFF, 0x6000, 0x4000, 0x2000, -0x2000, -0x4000, -0x7FFF}) {
      std::int16_t a = static_cast<std::int16_t>(start);
      unsigned t = 0;
      int prev_err = 70000;
      bool mono = true;
      while (a != 0 && t < 200) {
        a = turn_step(a, 0);
        const int e = a < 0 ? -a : a;
        mono = mono && e <= prev_err, prev_err = e, ++t;
      }
      CHECK(a == 0 && mono && t <= 60, "turn from %d converges monotonically in %u ticks (no snap: <= 0x400 per tick)", start, t);
    }
    HelmLook h;
    CHECK(!h.on() && !h.neck(10), "off: nothing");
    h.begin(1000);
    CHECK(h.on() && !h.neck(1000) && !h.neck(1000 + policy::kHelmLookDelayTicks - 1) && h.neck(1000 + policy::kHelmLookDelayTicks), "the hit reads first (%u ticks), then the neck follows",
          policy::kHelmLookDelayTicks);
    CHECK(!h.turn(9.0f) && !h.turn(23.9f) && h.turn(24.0f) && h.turn(41.0f), "the body turns only once the head is back over the shoulders (frame >= 24)");
    h.recovered_at(1030);
    CHECK(h.tail(1030) && h.tail(1030 + policy::kHelmLookTailTicks) && !h.tail(1031 + policy::kHelmLookTailTicks) && !h.turn(30.0f), "tail after the recovery, never a turn there");
    h.end();
    CHECK(!h.on() && h.runs == 1 && h.follows == 1 && h.tails == 1 && h.ends == 1, "one run, cleanly ended");
    // the five F1 Helm Splitter recoveries of the human log (contact frames 9.0 / 12.0 / 8.6 / 9.5 / 9.4, recovery at 1.25): the body may turn after (24 - f) / 1.25 ticks
    for (float f : {9.0f, 12.0f, 8.6f, 9.5f, 9.4f}) {
      const float t = (policy::kHelmTurnFromFrame - f) / kRecoverRate, left = (native::kEndTnaDamageR - policy::kHelmTurnFromFrame) / kRecoverRate;
      CHECK(t >= policy::kHelmLookDelayTicks && left >= 10.0f, "log contact at frame %.1f: neck after %u ticks, body turn after %.1f ticks, %.1f ticks of turn before the clip ends", f,
            policy::kHelmLookDelayTicks, t, left);
    }
  }
  // ---------------------------------------------------------------- D. FIX08-04: the defensive evasion
  {
    CHECK(policy::kEvadeOrdinalPct[0] == 0 && policy::kEvadeOrdinalPct[1] == 10 && policy::kEvadeOrdinalPct[2] == 15 && policy::kEvadeOrdinalPct[3] == 20 &&
              policy::kEvadeOrdinalPct[4] == 20 && policy::kEvadeOrdinalPct[5] == 20 && policy::kEvadeCooldownTicks == 90,
          "approved: 0 / 10 / 15 / 20+ %%, 90-tick cooldown between starts");
    CHECK(policy::kParryOrdinalPct[3] == 25 && policy::kParryOrdinalPct[4] == 45 && policy::kParryOrdinalPct[5] == 60 && policy::kParryOrdinalPct[0] == 0, "parry ordinals unchanged");
    // conditional distribution, 100 000 eligible swings per ordinal, no cooldown in between (each on a fresh actor clock)
    for (unsigned ord = 1; ord <= 6; ++ord) {
      Evade e;
      e.rng.seed(0xF1808ull + ord);
      unsigned won = 0;
      for (unsigned k = 1; k <= 100000; ++k) {
        EvadeView v;
        v.ordinal = ord, v.key = k, v.now = static_cast<std::uint64_t>(k) * 1000;
        const Ev r = e.decide(v);
        if (r == Ev::Won) ++won, e.started(v.now, Ed::Right);
      }
      const double pct = won / 1000.0, want = policy::kEvadeOrdinalPct[ord - 1];
      std::printf("FIX08 evade ordinal %u: %.2f %% (approved %.0f %%)\n", ord, pct, want);
      CHECK(std::fabs(pct - want) < 0.5 && (ord != 1 || won == 0) && e.rolls[ord - 1] == (ord == 1 ? 0u : 100000u), "ordinal %u: %.2f %% vs %.0f %%, the first swing never rolls", ord, pct,
            want);
    }
    Evade e;
    e.rng.seed(7);
    EvadeView v;
    v.ordinal = 4, v.key = 11, v.now = 500;
    const Rng before = e.rng;
    EvadeView bad = v;
    bad.eligible = false;
    CHECK(e.decide(bad) == Ev::NotEligible && e.rng.s == before.s && e.rolled_key == 0, "an ineligible attempt consumes neither the key nor the RNG");
    bad = v, bad.phase2 = false;
    CHECK(e.decide(bad) == Ev::NotEligible && e.rolled_key == 0, "phase 1 never evades (no native phase-1 jump)");
    const Ev first = e.decide(v);
    CHECK(first != Ev::NotEligible && e.decide(v) == Ev::Rerolled && e.rerolls_refused == 1, "ONE decision per swing key (never a reroll per tick / collider)");
    e.started(500, Ed::Left);
    for (std::uint64_t k = 12; k < 400; ++k) {
      v.key = k, v.now = 500 + (k - 11) * 7;  // a new swing every 7 ticks
      const Ev r = e.decide(v);
      if (v.now < 500 + policy::kEvadeCooldownTicks) CHECK(r == Ev::Cooldown, "swing at +%llu inside the 90-tick cooldown: no evasion", static_cast<unsigned long long>(v.now - 500));
      if (r == Ev::Won) {
        CHECK(v.now >= e.last_start + policy::kEvadeCooldownTicks, "two starts >= 90 ticks apart");
        e.started(v.now, Ed::Back);
      }
    }
    // native paths: side 11 / 14 and back 6, Darknut at the origin facing +z, Link 250 ahead
    const EvPath L = evade_path(Ed::Left, 0, 250, 0), R = evade_path(Ed::Right, 0, 250, 0), B = evade_path(Ed::Back, 0, 250, 0);
    CHECK(L.n > 9 && R.n > 9 && B.n > 9 && L.air == 9 && R.air == 9 && B.air == 9, "native take-off at frame 4, 9 airborne ticks (side %u / back %u samples)", L.n, B.n);
    CHECK(std::fabs(B.dist - 450.0f) < 2.0f && B.z[B.n - 1] < -440.0f, "back jump 450 units straight away from Link (%.1f)", B.dist);
    CHECK(L.x[L.n - 1] > 150.0f && R.x[R.n - 1] < -150.0f, "left = +x of the Darknut's frame (shape + 0x4000), right = -x (%.0f / %.0f)", L.x[L.n - 1], R.x[R.n - 1]);
    CHECK(L.dist > 230.0f && L.dist < 345.0f && std::fabs(L.dist - R.dist) < 1.0f, "side jump: an arc around Link (chord %.0f), symmetric", L.dist);
    {  // the native speeds of executeChaseL 11 / 14 and 12 / 15, step by step: 45, 40, 35, 33 x6 in the air, then the landing 15, 8, 1 = 342 units of path
      float arc = 0, px = 0, pz = 0;
      for (unsigned i = 0; i < L.n; ++i) arc += std::sqrt((L.x[i] - px) * (L.x[i] - px) + (L.z[i] - pz) * (L.z[i] - pz)), px = L.x[i], pz = L.z[i];
      CHECK(std::fabs(arc - 342.0f) < 1.0f, "side jump path length %.1f == 342 (native speeds)", arc);
      float barc = 0;
      px = pz = 0;
      for (unsigned i = 0; i < B.n; ++i) barc += std::sqrt((B.x[i] - px) * (B.x[i] - px) + (B.z[i] - pz) * (B.z[i] - pz)), px = B.x[i], pz = B.z[i];
      CHECK(std::fabs(barc - 450.0f) < 1.0f, "back jump path length %.1f == 450 (50 x 9)", barc);
    }
    float closest = 1e9f;
    for (unsigned i = 0; i < L.n; ++i) {
      const float dx = L.x[i] - 0, dz = L.z[i] - 250;
      const float d2 = dx * dx + dz * dz;
      closest = d2 < closest ? d2 : closest;
    }
    CHECK(closest > 200.0f * 200.0f, "the side arc never crosses Link (closest %.0f)", static_cast<double>(closest));
    CHECK(evade_mode(Ed::Left) == 14 && evade_mode(Ed::Right) == 11 && evade_mode(Ed::Back) == 6 && evade_mode(Ed::None) == -1, "the native modes checkNextMove itself draws for Link's attacks");
    CHECK(native_evade_entry(native::kActChaseL, 6) && native_evade_entry(native::kActChaseL, 11) && native_evade_entry(native::kActChaseL, 14) &&
              !native_evade_entry(native::kActChaseL, 10) && !native_evade_entry(native::kActChaseL, 13) && !native_evade_entry(native::kActChaseL, 8) && !native_evade_entry(native::kActGuardL, 6),
          "native evasion entries taken back: 6 / 11 / 14 (sword pressure); 10 / 13 (projectiles) stay native; the leaps are FIX02 / FIX07's");
    CHECK(evade_jump_mode(11) && evade_jump_mode(12) && evade_jump_mode(14) && evade_jump_mode(15) && evade_jump_mode(6) && evade_jump_mode(7) && !evade_jump_mode(1) && !evade_jump_mode(8),
          "the evasion lease follows take-off, flight and the native landing");
    // direction
    CHECK(evade_pick(120, 0, true, true, true, true) == Ed::Back && evade_pick(120, 0, true, true, true, false) == Ed::Right, "very close: back first, else a side");
    CHECK(evade_pick(250, 0x800, false, true, true, true) == Ed::Right && evade_pick(250, -0x800, true, true, true, true) == Ed::Left, "away from Link's side");
    CHECK(evade_pick(250, 0x800, false, true, false, true) == Ed::Left && evade_pick(250, 0x800, false, false, false, true) == Ed::Back && evade_pick(250, 0x800, false, false, false, false) == Ed::None,
          "unsafe side: the other side, then back, none = the ordinary defense (no reroll)");
    CHECK(leap_hold_after_evade(1000, 990, false) && !leap_hold_after_evade(1000, 990, true) && !leap_hold_after_evade(990 + policy::kEvadeLeapHoldTicks, 990, false) &&
              !leap_hold_after_evade(1000, 0, false),
          "the distance the evasion created is not Link's retreat: no leap roll until he came close or 90 ticks passed");
    for (int s : {0, 0x2000, -0x2000, 0x7000})
      for (float lx : {-200.0f, 0.0f, 150.0f}) {
        const EvPath q = evade_path(Ed::Right, lx, 260, static_cast<std::int16_t>(s));
        CHECK(q.n >= 10 && q.n <= 40 && q.dist > 150.0f, "path bounded (facing %d, Link x %.0f): %u samples, %.0f", s, lx, q.n, q.dist);
      }
  }
  // ---------------------------------------------------------------- E. FIX08-07: the swing threat and the DEFENSE -> NEUTRAL -> ORDINARY_INIT receipt
  {
    CHECK(swing_threat(true, true, 331, 0x20, 11), "log 14602: a FINISH_STAB 11 ticks old at 331, in front: a running swing that connects (it did, at 14604)");
    CHECK(!swing_threat(true, true, 331, 0x01, 11) && swing_threat(true, true, 280, 0x01, 11), "an ordinary vertical cut connects within 300 only");
    CHECK(!swing_threat(true, true, 331, 0x20, policy::kSwingActiveTicks + 1) && !swing_threat(false, true, 100, 0x01, 1) && !swing_threat(true, false, 100, 0x01, 1),
          "an old swing (recovery frames) / no cut / behind: no threat");
    CHECK(!swing_threat(true, true, 460, 0x0A, 5) && swing_threat(true, true, 440, 0x0A, 5), "a jump attack connects within 450");
    Initiative ini;
    ini.rng.seed(3);
    InitView iv;
    iv.phase2 = true, iv.dist = 300, iv.now = 1000, iv.link_swing = true;
    const std::uint64_t opp = ini.opportunities;
    const Decision d = ini.decide(iv);
    CHECK(d.pick == Pick::None && d.why == NoInit::LinkSwinging && ini.opportunities == opp, "an ordinary initiative never starts over that swing (and it is no opportunity)");
    iv.link_swing = false;
    CHECK(ini.decide(iv).pick == Pick::Combo, "the same moment without the swing: the ordinary initiative");
    NeutralGate g;
    CHECK(g.ready(10, false) && g.ready(10, true), "nothing pending: free");
    g.defense(100, 1);
    CHECK(!g.ready(101, false) && !g.ready(102, true), "after a defense: the guard clip never counts, the first neutral tick neither");
    CHECK(!g.ready(102 + policy::kGuardLeaveTicks - 1, true) && g.ready(102 + policy::kGuardLeaveTicks, true) && g.receipts == 1, "neutral chase for %u ticks -> the receipt",
          policy::kGuardLeaveTicks);
    g.defense(300, 4);
    CHECK(!g.ready(301, true) && !g.ready(305, false) && !g.ready(306, true) && !g.ready(306 + policy::kGuardLeaveTicks - 1, true) && g.ready(306 + policy::kGuardLeaveTicks, true),
          "a non-neutral tick (a jump / guard) restarts the neutral count");
    CHECK(std::string_view(nr_cause(4)) == "EVASION" && std::string_view(name(NoInit::LinkSwinging)) == "LINK_SWING_ACTIVE", "names");
  }
  // ---------------------------------------------------------------- F. FIX08-09: the B1 recovery (constants; the native morph is measured by morph_sim.py)
  {
    CHECK(policy::kB1RecoverBck == 0x2C && policy::kB1RecoverMorph < 0.0f && policy::kB1RecoverMorph == -20.0f && policy::kB1RecoverTicks == 18,
          "B1 ends into TNB_WAIT_B_2 with the native eased morph -20, the native wait 18 ticks (was TNB_WAIT morph 10, 30)");
    const MoveInfo& b1 = info(Move::F2B1);
    CHECK(b1.impacts == 1 && b1.impact[0] == 15.5f && b1.at_open[0] == 13.0f && b1.at_close[0] == 20.0f && b1.c_min == 20.0f && b1.c_max == 20.0f && b1.link_block == 23 && b1.link_direct == 33,
          "B1 itself unchanged: one cut, AT 13..20, cut at 20 (never the second cut)");
    const Plan p = plan(kCombos[4], 4);
    CHECK(p.seg[3].m == Move::F2B1 && p.seg[3].cut == 20.0f && kCombos[4].n_impacts == 7, "F2-C1 keeps its 7 impacts and its B1 at 20");
  }
  // ---------------------------------------------------------------- G. FIX08-05: the sword's darkening value (Darknut only; the write route is FIX07's entryDL)
  {
    CHECK(sword_tint_value(0.0f) == 0 && sword_tint_value(-0.3f) == 0 && sword_tint_value(-0.9f) == 0 && sword_tint_value(-1.0f) == -3 && sword_tint_value(3.0f) == 0,
          "steps with the body's own s16 (the body's C0 is 0 for -0.3: so is the sword's)");
    CHECK(sword_tint_value(-13.5f) == -32 && sword_tint_value(-26.7f) == -64 && sword_tint_value(-50.0f) == -100 && sword_tint_value(-80.0f) == -100,
          "F3 Finish end (-13.5) -> -32, HP0 end (~-26.7) -> -64, bounded at -100 (never a black-out, alpha untouched)");
    short prev = 1;
    for (int b = 0; b >= -60; --b) {
      const short v = sword_tint_value(static_cast<float>(b));
      CHECK(v <= 0 && v <= prev && v >= -100 && (b == 0 || v < static_cast<short>(b) + 1), "monotonic, never lighter than the body's C0 (%d -> %d)", b, v);
      prev = v;
    }
    CHECK(sword_tint(true, -0.3f) && !sword_tint(false, -13.0f) && !sword_tint(true, 0.0f), "still only the definitive death of THIS Darknut");
  }
}
