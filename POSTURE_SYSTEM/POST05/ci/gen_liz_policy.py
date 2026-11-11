"""Regenerates ci/liz_policy.json (developer tool, never run by CI): the exact counted inverse patch that turns every changed 0.11.12 production
file back into its 0.11.12 bytes, and the hash pins of the modules added since. Usage, from the repository root of the worktree:
  python3 POSTURE_SYSTEM/POST05/ci/gen_liz_policy.py <git-ref-of-0.11.12, e.g. 2a57ebe>
The audit (liz_audit.py) then proves the patch: applied to today's files it must reproduce the frozen 0.11.12 hashes byte for byte."""
from pathlib import Path
import difflib, hashlib, json, subprocess, sys

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parent
sys.path.insert(0, str(HERE))
import liz_audit  # noqa: E402


def git_show(ref, rel):
    root = subprocess.run(['git', 'rev-parse', '--show-toplevel'], capture_output=True, text=True, cwd=PROJECT, check=True).stdout.strip()
    rel_repo = (PROJECT / rel).resolve().relative_to(root).as_posix()
    return subprocess.run(['git', 'show', '%s:%s' % (ref, rel_repo)], capture_output=True, cwd=root, check=True).stdout.decode('utf-8')


def hunks(old, new):
    """Inverse edits {before: old text, after: new text}: one per group of nearby changes, context grown until 'after' is unique in new."""
    a, b = old.splitlines(keepends=True), new.splitlines(keepends=True)
    sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
    out = []
    for group in sm.get_grouped_opcodes(2):
        i1, j1 = group[0][1], group[0][3]
        i2, j2 = group[-1][2], group[-1][4]
        ctx = 0
        while True:
            bi1, bi2 = max(0, i1 - ctx), min(len(a), i2 + ctx)
            bj1, bj2 = max(0, j1 - ctx), min(len(b), j2 + ctx)
            before, after = ''.join(a[bi1:bi2]), ''.join(b[bj1:bj2])
            if after and new.count(after) == 1:
                break
            ctx += 1
            if ctx > 60:
                raise RuntimeError('cannot make a unique hunk')
        out.append({'before': before, 'after': after})
    return out


def main(ref):
    pol = json.loads((HERE / 'liz_policy.json').read_text(encoding='utf-8'))
    inverse = {}
    for rel in pol['baseline_files']:
        if not rel.endswith(('.cpp', '.hpp', '.inc', '.h')):
            continue
        old = git_show(ref, rel)
        new = (PROJECT / rel).read_text(encoding='utf-8')
        if old == new:
            continue
        hs = hunks(old, new)
        t = new
        for e in reversed(hs):
            assert t.count(e['after']) == 1
            t = t.replace(e['after'], e['before'], 1)
        if t != old:
            raise RuntimeError('inverse patch does not reproduce ' + rel)
        inverse[Path(rel).name] = hs
    pol['inverse_patch'] = inverse
    pol['files'] = {'src/' + n: hashlib.sha256((PROJECT / 'src' / n).read_bytes()).hexdigest() for n in sorted(liz_audit.NEW)}
    pol['version'] = '0.12.3'
    pol['note'] = ('LIZALFOS 0.12.0 .. FIX03 0.12.3: every 0.11.12 production file must be reproduced byte for byte by this exact counted inverse patch; '
                   'the modules added since 0.11.12 (Lizalfos brain/adapter, Link break damage, FIX03 lock-on and parry FX) are hash-pinned.')
    (HERE / 'liz_policy.json').write_text(json.dumps(pol, indent=1, ensure_ascii=False) + '\n', encoding='utf-8')
    print(json.dumps({'inverse_patch': {k: len(v) for k, v in inverse.items()}, 'files': list(pol['files'])}, indent=1))


if __name__ == '__main__':
    main(sys.argv[1])
