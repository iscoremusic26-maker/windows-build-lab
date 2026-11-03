"""Review barrier negatives, distinct from behavioral mutation kills."""
from pathlib import Path
import subprocess,tempfile,shutil,sys,json
root=Path(__file__).resolve().parents[1]
cases=[
 ('native_mode_without_readback','src/cp04_adapter.inc','return a->mActionMode==mode&&a->mOcState==0;','return true;'),
 ('foreign_speed_write','src/cp04_adapter.inc','!cp04::normal_rate(a->mpMorf->getPlaySpeed())','false'),
 ('unbuffered_native_restart','src/cp04_adapter.inc','c4.shield.can_restart(g.p8.tick,safe,attack_recovery||slip_recovery||free_recovery)','true'),
 ('real_clip_check_removed','src/cp04_adapter.inc','link->checkUnderMove0BckNoArc(daAlink_c::ANM_SHIELD_ATTACK)','true'),
 ('held_B_grants_credits','src/shield_chain_core.hpp','!trigger)return','false)return'),
 ('native_contact_bypass','src/mod.cpp','g.cur.pv = g.parry.judge(in);','g.cur.pv.result=post08::Result::Confirmed;'),
 ('HUD_root_hidden','src/mod.cpp','ctx.owned[i]=ctx.mask[i]=false;','ctx.owned[i]=ctx.mask[i]=true;'),
 ('Blue_reward_changed','src/balance_core.hpp','{{25,40,50}','{{25,20,50}'),
 ('penalty_shortened','src/lnks2fix_core.hpp','kTotalPenaltyTicks = 75','kTotalPenaltyTicks = 30'),
 ('native_attack_original_changed','tests/reference/cp04_native_attack.inc','getFrame() >= 14.0f','getFrame() >= 1.0f'),
 ('Expansion_oracle_speed_changed','tests/cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/blue_ai_policy.hpp','kRatePair=1.15f','kRatePair=1.0f'),
]
results=[]
for name,rel,old,new in cases:
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);shutil.copytree(root/'src',p/'src');shutil.copytree(root/'ci',p/'ci');shutil.copytree(root/'tests',p/'tests');f=p/rel;s=f.read_text(encoding='utf-8');assert old in s,name;f.write_text(s.replace(old,new,1),encoding='utf-8')
  c=subprocess.run([sys.executable,'-I',str(root/'ci/source_audit.py'),str(p)],capture_output=True,text=True,timeout=60);r=json.loads(c.stdout);assert c.returncode!=0 and r['errors'],name;results.append({'case':name,'rejected':True})
print(json.dumps({'status':'NEGATIVES_PASS','count':len(results),'cases':results},indent=2))
