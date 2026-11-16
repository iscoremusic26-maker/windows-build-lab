import shutil, subprocess, sys, pathlib, os
T = pathlib.Path(sys.argv[1]); W = pathlib.Path(sys.argv[2])
src = (T / 'extract_post05_runtime.py').read_text()
M = [("if set(kv) not in (DEC_050, DEC_051):", "if False:"),
     ("if not rx.match(v):\n                        raise ValueError('POST05_DECISION", "if False:\n                        raise ValueError('POST05_DECISION"),
     ("if t and t.group(1) not in POST05_TAGS:", "if False:"),
     ("if not EXP_FIELDS[k].match(v):", "if False:"),
     ("v.append('two POST05_DECISION lines for event %d' % d['event'])", "pass"),
     ("v.append('HIT_CONFIRMED %d without POST05_DECISION' % ev)", "pass"),
     ("!= (h['id'], h['generation'], h['family'])", "!= (d['id'], d['generation'], d['family'])"),
     ("!= (h['mode_pre'], h['mode_post'], h['state_pre'], h['state_post'])", "!= (d['pre_mode'], d['post_mode'], d['pre_state'], d['post_state'])"),
     ("if p and d['posture'] !=", "if False and d['posture'] !="),
     ("v.append('non-confirmed event %d has a decision line' % ev)", "pass"),
     ("v.append('decision for unknown event %d' % ev)", "pass"),
     ("and d['decision'] != 'SUPPRESS_ELIGIBLE'", "and False"),
     ("if s.get(k) != val:", "if False:"),
     ("if s.get('verify_failed', 0) or s.get('recheck_failed', 0):", "if s.get('verify_failed', 0):"),
     ("if extra['conservation'].get('ok') != 1:", "if False:"),
     ("B.privacy_check(b, [a for a in allowed if a], words)", "pass"),
     ("viol += ['%s: %s' % (label, x) for x in bv + ev]", "viol += ['%s: %s' % (label, x) for x in ev]"),
     ("'applied': sum(d['action'] == 'SUPPRESSION_APPLIED' for d in decs.values())", "'applied': s.get('applied')"),
     ("if k in ('version', 'policy_id', 'writes_to_game', 'admission_policy', 'hook_skip_original')}", "}"),
]
surv = []
for i, (a, b) in enumerate(M):
    assert src.count(a) == 1, (i, a)
    d = W / ('m%02d' % i); d.mkdir(parents=True, exist_ok=True)
    shutil.copy(T / 'posture_base_extract.py', d); shutil.copy(T / 'test_extract_post05_runtime.py', d)
    (d / 'extract_post05_runtime.py').write_text(src.replace(a, b))
    # el test importa POST04/evidence_tools via HERE.parents[2]; replicar estructura
    t = (d / 'test_extract_post05_runtime.py').read_text().replace("HERE.parents[2] / 'POST04' / 'evidence_tools'", "%r" % str(T.parents[2] / 'POST04' / 'evidence_tools'))
    (d / 'test_extract_post05_runtime.py').write_text(t)
    r = subprocess.run([sys.executable, '-m', 'unittest', 'test_extract_post05_runtime'], cwd=d, capture_output=True, text=True, env={**os.environ, 'PYTHONDONTWRITEBYTECODE': '1'})
    print(i, 'KILLED' if r.returncode else 'SURVIVED', a[:60])
    if not r.returncode: surv.append(i)
print('killed %d/%d' % (len(M) - len(surv), len(M)), 'survivors', surv)
