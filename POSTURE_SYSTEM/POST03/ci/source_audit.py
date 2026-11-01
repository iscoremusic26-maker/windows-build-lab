"""POST03: auditoria estatica de PASIVIDAD sobre src/*.cpp|hpp (texto). No es una prueba formal: es una red contra regresiones.
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
                    'GetTgHitAc', 'GetAc(', 'mHitType', 'mHitBit', 'mAttackPower', 'mpActor', 'mpSound', 'mHitDirection', 'mPowerType',
                    'mHitStatus', 'getCutType', 'daPy_getPlayerActorClass', 'mStts']
# Asignacion / incremento sobre un miembro alcanzado desde un puntero (a->x = ..., a->x[i].y += ..., ++a->x)
ASSIGN = re.compile(r'(?<![\w.])(?:a|out)\s*->\s*[\w\[\]\.>\-\(\)]*?\s*(=(?!=)|\+=|-=|\|=|&=|\^=|<<=|>>=|\+\+|--)')
PREINC = re.compile(r'(\+\+|--)\s*(?:a|out)\s*->')


def main(project):
    project = Path(project)
    res = {'files': [], 'errors': []}
    hook_defs = 0
    for f in sorted((project / 'src').glob('*')):
        if f.suffix not in ('.cpp', '.hpp'):
            continue
        text = strip_comments(f.read_text())
        text = re.sub(r'static_assert\((?:[^;])*?\);', ' ', text, flags=re.S)  # los static_assert son de compilacion, no de ejecucion
        res['files'].append(f.name)
        for t in FORBIDDEN_CALLS + FORBIDDEN_TOKENS:
            if t in text:
                res['errors'].append('%s: forbidden token %s' % (f.name, t))
        for m in ASSIGN.finditer(text):
            res['errors'].append('%s: member assignment through game pointer: %s' % (f.name, m.group(0)[:60]))
        for m in PREINC.finditer(text):
            res['errors'].append('%s: increment through game pointer: %s' % (f.name, m.group(0)[:60]))
        hook_defs += len(re.findall(r'\bDEFINE_HOOK\s*\(', text))
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
    # DEFINE_HOOK aparece en mod.cpp una sola vez (el nombre de objetivo se comprueba en el PE)
    if hook_defs != 1:
        res['errors'].append('DEFINE_HOOK count %d != 1' % hook_defs)
    res['define_hook_count'] = hook_defs
    res['status'] = 'SOURCE_AUDIT_PASS' if not res['errors'] else 'SOURCE_AUDIT_FAIL'
    print(json.dumps(res, indent=2))
    return 0 if not res['errors'] else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1]))
