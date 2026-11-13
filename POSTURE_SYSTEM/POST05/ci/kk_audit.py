"""LIGHT FADES TWICE 0.13.0 CHILFOS01 additive barrier: the OUTERMOST layer of the inverse-patch chain. Every shared production file touched by
CHILFOS01 (mod.cpp, parry_fx_core.hpp, parry_fx_adapter.inc) must be reproduced byte for byte as FIX05 0.12.5 (compiled eade22d6) by the exact,
counted inverse patch in kk_policy.json; the older chain (liz_audit and below) then applies to that reconstruction exactly as it did for FIX05.
The four Chilfos modules are the only new production files."""
from pathlib import Path
import hashlib, json, sys
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import dkr_audit  # noqa: E402  LFT 0.13.8: the DKR01 layer (its three modules, its exact mod.cpp hunks) sits outside this one
NEW = {'kk_core.hpp', 'kk_adapter.inc', 'kk_bridge.inc', 'kk_pfx.inc'}


def policy():
    return json.loads((HERE / 'kk_policy.json').read_text(encoding='utf-8'))


def legacy_source(name, text):
    for e in reversed(policy()['inverse_patch'].get(name, [])):
        if text.count(e['after']) == e.get('count', 1):
            text = text.replace(e['after'], e['before'], e.get('count', 1))
    return text


def audit(project):
    p = Path(project)
    pin = policy()
    errors = []
    if set(pin['new_files']) != NEW:
        errors.append('CHILFOS01 new-module inventory changed')
    for n in NEW:
        if not (p / 'src' / n).is_file():
            errors.append('CHILFOS01 module missing: ' + n)
    for name, edits in pin['inverse_patch'].items():
        t = (p / 'src' / name).read_text(encoding='utf-8')
        for e in reversed(edits):
            if t.count(e['after']) != e.get('count', 1):
                errors.append('CHILFOS01 exact integration changed: ' + name)
            else:
                t = t.replace(e['after'], e['before'], e.get('count', 1))
        if hashlib.sha256(t.encode('utf-8')).hexdigest() != pin['fix05_sha256'][name]:
            errors.append('CHILFOS01 shared file changed outside the exact counted inverse patch (not FIX05 after removal): ' + name)
    return errors


if __name__ == '__main__':
    e = audit(Path(sys.argv[1]) if len(sys.argv) > 1 else HERE.parent)
    # negative: one byte outside the authorized hunks must be rejected
    import tempfile, shutil
    with tempfile.TemporaryDirectory() as td:
        q = Path(td)
        shutil.copytree(HERE.parent / 'src', q / 'src')
        f = q / 'src' / 'mod.cpp'
        f.write_text(f.read_text(encoding='utf-8').replace('DEFINE_MOD();', 'DEFINE_MOD(); ', 1), encoding='utf-8')
        neg = bool(audit(q))
    out = {'status': 'KK_AUDIT_PASS' if not e and neg else 'KK_AUDIT_FAIL', 'errors': e, 'negative_rejected': neg}
    print(json.dumps(out, indent=1))
    sys.exit(0 if out['status'] == 'KK_AUDIT_PASS' else 1)
