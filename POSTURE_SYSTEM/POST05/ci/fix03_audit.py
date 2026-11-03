"""Exact FIX03 drawing policy plus 0.11.4 presentation/alignment parity.
Does not replace frozen0.11.2 combat wall or Windows PE/store audits.
"""
from pathlib import Path
import json,sys,hashlib
sys.path.insert(0,str(Path(__file__).resolve().parent))
import store_audit

def audit(project):
 p=Path(project);s=(p/'src/mod.cpp').read_text(encoding='utf-8');pin=json.loads((p/'ci/fix03_policy.json').read_text());errors=[]
 for n,h in pin['unchanged_functions'].items():
  if store_audit.sha(store_audit.function(s,n))!=h:errors.append('FIX03 changed preserved0.11.4 function: '+n)
 for n,h in pin['unchanged_headers'].items():
  if hashlib.sha256((p/'src'/n).read_bytes()).hexdigest()!=h:errors.append('FIX03 changed preserved header: '+n)
 try:block=s[s.index('// FIX03: suppression requires'):s.index('// Link viva')]
 except ValueError:return errors+['FIX03 ownership policy missing']
 if store_audit.sha(store_audit.canonical(block))!=pin['owner_policy_sha256']:errors.append('FIX03 owner-first exact policy changed: root/foreign suppression not authorized')
 fixture=p/'tests/fixtures/hudfix02_native_base.inc'
 if hashlib.sha256(fixture.read_bytes()).hexdigest()!=pin['base_fixture_sha256']:errors.append('FIX03 historical reproduction changed')
 block=s[s.index('namespace align02native {'):s.index('// One snapshot and one player draw')]
 if hashlib.sha256(block.encode()).hexdigest()!=pin['exact_alignment_and_packet_block_sha256']:errors.append('FIX03 alignment/packet exact0.11.4 bytes changed')
 return errors
if __name__=='__main__':
 p=Path(sys.argv[1]);e=audit(p);print(json.dumps({'status':'SOURCE_AUDIT_PASS' if not e else 'SOURCE_AUDIT_FAIL','preserved_base':'061e1793d687c958d10e9d0c63ae898c369bda24','errors':e,'native_runtime':False},indent=2));sys.exit(bool(e))
