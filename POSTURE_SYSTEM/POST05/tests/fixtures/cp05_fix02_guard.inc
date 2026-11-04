// Native collision registration list is rebuilt each pass. Contacts injected
// below model one genuine native last-window shield collision, not a parry.
post08::ArmTrack guard_arm;post08::Parry guard_judge;
struct GuardRun{unsigned starts=0,windows=0,early=0,cleared=0,mask=0;bool prior_at=false;};
void guard_tick(daE_OC_c& a,int controller,bool held,GuardRun& result,unsigned level=1,bool shared=false,bool same_frame=false,bool repeated=false){
 if(!same_frame)++g.p8.tick;player_guard=held;lf.r_held=lf.guard_accepted=held;const int slot=slot_of(a);auto& p=policy_ports[slot];auto* self=&a;void* args[]={&self};
 const auto blue_pre=devf_blue::make_snap(level==1,true,player_distance,a.mActionMode,a.mOcState,a.mOldActionMode,a.field_0x6c0,a.field_0x6c2);
 if(a.atp!=1)expansion_pre(a,p);
 if(controller>=1)on_eoc_pre_cp04(nullptr,args,nullptr,nullptr);if(controller>=2)on_eoc_pre_cp05(nullptr,args,nullptr,nullptr);
 if(a.field_0x6ca)--a.field_0x6ca;if(a.field_0x6c0)--a.field_0x6c0;if(a.field_0x6c2)--a.field_0x6c2;a.mSphs_at[0].on=a.mSphs_at[1].on=false;
 auto& c=g.cur;c={};c.actor=reinterpret_cast<std::uintptr_t>(&a);c.slot=slot;c.id=a.id;c.generation=g.eng.obs.ledger.slot(slot).generation;c.tracked=true;
 auto& in=c.pin;in.slot=slot;in.id=a.id;in.generation=c.generation;in.tracked=true;in.hp=a.health;in.mode=a.mActionMode;in.state=a.mOcState;in.anim_ok=true;in.anim_frame=a.morf.getFrame();in.anim_rate=a.morf.getPlaySpeed();in.e_oc2=a.atp!=1;in.atp=a.atp;in.link_ok=true;in.pass.valid=true;in.pass.proc=player.mProcID;in.now=in.pass;in.fresh=guard_arm.fresh(slot,a.id,c.generation,g.p8.tick);
 for(unsigned n=0;n<2;++n){in.at_hit[n]=a.mSphs_at[n].hit;in.at_on_link[n]=a.mSphs_at[n].GetAtHitObj()==&player.mTgCyls[0];in.at_shield[n]=a.mSphs_at[n].shield;}
 c.pv=guard_judge.judge(in);if(controller==3)fix02_guard_contact();
 const bool entry=a.mActionMode==4&&a.mOcState==0;if(entry)++result.starts;
 const float preframe=a.morf.getFrame();const int prestate=a.mOcState;
 if(a.mActionMode==4){if(controller>=1)on_attack_pre_cp04(nullptr,args,nullptr,nullptr);if(controller>=2)on_attack_pre_cp05(nullptr,args,nullptr,nullptr);a.executeAttack();}else if(a.mActionMode==3)a.executeFind();
 if(prestate<=2&&a.mActionMode==4&&a.mOcState==3&&preframe<14)++result.early;
 const bool at=a.mSphs_at[0].on||a.mSphs_at[1].on;if(at&&!result.prior_at)++result.windows;result.prior_at=at;
 guard_arm.note_post(slot,a.id,c.generation,g.p8.tick,at);
 native_update(a.morf);if(!shared)collision_world.reset();if(controller==3)on_cc_pre_fix01(nullptr,args,nullptr,nullptr);a.cc_set();
 if(controller==3){result.mask|=fx.cc_added;on_cc_post_fix01(nullptr,args,nullptr,nullptr);ck(!fx.cc_added,"paired lease never survives cc_set");}
 ck((a.mSphs_at[0].on||a.mSphs_at[1].on)==at,"temporary registration does not arm the collision pass");
 const bool had=a.mSphs_at[0].hit||a.mSphs_at[1].hit;if(!shared){collision_world.ClrAtHitInf();if(had&&!a.mSphs_at[0].hit&&!a.mSphs_at[1].hit)++result.cleared;}
 if(repeated&&at&&a.morf.getFrame()>22&&a.mActionMode==4&&a.mOcState<=2){a.mSphs_at[0].hit=a.mSphs_at[1].hit=true;a.mSphs_at[0].shield=a.mSphs_at[1].shield=true;}
 if(controller>=2)on_eoc_early_post_cp05(nullptr,args,nullptr,nullptr);
 if(a.atp==1){devf_blue::AtkView v;v.mode=a.mActionMode;v.state=a.mOcState;v.anim_b=a.checkBck(5);v.anim_c=a.checkBck(6);v.frame=a.morf.getFrame();v.play_speed=a.morf.getPlaySpeed();if(devf_blue::decide_rate(v,level,false)==devf_blue::RateAction::Apply)a.morf.setPlaySpeed(devf_blue::rate_for(level));int value=devf_blue::lockout_write(blue_pre,a.mActionMode,a.field_0x6c2,level);if(value>=0)a.field_0x6c2=short(value);}else expansion_post(a,p);
 if(controller>=1)on_eoc_post_cp04(nullptr,args,nullptr,nullptr);if(controller>=2)on_eoc_post_cp05(nullptr,args,nullptr,nullptr);
}
void boundary_control(int variant,int controller,bool release,unsigned level=1,unsigned mask=3,bool repeated=false){
 reset();player_distance=225;fx={};f2={};guard_arm=post08::ArmTrack{};guard_judge=post08::Parry{};guard_judge.enabled=true;daE_OC_c a;a.id=110+unsigned(variant);a.blue=variant==0;a.atp=variant==0?1:variant==1?2:variant==2?3:6;a.mName=a.blue?"E_OC":"E_OC2";post03::Events events;const auto admit_kind=a.blue?post03::Admission::E_OC_CANDIDATE:post03::Admission::E_OC2_CANDIDATE;
 const auto ob=g.eng.obs.ledger.observe(a.id,reinterpret_cast<std::uintptr_t>(&a),admit_kind,0,1,events);g.pe.px.sync(g.eng.obs.ledger);if(a.atp>=3)lg.posture.variants.observe(ob.slot,a.id,ob.generation,true,a.atp);
 if(a.atp==6)policy_ports[ob.slot].fury=5000;
 a.setBck(5,0,0,1);a.mOcState=1;a.morf.mFrameCtrl.frame=23;a.mSphs_at[0].hit=a.mSphs_at[0].shield=(mask&1)!=0;a.mSphs_at[1].hit=a.mSphs_at[1].shield=(mask&2)!=0;
 guard_arm.note_post(ob.slot,a.id,ob.generation,g.p8.tick,true);GuardRun result;
 for(unsigned t=0;t<700;++t)guard_tick(a,controller,!release||t<300,result,level,false,false,repeated);
 std::printf("FIX02_GUARD_AB variant=%d level=%u controller=%d release_R=%u starts=%u actual_AT_windows=%u early_rebounds=%u engine_clears=%u leased_mask=%u native_game_executed=false\n",variant,level,controller,unsigned(release),result.starts,result.windows,result.early,result.cleared,result.mask);
 if(controller<3&&!release){ck(result.early>=3,"old native/CP04/FIX01 reproduces guard-induced early recoil loop");ck(result.windows==0,"old residual contact prevents every subsequent native window");}
 else{ck(result.windows>=1,"release R or genuine FIX02 engine cleanup restores attack progression");if(controller==3){ck(result.early==0,"FIX02 removes stale early-recoil cause without force attack");ck(result.cleared>=1&&result.mask==mask,"engine cleared both registered stale attack objects");}}
 ck(guard_judge.c.confirmed==0,"normal guard cleanup never creates a parry");ck(a.health==220,"guard cleanup never changes enemy HP");
}

void simultaneous(){
 reset();player_distance=225;fx={};f2={};guard_arm.reset();guard_judge=post08::Parry{};guard_judge.enabled=true;
 daE_OC_c actors[8];GuardRun runs[8];
 for(unsigned i=0;i<8;++i){auto& a=actors[i];a.id=300+i;unsigned v=i%4;a.blue=v==0;a.atp=v==0?1:v==1?2:v==2?3:6;a.mName=a.blue?"E_OC":"E_OC2";post03::Events ev;auto o=g.eng.obs.ledger.observe(a.id,reinterpret_cast<std::uintptr_t>(&a),a.blue?Admission::E_OC_CANDIDATE:Admission::E_OC2_CANDIDATE,0,1,ev);g.pe.px.sync(g.eng.obs.ledger);if(!a.blue)lg.posture.variants.observe(o.slot,a.id,o.generation,true,a.atp);if(v==3)policy_ports[o.slot].fury=5000;a.setBck(5,0,0,1);a.mOcState=1;a.morf.mFrameCtrl.frame=23;for(auto& sphere:a.mSphs_at)sphere.hit=sphere.shield=true;guard_arm.note_post(o.slot,a.id,o.generation,g.p8.tick,true);}
 for(unsigned tick=0;tick<500;++tick){++g.p8.tick;collision_world.reset();for(unsigned i=0;i<8;++i)guard_tick(actors[i],3,true,runs[i],1,true,true);collision_world.ClrAtHitInf();for(auto& a:actors)ck(a.health==220,"multi-actor shared collision pass never changes HP");}
 for(auto& r:runs)ck(r.windows>0&&r.early==0&&r.mask==3,"simultaneous variants retain independent cleanup, no stale-loop inheritance");
 ck(fx.cleanup.c.failed==0,"all eight paired native leases restore");std::printf("FIX02_GUARD_MULTI actors=8 native_ticks=500 own_tokens=true source_native_collision_lists=true\n");
}
int main(){for(int v=0;v<4;++v)for(int c=0;c<4;++c)for(bool release:{false,true})boundary_control(v,c,release);for(unsigned level:{2u,3u})for(int c:{1,2,3})boundary_control(0,c,false,level);for(unsigned mask:{1u,2u})boundary_control(1,3,false,1,mask);for(int v=0;v<4;++v)boundary_control(v,3,false,1,3,true);simultaneous();std::printf("CP05_FIX02_GUARD checks=%llu failures=0 RESULT=LAB_PASS native_game_executed=false visual=HUMAN_RUNTIME_PENDING\n",checks);}
