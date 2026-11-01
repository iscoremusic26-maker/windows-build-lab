// Enemy Posture System, POST01: observador PASIVO de daE_OC_c::damage_check (Dusklight 2.0.3, Windows x64).
// Un unico hook (PRE siempre HOOK_CONTINUE + POST). Solo lee. No escribe nada del juego.
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
#include "observer_core.hpp"

#ifndef POST01_SOURCE_COMMIT
#define POST01_SOURCE_COMMIT "unset"
#endif

DEFINE_MOD();
IMPORT_SERVICE(HostService, svc_host);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);
// El UNICO objetivo de hook de este modulo.
DEFINE_HOOK(&daE_OC_c::damage_check, EocDamageCheckPost01);

// ---- ABI: tipos y offsets Win64 comprobados en compilacion --------------------------------------------------
// Offsets medidos en el dusklight.exe oficial (SHA ffa99203...0049) desplazando el desensamblado de
// daE_OC_c::damage_check/setActionMode (POST01 EVIDENCE.md). NO son los comentarios /* 0x.. */ de retail.
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

namespace {

using namespace post01;

struct Runtime {
  Observer obs;
  bool installed = false;
  unsigned consecutive_log_errors = 0;
} g;

constexpr AtMasks kMasks = {
    static_cast<std::uint32_t>(AT_TYPE_MASTER_SWORD) | static_cast<std::uint32_t>(AT_TYPE_NORMAL_SWORD),
    static_cast<std::uint32_t>(AT_TYPE_SHIELD_ATTACK),
    static_cast<std::uint32_t>(AT_TYPE_ARROW),
    static_cast<std::uint32_t>(AT_TYPE_BOMB),
    static_cast<std::uint32_t>(AT_TYPE_IRON_BALL),
    static_cast<std::uint32_t>(AT_TYPE_BOOMERANG),
    static_cast<std::uint32_t>(AT_TYPE_40),
    static_cast<std::uint32_t>(AT_TYPE_SLINGSHOT)};

template <class T>
bool compatible(const T* p, unsigned major, unsigned minor) noexcept {
  return p && p->header.struct_size >= sizeof(T) && p->header.major_version == major &&
         p->header.minor_version >= minor;
}

// ---- log (cap duro, fail-open) ---------------------------------------------------------------------------
void emit(LogCat cat, LogLevel level, const char* line) noexcept {
  if (!svc_log || !svc_log->write) return;
  if (!g.obs.budget.take(cat, g.obs.c)) return;
  try {
    svc_log->write(mod_ctx, level, line);
    g.consecutive_log_errors = 0;
  } catch (...) {
    ++g.obs.c.log_errors;
    if (++g.consecutive_log_errors >= kDisableLogErrors) g.obs.disable("LOG_SERVICE_EXCEPTIONS");
  }
}

void emit_disabled_once() noexcept {
  if (!g.obs.disabled || g.obs.disabled_logged) return;
  g.obs.disabled_logged = true;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "POST01_DISABLED reason=%s gameplay_untouched=1", g.obs.disabled_reason);
  emit(LogCat::Other, LOG_LEVEL_WARN, line);
}

// ---- lecturas (solo accesores const/inline; ninguna muta flags ni cachea) ----------------------------------
bool tg_hit(dCcD_Sph& s) noexcept {
  // Replica inline de dCcD_GObjInf::ChkTgHit() (d_cc_d.cpp:221-229) sin importarlo: bit TG crudo y, salvo NoActor, actor del hit.
  if (!s.cCcD_ObjHitInf::ChkTgHit()) return false;
  return s.ChkTgHitNoActor() || s.GetTgHitAc() != nullptr;
}

ActorFacts read_facts(daE_OC_c* a) noexcept {
  ActorFacts f;
  f.profile_ok = static_cast<int>(a->profname) == static_cast<int>(fpcNm_E_OC_e);
  f.live = a->state.init_state >= 0 && a->state.init_state != 3;  // 3 = en borrado (patron DEVF CP03)
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

// ---- callbacks ----------------------------------------------------------------------------------------------
void do_pre(daE_OC_c* a) noexcept {
  Observer& o = g.obs;
  ++o.c.calls;
  const std::uintptr_t key = reinterpret_cast<std::uintptr_t>(a);
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(a));
  const AdmissionResult adm = admit(read_facts(a));
  ++o.c.calls_adm[static_cast<unsigned>(adm.adm)];

  Snapshot s{};
  PreClass cls = PreClass::UNKNOWN;
  const bool detail = adm.adm == Admission::E_OC_CANDIDATE;
  if (detail) {
    read_core(a, s);
    // El collider del impacto se lee SOLO dentro de este callback (misma lectura que hace damage_check despues).
    const int src = tg_source(s.tg_bits);
    if (src >= 0) {
      cCcD_Obj* hit = a->mSphs_cc[src].cCcD_ObjHitInf::GetTgHitObj();
      if (hit) {
        s.at_type = hit->GetAtType();
        s.at_valid = true;
      } else {
        ++o.c.at_hit_obj_null;
      }
    }
    cls = classify_pre(true, s.cutdown != 0, s.cooldown, (s.tg_bits & 1) != 0, (s.tg_bits & 2) != 0);
    ++o.c.pre_class[static_cast<unsigned>(cls)];
  }
  if (Frame* fr = o.push(key, id)) {
    fr->adm = adm.adm;
    fr->cls = cls;
    fr->detail = detail;
    fr->pre = s;
  }

  const Touch t = o.touch(id, key);
  char line[kLogLineMax];
  if (t == Touch::Inserted) {
    fmt_seen(line, sizeof(line), o.c.calls, id, adm.adm, adm.reason, detail, s);
    emit(LogCat::Seen, LOG_LEVEL_INFO, line);
  } else if (t == Touch::PtrMismatch) {
    std::snprintf(line, sizeof(line), "POST01_ID_PTR_MISMATCH call=%llu actor_id=%u (same id seen at a different address)",
                  static_cast<unsigned long long>(o.c.calls), id);
    emit(LogCat::Other, LOG_LEVEL_WARN, line);
  }
}

void do_post(daE_OC_c* a) noexcept {
  Observer& o = g.obs;
  Frame fr;
  if (!o.end(reinterpret_cast<std::uintptr_t>(a), fr)) return;  // sin PRE emparejado: nada que comparar
  if (!fr.detail) return;
  Snapshot post{};
  read_core(a, post);
  const Evaluation e = evaluate(fr.cls, fr.pre, post);
  char line[kLogLineMax];
  if (e.anomaly) {
    ++o.c.anomalies;
    std::snprintf(line, sizeof(line),
                  "POST01_ANOMALY call=%llu actor_id=%u pre=%s hp=%d>%d mode=%d>%d state=%d>%d (state changed without TG/finish)",
                  static_cast<unsigned long long>(o.c.calls), fr.id, name(fr.cls), fr.pre.health, post.health,
                  fr.pre.mode, post.mode, fr.pre.state, post.state);
    emit(LogCat::Other, LOG_LEVEL_WARN, line);
  }
  if (!e.emit) return;
  ++o.c.hit_events;
  if (e.processed) ++o.c.processed;
  switch (e.verdict) {
    case Verdict::ACCEPTED_LIKELY: ++o.c.accepted_likely; break;
    case Verdict::EARLY_RETURN_POSSIBLE: ++o.c.early_return; break;
    case Verdict::PROCESSED_NO_VISIBLE_CHANGE: ++o.c.no_visible; break;
    case Verdict::FINISH_PATH_OBSERVED: ++o.c.finish; break;
    default: break;
  }
  fmt_hit(line, sizeof(line), o.c.calls, fr.id, fr.cls, fr.pre, post, e,
          fr.pre.at_valid ? classify_at(fr.pre.at_type, kMasks) : "UNKNOWN");
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
  void* declared = EocDamageCheckPost01::resolved_target();
  const unsigned f = static_cast<unsigned>(flags);
  std::snprintf(line, sizeof(line),
                "POST01_TARGET resolve=%s declared_target_resolved=%d same_address=%d flags=0x%02x inline_sites_flag=%d "
                "multi_name_flag=%d (inline_sites_flag=1 => G1 NOT_APPROVED)",
                result_text(r), declared ? 1 : 0, (declared && addr == declared) ? 1 : 0, f, (f & 0x20u) ? 1 : 0,
                (f & static_cast<unsigned>(HOOK_SYMBOL_MULTI_NAME)) ? 1 : 0);
  emit(LogCat::Other, (f & 0x20u) ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}

}  // namespace

extern "C" {

MOD_EXPORT ModResult mod_initialize(ModError* err) {
  char line[kLogLineMax];
  const bool log_ok = compatible(svc_log, 1, 0) && svc_log->write;
  if (!log_ok) return mods::set_error(err, MOD_UNSUPPORTED, "POST01: LogService incompatible");
  if (!mod_ctx || !compatible(svc_host, 2, 2) || !svc_host->version ||
      (std::strcmp(svc_host->version, "2.0.3") != 0 && std::strcmp(svc_host->version, "2.0.3.0") != 0)) {
    g.obs.disable("HOST_NOT_2_0_3");
    std::snprintf(line, sizeof(line), "POST01_HOOK_FAILED stage=HOST_CHECK reason=HOST_NOT_2_0_3 host=%s",
                  (svc_host && svc_host->version) ? svc_host->version : "UNKNOWN");
    emit(LogCat::Other, LOG_LEVEL_ERROR, line);
    return mods::set_error(err, MOD_UNSUPPORTED, "POST01 requires Dusklight 2.0.3");
  }
  if (!compatible(svc_hook, 1, 1) || !svc_hook->install || !svc_hook->add_pre || !svc_hook->add_post ||
      !svc_hook->dispatch_pre || !svc_hook->dispatch_post || !svc_hook->uninstall || !svc_hook->resolve) {
    g.obs.disable("HOOK_SERVICE_INCOMPATIBLE");
    emit(LogCat::Other, LOG_LEVEL_ERROR, "POST01_HOOK_FAILED stage=HOOK_SERVICE_CHECK reason=HOOK_SERVICE_INCOMPATIBLE");
    return mods::set_error(err, MOD_UNSUPPORTED, "POST01: HookService 1.1 required");
  }

  char build_id[48] = {};
  for (unsigned i = 0; svc_host->build_id && i < svc_host->build_id_len && i < 20; ++i)
    std::snprintf(build_id + i * 2, sizeof(build_id) - i * 2, "%02x", svc_host->build_id[i]);
  std::snprintf(line, sizeof(line),
                "POST01_INIT id=dev.posture.post01_passive_observer version=%s source_commit=%s host=%s host_build_id=%s "
                "passive=1 hook_target=daE_OC_c::damage_check pmf_bytes=%u log_cap=%u",
                kModVersion, POST01_SOURCE_COMMIT, svc_host->version, build_id[0] ? build_id : "EMPTY",
                static_cast<unsigned>(sizeof(decltype(&daE_OC_c::damage_check))), kBudgetTotal);
  emit(LogCat::Other, LOG_LEVEL_INFO, line);
  log_target_resolution();

  const InstallOutcome out = install_sequence(
      [] { return static_cast<int>(mods::hook::add_post<EocDamageCheckPost01>(on_post)); },
      [] { return static_cast<int>(mods::hook::add_pre<EocDamageCheckPost01>(on_pre)); },
      [] { return static_cast<int>(mods::hook::uninstall<EocDamageCheckPost01>()); });
  if (out.state != InstallState::Ok) {
    g.obs.disable("HOOK_INSTALL_FAILED");  // aunque algun callback hubiese quedado registrado, no hara nada
    std::snprintf(line, sizeof(line),
                  "POST01_HOOK_FAILED stage=%s post=%s pre=%s rollback=%s (no partial execution: callbacks disabled)",
                  name(out.state), result_text(out.post), result_text(out.pre), result_text(out.rollback));
    emit(LogCat::Other, LOG_LEVEL_ERROR, line);
    return mods::set_error(err, MOD_ERROR, "POST01: damage_check hook installation failed; mod inert");
  }
  g.installed = true;
  emit(LogCat::Other, LOG_LEVEL_INFO,
       "POST01_HOOK_INSTALLED target=daE_OC_c::damage_check post=MOD_OK pre=MOD_OK pre_returns=HOOK_CONTINUE_ALWAYS");
  return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
  // Sin escrituras en el juego. Solo resumen periodico y aviso de apagado (fuera del camino caliente).
  emit_disabled_once();
  if (!g.obs.disabled && g.obs.summary_due()) {
    char line[kLogLineMax * 2];
    fmt_summary(line, sizeof(line), g.obs.c, "PERIODIC");
    emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  }
  return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
  char line[kLogLineMax * 2];
  fmt_summary(line, sizeof(line), g.obs.c, "SHUTDOWN");
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  emit(LogCat::Other, LOG_LEVEL_INFO, "POST01_SHUTDOWN hook_removal=HOST_DETACH");
  g.obs.disable("SHUTDOWN");  // el host retira los hooks al descargar el mod (hook_remove_mod)
  return MOD_OK;
}

}  // extern "C"
