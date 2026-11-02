"""POST05: auditoria estatica de PASIVIDAD sobre src/*.cpp|hpp (texto). No es una prueba formal: es una red contra regresiones.
Uso: source_audit.py <proyecto>  -> JSON; salida distinta de 0 si falla alguna regla."""
import json
import re
import sys
from pathlib import Path


def strip_comments(t):
    t = re.sub(r'/\*.*?\*/', ' ', t, flags=re.S)
    t = re.sub(r'//[^\n]*', ' ', t)
    return re.sub(r'"(?:\\.|[^"\\])*"', '""', t)  # cadenas fuera (los textos de log no cuentan como codigo)


FORBIDDEN_CALLS = ['setActionMode', 'offTgSph', 'cc_at_check', 'at_power_check', 'ClrTgHit', 'ResetTgHit', 'OffTgSetBit',
                   'OnTgSetBit', 'OffCoSetBit', 'OnCoSetBit', 'onDownFlg', 'offDownFlg', 'offCutDownHitFlg', 'onCutDownHitFlg',
                   'setStabPos', 'setBck', 'executeBigDamage', 'executeDamage', 'damage_check(', 'Move()', 'fopAcM_delete',
                   'dComIfGp_setHitMark', 'mDoAud', 'startCollisionSE', 'startCreatureVoice', 'SetAtAtp',
                   # POST05-RB: la animacion y la velocidad solo se LEEN
                   'setPlaySpeed', 'setFrame', 'setAnm', 'setRate(', 'setAttribute', 'setEnd(', 'setStart(', 'setLoop(', 'changeAnm', 'checkBck']
FORBIDDEN_TOKENS = ['HOOK_SKIP_ORIGINAL', 'replace<', 'DEFINE_HOOK_SYMBOL', 'VirtualProtect', 'WriteProcessMemory', 'funchook',
                    'memcpy', 'memset', 'const_cast', 'reinterpret_cast<std::uintptr_t*>',
                    'fpcM_Search', 'fpcEx_Search', 'fopAcIt_', 'g_fopAcTg_Queue', 'dComIfG_Ccsp', 'fopAcM_SearchByName', 'fopAcM_Search(',
                    'GetTgHitAc', 'GetAc(', 'IsBadReadPtr', 'IsBadWritePtr', 'VirtualQuery', '__try', '__except', 'SetUnhandledExceptionFilter', 'mHitType', 'mHitBit', 'mAttackPower', 'mpActor', 'mpSound', 'mHitDirection', 'mPowerType',
                    'mHitStatus', 'getCutType', 'daPy_getPlayerActorClass', 'mStts']
# POST04: nada de memoria dinamica, E/S, hilos ni dependencias de otros mods en el camino de la mod (memoria O(64) fija)
DYNAMIC = [r'\bnew\b', r'\bdelete\b', r'\bmalloc\b', r'\bcalloc\b', r'\brealloc\b', r'\bfree\s*\(', r'std::vector', r'std::map', r'std::unordered', r'std::string\b',
           r'std::unique_ptr', r'std::shared_ptr', r'make_unique', r'make_shared', r'std::function', r'std::mutex', r'std::thread', r'std::atomic', r'std::ofstream',
           r'std::ifstream', r'std::fstream', r'\bfopen\b', r'\bfwrite\b', r'std::cout', r'std::cerr', r'std::list', r'std::deque', r'std::set\b', r'(?<![a-z_])printf\s*\(']
OTHER_MODS = [r'EnemyHealth', r'enemy_health', r'HmService', r'HealthBars', r'bars_203', r'\bDEVF', r'devf', r'Bokoblin', r'bokoblin', r'Expansion']
LOCAL_INCLUDES = {'engine.hpp', 'hit_core.hpp', 'lifecycle_core.hpp', 'posture_core.hpp'}
STD_INCLUDES = {'cstddef', 'cstdint', 'cstdio', 'cstring', 'type_traits'}
ALLOWED_SERVICES = {'HostService', 'LogService', 'HookService'}
# Asignacion / incremento sobre un miembro alcanzado desde un puntero (a->x = ..., a->x[i].y += ..., ++a->x)
ASSIGN = re.compile(r'(?<![\w.])(?:a|out)\s*->\s*[\w\[\]\.>\-\(\)]*?\s*(=(?!=)|\+=|-=|\|=|&=|\^=|<<=|>>=|\+\+|--)')
PREINC = re.compile(r'(\+\+|--)\s*(?:a|out)\s*->')


def main(project):
    project = Path(project)
    res = {'files': [], 'errors': []}
    hook_defs = 0
    all_text = []
    for f in sorted((project / 'src').glob('*')):
        if f.suffix not in ('.cpp', '.hpp'):
            continue
        text = strip_comments(f.read_text())
        text = re.sub(r'static_assert\((?:[^;])*?\);', ' ', text, flags=re.S)  # los static_assert son de compilacion, no de ejecucion
        res['files'].append(f.name)
        raw_text = re.sub(r'/\*.*?\*/', ' ', f.read_text(), flags=re.S)
        raw_text = re.sub(r'//[^\n]*', ' ', raw_text)
        for pat in (r'%p\b', r'%[0-9]*(?:ll|l|z|I64|j|t)[xX]', r'%0?16[xX]', r'PRIx', r'PRIX'):
            for m in re.finditer(pat, raw_text):
                res['errors'].append('%s: pointer-width hex formatting is forbidden in log strings (%s)' % (f.name, m.group(0)))
        for pat in DYNAMIC:
            if re.search(pat, text):
                res['errors'].append('%s: dynamic allocation / IO / threading is forbidden (%s)' % (f.name, pat))
        for pat in OTHER_MODS:
            if re.search(pat, text):
                res['errors'].append('%s: dependency on another mod is forbidden (%s)' % (f.name, pat))
        if f.suffix == '.hpp':  # nucleos puros: solo biblioteca estandar y cabeceras locales; ninguna cabecera del juego o del SDK
            for inc in re.findall(r'#\s*include\s*[<"]([^>"]+)[>"]', raw_text):
                if inc not in LOCAL_INCLUDES and inc not in STD_INCLUDES:
                    res['errors'].append('%s: pure core includes a non-core header: %s' % (f.name, inc))
        if f.name == 'posture_core.hpp':
            for line in text.splitlines():
                if 'uintptr_t' in line and 'actor' not in line:
                    res['errors'].append('posture_core.hpp: the posture sidecar must not store addresses (uintptr_t outside a pass-through parameter)')
        # POST05-GENERAL: los setters inline de bits Tg/Co y HeadLock solo se permiten DENTRO de restore_reaction (se audita aparte)
        scan = re.sub(r'void restore_reaction\(daE_OC_c\* r, const post05::LiveMode& t\) noexcept \{.*?\n\}\n', ' ', text, flags=re.S) if f.name == 'mod.cpp' else text
        for t in FORBIDDEN_CALLS + FORBIDDEN_TOKENS + (['onHeadLockFlg', 'offHeadLockFlg', 'OffCoSetBit'] if f.name == 'mod.cpp' else []):
            if t in scan:
                res['errors'].append('%s: forbidden token %s' % (f.name, t))
        for m in ASSIGN.finditer(text):
            res['errors'].append('%s: member assignment through game pointer: %s' % (f.name, m.group(0)[:60]))
        for m in PREINC.finditer(text):
            res['errors'].append('%s: increment through game pointer: %s' % (f.name, m.group(0)[:60]))
        hook_defs += len(re.findall(r'\bDEFINE_HOOK\s*\(', text))
        all_text.append(text)
        if f.name == 'mod.cpp':
            for svc in re.findall(r'IMPORT_SERVICE\s*\(\s*(\w+)', text):
                if svc not in ALLOWED_SERVICES:
                    res['errors'].append('mod.cpp: imports service %s (only Host/Log/Hook are allowed)' % svc)
        if f.name == 'mod.cpp':
            for m in re.finditer(r'return\s+(HOOK_\w+)\s*;', text):
                if m.group(1) != 'HOOK_CONTINUE':
                    res['errors'].append('mod.cpp: pre-hook returns %s' % m.group(1))
            if len(re.findall(r'add_pre<', text)) != 1 or len(re.findall(r'add_post<', text)) != 1:
                res['errors'].append('mod.cpp: expected exactly one add_pre and one add_post')
            # La unica consulta de existencia: una llamada a fopAcM_SearchByID(, dentro de lookup_actor, invocada solo desde do_update (mod_update).
            calls = [m.start() for m in re.finditer(r'fopAcM_SearchByID\s*\(', text)]
            body = re.search(r'Lookup lookup_actor\(.*?\n}\n', text, flags=re.S)
            if len(calls) != 1 or not body or not (body.start() <= calls[0] < body.end()):
                res['errors'].append('mod.cpp: fopAcM_SearchByID must be called exactly once, inside lookup_actor')
            if len(re.findall(r'\blookup_actor\s*\(', text)) != 2:
                res['errors'].append('mod.cpp: lookup_actor must be defined once and called once (from do_update)')
            if len(re.findall(r'\bdo_update\s*\(', text)) != 2:
                res['errors'].append('mod.cpp: do_update must be defined once and called once (from mod_update)')
            # POST03: lecturas de collider
            if len(re.findall(r'mAtInfo\s*\.\s*mpCollider', text)) != 1 or len(re.findall(r'mAtInfo', text)) != 1:
                res['errors'].append('mod.cpp: mAtInfo may only be touched once, as mAtInfo.mpCollider (pointer value, in do_post)')
            for m in re.finditer(r'ChkTgHit', text):
                if not text[:m.start()].endswith('cCcD_ObjHitInf::'):
                    res['errors'].append('mod.cpp: ChkTgHit must be the raw base-class cCcD_ObjHitInf::ChkTgHit')
            # POST05: la UNICA escritura al juego. Toda asignacion a traves de '->' en mod.cpp debe estar en restore_reaction y ser una de las tres.
            # POST05-GENERAL: el escritor asigna SOLO los 5 campos de setActionMode (modo, estado, modo previo, timers) a partir de t, y llama SOLO a los
            # setters inline que setActionMode/offTgSph usan (On/OffTgSetBit, On/OffCoSetBit de mSphs_cc[0|1], on/offHeadLockFlg) sobre r.
            rr = re.search(r'void restore_reaction\(daE_OC_c\* r, const post05::LiveMode& t\) noexcept \{(.*?)\n\}\n', text, flags=re.S)
            writes = list(re.finditer(r'(\w+)\s*->\s*([\w\.\[\]]+)\s*(=(?!=)|\+=|-=|\|=|&=|\^=|<<=|>>=|\+\+|--)', text))
            allowed = {'mActionMode': 't.mode', 'mOcState': 't.state', 'mOldActionMode': 't.old_mode', 'field_0x6c0': 't.t6c0', 'field_0x6c2': 't.t6c2'}
            inside = [w for w in writes if rr and rr.start(1) <= w.start() < rr.end(1)]
            if not rr or len(inside) != 5 or {w.group(2) for w in inside} != set(allowed) or any(w.group(1) != 'r' or w.group(3) != '=' for w in inside):
                res['errors'].append('mod.cpp: restore_reaction must assign exactly mActionMode, mOcState, mOldActionMode, field_0x6c0, field_0x6c2 through r')
            elif any(not re.search(r'r->' + k + r'\s*=\s*' + re.escape(v) + r'\s*;', rr.group(1)) for k, v in allowed.items()):
                res['errors'].append('mod.cpp: restore_reaction fields must come from the plan target t')
            if rr:
                calls_rr = re.findall(r'r->([\w\[\]\.]+)\(\)', rr.group(1))
                ok_calls = {'onHeadLockFlg', 'offHeadLockFlg'} | {'mSphs_cc[%d].%s' % (i, s) for i in (0, 1) for s in ('OnTgSetBit', 'OffTgSetBit', 'OnCoSetBit', 'OffCoSetBit')}
                if sorted(calls_rr) != sorted(['onHeadLockFlg', 'offHeadLockFlg'] + ['mSphs_cc[%d].%s' % (i, s) for i in (0, 1) for s in ('OnTgSetBit', 'OffTgSetBit', 'OnCoSetBit', 'OffCoSetBit')]) or not set(calls_rr) <= ok_calls:
                    res['errors'].append('mod.cpp: restore_reaction may only call the inline Tg/Co set-bit and HeadLock setters, once each branch')
                if re.search(r'\b(health|field_0x6cc|mSphs_at|mpMorf|speedF|mAtInfo|ClrTgHit|setActionMode)\b', rr.group(1)):
                    res['errors'].append('mod.cpp: restore_reaction touches a field outside the reaction write set')
            if len(re.findall(r'\.stagger_policy\s*=(?!=)', text)) != 1 or len(re.findall(r'\bg\.rx\.stagger_policy\s*=\s*post05::StaggerPolicy::General\s*;', text)) != 1:
                res['errors'].append('mod.cpp: g.rx.stagger_policy must be set exactly once, to post05::StaggerPolicy::General')
            for w in writes:
                if w not in inside:
                    res['errors'].append('mod.cpp: write through a pointer outside restore_reaction: %s' % w.group(0)[:60])
            if len(re.findall(r'\brestore_reaction\s*\(', text)) != 2:
                res['errors'].append('mod.cpp: restore_reaction must be defined once and called once (from the POST05 writer)')
            # POST05-COMPAT: el mod usa la admision V2 (y solo la fija una vez); toda llamada a admit() del mod usa la politica del motor
            if len(re.findall(r'\.policy\s*=(?!=)', text)) != 1 or len(re.findall(r'\bg\.eng\.policy\s*=\s*AdmissionPolicy::V2Compat\s*;', text)) != 1:
                res['errors'].append('mod.cpp: g.eng.policy must be set exactly once, to AdmissionPolicy::V2Compat')
            adm_calls = re.findall(r'\badmit\s*\(([^)]*)\)', text)
            if not adm_calls or any(c.replace(' ', '') != 'r.facts,g.eng.policy' for c in adm_calls):
                res['errors'].append('mod.cpp: every admit() call must pass the engine policy (admit(r.facts, g.eng.policy))')
            # POST05-RB: la politica E_OC2 se fija UNA vez, a MatureBC; mpMorf/speedF solo se leen en read_aux (y la direccion de mFrameCtrl en layout)
            if len(re.findall(r'\.e2_policy\s*=(?!=)', text)) != 1 or len(re.findall(r'\bg\.rx\.e2_policy\s*=\s*post05::E2Policy::MatureBC\s*;', text)) != 1:
                res['errors'].append('mod.cpp: g.rx.e2_policy must be set exactly once, to post05::E2Policy::MatureBC')
            ra_ = re.search(r'NativeAux read_aux\(const daE_OC_c\* a\) noexcept \{.*?\n\}\n', text, flags=re.S)
            lm_ = re.search(r'bool layout_matches_binary\(daE_OC_c\* a\) noexcept \{.*?\n\}\n', text, flags=re.S)
            for tok in ('mpMorf', 'speedF', 'mFrameCtrl'):
                for m in re.finditer(r'\b' + tok + r'\b', text):
                    if not ((ra_ and ra_.start() <= m.start() < ra_.end()) or (tok != 'speedF' and lm_ and lm_.start() <= m.start() < lm_.end())):
                        res['errors'].append('mod.cpp: %s may only be read inside read_aux (or addressed in layout_matches_binary)' % tok)
            if re.search(r'\bfc\s*\.\s*\w+\s*(=(?!=)|\+=|-=)', text) or re.search(r'const\s+J3DFrameCtrl\s*&\s*fc', text) is None:
                res['errors'].append('mod.cpp: the frame controller must be bound as const J3DFrameCtrl& fc and never written')
            # POST05-FINALIZE: UNA consulta de servicio ajeno, solo de existencia, solo en probe_variant_ai_env (llamada una vez, desde do_update);
            # el resultado solo se compara con nulo; e2_env solo se asigna ahi. Contrato nuevo y acotado (ver FINALIZE/).
            pe_ = re.search(r'void probe_variant_ai_env\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            calls_ = [m.start() for m in re.finditer(r'get_service\b', text)]
            if not pe_ or len(calls_) != 2 or any(not (pe_.start() <= c < pe_.end()) for c in calls_):
                res['errors'].append('mod.cpp: get_service may only appear inside probe_variant_ai_env (one guarded call)')
            if pe_ and (len(re.findall(r'\bfound\b', pe_.group(0))) != 3 or '&found' not in pe_.group(0).replace(' ', '') or 'found==nullptr' not in pe_.group(0).replace(' ', '')):
                res['errors'].append('mod.cpp: the service pointer returned by get_service may only be compared with nullptr')
            if raw_text.count('"dev.devf.encounter_placement.v11"') != 1 or not re.search(r'get_service\(mod_ctx, kVariantAiRequiredService, 1, 0, &found\)', text):
                res['errors'].append('mod.cpp: get_service must query exactly kVariantAiRequiredService ("dev.devf.encounter_placement.v11", major 1)')
            for m in re.finditer(r'\.e2_env\s*=(?!=)', text):
                if not (pe_ and pe_.start() <= m.start() < pe_.end()):
                    res['errors'].append('mod.cpp: e2_env may only be assigned inside probe_variant_ai_env')
            if len(re.findall(r'\bprobe_variant_ai_env\s*\(', text)) != 2 or not re.search(r'void do_update\(\) noexcept \{\n  probe_variant_ai_env\(\);', text):
                res['errors'].append('mod.cpp: probe_variant_ai_env must be defined once and called once, first thing in do_update (mod_update only)')
            for m in re.finditer(r'Chk(?:Tg|Co)Set', text):
                if not text[:m.start()].endswith('cCcD_ObjHitInf::'):
                    res['errors'].append('mod.cpp: ChkTgSet/ChkCoSet must be the raw base-class const getters')
            lm = re.search(r'bool layout_matches_binary\(daE_OC_c\* a\) noexcept \{.*?\n\}\n', text, flags=re.S)
            gets = [m.start() for m in re.finditer(r'GetObj(?:Tg|Co)\s*\(', text)]
            if len(gets) != 4 or not lm or any(not (lm.start() <= g < lm.end()) for g in gets):
                res['errors'].append('mod.cpp: GetObjTg/GetObjCo may only be used (address only) inside layout_matches_binary')
            for m in re.finditer(r'GetTgHitObj', text):
                if not text[:m.start()].endswith('cCcD_ObjHitInf::'):
                    res['errors'].append('mod.cpp: GetTgHitObj must be the raw base-class cCcD_ObjHitInf::GetTgHitObj')
            if len(re.findall(r'GetAtType\s*\(', text)) != 1:
                res['errors'].append('mod.cpp: GetAtType() must be called exactly once')
            rt = re.search(r'bool read_at_type\(.*?\n}\n', text, flags=re.S)
            if not rt or 'GetAtType' not in rt.group(0) or len(re.findall(r'reinterpret_cast\s*<\s*const\s+cCcD_Obj\s*\*\s*>', text)) != 1:
                res['errors'].append('mod.cpp: the only collider dereference must be inside read_at_type')
            for cb in ('do_pre', 'do_post'):
                m = re.search(r'void ' + cb + r'\(.*?\n}\n', text, flags=re.S)
                if not m or 'lookup_actor' in m.group(0) or 'SearchByID' in m.group(0):
                    res['errors'].append('mod.cpp: lookups are forbidden inside ' + cb)
    blob = '\n'.join(all_text)
    # POST05-RB: E_OC2 solo llega a ELIGIBLE con la politica MatureBC, senales coherentes y el gate de fase, que va tras las reglas comunes y antes de seguridad
    rc = [t for t in all_text if 'Reason classify(' in t]
    if len(rc) != 1:
        res['errors'].append('reaction_core.hpp: classify() not found exactly once')
    else:
        body = re.search(r'Reason classify\(.*?\n  \}\n', rc[0], flags=re.S)
        b = body.group(0) if body else ''
        defer = b.find('if (e2 && !(e2_policy == E2Policy::MatureBC && e2_consistent(in))) return Reason::E_OC2SuppressionDeferred;')
        elig = b.find('return Reason::Eligible;')
        first_policy = b.find('return Reason::FamilyNotSword;')
        last_common = b.find('return Reason::PostStateNotOrdinary;')
        phase = b.find('if (!e2_allowed(phase, e2_env, r.aux_pre.atp, r.aux_pre.anim_rate, compat)) {')
        safety = b.find('if (const Reason s = common_safety(in, e2); s != Reason::Count) return s;')
        cand = safety + 1 if safety >= 0 and 'in.adm != (e2 ? Admission::E_OC2_CANDIDATE : Admission::E_OC_CANDIDATE)' in blob else -1
        anim = b.find('if (e2 && !same_anim(r.aux_pre, r.aux_post)) return Reason::PostAuxUnexpected;')
        power = b.find('return Reason::PowerChangedInCall;')
        if not body or min(defer, elig, first_policy, last_common, phase, safety, cand, anim, power) < 0 or \
                not (defer < first_policy < last_common < phase < safety < cand < anim < power < elig) or b.count('return Reason::Eligible;') != 1 or \
                'const bool e2 = any_e_oc2(in);' not in b:
            res['errors'].append('reaction_core.hpp: classify() order must be: E_OC2 deferral (unless MatureBC+consistent) < common policy < E_OC2 phase gate < '
                                 'safety (candidate per resource, same animation, power) < Eligible once, last')
    cs = re.search(r'Reason common_safety\(const DecisionInput& in, bool e2\) const noexcept \{.*?\n  \}\n', blob, flags=re.S)
    if not cs or [cs.group(0).find(x) >= 0 for x in ('return Reason::SessionDisabled;', 'return Reason::TokenMismatch;', 'return Reason::NotCandidateActive;',
            'return Reason::AdmissionEverUnstable;', 'return Reason::FrozenOrDead;', 'return Reason::PostSignatureMismatch;', 'return Reason::PreNotFresh;',
            'return Reason::ColliderNotFresh;', 'return Reason::AuxMissing;', 'return Reason::Count;')] != [True] * 10 or blob.count('common_safety(in, e2)') != 2:
        res['errors'].append('reaction_core.hpp: common_safety() must hold the shared identity/generation/signature/freshness/collider/aux checks and be used by both policies')
    if 'StaggerPolicy stagger_policy = StaggerPolicy::AttackOnly;' not in blob:
        res['errors'].append('reaction_core.hpp: Reaction::stagger_policy must default to AttackOnly (inherited laboratories keep the 0.5.3 policy)')
    gc = re.search(r'Reason classify_general\(.*?\n  \}\n', blob, flags=re.S)
    if not gc:
        res['errors'].append('reaction_core.hpp: classify_general() not found')
    else:
        g_ = gc.group(0)
        pos = [g_.find(s) for s in ('return Reason::NotConfirmed;', 'return Reason::Lethal;', 'return Reason::PostShapeUnknown;', 'return Reason::CrossesThreshold;',
                                    'return Reason::LatchedAfter;', 'return Reason::PostureSkipUnsafe;', 'return Reason::PreModeScripted;', 'if (const Reason s = common_safety(in, e2); s != Reason::Count) return s;',
                                    'return Reason::PostAuxUnexpected;', 'return Reason::PowerChangedInCall;', 'return Reason::Eligible;')]
        if min(pos) < 0 or pos != sorted(pos) or g_.count('return Reason::Eligible;') != 1 or 'PreModeNotAttack' in g_ or 'kModeAttack)' in g_.split('return Reason::PreModeScripted;')[0]:
            res['errors'].append('reaction_core.hpp: classify_general order must be confirmed < lethal < post shape < break/latch < posture skip < scripted mode < '
                                 'safety (signature, post aux, power) < Eligible once, last; no ATTACK condition')
    if 'E2Env e2_env = E2Env::Possible;' not in blob:
        res['errors'].append('reaction_core.hpp: Reaction::e2_env must default to Possible (conservative until the service probe proves absence)')
    if 'E2Policy e2_policy = E2Policy::Deferred;' not in blob:
        res['errors'].append('reaction_core.hpp: Reaction::e2_policy must default to Deferred (inherited laboratories keep the 0.5.1 E_OC2 behaviour)')
    if 'AdmissionPolicy pol = AdmissionPolicy::V1Strict' not in blob:
        res['errors'].append('hit_core.hpp: admit() must default to V1Strict (inherited laboratories keep the 0.5.0 admission)')
    # el reloj de postura: UN avance por mod_update y UN consumo por evento, ambos solo desde la envoltura
    if len(re.findall(r'\.advance\s*\(\s*\)', blob)) != 1 or len(re.findall(r'\bvoid\s+advance\s*\(', blob)) != 1:
        res['errors'].append('px.advance() must be defined once and called once (PostureEngine::on_update)')
    if len(re.findall(r'\.consume\s*\(', blob)) != 1 or len(re.findall(r'\b(?:void|ConsumeOutcome)\s+consume\s*\(', blob)) != 1:
        res['errors'].append('px.consume() must be defined once and called once (PostureEngine::on_post)')
    mc = [t for t in all_text if 'do_update' in t]
    if mc and len(re.findall(r'\bg\.pe\.on_update\s*\(', mc[0])) != 1:
        res['errors'].append('mod.cpp: g.pe.on_update() must be called exactly once (inside do_update)')
    # DEFINE_HOOK aparece en mod.cpp una sola vez (el nombre de objetivo se comprueba en el PE)
    if hook_defs != 1:
        res['errors'].append('DEFINE_HOOK count %d != 1' % hook_defs)
    res['define_hook_count'] = hook_defs
    res['status'] = 'SOURCE_AUDIT_PASS' if not res['errors'] else 'SOURCE_AUDIT_FAIL'
    print(json.dumps(res, indent=2))
    return 0 if not res['errors'] else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1]))
