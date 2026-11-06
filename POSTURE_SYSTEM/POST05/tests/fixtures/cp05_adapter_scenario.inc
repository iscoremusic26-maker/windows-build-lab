#include "ROOT/tests/cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/red_ai_policy.hpp"
#include "ROOT/tests/cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/black_ai_policy.hpp"
#include "ROOT/tests/cp21/DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP10_6_DYNAMIC_BLUE_BLACK/expansion/dynamic_policy.hpp"
int daAlink_c::procGuardAttackInit(){mProcID=PROC_GUARD_ATTACK;actual_clip=true;mUnderFrameCtrl[0].frame=1;mUnderFrameCtrl[0].rate=1;field_0x3478=1;field_0x347c=12;auto* self=this;void* args[]={&self};on_guard_init_post_cp04(nullptr,args,nullptr,nullptr);return 1;}
struct PolicyPort{devf_red_ai::State red;devf_black_ai::State black;devf_black_ai::Rooms rooms;unsigned fury=0;bool random=false;int pre_mode=-1,pre_state=-1,pre_timer=0;devf_red_ai::Link link;};
static PolicyPort policy_ports[64];static bool expansion=true,reverse_registration=false;static float foreign_rate=0;static bool foreign_timer=false;static unsigned lease_readbacks=0;
void reset(){
 c4=Cp04Runtime{};c5=Cp05Runtime{};g.pe=post04::PostureEngine{};g.p8.tick=1;g.layout=1;g.rx.off=false;
 player=daAlink_c{};player.mShieldModel=&player;player.id=999;player.profname=fpcNm_ALINK_e;
 lf.enabled=true;lf.def=lnkfx::Defense{};lf.sample_ok=false;lf.r_held=lf.b_trig=lf.a_trig=false;lf.key=0;lf.sample_tick=0;lf.bash_offered_now=lf.bash_offered_prev=lf.bash_pending=lf.b_consumed=false;lg.posture=lnk01::Posture{};lb.m=lnk05::Machine{};
 registration_count=0;ck(cp04_install(),"frozen CP04 install");ck(cp05_install(),"complete CP05 install");ck(registration_count==10,"four extra callbacks no new target");
 ck(registration_priorities[6]==-200&&registration_priorities[7]==-200&&registration_priorities[8]==100&&registration_priorities[9]==-200,"documented PRE late POST early/late priorities");
 foreign_rate=0;foreign_timer=false;lease_readbacks=0;for(auto& p:policy_ports)p=PolicyPort{};pause=event=false;window=0;life=40;layer=0;stage="LAB";player_guard=false;player_distance=300;hit_actor.profname=fpcNm_ALINK_e;
}
int admit(daE_OC_c& a,bool black){a.blue=false;a.mName="E_OC2";a.atp=black?3:2;post03::Events ev;auto o=g.eng.obs.ledger.observe(a.id,reinterpret_cast<std::uintptr_t>(&a),Admission::E_OC2_CANDIDATE,0,1,ev);g.pe.px.sync(g.eng.obs.ledger);return o.slot;}
int slot_of(daE_OC_c& a){return cp04_slot(&a);}
devf_red_ai::Actor red_frame(daE_OC_c& a){
 devf_red_ai::Actor f;int slot=slot_of(a);f.id=a.id;f.gen=unsigned(g.eng.obs.ledger.slot(slot).generation);f.variant=devf_variant::Variant::Red;std::snprintf(f.stage,9,"%s",stage);f.room=a.room;f.layer=layer;f.mode=a.mActionMode;f.state=a.mOcState;f.health=a.health;f.pos={a.current.pos.x,a.current.pos.y,a.current.pos.z};f.range=5000;f.ready=true;f.perceived=true;f.speed=a.speedF;f.lockout=a.field_0x6c2;f.frame=a.mpMorf->getFrame();f.rate=a.mpMorf->getPlaySpeed();f.end=a.mpMorf->mFrameCtrl.end;f.b=a.checkBck(5);f.c=a.checkBck(6);f.stopped=a.mpMorf->isStop();f.at_set=a.mSphs_at[0].on||a.mSphs_at[1].on;f.at_hit=a.mSphs_at[0].hit||a.mSphs_at[1].hit;f.hit_link=f.at_hit&&hit_actor.profname==fpcNm_ALINK_e;return f;
}
devf_black_ai::Actor black_frame(daE_OC_c& a){auto rf=red_frame(a);devf_black_ai::Actor f;f.id=rf.id;f.gen=rf.gen;f.stage=stage_token(stage);f.variant=devf_variant::Variant::Black;f.room=rf.room;f.mode=rf.mode;f.state=rf.state;f.health=rf.health;f.pos={rf.pos.x,rf.pos.y,rf.pos.z};f.range=5000;f.speed=rf.speed;f.lockout=rf.lockout;f.timer=a.field_0x6c0;f.leash=a.field_0x6e3;f.b=rf.b;f.c=rf.c;f.wait_bck=a.checkBck(28);f.frame=rf.frame;f.rate=rf.rate;f.end=rf.end;f.stopped=rf.stopped;f.at_set=rf.at_set;f.at_hit=rf.at_hit;f.hit_link=rf.hit_link;f.los=true;return f;}
devf_black_ai::Link black_link(){devf_black_ai::Link l;l.present=true;l.id=player.id;l.room=0;l.stage=stage_token(stage);l.pos={0,0,player_distance};return l;}
void expansion_pre(daE_OC_c& a,PolicyPort& p){
 if(!expansion)return;
 if(a.atp==2){auto f=red_frame(a);auto& s=p.red;if(!s.id){s.id=f.id;s.gen=f.gen;s.room=f.room;s.layer=f.layer;std::memcpy(s.stage,f.stage,9);devf_red_ai::seed(s,f.id,f.gen,1);if(!p.random)s.next_random=100000;}
  auto& l=p.link;l.present=l.basic=l.canary=l.safe=l.los=l.grounded=true;l.id=player.id;l.room=a.room;l.layer=layer;std::memcpy(l.stage,f.stage,9);l.pos={0,0,player_distance};if(l.motion==devf_red_ai::Motion::Other)l.motion=devf_red_ai::Motion::Offensive;
  auto cmd=devf_red_ai::pre(s,f,l,[](auto...){ });if(cmd.force)a.setActionMode(4,0);if(cmd.zero_speed)a.speedF=0;if(cmd.rush)a.speedF=cmd.speed;if(cmd.close){a.speedF=cmd.speed;if(cmd.close_lockout)a.field_0x6c2=2;}
 }else{auto f=black_frame(a);auto& s=p.black;if(!s.id){s.id=f.id;s.gen=f.gen;}auto cmd=devf_black_ai::pre(s,f,black_link(),p.rooms,p.fury,[](auto...){ });if(cmd.set_mode)a.setActionMode(cmd.mode,cmd.state);if(cmd.zero_speed)a.speedF=0;if(cmd.timer)a.field_0x6c0=cmd.timer_value;if(cmd.lockout)a.field_0x6c2=cmd.lockout_value;if(cmd.close)a.speedF=cmd.speed;}
}
void expansion_post(daE_OC_c& a,PolicyPort& p){
 if(!expansion)return;
 if(a.atp==2){auto f=red_frame(a);auto rp=devf_red_ai::rush_post(p.red,f,[](auto...){ });if(rp.write)a.speedF=rp.value;auto cp=devf_red_ai::close_after(p.red,f,[](auto...){ });if(cp.speed)a.speedF=cp.speed_value;if(cp.lockout)a.field_0x6c2=0;auto out=devf_red_ai::post(p.red,f,[](auto...){ });if(out.write)a.morf.setPlaySpeed(out.value);}
 else{auto f=black_frame(a);auto cp=devf_black_ai::close_after(p.black,f,[](auto...){ });if(cp.speed)a.speedF=cp.speed_value;if(cp.lockout)a.field_0x6c2=0;auto out=devf_black_ai::post(p.black,f,p.fury,[](auto...){ });if(out.write)a.morf.setPlaySpeed(out.value);}
}
void native_update(Morf& m){m.prev=m.mFrameCtrl.frame;m.mFrameCtrl.frame+=m.mFrameCtrl.rate;if(m.mFrameCtrl.frame<0){m.mFrameCtrl.frame=0;m.mFrameCtrl.rate=0;m.stopped=true;}if(m.mFrameCtrl.frame>=m.mFrameCtrl.end){m.mFrameCtrl.frame=m.mFrameCtrl.end-.001f;m.mFrameCtrl.rate=0;m.stopped=true;}}
// Stable priority dispatch covers BOTH initial registration orders. Expansion
// is real unmodified policy code; engine I/O and broadphase are modeled.
void native_tick(daE_OC_c& a,bool advance=true,int priority=-1){
 if(advance)++g.p8.tick;auto* self=&a;void* args[]={&self};int slot=slot_of(a);ck(slot>=0,"live ledger slot");auto& p=policy_ports[slot];
 struct Callback{int priority;int kind;};Callback pre[3]={{0,0},{registration_priorities[0],1},{registration_priorities[6],2}},post[4]={{registration_priorities[8],0},{0,1},{registration_priorities[1],2},{registration_priorities[9],3}};
 if(reverse_registration){std::reverse(pre,pre+3);std::reverse(post,post+4);}std::stable_sort(pre,pre+3,[](auto x,auto y){return x.priority>y.priority;});std::stable_sort(post,post+4,[](auto x,auto y){return x.priority>y.priority;});
 for(auto x:pre){if(x.kind==0){expansion_pre(a,p);p.pre_mode=a.mActionMode;p.pre_state=a.mOcState;p.pre_timer=a.field_0x6c2;}else if(x.kind==1)on_eoc_pre_cp04(nullptr,args,nullptr,nullptr);else on_eoc_pre_cp05(nullptr,args,nullptr,nullptr);}
 if(a.field_0x6ca)--a.field_0x6ca;if(a.field_0x6c0)--a.field_0x6c0;if(a.field_0x6c2)--a.field_0x6c2;a.mSphs_at[0].on=a.mSphs_at[1].on=false;
 if(priority>=0)a.setActionMode(priority,0);
 if(a.mActionMode==4){on_attack_pre_cp04(nullptr,args,nullptr,nullptr);on_attack_pre_cp05(nullptr,args,nullptr,nullptr);a.executeAttack();}else if(a.mActionMode==3)a.executeFind();
 native_update(a.morf);
 if(foreign_timer&&c5.slots[slot].lease)a.field_0x6c2=7;
 for(auto x:post){if(x.kind==0)on_eoc_early_post_cp05(nullptr,args,nullptr,nullptr);else if(x.kind==1){if(!foreign_timer&&c5.slots[slot].phase==cp05::Phase::Gap&&a.mActionMode==3&&a.mOcState==4&&c5.slots[slot].native_timer>=0){ck(!c5.slots[slot].lease,"lease restored BEFORE Expansion POST");if(p.pre_mode==3&&p.pre_state==4&&!event)ck(a.field_0x6c2==(p.pre_timer>0?p.pre_timer-1:0),"exact native timer decrement visible BEFORE Expansion POST");++lease_readbacks;}expansion_post(a,p);}else if(x.kind==2)on_eoc_post_cp04(nullptr,args,nullptr,nullptr);else{if(foreign_rate)a.morf.setPlaySpeed(foreign_rate);on_eoc_post_cp05(nullptr,args,nullptr,nullptr);}}
}
int prepare(daE_OC_c& a,cp05::Kind kind,bool black_normal=false){
 for(unsigned id=1;id<1000;++id){reset();a=daE_OC_c{};a.mpMorf=&a.morf;a.id=id;int slot=admit(a,kind==cp05::Kind::Black8||kind==cp05::Kind::Black7||black_normal);auto* s=cp05_actor(&a);ck(s,"certified E_OC2 profile");auto copy=*s;cp05::Counters c;if(copy.decide(2,true,c)==kind)return slot;}
 ck(false,"deterministic selection seed");return -1;
}
void all_patterns(){
 for(auto kind:{cp05::Kind::Red4,cp05::Kind::Black8,cp05::Kind::Black7})for(bool reverse:{false,true})for(int clip_mode:{0,1,2}){bool clip_c=clip_mode==1;
  daE_OC_c a;int slot=prepare(a,kind);reverse_registration=reverse;variant_c=clip_c;auto& s=c5.slots[slot];unsigned starts=0,windows=0;std::uint64_t old_token=0;bool last_at=false;std::uint64_t at[8]{};
  for(unsigned tick=0;tick<2400&&c5.c.kind[unsigned(kind)].completed==0;++tick){if(clip_mode==2)variant_c=(s.swing%2)==0;native_tick(a);
   if(s.token!=old_token){ck(s.token==old_token+1,"one fresh token each native B/C");old_token=s.token;++starts;ck(a.morf.getPlaySpeed()==cp05::rate(s.kind,s.swing),"one real fresh handoff");}
   bool now=a.mSphs_at[0].on||a.mSphs_at[1].on;if(now&&!last_at){ck(windows<8,"bounded native weapon windows");at[windows++]=s.local_tick;}last_at=now;
   ck(!s.lease,"no timer lease retained across callbacks");
  }
  ck(starts==cp05::count(kind),"exact native swing count");ck(windows==starts,"one actual native AT interval each swing");ck(c5.c.kind[unsigned(kind)].completed==1,"native complete and return");
  ck(c5.c.distribution[unsigned(s.variant)].opportunities==1,"no reroll between swings");
  for(unsigned i=0;i+1<starts;++i){auto delta=at[i+1]-at[i];ck(delta>=36,"physical cadence floor no7tick fiction");ck(delta>=cp05::target(kind,i)&&delta<=cp05::target(kind,i)+2,"AT deadlines measured from actual native openings");}
  std::printf("CP05_TIMING kind=%s clip=%c reverse=%d intervals=",cp05::name(kind),clip_mode==2?'M':clip_c?'C':'B',int(reverse));for(unsigned i=1;i<starts;++i)std::printf("%s%llu",i==1?"":",",(unsigned long long)(at[i]-at[i-1]));std::puts(" native_game_executed=false");
 }
}
void normals_and_exclusions(){
 for(bool black:{false,true})for(bool reverse:{false,true}){
  daE_OC_c a;auto slot=prepare(a,cp05::Kind::Normal,black);reverse_registration=reverse;native_tick(a);auto& q=c5.slots[slot];
  ck(a.morf.getPlaySpeed()==1.15f&&q.token==0&&q.phase==cp05::Phase::Idle,"native normal115 untouched");ck(c5.c.distribution[black?1:0].normal==1,"normal choice correct denominator");
 }
 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Red4);expansion=false;native_tick(a);ck(a.morf.getPlaySpeed()==1&&c5.c.distribution[0].opportunities==0&&!c5.slots[slot].active(),"without Expansion unsupported witness is fail closed");expansion=true;}
 {daE_OC_c a;prepare(a,cp05::Kind::Black8);foreign_rate=1.7f;native_tick(a);ck(c5.c.distribution[1].opportunities==0&&a.morf.getPlaySpeed()==1.7f,"foreign rate never overwritten or counted");}
 // Each source-defined nonNormal cause uses the authentic pending-frame2
 // branch. The test setup is a source-policy State, not a production API.
 for(unsigned cause=1;cause<unsigned(devf_red_ai::Cause::Count);++cause){
  daE_OC_c a;int slot=prepare(a,cp05::Kind::Red4);auto& r=policy_ports[slot].red;
  auto f=red_frame(a);r.id=f.id;r.gen=f.gen;r.link_id=player.id;r.room=f.room;r.layer=f.layer;std::memcpy(r.stage,f.stage,9);r.next_random=100000;r.intent=true;r.intent_cause=devf_red_ai::Cause(cause);r.intent_start=0;
  if(cause==unsigned(devf_red_ai::Cause::ChargeSpin))policy_ports[slot].link.charge=devf_red_ai::Charge::Spin;
  if(cause==unsigned(devf_red_ai::Cause::ChargeJump))policy_ports[slot].link.charge=devf_red_ai::Charge::Jump;
  if(cause==unsigned(devf_red_ai::Cause::Passive))policy_ports[slot].link.motion=devf_red_ai::Motion::Quiet;
  native_tick(a);ck(c5.c.distribution[0].opportunities==0&&a.morf.getPlaySpeed()==1,"special never converted to first combo swing");
  native_tick(a);ck(a.morf.getPlaySpeed()==2,"authentic special pending x2 atframe2 unchanged");
 }
 // Genuine RANDOM selection (unmodified source PRNG/cooldowns), not a cause
 // inferred from its eventual2.0 rate or a synthesized fake attack.
 bool found=false;
 for(unsigned id=1;id<100&&!found;++id){reset();daE_OC_c a;a.id=id;a.mpMorf=&a.morf;int slot=admit(a,false);policy_ports[slot].random=true;native_tick(a);
  if(policy_ports[slot].red.blow_cause==devf_red_ai::Cause::Random){found=true;ck(c5.c.distribution[0].opportunities==0,"actual Random opportunity excluded");auto deadline=policy_ports[slot].red.next_random;ck(deadline>=121&&deadline<=211,"native Random120..210 cooldown retained");native_tick(a);ck(a.morf.getPlaySpeed()==2,"actual Random x2 retained");}}
 ck(found,"authentic Random seed reached");
}
void priority_during_gap(){
 daE_OC_c a;int slot=prepare(a,cp05::Kind::Red4);variant_c=false;auto& s=c5.slots[slot];
 for(unsigned n=0;n<150&&s.phase!=cp05::Phase::Gap;++n)native_tick(a);ck(s.phase==cp05::Phase::Gap,"longRed gap reached");
 for(unsigned n=0;n<21;++n)native_tick(a);ck(a.field_0x6c2==0&&lease_readbacks>10&&s.phase==cp05::Phase::Gap,"Expansion observes original20tick lockout expiry inside compact segment gap");
 player_distance=230;policy_ports[slot].link.charge=devf_red_ai::Charge::Spin;native_tick(a);
 ck(!s.active()&&s.token==1&&policy_ports[slot].red.blow_cause==devf_red_ai::Cause::ChargeSpin,"charge priority interrupts long gap without being blocked");native_tick(a);ck(a.morf.getPlaySpeed()==2,"charge reaction retains2.0 handoff");
 {daE_OC_c b;int q=prepare(b,cp05::Kind::Red4);for(unsigned n=0;n<80&&c5.slots[q].phase!=cp05::Phase::Gap;++n)native_tick(b);native_tick(b);foreign_timer=true;native_tick(b);ck(!c5.slots[q].active()&&b.field_0x6c2==7,"foreign timer not restored to our baseline");}
}
void fury_and_safety(){
 for(auto kind:{cp05::Kind::Black8,cp05::Kind::Black7})for(unsigned index=1;index<=cp05::count(kind);++index){
  daE_OC_c a;int slot=prepare(a,kind);auto& s=c5.slots[slot];for(unsigned t=0;t<2500&&(s.swing<index||s.phase!=cp05::Phase::Swing);++t)native_tick(a);
  ck(s.swing==index,"fury boundary reached requested actual swing");auto token=s.token;auto rate=a.morf.getPlaySpeed();a.atp=6;policy_ports[slot].fury=300;native_tick(a);
  ck(!s.active()&&s.token==token&&a.morf.getPlaySpeed()==rate,"fury cancels future only, no retime current clip");
 }
 for(unsigned mode=0;mode<10;++mode){
  daE_OC_c a;int slot=prepare(a,cp05::Kind::Red4);native_tick(a);auto& s=c5.slots[slot];auto token=s.token;
  if(mode==0)pause=true;if(mode==1)event=true;if(mode==2)window=1;if(mode==3)life=0;if(mode==4)a.mHide=1;if(mode==5)a.field_0x6e3=1;if(mode==6)a.mSphs_cc[0].tg=true;if(mode==7)foreign_rate=1.7f;if(mode==8)g.p8.tick+=5;
  native_tick(a,true,(mode==9||mode==6)?5:-1);ck(!s.active()&&s.token==token,"priority/pause/hit/hidden/leash/foreign/culling cancels without new token");
 }
}
void dense_context_and_summary(){
 reset();daE_OC_c actors[65];for(unsigned i=0;i<65;++i){actors[i].id=i+1;actors[i].mpMorf=&actors[i].morf;auto slot=admit(actors[i],i%2);ck(i<64?slot==int(i):slot<0,"64slots capacity no stealing");}
 ++g.p8.tick;for(unsigned i=0;i<64;++i)native_tick(actors[i],false);ck(c5.c.distribution[0].opportunities+c5.c.distribution[1].opportunities==64,"multi actor64 independent opportunities");
 for(unsigned i=0;i<64;++i){ck(c5.slots[i].key.id==i+1,"actor identity isolated");for(unsigned j=0;j<i;++j)ck(c5.slots[i].seed!=c5.slots[j].seed,"PRNG streams distinct");}
 auto before=c5.c.distribution[0].opportunities;auto token=c5.slots[0].token;auto* self=&actors[0];void* args[]={&self};on_eoc_pre_cp05(nullptr,args,nullptr,nullptr);on_attack_pre_cp05(nullptr,args,nullptr,nullptr);on_eoc_early_post_cp05(nullptr,args,nullptr,nullptr);on_eoc_post_cp05(nullptr,args,nullptr,nullptr);ck(c5.c.distribution[0].opportunities==before&&c5.slots[0].token==token,"duplicate callbacks no reroll/token");
 auto clock=c5.slots[0].local_tick;for(unsigned n=0;n<60;++n)cp05_maintenance();ck(c5.slots[0].local_tick==clock,"presentation-only60frames don't simulate actors");
 c5.lines=800;auto count=cp05_summary_lines;g.p8.tick=1800;cp05_maintenance();ck(cp05_summary_lines>count&&c5.enabled,"periodic complete outside detailcap");count=cp05_summary_lines;for(unsigned n=0;n<60;++n)cp05_maintenance();ck(cp05_summary_lines==count,"periodic dedup same tick");cp05_summary();ck(cp05_summary_lines>count&&!c5.enabled&&c4.enabled,"shutdown summary complete C4 unchanged");
 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Red4);native_tick(a);auto& s=c5.slots[slot];++g.p8.tick;auto* ptr=&a;void* raw[]={&ptr};on_eoc_pre_cp05(nullptr,raw,nullptr,nullptr);on_eoc_pre_cp05(nullptr,raw,nullptr,nullptr);on_eoc_post_cp05(nullptr,raw,nullptr,nullptr);on_eoc_early_post_cp05(nullptr,raw,nullptr,nullptr);on_eoc_post_cp05(nullptr,raw,nullptr,nullptr);ck(!s.active()&&c5.c.reentered==1,"nested callback cancels safely");}
}
void shield_input(bool trigger){
 auto* link=&player;void* args[]={&link};player.mItemButton=daAlink_c::BTN_R;player.mItemTrigger=trigger?daAlink_c::BTN_B:0;
 struct C{int priority;bool cp04;};C c[2]={{0,false},{registration_priorities[3],true}};if(reverse_registration)std::swap(c[0],c[1]);std::stable_sort(c,c+2,[](auto a,auto b){return a.priority>b.priority;});
 for(auto x:c)if(x.cp04)on_stick_post_cp04(nullptr,args,nullptr,nullptr);else on_stick_post_fx(nullptr,args,nullptr,nullptr);
}
void parry_case(cp05::Kind kind,unsigned mask,int initial,bool regen,bool ordinary_block){
 daE_OC_c a;int slot=prepare(a,kind);auto& s=c5.slots[slot];auto& e=const_cast<post04::Entry&>(g.pe.px.entry(unsigned(slot)));e.posture=static_cast<unsigned char>(initial);
 ck(g.pe.px.set_regen_rate(slot,e.generation,e.id,10),"unchanged CP02 highHP band rate10");
 post08::Parry judge;judge.enabled=true;unsigned handled=0,inputs=0,confirmed=0,shield_index=0,break_index=0;bool pending_recoil=false;std::uint64_t previous_sa=0;
 for(unsigned t=0;t<3200&&c5.c.kind[unsigned(kind)].completed==0&&!e.latched;++t){
  if(regen)g.pe.on_update(true);native_tick(a);
  if(pending_recoil){ck(a.mOcState==3&&a.morf.getPlaySpeed()==-1,"complete ORIGINAL native shield recoil really started");a.mSphs_at[0].hit=a.mSphs_at[1].hit=false;player_guard=false;pending_recoil=false;}
  // This port models Link's update/input/pose I/O. Only actual production
  // observers, actual Parry policy and posture transaction can credit a parry.
  // It NEVER certifies visible motion, native broadphase or real Link physics.
  bool selected=s.phase==cp05::Phase::Swing&&s.swing&&((mask>>(s.swing-1))&1);
  bool started=false;
  if(selected&&shield_index!=s.swing&&s.local_tick>=s.start_tick+8){
   player.mProcID=3;player.actual_clip=false;lf.bash_offered_now=true;shield_input(true);player.procGuardAttackInit();shield_index=s.swing;++inputs;started=true;
   ck(c4.shield.native_serial>previous_sa,"each R+B starts a distinct VERIFIED native Shield Attack serial");previous_sa=c4.shield.native_serial;
   cp04_observe_motion(&player);ck(!cp04_guard_pass(),"init/input without positive clip advancement never parries");
  }
  if(!started&&player.mProcID==daAlink_c::PROC_GUARD_ATTACK){++player.mUnderFrameCtrl[0].frame;if(player.mUnderFrameCtrl[0].frame>=18){player.mProcID=3;player.actual_clip=false;}}
  cp04_observe_motion(&player);
  post08::Input in;in.tracked=true;in.slot=slot;in.id=s.key.id;in.generation=s.key.generation;in.hp=a.health;in.mode=a.mActionMode;in.state=a.mOcState;in.anim_ok=true;in.anim_frame=a.morf.getFrame();in.anim_rate=a.morf.getPlaySpeed();in.e_oc2=true;in.atp=a.atp;in.link_ok=true;in.latched=e.latched;in.fresh=a.mSphs_at[0].on;
  if(s.phase!=cp05::Phase::Swing||!in.fresh||s.swing==handled){(void)judge.judge(in);continue;}
  if(!selected&&!ordinary_block){(void)judge.judge(in);continue;}
  handled=s.swing;in.at_hit[0]=in.at_hit[1]=in.at_on_link[0]=in.at_on_link[1]=in.at_shield[0]=in.at_shield[1]=true;
  in.pass.valid=true;in.pass.guard_attack=selected&&cp04_guard_pass();in.pass.frame=player.mUnderFrameCtrl[0].frame;in.pass.start=1;in.pass.end=12;in.pass.serial=c4.shield.pass_serial;in.pass.age_ticks=unsigned(in.pass.frame-1);
  const auto v=judge.judge(in);
  if(selected){
   ck(in.pass.guard_attack&&v.result==post08::Result::Confirmed,"each fresh E_OC2 index parries with unchanged CP03 and verified moving Shield Attack");
   const auto plan=post08::deflect_plan({4,a.mOcState,3},{4,a.mOcState,3},true);ck(plan==post08::DeflectPlan::NativeContinues,"unchanged deflection continuation gate");
   const auto profile=postbal01::capture(g.eng.obs.ledger,lg.posture.variants,slot,in.id,in.generation,read_aux(&a));
   const auto preview=g.pe.px.preview_parry(slot,in.generation,in.id,v.token,g.eng.obs.ledger,profile);
   const auto out=g.pe.px.apply_parry(slot,in.generation,in.id,v.token,g.eng.obs.ledger,profile,&preview);
   const int gain=kind==cp05::Kind::Red4?27:20;ck(out.applied&&out.gain==gain&&out.after==std::min(100,out.before+gain),"actual approved27/20 saturated transaction committed");
   judge.commit(slot,in.generation,in.id,v.token,true);cp04_contact(in,v,true,out.before,out.after);cp05_contact(in,v,true,out.before,out.after);++confirmed;
   ck(!cp04_guard_pass(),"one real shield action cannot confirm another contact");ck(judge.judge(in).result==post08::Result::Duplicate,"two spheres one confirmed parry episode");
   if(out.crossed){break_index=s.swing;ck(!s.active()&&!s.launch_requested,"100 cancels pending synchronously");break;}
  }else{ck(v.blocked&&v.result!=post08::Result::Confirmed,"ordinary guard is not a parry");cp04_contact(in,v,false,-1,-1);cp05_contact(in,v,false,-1,-1);}
  a.mSphs_at[0].hit=true;player_guard=true;pending_recoil=true;player.mProcID=daAlink_c::PROC_GUARD_SLIP;player.actual_clip=false;
 }
 ck(confirmed==inputs,"every credited defense has independent native action/progression, no free credits");
 if(!e.latched)ck(c5.c.kind[unsigned(kind)].completed==1,"all promised native swings complete through misses and full recoil");
 if(initial==0&&!regen&&mask==((1u<<cp05::count(kind))-1))ck(e.latched&&break_index==(kind==cp05::Kind::Red4?4u:5u),"27x4 or20x5 breaks only without intervening regen");
 ck(std::uint64_t(initial)+g.pe.px.c.units_added==std::uint64_t(e.posture)+g.pe.px.c.units_recovered,"actual unchanged regeneration and saturated gains conserve posture under the selected cadence");
 if(e.latched){auto token=s.token;native_tick(a);ck(s.token==token&&!s.active(),"latched next callback never launches extra CP05 swing");}
}
void parry_matrix(){
 for(auto kind:{cp05::Kind::Red4,cp05::Kind::Black8,cp05::Kind::Black7}){
  for(unsigned n=0;n<cp05::count(kind);++n)for(int initial:{0,20,60,80,99})for(bool regen:{false,true})parry_case(kind,1u<<n,initial,regen,false);
  for(unsigned mask:{3u,6u,24u,48u,(1u<<cp05::count(kind))-1})if(mask<(1u<<cp05::count(kind)))for(bool regen:{false,true})parry_case(kind,mask,0,regen,false);
  parry_case(kind,0,0,false,true);
 }
 // The existing fury/chip refusal is intact; normal rates must remainbelow1.5.
 ck(cp05::kBlackMaxRate<post08::policy::kFuryRateThreshold,"Black normal cannot cross legacy possible-fury gate");
}
void other_and_context(){
 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Black8);auto& s=c5.slots[slot];for(unsigned n=0;n<100&&!s.at_live;++n)native_tick(a);ck(s.at_live,"actual weapon window for other target");hit_actor.profname=99;a.mSphs_at[0].hit=true;native_tick(a);ck(c5.c.kind[2].other[0]==1&&c5.c.kind[2].parries[0]==0,"other target observed once without parry credit or unsafe target dereference");native_tick(a);ck(c5.c.kind[2].other[0]==1,"other target duplicate not counted again");a.mSphs_at[0].hit=false;hit_actor.profname=fpcNm_ALINK_e;}
 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Red4);native_tick(a);auto& s=c5.slots[slot];auto serial=s.context_serial;auto seed=s.seed;stage="ROOM_B";cp05_maintenance();ck(!s.active()&&!s.bound,"management context change releases no retained actor pointer");(void)cp05_actor(&a);ck(s.context_serial>serial&&s.seed!=seed,"new context changes isolated stream");stage="LAB";cp05_maintenance();(void)cp05_actor(&a);ck(s.context_serial>serial+1&&s.seed!=seed,"A-B-A cannot reuse old stream or combo identity");}
 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Red4);native_tick(a);auto& s=c5.slots[slot];auto token=s.token;auto* ptr=&a;void* args[]={&ptr};++g.p8.tick;on_eoc_pre_cp05(nullptr,args,nullptr,nullptr);a.morf.mFrameCtrl.frame+=20;on_eoc_early_post_cp05(nullptr,args,nullptr,nullptr);on_eoc_post_cp05(nullptr,args,nullptr,nullptr);ck(!s.active()&&s.token==token,"foreign animation frame jump cannot retain ownership");}
}
void authentic_fury_projection(){
 daE_OC_c a;int slot=prepare(a,cp05::Kind::Black8);native_tick(a);auto& s=c5.slots[slot];auto token=s.token;
 struct Io{unsigned char p[2]={2,2};unsigned char get(unsigned i){return p[i];}void set(unsigned i,unsigned char v){p[i]=v;}}io;
 devf_dyn::Ledger ledger;auto admission=devf_dyn::admit(ledger,a.id,&a,unsigned(s.key.generation),devf_variant::Variant::Black,"E_OC2",io);
 ck(admission.rec&&admission.rec->owned&&io.p[0]==3&&io.p[1]==3,"authentic Expansion admission owns a pristine two-sphere pair");auto& e=*admission.rec;
 devf_dyn::capture_pre(e,220);devf_dyn::Post p;p.post_health=219;p.action=4;p.room=0;p.stage=stage_token(stage);p.seq=1;
 auto ev=devf_dyn::post_update(ledger,e,io,p,[](const auto&){return true;});
 ck(ev.kind==devf_dyn::Ev::FuryStart&&e.fury==300&&io.p[0]==6&&io.p[1]==6,"actual CP10.6 HP-loss source projects Fury300 to BOTH ATP6 spheres");
 a.atp=io.p[0];policy_ports[slot].fury=e.fury;native_tick(a);ck(!s.active()&&s.token==token,"normal combo loses ownership on authoritative ATP6 projection");
 for(unsigned n=0;n<300;++n){devf_dyn::capture_pre(e,219);++p.seq;ev=devf_dyn::post_update(ledger,e,io,p,[](const auto&){return true;});}
 ck(e.fury==0&&io.p[0]==3&&io.p[1]==3&&ev.kind==devf_dyn::Ev::FuryEnd,"authentic300 ownPOST expiry projects normal3 again");
 post08::Input in;in.e_oc2=true;in.anim_ok=true;in.atp=6;in.anim_rate=2;ck(post08::Parry::fury_chip_possible(in),"Fury chip parry exclusion unchanged");
 in.atp=3;ck(post08::Parry::fury_chip_possible(in),"expiry midFury2.0 clip cannot turn that clip into normal parry");
 in.anim_rate=cp05::kBlackMaxRate;ck(!post08::Parry::fury_chip_possible(in),"new Black NORMAL rate stays below the frozen conservative threshold");
 a.atp=3;policy_ports[slot].fury=0;a.morf.setPlaySpeed(2);auto opportunities=c5.c.distribution[1].opportunities;native_tick(a);ck(c5.c.distribution[1].opportunities==opportunities,"nonfresh expiredFury clip never rerolled");
}
void failed_commit_and_invalid_signature(){
 {reset();daE_OC_c a;int slot=admit(a,false);a.setBck(5,0,5,1);a.morf.mFrameCtrl.frame=1;a.mOcState=1;
 auto* self=&a;void* args[]={&self};++g.p8.tick;on_eoc_pre_cp05(nullptr,args,nullptr,nullptr);
 // A stand-in FOREIGN writer imitates115 after earlyPOST. There was NO
 // executeAttack/0 receipt. Speed and apparent frame1 alone are insufficient.
 on_eoc_early_post_cp05(nullptr,args,nullptr,nullptr);a.morf.setPlaySpeed(1.15f);on_eoc_post_cp05(nullptr,args,nullptr,nullptr);
 ck(c5.c.distribution[0].opportunities==0&&c5.slots[slot].token==0,"foreign115 without genuine Attack0 receipt cannot create opportunity");}

 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Red4);auto& s=c5.slots[slot];for(unsigned n=0;n<100&&!s.at_live;++n)native_tick(a);
 post08::Input in;in.slot=slot;in.id=s.key.id;in.generation=s.key.generation;in.at_on_link[0]=true;post08::Verdict v;v.first_contact=true;v.result=post08::Result::Confirmed;v.blocked=true;v.token=1;
 cp05_contact(in,v,false,0,0);ck(c5.c.kind[1].parries[0]==0&&g.pe.px.entry(slot).posture==0,"a verdict without successful posture+deflection commit cannot become a CP05 parry");}
 for(int atp:{1,4,-2}){reset();daE_OC_c a;admit(a,false);a.atp=atp;ck(cp05_actor(&a)==nullptr,"unknown/legacyBlue/mismatched native spheres are never Red/Black");}
}
int main(){authentic_fury_projection();failed_commit_and_invalid_signature();other_and_context();parry_matrix();all_patterns();normals_and_exclusions();priority_during_gap();fury_and_safety();dense_context_and_summary();std::printf("CP05_ADAPTER checks=%llu failures=0 RESULT=LAB_PASS native_attack_and_find_exact=true expansion_red_black_exact_policy=true native_game_executed=false visual=HUMAN_RUNTIME_PENDING\n",checks);}
