"""POST01: auditoria estatica de PASIVIDAD sobre src/*.cpp|hpp (texto). No es una prueba formal: es una red contra regresiones.
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
                    'memcpy', 'memset', 'const_cast', 'reinterpret_cast<std::uintptr_t*>', 'fopAcM_SearchByID',
                    'fpcM_Search', 'fopAcIt_', 'g_fopAcTg_Queue', 'dComIfG_Ccsp']
# Asignacion / incremento sobre un miembro alcanzado desde un puntero (a->x = ..., a->x[i].y += ..., ++a->x)
ASSIGN = re.compile(r'(?<![\w.])a\s*->\s*[\w\[\]\.>\-\(\)]*?\s*(=(?!=)|\+=|-=|\|=|&=|\^=|<<=|>>=|\+\+|--)')
PREINC = re.compile(r'(\+\+|--)\s*a\s*->')


def main(project):
    project = Path(project)
    res = {'files': [], 'errors': []}
    hook_defs = 0
    for f in sorted((project / 'src').glob('*')):
        if f.suffix not in ('.cpp', '.hpp'):
            continue
        text = strip_comments(f.read_text())
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
    # DEFINE_HOOK aparece en mod.cpp una sola vez (el nombre de objetivo se comprueba en el PE)
    if hook_defs != 1:
        res['errors'].append('DEFINE_HOOK count %d != 1' % hook_defs)
    res['define_hook_count'] = hook_defs
    res['status'] = 'SOURCE_AUDIT_PASS' if not res['errors'] else 'SOURCE_AUDIT_FAIL'
    print(json.dumps(res, indent=2))
    return 0 if not res['errors'] else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1]))
