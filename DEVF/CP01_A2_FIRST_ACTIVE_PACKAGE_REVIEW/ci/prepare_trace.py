"""Generate only an honest arming trace event; never edit historical source."""
import argparse,json,hashlib,difflib
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--pilot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
changes={'trace.hpp':[('LogBudgetExhausted,LogSinkFailed };','LogBudgetExhausted,LogSinkFailed,PackageProviderArmed };'),('std::array<Record,26> last_{};std::array<bool,26> seen_{};','std::array<Record,27> last_{};std::array<bool,27> seen_{};')], 'trace.cpp':[('E(ModuleInit,"MODULE_INIT");','E(PackageProviderArmed,"PACKAGE_PROVIDER_ARMED_NOT_EXECUTION_PERMISSION");E(ModuleInit,"MODULE_INIT");')]}
ledger={};diff=[]
for name,edits in changes.items():
 before=(a.pilot/'src'/name).read_text();s=before
 for x,y in edits:
  if s.count(x)!=1:raise RuntimeError('Trace anchor not unique')
  s=s.replace(x,y)
 (a.output/name).write_text(s,encoding='utf-8',newline='\n');sha=lambda s:hashlib.sha256(s.encode()).hexdigest()
 ledger[name]={'input_lf_sha256':sha(before),'output_lf_sha256':sha(s),'edits':len(edits)}
 diff.extend(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='pilot/'+name,tofile='active/'+name))
(a.output/'trace-ledger.json').write_text(json.dumps(ledger,indent=2)+'\n');(a.output/'trace-diff.log').write_text(''.join(diff))
