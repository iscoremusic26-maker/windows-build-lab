// LIGHT FADES TWICE 0.13.18 DKN01 FIX09: the dkn_lab section of the FIX09 deltas (included by tests/dkn_lab.cpp after the FIX08 section; same CHECK).
// Pure core only: the caution band R = min(H, Q) and its exact borders, Table B by band, Table A, the back-first reach by band, the evasion's 6-tick i-frame window
// (take-off, executed ticks, no re-arm, cancel), the same-swing continuation, the crossing verdict, the phase-2/3 front rule, the voluntary end after a confirmed
// life hit (route per move / impact), the one post-hit decision (no roll when ineligible, one roll otherwise, no reroll on geometry), Link's bounded reaction gate,
// the native evasion entry written by an action(), and adversarial sequences (jump-attack spam, forced evasions, retreat + evasion cooldown sharing, long fights).
// What a player sees / hears is HUMAN_RUNTIME_PENDING.
static void fix09_section() {
  std::printf("\n==== FIX09 (0.13.18) ====\n");
  CHECK(std::string_view(kVersion) == "0.13.18" && std::string_view(kTag) == "DKN01_FIX09", "FIX09 version / tag");
  // ---------------------------------------------------------------- A. R = min(H, Q): exact borders in integer units, the worse of the two, every stage maximum
  {
    const long long U = kUnits;
    // H borders (posture 0): 75 % exactly belongs to the lower band
    CHECK(caution_band(361, 480, 0, 160) == 0 && caution_band(360, 480, 0, 160) == 1, "H 361/480 normal, 360/480 (=75%%) moderate");
    CHECK(caution_band(241, 480, 0, 160) == 1 && caution_band(240, 480, 0, 160) == 2, "H 241/480 moderate, 240/480 (=50%%) high");
    CHECK(caution_band(121, 480, 0, 160) == 2 && caution_band(120, 480, 0, 160) == 3, "H 121/480 high, 120/480 (=25%%) critical");
    CHECK(caution_band(0, 480, 0, 160) == 3 && caution_band(480, 480, 0, 160) == 0, "H 0 critical, full normal");
    CHECK(caution_band(451, 600, 0, 200) == 0 && caution_band(450, 600, 0, 200) == 1, "F1 600: 451 normal, 450 moderate");
    // Q borders (HP full): posture 40/160 = Q 75 % -> moderate; one unit less -> normal
    CHECK(caution_band(480, 480, 40 * U, 160) == 1 && caution_band(480, 480, 40 * U - 1, 160) == 0, "Q: posture 40/160 exactly = moderate, one unit less = normal");
    CHECK(caution_band(480, 480, 80 * U, 160) == 2 && caution_band(480, 480, 80 * U - 1, 160) == 1, "Q: 80/160 high");
    CHECK(caution_band(480, 480, 120 * U, 160) == 3 && caution_band(480, 480, 120 * U - 1, 160) == 2, "Q: 120/160 critical");
    CHECK(caution_band(480, 480, 45 * U, 180) == 1 && caution_band(480, 480, 45 * U - 1, 180) == 0, "F3 180: 45 moderate, just under normal");
    CHECK(caution_band(480, 480, 160 * U, 160) == 3, "posture saturated (break) = critical (eligibility gates decide, never the band)");
    // the worse of the two (never a sum, an average or a product)
    CHECK(caution_band(400, 480, 100 * U, 160) == 2, "H 83%% / Q 37.5%%: R = Q -> high");
    CHECK(caution_band(130, 480, 10 * U, 160) == 2, "H 27%% / Q 94%%: R = H -> high");
    CHECK(caution_band(300, 480, 50 * U, 160) == 1, "H 62.5%% / Q 68.75%%: moderate (an average would say normal-ish, a product high)");
    // clamping: out-of-range inputs never invert the meaning
    CHECK(caution_band(-5, 480, 0, 160) == 3 && caution_band(999, 480, 0, 160) == 0 && caution_band(480, 480, -U, 160) == 0 && caution_band(480, 480, 999 * U, 160) == 3, "clamped inputs");
    CHECK(caution_band(480, 0, 0, 160) == 3 && caution_band(480, 480, 0, 0) == 3, "a zero maximum never reads as healthy");
    // monotonic: less HP or more posture never lowers the band (exhaustive over HP and a posture grid)
    bool mono = true;
    for (int mx : {480, 600})
      for (int pm : {160, 180, 200})
        for (int hp = mx; hp > 0 && mono; --hp)
          for (int pp = 0; pp <= pm && mono; pp += 5) {
            const unsigned b = caution_band(hp, mx, pp * U, pm);
            mono = caution_band(hp - 1, mx, pp * U, pm) >= b && (pp + 5 > pm || caution_band(hp, mx, (pp + 5) * U, pm) >= b);
          }
    CHECK(mono, "the band is monotonic in HP loss and in posture gain (every stage maximum)");
  }
  // ---------------------------------------------------------------- B. Table B by band (approved) and Table A (approved)
  {
    const int want[4][6] = {{0, 10, 15, 20, 20, 20}, {0, 12, 18, 25, 25, 25}, {0, 15, 22, 30, 30, 30}, {0, 18, 27, 35, 35, 35}};
    bool ok = true;
    for (unsigned b = 0; b < 4; ++b)
      for (unsigned o = 1; o <= 6; ++o) ok = ok && evade_pct(b, o) == want[b][o - 1];
    CHECK(ok, "Table B: 0/10/15/20, 0/12/18/25, 0/15/22/30, 0/18/27/35 (4th and later share the 4+ column)");
    for (unsigned o = 1; o <= 6; ++o) CHECK(evade_pct(0, o) == policy::kEvadeOrdinalPct[o - 1], "band 0 = the FIX08 row exactly (ordinal %u)", o);
    for (unsigned b = 0; b < 4; ++b) CHECK(evade_pct(b, 1) == 0 && evade_pct(b, 0) == 0 && evade_pct(b, 99) == evade_pct(b, 4), "band %u: first swing 0 %%, ordinal clamped", b);
    bool mono = true;
    for (unsigned b = 0; b < 3; ++b)
      for (unsigned o = 1; o <= 6; ++o) mono = mono && evade_pct(b + 1, o) >= evade_pct(b, o);
    CHECK(mono, "Table B never decreases with a worse band");
    CHECK(retreat_pct(0) == 20 && retreat_pct(1) == 35 && retreat_pct(2) == 50 && retreat_pct(3) == 65 && retreat_pct(9) == 65, "Table A: 20 / 35 / 50 / 65 %% back");
    CHECK(policy::kEvadeIframeTicks == 6 && policy::kEvadeCooldownTicks == 90, "6 i-frame ticks, 90 ticks between maneuver starts");
    CHECK(policy::kEvadeBackReach[0] == policy::kEvadeBackDist && policy::kEvadeBackReach[0] < policy::kEvadeBackReach[1] && policy::kEvadeBackReach[1] < policy::kEvadeBackReach[2] &&
              policy::kEvadeBackReach[2] < policy::kEvadeBackReach[3] && policy::kEvadeBackReach[3] < policy::kGuardReach,
          "back-first reach: band 0 = FIX08 170, strictly increasing, always below the 300 roll reach (lateral evasions remain)");
  }
  // ---------------------------------------------------------------- C. the evasion roll by band: conditional distribution, one roll per swing, cooldown, RNG untouched when not eligible
  {
    const int want[4] = {20, 25, 30, 35};
    for (unsigned b = 0; b < 4; ++b) {
      Evade e;
      e.rng.seed(0xF19u + b);
      unsigned won = 0, rolls = 0;
      for (unsigned i = 1; i <= 100000; ++i) {
        EvadeView v;
        v.band = b, v.ordinal = 4, v.key = i, v.now = i * 1000ull;  // cooldown never blocks here
        const Ev r = e.decide(v);
        rolls += r == Ev::Won || r == Ev::Lost;
        won += r == Ev::Won;
      }
      const double pct = 100.0 * won / rolls;
      std::printf("FIX09 Table B band %u ordinal 4+: %u/%u won = %.2f %% (approved %d)\n", b, won, rolls, pct, want[b]);
      CHECK(rolls == 100000 && std::fabs(pct - want[b]) < 0.6, "band %u ordinal 4+: %.2f %% ~ %d %%", b, pct, want[b]);
    }
    for (unsigned b = 0; b < 4; ++b) {  // ordinals 2 / 3 too
      for (unsigned o = 2; o <= 3; ++o) {
        Evade e;
        e.rng.seed(0xB0Bu * (b + 1) + o);
        unsigned won = 0;
        for (unsigned i = 1; i <= 60000; ++i) {
          EvadeView v;
          v.band = b, v.ordinal = o, v.key = i, v.now = i * 1000ull;
          won += e.decide(v) == Ev::Won;
        }
        const double pct = 100.0 * won / 60000.0;
        CHECK(std::fabs(pct - evade_pct(b, o)) < 0.8, "band %u ordinal %u: %.2f %% ~ %d %%", b, o, pct, evade_pct(b, o));
      }
    }
    Evade e;
    e.rng.seed(77);
    EvadeView v;
    v.band = 3, v.ordinal = 6, v.key = 5, v.now = 1000;
    const Ev first = e.decide(v);
    v.band = 0;  // the band changed between two reads of the SAME swing: never a second roll
    CHECK(e.decide(v) == Ev::Rerolled, "one roll per swing key, even when the band changes on the boundary");
    Evade q;
    q.rng.seed(5);
    const std::uint64_t s0 = q.rng.s;
    v.key = 9, v.eligible = false;
    CHECK(q.decide(v) == Ev::NotEligible && q.rng.s == s0 && q.rolled_key == 0, "an ineligible swing consumes neither the key nor the RNG");
    (void)first;
    Evade c;
    c.rng.seed(1);
    c.started(1000, Ed::Back);
    EvadeView w;
    w.band = 3, w.ordinal = 6, w.key = 11, w.now = 1089;
    CHECK(c.decide(w) == Ev::Cooldown, "tick 89 after a start: cooldown (also in the critical band: never shortened)");
    w.key = 12, w.now = 1090;
    CHECK(c.decide(w) != Ev::Cooldown, "tick 90: free again");
  }
  // ---------------------------------------------------------------- D. the back-first reach by band (the FIX08 selector, distance shifted; no new probability)
  {
    bool same = true, mono = true, safe = true;
    for (int d = 0; d <= 300; d += 3)
      for (int side : {-0x3000, -1, 0, 1, 0x3000})
        for (unsigned m = 0; m < 8; ++m) {
          const bool l = m & 1, r = m & 2, bk = m & 4;
          same = same && evade_pick_band(0, float(d), side, true, l, r, bk) == evade_pick(float(d), side, true, l, r, bk);
          for (unsigned b = 0; b < 3; ++b)
            if (evade_pick_band(b, float(d), side, true, l, r, bk) == Ed::Back) mono = mono && evade_pick_band(b + 1, float(d), side, true, l, r, bk) == Ed::Back;
          for (unsigned b = 0; b < 4; ++b) {
            const Ed x = evade_pick_band(b, float(d), side, true, l, r, bk);
            safe = safe && (x == Ed::None || (x == Ed::Left && l) || (x == Ed::Right && r) || (x == Ed::Back && bk)) && (x != Ed::None || (!l && !r && !bk));
          }
        }
    CHECK(same, "band 0 picks exactly as FIX08");
    CHECK(mono, "a back pick stays a back pick in every worse band (monotonic preference)");
    CHECK(safe, "the pick never chooses an unsafe direction; none safe = no evasion");
    CHECK(evade_pick_band(3, 240.0f, 0x1000, true, true, true, true) == Ed::Back && evade_pick_band(0, 240.0f, 0x1000, true, true, true, true) == Ed::Right &&
              evade_pick_band(3, 260.0f, 0x1000, true, true, true, true) == Ed::Right,
          "critical at 240: back; normal at 240: side; critical at 260: side (not every jump goes back)");
  }
  // ---------------------------------------------------------------- E. the i-frame window: 6 executed ticks from the take-off, never re-armed, closed by the end
  {
    Iframe f;
    f.begin(1);
    std::uint64_t prot[64] = {};
    unsigned n = 0;
    for (std::uint64_t t = 100; t < 140; ++t) {
      const bool takeoff = t >= 104;  // the native take-off speed seen from tick 104 on (and every later tick: speed stays > 0)
      if (f.tick(t, takeoff, true)) prot[n++] = t;
      CHECK(f.covers(t) == (t >= 104 && t < 110), "tick %llu covered = %d (window [104,110))", static_cast<unsigned long long>(t), f.covers(t) ? 1 : 0);
    }
    CHECK(n == 6 && prot[0] == 104 && prot[5] == 109, "exactly 6 protected passes: the take-off tick and the 5 after it");
    CHECK(!f.covers(103) && f.covers(110) == false && f.closed_full == 1, "-1 and +6 are vulnerable; the window closed full once");
    // a take-off seen again (B_1 -> B_2, a clip restart, a repeated hook) never re-arms the same token
    for (std::uint64_t t = 140; t < 160; ++t) CHECK(!f.tick(t, true, true), "no re-arm on tick %llu", static_cast<unsigned long long>(t));
    // pause / hitstop: only executed ticks count (the actor not running = its spheres are not registered either)
    Iframe p;
    p.begin(2);
    unsigned m = 0;
    const std::uint64_t exec_ticks[] = {200, 201, 204, 205, 206, 210, 211, 212, 213};
    for (std::uint64_t t : exec_ticks) m += p.tick(t, true, true);
    CHECK(m == 6 && p.covers(213) == false && !p.covers(212), "6 EXECUTED ticks across a 2-tick and a 3-tick pause");
    // cancelled before the take-off: 0 ticks; cancelled during: closes at once
    Iframe c;
    c.begin(3);
    CHECK(!c.tick(300, false, true) && !c.tick(301, false, true) && !c.tick(302, true, false) && c.opened == 0 && c.never_opened == 1, "cancelled before take-off: no i-frame");
    Iframe d;
    d.begin(4);
    CHECK(d.tick(400, true, true) && d.tick(401, true, true) && !d.tick(402, true, false) && !d.covers(402) && d.closed_early == 1, "cancelled during: closed at once (2 ticks used)");
    // a stale pass never matches a later tick (a POST that did not run leaves nothing open for the future)
    Iframe e;
    e.begin(5);
    e.tick(500, true, true);
    CHECK(e.covers(500) && !e.covers(501) && !e.covers(502), "the window covers only the pass of the tick that counted it");
    // a new maneuver = a new token = its own window (never stacked: the cooldown is 90 >> 6)
    e.end();
    e.begin(6);
    unsigned k = 0;
    for (std::uint64_t t = 600; t < 620; ++t) k += e.tick(t, true, true);
    CHECK(k == 6, "the next maneuver has its own 6 ticks");
  }
  // ---------------------------------------------------------------- F. the same swing crossing on after the window, the crossing verdict, the front rule
  {
    IfHeld h;
    h.note(42, 1000, 7);
    CHECK(h.continues(42, 1000, 7) && h.continues(42, 1001, 7), "the same swing in the same / next pass: still evaded");
    CHECK(!h.continues(42, 1002, 7) && !h.continues(43, 1001, 7) && !h.continues(42, 1001, 8), "a gap, a NEW swing or another maneuver: native (a new contact may hit)");
    XIn x;
    CHECK(cross_verdict(x) == Xv::Native, "nothing of Link's melee: native");
    x.melee = true, x.combat = true, x.phase2 = true;
    CHECK(cross_verdict(x) == Xv::Native, "a sword crossing outside every window: native (damage / block by the rules)");
    x.window = true;
    CHECK(cross_verdict(x) == Xv::Evaded, "inside the window: evaded (no hit, no mark, no clash)");
    x.window = false, x.held = true;
    CHECK(cross_verdict(x) == Xv::EvadedHeld, "the same swing right after: evaded");
    x.held = false, x.shield_attack = true;
    CHECK(cross_verdict(x) == Xv::NoFalseGuard, "phases 2-3: Link's rejected Shield Attack never crosses (no clank, no forced guard)");
    x.helm_route = true;
    CHECK(cross_verdict(x) == Xv::Native, "a NEW Shield Attack on the first stagger: native (the Helm Splitter preparation)");
    x.helm_route = false, x.phase2 = false;
    CHECK(cross_verdict(x) == Xv::Native, "phase 1: the shield is physical: native");
    x.phase2 = true, x.combat = false;
    CHECK(cross_verdict(x) == Xv::Native, "not in combat (execution / transformation): native");
    CHECK(guard_needs_front_clear(true, true, false, false), "F2/F3 guard-ready, Link at the flank / back, no parry timer: the guard is cleared (damage)");
    CHECK(!guard_needs_front_clear(true, true, true, false) && !guard_needs_front_clear(true, true, false, true) && !guard_needs_front_clear(false, true, false, false) &&
              !guard_needs_front_clear(true, false, false, false),
          "in front / armed parry window / phase 1 / not guard-ready: nothing changes");
  }
  // ---------------------------------------------------------------- G. the voluntary end after a confirmed life hit: the route of every impact of every plan
  {
    CHECK(hit_end_route(Move::F2B, 0, Move::F2B, true, false) == He::EasedRecover && hit_end_route(Move::F2B, 0, Move::F2B, true, true) == He::EasedRecover,
          "the double's FIRST cut hurt Link: its second cut never plays (also as the plan's last move)");
    CHECK(hit_end_route(Move::F2B, 1, Move::F2B, true, false) == He::Tail && hit_end_route(Move::F2A, 0, Move::F2A, true, false) == He::Tail &&
              hit_end_route(Move::F2C, 0, Move::F2C, true, false) == He::Tail && hit_end_route(Move::F1A, 0, Move::F1A, true, false) == He::Tail,
          "second cut / thrust / kick / F1 strike in the middle of a plan: the move's own follow-through");
    CHECK(hit_end_route(Move::F2A, 0, Move::F2A, true, true) == He::LetPlanEnd && hit_end_route(Move::F2B1, 0, Move::F2B1, true, true) == He::LetPlanEnd,
          "the plan's last move (B1 included): ends exactly as planned, never a second recovery");
    CHECK(hit_end_route(Move::F2A, 0, Move::F2B, false, false) == He::EasedRecover && hit_end_route(Move::F1B, 0, Move::F1A, false, false) == He::NeutralF1,
          "a later segment already started: sword phases ease back, phase 1 to its native chase (no TNB clip in F1)");
    // every impact of every combo and plan: after the end NO further window of the plan can open (the hit impact is the move's last unresolved window, or the cut 2 is skipped)
    unsigned checked = 0;
    bool ok = true;
    for (unsigned pi = 0; pi < kPlans; ++pi) {
      const Plan p = plan(combo_of(pi), pi);
      for (unsigned sg = 0; sg < p.n; ++sg) {
        const MoveInfo& mi = info(p.seg[sg].m);
        for (unsigned kx = 0; kx < mi.impacts; ++kx) {
          const He r = hit_end_route(p.seg[sg].m, kx, p.seg[sg].m, true, sg + 1 >= p.n);
          const bool more_in_move = kx + 1 < mi.impacts;  // a later window of the same move
          ok = ok && r != He::None && (!more_in_move || r == He::EasedRecover);
          ++checked;
        }
      }
    }
    CHECK(ok && checked >= 46, "every one of %u impacts (46 combo impacts + singles / counters): the end never leaves a later window of its move armed", checked);
    // the double counter / combo double keeps BOTH cuts when nothing hurt Link: the route only exists after a confirmed receipt (adapter): the plan is unchanged
    CHECK(info(Move::F2B).impacts == 2 && window_of(Move::F2B, 25.0f) == 1, "the double still has its two windows (a block / parry / evade continues the plan)");
  }
  // ---------------------------------------------------------------- H. the ONE post-hit decision (Table A): no roll when ineligible, one roll, no reroll on geometry
  {
    PostHit p;
    p.rng.seed(9);
    std::uint64_t s0 = p.rng.s;
    unsigned asked = 0;
    auto never = [&]() noexcept { return ++asked, true; };
    CHECK(p.decide(false, true, 3, never) == Ph9::HoldF1 && p.rng.s == s0 && asked == 0, "F1: hold, no roll, no geometry read");
    CHECK(p.decide(true, false, 3, never) == Ph9::HoldIneligible && p.rng.s == s0 && asked == 0, "not eligible (cooldown / event / ...): hold, no roll");
    unsigned backs = 0, geo = 0;
    for (unsigned b = 0; b < 4; ++b) {
      PostHit q;
      q.rng.seed(0xAB0u + b);
      unsigned back = 0;
      for (unsigned i = 0; i < 100000; ++i) back += q.decide(true, true, b, []() noexcept { return true; }) == Ph9::Back;
      const double pct = back / 1000.0;
      std::printf("FIX09 Table A band %u: back %.2f %% (approved %d)\n", b, pct, retreat_pct(b));
      CHECK(std::fabs(pct - retreat_pct(b)) < 0.6 && q.rolls == 100000, "Table A band %u: %.2f %% back ~ %d %%, one roll each", b, pct, retreat_pct(b));
      backs += back;
    }
    PostHit g;
    g.rng.seed(3);
    for (unsigned i = 0; i < 2000; ++i) {
      const std::uint64_t before = g.rng.s;
      const Ph9 v = g.decide(true, true, 3, []() noexcept { return false; });
      geo += v == Ph9::HoldGeometry;
      CHECK(v == Ph9::HoldGeometry || v == Ph9::HoldRoll, "unsafe back path: hold (no lateral / forward substitute)");
      CHECK(g.rng.s != before, "exactly the one roll was consumed (never a reroll to find a jump)");
    }
    CHECK(geo > 1100 && geo < 1500, "critical band with an unsafe path: ~65 %% of decisions are geometry holds (%u/2000)", geo);
    // independence of the streams: the evasion stream is never consumed by the decision
    Evade e;
    e.rng.seed(5);
    const std::uint64_t es = e.rng.s;
    PostHit z;
    z.rng.seed(5);
    (void)z.decide(true, true, 2, []() noexcept { return true; });
    CHECK(e.rng.s == es, "the post-hit decision never touches the evasion stream");
    // the watchdog of a decision waiting for a recovery
    PostHit w;
    w.arm(1000, 7);
    CHECK(!w.expired(1000 + policy::kPostHitDecideMax) && w.expired(1001 + policy::kPostHitDecideMax), "the decision waits at most kPostHitDecideMax ticks");
    (void)backs;
  }
  // ---------------------------------------------------------------- I. Link's reaction gate, the initiative, the native entry written by an action(), the sequence end
  {
    CHECK(link_damage_proc(0x32) && link_damage_proc(0x33) && link_damage_proc(0x34) && link_damage_proc(0x159) && link_damage_proc(0x15A) && !link_damage_proc(0x1F) &&
              !link_damage_proc(0x1E) && !link_damage_proc(3),
          "Link's damage procedures (never his guard break / Shield Attack / wait)");
    CHECK(link_reacting(true, 1000, 950) && link_reacting(true, 1040, 950) && !link_reacting(true, 1041, 950) && !link_reacting(false, 960, 950) && !link_reacting(true, 960, 0),
          "the reaction gate is bounded (90 ticks after the receipt), needs a damage procedure and a receipt");
    Initiative ini;
    ini.rng.seed(1);
    InitView v;
    v.phase2 = true, v.dist = 200, v.now = 5000, v.link_reacting = true;
    CHECK(ini.decide(v).why == NoInit::LinkReacting, "no ordinary initiative into Link's reaction to this hit");
    v.link_reacting = false;
    CHECK(ini.decide(v).pick == Pick::Combo, "Link free: the ordinary initiative works again (no infinite hold)");
    CHECK(std::string_view(name(NoInit::LinkReacting)) == "LINK_REACTING_TO_HIT" && std::string_view(nr_cause(6)) == "POST_HIT_LINK_FREE", "names");
    CHECK(native_evade_written(9, 1, 9, 11) && native_evade_written(9, 4, 9, 14) && native_evade_written(9, 5, 9, 6) && native_evade_written(15, 10, 9, 14),
          "a native 6 / 11 / 14 written during the action (strafe or yoroke before it)");
    CHECK(!native_evade_written(9, 11, 9, 11) && !native_evade_written(9, 6, 9, 6) && !native_evade_written(9, 1, 9, 10) && !native_evade_written(9, 1, 9, 13) &&
              !native_evade_written(9, 1, 9, 8),
          "the module's own jump (already there before the action), the projectile evasions 10 / 13, the forward leap: not a native evasion entry");
    CHECK(seq_end_of("LINK_HP_HIT") == SqEnd::LinkHit && std::string_view(name(SqEnd::LinkHit)) == "LINK_HP_HIT_VOLUNTARY_END", "the sequence end of a voluntary end");
    Seq q;
    q.rng.seed(4);
    CHECK(q.begin(100, 99, false, 12) && q.started(false), "a counter sequence");
    const std::uint64_t rolls0 = q.cont_rolls;
    q.finish(SqEnd::LinkHit);
    CHECK(!q.owns() && q.cont_rolls == rolls0 && q.ended[unsigned(SqEnd::LinkHit)] == 1, "a counter that hurt Link: the sequence ends, the 80 / 20 continuation is never rolled");
  }
  // ---------------------------------------------------------------- J. adversarial sequences (pure policy): spam, forced evasions, retreat + evasion sharing one cooldown
  {
    // (a) jump-attack / swing spam every 4 ticks for 10 minutes at the critical band, ordinal saturated: starts never closer than 90, i-frames never more than 6 per start
    Evade e;
    e.rng.seed(0x5EED);
    Iframe f;
    std::uint64_t last_start = 0, starts = 0, prot = 0, min_gap = ~0ull, prot_in_90 = 0, max_prot_90 = 0;
    std::uint64_t win_start = 0;
    bool live = false;
    unsigned live_ticks = 0;
    for (std::uint64_t t = 1; t <= 18000; ++t) {
      if (t % 4 == 0 && !live) {
        EvadeView v;
        v.band = 3, v.ordinal = 6, v.key = t, v.now = t;
        if (e.decide(v) == Ev::Won) {
          if (starts) min_gap = t - last_start < min_gap ? t - last_start : min_gap;
          e.started(t, Ed::Back), f.begin(++starts), last_start = t, live = true, live_ticks = 0;
        }
      }
      if (live && ++live_ticks > 22) live = false;  // the native back jump lasts ~21 ticks
      const bool p = f.tick(t, live && live_ticks >= 5, live);
      prot += p;
      if (t - win_start >= 90) max_prot_90 = prot_in_90 > max_prot_90 ? prot_in_90 : max_prot_90, prot_in_90 = 0, win_start = t;
      prot_in_90 += p;
    }
    std::printf("FIX09 spam 10 min critical: %llu maneuvers, min gap %llu ticks, %llu protected ticks, max %llu per 90-tick window\n", static_cast<unsigned long long>(starts),
                static_cast<unsigned long long>(min_gap), static_cast<unsigned long long>(prot), static_cast<unsigned long long>(max_prot_90));
    CHECK(starts > 50 && min_gap >= 90 && prot == starts * 6 && max_prot_90 <= 12, "spam: never closer than 90, exactly 6 per maneuver, at most 2 windows in any 90 ticks");
    // (b) a retreat decision 30 ticks after an evasion start: the shared cooldown makes it ineligible (hold, no roll): no evade / retreat alternation every half second
    Evade c;
    c.started(1000, Ed::Left);
    PostHit p;
    p.rng.seed(1);
    const std::uint64_t s0 = p.rng.s;
    const bool eligible = 1030 >= c.cooldown_until;
    CHECK(!eligible && p.decide(true, eligible, 3, []() noexcept { return true; }) == Ph9::HoldIneligible && p.rng.s == s0, "evasion then retreat 30 ticks later: hold (common cooldown)");
    // (c) long fight: alternating bands at every swing, the conditional rate per band stays the approved one (no compensation, no drift)
    Evade d;
    d.rng.seed(0xC0FFEE);
    unsigned won[4] = {}, rolls[4] = {};
    for (unsigned i = 1; i <= 200000; ++i) {
      EvadeView v;
      v.band = i % 4, v.ordinal = 3, v.key = i, v.now = i * 100ull;
      const Ev r = d.decide(v);
      rolls[v.band] += r == Ev::Won || r == Ev::Lost, won[v.band] += r == Ev::Won;
    }
    for (unsigned b = 0; b < 4; ++b) CHECK(std::fabs(100.0 * won[b] / rolls[b] - evade_pct(b, 3)) < 0.7, "alternating bands, band %u ordinal 3 stays %d %%", b, evade_pct(b, 3));
  }
}
