int daAlink_c::procGuardAttackInit(){return 1;}
void synthetic_pose(daE_OC_c& a,float frame){for(unsigned n=0;n<32;++n)for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)a.morf.model.m[n][r][c]=r==c?1.f:0.f;a.morf.model.m[12][0][3]=130-10*frame;a.morf.model.m[12][1][3]=80;}
bool native_pair(cCcD_ObjHitInf& at,HitObj& tg,const cM3dGCyl& cyl){
 if(!at.on)return false;cM3dGSph sphere{at.GetC(),at.GetR()};float penetration=-1;if(!cM3d_Cross_CylSph(&cyl,&sphere,&penetration))return false;ck(penetration>=0,"native pair has nonnegative overlap");return collision_world.ChkShield(&at,&tg,&at,&tg,&sphere.centre);
}
unsigned native_samples(float rate,bool clip_c){daE_OC_c a;a.blue=false;a.mName="E_OC2";variant_c=clip_c;a.executeAttack();a.morf.mFrameCtrl.frame=1;a.morf.setPlaySpeed(rate);unsigned n=0;
 for(unsigned t=0;t<80&&a.mOcState!=0;++t){a.mSphs_at[0].on=a.mSphs_at[1].on=false;a.executeAttack();if(a.mSphs_at[0].on)++n;a.morf.mFrameCtrl.frame+=a.morf.getPlaySpeed();if(a.morf.mFrameCtrl.frame>=a.morf.mFrameCtrl.end){a.morf.mFrameCtrl.frame=a.morf.mFrameCtrl.end-.001f;a.morf.stopped=true;}}
 return n;
}
int main(){
 for(bool clip_c:{false,true}){const unsigned slow=native_samples(1,clip_c),normal=native_samples(1.15f,clip_c),old_black=native_samples(1.45f,clip_c),old_red=native_samples(1.5f,clip_c);const unsigned cp06_max=native_samples(cp05::kRedMaxRate,clip_c);ck(normal>=old_black&&normal>=old_red&&slow>=normal&&cp06_max>=old_red&&cp06_max>=6,"CP06 maximum keeps at least the human-approved Blue1.50 sampling density (>=6 armed passes)");std::printf("CP05_FIX01_NATIVE_SAMPLES clip=%s rate1=%u rate115=%u old_rate145=%u old_rate150=%u cp06_rate135=%u physical_trajectory=NOT_AUTHENTICATED\n",clip_c?"C":"B",slow,normal,old_black,old_red,cp06_max);}
 unsigned episodes=0,positive=0,negative=0;HitObj tg;tg.owner=&player;tg.shield=true;tg.heading=0;tg.range=0x2000;
 for(auto kind:{cp05::Kind::Red4,cp05::Kind::Black8,cp05::Kind::Black7})for(unsigned swing=1;swing<=cp05::count(kind);++swing)for(bool clip_c:{false,true}){
  daE_OC_c a;a.blue=false;a.mName="E_OC2";a.mActionMode=4;a.mOcState=0;variant_c=clip_c;a.executeAttack();ck(a.checkBck(clip_c?6:5),"original chooses genuine B/C before geometric sampling");a.morf.mFrameCtrl.frame=1;a.morf.setPlaySpeed(cp05::rate(kind,swing));const float r=a.morf.getPlaySpeed();ck(r==cp05::rate(kind,swing)&&r>=1.05f&&r<=cp05::kRedMaxRate&&(kind==cp05::Kind::Red4||r<=cp05::kBlackMaxRate),"CP06 candidate stays inside the audited 1.05..1.35 envelope (Black<=1.30)");unsigned windows=0;bool last=false;unsigned block_samples=0;
  for(unsigned t=0;t<70&&a.mActionMode==4&&a.mOcState!=0;++t){a.mSphs_at[0].on=a.mSphs_at[1].on=false;a.executeAttack();a.morf.mFrameCtrl.frame+=a.morf.getPlaySpeed();if(a.morf.mFrameCtrl.frame>=a.morf.mFrameCtrl.end){a.morf.mFrameCtrl.frame=a.morf.mFrameCtrl.end-.001f;a.morf.stopped=true;}
   synthetic_pose(a,a.morf.getFrame());collision_world.registered.clear();a.cc_set();ck(collision_world.registered.size()==4,"exact native cc_set registers two body and two attack spheres");
   ck(a.mSphs_at[0].GetR()==45&&a.mSphs_at[1].GetR()==30,"authored native AT radii remain unchanged");ck(a.mSphs_at[0].GetC().z==70&&a.mSphs_at[1].GetC().z==20,"exact weapon local offsets from original cc_set");
   const bool armed=a.mSphs_at[0].on||a.mSphs_at[1].on;if(armed&&!last)++windows;last=armed;
   for(unsigned n=0;n<2;++n){auto& at=a.mSphs_at[n];at.owner=&a;at.particle_y=short(0x8000);tg.shield=true;const cM3dGCyl near{{0,0,0},35,160},far{{1000,0,0},35,160},high{{0,500,0},35,160};
    if(native_pair(at,tg,near)){++positive;++block_samples;}
    ck(!native_pair(at,tg,far),"no false block outside native sphere/cylinder overlap");++negative;
    ck(!native_pair(at,tg,high),"no false block outside native vertical overlap");++negative;
    at.particle_y=0;ck(!native_pair(at,tg,near),"native shield front range refuses a rear attack");++negative;at.particle_y=short(0x8000);
    tg.shield=false;ck(!native_pair(at,tg,near),"sword proximity without a raised native shield is not a block");++negative;tg.shield=true;
    at.no_guard=true;ck(!native_pair(at,tg,near),"native no-guard attribute is authoritative");++negative;at.no_guard=false;
   }
  }
  ck(windows==1&&block_samples>0,"every candidate swing has one registered native window and an overlapping sample in this synthetic scene");++episodes;
 }
 std::printf("CP05_FIX01_GEOMETRY checks=%llu failures=0 RESULT=LAB_PASS episodes=%u native_shield_pairs=%u negative_cases=%u native_cc_set_and_pair_and_shield_exact=true joint_poses=SYNTHETIC engine_broadphase=NOT_EXECUTED visual=HUMAN_RUNTIME_PENDING\n",checks,episodes,positive,negative);
}
