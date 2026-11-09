"""LIZALFOS 0.12.2: complete positive clone, then targeted source/writer/policy negatives against the full source wall. Each must be REJECTED for its stated cause."""
from pathlib import Path
import shutil, tempfile, subprocess, sys, json
root = Path(__file__).resolve().parents[1]
def clone(p):
    for x in ['src', 'tests', 'ci']:
        shutil.copytree(root / x, p / x, ignore=shutil.ignore_patterns('__pycache__'))
def audit(p):
    r = subprocess.run([sys.executable, '-I', str(p / 'ci/source_audit.py'), str(p)], capture_output=True, text=True, timeout=180)
    return r, json.loads(r.stdout)
with tempfile.TemporaryDirectory() as td:
    p = Path(td)
    clone(p)
    r, d = audit(p)
    assert r.returncode == 0 and d['status'] == 'SOURCE_AUDIT_PASS', (r.stdout[-3000:], r.stderr[-1000:])
A = 'src/liz_adapter.inc'; C = 'src/liz_core.hpp'; M = 'src/mod.cpp'; L = 'src/lnk_core.hpp'
Ins = "  liz_apply(a, s->b.on_attack_pre(liz_view(a, f1_link())));\n"
cases = [
 ('action_write_outside_writer', A, Ins, Ins + "  a->action = 3;\n", 'assignment through a game pointer outside a writer'),
 ('hp_write_outside_writer', A, Ins, Ins + "  static_cast<fopAc_ac_c*>(&a->actor)->health = 370;\n", 'assignment through a game pointer outside a writer' ),
 ('frame_write_outside_writer', A, Ins, Ins + "  a->anm_p->mFrameCtrl.mFrame = 0;\n", 'assignment through a game pointer outside a writer'),
 ('fifteenth_writer', A, '// ---------------------------------------------------------------- reads\n', 'LIZ_NOINLINE void liz_native_extra(e_dn_class* a) noexcept{(void)a;}\n// ---------------------------------------------------------------- reads\n', 'exactly fourteen noinline writer functions'),
 ('native_rng_in_adapter', A, Ins, Ins + "  (void)cM_rndF(1.0f);\n", 'forbidden native operation: cM_rnd'),
 ('touches_bokoblin', A, Ins, Ins + "  (void)sizeof(daE_OC_c);\n", 'forbidden native operation: daE_OC_c'),
 ('link_proc_call', A, Ins, Ins + "  (void)f1_link()->procGuardBreakInit();\n", 'forbidden native operation: procGuardBreakInit'),
 ('at_flag_forced', A, Ins, Ins + "  a->at_sph.OnAtSetBit();\n", 'forbidden native operation: OnAtSetBit'),
 ('cancel_binding_removed', A, 'if (s->b.on_at_hit_post(liz_view(a, f1_link()), is_link)) *r = nullptr;', '(void)is_link;', 'production binding missing'),
 ('shield_provenance_removed', A, 'if (L.pass.guard_attack && !cp04_guard_pass()) L.pass.guard_attack = false;', '', 'production binding missing'),
 ('all_or_nothing_removed', A, 'lz.enabled = ok;', 'lz.enabled = true;', 'production binding missing'),
 ('extra_hook', A, 'mods::hook::add_pre<LizDelete>(on_liz_delete_pre),', 'mods::hook::add_pre<LizDelete>(on_liz_delete_pre),mods::hook::add_post<LizDelete>(on_liz_exec_post),', 'hook registrations changed'),
 ('hp_value', C, 'kMaxHp = 370', 'kMaxHp = 400', 'fixed value or rule missing'),
 ('gain_value', C, 'kGainLinkParry = 12', 'kGainLinkParry = 15', 'fixed value or rule missing'),
 ('sword_gain_value', C, 'kGainSword = 6', 'kGainSword = 9', 'fixed value or rule missing'),
 ('sustained_value', C, 'kSustainedSwings = 4', 'kSustainedSwings = 6', 'fixed value or rule missing'),
 ('deflect_pct_value', C, 'kDeflectPct = 80', 'kDeflectPct = 100', 'fixed value or rule missing'),
 ('wound_pct_value', C, 'kGuardPctHit1 = 45', 'kGuardPctHit1 = 80', 'fixed value or rule missing'),
 ('two_blocks_arm_counter', C, 'if (!armed) return;\n    ++st.deflect_armed;\n    deflect = Deflect{};', 'if (!armed) return;\n    arm_counter(0);\n    deflect = Deflect{};', 'fixed value or rule missing'),
 ('bonus_rule', C, 'decisive_extra = static_cast<std::int16_t>((hp_pre - v.hp + 1) / 2);', 'decisive_extra = 0;', 'fixed value or rule missing'),
 ('link_bonus_removed', A, 'amount += o.extra;', '', 'production binding missing'),
 ('rebound_binding_removed', A, 'if (!parry) *r = FALSE;', '', 'production binding missing'),
 ('learn_write_outside_writer', A, Ins, Ins + "  a->learn = 0;\n", 'native write outside its writer' if False else 'assignment through a game pointer outside a writer'),
 ('ut2_write_outside_writer', A, Ins, Ins + "  a->unk_timer_2 = 0;\n", 'assignment through a game pointer outside a writer'),
 ('new_e_oc_hook', A, 'mods::hook::add_post<LizGuard>(on_liz_guard_post)', 'mods::hook::add_post<LizGuard>(on_liz_guard_post), mods::hook::add_post<Cp04EocExecute>(on_liz_guard_post)', 'hook registrations changed'),
 ('cooldown_value', C, 'kParryCooldownB = 90', 'kParryCooldownB = 120', 'fixed value or rule missing'),
 ('speed_d_value', C, 'kRateD = 1.10f', 'kRateD = 1.05f', 'fixed value or rule missing'),
 ('gap_value', C, '{0.58f, 0.60f, 0.54f, 0.66f, 0.67f}', '{0.70f, 0.60f, 0.54f, 0.66f, 0.67f}', 'fixed value or rule missing'),
 ('sound_invented', A, Ins, Ins + "  a->sound.startCollisionSE(0, 0);\n", 'forbidden native operation: startCollisionSE'),
 ('hit_mark_invented', A, Ins, Ins + "  dComIfGp_setHitMark(0, nullptr, nullptr, nullptr, nullptr, 0);\n", 'forbidden native operation: setHitMark'),
 ('hitbox_widened', A, Ins, Ins + "  a->at_sph.SetR(200.0f);\n", 'forbidden native operation: SetR('),
 ('atp_changed', A, Ins, Ins + "  a->at_sph.SetAtAtp(9);\n", 'forbidden native operation: SetAtAtp'),
 ('at_write_outside_writer', A, Ins, Ins + "  a->at_sph.SetC(cXyz());\n", 'native write outside its writer'),
 ('link_speed_outside_writer', A, Ins, Ins + "  f1_link()->mNormalSpeed = 0.0f;\n", 'assignment through a game pointer outside a writer'),
 ('slip_gate_removed', A, 'if (!c.cCcD_ObjHitInf::ChkTgHit() || !c.ChkTgShieldHit()) continue;', 'if (!c.cCcD_ObjHitInf::ChkTgHit()) continue;', 'production binding missing'),
 ('duplicate_gate_removed', A, 'if (s->b.feedback_impact == s->b.impact_counter) return;', '', 'production binding missing'),
 ('restore_check_removed', A, 'bool same = m->getFrame() == restore;', 'bool same = true;', 'production binding missing'),
 ('decisive_rule_changed', C, 'if (first && v.hp < hp_pre && last_swing != break_swing) {', 'if (first) {', 'fixed value or rule missing'),
 ('helm_rule_changed', C, 'shield_serial != 0 && shield_serial != break_shield) {', 'true) {', 'fixed value or rule missing'),
 ('pct_value', C, 'kParryPctC = 45', 'kParryPctC = 60', 'fixed value or rule missing'),
 ('rate_value', C, 'kRateN = 1.25f', 'kRateN = 1.5f', 'fixed value or rule missing'),
 ('pattern_changed', C, '{6, 9, {Act::D, Act::N, Act::D, Act::N, Act::N, Act::D}', '{6, 9, {Act::N, Act::D, Act::D, Act::N, Act::N, Act::D}', 'fixed value or rule missing'),
 ('core_uses_game_type', C, '#include "parry_core.hpp"\n', '#include "parry_core.hpp"\nclass e_dn_class;\n', 'core is not pure'),
 ('core_uses_crt_math', C, '#include "parry_core.hpp"\n', '#include "parry_core.hpp"\n#include <cmath>\n', 'core is not pure'),
 ('bokoblin_weight_changed', 'src/posture_core.hpp', 'inline constexpr', 'constexpr', 'production changed outside exact authorized edits'),
 ('mod_cpp_extra_line', M, '  liz_install();\n', '  liz_install();\n  g.p8.tick += 0;\n', 'exact integration changed'),
 ('link_core_extra_gain', L, 'o.weight = points;', 'o.weight = points + 1;', 'exact integration changed'),
 ('version_changed', 'src/lifecycle_core.hpp', 'kModVersion = "0.12.2"', 'kModVersion = "0.12.3"', 'exact integration changed'),
 ('new_source_file', 'src/liz_extra.hpp', None, '// extra\n', 'production file set differs'),
]
rows = []
for name, rel, old, new, want in cases:
    with tempfile.TemporaryDirectory() as td:
        p = Path(td)
        clone(p)
        f = p / rel
        if old is None:
            f.write_text(new, encoding='utf-8')
        else:
            t = f.read_text(encoding='utf-8')
            assert old in t, (name, old)
            f.write_text(t.replace(old, new, 1), encoding='utf-8')
        r, d = audit(p)
        assert r.returncode == 1 and any(want in e for e in d['errors']), (name, want, d['errors'][:6])
        rows.append({'case': name, 'rejected': True, 'expected_cause': want})
print(json.dumps({'positive_control': 'SOURCE_AUDIT_PASS', 'count': len(rows), 'all_rejected': True, 'cases': rows}, indent=1))
