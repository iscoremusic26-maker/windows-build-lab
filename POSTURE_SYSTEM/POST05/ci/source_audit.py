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
                    'fpcM_Search', 'fpcEx_Search', 'fopAcIt_', 'dComIfG_Ccsp', 'fopAcM_SearchByName', 'fopAcM_Search(',
                    'GetTgHitAc', 'GetAc(', 'IsBadReadPtr', 'IsBadWritePtr', 'VirtualQuery', '__try', '__except', 'SetUnhandledExceptionFilter', 'mHitType', 'mHitBit', 'mAttackPower', 'mpActor', 'mpSound', 'mHitDirection', 'mPowerType',
                    'mHitStatus', 'getCutType', 'daPy_getPlayerActorClass', 'mStts']
# POST04: nada de memoria dinamica, E/S, hilos ni dependencias de otros mods en el camino de la mod (memoria O(64) fija)
DYNAMIC = [r'\bnew\b', r'\bdelete\b', r'\bmalloc\b', r'\bcalloc\b', r'\brealloc\b', r'\bfree\s*\(', r'std::vector', r'std::map', r'std::unordered', r'std::string\b',
           r'std::unique_ptr', r'std::shared_ptr', r'make_unique', r'make_shared', r'std::function', r'std::mutex', r'std::thread', r'std::atomic', r'std::ofstream',
           r'std::ifstream', r'std::fstream', r'\bfopen\b', r'\bfwrite\b', r'std::cout', r'std::cerr', r'std::list', r'std::deque', r'std::set\b', r'(?<![a-z_])printf\s*\(']
OTHER_MODS = [r'EnemyHealth', r'enemy_health', r'HmService', r'HealthBars', r'bars_203', r'\bDEVF', r'devf', r'Bokoblin', r'bokoblin', r'Expansion']
LOCAL_INCLUDES = {'engine.hpp', 'hit_core.hpp', 'lifecycle_core.hpp', 'posture_core.hpp', 'reaction_core.hpp', 'hud_core.hpp', 'parry_core.hpp', 'lnk_core.hpp'}
# LNK01: la supresion de prompts es SOLO visual y focal: ni ocultar todo el HUD, ni alfas/padres globales, ni busquedas de panes, ni volver a mostrar nada.
LNK01_FORBIDDEN = ['->show(', '.show(', 'hideAll', '2dShowOff', 'mParentAlpha', 'setAlphaRate', 'g_drawHIO', 'search(', 'emphasisButtonDelete',
                   'mpParent', 'field_0x2fd5', 'mpButtonParent', 'mpMeterDraw->mpScreen->hide', 'getMainScreenPtr', 'mStatus =', 'setString(', 'mpTextScreen',
                   'mpScreen->hide', 'dMeter2Info_set', 'dComIfGp_set', 'mDoAud', 'setLife', 'setMaxLife', 'onNoResetFlg', 'offNoResetFlg', 'procGuard', 'setItemLifeCount',
                   'setDamagePoint', 'clearItemLifeCount', 'setDamageColorTime']
STD_INCLUDES = {'cstddef', 'cstdint', 'cstdio', 'cstring', 'type_traits'}
ALLOWED_SERVICES = {'HostService', 'LogService', 'HookService'}
ALLOWED_OPTIONAL_SERVICES = {'InterpService'}  # POST07: presentation interpolation of the HUD anchor only
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
            if t in scan.replace('DEFINE_HOOK(&daE_OC_c::executeDamage, EocShieldStunPost06);', ''):
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
            pres = sorted(re.findall(r'add_pre<(\w+)>', text)); posts = sorted(re.findall(r'add_post<(\w+)>', text))
            if pres != ['EocDamageCheckPost05', 'MeterDeletePost07', 'PromptDrawLnk01'] or posts != ['EocDamageCheckPost05', 'EocShieldStunPost06', 'MeterDrawPost07', 'MeterExecutePost07', 'PromptDrawLnk01']:
                res['errors'].append('mod.cpp: unexpected hook registrations pre=%s post=%s' % (pres, posts))
            for svc in re.findall(r'IMPORT_OPTIONAL_SERVICE\w*\s*\(\s*(\w+)', text):
                if svc not in ALLOWED_OPTIONAL_SERVICES:
                    res['errors'].append('mod.cpp: optional service %s is not allowed' % svc)
            hooks_declared = re.findall(r'DEFINE_HOOK\(&(\w+)::(\w+),\s*(\w+)\)', text)
            if sorted(hooks_declared) != sorted([('daE_OC_c', 'damage_check', 'EocDamageCheckPost05'), ('daE_OC_c', 'executeDamage', 'EocShieldStunPost06'),
                                                 ('dMeter2_c', '_execute', 'MeterExecutePost07'), ('dMeter2_c', '_draw', 'MeterDrawPost07'), ('dMeter2_c', '_delete', 'MeterDeletePost07'),
                                                 ('dMeterButton_c', 'draw', 'PromptDrawLnk01')]):
                res['errors'].append('mod.cpp: unexpected hook targets %s' % hooks_declared)
            # POST08 PARRY: lecturas crudas en post08_judge; UNA escritura (post08_deflect = restore_reaction del PRE con estado 3) llamada una vez desde
            # post08_step; postura solo por preview_parry/apply_parry (una llamada cada una, en post08_step); nada escribe vida, Link ni animacion.
            pj = re.search(r'void post08_judge\(.*?\n\}\n', text, flags=re.S)
            ps = re.search(r'bool post08_step\(.*?\n\}\n', text, flags=re.S)
            if not pj or not ps:
                res['errors'].append('mod.cpp: post08_judge/post08_step not found')
            else:
                for m in re.finditer(r'(ChkAtHit|GetAtHitObj)\s*\(', text):
                    if not text[:m.start()].endswith('cCcD_ObjHitInf::'):
                        res['errors'].append('mod.cpp: %s must be the raw base-class cCcD_ObjHitInf getter' % m.group(1))
                    elif not (pj.start() <= m.start() < pj.end()):
                        res['errors'].append('mod.cpp: At-hit reads only inside post08_judge')
                if len(re.findall(r'ChkAtShieldHit\s*\(', text)) != 1 or 'ChkAtShieldHit' not in pj.group(0):
                    res['errors'].append('mod.cpp: ChkAtShieldHit read exactly once, inside post08_judge')
                for tok in ('GetAtHitAc', 'GetAtHitGObj', 'restore_reaction', 'write_vitality', '.rearm(', '.consume(', 'apply_parry', 'post08_deflect'):
                    if tok in pj.group(0):
                        res['errors'].append('mod.cpp: post08_judge is read-only (found %s)' % tok)
                cd = [m.start() for m in re.finditer(r'\bpost08_deflect\s*\(', text)]
                if len(cd) != 2 or not (ps.start() <= cd[1] < ps.end()):
                    res['errors'].append('mod.cpp: post08_deflect must be defined once and called once, from post08_step')
                for fn in ('apply_parry', 'preview_parry'):
                    cs = [m.start() for m in re.finditer(r'\.' + fn + r'\s*\(', text)]
                    if len(cs) != 1 or not (ps.start() <= cs[0] < ps.end()):
                        res['errors'].append('mod.cpp: %s must be called exactly once, inside post08_step' % fn)
                if 't.state = post08::kDeflectState;' not in ps.group(0) or 'post05::LiveMode t = g.cur.pre_live;' not in ps.group(0):
                    res['errors'].append('mod.cpp: the deflection target must be the PRE live mode with state = post08::kDeflectState')
                if 'if (plan == post08::DeflectPlan::Refuse) stage = "";' not in ps.group(0) or 'a->health != g.cur.pre_hp' not in ps.group(0) \
                        or 'post08::deflect_plan(pre_s, now_s, post08::link_protected_now(in.now))' not in ps.group(0):
                    res['errors'].append('mod.cpp: post08_step must plan the deflection from the reaction shape and Link protection, and check HP before writing')
                # FIX02: el desvio solo escribe RestorePre (PRE exacto) o ForceRecoil (PRE con estado 3); NativeContinues no escribe nada
                if 'if (plan == post08::DeflectPlan::ForceRecoil) t.state = post08::kDeflectState;' not in ps.group(0) or len(re.findall(r't\.state\s*=', ps.group(0))) != 1:
                    res['errors'].append('mod.cpp: the only deflection state change is ForceRecoil -> kDeflectState')
                # FIX02: frescura del registro At: un solo note_post (en do_post, tras todas las escrituras) y el juicio exige g.arm.fresh
                dp_ = re.search(r'void do_post\(daE_OC_c\* a\) noexcept \{.*?\n\}\n', text, flags=re.S)
                np_ = [m.start() for m in re.finditer(r'arm\.note_post\s*\(', text)]
                if not dp_ or len(np_) != 1 or not (dp_.start() <= np_[0] < dp_.end()) or dp_.group(0).find('arm.note_post') < dp_.group(0).find('post06_step(a, r, has_event, rep, parry_crossed)'):
                    res['errors'].append('mod.cpp: g.arm.note_post exactly once, in do_post, after post06_step')
                if 'in.fresh = in.tracked && g.arm.fresh(g.cur.slot, g.cur.id, g.cur.generation, g.p8.tick);' not in pj.group(0):
                    res['errors'].append('mod.cpp: the verdict must require a fresh (armed previous pass) At record')
                # FIX02: el observador de IA es de solo lectura
                for fn in ('post08_watch_out', 'post08_watch_feed'):
                    fb = re.search(r'void ' + fn + r'\([^;{]*\) noexcept \{.*?\n\}\n', text, flags=re.S)
                    if not fb or any(t in fb.group(0) for t in ('restore_reaction', 'write_vitality', 'post08_deflect', 'apply_parry', '.rearm(', 'helm_set_cut')):
                        res['errors'].append('mod.cpp: %s must exist and be read-only' % fn)
                # FIX01: el estado de Link se captura UNA vez por tick, en post08_observe_link (solo lectura), llamado una vez desde do_update; el juicio
                # usa esa instantanea (pass_for) y nunca decide con el estado de Link de la propia llamada.
                ol = re.search(r'void post08_observe_link\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
                if not ol or len(re.findall(r'\bpost08_observe_link\s*\(', text)) != 2 or not re.search(r'void do_update\(\) noexcept \{\n  probe_variant_ai_env\(\);\n  post08_update\(\);\n  post08_observe_link\(\);', text):
                    res['errors'].append('mod.cpp: post08_observe_link must be defined once and called once, from do_update (mod_update)')
                else:
                    if len(re.findall(r'link_track\.observe\s*\(', text)) != 1 or 'link_track.observe(' not in ol.group(0):
                        res['errors'].append('mod.cpp: link_track.observe only inside post08_observe_link')
                    for tok in ('restore_reaction', 'write_vitality', 'helm_set_cut', 'post08_deflect', 'apply_parry', '.rearm('):
                        if tok in ol.group(0):
                            res['errors'].append('mod.cpp: post08_observe_link is read-only (found %s)' % tok)
                if 'in.pass = g.link_track.pass_for(g.p8.tick, in.link_ok, link_id);' not in pj.group(0) or re.search(r'in\.pass\s*=\s*in\.now', text):
                    res['errors'].append('mod.cpp: the verdict must use the per-tick pass snapshot (pass_for), never the current Link state')
                if len(re.findall(r'\.parry\.enabled\s*=(?!=)', text)) != 1 or 'g.parry.enabled = true;' not in text:
                    res['errors'].append('mod.cpp: g.parry.enabled must be set exactly once, to true')
            # POST07 HUD: DISPLAY ONLY. The HUD block (struct HudGlue .. emit_hud_summary) never writes game/posture/POST05/POST06 state.
            hb = re.search(r'struct HudGlue \{.*?void emit_hud_summary\(', text, flags=re.S)
            if not hb:
                res['errors'].append('mod.cpp: POST07 HUD block not found')
            else:
                h = hb.group(0)
                for tok in ('restore_reaction', 'write_vitality', 'helm_set_cut', 'helm_guard_duplicate', '.rearm(', 'set_regen_rate', '.consume(', '.step(', '.commit(',
                            'g.rx', 'on_pre(', 'on_post(', 'on_update(', 'setActionMode', 'fopAcM_SearchByID', 'get_service', 'mAtInfo', 'mCutType', 'field_0x6cc'):
                    if tok in h:
                        res['errors'].append('mod.cpp: the HUD block may not use %s (display only)' % tok)
                for m in re.finditer(r'\bg\.(pe|br|eng)\b[\w\.]*', h):
                    if m.group(0) not in ('g.pe.px.entry', 'g.br.slot', 'g.eng.obs.ledger.slot', 'g.eng.obs.disabled'):
                        res['errors'].append('mod.cpp: the HUD block may only read g.pe.px.entry/g.br.slot/g.eng.obs.ledger.slot/g.eng.obs.disabled, not %s' % m.group(0))
                if re.search(r'->\s*[\w\.\[\]]+\s*(=(?!=)|\+=|-=|\|=|&=|\^=|<<=|>>=|\+\+|--)', h) or re.search(r'(\+\+|--)\s*\w+\s*->', h):
                    res['errors'].append('mod.cpp: the HUD block writes through a pointer')
                for m in re.finditer(r'(?<![\w.])(?:const\s+)?(?:fopAc_ac_c|daE_OC_c)\s*\*\s*(\w+)', h):
                    if not m.group(0).startswith('const') and m.group(1) not in ('act', 'player'):
                        res['errors'].append('mod.cpp: unexpected actor pointer %s in the HUD block' % m.group(1))
            # one read-only walk of the live actor queue, inside hud_sim_pass; one queue of OUR packet; scrub only in hud_drop_all
            sp = re.search(r'void hud_sim_pass\(dMeter2_c\* meter\) noexcept \{.*?\n\}\n', text, flags=re.S)
            dp = re.search(r'void hud_drop_all\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            pp = re.search(r'void hud_packet_paint\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            qs = [m.start() for m in re.finditer(r'g_fopAcTg_Queue', text)]
            if not sp or len(qs) != 1 or not (sp.start() <= qs[0] < sp.end()):
                res['errors'].append('mod.cpp: g_fopAcTg_Queue may be walked exactly once, inside hud_sim_pass')
            qa = [m.start() for m in re.finditer(r'queue_after_hud\s*\(', text)]
            if not sp or len(qa) != 1 or not (sp.start() <= qa[0] < sp.end()) or 'set2DXlu' in text or re.search(r'\.set\s*\(', text):
                res['errors'].append('mod.cpp: the packet is queued exactly once per pass (queue_after_hud in hud_sim_pass); no direct set()/set2DXlu')
            sc = [m.start() for m in re.finditer(r'scrub_all\s*\(', text)]
            if not dp or len(sc) != 1 or not (dp.start() <= sc[0] < dp.end()):
                res['errors'].append('mod.cpp: scrub_all only inside hud_drop_all')
            if len(re.findall(r'\bhud_drop_all\s*\(', text)) != 4:
                res['errors'].append('mod.cpp: hud_drop_all must be defined once and called from the draw-pass failure path, the meter delete pre-hook and mod_shutdown')
            if not pp:
                res['errors'].append('mod.cpp: hud_packet_paint not found')
            else:
                for tok in ('fopAc_ac_c', 'daE_OC_c', 'g_fopAcTg_Queue', 'g.eng', 'g.pe', 'g.br', 'samples', 'act', 'anchors.update', 'publish('):
                    if re.search(r'(?<![\w.])' + re.escape(tok) + r'(?![\w])', pp.group(0)):
                        res['errors'].append('mod.cpp: the painter callback may only read hg.frame value copies (found %s)' % tok)
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
            # POST06: segundo escritor ACOTADO: write_vitality asigna SOLO r->health = hp y (rama letal) r->attention_info.flags = 0.
            wv = re.search(r'void write_vitality\(daE_OC_c\* r, int hp, bool lethal\) noexcept \{(.*?)\n\}\n', text, flags=re.S)
            inside_wv = [w for w in writes if wv and wv.start(1) <= w.start() < wv.end(1)]
            if not wv or sorted(w.group(2) for w in inside_wv) != ['attention_info.flags', 'health'] or any(w.group(1) != 'r' or w.group(3) != '=' for w in inside_wv) \
                    or 'r->attention_info.flags = 0;' not in wv.group(1) or 'r->health = static_cast<s16>(hp);' not in wv.group(1):
                res['errors'].append('mod.cpp: write_vitality must assign exactly r->health = hp and r->attention_info.flags = 0')
            extra=[]
            # LNK01: escritores VISUALES (HUD_VISUAL_ONLY) exactos: hide() de la raiz (una llamada) y los dos slots de brillo del prompt (dos asignaciones).
            ah = re.search(r'void action_ui_hide\(J2DScreen\* r\) noexcept \{ r->hide\(\); \}', text)
            asl = re.search(r'void action_ui_slots\(dMeterButton_c\* r, std::uint8_t s0, std::uint8_t s1\) noexcept \{\n  r->field_0x4be\[0\] = s0;\n  r->field_0x4be\[1\] = s1;\n\}\n', text)
            if not ah or not asl:
                res['errors'].append('mod.cpp: LNK01 visual writers must be exactly action_ui_hide (r->hide()) and action_ui_slots (field_0x4be[0..1])')
            else:
                extra += [w for w in writes if asl.start() <= w.start() < asl.end()]
            if len(re.findall(r'hide\s*\(\s*\)', text)) != 1 or len(re.findall(r'\baction_ui_hide\s*\(', text)) != 2 or len(re.findall(r'\baction_ui_slots\s*\(', text)) != 3:
                res['errors'].append('mod.cpp: hide() exactly once (action_ui_hide, called once from action_ui_step); action_ui_slots defined once and called from the prompt draw PRE and POST')
            for t_ in LNK01_FORBIDDEN:
                if t_ in text:
                    res['errors'].append('mod.cpp: LNK01 forbidden token %s (focal visual suppression only; native guard counter untouched)' % t_)
            stp = re.search(r'void action_ui_step\(dMeter2_c\* meter, unsigned site\) noexcept \{.*?\n\}\n', text, flags=re.S)
            if not stp or 'action_ui_hide(s)' not in stp.group(0) or 'if (d.hide && s)' not in stp.group(0):
                res['errors'].append('mod.cpp: the root is hidden only from action_ui_step, only on a validated decision')
            rd = re.search(r'lnk01::UiInput action_ui_read\(.*?\n\}\n', text, flags=re.S)
            if not rd or 'in.vptr_ok = vs != nullptr && vs == *reinterpret_cast<void* const*>(ref) && vs == *reinterpret_cast<void* const*>(waku);' not in rd.group(0) \
                    or 'if (!in.vptr_ok) return in;' not in rd.group(0) or re.search(r'->\s*[\w\.\[\]]+\s*(=(?!=))', rd.group(0)):
                res['errors'].append('mod.cpp: action_ui_read must fail closed on the J2DScreen vptr check and be read-only')
            pre_ = re.search(r'HookAction on_prompt_draw_pre\(.*?\n\}\n', text, flags=re.S)
            if not pre_ or 'meter->mpEmpButton == self' not in pre_.group(0) or 'lg.ui.layout_ok_for(key)' not in pre_.group(0):
                res['errors'].append('mod.cpp: the sparkle mask applies only to the LIVE validated prompt (mpEmpButton == self)')
            ins = re.search(r'hq = mods::hook::add_post<PromptDrawLnk01>\(on_prompt_draw_post\);\n    hp = hq == MOD_OK \? mods::hook::add_pre<PromptDrawLnk01>\(on_prompt_draw_pre\) : MOD_ERROR;', text)
            if not ins:
                res['errors'].append('mod.cpp: the prompt draw POST must be installed before (and as a precondition of) its PRE')
            # LNK01: postura de Link = contador PRIVADO alimentado solo por el veredicto POST08 (una vez, en do_post tras post08_step) y por un tick en mod_update
            lc = re.search(r'void lnk01_contact\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            lu = re.search(r'void lnk01_update\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            dp2 = re.search(r'void do_post\(daE_OC_c\* a\) noexcept \{.*?\n\}\n', text, flags=re.S)
            du2 = re.search(r'void do_update\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            if not lc or not lu or not dp2 or not du2:
                res['errors'].append('mod.cpp: lnk01_contact/lnk01_update/do_post/do_update not found')
            else:
                for fb_, nm in ((lc, 'lnk01_contact'), (lu, 'lnk01_update')):
                    if any(t_ in fb_.group(0) for t_ in ('restore_reaction', 'write_vitality', 'helm_set_cut', 'helm_guard_duplicate', 'post08_deflect', 'apply_parry',
                                                         '.rearm(', '.commit(', 'action_ui_', 'judge(')) or re.search(r'->\s*[\w\.\[\]]+\s*(=(?!=)|\+=|-=|\+\+|--)', fb_.group(0)):
                        res['errors'].append('mod.cpp: %s may only update the private Link posture counter' % nm)
                if len(re.findall(r'\.on_contact\s*\(', text)) != 1 or '.on_contact(' not in lc.group(0) or len(re.findall(r'\blnk01_contact\s*\(', text)) != 2 \
                        or 'lnk01_contact();' not in dp2.group(0) or dp2.group(0).find('lnk01_contact();') < dp2.group(0).find('post08_step(a, r)'):
                    res['errors'].append('mod.cpp: Link posture contacts only from the POST08 verdict, once, in do_post after post08_step')
                if 'k.result = v.result;' not in lc.group(0) or 'k.blocked = v.blocked;' not in lc.group(0) or 'k.token = v.token;' not in lc.group(0) or 'e.parried' not in lc.group(0):
                    res['errors'].append('mod.cpp: the Link contact must carry the POST08 verdict, native block flag, attack token and the parry episode')
                if len(re.findall(r'\.posture\.tick\s*\(', text)) != 1 or 'lg.posture.tick(lg.sim_active, pm, ob.guard_safe)' not in lu.group(0) or len(re.findall(r'\blnk01_update\s*\(', text)) != 2 \
                        or 'lnk01_update();' not in du2.group(0):
                    res['errors'].append('mod.cpp: Link posture regeneration ticks exactly once per mod_update (lnk01_update from do_update)')
                if not re.search(r'lg\.sim_active = l\.valid && life > 0 && !g_dComIfG_gameInfo\.play\.isPauseFlag\(\) && !g_dComIfG_gameInfo\.play\.getEvent\(\)->runCheck\(\) &&\s*dMeter2Info_getWindowStatus\(\) == 0;', lu.group(0)):
                    res['errors'].append('mod.cpp: regeneration must freeze without Link, at 0 life, in pause, in events and with a menu window open')
            # LNK SESSION01: confirmacion de golpes directos UNA vez por mod_update, despues de observe_link; lectura PRE de atribucion de solo lectura
            if lc and lu:
                if len(re.findall(r'\.confirm_direct\s*\(', text)) != 1 or 'lg.posture.confirm_direct(ob, g.p8.tick, de);' not in lu.group(0) \
                        or lu.group(0).find('lg.posture.confirm_direct(') < lu.group(0).find('lg.posture.observe_link(') \
                        or lu.group(0).find('lg.posture.confirm_direct(') > lu.group(0).find('lg.posture.tick('):
                    res['errors'].append('mod.cpp: confirm_direct exactly once per mod_update, after observe_link and before the regeneration tick')
                if 'k.tg_match = g.cur.lnk_tg_match;' not in lc.group(0):
                    res['errors'].append('mod.cpp: a direct hit must carry the Link Tg attribution read in the PRE')
                ob_ = re.search(r'ob\.guard_safe = lg\.sim_active && ob\.human && l\.upper_guard && l\.damage_timer == 0 && l\.proc != daAlink_c::PROC_GUARD_SLIP &&\s*'
                                r'l\.proc != daAlink_c::PROC_GUARD_ATTACK && l\.proc != daAlink_c::PROC_GUARD_BREAK && l\.proc != daAlink_c::PROC_CUT_REVERSE;', lu.group(0))
                if not ob_:
                    res['errors'].append('mod.cpp: safe guard = active human upper-guard without GUARD_SLIP/ATTACK/BREAK/CUT_REVERSE and without invulnerability')
            lp_ = re.search(r'void lnk_pre\(daE_OC_c\* a, const PreReads& r, daPy_py_c\* lp\) noexcept \{.*?\n\}\n', text, flags=re.S)
            if not lp_ or len(re.findall(r'\blnk_pre\s*\(', text)) != 2 or 'lnk_pre(a,r,lp);' not in text \
                    or re.search(r'->\s*[\w\.\[\]]+\s*(=(?!=)|\+=|-=|\+\+|--)', lp_.group(0)) \
                    or any(t_ in lp_.group(0) for t_ in ('restore_reaction', 'write_vitality', 'helm_set_cut', 'post08_deflect', 'apply_parry', '.on_contact(', '.rearm(')):
                res['errors'].append('mod.cpp: lnk_pre must exist, be read-only and be called once (from do_pre)')
            elif 'break;' not in lp_.group(0) or 'cCcD_ObjHitInf::ChkTgHit()' not in lp_.group(0) or 'cCcD_ObjHitInf::GetTgHitObj()' not in lp_.group(0):
                res['errors'].append('mod.cpp: the attribution must use the FIRST Link Tg cylinder with a raw hit (checkDamageAction order)')
            pp2 = re.search(r'void hud_packet_paint\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            if pp2 and any(t_ in pp2.group(0) for t_ in ('lg.posture', 'lg.hud.', 'lg.ui', 'action_ui_', 'lnk01_link_now')):
                res['errors'].append('mod.cpp: the painter draws the Link bar only from the value copy hg.player')
            if len(re.findall(r'\blg\.hud\.tick\s*\(', text)) != 1:
                res['errors'].append('mod.cpp: the Link bar animates exactly once per simulation pass (hud_sim_pass)')
            for fn,field in [('helm_set_cut','mCutType'),('helm_guard_duplicate','field_0x6cc')]:
                fbody=re.search(r'void '+fn+r'\(.*?\n\}\n',text,flags=re.S)
                found=[w for w in writes if fbody and fbody.start()<=w.start()<fbody.end()]
                if len(found)!=1 or found[0].group(1)!='r' or found[0].group(2)!=field or found[0].group(3)!='=':
                    res['errors'].append('invalid audited Helm writer '+fn)
                extra+=found
            if 'if(cut==5||cut==8) r->mCutType=cut;' not in text or 'r->field_0x6cc=1;' not in text:
                res['errors'].append('Helm writers must keep cut5/8 and cooldown1 bounds')
            for w in writes:
                if w not in inside and w not in inside_wv and w not in extra:
                    res['errors'].append('mod.cpp: write through a pointer outside restore_reaction/write_vitality: %s' % w.group(0)[:60])
            calls_r = [m.start() for m in re.finditer(r'\brestore_reaction\s*\(', text)]
            hosts = [re.search(r'void restore_reaction\(daE_OC_c\* r, const post05::LiveMode& t\) noexcept \{', text)] + \
                    [re.search(p_, text, flags=re.S) for p_ in (r'void do_post\(daE_OC_c\* a\) noexcept \{.*?\n\}\n', r'bool post06_bonus\(.*?\n\}\n', r'bool post06_stun\(.*?\n\}\n',r'bool post06_recover\(.*?\n\}\n',r'void on_stun_post\(.*?\n\}\n',r'void post08_deflect\(daE_OC_c\* a, const post05::LiveMode& t\) noexcept \{ restore_reaction\(a, t\); \}')]
            if len(calls_r) != 8 or any(h is None for h in hosts) or not all(any(h.start() <= c < h.end() for h in hosts) for c in calls_r):
                res['errors'].append('mod.cpp: restore_reaction must be defined once and called only from do_post (POST05), post06_bonus and post06_drop')
            if len(re.findall(r'\bwrite_vitality\s*\(', text)) != 3:
                res['errors'].append('mod.cpp: write_vitality must be defined once and called once (post06_bonus)')
            if len(re.findall(r'\.br\.enabled\s*=(?!=)', text)) != 1 or len(re.findall(r'\bg\.br\.enabled\s*=\s*true\s*;', text)) != 1:
                res['errors'].append('mod.cpp: g.br.enabled must be set exactly once, to true')
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
    if hook_defs != 6:
        res['errors'].append('DEFINE_HOOK count %d != 6 (damage_check, executeDamage, dMeter2_c::_execute/_draw/_delete, dMeterButton_c::draw)' % hook_defs)
    # POST07: the HUD core is pure display logic: it never names a posture/POST05/POST06 type and owns no address
    hc = [t for f_, t in zip(res['files'], all_text) if f_ == 'hud_core.hpp']
    if len(hc) != 1:
        res['errors'].append('hud_core.hpp missing')
    else:
        for tok in ('post04', 'post05', 'post06', 'post03', 'uintptr_t', 'reinterpret_cast', 'static_cast<fopAc', '->'):
            if tok in hc[0]:
                res['errors'].append('hud_core.hpp: display core may not use %s' % tok)
    # LNK01: el nucleo de Link es logica pura: ni tipos de POST03-06 ni direcciones
    lk = [t for f_, t in zip(res['files'], all_text) if f_ == 'lnk_core.hpp']
    if len(lk) != 1:
        res['errors'].append('lnk_core.hpp missing')
    else:
        for tok in ('post04', 'post05', 'post06', 'post03', 'uintptr_t', 'reinterpret_cast', 'static_cast<fopAc', 'daAlink', 'dMeter'):
            if tok in lk[0]:
                res['errors'].append('lnk_core.hpp: pure core may not use %s' % tok)
    res['define_hook_count'] = hook_defs
    res['status'] = 'SOURCE_AUDIT_PASS' if not res['errors'] else 'SOURCE_AUDIT_FAIL'
    print(json.dumps(res, indent=2))
    return 0 if not res['errors'] else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1]))
