int daAlink_c::procGuardAttackInit(){mProcID=PROC_GUARD_ATTACK;actual_clip=true;mUnderFrameCtrl[0].frame=1;mUnderFrameCtrl[0].rate=1;field_0x3478=1;field_0x347c=12;
 auto* self=this;void* args[]={&self};on_guard_init_post_cp04(nullptr,args,nullptr,nullptr);return 1;}
static unsigned level=1;static bool expansion=false,load_reverse=false,foreign_speed=false;
// A stable descending priority dispatcher, matching HookService in2.0.3.
void dispatch_post(void* args,const devf_blue::PreSnap& pre,daE_OC_c& a){
 auto expansion_post=[&]{if(!expansion)return;
  devf_blue::AtkView v;v.mode=a.mActionMode;v.state=a.mOcState;v.anim_b=a.checkBck(5);v.anim_c=a.checkBck(6);v.frame=a.mpMorf->getFrame();v.play_speed=a.mpMorf->getPlaySpeed();
  if(devf_blue::decide_rate(v,level,false)==devf_blue::RateAction::Apply)a.mpMorf->setPlaySpeed(devf_blue::rate_for(level));
  int lock=devf_blue::lockout_write(pre,a.mActionMode,a.field_0x6c2,level);if(lock>=0)a.field_0x6c2=short(lock);
 };
 struct Callback{int priority;int order;bool ours;};Callback cb[2]={{registration_priorities[1],0,true},{0,1,false}};
 if(load_reverse)std::swap(cb[0],cb[1]);
 std::stable_sort(cb,cb+2,[](auto x,auto y){return x.priority>y.priority;});
 for(auto f:cb)if(f.ours)on_eoc_post_cp04(nullptr,args,nullptr,nullptr);else expansion_post();
}
void native_update(Morf& m){
 m.prev=m.mFrameCtrl.frame;m.mFrameCtrl.frame+=m.mFrameCtrl.rate;
 if(m.mFrameCtrl.frame<0){m.mFrameCtrl.frame=0;m.mFrameCtrl.rate=0;m.stopped=true;}
 if(m.mFrameCtrl.frame>=m.mFrameCtrl.end){m.mFrameCtrl.frame=m.mFrameCtrl.end-.001f;m.mFrameCtrl.rate=0;m.stopped=true;}
}
void native_tick(daE_OC_c& a,int priority_mode=-1){
 ++g.p8.tick;auto* ptr=&a;void* args[]={&ptr};
 auto pre=devf_blue::make_snap(level==1,true,300,a.mActionMode,a.mOcState,a.mOldActionMode,a.field_0x6c0,a.field_0x6c2);
 on_eoc_pre_cp04(nullptr,args,nullptr,nullptr);
 // Native execute decrements timers; action clears AtSet before dispatch,
 // and damage/water/fall keep their priority over a requested attack.
 if(a.field_0x6c2)--a.field_0x6c2;if(a.field_0x6c0)--a.field_0x6c0;
 a.mSphs_at[0].on=a.mSphs_at[1].on=false;
 if(priority_mode>=0)a.setActionMode(priority_mode,0);
 if(a.mActionMode==4){on_attack_pre_cp04(nullptr,args,nullptr,nullptr);a.executeAttack();}
 else if(a.mActionMode==3){if(a.mOcState==0){a.field_0x6c2=20;a.field_0x6c0=30;a.speedF=0;a.mOcState=4;}
  // Find approaches at native speed and only selects attack on timer0.
  else if(a.field_0x6c2==0){if(player_distance>200&&player_distance<400)a.setActionMode(4,0);}
  else cLib_chaseF(&a.speedF,player_distance>200&&player_distance<400?0.f:player_distance<300?-3.f:20.f,1.f);
 }
 native_update(a.morf);
 if(foreign_speed)a.morf.setPlaySpeed(1.7f);
 dispatch_post(args,pre,a);
}
void reset(){
 c4=Cp04Runtime{};c4.enabled=true;g.pe=post04::PostureEngine{};g.p8.tick=1;g.rx.off=false;g.layout=1;
 player=daAlink_c{};player.mShieldModel=&player;player.id=999;player.profname=fpcNm_ALINK_e;
 lf.enabled=true;lf.def=lnkfx::Defense{};lg.posture=lnk01::Posture{};lb.m=lnk05::Machine{};
 registration_count=0;ck(cp04_install(),"complete actual hook installation");ck(registration_count==6,"six additional callbacks, two targets");
 player_distance=300;pause=event=false;window=0;life=40;layer=0;stage="LAB";foreign_speed=false;player_guard=false;hit_actor.profname=fpcNm_ALINK_e;
}
int admit(daE_OC_c& a){post03::Events ev;auto o=g.eng.obs.ledger.observe(a.id,reinterpret_cast<std::uintptr_t>(&a),Admission::E_OC_CANDIDATE,0,1,ev);g.pe.px.sync(g.eng.obs.ledger);return o.slot;}
int prepare(daE_OC_c& a,bool select){
 // Deterministic seeds exercise both REAL branches without overriding RNG.
 for(unsigned id=1;id<1000;++id){reset();a=daE_OC_c{};a.mpMorf=&a.morf;a.id=id;int slot=admit(a);auto* s=cp04_actor(&a);ck(s!=nullptr,"verified native Blue");auto copy=*s;cp04::Counters c;
  if(copy.decide(g.p8.tick+1,true,c)==select)return slot;
 }ck(false,"seed search");return -1;
}
void four_swings(){
 for(bool exp:{false,true})for(bool reverse:{false,true})for(unsigned group=1;group<=3;++group)for(bool clip_c:{false,true}){
  daE_OC_c a;int slot=prepare(a,true);expansion=exp;load_reverse=reverse;level=group;variant_c=clip_c;
  std::uint64_t old_token=0;unsigned seen=0,at_begins=0;bool last_at=false;std::uint64_t start[4]{},finish[4]{},at_tick[4]{};
  for(unsigned t=0;t<600&&c4.c.completed==0;++t){auto old_phase=c4.slots[slot].phase;unsigned old_swing=c4.slots[slot].swing;native_tick(a);auto& s=c4.slots[slot];
   if(s.token!=old_token){ck(s.token==old_token+1,"unique genuine B/C episode");old_token=s.token;++seen;start[seen-1]=s.local_tick;
    ck(a.mOcState==1||a.mOcState==2,"native attack state");ck(a.morf.clip==(clip_c?6:5),"native RNG B/C retained");ck(a.morf.getPlaySpeed()==(seen<=2?1.f:1.5f),"combo exact rates independent of group/load order");}
   if(old_phase==cp04::Phase::Swing&&s.phase!=cp04::Phase::Swing)finish[old_swing-1]=s.local_tick;
   bool at=a.mSphs_at[0].on||a.mSphs_at[1].on;if(at&&!last_at){at_tick[at_begins]=s.local_tick;++at_begins;}last_at=at;
   ck(c4.c.opportunities==1,"no intermediate reroll");ck(c4.c.foreign==0,"no speed ownership race");
  }
  ck(c4.c.completed==1&&seen==4&&at_begins==4,"four native armed windows, never fifth");
  ck(start[1]-finish[0]==12&&start[2]-finish[1]==12&&start[3]-finish[2]==2,"actual production gap schedule");
  ck(at_tick[1]-at_tick[0]>at_tick[3]-at_tick[2]&&at_tick[2]-at_tick[1]>at_tick[3]-at_tick[2],"actual native AtSet timestamps show moderate/moderate/short cadence");
  ck(a.mSound.calls[0]==4,"four native start voices");ck(a.mSound.calls[1]==4&&a.mSound.calls[2]==4,"native B/C crossing cues preserved at1.5");
  ck(clip_c||a.mSound.calls[3]==4,"native B weapon-hit SFX not skipped");
  ck(a.morf.getPlaySpeed()==1.f,"native idle restores own rate");
  native_tick(a);ck(a.field_0x6c2==short(exp?devf_blue::lockout_for(level):20),"normal post-combo recovery20/17/15");
  // Native root displacement remains bounded and forward, with no mod writes.
  ck(a.current.pos.z>0&&a.current.pos.z<=448.1f,"actual native root translation");
 }
}
void normals_and_priorities(){
 for(unsigned group=1;group<=3;++group){daE_OC_c a;prepare(a,false);expansion=true;level=group;native_tick(a);
  ck(c4.c.normal==1&&c4.c.selected==0,"normal branch preserved");ck(a.morf.getPlaySpeed()==devf_blue::rate_for(group),"normal native Expansion group rate unchanged");
 }
 for(int mode:{5,6,7,8,9,10,14,16}){daE_OC_c a;auto slot=prepare(a,true);native_tick(a);ck(c4.slots[slot].active(),"combo start");native_tick(a,mode);ck(!c4.slots[slot].active(),"native damage/death/recruit/fall/leash priority cancels");ck(a.mActionMode==mode,"never restore native attack after priority");}
 for(unsigned i=1;i<=4;++i){daE_OC_c a;auto slot=prepare(a,true);expansion=true;level=3;
  for(unsigned t=0;t<600&&c4.slots[slot].swing<i;++t)native_tick(a);ck(c4.slots[slot].swing==i,"break at each swing");
  const auto starts=c4.c.starts[0]+c4.c.starts[1]+c4.c.starts[2]+c4.c.starts[3];
  auto& e=const_cast<post04::Entry&>(g.pe.px.entry(unsigned(slot)));e.latched=true;e.posture=100;
  native_tick(a);ck(!c4.slots[slot].active(),"posture100 priority");ck(c4.c.starts[0]+c4.c.starts[1]+c4.c.starts[2]+c4.c.starts[3]==starts,"no extra swing after100");
 }
 for(int test=0;test<7;++test){daE_OC_c a;auto slot=prepare(a,true);native_tick(a);
  if(test==0)pause=true;if(test==1)event=true;if(test==2)a.field_0x6e3=1;if(test==3)a.room=1;if(test==4)layer=1;if(test==5)a.health=1;if(test==6)stage="OTHER";
  native_tick(a);ck(!c4.slots[slot].active(),"pause/context/lethal cancellation");
 }
 {daE_OC_c a;prepare(a,true);foreign_speed=true;native_tick(a);ck(c4.c.starts[0]==0&&c4.c.foreign==1,"foreign-rate refusal, no overwrite");ck(a.morf.getPlaySpeed()==1.7f,"foreign owner retained");}
 {daE_OC_c a;auto slot=prepare(a,true);native_tick(a);auto rng=c4.slots[slot].rng,clock=c4.slots[slot].local_tick;
  g.p8.tick+=100;ck(c4.slots[slot].rng==rng&&c4.slots[slot].local_tick==clock,"culling/no-execute freezes local clocks");}
}


void native_gap_windup(){
 for(float distance:{150.f,199.f,201.f,300.f,399.f,401.f,650.f}){
  daE_OC_c a;auto slot=prepare(a,true);player_distance=distance;expansion=true;level=3;
  for(unsigned t=0;t<600&&!c4.c.completed;++t)native_tick(a);
  ck(c4.c.completed==1&&c4.c.starts[3]==1,"miss/distance change retains full combo through real native chaseF braking");
  ck(c4.c.opportunities==1,"no reroll in native gap windup");
 }
}
void dense_and_capacity(){
 reset();daE_OC_c actors[65];
 for(unsigned i=0;i<64;++i){actors[i].id=i+1;ck(admit(actors[i])>=0,"64 modeled actors admitted");}
 actors[64].id=65;ck(admit(actors[64])<0&&cp04_actor(&actors[64])==nullptr,"capacity65 fails closed");
 for(unsigned i=0;i<64;++i){auto* a=&actors[i];void* args[]={&a};on_eoc_pre_cp04(nullptr,args,nullptr,nullptr);on_attack_pre_cp04(nullptr,args,nullptr,nullptr);a->executeAttack();native_update(a->morf);on_eoc_post_cp04(nullptr,args,nullptr,nullptr);}
 ck(c4.c.opportunities==64&&c4.c.selected+c4.c.normal==64,"one independent opportunity per actual actor");
 for(unsigned i=0;i<64;++i){ck(c4.slots[i].key.id==i+1,"no cross-actor inheritance");for(unsigned j=0;j<i;++j)ck(c4.slots[i].seed!=c4.slots[j].seed,"distinct identity streams");}
 // A presentation-only frame must never advance a native execution clock.
 auto clock=c4.slots[0].local_tick;for(unsigned frames=0;frames<120;++frames)ck(c4.slots[0].local_tick==clock,"30/60/120 presentation-only frames do not simulate enemies");
}
void shield_input(bool trigger){auto* link=&player;void* args[]={&link};player.mItemButton=daAlink_c::BTN_R;player.mItemTrigger=trigger?daAlink_c::BTN_B:0;on_stick_post_cp04(nullptr,args,nullptr,nullptr);}
void shield_physical(){
 reset();shield_input(true);player.procGuardAttackInit();ck(c4.shield.c.native_starts==1,"native Shield Attack provenance receipt");
 cp04_observe_motion(&player);ck(!cp04_guard_pass(),"input/clip init alone never parry");
 ++g.p8.tick;player.mUnderFrameCtrl[0].frame=2;cp04_observe_motion(&player);ck(cp04_guard_pass(),"actual native clip progressed");
 post08::Input in;post08::Verdict v;in.pass.frame=2;v.first_contact=true;cp04_contact(in,v,true,0,40);
 ck(c4.shield.c.matched==1&&!cp04_guard_pass(),"one Shield Attack consumes exactly once");
 // Native shield-slip response is retained, then allowed to finish braking
 // and body interpolation. Two distinct inputs must start two distinct clips.
 player.mProcID=daAlink_c::PROC_GUARD_SLIP;player.actual_clip=false;
 for(unsigned t=0;t<12;++t){++g.p8.tick;shield_input(t==10);auto* link=&player;void* args[]={&link};on_link_damage_post_cp04(nullptr,args,nullptr,nullptr);}
 ck(c4.shield.c.rearms==1&&c4.shield.c.native_starts==2,"buffer produces a second genuine native init");
 ck(player.mProcID==daAlink_c::PROC_GUARD_ATTACK&&player.actual_clip,"physical shield clip restarted");
 cp04_observe_motion(&player);ck(!cp04_guard_pass(),"second contact before actual shield advance refused");
 ++g.p8.tick;player.mUnderFrameCtrl[0].frame=2;cp04_observe_motion(&player);ck(cp04_guard_pass(),"second clip independent motion");cp04_contact(in,v,true,40,80);ck(c4.shield.c.matched==2,"two distinct parries");
 for(int state=0;state<7;++state){reset();shield_input(true);player.procGuardAttackInit();++g.p8.tick;player.mUnderFrameCtrl[0].frame=2;cp04_observe_motion(&player);cp04_contact(in,v,true,0,40);g.p8.tick+=12;
  if(state==0)player.mDamageTimer=1;if(state==1)player.wolf=true;if(state==2)life=0;if(state==3)event=true;if(state==4)player.mProcID=0x2d;if(state==5)player.draw=false;if(state==6)player.mModeFlg=0x70c52;
  shield_input(true);auto* link=&player;void* args[]={&link};on_link_damage_post_cp04(nullptr,args,nullptr,nullptr);ck(c4.shield.c.rearms==0,"damage/wolf/death/event/Helm/no shield/mount veto");}
}

void recoil_and_posture(){
 for(int mask:{0,1,2,4,8,3,6,12,7})for(int initial:{0,25,60,80,99}){
  daE_OC_c a;auto slot=prepare(a,true);expansion=true;level=3;variant_c=bool(mask&1);
  auto& entry=const_cast<post04::Entry&>(g.pe.px.entry(unsigned(slot)));entry.posture=static_cast<unsigned char>(initial);
  post08::Parry parry;parry.enabled=true;unsigned last_swing=0,contacts=0,parries=0;bool broke=false;
  for(unsigned t=0;t<600&&!c4.c.completed&&!broke;++t){
   native_tick(a);auto& s=c4.slots[slot];
   if(s.phase!=cp04::Phase::Swing)continue;
   post08::Input in;in.tracked=true;in.slot=slot;in.id=s.key.id;in.generation=s.key.generation;in.hp=a.health;in.mode=a.mActionMode;in.state=a.mOcState;
   in.anim_ok=true;in.anim_frame=a.morf.getFrame();in.anim_rate=a.morf.getPlaySpeed();in.link_ok=true;in.fresh=a.mSphs_at[0].on;
   in.latched=entry.latched;
   if(s.swing==last_swing||!in.fresh){(void)parry.judge(in);continue;}
   last_swing=s.swing;++contacts;
   bool do_parry=(mask&(1<<(s.swing-1)))!=0;
   // Injected collider contact is admitted ONLY in an actual native AtSet
   // window. Broadphase/pose are modeled; this is never native runtime proof.
   in.at_hit[0]=in.at_hit[1]=true;in.at_on_link[0]=in.at_on_link[1]=true;in.at_shield[0]=in.at_shield[1]=do_parry;
   if(do_parry){
    shield_input(true);player.procGuardAttackInit();++g.p8.tick;player.mUnderFrameCtrl[0].frame=2;cp04_observe_motion(&player);
    in.pass.valid=true;in.pass.guard_attack=cp04_guard_pass();in.pass.frame=2;in.pass.start=1;in.pass.end=12;in.pass.serial=c4.shield.native_serial;in.pass.age_ticks=1;
   }else{in.pass.valid=true;in.pass.guard_attack=false;}
   auto v=parry.judge(in);
   if(do_parry){
    ck(v.result==post08::Result::Confirmed,"real legacy collision/episode/window classifier");
    auto profile=postbal01::capture(g.eng.obs.ledger,lg.posture.variants,slot,in.id,in.generation,read_aux(&a));
    auto out=g.pe.px.apply_parry(slot,in.generation,in.id,v.token,g.eng.obs.ledger,profile);
    ck(out.applied&&out.gain==40,"unchanged actual Blue parry gain40");
    ck(out.after==std::min(100,out.before+40),"actual posture threshold and saturation");
    parry.commit(slot,in.generation,in.id,v.token,true);cp04_contact(in,v,true,out.before,out.after);++parries;
    ck(parry.judge(in).result==post08::Result::Duplicate,"two spheres one parry token");
    if(out.crossed){broke=true;ck(!s.active(),"synchronous100 cancellation before another native attack");ck(c4.c.starts[3]==0||s.swing==4,"no fourth if third already broke");break;}
   }else cp04_contact(in,v,false,-1,-1);
   // Ordinary shield block/recoil must NOT cancel the planner. Execute the
   // ORIGINAL native state3 reverse animation and let it reach its real end.
   player_guard=true;a.mSphs_at[0].hit=true;native_tick(a);a.mSphs_at[0].hit=a.mSphs_at[1].hit=false;player_guard=false;
   ck(a.mOcState==3&&a.morf.getPlaySpeed()==-1,"native shield recoil actually executed");
   ck(s.active(),"sub100 parry or ordinary block preserves combo commitment");
  }
  if(!broke)ck(c4.c.completed==1&&contacts==4,"full four-swing physical sequence after recoil");
  if(mask==7&&initial==0)ck(broke&&parries==3&&entry.posture==100&&c4.c.starts[3]==0,"40->80->100 breaks at3, no4");
  if(mask==12&&initial==0)ck(!broke&&parries==2&&entry.posture==80,"only3 and4 individually parried");
 }
 // Waiting for native chaseF to reach zero never rerolls per frame.
 {daE_OC_c a;prepare(a,true);a.speedF=6;for(int i=0;i<5;++i){native_tick(a);ck(c4.c.opportunities==0,"no waiting-frame chance");}native_tick(a);ck(c4.c.opportunities==1,"one native opportunity once speed reaches0");}
}
int main(){four_swings();normals_and_priorities();shield_physical();recoil_and_posture();native_gap_windup();dense_and_capacity();std::printf("CP04_ADAPTER checks=%llu failures=0 RESULT=LAB_PASS complete_production_callbacks=true unchanged_native_executeAttack=true actual_Expansion_policy=true native_game_executed=false visual=VISUAL_SHIELD_MOTION_HUMAN_PENDING\n",checks);}
