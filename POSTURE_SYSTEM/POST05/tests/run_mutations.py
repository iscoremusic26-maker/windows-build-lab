"""Rework mutations. Historical 240 are retained at immutable a0a815356394."""
from pathlib import Path
import subprocess,tempfile,shutil,concurrent.futures,json,sys,os
root=Path(__file__).resolve().parents[1]
logic=[
 ('nondecisive_fall','src/break_core.hpp','if(v.hit&&v.native_hp>1&&(v.mode==5||v.mode==6))','if(false)'),
 ('helm_episode_lost','src/helm_core.hpp','if(link_id!=player||!helm_proc)seen=false;','seen=false;'),
 ('timed_fall','src/break_core.hpp','a.act=Act::Recover;a.why=v.stun_ended','a.act=Act::Bonus;a.why=v.stun_ended'),
 ('old_120','src/break_core.hpp','a.extra=(base+1)/2','a.extra=base/5'),
 ('shield_bonus','src/break_core.hpp','a.act=Act::Stun;a.why=Why::ShieldPrepared','a.act=Act::Bonus;a.why=Why::ShieldPrepared'),
 ('break_bonus','src/break_core.hpp','a.act=Act::Stun;a.why=Why::BreakStanding','a.act=Act::Bonus;a.why=Why::BreakStanding'),
 ('duplicate_bonus','src/break_core.hpp','||a.token<=s.last_commit',''),
 ('foreign_sword','src/break_core.hpp','v.sword&&v.from_link','v.sword'),
 ('helm_without_shield','src/break_core.hpp','(!v.helm||s.shield_prepared)','true'),
 ('renew_shield_window','src/break_core.hpp','s.shield_prepared=true;++c.shields','s.punish_start=s.exec;s.shield_prepared=true;++c.shields'),
 ('long_timeout','src/break_core.hpp','kPunishMaxExec=90','kPunishMaxExec=180'),
 ('wrong_abort_posture','src/break_core.hpp','a.rearm_to=50','a.rearm_to=0'),
 ('wrong_timeout_posture','src/break_core.hpp','Why::NativeStunEnded:Why::TimeoutStanding;a.rearm_to=50','Why::NativeStunEnded:Why::TimeoutStanding;a.rearm_to=0'),
 ('early_rearm','src/break_core.hpp','s.phase==Phase::GettingUp&&v.mode==3&&v.state==5','s.phase==Phase::Down&&v.mode==3&&v.state==5'),
 ('no_normalize_helm','src/helm_core.hpp','r.hp=pre>r.base?pre-r.base:0','r.hp=native'),
 ('heal_native_loss','src/helm_core.hpp','r.hp=pre>r.base?pre-r.base:0','r.hp=pre>floor?pre-floor:0'),
 ('double_rounding','src/break_core.hpp','a.extra=(base+1)/2','a.extra=(base+2)/2'),
]
source=[('extra_hp','r->health=7;'),('power','r->mSphs_at[0].SetAtAtp(6);'),('anim_rate','r->mpMorf->setPlaySpeed(2);'),('cooldown_elsewhere','r->field_0x6cc=0;'),('second_writer','r->mActionMode=6;')]
def one(item):
 name,fn,old,new=item
 with tempfile.TemporaryDirectory() as td:
  td=Path(td);shutil.copytree(root/'src',td/'src');shutil.copytree(root/'tests',td/'tests');p=td/fn;s=p.read_text();assert old in s,name;p.write_text(s.replace(old,new,1))
  exe=td/('lab.exe' if os.name=='nt' else 'lab');cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(td/'tests/break_lab.cpp'),'/Fe'+str(exe),'/Fo'+str(td/'lab.obj')] if os.name=='nt' else ['g++','-std=c++20','-O1',str(td/'tests/break_lab.cpp'),'-o',str(exe)]
  r=subprocess.run(cmd,capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr
  r=subprocess.run([str(exe)],capture_output=True,text=True);return {'mutation':name,'detected':r.returncode!=0}
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as e:results=list(e.map(one,logic))
for name,code in source:
 with tempfile.TemporaryDirectory() as td:
  td=Path(td);shutil.copytree(root/'src',td/'src');p=td/'src/mod.cpp';s=p.read_text();p.write_text(s.replace('  r->health = static_cast<s16>(hp);','  r->health = static_cast<s16>(hp);\n  '+code,1))
  r=subprocess.run([sys.executable,str(root/'ci/source_audit.py'),str(td)],capture_output=True,text=True);results.append({'mutation':name,'detected':r.returncode!=0})
print(json.dumps({'mutations':results,'count':len(results),'detected':sum(x['detected'] for x in results)},indent=2));sys.exit(0 if all(x['detected'] for x in results) else 1)
