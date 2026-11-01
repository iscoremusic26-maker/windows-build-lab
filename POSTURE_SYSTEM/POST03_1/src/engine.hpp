#pragma once
// Enemy Posture System, POST03: MOTOR puro PRE/POST. mod.cpp solo LEE el juego y le pasa valores; todo el criterio de
// «un procesamiento = un evento» vive aqui para poder probarse con dobles. Nada de este archivo toca memoria del juego.
//
// Contrato de una llamada a damage_check:
//   on_pre : identidad (ledger) + clasificacion PRE + apila UN frame (token con serial monotono).
//   on_post: desapila el frame del mismo actor, valida generacion, pasa por la puerta de serial y emite COMO MAXIMO UN evento.
#include "lifecycle_core.hpp"

namespace post03 {

struct PreReads {
  ActorFacts facts{};
  Snapshot snap{};            // valido solo si facts.profile_ok && facts.live y el actor es candidato E_OC
  std::uintptr_t raw0 = 0, raw1 = 0;  // hit obj CRUDO de cada esfera (opacos; no se desreferencian)
};
struct PostReads {
  bool alive = false;                    // perfil y estado permiten leer miembros
  Snapshot snap{};                       // health/mode/state/cooldown/old_mode/cutdown/tg tras el original
  std::uintptr_t selected = 0;           // mAtInfo.mpCollider tras el original (valor opaco)
};

class Engine {
 public:
  Observer obs;
  AtMasks masks{};

  // PRE: devuelve el resultado de observar y rellena eventos de ciclo de vida.
  Observation on_pre(std::uintptr_t actor, std::uint32_t id, const PreReads& r, Events& ev) noexcept {
    Counters& c = obs.ledger.c;
    ++c.calls;
    const AdmissionResult adm = admit(r.facts);
    ++c.calls_adm[static_cast<unsigned>(adm.adm)];
    if (adm.adm == Admission::E_OC2_EXCLUDED || adm.adm == Admission::PROFILE_OTHER) ++c.excluded_calls;
    if (adm.adm == Admission::UNKNOWN_UNSAFE) ++c.unknown_unsafe_calls;

    const Observation o = obs.ledger.observe(id, actor, adm.adm, obs.epoch.epoch, c.calls, ev);
    const bool detail = o.tracked && adm.adm == Admission::E_OC_CANDIDATE;
    PreClass cls = PreClass::UNKNOWN;
    if (detail) {
      cls = classify_pre(true, r.snap.cutdown != 0, r.snap.cooldown, (r.snap.tg_bits & 1) != 0, (r.snap.tg_bits & 2) != 0);
      ++c.pre_class[static_cast<unsigned>(cls)];
      if (cls == PreClass::COOLDOWN_NO_PROCESS) {
        ++c.cooldown_blocked;
        if (r.snap.tg_bits & 3u) ++c.cooldown_with_tg;  // TG durante el bloqueo: NO es un impacto
      } else if (cls == PreClass::NO_TG_CANDIDATE) {
        ++c.no_tg;
      }
    }
    if (CallFrame* fr = obs.frames.push(actor, id, c)) {
      fr->tracked = o.tracked;
      fr->slot = o.slot;
      fr->generation = o.generation;
      fr->alive = r.facts.profile_ok && r.facts.live;
      fr->detail = detail;
      fr->cls = cls;
      fr->pre = r.snap;
      fr->raw0 = r.raw0;
      fr->raw1 = r.raw1;
      if (detail && cls == PreClass::TG_CANDIDATE) {
        fr->started = true;
        ++c.cand_started;
        ++c.raw_tg;
        if ((r.snap.tg_bits & 3u) == 3u) {
          ++c.dual_tg;
          if (r.raw0 != r.raw1) ++c.pair_distinct;
        }
      } else if (detail && cls == PreClass::CUT_DOWN_FINISH_PATH) {
        fr->started = true;
        ++c.finish_started;
      }
    }
    return o;
  }

  // POST. read_at(selected, type) hace la UNICA lectura de collider; solo se invoca si el veredicto lo autoriza.
  // Devuelve true si hay un evento de impacto que registrar (rep rellenado); `ev` recibe eventos de ciclo de vida (muerte vista).
  template <class ReadAt>
  bool on_post(std::uintptr_t actor, const PostReads& r, ReadAt read_at, Events& ev, HitReport& rep) noexcept {
    Counters& c = obs.ledger.c;
    CallFrame fr;
    if (!obs.frames.end(actor, fr, c)) return false;  // sin frame: nada que comparar (end() ya contabilizo orfano/duplicado)
    return finalize(fr, r, read_at, ev, rep);
  }

  // Cierra UN frame: como maximo un evento por token (serial). Es publico para poder probar el replay del mismo frame.
  template <class ReadAt>
  bool finalize(const CallFrame& fr, const PostReads& r, ReadAt read_at, Events& ev, HitReport& rep) noexcept {
    Counters& c = obs.ledger.c;
    if (!fr.tracked) return false;                    // omitido por capacidad o clave invalida
    const bool valid = obs.ledger.valid(fr.slot, fr.generation) && obs.ledger.slot(static_cast<unsigned>(fr.slot)).id == fr.id;
    if (!valid) {  // retirada/cambio de generacion entre PRE y POST: el snapshot NO se usa ni se asigna a la instancia nueva
      ++c.post_generation_mismatch;
      if (fr.started) ++c.cross_generation_drops;
      return false;
    }
    if (fr.alive && death_evidence(r.snap.health, r.snap.mode)) obs.ledger.mark_death(fr.slot, fr.generation, obs.epoch.epoch, ev);
    if (!fr.started) return false;
    if (!obs.ledger.try_emit(fr.slot, fr.generation, fr.serial)) return false;  // serial repetido: duplicado suprimido

    const ColliderRes col = fr.cls == PreClass::TG_CANDIDATE ? resolve_collider(r.selected, fr.raw0, fr.raw1, fr.pre.tg_bits) : ColliderRes{};
    Snapshot post = r.snap;
    post.tg_bits = r.snap.tg_bits;
    const Judgement j = judge(fr.cls, fr.pre, post, col);
    rep = HitReport{};
    rep.event = ++event_serial_;
    rep.call = c.calls;
    rep.generation = fr.generation;
    rep.id = fr.id;
    rep.life = obs.ledger.slot(static_cast<unsigned>(fr.slot)).life;
    rep.cls = fr.cls;
    rep.pre = fr.pre;
    rep.post = post;
    rep.j = j;
    rep.col = col;
    if (j.lethal) ++c.lethal;
    if (j.hp_changed) ++c.hp_changed;
    switch (j.kind) {
      case HitKind::FinishNative: ++c.finish; break;
      case HitKind::FinishUnexpected: ++c.finish_unexpected; ++c.anomalies; break;
      case HitKind::ProcessedMismatch: ++c.processed_mismatch; rep.fam_reason = "SIGNATURE_MISMATCH"; break;
      case HitKind::CandidateNotProcessed: ++c.not_processed; rep.fam_reason = "NOT_PROCESSED"; break;
      case HitKind::Confirmed: {
        ++c.confirmed;
        bool ok = false;
        std::uint32_t type = 0;
        if (j.read_collider) ok = read_at(r.selected, type);  // unica desreferencia del modulo
        ++c.col_reason[static_cast<unsigned>(col.reason)];
        ++c.sel_class[static_cast<unsigned>(col.sel_class)];
        ++c.raw0_class[static_cast<unsigned>(col.raw0_class)];
        ++c.raw1_class[static_cast<unsigned>(col.raw1_class)];
        if (!j.read_collider) {
          ++c.collider_unmatched;
          rep.fam_reason = name(col.reason);
        } else if (!ok) {
          rep.fam_reason = "AT_TYPE_READ_REFUSED";
        } else {
          rep.at_valid = true;
          rep.at_type = type;
          if (col.sel_class == PtrClass::Mod8Four) ++c.deref_mod8_4;
          else ++c.deref_mod8_0;
          rep.fam = classify_family(type, masks);
          if (rep.fam.family == Family::Unavailable) rep.fam_reason = rep.fam.reason;
        }
        if (rep.fam.family == Family::Unavailable) ++c.classification_unavailable;
        ++c.family_count[static_cast<unsigned>(rep.fam.family)];
        break;
      }
      default: return false;
    }
    return true;
  }

  // Conservacion aritmetica de frames con evento (candidatos TG + finish) al cerrar:
  //   cand_started + finish_started == judged + dup_serial_gate + cross_generation_drops + cand_discarded + finish_discarded + pending
  std::uint64_t judged() const noexcept {
    const Counters& c = obs.ledger.c;
    return c.confirmed + c.processed_mismatch + c.not_processed + c.finish + c.finish_unexpected;
  }
  bool conserved() const noexcept {
    const Counters& c = obs.ledger.c;
    return c.cand_started + c.finish_started ==
           judged() + c.dup_serial_gate + c.cross_generation_drops + c.cand_discarded + c.finish_discarded + obs.frames.pending_started();
  }

 private:
  std::uint64_t event_serial_ = 0;
};

}  // namespace post03
