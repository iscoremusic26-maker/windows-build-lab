"""POST03.1: pruebas del extractor de evidencia (logs SINTETICOS; opcionalmente los reales si POST031_REAL_LOG_A/B apuntan a ellos fuera del repo)."""
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import extract_runtime_evidence as X  # noqa: E402


def hit(event, call, id_, gen, kind='HIT_CONFIRMED', tg='cc1', dual=0, distinct=0, src='none', collider='SELECTED_COLLIDER_IMPLAUSIBLE', family='UNAVAILABLE', at='UNAVAILABLE', hp='40>20'):
    return ('[INFO | dev.posture.post03_hit_classification] POST03_HIT_EVENT event=%d call=%d id=%d generation=%d life=ACTIVE kind=%s branch=PROCESSED cc_at_check=UNDETERMINED '
            'origin=UNDETERMINED tg=%s dual=%d distinct=%d collider_src=%s collider=%s family=%s native_first=NA basis=NOT_EVALUATED at=%s hp=%s lethal=0 mode=3>5 '
            'old_mode=3 state=3>2 cd=0>10 cutdown=0 eligible=CONFIRMED reason=COOLDOWN_SET_AND_REACTION_COMMITTED' % (event, call, id_, gen, kind, tg, dual, distinct, src, collider, family, at, hp))


def log_text(events, finish=0):
    lines = ['  Backend: D3D12',
             "[INFO | zora] found 'Zora Swim' v1.3.0 by Someone (~\\OneDrive\\Documentos\\Juegos\\mods\\zora.dusk)",
             "[INFO | dev.posture.post03_hit_classification] found 'POST03' v0.3.0 by X (C:\\Users\\Someone\\AppData\\Roaming\\mods\\p.dusk)",
             '[INFO | dusk::mods::loader] 2/2 mod(s) active',
             '[INFO | dev.posture.post03_hit_classification] POST03_INIT id=a version=0.3.0 source_commit=' + 'a' * 40 + ' host=2.0.3.0 capacity=64 log_cap=2000',
             '[INFO | dev.posture.post03_hit_classification] POST03_TARGET resolve=MOD_OK declared_target_resolved=1 same_address=1 flags=0x09 inline_sites_flag=0 multi_name_flag=1 (inline_sites_flag=1 => NOT_APPROVED)',
             '[INFO | dev.posture.post03_hit_classification] POST03_HOOK_INSTALLED target=x']
    lines += events
    lines += ['[WARNING | dusk] CalcDivideInfo: xDiv var1 out of range: 32'] * 3
    lines += ['[WARNING | aurora::texture] texture_replacement: expected 4x4 for mip C:\\Users\\Someone\\AppData\\x.png'] * 2
    lines += ['[INFO | dev.posture.post03_hit_classification] POST03_SUMMARY v=0.3.1 tag=SHUTDOWN calls=10 adm(cand/e_oc2/other/unsafe)=5/5/0/0 confirmed=0 pending=0 log(life/hit/other/summary written)=1/2/3/4 dropped=0/0/0/0',
              '[INFO | dev.posture.post03_hit_classification] POST03_HIT_SUMMARY tag=SHUTDOWN confirmed=%d finish=%d family(sword/shield/arrow/bomb/iron/boomerang/at40/sling/other/ambiguous/unavail)=0/0/0/0/0/0/0/0/0/0/%d' % (len(events) - finish, finish, len(events) - finish),
              '[INFO | dev.posture.post03_hit_classification] POST03_CONSERVATION tag=SHUTDOWN ok=1 started=%d judged=%d pending=0' % (len(events), len(events)),
              '[INFO | dusk::mods::loader] all mods unloaded']
    return '\n'.join(lines) + '\n'


class Extract(unittest.TestCase):
    def setUp(self):
        self.td = tempfile.TemporaryDirectory()
        self.d = Path(self.td.name)
        self.a = self.d / 'a.log'
        self.b = self.d / 'b.log'
        self.a.write_text(log_text([hit(1, 100, 490, 16), hit(2, 200, 491, 17, tg='cc0+cc1', dual=1, distinct=1)]), encoding='utf-8')
        self.b.write_text(log_text([hit(1, 100, 562, 19, collider='SELECTED_FRESH_MATCH_CC1', src='cc1', family='SWORD', at='0x04000002')] + [hit(2, 300, 561, 18)]), encoding='utf-8')

    def tearDown(self):
        self.td.cleanup()

    def test_counts_and_determinism(self):
        o1, o2 = self.d / 'o1', self.d / 'o2'
        s1 = X.run(self.a, self.b, o1)
        s2 = X.run(self.a, self.b, o2)
        self.assertEqual(s1['aggregate'], {'events': 4, 'hit_confirmed': 4, 'finish_native': 0, 'family_unavailable': 3, 'family_sword': 1})
        for f in ('POST03_AB_EVENTS_SANITIZED.tsv', 'POST03_AB_SUMMARY_SANITIZED.json'):
            self.assertEqual((o1 / f).read_bytes(), (o2 / f).read_bytes())
        self.assertEqual(s1['sessions']['A']['hook_target']['inline_sites_flag'], '0')  # no se confunde con la leyenda «(inline_sites_flag=1 => ...)»
        self.assertEqual(s1['sessions']['A']['incidents']['CalcDivideInfo_warning_lines'], 3)
        self.assertEqual(s1['sessions']['A']['incidents']['texture_replacement_warning_lines'], 2)

    def test_sha_check(self):
        with self.assertRaises(SystemExit):
            X.run(self.a, self.b, self.d / 'o', sha_a='0' * 64)

    def test_no_private_data_and_allowlist(self):
        out = self.d / 'o'
        X.run(self.a, self.b, out)
        blob = (out / 'POST03_AB_EVENTS_SANITIZED.tsv').read_text() + (out / 'POST03_AB_SUMMARY_SANITIZED.json').read_text()
        for bad in ('OneDrive', 'AppData', 'Users', 'Documentos', 'C:\\', '.dusk', 'Someone'):
            self.assertNotIn(bad, blob)
        header = (out / 'POST03_AB_EVENTS_SANITIZED.tsv').read_text().splitlines()[0].split('\t')
        self.assertEqual(header, X.TSV_COLS)
        for line in (out / 'POST03_AB_EVENTS_SANITIZED.tsv').read_text().splitlines()[1:]:
            self.assertEqual(len(line.split('\t')), len(X.TSV_COLS))
        j = json.loads((out / 'POST03_AB_SUMMARY_SANITIZED.json').read_text())
        self.assertEqual([m['name'] for m in j['sessions']['A']['mods']], ['Zora Swim', 'POST03'])  # nombre y version, nunca la ruta

    def test_rejects_non_allowlisted_event(self):
        bad = hit(1, 100, 490, 16).replace('family=UNAVAILABLE', 'family=UNAVAILABLE extra=1')
        self.a.write_text(log_text([bad]), encoding='utf-8')
        with self.assertRaises(ValueError):
            X.run(self.a, self.b, self.d / 'o')
        bad2 = hit(1, 100, 490, 16).replace('collider=SELECTED_COLLIDER_IMPLAUSIBLE', 'collider=C:\\Users\\x')
        self.a.write_text(log_text([bad2]), encoding='utf-8')
        with self.assertRaises(ValueError):
            X.run(self.a, self.b, self.d / 'o')

    def test_privacy_scanner_trips(self):
        with self.assertRaises(SystemExit):
            X.privacy_check('see C:\\Users\\x')
        with self.assertRaises(SystemExit):
            X.privacy_check('address 0x00007ff7ebfda770')
        X.privacy_check('hash ' + 'a' * 64, allowed=['a' * 64])


@unittest.skipUnless(os.environ.get('POST031_REAL_LOG_A') and os.environ.get('POST031_REAL_LOG_B'), 'logs reales fuera del repo no disponibles')
class RealLogs(unittest.TestCase):
    def test_real(self):
        with tempfile.TemporaryDirectory() as td:
            s = X.run(os.environ['POST031_REAL_LOG_A'], os.environ['POST031_REAL_LOG_B'], td,
                      'e495427aaf3ba3f5ee4c6b8ab887f24a4df290faa43455903da16f8fcffb1bb5', '92495e7359458c23c73eb2c5139c5b0f5b6bf46dfd50fb0359db85dfa8747a06')
        A, B = s['sessions']['A']['derived'], s['sessions']['B']['derived']
        self.assertEqual((A['events'], B['events']), (6, 20))
        self.assertEqual((A['hit_confirmed'], B['hit_confirmed']), (6, 18))
        self.assertEqual((A['finish_native'], B['finish_native']), (0, 2))
        self.assertEqual((A['family_unavailable'], B['family_unavailable']), (6, 14))
        self.assertEqual((A['family_sword'], B['family_sword']), (0, 4))
        self.assertEqual(s['aggregate'], {'events': 26, 'hit_confirmed': 24, 'finish_native': 2, 'family_unavailable': 20, 'family_sword': 4})


if __name__ == '__main__':
    unittest.main(verbosity=1)
