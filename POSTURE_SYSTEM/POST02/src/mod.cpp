// Enemy Posture System, POST02: identidad por instancia (ID + direccion + GENERACION) y ciclo de vida, PASIVO (Dusklight 2.0.3, Windows x64).
// Un unico hook: daE_OC_c::damage_check (PRE siempre HOOK_CONTINUE + POST). La unica consulta extra es de SOLO LECTURA, corre en mod_update
// (fuera de fpcM_Management) y es fopAcM_SearchByID(id, &out) sobre un ID ya observado. Nunca se desreferencia un puntero guardado.
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
#include "lifecycle_core.hpp"

#ifndef POST02_SOURCE_COMMIT
#define POST02_SOURCE_COMMIT "unset"
#endif

DEFINE_MOD();
IMPORT_SERVICE(HostService, svc_host);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);
// El UNICO objetivo de hook de este modulo.
DEFINE_HOOK(&daE_OC_c::damage_check, EocDamageCheckPost02);

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
static_assert(MOD_OK == 0, "install_sequence assumes MOD_OK == 0");
// Consulta de existencia: la sobrecarga NO inline (f_op_actor_mng.cpp:152): 0 = ausente; 1 con out==NULL = en creacion; 1 con out = presente.
static_assert(std::is_same_v<decltype(fopAcM_SearchByID(std::declval<fpc_ProcID>(), std::declval<fopAc_ac_c**>())), s32>,
              "fopAcM_SearchByID(id, out) signature");
static_assert(std::is_same_v<fpc_ProcID, unsigned int>, "fpc_ProcID is u32");

namespace {

using namespace post02;

struct Runtime {
  Observer obs;
  bool installed = false;
  unsigned consecutive_log_errors = 0;
} g;

template <class T>
bool compatible(const T* p, unsigned major, unsigned minor) noexcept {
  return p && p->header.struct_size >= sizeof(T) && p->header.major_version == major && p->header.minor_version >= minor;
}

// ---- log (cap duro, fail-open) --------------------------------------------------------------------------------------
void emit(LogCat cat, LogLevel level, const char* line) noexcept {
  if (!svc_log || !svc_log->write) return;
  if (!g.obs.budget.take(cat, g.obs.ledger.c)) return;
  try {
    svc_log->write(mod_ctx, level, line);
    g.consecutive_log_errors = 0;
  } catch (...) {
    ++g.obs.ledger.c.log_errors;
    if (++g.consecutive_log_errors >= kDisableLogErrors) g.obs.disable("LOG_SERVICE_EXCEPTIONS");
  }
}

void emit_events(const Events& ev) noexcept {
  char line[kLogLineMax];
  for (unsigned i = 0; i < ev.n; ++i) {
    if (!g.obs.ledger.may_log(ev.e[i].kind)) continue;
    if (fmt_event(line, sizeof(line), ev.e[i], g.obs.ledger.c.calls) <= 0) continue;
    const bool warn = ev.e[i].kind == EvKind::CapacityExhausted || ev.e[i].kind == EvKind::InvalidKey || ev.e[i].kind == EvKind::Unverified;
    emit(LogCat::Life, warn ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
  }
}

void emit_disabled_once() noexcept {
  if (!g.obs.disabled || g.obs.disabled_logged) return;
  g.obs.disabled_logged = true;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "POST02_DISABLED reason=%s gameplay_untouched=1", g.obs.disabled_reason);
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
  s.cooldown = a->field_0x6cc;
  s.cutdown = a->checkCutDownHitFlg() ? 1 : 0;
  s.tg_bits = static_cast<std::uint8_t>((tg_hit(a->mSphs_cc[0]) ? 1 : 0) | (tg_hit(a->mSphs_cc[1]) ? 2 : 0));
}

// ---- callbacks del hook ------------------------------------------------------------------------------------------------
void do_pre(daE_OC_c* a) noexcept {
  Observer& o = g.obs;
  Counters& c = o.ledger.c;
  ++c.calls;
  const std::uintptr_t key = reinterpret_cast<std::uintptr_t>(a);
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(a));
  const ActorFacts facts = read_facts(a);
  const AdmissionResult adm = admit(facts);
  ++c.calls_adm[static_cast<unsigned>(adm.adm)];

  Events ev;
  const Observation obs = o.ledger.observe(id, key, adm.adm, o.epoch.epoch, c.calls, ev);

  Snapshot s{};
  PreClass cls = PreClass::UNKNOWN;
  bool detail = false;
  if (obs.tracked && adm.adm == Admission::E_OC_CANDIDATE) {
    detail = true;
    read_core(a, s);
    cls = classify_pre(true, s.cutdown != 0, s.cooldown, (s.tg_bits & 1) != 0, (s.tg_bits & 2) != 0);
    ++c.pre_class[static_cast<unsigned>(cls)];
  }
  if (CallFrame* fr = o.frames.push(key, id, c)) {
    fr->tracked = obs.tracked;
    fr->slot = obs.slot;
    fr->generation = obs.generation;
    fr->alive = facts.profile_ok && facts.live;
    fr->detail = detail;
    fr->cls = cls;
    fr->pre = s;
  }
  emit_events(ev);
}

void do_post(daE_OC_c* a) noexcept {
  Observer& o = g.obs;
  Counters& c = o.ledger.c;
  CallFrame fr;
  if (!o.frames.end(reinterpret_cast<std::uintptr_t>(a), fr, c)) return;  // sin PRE emparejado: nada que comparar
  if (!fr.tracked) return;                                                // omitido por capacidad o clave invalida
  if (!o.ledger.valid(fr.slot, fr.generation) || o.ledger.slot(static_cast<unsigned>(fr.slot)).id != fr.id) {
    ++c.post_generation_mismatch;  // la instancia se retiro o cambio de generacion entre PRE y POST: el snapshot no se usa
    return;
  }
  Events ev;
  if (fr.alive) {
    // Evidencia de muerte (HP<=0 o modo mortal nativo). Es una SENAL de estado, no un borrado.
    const std::int32_t hp = a->health, md = a->mActionMode;
    if (death_evidence(hp, md)) o.ledger.mark_death(fr.slot, fr.generation, o.epoch.epoch, ev);
  }
  emit_events(ev);
  if (!fr.detail) return;
  Snapshot post{};
  read_core(a, post);
  const Evaluation e = evaluate(fr.cls, fr.pre, post);
  char line[kLogLineMax];
  if (e.anomaly) {
    ++c.anomalies;
    std::snprintf(line, sizeof(line),
                  "POST02_ANOMALY call=%llu id=%u generation=%llu pre=%s hp=%d>%d mode=%d>%d state=%d>%d (state changed without TG/finish)",
                  static_cast<unsigned long long>(c.calls), fr.id, static_cast<unsigned long long>(fr.generation), name(fr.cls), fr.pre.health,
                  post.health, fr.pre.mode, post.mode, fr.pre.state, post.state);
    emit(LogCat::Other, LOG_LEVEL_WARN, line);
  }
  if (!e.emit) return;
  ++c.hit_events;
  if (e.processed) ++c.processed;
  switch (e.verdict) {
    case Verdict::ACCEPTED_LIKELY: ++c.accepted_likely; break;
    case Verdict::EARLY_RETURN_POSSIBLE: ++c.early_return; break;
    case Verdict::PROCESSED_NO_VISIBLE_CHANGE: ++c.no_visible; break;
    case Verdict::FINISH_PATH_OBSERVED: ++c.finish; break;
    default: break;
  }
  o.ledger.add_hit(fr.slot, fr.generation);
  fmt_hit(line, sizeof(line), c.calls, fr.id, fr.generation, o.ledger.slot(static_cast<unsigned>(fr.slot)).life, fr.cls, fr.pre, post, e);
  emit(LogCat::Hit, LOG_LEVEL_INFO, line);
}

HookAction on_pre(ModContext*, void* args, void*, void*) noexcept {
  if (g.obs.disabled || !args) return HOOK_CONTINUE;  // SIEMPRE HOOK_CONTINUE
  try {
    daE_OC_c* a = mods::arg<daE_OC_c*>(args, 0);
    if (a) do_pre(a);
  } catch (...) {
    g.obs.disable("EXCEPTION_IN_PRE");
  }
  return HOOK_CONTINUE;
}

void on_post(ModContext*, void* args, void*, void*) noexcept {
  if (g.obs.disabled || !args) return;
  try {
    daE_OC_c* a = mods::arg<daE_OC_c*>(args, 0);
    if (a) do_post(a);
  } catch (...) {
    g.obs.disable("EXCEPTION_IN_POST");
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

void do_update() noexcept {
  Observer& o = g.obs;
  o.epoch.update(stage_token(dComIfGp_getStartStageName()));
  if (!o.sweep_due()) return;
  for (unsigned q = 0; q < kSweepQuota; ++q) {
    Pick p;
    if (!o.ledger.next_sweep(p, o.sweep_stamp)) break;
    const Lookup r = lookup_actor(p);
    Events ev;
    o.ledger.apply_lookup(p, r, o.epoch.epoch, ev);
    emit_events(ev);
  }
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
  void* declared = EocDamageCheckPost02::resolved_target();
  const unsigned f = static_cast<unsigned>(flags);
  std::snprintf(line, sizeof(line),
                "POST02_TARGET resolve=%s declared_target_resolved=%d same_address=%d flags=0x%02x inline_sites_flag=%d "
                "multi_name_flag=%d (inline_sites_flag=1 => NOT_APPROVED)",
                result_text(r), declared ? 1 : 0, (declared && addr == declared) ? 1 : 0, f, (f & 0x20u) ? 1 : 0,
                (f & static_cast<unsigned>(HOOK_SYMBOL_MULTI_NAME)) ? 1 : 0);
  emit(LogCat::Other, (f & 0x20u) ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}

void emit_summary(const char* tag) noexcept {
  char line[kLogLineMax * 3];
  fmt_summary(line, sizeof(line), g.obs.ledger.c, g.obs.ledger.active(), g.obs.epoch.epoch, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
}

}  // namespace

extern "C" {

MOD_EXPORT ModResult mod_initialize(ModError* err) {
  char line[kLogLineMax];
  const bool log_ok = compatible(svc_log, 1, 0) && svc_log->write;
  if (!log_ok) return mods::set_error(err, MOD_UNSUPPORTED, "POST02: LogService incompatible");
  if (!mod_ctx || !compatible(svc_host, 2, 2) || !svc_host->version ||
      (std::strcmp(svc_host->version, "2.0.3") != 0 && std::strcmp(svc_host->version, "2.0.3.0") != 0)) {
    g.obs.disable("HOST_NOT_2_0_3");
    std::snprintf(line, sizeof(line), "POST02_HOOK_FAILED stage=HOST_CHECK reason=HOST_NOT_2_0_3 host=%s",
                  (svc_host && svc_host->version) ? svc_host->version : "UNKNOWN");
    emit(LogCat::Other, LOG_LEVEL_ERROR, line);
    return mods::set_error(err, MOD_UNSUPPORTED, "POST02 requires Dusklight 2.0.3");
  }
  if (!compatible(svc_hook, 1, 1) || !svc_hook->install || !svc_hook->add_pre || !svc_hook->add_post || !svc_hook->dispatch_pre ||
      !svc_hook->dispatch_post || !svc_hook->uninstall || !svc_hook->resolve) {
    g.obs.disable("HOOK_SERVICE_INCOMPATIBLE");
    emit(LogCat::Other, LOG_LEVEL_ERROR, "POST02_HOOK_FAILED stage=HOOK_SERVICE_CHECK reason=HOOK_SERVICE_INCOMPATIBLE");
    return mods::set_error(err, MOD_UNSUPPORTED, "POST02: HookService 1.1 required");
  }

  char build_id[48] = {};
  for (unsigned i = 0; svc_host->build_id && i < svc_host->build_id_len && i < 20; ++i)
    std::snprintf(build_id + i * 2, sizeof(build_id) - i * 2, "%02x", svc_host->build_id[i]);
  std::snprintf(line, sizeof(line),
                "POST02_INIT id=dev.posture.post02_instance_lifecycle version=%s source_commit=%s host=%s host_build_id=%s passive=1 "
                "hook_target=daE_OC_c::damage_check pmf_bytes=%u capacity=%u sweep_period_ticks=%u sweep_quota=%u log_cap=%u "
                "lookup=fopAcM_SearchByID(mod_update_only) at_type=OMITTED_NO_SAFE_ACTOR_GUARD",
                kModVersion, POST02_SOURCE_COMMIT, svc_host->version, build_id[0] ? build_id : "EMPTY",
                static_cast<unsigned>(sizeof(decltype(&daE_OC_c::damage_check))), kCapacity, kSweepPeriodTicks, kSweepQuota, kBudgetTotal);
  emit(LogCat::Other, LOG_LEVEL_INFO, line);
  log_target_resolution();

  const InstallOutcome out = install_sequence(
      [] { return static_cast<int>(mods::hook::add_post<EocDamageCheckPost02>(on_post)); },
      [] { return static_cast<int>(mods::hook::add_pre<EocDamageCheckPost02>(on_pre)); },
      [] { return static_cast<int>(mods::hook::uninstall<EocDamageCheckPost02>()); });
  if (out.state != InstallState::Ok) {
    g.obs.disable("HOOK_INSTALL_FAILED");  // aunque algun callback hubiese quedado registrado, no hara nada
    std::snprintf(line, sizeof(line), "POST02_HOOK_FAILED stage=%s post=%s pre=%s rollback=%s (no partial execution: callbacks disabled)",
                  name(out.state), result_text(out.post), result_text(out.pre), result_text(out.rollback));
    emit(LogCat::Other, LOG_LEVEL_ERROR, line);
    return mods::set_error(err, MOD_ERROR, "POST02: damage_check hook installation failed; mod inert");
  }
  g.installed = true;
  emit(LogCat::Other, LOG_LEVEL_INFO,
       "POST02_HOOK_INSTALLED target=daE_OC_c::damage_check post=MOD_OK pre=MOD_OK pre_returns=HOOK_CONTINUE_ALWAYS");
  return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
  // Sin escrituras en el juego. Epoch de contexto, barrido por presupuesto y resumen periodico, fuera del camino caliente del hook.
  emit_disabled_once();
  if (g.obs.disabled) return MOD_OK;
  try {
    do_update();
    if (g.obs.summary_due()) emit_summary("PERIODIC");
  } catch (...) {
    g.obs.disable("EXCEPTION_IN_UPDATE");
  }
  return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
  // SESSION_SHUTDOWN: no se consulta ni se desreferencia ninguna instancia.
  emit_summary("SHUTDOWN");
  emit(LogCat::Other, LOG_LEVEL_INFO, "POST02_SHUTDOWN hook_removal=HOST_DETACH instances_dereferenced=0");
  g.obs.disable("SHUTDOWN");  // el host retira los hooks al descargar el mod (hook_remove_mod)
  return MOD_OK;
}

}  // extern "C"
