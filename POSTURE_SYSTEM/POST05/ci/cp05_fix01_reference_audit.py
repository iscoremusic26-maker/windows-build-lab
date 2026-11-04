"""Exact native geometry/damage/mode references and actual SDK call ordering."""
from pathlib import Path
import sys,json
p=Path(__file__).resolve().parents[1];sdk=Path(sys.argv[1]);errors=[]
refs=[('src/d/actor/d_a_e_oc.cpp','void daE_OC_c::setActionMode(int i_action, int i_state) {','\nint daE_OC_c::getCutType()','cp05_native_set_action.inc'),('src/d/actor/d_a_e_oc.cpp','void daE_OC_c::offTgSph() {','\nvoid daE_OC_c::damage_check()','cp05_native_off_tg.inc'),('src/d/actor/d_a_e_oc.cpp','void daE_OC_c::damage_check() {','\nvoid daE_OC_c::setBck(','cp05_native_damage.inc'),('src/d/actor/d_a_e_oc.cpp','void daE_OC_c::cc_set() {','\nint daE_OC_c::execute()','cp05_native_cc_set.inc'),('src/SSystem/SComponent/c_m3d.cpp','bool cM3d_Cross_CylSph(cM3dGCyl const* cyl, cM3dGSph const* sph, f32* param_2) {','\nbool cM3d_Cross_CylSph(cM3dGCyl const* cyl, cM3dGSph const* sph, Vec*','cp05_native_cyl_sph.inc'),('src/d/d_cc_s.cpp','bool dCcS::ChkShieldFrontRange(','\nvoid dCcS::CalcTgPlusDmg','cp05_native_shield.inc')]
for f,a,b,n in refs:
 text=(sdk/f).read_text(encoding='utf-8');i=text.index(a);j=text.index(b,i);fixture=(p/'tests/reference'/n).read_text(encoding='utf-8')
 if text[i:j].rstrip()+'\n'!=fixture:errors.append('Native FIX01 fixture mismatch: '+n)
s=(sdk/'src/d/actor/d_a_e_oc.cpp').read_text(encoding='utf-8');action=s[s.index('void daE_OC_c::action() {'):s.index('void daE_OC_c::mtx_set() {')];execute=s[s.index('int daE_OC_c::execute() {'):s.index('static int daE_OC_Execute(')]
assert action.index('OffAtSetBit();')<action.index('damage_check();')<action.index('executeAttack();')<action.index('mpMorf->play(')
assert execute.index('action();')<execute.index('mtx_set();')<execute.index('cc_set();')
for token in ['if (field_0x6cc) {','if (field_0x6ca) {','--field_0x6ca;','if (field_0x6c2) {','--field_0x6c2;']:
 if token not in s:errors.append('Native timer contract missing: '+token)
collision=(sdk/'src/d/d_cc_s.cpp').read_text(encoding='utf-8')
for token in ['bool chk_shield = ChkShield(','atObjInf->OnAtShieldHit();','tgObjInf->OnTgShieldHit();']:
 if token not in collision:errors.append('Native shield flag route missing: '+token)
print(json.dumps({'status':'NATIVE_REFERENCE_SOURCE_PASS' if not errors else 'NATIVE_REFERENCE_SOURCE_FAIL','sdk_commit':'40457c6adb381928e4b5fef6ed459ed291edd5e2','exact_new_references':len(refs),'ordering':'OffAt -> native damage_check plus its POST callbacks -> original Attack/Find -> play -> matrices -> cc_set -> execute POST','raw_Tg':'Native cooldown can ignore it; POST05 can suppress its zeroHP reaction. Observe committed outcome before cancelling.','geometry':'Actual cc_set centres/radii and native pair/front predicates; lab joint matrices and broadphase I/O remain synthetic','authentic_BCK_joint_poses_available':False,'native_game_executed':False,'errors':errors},indent=2));sys.exit(bool(errors))
