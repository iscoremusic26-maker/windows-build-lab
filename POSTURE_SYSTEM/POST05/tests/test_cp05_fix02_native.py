"""Real native damage/SetAction/Attack/Find and POST05 transaction, plus the
actual FIX02 production terminal proof. SDK I/O and Link origin are modeled.
"""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1];ns={'__file__':str(root/'tests/test_cp05_fix01_native.py')}
exec((root/'tests/test_cp05_fix01_native.py').read_text(encoding='utf-8').split('results=[]')[0],ns)
head=ns['head'];prefix=ns['prefix'];helpers=ns['helpers'].split('void guard_controller_ab(){')[0]
old='struct{post04::PostureEngine pe;';assert head.count(old)==1
head=head.replace(old,'struct{bool reentered=false;struct{std::uintptr_t actor=0;std::uint32_t id=0;std::uint64_t generation=0;int slot=-1;bool tracked=false,from_link=false;post05::LiveMode pre_live;post03::NativeAux pre_aux;}cur;post04::PostureEngine pe;',1)
head+='#include "'+str(root/'src/cp05_fix02_core.hpp')+'"\nstruct{std::uint64_t terminal_seen=0;const char* last_terminal_gate="NONE";}f2;\n'
adapter=(root/'src/cp05_fix02_adapter.inc').read_text(encoding='utf-8')
proof=re.search(r'^bool fix02_terminal_proof\(.*?^\}\n',adapter,re.M|re.S)[0]
helpers=helpers.replace('pre.facts.status_10000=false;','pre.facts.status_10000=a.blue;')
helpers=helpers.replace('pre.facts.name=post03::NameKind::E_OC2;','pre.facts.name=a.blue?post03::NameKind::E_OC:post03::NameKind::E_OC2;')
anchor='g.eng.policy=post03::AdmissionPolicy::V2Compat;'
helpers=helpers.replace(anchor,'g.cur={};g.cur.actor=reinterpret_cast<std::uintptr_t>(&a);g.cur.slot=slot_of(a);g.cur.id=a.id;g.cur.generation=g.eng.obs.ledger.slot(g.cur.slot).generation;g.cur.tracked=true;g.cur.from_link=true;g.cur.pre_live=damage_live(a);g.cur.pre_aux=pre.aux;\n '+anchor,1)
needle='lnks1fix::continuation(pre.snap.mode,pre.snap.state,pre.aux,post.aux)==lnks1fix::Continuation::Safe'
assert helpers.count(needle)==1
helpers=helpers.replace(needle,'('+needle+'||(use_fix02&&fix02_terminal_proof(&a,post,report)))')
helpers=helpers.replace('if(hit&&suppression_enabled){','last_native_post=post;last_native_report=report;\n if(hit&&suppression_enabled){')
helpers='static bool use_fix02=false;post03::PostReads last_native_post;post03::HitReport last_native_report;\n'+helpers
source=head+(root/'src/cp04_adapter.inc').read_text(encoding='utf-8')+(root/'src/cp05_adapter.inc').read_text(encoding='utf-8')+''.join((root/'tests/reference'/n).read_text(encoding='utf-8') for n in ['cp05_native_set_action.inc','cp05_native_off_tg.inc','cp04_native_attack.inc','cp05_native_find.inc','cp05_native_damage.inc'])
mod=(root/'src/mod.cpp').read_text(encoding='utf-8');source+=re.search(r'^void on_stick_post_fx\(.*?^\}\n',mod,re.M|re.S)[0]+prefix+proof+helpers+(root/'tests/fixtures/cp05_fix02_native.inc').read_text(encoding='utf-8')
with tempfile.TemporaryDirectory() as td:
 p=Path(td);f=p/'terminal.cpp';f.write_text(source,encoding='utf-8');exe=p/('terminal.exe' if os.name=='nt' else 'terminal')
 cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8','/DAVOID_UB=1',str(f),'/Fe'+str(exe),'/Fo'+str(p/'terminal.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1','-DAVOID_UB=1','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',str(f),'-o',str(exe)]
 c=subprocess.run(cmd,capture_output=True,text=True,timeout=180)
 if c.returncode:print(c.stdout+c.stderr);raise SystemExit(c.returncode)
 c=subprocess.run([str(exe)],capture_output=True,text=True,timeout=90,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'));print(c.stdout+c.stderr);raise SystemExit(c.returncode)
