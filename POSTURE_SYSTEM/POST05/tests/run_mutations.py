"""POST05: mutaciones de prueba SOBRE COPIAS TEMPORALES (herramienta local, no forma parte del modulo).
Cada mutacion de logica debe hacer FALLAR los laboratorios (hit/lifecycle/collider/posture); cada violacion de pasividad plantada en mod.cpp debe ser RECHAZADA por source_audit.
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

ALL_LABS = ('hit_lab', 'lifecycle_lab', 'collider_lab', 'posture_lab', 'reaction_lab')
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
 ('P17 el golpe no reinicia el retardo', 'src/posture_core.hpp', [('    e->k = 0;\n    e->last_hit_tick = now_;', '    e->last_hit_tick = now_;')]),
 ('P18 sin tope de saturacion', 'src/posture_core.hpp', [('    if (sum > policy::kMaxPosture) {\n      sum = policy::kMaxPosture;\n      ++c.saturations;\n    }\n', '')]),
 ('P19 sin guarda de underflow', 'src/posture_core.hpp', [('      if (e.posture == 0) continue;\n      ++e.k;', '      ++e.k;')]),
 ('P20 E_OC2 recibe sidecar', 'src/posture_core.hpp', [('      if (in.adm == Admission::E_OC_CANDIDATE) {\n        bind(i, in);', '      if (in.adm != Admission::UNKNOWN_UNSAFE) {\n        bind(i, in);')]),
 ('P21 doble avance por mod_update', 'src/posture_core.hpp', [('    px.sync(eng.obs.ledger);\n    px.advance();', '    px.sync(eng.obs.ledger);\n    px.advance();\n    px.advance();')]),
 ('P22 retiro no cierra unidades (conservacion)', 'src/posture_core.hpp', [('    c.units_closed += e.posture;\n', '')]),
 ('P23 el consumo ocurre tras sincronizar la muerte', 'src/posture_core.hpp', [('    if (has) last = px.consume(rep, eng.obs.ledger);  // ANTES de sincronizar la muerte de este mismo golpe: asi un golpe letal se omite como LETHAL\n    if (has || ev.n > 0) px.sync(eng.obs.ledger);', '    if (has || ev.n > 0) px.sync(eng.obs.ledger);\n    if (has) last = px.consume(rep, eng.obs.ledger);')]),
 ('P24 modo deshabilitado sigue acumulando', 'src/posture_core.hpp', [('    if (eng.obs.disabled) return false;\n    const bool has = eng.on_post', '    const bool has = eng.on_post')]),
 ('P25 el tick elegible del propio golpe cuenta', 'src/posture_core.hpp', [('    e->k = 0;\n    e->last_hit_tick = now_;', '    e->k = 1;\n    e->last_hit_tick = now_;')]),
 ('P26 regenera en cada barrido en vez de cada tick', 'src/posture_core.hpp', [('      if (e.k > policy::kRecoverDelayTicks && (e.k - policy::kRecoverDelayTicks) % policy::kRecoverPeriodTicks == 0) {', '      if (e.k > policy::kRecoverDelayTicks && (e.k - policy::kRecoverDelayTicks) % (policy::kRecoverPeriodTicks + 1) == 0) {')]),
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
 ('R12 firma P2 ignorada', R, [('    if (r.post.old_mode != r.pre.mode) return Reason::PostSignatureMismatch;\n', '')]),
 ('R13 salto cancelado (cooldown 3) suprimible', R, [('    if (r.post.cooldown != p::kCooldownOrdinary) return Reason::CooldownNotOrdinary;\n', '')]),
 ('R14 plan no restaura el modo', R, [('      d.plan.mode = r.pre.mode;\n', '      d.plan.mode = r.post.mode;\n')]),
 ('R15 mOldActionMode mal restaurado', R, [('      d.plan.old_mode = r.pre.old_mode;\n', '      d.plan.old_mode = r.post.old_mode;\n')]),
 ('R16 sin re-comprobacion antes de escribir', R, [('    if (now.mode != d.plan.expect_mode || now.state != d.plan.expect_state || now.old_mode != d.plan.expect_old) {', '    if (false) {')]),
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
            anchor = 'void restore_reaction(daE_OC_c* r, int mode, int state, int old_mode) noexcept {\n'
            if anchor not in s: return (name, 'INVALID_MUTATION', 'anchor missing')
            s = s.replace(anchor, anchor + '  ' + snippet + '\n', 1)
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
    print(json.dumps({'logic_mutations_inherited': len(LOGIC), 'posture_mutations': len(POSTURE), 'source_negatives': len(SRC), 'not_detected': bad}))
    return 1 if bad else 0
if __name__ == '__main__': sys.exit(main())
