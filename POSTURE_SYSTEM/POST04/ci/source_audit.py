"""POST04: auditoria estatica de PASIVIDAD sobre src/*.cpp|hpp (texto). No es una prueba formal: es una red contra regresiones.
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
                   'dComIfGp_setHitMark', 'mDoAud', 'startCollisionSE', 'startCreatureVoice']
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
STD_INCLUDES = {'cstddef', 'cstdint', 'cstdio', 'cstring'}
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
        for t in FORBIDDEN_CALLS + FORBIDDEN_TOKENS:
            if t in text:
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
    # el reloj de postura: UN avance por mod_update y UN consumo por evento, ambos solo desde la envoltura
    if len(re.findall(r'\.advance\s*\(\s*\)', blob)) != 1 or len(re.findall(r'\bvoid\s+advance\s*\(', blob)) != 1:
        res['errors'].append('px.advance() must be defined once and called once (PostureEngine::on_update)')
    if len(re.findall(r'\.consume\s*\(', blob)) != 1 or len(re.findall(r'\bvoid\s+consume\s*\(', blob)) != 1:
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
