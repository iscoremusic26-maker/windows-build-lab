"""POST05: mutaciones de prueba SOBRE COPIAS TEMPORALES (herramienta local, no forma parte del modulo).
Cada mutacion de logica debe hacer FALLAR los laboratorios (hit/lifecycle/collider/posture/reaction/compat); cada violacion de pasividad plantada en mod.cpp debe ser RECHAZADA por source_audit.
Uso: python3 -I tests/run_mutations.py   (requiere g++ C++20)"""
import json, shutil, subprocess, sys, tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
LOGIC = [
 ('M01 sin puerta de serial (duplicados pasan)', 'src/engine.hpp', 'if (!obs.ledger.try_emit(fr.slot, fr.generation, fr.serial)) return false;', 'obs.ledger.try_emit(fr.slot, fr.generation, fr.serial);'),
 ('M02 cooldown contado como candidato', 'src/hit_core.hpp', '  if (cooldown != 0) return PreClass::COOLDOWN_NO_PROCESS;\n', ''),
 ('M03 mpCollider obsoleto aceptado (src forzado)', 'src/hit_core.hpp', '    r.reason = ((b0 && !raw0) || (b1 && !raw1)) ? ColReason::RawNull : ColReason::NotEqualToRaw;\n    return r;', '    r.reason = ((b0 && !raw0) || (b1 && !raw1)) ? ColReason::RawNull : ColReason::NotEqualToRaw;\n    r.src = 0; r.deref_ok = true;\n    return r;'),
 ('M04 generacion constante (resurreccion)', 'src/lifecycle_core.hpp', 'n.generation = next_generation();', 'n.generation = 1;'),
 ('M05 sin validar generacion entre PRE y POST', 'src/engine.hpp', 'if (!valid) {  // retirada', 'if (false) {  // retirada'),
 ('M06 todo HIT_TYPE normal = espada', 'src/hit_core.hpp', '  if (type == 0) return r;\n', '  if (type == 0) return r;\n  r.family = Family::Sword; r.native_first = Family::Sword; r.groups = 1; r.reason = "NATIVE_BRANCH_ORDER"; return r;\n'),
 ('M07 confirmar sin proceso (sin cooldown)', 'src/hit_core.hpp', 'j.processed = pre.cooldown == 0 && post.cooldown != 0;', 'j.processed = true;'),
 ('M08 leer collider sin autorizacion', 'src/engine.hpp', 'if (j.read_collider) ok = read_at(r.selected, type);', 'ok = read_at(r.selected, type);'),
 ('M09 Finish nativo ignorado', 'src/hit_core.hpp', '    case PreClass::CUT_DOWN_FINISH_PATH:\n      if (post.health == 0', '    case PreClass::UNKNOWN:\n      if (post.health == 0'),
 ('M10 frames obsoletos no se descartan', 'src/lifecycle_core.hpp', 'const bool stale = frames_[i].actor == actor || frames_[i].serial + kStaleAge < serial_ + 1;', 'const bool stale = false;'),
 ('M11 firma sin mOldActionMode', 'src/hit_core.hpp', 'post.old_mode == pre.mode;', 'true;'),
 ('M12 serial repetido admitido (< en lugar de <=)', 'src/lifecycle_core.hpp', 'if (serial <= s.last_event_serial) {', 'if (serial < s.last_event_serial) {'),
 ('M13 fusion por actor distinto (dedup global)', 'src/lifecycle_core.hpp', 'if (serial <= s.last_event_serial) {', 'if (serial <= (last_global_ < s.last_event_serial ? s.last_event_serial : last_global_)) {'),
 ('M14 eviccion de ACTIVE', 'src/lifecycle_core.hpp', 'if (s_[i].life == Life::Unverified && (best < 0', 'if ((s_[i].life == Life::Unverified || s_[i].life == Life::Active) && (best < 0'),
 ('M15 direccion reciclada sin retiro', 'src/lifecycle_core.hpp', 'if (is_live(s) && s.ptr == ptr && s.id != id) {', 'if (false) {'),
 ('M16 dual TG cuenta pair_distinct siempre', 'src/engine.hpp', 'if (r.raw0 != r.raw1) ++c.pair_distinct;', '++c.pair_distinct;'),
]
LOGIC += [
 ('N01 alineacion de lectura no exigida', 'src/hit_core.hpp', 'return p >= kMinPlausible && (p & (kReadAlignBytes - 1u)) == 0;', 'return p >= kMinPlausible;'),
 ('N02 alineacion de 8 (regresion de POST03)', 'src/hit_core.hpp', 'return p >= kMinPlausible && (p & (kReadAlignBytes - 1u)) == 0;', 'return p >= kMinPlausible && (p & 7u) == 0;'),
 ('N03 coincidencia sin igualdad con el hit obj crudo', 'src/hit_core.hpp', 'if (b1 && raw1 == sel) {', 'if (b1) {'),
 ('N04 umbral minimo eliminado', 'src/hit_core.hpp', 'if (sel < kMinPlausible) {', 'if (false) {'),
 ('N05 cc1 deja de prevalecer', 'src/hit_core.hpp', 'if (b1 && raw1 == sel) {', 'if (b1 && raw1 == sel && !b0) {'),
 ('N06 se lee el collider equivocado (raw0)', 'src/engine.hpp', 'if (j.read_collider) ok = read_at(r.selected, type);', 'if (j.read_collider) ok = read_at(fr.raw0, type);'),
 ('N07 doble lectura por evento', 'src/engine.hpp', 'if (j.read_collider) ok = read_at(r.selected, type);', 'if (j.read_collider) { ok = read_at(r.selected, type); ok = read_at(r.selected, type); }'),
 ('N08 lectura sin autorizacion de judge', 'src/hit_core.hpp', 'j.read_collider = col.deref_ok;', 'j.read_collider = col.src >= -1;'),
 ('N09 desalineado coincidente se lee', 'src/hit_core.hpp', '  if (!deref_alignment_ok(sel)) {\n    r.reason = ColReason::SelectedAlignment;\n    return r;\n  }\n', ''),
]

ALL_LABS = ('hit_lab', 'lifecycle_lab', 'collider_lab', 'posture_lab', 'reaction_lab', 'compat_lab', 'rb_lab', 'general_lab', 'break_lab')
# Mutaciones del NUCLEO DE POSTURA (cada una debe ser detectada por posture_lab). (nombre, archivo, [(viejo, nuevo), ...])
POSTURE = [
 ('P01 OTHER/BOOMERANG con peso (politica rota)', 'src/posture_core.hpp', [('    default: return 0;\n  }\n}\nstatic_assert(kMaxPosture', '    default: return 10;\n  }\n}\nstatic_assert(kMaxPosture')]),
 ('P02 sin guarda de evento duplicado', 'src/posture_core.hpp', [('if (r.event != 0 && r.event <= last_event_) {', 'if (false) {')]),
 ('P03 golpe letal suma peso', 'src/posture_core.hpp', [('      else if (lethal) why = Skip::Lethal;\n', '')]),
 ('P04 actor muerto sigue acumulando', 'src/posture_core.hpp', [('      else if (e->dead) why = Skip::ActorDead;\n', '')]),
 ('P05 recupera antes de 120 ticks', 'src/posture_core.hpp', [('inline constexpr std::uint64_t kRecoverDelayTicks = 120;', 'inline constexpr std::uint64_t kRecoverDelayTicks = 60;')]),
 ('P06 recuperacion doble por tick', 'src/posture_core.hpp', [('        --e.posture;\n        ++c.units_recovered;', '        e.posture = e.posture > 1 ? e.posture - 2 : 0;\n        c.units_recovered += 2;')]),
 ('P07 retiro no limpia la entrada (fuga entre generaciones)', 'src/posture_core.hpp', [('    e = Entry{};  // la entrada vuelve a vacio: nada se hereda a la generacion siguiente', '    e.bound = false;')]),
 ('P08 el umbral no marca latch', 'src/posture_core.hpp', [('      e->latched = true;\n      ++c.crossings;', '      ++c.crossings;')]),
 ('P09 latched sigue recuperando', 'src/posture_core.hpp', [('      if (e.latched) {\n        ++c.latched_entry_ticks;\n        continue;\n      }\n', '')]),
 ('P10 peso derivado del dano (delta de HP)', 'src/posture_core.hpp', [('const int w = known_family ? policy::weight_of(r.fam.family) : 0;', 'const int w = known_family ? (r.pre.health - r.post.health) : 0;')]),
 ('P11 Finish cuenta como golpe', 'src/posture_core.hpp', [('    if (r.j.kind == HitKind::FinishNative || r.j.kind == HitKind::FinishUnexpected) {', '    if (false) {')]),
 ('P12 generacion no validada', 'src/posture_core.hpp', [('|| !L.valid(r.slot, r.generation) ||', '||')]),
 ('P13 acumula congelado (UNKNOWN_UNSAFE)', 'src/posture_core.hpp', [('      else if (e->frozen) why = Skip::Frozen;\n', '')]),
 ('P14 delete_claim siempre verdadero', 'src/posture_core.hpp', [('    it.delete_claim = claim;  // SOLO LOOKUP_ABSENT_VERIFIED del ledger', '    it.delete_claim = true;')]),
 ('P15 AMBIGUOUS se reduce a SWORD', 'src/posture_core.hpp', [('      else if (r.fam.family == Family::Ambiguous) why = Skip::Ambiguous;\n', ''), ('    case Family::IronBall: return kWeightIronBall;', '    case Family::IronBall: return kWeightIronBall;\n    case Family::Ambiguous: return kWeightSword;')]),
 ('P16 recupera estando congelado', 'src/posture_core.hpp', [('      if (e.frozen) {\n        ++c.frozen_entry_ticks;\n        continue;\n      }\n', '')]),
 ('P17 el golpe no reinicia el retardo', 'src/posture_core.hpp', [('    e->k = 0;\n    e->regen_acc = 0;\n    e->last_hit_tick = now_;', '    e->regen_acc = 0;\n    e->last_hit_tick = now_;')]),
 ('P18 sin tope de saturacion', 'src/posture_core.hpp', [('    if (sum > policy::kMaxPosture) {\n      sum = policy::kMaxPosture;\n      ++c.saturations;\n    }\n', '')]),
 ('P19 sin guarda de underflow', 'src/posture_core.hpp', [('      if (e.posture == 0) continue;\n      ++e.k;', '      ++e.k;')]),
 ('P20 E_OC2 EXCLUIDO (V1) / otro perfil recibe sidecar', 'src/posture_core.hpp', [('      if (post03::is_candidate(in.adm)) {  // POST05-COMPAT: E_OC y E_OC2 candidatos reciben postura (pasiva)\n        bind(i, in);', '      if (in.adm != Admission::UNKNOWN_UNSAFE) {\n        bind(i, in);')]),  # POST05-COMPAT: patron actualizado (misma mutacion semantica)
 ('P21 doble avance por mod_update', 'src/posture_core.hpp', [('    px.sync(eng.obs.ledger);\n    px.advance();', '    px.sync(eng.obs.ledger);\n    px.advance();\n    px.advance();')]),
 ('P22 retiro no cierra unidades (conservacion)', 'src/posture_core.hpp', [('    c.units_closed += e.posture;\n', '')]),
 ('P23 el consumo ocurre tras sincronizar la muerte', 'src/posture_core.hpp', [('    if (has) last = px.consume(rep, eng.obs.ledger);  // ANTES de sincronizar la muerte de este mismo golpe: asi un golpe letal se omite como LETHAL\n    if (has || ev.n > 0) px.sync(eng.obs.ledger);', '    if (has || ev.n > 0) px.sync(eng.obs.ledger);\n    if (has) last = px.consume(rep, eng.obs.ledger);')]),
 ('P24 modo deshabilitado sigue acumulando', 'src/posture_core.hpp', [('    if (eng.obs.disabled) return false;\n    const bool has = eng.on_post', '    const bool has = eng.on_post')]),
 ('P25 el tick elegible del propio golpe cuenta', 'src/posture_core.hpp', [('    e->k = 0;\n    e->regen_acc = 0;\n    e->last_hit_tick = now_;', '    e->k = 1;\n    e->regen_acc = 0;\n    e->last_hit_tick = now_;')]),
 ('P26 regenera mas deprisa que la tasa (acumulador por 2)', 'src/posture_core.hpp', [('      e.regen_acc = static_cast<std::uint8_t>(e.regen_acc + (e.regen_rate', '      e.regen_acc = static_cast<std::uint8_t>(e.regen_acc + 2 * (e.regen_rate')]),
 ('P27 crossing reemitido tras latch', 'src/posture_core.hpp', [('      else if (e->latched) why = Skip::Latched;\n', '')]),
 ('P28 peso de SWORD alterado', 'src/posture_core.hpp', [('inline constexpr int kWeightSword = 25;', 'inline constexpr int kWeightSword = 30;')]),
]

LOGIC = [(n, f, [(o, w)], ALL_LABS) for (n, f, o, w) in LOGIC]
R = 'src/reaction_core.hpp'
REACTION = [
 ('R01 BIG_DAMAGE suprimible', R, [('    if (r.post.mode == p::kModeBigDamage) return Reason::BigDamage;\n', '')]),
 ('R02 golpe letal suprimible', R, [('    if (r.j.lethal || r.post.health <= 1) return Reason::Lethal;\n', '')]),
 ('R03 golpe de umbral suprimible', R, [('    if (o.crossed || o.after >= post04::policy::kThreshold || o.before >= post04::policy::kThreshold) return Reason::CrossesThreshold;\n', '')]),
 ('R04 cualquier familia', R, [('if (!r.at_valid || r.fam.family != Family::Sword) return Reason::FamilyNotSword;', 'if (false) return Reason::FamilyNotSword;')]),
 ('R05 cualquier modo PRE', R, [('    if (r.pre.mode != p::kModeAttack) return Reason::PreModeNotAttack;\n', '')]),
 ('R06 ATTACK estado 3 (rebote) admitido', R, [('return s == 0 || s == 1 || s == 2; }\n// my_val', 'return s == 0 || s == 1 || s == 2 || s == 3; }\n// my_val')]),
 ('R07 sin comprobar temporizadores PRE', R, [('    if (r.aux_pre.t6c0 != 0 || r.aux_pre.t6c2 != 0) return Reason::PreTimersNonZero;\n', '')]),
 ('R08 sin comprobar HeadLock PRE', R, [('    if (r.aux_pre.head_lock != 0) return Reason::PreHeadLock;\n', '')]),
 ('R09 sin comprobar bits Tg/Co PRE', R, [('    if (r.aux_pre.set_bits != post03::kAllSetBits) return Reason::PreSetBitsOff;\n', '')]),
 ('R10 sin comprobar aux POST', R, [('if (r.aux_post.t6c0 != 0 || r.aux_post.t6c2 != 0 || r.aux_post.head_lock != 0 || r.aux_post.set_bits != post03::kAllSetBits) return Reason::PostAuxUnexpected;', 'if (false) return Reason::PostAuxUnexpected;')]),
 ('R11 admision inestable aceptada', R, [('    if (o.ever_frozen) return Reason::AdmissionEverUnstable;\n', '')]),
 ('R12 firma P2 ignorada', R, [('    if (r.post.old_mode != r.pre.mode) return Reason::PostSignatureMismatch;  // setActionMode corrio exactamente una vez en ESTA llamada\n', '')]),
 ('R13 salto cancelado (cooldown 3) suprimible', R, [('    if (r.post.cooldown != p::kCooldownOrdinary) return Reason::CooldownNotOrdinary;\n', '')]),
 ('R14 plan no restaura el modo', R, [('        d.plan.target = LiveMode{r.pre.mode, r.pre.state,', '        d.plan.target = LiveMode{r.post.mode, r.pre.state,')]),
 ('R15 mOldActionMode mal restaurado', R, [('LiveMode{r.pre.mode, r.pre.state, r.pre.old_mode, r.aux_pre.t6c0', 'LiveMode{r.pre.mode, r.pre.state, r.post.old_mode, r.aux_pre.t6c0')]),
 ('R16 sin re-comprobacion antes de escribir', R, [('    if (!same_live(now, d.plan.expect)) {', '    if (false) {')]),
 ('R17 verificacion no cierra la sesion', R, [('      session_disabled_ = true;  // fallo seguro', '      // fallo seguro')]),
 ('R18 sin guarda de duplicados', R, [('    if (r.event != 0 && r.event <= last_event_) {\n      ++c.duplicates;', '    if (false) {\n      ++c.duplicates;')]),
 ('R19 sesion cerrada ignorada en decide', R, [('    if (session_disabled_) return Reason::SessionDisabled;\n', '')]),
 ('R20 token no validado', R, [('if (!in.ledger_valid || o.event != r.event || o.slot != r.slot || o.generation != r.generation || r.slot < 0) return Reason::TokenMismatch;', 'if (false) return Reason::TokenMismatch;')]),
 ('R21 aplica sin elegibilidad', R, [('    if (!d.counted || d.decision != Decision::SuppressEligible) return Apply::NotAttempted;', '    if (!d.counted) return Apply::NotAttempted;')]),
 ('R22 suprime sin ponderacion de POST04', R, [('    if (!o.applied) return Reason::NotWeighted;\n', '')]),
 ('R23 congelado/muerto aceptado', R, [('    if (o.frozen || o.dead) return Reason::FrozenOrDead;\n', '')]),
 ('R24 contador attempted perdido', R, [('    ++c.attempted;\n', '')]),
 ('R25 estado POST no ordinario aceptado', R, [('    if (!p::ordinary_sword_damage_state(r.post.state)) return Reason::PostStateNotOrdinary;\n', '')]),
]
POSTURE = [(n, f, pairs, ('posture_lab', 'reaction_lab')) for (n, f, pairs) in POSTURE]
POSTURE += [(n, f, pairs, ('reaction_lab',)) for (n, f, pairs) in REACTION]
# POST05-COMPAT (0.5.1): mutaciones de admision V2, diferimiento de E_OC2, potencia en la llamada, mascara y formato (detector: compat_lab)
COMPAT = [
 ('C01 E_OC admite AtAtp 3/6', 'src/hit_core.hpp', [('    if (f.atp != 1 && f.atp != 2) return {Admission::UNKNOWN_UNSAFE, "E_OC_ATP_OUTSIDE_SET"};', '    if (f.atp < 1) return {Admission::UNKNOWN_UNSAFE, "E_OC_ATP_OUTSIDE_SET"};')]),
 ('C02 V2 acepta esferas en desacuerdo', 'src/hit_core.hpp', [('  if (f.atp == -2) return {Admission::UNKNOWN_UNSAFE, "E_OC_ATP_SPHERES_DISAGREE"};', '  if (f.atp == -2 && !v2) return {Admission::UNKNOWN_UNSAFE, "E_OC_ATP_SPHERES_DISAGREE"};')]),
 ('C03 E_OC2 con 0x10000 admitido', 'src/hit_core.hpp', [('if (f.status_10000) return {Admission::UNKNOWN_UNSAFE, "E_OC2_STATUS_10000_PRESENT"};', 'if (false) return {Admission::UNKNOWN_UNSAFE, "E_OC2_STATUS_10000_PRESENT"};')]),
 ('C04 E_OC2 admite AtAtp 4/5', 'src/hit_core.hpp', [('atp == 1 || atp == 2 || atp == 3 || atp == 6;', 'atp >= 1 && atp <= 6;')]),
 ('C05 politica por defecto V2 (labs heredados)', 'src/hit_core.hpp', [('AdmissionPolicy pol = AdmissionPolicy::V1Strict', 'AdmissionPolicy pol = AdmissionPolicy::V2Compat')]),
 ('C06 is_candidate excluye E_OC2', 'src/hit_core.hpp', [('return a == Admission::E_OC_CANDIDATE || a == Admission::E_OC2_CANDIDATE;', 'return a == Admission::E_OC_CANDIDATE;')]),
 ('C07 sin diferimiento de E_OC2', 'src/reaction_core.hpp', [('    if (e2 && !(e2_policy == E2Policy::MatureBC && e2_consistent(in))) return Reason::E_OC2SuppressionDeferred;\n', '')]),
 ('C08 diferimiento solo por admision del ledger', 'src/reaction_core.hpp', [('return in.adm == Admission::E_OC2_CANDIDATE || in.adm == Admission::E_OC2_EXCLUDED || in.rep.aux_pre.resource == k2 || in.rep.aux_post.resource == k2;', '(void)k2; return in.adm == Admission::E_OC2_CANDIDATE;')]),
 ('C09 sin POWER_CHANGED_IN_CALL', 'src/reaction_core.hpp', [('    if (r.aux_pre.atp != r.aux_post.atp || r.aux_pre.resource != r.aux_post.resource || r.aux_pre.atp == -2) return Reason::PowerChangedInCall;\n', '')]),
 ('C10 mascara: sin motivo de admision', 'src/posture_core.hpp', [('    if (!dead && in.adm == Admission::UNKNOWN_UNSAFE) e.unsafe_mask |= kUnsafeAdmission;\n', '')]),
 ('C11 mascara: sin motivo de lookup', 'src/posture_core.hpp', [('    if (!dead && in.life == Life::Unverified) e.unsafe_mask |= kUnsafeLookup;\n', '')]),
 ('C12 contador por recurso intercambiado', 'src/reaction_core.hpp', [('    if (any_e_oc2(in)) return 1;\n    return in.adm == Admission::E_OC_CANDIDATE ? 0 : 2;', '    if (any_e_oc2(in)) return 0;\n    return in.adm == Admission::E_OC_CANDIDATE ? 1 : 2;')]),
 ('C13 R6 vacia', 'src/reaction_core.hpp', [('a.r6 = c.by_resource[0] + c.by_resource[1] + c.by_resource[2] == c.decisions;', 'a.r6 = true;')]),
 ('C14 motor ignora la politica', 'src/engine.hpp', [('const AdmissionResult adm = admit(r.facts, policy);', 'const AdmissionResult adm = admit(r.facts);')]),
 ('C15 detalle solo para E_OC', 'src/engine.hpp', [('const bool detail = o.tracked && is_candidate(adm.adm);', 'const bool detail = o.tracked && adm.adm == Admission::E_OC_CANDIDATE;')]),
 ('C16 postura solo para E_OC', 'src/posture_core.hpp', [('      if (post03::is_candidate(in.adm)) {  // POST05-COMPAT', '      if (in.adm == Admission::E_OC_CANDIDATE) {  // POST05-COMPAT')]),
 ('C17 decision sin campo admission', 'src/reaction_core.hpp', [('"posture=%d>%d action=%s resource=%s atp=%d>%d admission=%s phase=%s frame=%.2f rate=%.2f speed=%.1f env=%s compat=%s plan=%s",', '"posture=%d>%d action=%s resource=%s atp=%d>%d phase=%s frame=%.2f rate=%.2f speed=%.1f env=%s compat=%s plan=%s",')]),
 ('C18 resumen POST03 sin e_oc2_cand', 'src/lifecycle_core.hpp', [('adm(cand/e_oc2/other/unsafe/e_oc2_cand)=%llu/%llu/%llu/%llu/%llu', 'adm(cand/e_oc2/other/unsafe)=%llu/%llu/%llu/%llu')]),
 ('C19 diferimiento evaluado tarde (tras reglas de politica)', 'src/reaction_core.hpp', [('    if (e2 && !(e2_policy == E2Policy::MatureBC && e2_consistent(in))) return Reason::E_OC2SuppressionDeferred;\n', ''), ('    if (session_disabled_) return Reason::SessionDisabled;', '    if (e2 && !(e2_policy == E2Policy::MatureBC && e2_consistent(in))) return Reason::E_OC2SuppressionDeferred;\n    if (session_disabled_) return Reason::SessionDisabled;')]),
 ('C20 V2 exige AtAtp 1 (regresion a 0.5.0)', 'src/hit_core.hpp', [('    if (f.atp != 1 && f.atp != 2) return {Admission::UNKNOWN_UNSAFE, "E_OC_ATP_OUTSIDE_SET"};', '    if (f.atp != 1) return {Admission::UNKNOWN_UNSAFE, "E_OC_ATP_OUTSIDE_SET"};')]),
 ('C21 POWER_CHANGED solo por AtAtp (recurso ignorado) y sin desacuerdo', 'src/reaction_core.hpp', [('r.aux_pre.atp != r.aux_post.atp || r.aux_pre.resource != r.aux_post.resource || r.aux_pre.atp == -2', 'r.aux_pre.atp != r.aux_post.atp && r.aux_pre.atp > 100')]),
 ('C22 decision de E_OC2 contada como E_OC', 'src/reaction_core.hpp', [('    d.adm = in.adm;\n', '    d.adm = in.adm == Admission::E_OC2_CANDIDATE ? Admission::E_OC_CANDIDATE : in.adm;\n')]),
]
POSTURE += [(n, f, pairs, ('compat_lab',)) for (n, f, pairs) in COMPAT]
# POST05-RB (0.5.2): mutaciones del gate E_OC2 de fase nativa, perfiles y contadores (detector: rb_lab; compat_lab cuando aplica)
RB = [
 ('RB01 frame maduro desde 1 (A5-2)', 'src/reaction_core.hpp', [('kE2MatureFrame = 1.999f', 'kE2MatureFrame = 0.999f')]),
 ('RB02 frenada desde 0.5 (A5-1)', 'src/reaction_core.hpp', [('kE2BrakeSpeed = 1.5f', 'kE2BrakeSpeed = 0.5f')]),
 ('RB03 sin autenticar el fin de clip', 'src/reaction_core.hpp', [('a.anim_end != want_end || ', '')]),
 ('RB04 rate desconocida aceptada', 'src/reaction_core.hpp', [('  if (!p::e2_rate_known(a.anim_rate)) return E2Phase::AnimUnverified;', '')]),
 ('RB05 fin de clip aceptado', 'src/reaction_core.hpp', [('  if ((a.anim_state & 1u) != 0 || a.anim_rate == 0.0f) return E2Phase::ClipEnding;', '')]),
 ('RB06 perfil atp 1 admitido', 'src/reaction_core.hpp', [('return atp == kE2AtpRed ||', 'return atp == 1 || atp == kE2AtpRed ||')]),
 ('RB07 sin comprobar animacion PRE==POST', 'src/reaction_core.hpp', [('    if (e2 && !same_anim(r.aux_pre, r.aux_post)) return Reason::PostAuxUnexpected;', '')]),
 ('RB08 coherencia E_OC2 sin recurso leido', 'src/reaction_core.hpp', [(' && in.rep.aux_pre.resource == k2 && in.rep.aux_post.resource == k2;', '; (void)k2;')]),
 ('RB09 E_OC2 exige admision E_OC', 'src/reaction_core.hpp', [('(e2 ? Admission::E_OC2_CANDIDATE : Admission::E_OC_CANDIDATE)', 'Admission::E_OC_CANDIDATE')]),
 ('RB10 motivos de fase como SKIP_UNSAFE', 'src/reaction_core.hpp', [('if (r <= Reason::PostureSkipUnsafe) return Decision::AllowVanilla;', 'if (r <= Reason::PostStateNotOrdinary) return Decision::AllowVanilla;')]),
 ('RB11 Black 3->6 deshabilitado', 'src/reaction_core.hpp', [('kE2BlackFuryStartEnabled = true', 'kE2BlackFuryStartEnabled = false')]),
 ('RB12 R7 sin contar perfiles', 'src/reaction_core.hpp', [('    ++c.applied_by_profile[d.profile < 4 ? d.profile : 0];\n', '')]),
 ('RB13 gate de fase ignorado', 'src/reaction_core.hpp', [('      phase = e2_phase(r.aux_pre, r.pre.state);', '      phase = E2Phase::BcMature;')]),
 ('RB14 tick de arranque tratado como frenada', 'src/reaction_core.hpp', [('return (a.speed > p::kE2BrakeSpeed || a.speed < -p::kE2BrakeSpeed) ? E2Phase::Brake : E2Phase::Startup;', 'return E2Phase::Brake;')]),
 ('RB15 politica E_OC2 por defecto MatureBC', 'src/reaction_core.hpp', [('E2Policy e2_policy = E2Policy::Deferred;', 'E2Policy e2_policy = E2Policy::MatureBC;')]),
 ('RB16 perfiles atp3/atp6 intercambiados', 'src/reaction_core.hpp', [('? 2 : atp == policy::kE2AtpBlackFury ? 3', '? 3 : atp == policy::kE2AtpBlackFury ? 2')]),
 ('RB17 frenada solo hacia delante', 'src/reaction_core.hpp', [(' || a.speed < -p::kE2BrakeSpeed) ?', ') ?')]),
 ('RB18 frame >= fin aceptado', 'src/reaction_core.hpp', [(' ||\n      a.anim_frame >= static_cast<float>(a.anim_end))', ')')]),
 ('RB19 modo de reproduccion no comprobado', 'src/reaction_core.hpp', [('a.anim_attr != 0 || ', '')]),
 ('RB20 E_OC2 elegible sin politica', 'src/reaction_core.hpp', [('if (e2 && !(e2_policy == E2Policy::MatureBC && e2_consistent(in)))', 'if (e2 && !e2_consistent(in))')]),
 ('RB21 NaN de velocidad aceptado', 'src/reaction_core.hpp', [('    if (!(a.speed == a.speed)) return E2Phase::AnimUnverified;  // NaN\n', '')]),
 ('RB22 frame de un estado leido como el otro', 'src/reaction_core.hpp', [('pre_state == 1 ? p::kClipEndB : p::kClipEndC', 'pre_state == 2 ? p::kClipEndB : p::kClipEndC')]),
]
# POST05-FINALIZE (0.5.3): entorno y fases tempranas/finales
RB += [
 ('RB23 regla NATIVE_ONLY sin exigir potencia de Red', 'src/reaction_core.hpp', [('if (env == E2Env::Absent && atp == p::kE2AtpRed)', 'if (env == E2Env::Absent)')]),
 ('RB24 entorno sin comprobar tratado como NATIVE_ONLY', 'src/reaction_core.hpp', [('if (env == E2Env::Absent && atp == p::kE2AtpRed)', 'if (env != E2Env::Possible && atp == p::kE2AtpRed)')]),
 ('RB25 frame 1 con cualquier rate distinta de 1', 'src/reaction_core.hpp', [('    if (rate == 1.15f) { why = E2Compat::X115Owned; return true; }', '    if (rate != 1.0f) { why = E2Compat::X115Owned; return true; }')]),
 ('RB26 Black en furia tratado como base', 'src/reaction_core.hpp', [('    if (atp == p::kE2AtpBlack) { why = E2Compat::BlackBaseNoPending; return true; }', '    if (atp != p::kE2AtpRed) { why = E2Compat::BlackBaseNoPending; return true; }')]),
 ('RB27 fin de clip permitido con Expansion', 'src/reaction_core.hpp', [('  why = ph == E2Phase::Startup ? E2Compat::RateDecisionThisTick : E2Compat::ClipEndChain;\n  return false;', '  why = ph == E2Phase::Startup ? E2Compat::RateDecisionThisTick : E2Compat::ClipEndChain;\n  return ph == E2Phase::ClipEnding;')]),
 ('RB28 arranque permitido con Expansion', 'src/reaction_core.hpp', [('  why = ph == E2Phase::Startup ? E2Compat::RateDecisionThisTick : E2Compat::ClipEndChain;\n  return false;', '  why = ph == E2Phase::Startup ? E2Compat::RateDecisionThisTick : E2Compat::ClipEndChain;\n  return ph == E2Phase::Startup;')]),
 ('RB29 R8 sin contar fases aplicadas', 'src/reaction_core.hpp', [('    if (d.profile != 0 && static_cast<unsigned>(d.phase) < kE2PhaseCount) ++c.e2_applied_phase[static_cast<unsigned>(d.phase)];\n', '')]),
 ('RB30 entorno por defecto NATIVE_ONLY', 'src/reaction_core.hpp', [('E2Env e2_env = E2Env::Possible;', 'E2Env e2_env = E2Env::Absent;')]),
 ('RB31 frame 1 x2 pendiente permitido', 'src/reaction_core.hpp', [('    why = E2Compat::X2PendingPossible;\n    return false;', '    why = E2Compat::X2PendingPossible;\n    return true;')]),
]
POSTURE += [(n, f, pairs, ('rb_lab', 'compat_lab')) for (n, f, pairs) in RB]
# POST05-GENERAL (0.5.4): regla general por postura y escritor ampliado (detector: general_lab; rb_lab/reaction_lab cuando aplica)
GEN = [
 ('GN01 cruce a 100 suprimido', 'src/reaction_core.hpp', [('    if (o.crossed) return Reason::CrossesThreshold;\n', '')]),
 ('GN02 latch 100 suprimido', 'src/reaction_core.hpp', [('    if (o.latched_after || o.before >= post04::policy::kThreshold || o.after >= post04::policy::kThreshold) return Reason::LatchedAfter;\n', '')]),
 ('GN03 postura sin sidecar suprimida', 'src/reaction_core.hpp', [('    if (!o.applied && o.skip != post04::Skip::ZeroWeight) return Reason::PostureSkipUnsafe;', '')]),
 ('GN04 letal suprimido', 'src/reaction_core.hpp', [('    if (r.post.health <= 1 || r.j.lethal) return Reason::Lethal;  // la muerte legitima manda\n', '')]),
 ('GN05 modos de guion permitidos', 'src/reaction_core.hpp', [('inline constexpr bool general_pre_mode_allowed(int m) noexcept { return m == 0', 'inline constexpr bool general_pre_mode_allowed(int m) noexcept { return m >= 0 || m == 0')]),
 ('GN06 desprevenido vuelve a WAIT', 'src/reaction_core.hpp', [('{ return m == kModeWait || m == kModeWalk || m == kModeWatch || m == kModeSoundWatch; }', '{ return m < 0 && (m == kModeWait || m == kModeWalk || m == kModeWatch || m == kModeSoundWatch); }')]),
 ('GN07 forma BIG_DAMAGE exige Tg activo', 'src/reaction_core.hpp', [('(big ? p::kBitsCoOnly : post03::kAllSetBits)', 'post03::kAllSetBits')]),
 ('GN08 restauracion con timers POST', 'src/reaction_core.hpp', [('r.aux_pre.t6c0, r.aux_pre.t6c2, r.aux_pre.head_lock, r.aux_pre.set_bits};', 'r.aux_post.t6c0, r.aux_post.t6c2, r.aux_post.head_lock, r.aux_post.set_bits};')]),
 ('GN09 FIND estado 0 en vez de 1', 'src/reaction_core.hpp', [('LiveMode{policy::kModeFind, 1, r.pre.mode, 0, 0, 0, post03::kAllSetBits};', 'LiveMode{policy::kModeFind, 0, r.pre.mode, 0, 0, 0, post03::kAllSetBits};')]),
 ('GN10 escritor de 3 campos con plan auxiliar', 'src/reaction_core.hpp', [('(!kFull && d.plan.aux_write)', 'false')]),
 ('GN11 relectura solo del modo', 'src/reaction_core.hpp', [('    if (!same_live(now, d.plan.expect)) {', '    if (now.mode != d.plan.expect.mode) {')]),
 ('GN12 verificacion solo del modo', 'src/reaction_core.hpp', [('    if (!same_live(after, d.plan.target)) {', '    if (after.mode != d.plan.target.mode) {')]),
 ('GN13 familia ambigua protegida', 'src/reaction_core.hpp', [('r.fam.family == Family::Unavailable || r.fam.family == Family::Ambiguous) return Reason::FamilyUnverified;', 'r.fam.family == Family::Unavailable) return Reason::FamilyUnverified;')]),
 ('GN14 politica por defecto General', 'src/reaction_core.hpp', [('StaggerPolicy stagger_policy = StaggerPolicy::AttackOnly;', 'StaggerPolicy stagger_policy = StaggerPolicy::General;')]),
 ('GN15 FIND tratado como desprevenido', 'src/reaction_core.hpp', [('{ return m == kModeWait || m == kModeWalk || m == kModeWatch || m == kModeSoundWatch; }', '{ return m == kModeWait || m == kModeWalk || m == kModeWatch || m == kModeSoundWatch || m == kModeFind; }')]),
 ('GN16 forma BIG_DAMAGE sin exigir offTgSph', 'src/reaction_core.hpp', [('inline constexpr std::uint8_t kBitsCoOnly = 0x0A;', 'inline constexpr std::uint8_t kBitsCoOnly = 0x0F;')]),
 ('GN19 E_OC2 dudoso protegido', 'src/reaction_core.hpp', [('    if (e2 && (e2_policy != E2Policy::MatureBC || !e2_consistent(in))) return Reason::E_OC2SuppressionDeferred;  // identidad E_OC2 dudosa: nativo\n', '')]),
 ('GN20 potencia cambiada en la llamada aceptada (general)', 'src/reaction_core.hpp', [('    if (r.aux_pre.atp == -2 || r.aux_post.atp != r.aux_pre.atp || r.aux_post.resource != r.aux_pre.resource) return Reason::PowerChangedInCall;', '')]),
 ('GN17 vuelve a exigir ATTACK', 'src/reaction_core.hpp', [('    if (!p::general_pre_mode_allowed(r.pre.mode)) return Reason::PreModeScripted;', '    if (r.pre.mode != p::kModeAttack) return Reason::PreModeScripted;')]),
 ('GN18 conteo de planes roto', 'src/reaction_core.hpp', [('    ++c.applied_by_kind[static_cast<unsigned>(d.plan.kind) < 3 ? static_cast<unsigned>(d.plan.kind) : 0];\n', '')]),
]
POSTURE += [(n, f, pairs, ('general_lab', 'reaction_lab', 'rb_lab')) for (n, f, pairs) in GEN]
# POST06 (0.6.0): ruptura/castigo/caida/rearme y bandas de recuperacion (detector: break_lab; posture_lab para el nucleo de postura)
BK = [
 ('BK01 bonus progresivo', 'src/break_core.hpp', [('            const int extra = s.frac / policy::kBonusDen;', '            const int extra = (s.frac + static_cast<int>(c.bonus_applied) * 2) / policy::kBonusDen;')]),
 ('BK02 espadazo no renueva la inactividad', 'src/break_core.hpp', [('          s.last_sword = s.exec;  // renueva SOLO la inactividad', '          (void)0;  // renueva SOLO la inactividad')]),
 ('BK03 espadazo extiende el techo de 3 s', 'src/break_core.hpp', [('          s.last_sword = s.exec;  // renueva SOLO la inactividad', '          s.last_sword = s.punish_start = s.exec;  // renueva SOLO la inactividad')]),
 ('BK04 rearme durante el levantamiento', 'src/break_core.hpp', [('        if (v.mode == policy::kModeBig) break;  // aun levantandose', '        // aun levantandose')]),
 ('BK05 bonus para cualquier familia', 'src/break_core.hpp', [('        if (v.hit && v.sword) {', '        if (v.hit) {')]),
 ('BK06 sin acumulador de fracciones', 'src/break_core.hpp', [('            s.frac %= policy::kBonusDen;', '            s.frac = 0;')]),
 ('BK07 inactividad de 2 s', 'src/break_core.hpp', [('kInactivityExec = 30;', 'kInactivityExec = 60;')]),
 ('BK08 techo de 6 s', 'src/break_core.hpp', [('kPunishMaxExec = 90;', 'kPunishMaxExec = 180;')]),
 ('BK09 bonus letal sin rama letal nativa', 'src/break_core.hpp', [('              a.convert_to_big = a.lethal && v.mode == policy::kModeDamage;', '              a.convert_to_big = false;')]),
 ('BK10 sin limite de seguridad', 'src/break_core.hpp', [('          if (s.exec - s.due_since >= policy::kDropSafetyExec) {', '          if (false) {')]),
 ('BK11 banda 75 % inclusiva', 'src/break_core.hpp', [('  if (pct100 > 75LL * hp_max) return 5;', '  if (pct100 >= 75LL * hp_max) return 5;')]),
 ('BK12 rearme sin soltar el latch', 'src/posture_core.hpp', [('    e.posture = 0;\n    e.latched = false;\n    e.k = 0;', '    e.posture = 0;\n    e.k = 0;')]),
 ('BK13 recuperacion ignora la banda', 'src/posture_core.hpp', [('(e.regen_rate >= 1 && e.regen_rate <= 5 ? e.regen_rate : policy::kRegenRateDefault)', '(policy::kRegenRateDefault)')]),
 ('BK14 slot reciclado hereda el ciclo', 'src/break_core.hpp', [('    if (!s.bound || s.generation != v.generation || s.id != v.id) {  // generacion nueva', '    if (!s.bound) {  // generacion nueva')]),
 ('BK15 muerte no observada', 'src/break_core.hpp', [('    const bool dead_now = policy::death_mode(v.mode) || (v.native_hp <= 0 && v.mode != policy::kModeBig);', '    const bool dead_now = false;')]),
 ('BK16 commit ignora el fallo de escritura', 'src/break_core.hpp', [('    if (!ok) { ++c.write_failed; return; }', '    if (!ok) { ++c.write_failed; }')]),
 ('BK17 bonus sobre el golpe de ruptura', 'src/break_core.hpp', [('          else { s.phase = Phase::Punish; s.punish_start = s.last_sword = s.exec; s.frac = 0; s.due = false; a.why = Why::BreakStanding; }',
   '          else { s.phase = Phase::Punish; s.punish_start = s.last_sword = s.exec; s.frac = 0; s.due = false; a.why = Why::BreakStanding; if (v.pre_hp - v.native_hp >= 5) { a.act = Act::Bonus; a.extra = (v.pre_hp - v.native_hp) / 5; a.base = v.pre_hp - v.native_hp; a.new_hp = v.native_hp - a.extra; } }')]),
 ('BK18 caida en el tick de un golpe', 'src/break_core.hpp', [('        if (v.hit) break;  // otra familia en esta llamada', '        if (false) break;  // otra familia en esta llamada')]),
]
POSTURE += [(n, f, pairs, ('break_lab', 'posture_lab')) for (n, f, pairs) in BK]
# Violaciones plantadas en el codigo (deben ser RECHAZADAS por source_audit). (nombre, archivo, modo, texto)
SRC = [
 ('S01 escribir health', 'mod.cpp', 'do_pre', 'a->health = 1;'), ('S02 escribir mpCollider', 'mod.cpp', 'do_pre', 'a->mAtInfo.mpCollider = nullptr;'),
 ('S03 getter cacheador GetTgHitAc', 'mod.cpp', 'do_pre', '(void)a->mSphs_cc[0].GetTgHitAc();'), ('S04 HOOK_SKIP_ORIGINAL', 'mod.cpp', 'skip', 'return HOOK_SKIP_ORIGINAL;'),
 ('S05 lookup dentro de do_pre', 'mod.cpp', 'do_pre', '{ Pick q{}; (void)lookup_actor(q); }'), ('S06 leer mHitType', 'mod.cpp', 'do_pre', '(void)a->mAtInfo.mHitType;'),
 ('S07 segunda desreferencia de collider', 'mod.cpp', 'deref', 'type = reinterpret_cast<const cCcD_Obj*>(sel)->GetAtType();'),
 ('S08 ChkTgHit derivado', 'mod.cpp', 'do_pre', '(void)a->mSphs_cc[0].ChkTgHit();'), ('S09 setActionMode', 'mod.cpp', 'do_pre', 'a->setActionMode(6, 0);'),
 ('S10 IsBadReadPtr', 'mod.cpp', 'do_pre', '(void)IsBadReadPtr(nullptr, 1);'), ('S11 VirtualQuery', 'mod.cpp', 'do_pre', '(void)VirtualQuery(nullptr, nullptr, 0);'),
 ('S12 imprimir un puntero', 'mod.cpp', 'do_pre', 'char q[32]; std::snprintf(q, sizeof(q), "%p", (void*)a);'),
 ('S13 direccion en hex ancho', 'mod.cpp', 'do_pre', 'char q[32]; std::snprintf(q, sizeof(q), "%016llx", 0ull);'),
 ('S14 escribir en actor via out', 'mod.cpp', 'do_pre', 'fopAc_ac_c* out2 = nullptr; out2 = out; out->health = 0;'),
 ('S15 new en el camino caliente', 'mod.cpp', 'do_pre', 'int* leak = new int[4]; (void)leak;'),
 ('S16 std::vector en el nucleo', 'posture_core.hpp', 'top', '#include <vector>\nstatic std::vector<int> g_oops;'),
 ('S17 segundo DEFINE_HOOK', 'mod.cpp', 'top', 'DEFINE_HOOK(&daE_OC_c::cc_at_check, EocSecondHook);'),
 ('S18 dependencia de HM (servicio importado)', 'mod.cpp', 'top', 'IMPORT_SERVICE(EnemyHealthService, svc_hm);'),
 ('S19 std::cout', 'mod.cpp', 'do_pre', 'std::cout << 1;'),
 ('S20 cabecera del juego en el nucleo puro', 'posture_core.hpp', 'top', '#include <d/d_com_inf_game.h>'),
 ('S21 segundo sitio de avance', 'mod.cpp', 'do_pre', 'g.pe.px.advance();'),
 ('S22 fopen', 'mod.cpp', 'do_pre', '(void)fopen("x", "w");'),
 ('S23 mutex', 'posture_core.hpp', 'top', 'static std::mutex g_m;'),
 ('S24 puntero de actor almacenado en el sidecar', 'posture_core.hpp', 'top', 'struct Oops { std::uintptr_t kept; };'),
 ('S25 segundo consumo de eventos', 'mod.cpp', 'do_post', 'g.pe.px.consume(HitReport{}, g.eng.obs.ledger);'),
 ('S26 nombre de otro mod', 'posture_core.hpp', 'top', 'static const int kEnemyHealthMultiplier = 2;'),
 ('S27 memcpy sobre estado', 'posture_core.hpp', 'top', 'static void oops(void* a, const void* b) { std::memcpy(a, b, 4); }'),
 ('S28 escritura extra en restore_reaction', 'mod.cpp', 'restore', 'r->health = 0;'),
 ('S29 escritura de modo fuera de restore_reaction', 'mod.cpp', 'do_post', 'a->mOcState = 1;'),
 ('S30 escritura por otro alias de puntero', 'mod.cpp', 'do_post', 'daE_OC_c* q = a; q->mActionMode = 4;'),
 ('S31 llamada nativa setActionMode', 'mod.cpp', 'do_post', 'a->setActionMode(4, 0);'),
 ('S32 getter derivado ChkTgSet', 'mod.cpp', 'do_post', '(void)a->mSphs_cc[0].ChkTgSet();'),
 ('S33 segunda llamada al escritor', 'mod.cpp', 'do_post', 'restore_reaction(a, 4, 0, 3);'),
 ('S34 escritura de cooldown', 'mod.cpp', 'restore', 'r->field_0x6cc = 0;'),
 ('S35 incremento a traves del puntero', 'mod.cpp', 'do_post', '++a->health;'),
 # POST05-COMPAT
 ('S36 mod.cpp vuelve a V1', 'mod.cpp', 'sub', ('g.eng.policy = AdmissionPolicy::V2Compat;', 'g.eng.policy = AdmissionPolicy::V1Strict;')),
 ('S37 do_pre admite sin la politica del motor', 'mod.cpp', 'sub', ('is_candidate(admit(r.facts, g.eng.policy).adm)', 'is_candidate(admit(r.facts).adm)')),
 ('S38 segunda asignacion de politica', 'mod.cpp', 'do_post', 'g.eng.policy = AdmissionPolicy::V2Compat;'),
 ('S39 diferimiento de E_OC2 eliminado', 'reaction_core.hpp', 'sub', ('    if (e2 && !(e2_policy == E2Policy::MatureBC && e2_consistent(in))) return Reason::E_OC2SuppressionDeferred;\n', '')),
 ('S40 admit() por defecto V2', 'hit_core.hpp', 'sub', ('AdmissionPolicy pol = AdmissionPolicy::V1Strict', 'AdmissionPolicy pol = AdmissionPolicy::V2Compat')),
 ('S41 escritura de AtAtp', 'mod.cpp', 'do_post', 'a->mSphs_at[0].SetAtAtp(1);'),
 ('S42 E_OC2 elegible por atajo', 'reaction_core.hpp', 'sub', ('    if (e2 && !(e2_policy == E2Policy::MatureBC && e2_consistent(in))) return Reason::E_OC2SuppressionDeferred;\n', '    if (e2) return Reason::Eligible;\n')),
 # POST05-RB
 ('S43 escritura de rate de la animacion', 'mod.cpp', 'sub', ('    x.anim_frame = fc.getFrame();', '    x.anim_frame = fc.getFrame();\n    a->mpMorf->setPlaySpeed(1.0f);')),
 ('S44 sin politica E_OC2 MatureBC', 'mod.cpp', 'sub', ('  g.rx.e2_policy = post05::E2Policy::MatureBC;', '')),
 ('S45 segunda asignacion de politica E_OC2', 'mod.cpp', 'do_post', 'g.rx.e2_policy = post05::E2Policy::MatureBC;'),
 ('S46 lectura de speedF fuera de read_aux', 'mod.cpp', 'do_post', 'if (a->speedF > 99.0f) return;'),
 ('S47 diferimiento sin coherencia de senales', 'reaction_core.hpp', 'sub', ('if (e2 && !(e2_policy == E2Policy::MatureBC && e2_consistent(in)))', 'if (e2 && !(e2_policy == E2Policy::MatureBC))')),
 ('S48 gate de fase eliminado', 'reaction_core.hpp', 'sub', ('      if (!e2_allowed(phase, e2_env, r.aux_pre.atp, r.aux_pre.anim_rate, compat)) {', '      if (false) {')),
 ('S49 politica E_OC2 por defecto MatureBC', 'reaction_core.hpp', 'sub', ('E2Policy e2_policy = E2Policy::Deferred;', 'E2Policy e2_policy = E2Policy::MatureBC;')),
 ('S50 escritura del controlador de frames', 'mod.cpp', 'sub', ('    x.anim_end = fc.getEnd();', '    x.anim_end = fc.getEnd();\n    const_cast<J3DFrameCtrl&>(fc).mFrame = 0;')),
 ('S51 mpMorf leido en do_pre', 'mod.cpp', 'do_pre', 'if (!a->mpMorf) return;'),
 # POST05-FINALIZE
 ('S52 segunda consulta de servicio', 'mod.cpp', 'do_post', 'const void* x = nullptr; if (svc_host->get_service) svc_host->get_service(mod_ctx, "dev.devf.encounter_placement.v11", 1, 0, &x);'),
 ('S53 desreferencia del servicio consultado', 'mod.cpp', 'sub', ('    absent = rc == MOD_UNAVAILABLE && found == nullptr;', '    absent = rc == MOD_UNAVAILABLE && found == nullptr;\n    if (found) (void)*static_cast<const unsigned char*>(found);')),
 ('S54 entorno asignado en el hook', 'mod.cpp', 'do_post', 'g.rx.e2_env = post05::E2Env::Absent;'),
 ('S55 consulta desde el hook PRE', 'mod.cpp', 'do_pre', 'probe_variant_ai_env();'),
 ('S56 otro servicio consultado', 'mod.cpp', 'sub', ('"dev.devf.encounter_placement.v11"', '"dev.devf.native.v1"')),
 ('S57 entorno por defecto NATIVE_ONLY', 'reaction_core.hpp', 'sub', ('E2Env e2_env = E2Env::Possible;', 'E2Env e2_env = E2Env::Absent;')),
 # POST05-GENERAL
 ('S58 escritor toca la vida', 'mod.cpp', 'restore', 'r->health = 1;'),
 ('S59 escritor limpia el golpe Tg', 'mod.cpp', 'restore', 'r->mSphs_cc[0].ClrTgHit();'),
 ('S60 bit Tg fuera del escritor', 'mod.cpp', 'do_post', 'a->mSphs_cc[0].OnTgSetBit();'),
 ('S61 segunda asignacion de politica general', 'mod.cpp', 'do_post', 'g.rx.stagger_policy = post05::StaggerPolicy::General;'),
 ('S62 timer escrito sin plan', 'mod.cpp', 'sub', ('  r->field_0x6c0 = t.t6c0;', '  r->field_0x6c0 = 0;')),
 ('S63 condicion ATTACK reintroducida', 'reaction_core.hpp', 'sub', ('    if (!p::general_pre_mode_allowed(r.pre.mode)) return Reason::PreModeScripted;', '    if (r.pre.mode != p::kModeAttack) return Reason::PreModeNotAttack;\n    if (!p::general_pre_mode_allowed(r.pre.mode)) return Reason::PreModeScripted;')),
 ('S64 escritor llama a setActionMode', 'mod.cpp', 'restore', 'r->setActionMode(3, 1);'),
 ('S65 politica general eliminada', 'mod.cpp', 'sub', ('  g.rx.stagger_policy = post05::StaggerPolicy::General;', '')),
 # POST06
 ('S66 escritor de vida toca otro campo', 'mod.cpp', 'sub', ('  r->health = static_cast<s16>(hp);', '  r->health = static_cast<s16>(hp);\n  r->speedF = 0.0f;')),
 ('S67 segunda llamada al escritor de vida', 'mod.cpp', 'do_post', 'write_vitality(a, 1, false);'),
 ('S68 POST06 activado dos veces', 'mod.cpp', 'do_post', 'g.br.enabled = true;'),
 ('S69 escritor de reaccion fuera de sus sitios', 'mod.cpp', 'top', 'void bad_drop(daE_OC_c* a) noexcept { restore_reaction(a, post05::LiveMode{}); }'),
 ('S70 vida escrita fuera del escritor', 'mod.cpp', 'do_post', 'a->health = 1;'),
 ('S71 escritor de vida sin rama letal exacta', 'mod.cpp', 'sub', ('  if (lethal) r->attention_info.flags = 0;', '  if (lethal) r->attention_info.flags = 1;')),
]
def sh(cmd, cwd, timeout=90):
    try:
        return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:  # una mutacion que cuelga un laboratorio TAMBIEN esta detectada (el laboratorio no pasa)
        return subprocess.CompletedProcess(cmd, 124, '', 'TIMEOUT')
def build_run(dirp, labs):
    res = []
    for lab in labs:
        b = sh(['g++', '-std=c++20', '-O1', f'tests/{lab}.cpp', '-o', f'{lab}.bin'], dirp)
        if b.returncode: return 'COMPILE_ERROR', b.stderr[:300]
        r = sh([f'./{lab}.bin'], dirp)
        res.append(r.returncode)
        if r.returncode == 124: return 'DETECTED', 'TIMEOUT'
    return ('DETECTED' if any(res) else 'SURVIVED'), ''
def run_logic(item):
    name, rel, pairs, labs = item
    with tempfile.TemporaryDirectory() as td:
        d = Path(td); shutil.copytree(ROOT / 'src', d / 'src'); shutil.copytree(ROOT / 'tests', d / 'tests')
        f = d / rel; s = f.read_text()
        for old, new in pairs:
            if s.count(old) != 1: return (name, 'INVALID_MUTATION', f'pattern count {s.count(old)}: {old[:50]!r}')
            if 'last_global_' in new: s = s.replace('  Instance s_[kCapacity]{};', '  Instance s_[kCapacity]{};\n  std::uint64_t last_global_ = 0;')
            s = s.replace(old, new)
        f.write_text(s)
        if 'last_global_' in pairs[0][1]: f.write_text(f.read_text().replace('s.last_event_serial = serial;', 's.last_event_serial = serial; last_global_ = serial;'))
        st, why = build_run(d, labs)
        return (name, st, why)
def run_src(item):
    name, rel, mode, snippet = item
    with tempfile.TemporaryDirectory() as td:
        d = Path(td); shutil.copytree(ROOT / 'src', d / 'src')
        f = d / 'src' / rel; s = f.read_text()
        if mode == 'deref': s = s.replace('  type = reinterpret_cast<const cCcD_Obj*>(sel)->GetAtType();  // inline const', '  type = reinterpret_cast<const cCcD_Obj*>(sel)->GetAtType();\n  ' + snippet)
        elif mode == 'skip': s = s.replace('if (g.eng.obs.disabled || !args) return HOOK_CONTINUE;', 'if (g.eng.obs.disabled || !args) ' + snippet)
        elif mode == 'restore':
            anchor = 'void restore_reaction(daE_OC_c* r, const post05::LiveMode& t) noexcept {\n'
            if anchor not in s: return (name, 'INVALID_MUTATION', 'anchor missing')
            s = s.replace(anchor, anchor + '  ' + snippet + '\n', 1)
        elif mode == 'sub':
            old, new = snippet
            if s.count(old) != 1: return (name, 'INVALID_MUTATION', 'pattern count %d' % s.count(old))
            s = s.replace(old, new)
        elif mode == 'top': s = s.replace('namespace {', snippet + '\nnamespace {', 1) if rel == 'mod.cpp' else s.replace('namespace post04 {', snippet + '\nnamespace post04 {', 1)
        else:
            anchor = f'void {mode}(' + ('daE_OC_c* a) noexcept {\n')
            if anchor not in s: return (name, 'INVALID_MUTATION', 'anchor missing')
            s = s.replace(anchor, anchor + '  ' + snippet + '\n', 1)
        f.write_text(s)
        r = sh([sys.executable, '-I', str(ROOT / 'ci/source_audit.py'), str(d)], d)
        return (name, 'REJECTED' if r.returncode else 'SURVIVED', '')
def main():
    with ThreadPoolExecutor(max_workers=8) as ex:
        a = list(ex.map(run_logic, LOGIC + POSTURE))
        b = list(ex.map(run_src, SRC))
    out = a + b
    bad = sum(1 for n, st, w in out if st not in ('DETECTED', 'REJECTED'))
    for name, st, why in out: print(f'{st:18s} {name} {why}')
    print(json.dumps({'logic_mutations_inherited': len(LOGIC), 'posture_reaction_compat_mutations': len(POSTURE), 'compat_mutations': len(COMPAT), 'rb_mutations': len(RB), 'general_mutations': len(GEN), 'post06_mutations': len(BK), 'source_negatives': len(SRC), 'not_detected': bad}))
    return 1 if bad else 0
if __name__ == '__main__': sys.exit(main())
