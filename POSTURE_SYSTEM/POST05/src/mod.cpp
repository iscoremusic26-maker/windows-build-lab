// Enemy Posture System, POST05: supresion SELECTIVA de UNA reaccion ordinaria (DAMAGE) sobre el observador de POST03.1 y la postura de POST04.
// Unica escritura al juego: restore_reaction() devuelve mActionMode/mOcState/mOldActionMode a sus valores PRE de ESTA llamada, solo cuando
// reaction_core.hpp lo autoriza (espada no letal ponderada, ATTACK 0/1/2, cooldown 0->10, efectos extra de setActionMode no-ops, layout = binario).
// Todo lo demas sigue pasivo. POST04_BREAK_READY sigue siendo solo una linea de log.
// Un unico hook: daE_OC_c::damage_check (PRE siempre HOOK_CONTINUE + POST). La unica consulta extra es de SOLO LECTURA, corre en mod_update
// (fuera de fpcM_Management) y es fopAcM_SearchByID(id, &out) sobre un ID ya observado. Nunca se desreferencia un puntero guardado.
// La unica desreferencia de collider ocurre en el POST, sobre mAtInfo.mpCollider, solo tras un impacto PROCESADO y confirmado cuyo
// puntero coincide con el hit obj crudo de una esfera de la MISMA llamada (ver HIT_PATH_AUDIT.md).
// No llama a setActionMode, offTgSph, cc_at_check, mStts.Move, ClrTgHit ni a ningun mutador.
#include <mods/service.hpp>
#include <mods/svc/hook.hpp>
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <m_Do/m_Do_ext.h>
#include <d/actor/d_a_e_oc.h>
#include <d/d_com_inf_game.h>
#include <f_pc/f_pc_manager.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <utility>
#include "reaction_core.hpp"

#ifndef POST05_SOURCE_COMMIT
#define POST05_SOURCE_COMMIT "unset"
#endif

DEFINE_MOD();
IMPORT_SERVICE(HostService, svc_host);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);
// El UNICO objetivo de hook de este modulo.
DEFINE_HOOK(&daE_OC_c::damage_check, EocDamageCheckPost05);

// ---- ABI: tipos y offsets Win64 comprobados en compilacion (los mismos de POST01, medidos en el EXE oficial) -------------
static_assert(std::is_same_v<decltype(&daE_OC_c::damage_check), void (daE_OC_c::*)()>, "damage_check signature");
static_assert(sizeof(decltype(&daE_OC_c::damage_check)) == 8, "compact Windows x64 PMF required");
static_assert(std::is_same_v<decltype(fopAc_ac_c::health), s16>, "health type");
static_assert(std::is_same_v<decltype(daE_OC_c::field_0x6cc), s16>, "field_0x6cc type");
static_assert(std::is_same_v<decltype(daE_OC_c::mActionMode), int>, "mActionMode type");
static_assert(std::is_same_v<decltype(daE_OC_c::mOcState), int>, "mOcState type");
static_assert(offsetof(fopAc_ac_c, health) == 0x656, "fopAc_ac_c::health offset != exe");
static_assert(offsetof(fopAc_ac_c, actor_status) == 0x580, "fopAc_ac_c::actor_status offset != exe");
static_assert(offsetof(daE_OC_c, mActionMode) == 0x8c8, "daE_OC_c::mActionMode offset != exe");
static_assert(offsetof(daE_OC_c, mOcState) == 0x8d0, "daE_OC_c::mOcState offset != exe");
static_assert(offsetof(daE_OC_c, field_0x6cc) == 0x8ec, "daE_OC_c::field_0x6cc offset != exe");
// POST03: offsets nuevos medidos en el desensamblado de damage_check del EXE oficial (HIT_PATH_AUDIT.md): mOldActionMode 0x8cc (setActionMode),
// mAtInfo 0x1398 (el mov del GetTgHitObj nativo a mAtInfo.mpCollider, que es el primer campo de dCcU_AtInfo).
static_assert(std::is_same_v<decltype(daE_OC_c::mOldActionMode), int>, "mOldActionMode type");
static_assert(offsetof(daE_OC_c, mOldActionMode) == 0x8cc, "daE_OC_c::mOldActionMode offset != exe");
static_assert(offsetof(daE_OC_c, mAtInfo) == 0x1398, "daE_OC_c::mAtInfo offset != exe");
static_assert(offsetof(dCcU_AtInfo, mpCollider) == 0, "dCcU_AtInfo::mpCollider must be the first field");
static_assert(std::is_same_v<decltype(std::declval<const cCcD_Obj&>().GetAtType()), u32>, "GetAtType() is an inline const u32 getter");
static_assert(sizeof(decltype(std::declval<const cCcD_Obj&>().GetAtType())) == 4, "the only collider read is a 4-byte int");
static_assert(alignof(cCcD_Obj) == 8, "hypothesis recorded in the audit: cCcD_Obj is declared 8-aligned (the host heap only guarantees 4)");
static_assert(MOD_OK == 0, "install_sequence assumes MOD_OK == 0");
// POST05: campos que setActionMode escribe ademas del modo (binario 0x140e0d260: u32 en +0x8e0, and $0xff7f en +0x69a) y que deben haber sido no-ops.
static_assert(std::is_same_v<decltype(daE_OC_c::field_0x6c0), s16> && std::is_same_v<decltype(daE_OC_c::field_0x6c2), s16>, "timer types");
static_assert(offsetof(daE_OC_c, field_0x6c0) == 0x8e0 && offsetof(daE_OC_c, field_0x6c2) == 0x8e2, "field_0x6c0/6c2 offsets != exe (u32 write at +0x8e0)");
static_assert(std::is_same_v<decltype(fopEn_enemy_c::mFlags), u16> && offsetof(fopEn_enemy_c, mFlags) == 0x69a, "fopEn_enemy_c::mFlags != exe (+0x69a)");
static_assert(fopEn_enemy_c::fopEn_flag_HeadLock == 0x80, "HeadLock bit != exe (and $0xff7f)");
static_assert(sizeof(dCcD_Sph) == 0x1b8, "dCcD_Sph stride != exe (0xea0 - 0xce8)");
static_assert(std::is_polymorphic_v<cCcD_ObjTg> && std::is_polymorphic_v<cCcD_ObjCo>, "Tg/Co objects carry a vptr before mSPrm under MSVC x64");
// Consulta de existencia: la sobrecarga NO inline (f_op_actor_mng.cpp:152): 0 = ausente; 1 con out==NULL = en creacion; 1 con out = presente.
static_assert(std::is_same_v<decltype(fopAcM_SearchByID(std::declval<fpc_ProcID>(), std::declval<fopAc_ac_c**>())), s32>,
              "fopAcM_SearchByID(id, out) signature");
static_assert(std::is_same_v<fpc_ProcID, unsigned int>, "fpc_ProcID is u32");
// POST05-RB (0.5.2): fase nativa (SOLO LECTURA). Offsets medidos en el EXE 2.0.3 (SHA-256 ffa99203...0049):
//   executeAttack 0x140e06f90: mov 0x6d8(%rcx),%rax ; movss 0x2c(%rax) (getFrame) ; test %cl,0x21(%rax) + movss 0x28(%rax) (isStop) ;
//   lea 0x620(%rsi),%rcx -> cLib_chaseF(&speedF, 0, 1.0) (ATTACK 0) ; J3DFrameCtrl::update 0x1416b3410: attr +8, state +9, end +0xc, rate +0x10, frame +0x14.
static_assert(std::is_same_v<decltype(daE_OC_c::mpMorf), mDoExt_McaMorfSO*>, "mpMorf type");
static_assert(offsetof(daE_OC_c, mpMorf) == 0x6d8, "daE_OC_c::mpMorf offset != exe (+0x6d8)");
static_assert(std::is_base_of_v<mDoExt_morf_c, mDoExt_McaMorfSO>, "mDoExt_McaMorfSO derives from mDoExt_morf_c");
static_assert(offsetof(mDoExt_morf_c, mFrameCtrl) == 0x18, "mDoExt_morf_c::mFrameCtrl offset != exe (+0x18)");
static_assert(offsetof(J3DFrameCtrl, mAttribute) == 0x8 && offsetof(J3DFrameCtrl, mState) == 0x9 && offsetof(J3DFrameCtrl, mEnd) == 0xc &&
                  offsetof(J3DFrameCtrl, mRate) == 0x10 && offsetof(J3DFrameCtrl, mFrame) == 0x14,
              "J3DFrameCtrl layout != exe (update: +8 attr, +9 state, +0xc end, +0x10 rate, +0x14 frame)");
static_assert(std::is_same_v<decltype(J3DFrameCtrl::mEnd), s16> && std::is_same_v<decltype(J3DFrameCtrl::mRate), f32> &&
                  std::is_same_v<decltype(J3DFrameCtrl::mFrame), f32> && std::is_same_v<decltype(J3DFrameCtrl::mAttribute), u8> &&
                  std::is_same_v<decltype(J3DFrameCtrl::mState), u8>,
              "J3DFrameCtrl field types");
static_assert(std::is_same_v<decltype(fopAc_ac_c::speedF), f32> && offsetof(fopAc_ac_c, speedF) == 0x620, "fopAc_ac_c::speedF != exe (+0x620)");

namespace {

using namespace post03;

struct Runtime {
  post04::PostureEngine pe;
  Engine& eng = pe.eng;
  post05::Reaction rx;
  int layout = -1;  // -1 sin comprobar, 0 no coincide con el binario (supresion cerrada para la sesion), 1 coincide
  bool installed = false;
  unsigned consecutive_log_errors = 0;
  // POST05-FINALIZE: entorno de la IA de variantes (ver probe_variant_ai_env)
  bool env_possible = false;
  unsigned env_ticks = 0, env_queries = 0, env_lines = 0;
} g;

template <class T>
bool compatible(const T* p, unsigned major, unsigned minor) noexcept {
  return p && p->header.struct_size >= sizeof(T) && p->header.major_version == major && p->header.minor_version >= minor;
}

// ---- log (cap duro, fail-open) --------------------------------------------------------------------------------------
void emit(LogCat cat, LogLevel level, const char* line) noexcept {
  if (!svc_log || !svc_log->write) return;
  if (!g.eng.obs.budget.take(cat, g.eng.obs.ledger.c)) return;
  try {
    svc_log->write(mod_ctx, level, line);
    g.consecutive_log_errors = 0;
  } catch (...) {
    ++g.eng.obs.ledger.c.log_errors;
    if (++g.consecutive_log_errors >= kDisableLogErrors) g.eng.obs.disable("LOG_SERVICE_EXCEPTIONS");
  }
}

// Vacia la cola de eventos de postura (tamano fijo) respetando los topes por tipo y el presupuesto global de 500 lineas.
void drain_posture() noexcept {
  post04::PItem it;
  char line[kLogLineMax];
  while (g.pe.px.pop(it)) {
    if (!g.pe.px.log_take(it.kind)) continue;
    if (post04::fmt_item(line, sizeof(line), it) <= 0) continue;
    try {
      if (svc_log && svc_log->write) svc_log->write(mod_ctx, LOG_LEVEL_INFO, line);
    } catch (...) {
      ++g.eng.obs.ledger.c.log_errors;
      if (++g.consecutive_log_errors >= kDisableLogErrors) g.eng.obs.disable("LOG_SERVICE_EXCEPTIONS");
    }
  }
}

void emit_events(const Events& ev) noexcept {
  char line[kLogLineMax];
  for (unsigned i = 0; i < ev.n; ++i) {
    if (!g.eng.obs.ledger.may_log(ev.e[i].kind)) continue;
    if (fmt_event(line, sizeof(line), ev.e[i], g.eng.obs.ledger.c.calls) <= 0) continue;
    const bool warn = ev.e[i].kind == EvKind::CapacityExhausted || ev.e[i].kind == EvKind::InvalidKey || ev.e[i].kind == EvKind::Unverified;
    emit(LogCat::Life, warn ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
  }
}

void emit_disabled_once() noexcept {
  if (!g.eng.obs.disabled || g.eng.obs.disabled_logged) return;
  g.eng.obs.disabled_logged = true;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "POST03_DISABLED reason=%s gameplay_untouched=1", g.eng.obs.disabled_reason);
  emit(LogCat::Other, LOG_LEVEL_WARN, line);
}

// ---- lecturas (solo accesores const/inline; ninguna muta flags ni cachea) ---------------------------------------------
bool tg_hit(const dCcD_Sph& s) noexcept {
  // SOLO el bit TG crudo. NO se usa GetTgHitAc()/GetAc(): escribe una cache (d_cc_d.cpp:26-37).
  return s.cCcD_ObjHitInf::ChkTgHit() != 0;
}

ActorFacts read_facts(daE_OC_c* a) noexcept {
  ActorFacts f;
  f.profile_ok = static_cast<int>(a->profname) == static_cast<int>(fpcNm_E_OC_e);
  f.live = a->state.init_state >= 0 && a->state.init_state != 3;  // 3 = en borrado
  if (f.profile_ok && f.live) {
    f.name = classify_name(a->mName);  // lectura acotada a 6 bytes
    f.status_10000 = (a->actor_status & static_cast<u32>(fopAcStts_UNK_0x10000_e)) != 0;
    const int a0 = a->mSphs_at[0].GetAtAtp(), a1 = a->mSphs_at[1].GetAtAtp();
    f.atp = a0 == a1 ? a0 : -2;
  }
  return f;
}

void read_core(daE_OC_c* a, Snapshot& s) noexcept {
  s.health = a->health;
  s.mode = a->mActionMode;
  s.state = a->mOcState;
  s.old_mode = a->mOldActionMode;
  s.cooldown = a->field_0x6cc;
  s.cutdown = a->checkCutDownHitFlg() ? 1 : 0;
  s.tg_bits = static_cast<std::uint8_t>((tg_hit(a->mSphs_cc[0]) ? 1 : 0) | (tg_hit(a->mSphs_cc[1]) ? 2 : 0));
}

// ---- callbacks del hook ------------------------------------------------------------------------------------------------
constexpr AtMasks kMasks = {
    static_cast<std::uint32_t>(AT_TYPE_MASTER_SWORD) | static_cast<std::uint32_t>(AT_TYPE_NORMAL_SWORD),
    static_cast<std::uint32_t>(AT_TYPE_SHIELD_ATTACK),
    static_cast<std::uint32_t>(AT_TYPE_ARROW),
    static_cast<std::uint32_t>(AT_TYPE_BOMB),
    static_cast<std::uint32_t>(AT_TYPE_IRON_BALL),
    static_cast<std::uint32_t>(AT_TYPE_BOOMERANG),
    static_cast<std::uint32_t>(AT_TYPE_40),
    static_cast<std::uint32_t>(AT_TYPE_SLINGSHOT)};

// Hit obj CRUDO de una esfera (cCcD_ObjHitInf::GetTgHitObj, inline const): valor opaco, solo para comparar; 0 si el bit TG no esta.
std::uintptr_t raw_hit_obj(const dCcD_Sph& s) noexcept {
  return s.cCcD_ObjHitInf::ChkTgHit() ? reinterpret_cast<std::uintptr_t>(s.cCcD_ObjHitInf::GetTgHitObj()) : 0;
}

// UNICA desreferencia de collider del modulo. Solo la invoca el motor tras un HIT_CONFIRMED cuyo mAtInfo.mpCollider coincide con el hit obj
// crudo de una esfera de esta misma llamada: es el mismo objeto que damage_check acaba de desreferenciar (ChkAtType, d_a_e_oc.cpp:673-754).
bool read_at_type(std::uintptr_t sel, std::uint32_t& type) noexcept {
  if (!deref_alignment_ok(sel)) return false;  // el unico campo leido es un int de 4 bytes: basta alineacion de 4 (ver COLLIDER_ROOT_CAUSE_AUDIT.md)
  type = reinterpret_cast<const cCcD_Obj*>(sel)->GetAtType();  // inline const
  return true;
}

void emit_hit(const HitReport& rep) noexcept {
  Observer& o = g.eng.obs;
  if (!o.may_log_hit(rep.j.kind)) return;
  char line[kLogLineMax * 2];
  if (fmt_hit_event(line, sizeof(line), rep) <= 0) return;
  emit(LogCat::Hit, rep.j.kind == HitKind::FinishUnexpected ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}

// POST05: lecturas auxiliares (solo getters const; mismos campos que setActionMode toca ademas del modo).
NativeAux read_aux(const daE_OC_c* a) noexcept {
  NativeAux x;
  x.valid = 1;
  x.t6c0 = a->field_0x6c0;
  x.t6c2 = a->field_0x6c2;
  x.head_lock = a->checkHeadLockFlg() ? 1 : 0;
  x.set_bits = static_cast<std::uint8_t>((a->mSphs_cc[0].cCcD_ObjHitInf::ChkTgSet() ? 1 : 0) | (a->mSphs_cc[0].cCcD_ObjHitInf::ChkCoSet() ? 2 : 0) |
                                         (a->mSphs_cc[1].cCcD_ObjHitInf::ChkTgSet() ? 4 : 0) | (a->mSphs_cc[1].cCcD_ObjHitInf::ChkCoSet() ? 8 : 0));
  // POST05-COMPAT: potencia y recurso en ESTE instante (getters const; lectura de mName acotada a 6 bytes). Nunca se escriben.
  const int a0 = a->mSphs_at[0].GetAtAtp(), a1 = a->mSphs_at[1].GetAtAtp();
  x.atp = static_cast<std::int8_t>(a0 == a1 ? (a0 > 100 ? 100 : a0) : -2);
  x.resource = static_cast<std::uint8_t>(classify_name(a->mName));
  // POST05-RB: fase nativa (getters const inline de J3DFrameCtrl; mpMorf vive mientras vive el actor y action() lo usa en esta misma llamada).
  x.speed = a->speedF;
  if (const mDoExt_McaMorfSO* m = a->mpMorf) {
    const J3DFrameCtrl& fc = m->mFrameCtrl;
    x.anim_ok = 1;
    x.anim_attr = fc.getAttribute();
    x.anim_state = fc.getState();
    x.anim_end = fc.getEnd();
    x.anim_rate = fc.getRate();
    x.anim_frame = fc.getFrame();
  }
  return x;
}

// POST05: el layout que leen ChkTgSet/ChkCoSet debe ser EXACTAMENTE el que escribe setActionMode en el EXE (orl $1 en +0xce8/+0xd08/+0xea0/+0xec0;
// mSPrm va tras el vptr). Solo aritmetica de direcciones de subobjetos: no lee ni escribe nada. Si no coincide, la supresion queda cerrada.
bool layout_matches_binary(daE_OC_c* a) noexcept {
  const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(a);
  const auto at = [&](const void* p) { return reinterpret_cast<std::uintptr_t>(p) - base + sizeof(void*); };
  // POST05-RB: la base mDoExt_morf_c de mpMorf esta en desplazamiento 0 (el EXE lee frame en [mpMorf+0x2c] directamente).
  const mDoExt_McaMorfSO* m = a->mpMorf;
  const bool morf_ok = !m || reinterpret_cast<std::uintptr_t>(&m->mFrameCtrl) - reinterpret_cast<std::uintptr_t>(m) == 0x18;
  return at(&a->mSphs_cc[0].GetObjTg()) == 0xce8 && at(&a->mSphs_cc[0].GetObjCo()) == 0xd08 && at(&a->mSphs_cc[1].GetObjTg()) == 0xea0 &&
         at(&a->mSphs_cc[1].GetObjCo()) == 0xec0 && morf_ok;
}

post05::LiveMode read_live(const daE_OC_c* r) noexcept { return post05::LiveMode{r->mActionMode, r->mOcState, r->mOldActionMode}; }

// UNICA escritura al juego de todo el modulo: deshace exactamente la ultima setActionMode(DAMAGE) de damage_check (ver reaction_core.hpp).
void restore_reaction(daE_OC_c* r, int mode, int state, int old_mode) noexcept {
  r->mActionMode = mode;
  r->mOcState = state;
  r->mOldActionMode = old_mode;
}

void emit_decision(const HitReport& rep, const post05::DecisionResult& d, post05::Apply ap) noexcept {
  if (!d.counted || rep.j.kind != HitKind::Confirmed) return;  // Finish/no confirmados: solo contadores
  if (!g.rx.log_take()) return;
  char line[kLogLineMax];
  if (post05::fmt_decision(line, sizeof(line), rep, g.pe.last, d, ap) <= 0) return;
  emit(LogCat::Hit, ap == post05::Apply::VerifyFailed ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}

void do_pre(daE_OC_c* a) noexcept {
  PreReads r;
  r.facts = read_facts(a);
  if (is_candidate(admit(r.facts, g.eng.policy).adm)) {  // POST05-COMPAT: misma politica que el motor (V2Compat)
    read_core(a, r.snap);
    r.raw0 = raw_hit_obj(a->mSphs_cc[0]);
    r.raw1 = raw_hit_obj(a->mSphs_cc[1]);
    r.aux = read_aux(a);
  }
  Events ev;
  g.pe.on_pre(reinterpret_cast<std::uintptr_t>(a), static_cast<std::uint32_t>(fopAcM_GetID(a)), r, ev);
  emit_events(ev);
  drain_posture();
}

void do_post(daE_OC_c* a) noexcept {
  PostReads r;
  r.alive = static_cast<int>(a->profname) == static_cast<int>(fpcNm_E_OC_e) && a->state.init_state >= 0 && a->state.init_state != 3;
  if (r.alive) {
    read_core(a, r.snap);
    r.selected = reinterpret_cast<std::uintptr_t>(a->mAtInfo.mpCollider);  // valor del puntero; no se desreferencia aqui
    r.aux = read_aux(a);
  }
  Events ev;
  HitReport rep;
  const bool has_event = g.pe.on_post(reinterpret_cast<std::uintptr_t>(a), r, read_at_type, ev, rep);
  emit_events(ev);
  if (has_event) emit_hit(rep);
  drain_posture();
  if (!has_event) return;
  if (g.layout < 0) g.layout = layout_matches_binary(a) ? 1 : 0;
  if (g.layout == 0 && !g.rx.session_disabled()) g.rx.close_session();  // layout distinto del binario: cerrado (se informa en el resumen)
  post05::Apply ap = post05::Apply::NotAttempted;
  const post05::DecisionResult d = post05::process(g.rx, g.pe, rep, [a] { return read_live(a); },
                                                   [a](int m, int st, int o) { restore_reaction(a, m, st, o); }, ap);
  emit_decision(rep, d, ap);
}

HookAction on_pre(ModContext*, void* args, void*, void*) noexcept {
  if (g.eng.obs.disabled || !args) return HOOK_CONTINUE;  // SIEMPRE HOOK_CONTINUE
  try {
    daE_OC_c* a = mods::arg<daE_OC_c*>(args, 0);
    if (a) do_pre(a);
  } catch (...) {
    g.eng.obs.disable("EXCEPTION_IN_PRE");
  }
  return HOOK_CONTINUE;
}

void on_post(ModContext*, void* args, void*, void*) noexcept {
  if (g.eng.obs.disabled || !args) return;
  try {
    daE_OC_c* a = mods::arg<daE_OC_c*>(args, 0);
    if (a) do_post(a);
  } catch (...) {
    g.eng.obs.disable("EXCEPTION_IN_POST");
  }
}

// ---- consulta de existencia (SOLO en mod_update) -----------------------------------------------------------------------
// fopAcM_SearchByID(id,&out): fpcM_IsCreating(id) y, si no, recorrido de solo lectura de g_fopAcTg_Queue con fpcSch_JudgeByID.
// Todo actor entra en esa cola en su primera llamada a fopAc_Create y sale en fopAc_Delete tras el delete del actor
// (f_op_actor.cpp:453-454 / 421): ausente + no en creacion = actor ya borrado. `out` es un puntero VIVO recien devuelto, nunca el guardado.
Lookup lookup_actor(const Pick& p) noexcept {
  try {
    fopAc_ac_c* out = nullptr;
    const s32 rc = fopAcM_SearchByID(static_cast<fpc_ProcID>(p.id), &out);
    if (rc == 0) return Lookup::Absent;
    if (!out) return Lookup::Creating;
    if (reinterpret_cast<std::uintptr_t>(out) != p.ptr) return Lookup::PresentMismatchPtr;
    if (static_cast<int>(out->profname) != static_cast<int>(fpcNm_E_OC_e) || static_cast<std::uint32_t>(fopAcM_GetID(out)) != p.id)
      return Lookup::PresentMismatchProfile;
    return Lookup::PresentMatch;
  } catch (...) {
    return Lookup::Failed;
  }
}

// POST05-FINALIZE: UNICA consulta de servicio ajeno del modulo, SOLO de existencia y SOLO en mod_update (hilo del juego, fuera del hook).
// Bokoblin Expansion 0.17.0 importa este servicio de EVF como OBLIGATORIO; el SDK (mods/api.h) no carga un mod con un import obligatorio sin
// resolver. Por tanto "no publicado" => Expansion no esta activo => nadie decide x1.15/x2 alrededor de damage_check. El puntero devuelto nunca se
// usa: solo se compara con nulo. Cualquier otro resultado, excepcion o host sin get_service => EXPANSION_POSSIBLE. La presencia es pegajosa.
constexpr const char* kVariantAiRequiredService = "dev.devf.encounter_placement.v11";
constexpr unsigned kEnvProbePeriod = 30;
void probe_variant_ai_env() noexcept {
  if (g.env_possible || (g.env_ticks++ % kEnvProbePeriod) != 0) return;
  bool absent = false;
  try {
    const void* found = nullptr;
    const ModResult rc = svc_host->get_service ? svc_host->get_service(mod_ctx, kVariantAiRequiredService, 1, 0, &found) : MOD_ERROR;
    absent = rc == MOD_UNAVAILABLE && found == nullptr;
  } catch (...) {
    absent = false;
  }
  ++g.env_queries;
  const post05::E2Env before = g.rx.e2_env;
  if (!absent) g.env_possible = true;
  g.rx.e2_env = absent ? post05::E2Env::Absent : post05::E2Env::Possible;
  if (g.rx.e2_env != before || g.env_queries == 1) {
    if (g.env_lines >= 4) return;
    ++g.env_lines;
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "POST05_E2_ENV env=%s queries=%u sticky=%d basis=REQUIRED_IMPORT_OF_VARIANT_AI_ABSENT_OR_NOT", name(g.rx.e2_env), g.env_queries,
                  g.env_possible ? 1 : 0);
    emit(LogCat::Other, LOG_LEVEL_INFO, line);
  }
}

void do_update() noexcept {
  probe_variant_ai_env();
  Observer& o = g.eng.obs;
  o.epoch.update(stage_token(dComIfGp_getStartStageName()));
  if (o.sweep_due()) {  // barrido de existencia por presupuesto (cada 10 mod_update): NO es el reloj de la postura
    for (unsigned q = 0; q < kSweepQuota; ++q) {
      Pick p;
      if (!o.ledger.next_sweep(p, o.sweep_stamp)) break;
      const Lookup r = lookup_actor(p);
      Events ev;
      o.ledger.apply_lookup(p, r, o.epoch.epoch, ev);
      emit_events(ev);
    }
  }
  g.pe.on_update();  // UN tick matematico por mod_update, siempre DESPUES del barrido (una retirada no recupera en ese tick)
  drain_posture();
}

const char* result_text(int r) noexcept {
  switch (r) {
    case MOD_OK: return "MOD_OK";
    case MOD_ERROR: return "MOD_ERROR";
    case MOD_UNAVAILABLE: return "MOD_UNAVAILABLE";
    case MOD_UNSUPPORTED: return "MOD_UNSUPPORTED";
    case MOD_CONFLICT: return "MOD_CONFLICT";
    case MOD_INVALID_ARGUMENT: return "MOD_INVALID_ARGUMENT";
    default: return "MOD_OTHER";
  }
}

// Diagnostico de solo lectura del manifiesto de simbolos (no es un hook ni instala nada).
void log_target_resolution() noexcept {
  char line[kLogLineMax];
  void* addr = nullptr;
  HookSymbolFlags flags = static_cast<HookSymbolFlags>(0);
  int r = MOD_ERROR;
  try {
    r = static_cast<int>(svc_hook->resolve(mod_ctx, "daE_OC_c::damage_check", &addr, &flags));
  } catch (...) {
    r = MOD_ERROR;
  }
  void* declared = EocDamageCheckPost05::resolved_target();
  const unsigned f = static_cast<unsigned>(flags);
  std::snprintf(line, sizeof(line),
                "POST03_TARGET resolve=%s declared_target_resolved=%d same_address=%d flags=0x%02x inline_sites_flag=%d "
                "multi_name_flag=%d (inline_sites_flag=1 => NOT_APPROVED)",
                result_text(r), declared ? 1 : 0, (declared && addr == declared) ? 1 : 0, f, (f & 0x20u) ? 1 : 0,
                (f & static_cast<unsigned>(HOOK_SYMBOL_MULTI_NAME)) ? 1 : 0);
  emit(LogCat::Other, (f & 0x20u) ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}

void emit_summary(const char* tag) noexcept {
  Observer& o = g.eng.obs;
  char line[kLogLineMax * 4];
  fmt_summary(line, sizeof(line), o.ledger.c, o.ledger.active(), o.epoch.epoch, o.frames.pending_started(), tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  fmt_hit_summary(line, sizeof(line), o.ledger.c, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  fmt_collider_diag(line, sizeof(line), o.ledger.c, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  const Counters& c = o.ledger.c;
  std::snprintf(line, sizeof(line),
                "POST03_CONSERVATION v=0.4.0 tag=%s ok=%d started=%llu judged=%llu dup_serial_gate=%llu dup_post_orphan=%llu cross_generation=%llu "
                "discarded=%llu pending=%u",
                tag, g.eng.conserved() ? 1 : 0, static_cast<unsigned long long>(c.cand_started + c.finish_started),
                static_cast<unsigned long long>(g.eng.judged()), static_cast<unsigned long long>(c.dup_serial_gate),
                static_cast<unsigned long long>(c.dup_post_orphan), static_cast<unsigned long long>(c.cross_generation_drops),
                static_cast<unsigned long long>(c.cand_discarded + c.finish_discarded), o.frames.pending_started());
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  const post04::AuditResult au = g.pe.px.audit();
  post04::fmt_summary(line, sizeof(line), g.pe.px, au, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  post04::fmt_conservation(line, sizeof(line), g.pe.px, au, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  post05::fmt_summary(line, sizeof(line), g.rx, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  std::snprintf(line, sizeof(line), "POST05_LAYOUT tag=%s layout=%s", tag, g.layout < 0 ? "UNCHECKED" : (g.layout ? "MATCHES_EXE" : "MISMATCH_SUPPRESSION_CLOSED"));
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  post05::fmt_conservation(line, sizeof(line), g.rx, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
}

}  // namespace

extern "C" {

MOD_EXPORT ModResult mod_initialize(ModError* err) {
  char line[kLogLineMax];
  const bool log_ok = compatible(svc_log, 1, 0) && svc_log->write;
  if (!log_ok) return mods::set_error(err, MOD_UNSUPPORTED, "POST05: LogService incompatible");
  if (!mod_ctx || !compatible(svc_host, 2, 2) || !svc_host->version ||
      (std::strcmp(svc_host->version, "2.0.3") != 0 && std::strcmp(svc_host->version, "2.0.3.0") != 0)) {
    g.eng.obs.disable("HOST_NOT_2_0_3");
    std::snprintf(line, sizeof(line), "POST03_HOOK_FAILED stage=HOST_CHECK reason=HOST_NOT_2_0_3 host=%s",
                  (svc_host && svc_host->version) ? svc_host->version : "UNKNOWN");
    emit(LogCat::Other, LOG_LEVEL_ERROR, line);
    return mods::set_error(err, MOD_UNSUPPORTED, "POST05 requires Dusklight 2.0.3");
  }
  if (!compatible(svc_hook, 1, 1) || !svc_hook->install || !svc_hook->add_pre || !svc_hook->add_post || !svc_hook->dispatch_pre ||
      !svc_hook->dispatch_post || !svc_hook->uninstall || !svc_hook->resolve) {
    g.eng.obs.disable("HOOK_SERVICE_INCOMPATIBLE");
    emit(LogCat::Other, LOG_LEVEL_ERROR, "POST03_HOOK_FAILED stage=HOOK_SERVICE_CHECK reason=HOOK_SERVICE_INCOMPATIBLE");
    return mods::set_error(err, MOD_UNSUPPORTED, "POST05: HookService 1.1 required");
  }

  g.eng.masks = kMasks;  // (g.eng es g.pe.eng)
  g.eng.policy = AdmissionPolicy::V2Compat;  // POST05-COMPAT 0.5.1: E_OC 1/1|2/2 y E_OC2 {1,2,3,6} admitidos
  g.rx.e2_policy = post05::E2Policy::MatureBC;  // POST05-RB 0.5.2: E_OC2 sigue las reglas de E_OC mas el gate de fase nativa (B/C maduro o frenada)
  char build_id[48] = {};
  for (unsigned i = 0; svc_host->build_id && i < svc_host->build_id_len && i < 20; ++i)
    std::snprintf(build_id + i * 2, sizeof(build_id) - i * 2, "%02x", svc_host->build_id[i]);
  std::snprintf(line, sizeof(line),
                "POST03_1_INIT id=dev.posture.post05_selective_reactions version=%s source_commit=%s host=%s host_build_id=%s passive=1 "
                "hook_target=daE_OC_c::damage_check pmf_bytes=%u capacity=%u sweep_period_ticks=%u sweep_quota=%u log_cap=%u "
                "lookup=fopAcM_SearchByID(mod_update_only) hit_signature=cooldown_set+reaction_committed collider_read=mAtInfo.mpCollider_after_confirmed_only "
                "deref_gate=eq_raw_hit_obj+align4 sizeof_cCcD_Obj=%u alignof_cCcD_Obj=%u",
                kModVersion, POST05_SOURCE_COMMIT, svc_host->version, build_id[0] ? build_id : "EMPTY",
                static_cast<unsigned>(sizeof(decltype(&daE_OC_c::damage_check))), kCapacity, kSweepPeriodTicks, kSweepQuota, kBudgetTotal,
                static_cast<unsigned>(sizeof(cCcD_Obj)), static_cast<unsigned>(alignof(cCcD_Obj)));
  emit(LogCat::Other, LOG_LEVEL_INFO, line);
  std::snprintf(line, sizeof(line),
                "POST04_INIT id=dev.posture.post05_selective_reactions version=%s source_commit=%s policy_id=%s posture_core_passive=1 "
                "hook_target=daE_OC_c::damage_check hook_pre=HOOK_CONTINUE_ALWAYS hook_post=passive capacity=%u key=slot+generation+id "
                "max_posture=%d threshold=%d weights(sword/iron/all_other)=%d/%d/0 recover_delay_ticks=%llu recover_period_ticks=%llu "
                "timebase=MOD_UPDATE_SIM_TICK timebase_nominal_hz=30 timebase_basis=source_game_clock_time_scale_1 TIMEBASE_PAUSE_UNVERIFIED balance=PROVISIONAL log_budget_posture=%u",
                post04::kPostureVersion, POST05_SOURCE_COMMIT, post04::policy::kPolicyId, post04::kCap, post04::policy::kMaxPosture, post04::policy::kThreshold,
                post04::policy::kWeightSword, post04::policy::kWeightIronBall, static_cast<unsigned long long>(post04::policy::kRecoverDelayTicks),
                static_cast<unsigned long long>(post04::policy::kRecoverPeriodTicks), post04::Posture::kLogBudget);
  emit(LogCat::Other, LOG_LEVEL_INFO, line);
  std::snprintf(line, sizeof(line),
                "POST05_INIT version=%s source_commit=%s policy_id=%s writes_to_game=1 write_set=mActionMode+mOcState+mOldActionMode_restore_pre "
                "trigger=final_setActionMode_DAMAGE eligible=SWORD_nonlethal_weighted_below_100 pre=ATTACK_0_1_2 hook_skip_original=0 hp_untouched=1 "
                "admission_policy=%s e2_policy=%s e2_atp=2|3>6|6 e2_phase=BC_MATURE|ATK0_BRAKE+EARLY_IF_NATIVE_ONLY|X1.15|BLACK3 power_change=SKIP_UNSAFE",
                post05::kReactionVersion, POST05_SOURCE_COMMIT, post05::policy::kPolicyId, name(g.eng.policy), name(g.rx.e2_policy));
  emit(LogCat::Other, LOG_LEVEL_INFO, line);
  log_target_resolution();

  const InstallOutcome out = install_sequence(
      [] { return static_cast<int>(mods::hook::add_post<EocDamageCheckPost05>(on_post)); },
      [] { return static_cast<int>(mods::hook::add_pre<EocDamageCheckPost05>(on_pre)); },
      [] { return static_cast<int>(mods::hook::uninstall<EocDamageCheckPost05>()); });
  if (out.state != InstallState::Ok) {
    g.eng.obs.disable("HOOK_INSTALL_FAILED");  // aunque algun callback hubiese quedado registrado, no hara nada
    std::snprintf(line, sizeof(line), "POST03_HOOK_FAILED stage=%s post=%s pre=%s rollback=%s (no partial execution: callbacks disabled)",
                  name(out.state), result_text(out.post), result_text(out.pre), result_text(out.rollback));
    emit(LogCat::Other, LOG_LEVEL_ERROR, line);
    return mods::set_error(err, MOD_ERROR, "POST05: damage_check hook installation failed; mod inert");
  }
  g.installed = true;
  emit(LogCat::Other, LOG_LEVEL_INFO,
       "POST03_HOOK_INSTALLED target=daE_OC_c::damage_check post=MOD_OK pre=MOD_OK pre_returns=HOOK_CONTINUE_ALWAYS");
  return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
  // Sin escrituras en el juego. Epoch de contexto, barrido por presupuesto y resumen periodico, fuera del camino caliente del hook.
  emit_disabled_once();
  if (g.eng.obs.disabled) return MOD_OK;
  try {
    do_update();
    if (g.eng.obs.summary_due()) emit_summary("PERIODIC");
  } catch (...) {
    g.eng.obs.disable("EXCEPTION_IN_UPDATE");
  }
  return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
  // SESSION_SHUTDOWN: no se consulta ni se desreferencia ninguna instancia.
  g.pe.on_shutdown();  // cierra carreras de recuperacion abiertas; solo aritmetica sobre el sidecar
  drain_posture();
  emit_summary("SHUTDOWN");
  emit(LogCat::Other, LOG_LEVEL_INFO, "POST03_SHUTDOWN hook_removal=HOST_DETACH instances_dereferenced=0");
  g.eng.obs.disable("SHUTDOWN");  // el host retira los hooks al descargar el mod (hook_remove_mod)
  return MOD_OK;
}

}  // extern "C"
