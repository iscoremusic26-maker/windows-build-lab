"""CP05 store widths/offsets: synthetic encoding negatives, separate from
actual MSVC instructions audited after linking. Never executes a DLL.
"""
from pathlib import Path
import sys,json
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'ci'))
import cp05_pe_audit as audit
n=0
rate=bytes.fromhex('89 48 28');timer=bytes.fromhex('66 89 81 e2 08 00 00')
for code,valid in [(rate,True),(bytes.fromhex('f3 0f 11 41 28'),True),(bytes.fromhex('44 89 48 28'),True),(bytes.fromhex('48 89 48 28'),False),(bytes.fromhex('89 48 2c'),False),(bytes.fromhex('89 44 28'),False),(b'',False)]:
 e,*_=audit.stores({'cp05_native_rate':code,'cp05_native_timer':timer});assert bool(e)!=valid,(code.hex(),e);n+=1
for code,valid in [(timer,True),(bytes.fromhex('66 44 89 81 e2 08 00 00'),True),(bytes.fromhex('66 48 89 81 e2 08 00 00'),False),(bytes.fromhex('89 81 e2 08 00 00'),False),(bytes.fromhex('66 89 81 e0 08 00 00'),False),(bytes.fromhex('66 89 84 e2 08 00 00'),False),(b'',False)]:
 e,*_=audit.stores({'cp05_native_rate':rate,'cp05_native_timer':code});assert bool(e)!=valid,(code.hex(),e);n+=1
print(json.dumps({'status':'CP05_PE_POLICY_PASS','checks':n,'native_runtime':False}))
