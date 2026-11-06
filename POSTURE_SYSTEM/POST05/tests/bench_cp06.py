"""CP06 synthetic CPU benchmark of production combat callbacks (base 0.11.10 tree vs candidate).
Usage: bench_cp06.py <POST05 tree> [--cp06] [actors ticks reps]. NOT an FPS or game measurement."""
from pathlib import Path
import os,re,subprocess,tempfile,sys
tree=Path(sys.argv[1]).resolve();cp06='--cp06' in sys.argv;rest=[a for a in sys.argv[2:] if a!='--cp06']
here=Path(__file__).resolve().parent
ns={'__file__':str(tree/'tests/test_cp05_fix02_guard.py')}
src=(tree/'tests/test_cp05_fix02_guard.py').read_text(encoding='utf-8')
head_code=src.split("mod=(root/'src/mod.cpp')")[0]
exec(head_code.replace("root=Path(__file__).resolve().parents[1]","root=Path(%r)"%str(tree)),ns);head=ns['head'];root=tree
mod=(root/'src/mod.cpp').read_text(encoding='utf-8')
lease=mod[mod.index('void fix01_at_mask('):mod.index('// POST06: segundo escritor')]
new=(root/'src/cp05_fix02_adapter.inc').read_text(encoding='utf-8');contact=re.search(r'^void fix02_guard_contact\(.*?^\}\n',new,re.M|re.S)[0]
input_fn=re.search(r'^void on_stick_post_fx\(.*?^\}\n',mod,re.M|re.S)[0]
prefix=(root/'tests/fixtures/cp05_adapter_scenario.inc').read_text(encoding='utf-8').split('void native_tick(')[0].replace('ROOT',root.as_posix())
native=''.join((root/'tests/reference'/n).read_text(encoding='utf-8') for n in ['cp04_native_attack.inc','cp05_native_find.inc','cp05_native_cc_set.inc','fix02_native_cc_register.inc','fix02_native_cc_clear.inc'])
extra=''
if cp06:
 head=head.replace('#include <algorithm>','#include <algorithm>\n#include "'+str(root/'src/cp06_combat_core.hpp')+'"',1)
 head=head.replace('enum{LOG_LEVEL_INFO,MOD_OK,MOD_ERROR};','enum{LOG_LEVEL_INFO,MOD_OK,MOD_ERROR,LOG_LEVEL_WARN};',1)
 head=head.replace('post08::Verdict pv;}cur;','post08::Verdict pv;postbal01::Snapshot balance_profile;}cur;',1).replace('struct{std::uint64_t tick=1;}p8;','struct{std::uint64_t tick=1;int layout=1;}p8;',1)
 head=head.replace('HitObj mTgCyls[3];void* mShieldModel=this;','cCcD_ObjHitInf mTgCyls[3];void* mShieldModel=this;',1)
 head=head.replace('struct cCcD_ObjHitInf:HitObj{','struct cCcD_ObjHitInf:HitObj{bool tg_shield=false;const void* tg_obj=nullptr;bool ChkTgShieldHit(){return tg_shield;}HitObj* GetTgHitObj(){return static_cast<HitObj*>(const_cast<void*>(tg_obj));}bool ChkAtShieldHit(){return shield;}',1)
 head=head.replace('struct{struct{bool isPauseFlag(){return pause;}','struct{struct{float item_life=0;unsigned char item_type=0;float getItemLifeCount(){return item_life;}unsigned char getItemLifeCountType(){return item_type;}void setItemLifeCount(float h,unsigned char t){item_life+=h;item_type=t;}bool isPauseFlag(){return pause;}',1)
 extra=(root/'src/cp06_combat_adapter.inc').read_text(encoding='utf-8')
source=head+(root/'src/cp04_adapter.inc').read_text(encoding='utf-8')+(root/'src/cp05_adapter.inc').read_text(encoding='utf-8')+input_fn+prefix+lease+contact+extra+native+(here/'bench/cp06_bench.inc').read_text(encoding='utf-8')
with tempfile.TemporaryDirectory() as td:
 p=Path(td);f=p/'bench.cpp';f.write_text(source,encoding='utf-8');exe=p/('bench.exe' if os.name=='nt' else 'bench')
 flags=['/DBENCH_CP06'] if cp06 and os.name=='nt' else (['-DBENCH_CP06'] if cp06 else [])
 cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8','/O2','/DAVOID_UB=1',*flags,str(f),'/Fe'+str(exe),'/Fo'+str(p/'bench.obj')] if os.name=='nt' else ['g++','-std=c++20','-O2','-DAVOID_UB=1',*flags,'-w',str(f),'-o',str(exe)]
 c=subprocess.run(cmd,capture_output=True,text=True,timeout=300)
 if c.returncode:print(c.stdout[-4000:]+c.stderr[-4000:]);raise SystemExit(c.returncode)
 c=subprocess.run([str(exe),*rest],capture_output=True,text=True,timeout=600);print(c.stdout+c.stderr[-2000:]);raise SystemExit(c.returncode)
