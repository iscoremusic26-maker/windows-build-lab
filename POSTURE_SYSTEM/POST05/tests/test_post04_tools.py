"""POST04: pruebas de las herramientas (generador de la traza real, extractor de logs de POST04). Logs sinteticos / salida del formateador real; ningun log crudo.
Variables opcionales: POST04_FRESH_SAMPLE=<salida nueva de `posture_lab --emit-sample>` (debe ser identica a la muestra versionada: detecta deriva del formateador)."""
import hashlib
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import extract_post04_evidence as X  # noqa: E402

SAMPLE = HERE / 'data' / 'sample_post04_formatter_output.log'


class T(unittest.TestCase):
    def setUp(self):
        self.d = Path(tempfile.mkdtemp())

    def write(self, name, text):
        p = self.d / name
        p.write_bytes(text.encode('utf-8'))
        return p

    def lines(self):
        return SAMPLE.read_text().splitlines()

    def test_generator_is_deterministic_and_current(self):
        out1, out2 = self.d / 'a.inc', self.d / 'b.inc'
        for o in (out1, out2):
            subprocess.run([sys.executable, '-I', str(HERE / 'gen_real_trace.py'), str(HERE / 'data' / 'real_trace_a1_b1_events.tsv'), str(o)], check=True, capture_output=True)
        self.assertEqual(out1.read_bytes(), out2.read_bytes())
        self.assertEqual(out1.read_bytes(), (HERE / 'real_trace.inc').read_bytes(), 'real_trace.inc is stale: rerun gen_real_trace.py')

    def test_reaction_trace_is_current(self):  # POST05: la traza de los 27 informes reales y su decision esperada
        out = self.d / 'r.inc'
        subprocess.run([sys.executable, '-I', str(HERE / 'gen_reaction_trace.py'), str(HERE / 'data' / 'post04_a1_b1_events.tsv'), str(out)], check=True, capture_output=True)
        self.assertEqual(out.read_bytes(), (HERE / 'reaction_trace.inc').read_bytes(), 'reaction_trace.inc is stale: rerun gen_reaction_trace.py')

    def test_sample_matches_fresh_formatter_output(self):
        fresh = os.environ.get('POST04_FRESH_SAMPLE')
        if not fresh:
            self.skipTest('no fresh sample provided')
        self.assertEqual(Path(fresh).read_bytes(), SAMPLE.read_bytes(), 'formatter output drifted from the committed sample')

    def test_extractor_accepts_real_formatter_output(self):
        s = X.run(SAMPLE, self.d / 'o')
        self.assertEqual(s['violations'], [])
        c = s['item_counts']
        self.assertGreaterEqual(c['POSTURE_HIT'], 5)
        self.assertEqual(c['BREAK_READY'], 1)
        self.assertEqual(s['session']['init']['writes_to_game'], '0')
        self.assertTrue(s['session']['init']['pause_unverified'])

    def test_determinism_byte_identical(self):
        X.run(SAMPLE, self.d / 'o1')
        X.run(SAMPLE, self.d / 'o2')
        for f in (self.d / 'o1').iterdir():
            self.assertEqual(f.read_bytes(), (self.d / 'o2' / f.name).read_bytes())

    def test_wrong_hash_bytes_lines(self):
        good = hashlib.sha256(SAMPLE.read_bytes()).hexdigest()
        for exp in ({'sha256': '0' * 64}, {'bytes': 1}, {'splitlines': 1}):
            with self.assertRaises(SystemExit):
                X.run(SAMPLE, self.d / 'x', exp)
        X.run(SAMPLE, self.d / 'ok', {'sha256': good, 'bytes': SAMPLE.stat().st_size, 'splitlines': len(self.lines())})

    def mutate(self, fn):
        return self.write('m.log', '\n'.join(fn(self.lines())) + '\n')

    def violations(self, fn):
        return X.run(self.mutate(fn), self.d / 'mo')['violations']

    def test_detects_arithmetic_and_policy_violations(self):
        self.assertTrue(self.violations(lambda L: [l.replace('before=0 delta=25 after=25', 'before=0 delta=25 after=26') for l in L]))
        self.assertTrue(self.violations(lambda L: [l.replace('family=SWORD at=0x04000000 eligible=CONFIRMED lethal=0 before=0 delta=25', 'family=SWORD at=0x04000000 eligible=CONFIRMED lethal=0 before=0 delta=30') for l in L]))
        self.assertTrue(self.violations(lambda L: [l.replace('family=BOOMERANG at=0x00010000 eligible=CONFIRMED lethal=0 before=0 delta=0', 'family=BOOMERANG at=0x00010000 eligible=CONFIRMED lethal=0 before=0 delta=5') for l in L]))

    def test_detects_second_break_ready_and_delete_claim(self):
        def dup_break(L):
            out = []
            for l in L:
                out.append(l)
                if 'POST04_BREAK_READY' in l:
                    out.append(l)
            return out
        self.assertTrue(any('second BREAK_READY' in v for v in self.violations(dup_break)))
        # un retiro que NO es ausencia verificada pero afirma delete_claim=1 debe detectarse
        def bad_claim(L):
            return [l.replace('reason=LOOKUP_ABSENT_VERIFIED', 'reason=CONTEXT_RETIRED') if 'POST04_POSTURE_RETIRE' in l else l for l in L]
        self.assertTrue(any('delete_claim' in v for v in self.violations(bad_claim)))
        # y el retiro con ausencia verificada y delete_claim=1 de la muestra es valido
        self.assertTrue(any('delete_claim=1' in l and 'LOOKUP_ABSENT_VERIFIED' in l for l in self.lines() if 'POST04_POSTURE_RETIRE' in l))

    def test_detects_conservation_and_counter_violations(self):
        self.assertTrue(self.violations(lambda L: [l.replace('e5_units=1', 'e5_units=0').replace('ok=1', 'ok=0') if 'POST04_CONSERVATION' in l else l for l in L]))
        self.assertTrue(self.violations(lambda L: [l.replace('overflow=0', 'overflow=1') if 'POST04_SUMMARY' in l else l for l in L]))
        self.assertTrue(self.violations(lambda L: [l.replace('confirmed=', 'confirmed=9') if 'POST03_HIT_SUMMARY' in l else l for l in L]))
        self.assertTrue(self.violations(lambda L: [l for l in L if 'POST04_SUMMARY' not in l]))                      # entrada incompleta
        self.assertTrue(self.violations(lambda L: [l.replace('passive=1', 'passive=0') if 'POST04_INIT' in l else l for l in L]))

    def test_rejects_out_of_allowlist(self):
        for fn in (lambda L: L + ['[INFO | dev.posture.post04_numeric_observer] POST04_NEW_THING secret=1'],
                   lambda L: L + ['[INFO | dev.posture.post04_numeric_observer] POST04_POSTURE_BIND id=1 generation=1 initial=0 tick=0 reason=X extra=1'],
                   lambda L: [l.replace('family=SWORD', 'family=SECRET') for l in L],
                   lambda L: [l.replace('reason=WEIGHTED_HIT', 'reason=C:\\Users\\x') for l in L],
                   lambda L: [l.replace('reason=WEIGHTED_HIT', 'reason=/mnt/c/x') for l in L]):
            with self.assertRaises((ValueError, SystemExit)):
                X.run(self.mutate(fn), self.d / 'rej')

    def test_privacy_scan_catches_leaks_in_allowed_shape(self):
        for leak in ('10.0.0.1', 'deadbeefdeadbeef', 'SomeAccount'):
            p = self.mutate(lambda L: [l.replace('reason=WEIGHTED_HIT', 'reason=' + leak) for l in L])
            with self.assertRaises(SystemExit):
                X.run(p, self.d / 'pv', None, ('SomeAccount',))

    def test_output_has_no_raw_lines_or_paths(self):
        X.run(SAMPLE, self.d / 'o')
        blob = ''.join(f.read_text() for f in (self.d / 'o').iterdir())
        for bad in ('dev.posture', 'INFO |', '\\', '/mnt', 'OneDrive'):
            self.assertNotIn(bad, blob)


if __name__ == '__main__':
    unittest.main()
