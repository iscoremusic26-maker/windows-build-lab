"""Pruebas adversariales de extract_post05_runtime.py (logs SINTETICOS coherentes; ningun log real en el repo).
Reutiliza el log sintetico de POST04 (POST04/evidence_tools/test_extract_posture_runtime.py) cambiando el modulo a POST05 y anadiendo
las lineas POST05_* y de Bokoblin Expansion.
Opcional: POSTURE_REAL_A1 / POSTURE_REAL_B1 = rutas a los logs reales de POST05 0.5.0 (134809 / 134505) para revalidar las cifras."""
import hashlib
import os
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parents[2] / 'POST04' / 'evidence_tools'))
import extract_post05_runtime as X  # noqa: E402
import test_extract_posture_runtime as P4  # noqa: E402

OLD = 'dev.posture.post04_numeric_observer'
MOD = 'dev.posture.post05_selective_reactions'
M = '[INFO | %s] ' % MOD
E = '[INFO | dev.bokoblin.expansion] '


def dec(ev, aid, gen, fam, decision, reason, pre, post, posture, action, extra=''):
    return M + ('POST05_DECISION id=%d generation=%d event=%d family=%s decision=%s reason=%s pre_mode=%d pre_state=0 post_mode=%d post_state=0 cd=0>10 '
                'posture=%s action=%s%s') % (aid, gen, ev, fam, decision, reason, pre, post, posture, action, extra)


DECS = [dec(1, 10, 1, 'SWORD', 'SUPPRESS_ELIGIBLE', 'ELIGIBLE', 4, 5, '0>25', 'SUPPRESSION_APPLIED'),
        dec(2, 10, 1, 'SHIELD_ATTACK', 'ALLOW_VANILLA', 'FAMILY_NOT_SWORD', 4, 5, '25>25', 'NONE'),
        dec(3, 10, 1, 'SWORD', 'SUPPRESS_ELIGIBLE', 'ELIGIBLE', 4, 5, '20>45', 'SUPPRESSION_APPLIED'),
        dec(4, 10, 1, 'SWORD', 'ALLOW_VANILLA', 'LETHAL', 4, 6, '45>45', 'NONE')]
TAIL = [M + 'POST05_SUMMARY v=0.5.0 tag=SHUTDOWN decisions=5 duplicates=0 not_applicable=1 allow_vanilla=2 skip_unsafe=0 eligible=2 confirmed=4 attempted=2 '
            'applied=2 recheck_failed=0 verify_failed=0 not_attempted=0 session_disabled=0',
        M + 'POST05_LAYOUT v=0.5.0 tag=SHUTDOWN layout=MATCHES_EXE',
        M + 'POST05_CONSERVATION v=0.5.0 tag=SHUTDOWN ok=1 r1_categories=1 r2_reasons=1 r3_eligible=1 r4_attempted=1 r5_order=1']
EXP = [E + 'event=VARIANT_ADMITTED actor_id=10 source=NATIVE_E_OC classified_variant=BLUE atp_from=1 atp_to=1',
       E + 'event=BLUE_GROUP_ON actor_id=10 atp_from=1 atp_to=2', E + 'event=BLUE_GROUP_OFF actor_id=10 atp_from=2 atp_to=1',
       E + 'event=PERF_WINDOW ms=3 secret=ignored']


def build(mut=None):
    L = [l.replace(OLD, MOD) for l in P4.build().rstrip('\n').split('\n')]
    L = [M + 'POST05_INIT version=0.5.0 policy_id=P writes_to_game=1 hook_skip_original=0'] + L
    out = []
    for l in L:
        out.append(l)
        for d in DECS:  # cada decision inmediatamente despues de su POST04_POSTURE_HIT (como en mod.cpp)
            ev = d.split(' event=')[1].split(' ')[0]
            if 'POST04_POSTURE_HIT' in l and ' event=%s ' % ev in l:
                out.append(d)
    i = next(n for n, l in enumerate(out) if 'POST04_CONSERVATION' in l and 'SHUTDOWN' in l)
    out = out[:5] + EXP + out[5:i + 1] + TAIL + out[i + 1:]
    if mut:
        out = mut(out)
    return '\n'.join(out) + '\n'


class T(unittest.TestCase):
    def setUp(self):
        self.d = Path(tempfile.mkdtemp())

    def run_on(self, text, expects=None, words=()):
        p = self.d / 'a.log'
        p.write_bytes(text.encode())
        return X.run({'S': p}, self.d / ('o%d' % len(list(self.d.iterdir()))), expects, words=words)

    def viol(self, mut):
        return self.run_on(build(mut))['violations']

    def test_coherent_session_has_no_violations(self):
        s = self.run_on(build())
        self.assertEqual(s['violations'], [])
        p = s['sessions']['S']['post05']
        self.assertEqual(p['decisions_by_reason'], {'ELIGIBLE': 2, 'FAMILY_NOT_SWORD': 1, 'LETHAL': 1})
        self.assertEqual(p['expansion_variants'], {'10': {'variant': ['BLUE'], 'source': ['NATIVE_E_OC']}})
        self.assertEqual(p['blue_group_transitions'], {'10': ['ON:1>2', 'OFF:2>1']})
        self.assertEqual(p['layout'], 'MATCHES_EXE')

    def test_determinism(self):
        p = self.d / 'a.log'
        p.write_bytes(build().encode())
        X.run({'S': p}, self.d / 'k1')
        X.run({'S': p}, self.d / 'k2')
        names = sorted(f.name for f in (self.d / 'k1').iterdir())
        self.assertEqual(len(names), 3)
        for n in names:
            self.assertEqual((self.d / 'k1' / n).read_bytes(), (self.d / 'k2' / n).read_bytes())

    def test_hash_mismatch(self):
        t = build()
        good = hashlib.sha256(t.encode()).hexdigest()
        self.assertEqual(self.run_on(t, {'S': {'sha256': good}})['violations'], [])
        with self.assertRaises(SystemExit):
            self.run_on(t, {'S': {'sha256': '0' * 64}})

    def test_decision_faults(self):
        self.assertTrue(any('without POST05_DECISION' in v for v in self.viol(lambda L: [l for l in L if 'POST05_DECISION' not in l or 'event=3 ' not in l])))
        self.assertTrue(any('two POST05_DECISION' in v for v in self.viol(lambda L: [x for l in L for x in ([l, l] if 'POST05_DECISION' in l and 'event=2 ' in l else [l])])))
        self.assertTrue(any('non-confirmed' in v for v in self.viol(lambda L: L + [dec(5, 11, 2, 'UNAVAILABLE', 'ALLOW_VANILLA', 'LETHAL', 6, 9, '0>0', 'NONE')])))
        self.assertTrue(any('unknown event' in v for v in self.viol(lambda L: L + [dec(9, 11, 2, 'SWORD', 'ALLOW_VANILLA', 'LETHAL', 6, 9, '0>0', 'NONE')])))
        self.assertTrue(any('identity/family' in v for v in self.viol(lambda L: [l.replace('id=10 generation=1 event=1 family=SWORD decision', 'id=10 generation=2 event=1 family=SWORD decision') for l in L])))
        self.assertTrue(any('identity/family' in v for v in self.viol(lambda L: [l.replace('event=2 family=SHIELD_ATTACK decision', 'event=2 family=SWORD decision') for l in L])))
        self.assertTrue(any('modes differ' in v for v in self.viol(lambda L: [l.replace('event=4 family=SWORD decision=ALLOW_VANILLA reason=LETHAL pre_mode=4', 'event=4 family=SWORD decision=ALLOW_VANILLA reason=LETHAL pre_mode=3') for l in L])))
        self.assertTrue(any('posture' in v for v in self.viol(lambda L: [l.replace('posture=20>45', 'posture=20>50') for l in L])))
        self.assertTrue(any('without SUPPRESS_ELIGIBLE' in v for v in self.viol(lambda L: [l.replace('reason=LETHAL pre_mode=4 pre_state=0 post_mode=6 post_state=0 cd=0>10 posture=45>45 action=NONE', 'reason=LETHAL pre_mode=4 pre_state=0 post_mode=6 post_state=0 cd=0>10 posture=45>45 action=SUPPRESSION_APPLIED') for l in L])))

    def test_summary_and_conservation_faults(self):
        for a, b, key in (('applied=2 recheck', 'applied=1 recheck', 'applied'), ('eligible=2 confirmed', 'eligible=3 confirmed', 'eligible'),
                          ('allow_vanilla=2', 'allow_vanilla=3', 'allow_vanilla'), ('not_applicable=1', 'not_applicable=0', 'not_applicable'),
                          ('skip_unsafe=0', 'skip_unsafe=1', 'skip_unsafe'), ('decisions=5', 'decisions=4', 'decisions')):
            self.assertTrue(any('POST05_SUMMARY %s=' % key in v for v in self.viol(lambda L: [l.replace(a, b) for l in L])), key)
        self.assertTrue(any('recheck/verify' in v for v in self.viol(lambda L: [l.replace('verify_failed=0', 'verify_failed=1') for l in L])))
        self.assertTrue(any('recheck/verify' in v for v in self.viol(lambda L: [l.replace('recheck_failed=0', 'recheck_failed=1') for l in L])))
        self.assertTrue(any('CONSERVATION not ok' in v for v in self.viol(lambda L: [l.replace('POST05_CONSERVATION v=0.5.0 tag=SHUTDOWN ok=1', 'POST05_CONSERVATION v=0.5.0 tag=SHUTDOWN ok=0') for l in L])))
        self.assertTrue(any('missing POST05_SUMMARY' in v for v in self.viol(lambda L: [l for l in L if 'POST05_SUMMARY' not in l])))
        self.assertTrue(any('CONSERVATION not ok' in v for v in self.viol(lambda L: [l for l in L if 'POST05_CONSERVATION' not in l])))

    def test_inherited_post03_post04_checks_still_apply(self):
        self.assertTrue(self.viol(lambda L: [l.replace('before=20 delta=25 after=45', 'before=20 delta=25 after=46') for l in L]))
        self.assertTrue(self.viol(lambda L: [l.replace('inline_sites_flag=0 multi', 'inline_sites_flag=1 multi') for l in L]))
        self.assertTrue(self.viol(lambda L: [l for l in L if 'all mods unloaded' not in l]))

    def test_allowlist(self):
        for mut in (lambda L: [l.replace('action=NONE', 'action=NONE extra=1', 1) if 'event=2 ' in l else l for l in L],     # campo nuevo
                    lambda L: [l.replace('reason=FAMILY_NOT_SWORD', 'reason=C:\\Users\\x') for l in L],                       # ruta
                    lambda L: [l.replace('posture=25>25', 'posture=0x7ff6a0001000') for l in L],                              # puntero
                    lambda L: [l.replace('event=BLUE_GROUP_ON actor_id=10', 'event=BLUE_GROUP_ON actor_id=0x7ff6') for l in L],
                    lambda L: [l.replace('classified_variant=BLUE', 'classified_variant=/mnt/c/x') for l in L],
                    lambda L: L + [M + 'POST05_SECRET x=1']):
            with self.assertRaises((ValueError, SystemExit)):
                self.run_on(build(mut))

    def test_0_5_1_fields_accepted_and_validated(self):
        ok = lambda L: [l + ' resource=E_OC2 atp=2>2 admission=E_OC2_CANDIDATE' if 'POST05_DECISION' in l else l for l in L]
        self.assertEqual(self.viol(ok), [])
        s = self.run_on(build(ok))
        self.assertEqual(s['violations'], [])
        bad = lambda L: [l + ' resource=E_OC2 atp=two admission=E_OC2_CANDIDATE' if 'POST05_DECISION' in l else l for l in L]
        with self.assertRaises(ValueError):
            self.run_on(build(bad))
        half = lambda L: [l + ' resource=E_OC2' if 'POST05_DECISION' in l else l for l in L]
        with self.assertRaises(ValueError):
            self.run_on(build(half))

    def test_privacy(self):
        self.run_on(build())
        blob = ''.join(f.read_text() for d in self.d.iterdir() if d.is_dir() for f in d.iterdir())
        for bad in ('OneDrive', 'Someone', 'cdcdcdcd', 'host_build_id', '.dusk', 'secret', 'PERF_WINDOW ms'):
            self.assertNotIn(bad, blob)
        with self.assertRaises((ValueError, SystemExit)):
            self.run_on(build(lambda L: [l.replace('reason=LETHAL', 'reason=SomeAccount') for l in L]), words=('SomeAccount',))

    def test_each_privacy_layer_alone(self):
        # capa 1: patron por campo de Expansion (un valor en minusculas no lo detectaria el escaner final sin lista de palabras)
        with self.assertRaises(ValueError):
            self.run_on(build(lambda L: [l.replace('classified_variant=BLUE', 'classified_variant=someone') for l in L]))
        # capa 2: escaner final con palabras prohibidas, sobre un valor que SI cumple el patron ENUM
        with self.assertRaises(SystemExit):
            self.run_on(build(lambda L: [l.replace('classified_variant=BLUE', 'classified_variant=SOMEACCOUNT') for l in L]), words=('SOMEACCOUNT',))
        # capa 3: POST05_INIT solo conserva campos de la lista blanca
        s = self.run_on(build(lambda L: [l.replace('hook_skip_original=0', 'hook_skip_original=0 user=SOMEACCOUNT') for l in L]))
        self.assertEqual(sorted(s['sessions']['S']['post05']['init']), ['hook_skip_original', 'policy_id', 'version', 'writes_to_game'])

    @unittest.skipUnless(os.environ.get('POSTURE_REAL_A1') and os.environ.get('POSTURE_REAL_B1'), 'real logs not supplied')
    def test_real_post05_logs(self):
        s = X.run({'A1': os.environ['POSTURE_REAL_A1'], 'B1': os.environ['POSTURE_REAL_B1']}, self.d / 'real',
                  {'A1': {'sha256': 'e58221c8d0783cdf6898c6dd8315cec3172d173a31ae3a2d12d739370478d291', 'bytes': 243083, 'splitlines': 2878},
                   'B1': {'sha256': '7be6645afe66638f288db84f24c46db7d821192126c0560250897c246e8a595f', 'bytes': 525010, 'splitlines': 2979}})
        self.assertEqual(s['violations'], [])
        a, b = s['sessions']['A1']['post05'], s['sessions']['B1']['post05']
        self.assertEqual((a['summary']['decisions'], a['summary']['eligible'], a['summary']['applied']), (12, 4, 4))
        self.assertEqual((b['summary']['decisions'], b['summary']['eligible'], b['summary']['skip_unsafe']), (9, 0, 1))
        self.assertEqual(b['decisions_by_reason'].get('ADMISSION_EVER_UNSTABLE'), 1)


if __name__ == '__main__':
    unittest.main()
