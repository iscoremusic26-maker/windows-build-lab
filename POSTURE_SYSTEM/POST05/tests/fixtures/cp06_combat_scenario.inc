// CP06 combat laboratory. Production CP04/CP05/CP06/FIX02 callbacks, unchanged native
// executeAttack/Find/cc_set/cCcS register-clear, actual POST08 judge, shield chain and
// posture engine, real Expansion Red/Black/CP10.6 value policies and the transcribed
// Expansion CP19F2 chip. Link motion, collision broadphase/contacts and render are MODELED.
int dComIfGs_getLife(){return life;}
float dComIfGp_getItemLifeCount(){return g_dComIfG_gameInfo.play.getItemLifeCount();}
unsigned char dComIfGp_getItemLifeCountType(){return g_dComIfG_gameInfo.play.getItemLifeCountType();}
void dComIfGp_setItemLifeCount(float h,unsigned char t){g_dComIfG_gameInfo.play.setItemLifeCount(h,t);}
#include "ROOT/tests/reference/cp06_expansion_cp19f2_chip.inc"
post08::Parry judge6;post08::ArmTrack arm6;
void shield_input(bool trigger){auto* link=&player;void* args[]={&link};player.mItemButton=daAlink_c::BTN_R;player.mItemTrigger=trigger?daAlink_c::BTN_B:0;on_stick_post_fx(nullptr,args,nullptr,nullptr);on_stick_post_cp04(nullptr,args,nullptr,nullptr);}
int prepare(daE_OC_c& a,cp05::Kind kind,bool black_normal){
 for(unsigned id=1;id<1000;++id){reset();a=daE_OC_c{};a.mpMorf=&a.morf;a.id=id;int slot=admit(a,kind==cp05::Kind::Black8||kind==cp05::Kind::Black7||black_normal);auto* s=cp05_actor(&a);ck(s,"certified E_OC2 profile");auto copy=*s;cp05::Counters c;if(copy.decide(2,true,c)==kind)return slot;}
 ck(false,"deterministic selection seed");return -1;
}
struct LinkSim{bool hold=true;unsigned slip=0;bool blocked_prev=false,sa_blocked=false;}lsim;
// Third-party pending-life writer between the two CP06 POSTs (negative control only).
static bool force_commit_fail=false;static float foreign_pending=0;static int foreign_life=0;static unsigned char foreign_type=0xff;
struct Swing{std::uint64_t token=0;bool pressed=false,contacted=false;int result=-1;std::uint64_t press_tick=0,contact_tick=0;};
struct Ledger6{unsigned confirmed=0,normal=0,unblocked=0,other=0,late_logged=0;std::uint64_t serials[16]{};unsigned nserial=0;int verdicts[16];float deltas[16];unsigned swings[16];};
enum class Press{Never,Timely,Late,Early};
void link_mod_update(){
 ++g.p8.tick;const bool sa=player.mProcID==daAlink_c::PROC_GUARD_ATTACK;
 g.link_track.observe(g.p8.tick,true,player.id,player.mProcID,sa,player.mUnderFrameCtrl[0].frame,player.field_0x3478,player.field_0x347c);
 cp04_observe_motion(&player);
}
void link_execute(bool press){
 if(lsim.blocked_prev&&player.mProcID==daAlink_c::PROC_GUARD_ATTACK){player.mProcID=daAlink_c::PROC_GUARD_SLIP;player.actual_clip=false;lsim.slip=12;}
 else if(player.mProcID==daAlink_c::PROC_GUARD_SLIP){if(lsim.slip&&--lsim.slip==0)player.mProcID=6;}
 else if(player.mProcID==daAlink_c::PROC_GUARD_ATTACK){player.mUnderFrameCtrl[0].frame+=1;if(player.mUnderFrameCtrl[0].frame>=18){player.mProcID=lsim.hold?6:3;player.actual_clip=false;}}
 lsim.blocked_prev=false;
 if(press)lf.bash_offered_now=true;
 shield_input(press);
 if(press&&(player.mProcID==6||player.mProcID==3))player.procGuardAttackInit();
 auto* link=&player;void* largs[]={&link};on_link_damage_post_cp04(nullptr,largs,nullptr,nullptr);
 player_guard=lsim.hold||player.mProcID==daAlink_c::PROC_GUARD_ATTACK||player.mProcID==daAlink_c::PROC_GUARD_SLIP;
}
// One enemy execute + its share of the collision pass.
post08::Verdict enemy_tick(daE_OC_c& a,bool want_contact,bool shield,Ledger6& L,Swing& sw){
 const int slot=slot_of(a);auto& p=policy_ports[slot];auto* self=&a;void* args[]={&self};
 if(a.atp==3||a.atp==6){a.atp=p.fury?6:3;}
 const auto gen=g.eng.obs.ledger.slot(slot).generation;if(!a.blue)lg.posture.variants.observe(slot,a.id,gen,true,a.atp);
 struct Cb{int priority;int kind;};
 Cb pre[3]={{0,0},{registration_priorities[0],1},{registration_priorities[6],2}};
 Cb post[6]={{registration_priorities[8],0},{registration_priorities[10],4},{0,1},{registration_priorities[1],2},{registration_priorities[9],3},{registration_priorities[11],5}};
 if(reverse_registration){std::reverse(pre,pre+3);std::reverse(post,post+6);}
 std::stable_sort(pre,pre+3,[](auto x,auto y){return x.priority>y.priority;});std::stable_sort(post,post+6,[](auto x,auto y){return x.priority>y.priority;});
 for(auto x:pre){if(x.kind==0){expansion_pre(a,p);p.pre_mode=a.mActionMode;p.pre_state=a.mOcState;p.pre_timer=a.field_0x6c2;}else if(x.kind==1)on_eoc_pre_cp04(nullptr,args,nullptr,nullptr);else on_eoc_pre_cp05(nullptr,args,nullptr,nullptr);}
 if(a.field_0x6ca)--a.field_0x6ca;if(a.field_0x6c0)--a.field_0x6c0;if(a.field_0x6c2)--a.field_0x6c2;a.mSphs_at[0].on=a.mSphs_at[1].on=false;
 // damage_check PRE: POST08 judge on the previous pass, identical inputs to production post08_judge.
 auto& c=g.cur;c={};c.actor=reinterpret_cast<std::uintptr_t>(&a);c.slot=slot;c.id=a.id;c.generation=gen;c.tracked=true;
 auto& in=c.pin;in.slot=slot;in.id=a.id;in.generation=gen;in.tracked=true;const auto& e=g.pe.px.entry(unsigned(slot));in.latched=e.latched;in.frozen=e.frozen;in.dead=e.dead;
 in.hp=a.health;in.mode=a.mActionMode;in.state=a.mOcState;in.anim_ok=true;in.anim_frame=a.morf.getFrame();in.anim_rate=a.morf.getPlaySpeed();in.e_oc2=!a.blue;in.atp=a.atp;in.link_ok=true;
 in.now.valid=true;in.now.proc=player.mProcID;in.now.guard_attack=player.mProcID==daAlink_c::PROC_GUARD_ATTACK;in.now.frame=player.mUnderFrameCtrl[0].frame;in.now.start=1;in.now.end=12;
 in.pass=g.link_track.pass_for(g.p8.tick,true,player.id);if(in.pass.guard_attack&&!cp04_guard_pass())in.pass.guard_attack=false;
 in.fresh=arm6.fresh(slot,a.id,gen,g.p8.tick);
 for(unsigned n=0;n<2;++n){in.at_hit[n]=a.mSphs_at[n].hit;in.at_on_link[n]=a.mSphs_at[n].hit&&a.mSphs_at[n].GetAtHitObj()==static_cast<HitObj*>(&player.mTgCyls[0]);in.at_shield[n]=a.mSphs_at[n].shield;}
 PreReads r;r.aux=read_aux(&a);in.fury_verified=cp06_fury_verified(r);
 c.balance_profile=postbal01::capture(g.eng.obs.ledger,lg.posture.variants,slot,a.id,gen,r.aux);
 const auto v=judge6.judge(in);c.pv=v;
 if(v.result!=post08::Result::None){
  fix02_guard_contact(&a);
  if(v.first_contact&&v.blocked&&in.pass.guard_attack&&player.mProcID==daAlink_c::PROC_GUARD_ATTACK)lsim.blocked_prev=true;
  if(v.result==post08::Result::Confirmed&&force_commit_fail){judge6.commit(slot,gen,a.id,v.token,false);cp04_contact(in,v,false,-1,-1);cp05_contact(in,v,false,-1,-1);cp06_parry_committed(in,v,false);++L.confirmed;}
  else if(v.result==post08::Result::Confirmed){
   const auto out=g.pe.px.apply_parry(slot,gen,a.id,v.token,g.eng.obs.ledger,c.balance_profile);ck(out.applied,"actual posture parry transaction");
   judge6.commit(slot,gen,a.id,v.token,true);cp04_contact(in,v,true,out.before,out.after);cp05_contact(in,v,true,out.before,out.after);cp06_parry_committed(in,v,true);
   if(L.nserial<16){L.serials[L.nserial]=in.pass.serial;L.deltas[L.nserial]=v.delta;L.swings[L.nserial]=c5.slots[slot].swing;++L.nserial;}++L.confirmed;
  }else{cp04_contact(in,v,false,-1,-1);cp05_contact(in,v,false,-1,-1);cp06_contact(in,v);
   if(v.first_contact){if(v.result==post08::Result::NormalGuard)++L.normal;else if(v.result==post08::Result::NotBlocked)++L.unblocked;else ++L.other;}}
  if(v.first_contact&&sw.result<0)sw.result=int(v.result);
 }
 if(a.mActionMode==4){on_attack_pre_cp04(nullptr,args,nullptr,nullptr);on_attack_pre_cp05(nullptr,args,nullptr,nullptr);a.executeAttack();}else if(a.mActionMode==3)a.executeFind();
 const bool at=a.mSphs_at[0].on||a.mSphs_at[1].on;arm6.note_post(slot,a.id,gen,g.p8.tick,at);
 native_update(a.morf);collision_world.reset();on_cc_pre_fix01(nullptr,args,nullptr,nullptr);a.cc_set();on_cc_post_fix01(nullptr,args,nullptr,nullptr);
 ck((a.mSphs_at[0].on||a.mSphs_at[1].on)==at,"temporary cleanup registration never arms the pass");
 for(auto x:post){
  if(x.kind==0)on_eoc_early_post_cp05(nullptr,args,nullptr,nullptr);
  else if(x.kind==4)on_eoc_post_cp06_capture(nullptr,args,nullptr,nullptr);
  else if(x.kind==1){expansion_post(a,p);if(!a.blue&&a.atp!=2){auto f=black_frame(a);expansion_chip_post(&a,unsigned(slot),p.black,p.fury,f);}
   if(foreign_pending!=0||foreign_type!=0xff)g_dComIfG_gameInfo.play.setItemLifeCount(foreign_pending,foreign_type==0xff?g_dComIfG_gameInfo.play.getItemLifeCountType():foreign_type);if(foreign_life)life+=foreign_life;}
  else if(x.kind==2)on_eoc_post_cp04(nullptr,args,nullptr,nullptr);
  else if(x.kind==3)on_eoc_post_cp05(nullptr,args,nullptr,nullptr);
  else on_eoc_post_cp06_chip(nullptr,args,nullptr,nullptr);
 }
 // collision pass of this tick: engine clears every REGISTERED At object and Link's Tg; modeled contact afterwards.
 collision_world.ClrAtHitInf();for(auto& cyl:player.mTgCyls){cyl.tg=false;cyl.tg_shield=false;cyl.tg_obj=nullptr;}
 if(want_contact&&at&&!sw.contacted){sw.contacted=true;sw.contact_tick=g.p8.tick;hit_actor.profname=fpcNm_ALINK_e;auto& s0=a.mSphs_at[0];s0.hit=true;s0.shield=shield&&player_guard;
  auto& cyl=player.mTgCyls[0];cyl.tg=true;cyl.tg_shield=s0.shield;cyl.tg_obj=static_cast<const void*>(static_cast<HitObj*>(&s0));}
 return v;
}
int prepare6(daE_OC_c& a,cp05::Kind kind,bool black,int start_posture=0){
 int slot=prepare(a,kind,black);judge6=post08::Parry{};judge6.enabled=true;arm6.reset();lsim=LinkSim{};fx={};f2={};c6=Cp06Runtime{};ck(cp06_install(),"CP06 install on the existing E_OC execute target");
 ck(registration_count==12&&registration_priorities[10]==90&&registration_priorities[11]==-250,"two CP06 POST callbacks, before/after Expansion priority0");
 g.layout=1;g.p8.layout=1;player.mProcID=6;player.actual_clip=false;g_dComIfG_gameInfo.play.item_life=0;g_dComIfG_gameInfo.play.item_type=0;expansion_chip_blows={};expansion_chip_counts={};foreign_pending=0;foreign_life=0;foreign_type=0xff;life=40;
 auto& entry=const_cast<post04::Entry&>(g.pe.px.entry(unsigned(slot)));entry.posture=static_cast<unsigned char>(start_posture);
 if(black)lg.posture.variants.observe(slot,a.id,g.eng.obs.ledger.slot(slot).generation,true,3);
 return slot;
}
// Press policy: Timely = new R+B about three native steps before frame14 (Shield Attack
// frames 1..4 at the first armed pass). Late = one tick after the resolving pass.
bool should_press(const daE_OC_c& a,Press how,Swing& sw,float rate){
 if(how==Press::Never||sw.pressed)return false;
 if(how==Press::Late)return sw.contacted&&g.p8.tick>sw.contact_tick;
 const bool swing=a.mActionMode==4&&(a.mOcState==1||a.mOcState==2);if(!swing)return false;
 const float lead=how==Press::Early?10.f:3.f;return a.morf.getFrame()+lead*rate>=14.f&&a.morf.getFrame()<14.f;
}
void combo_parry_index(cp05::Kind kind,unsigned mask,bool clip_c,bool reverse){
 daE_OC_c a;const bool black=kind!=cp05::Kind::Red4;int slot=prepare6(a,kind,black);reverse_registration=reverse;variant_c=clip_c;auto& s=c5.slots[slot];
 Ledger6 L;Swing sw[9];unsigned last_token=0;const unsigned n=cp05::count(kind);
 for(unsigned t=0;t<2600&&c5.c.kind[unsigned(kind)].completed==0&&(t<3||s.active());++t){
  const unsigned idx=s.phase==cp05::Phase::Swing?s.swing:0;Swing& cur=sw[idx<9?idx:0];const bool want=idx>0&&idx<=n;
  const bool parry=want&&(mask&(1u<<(idx-1)));
  link_mod_update();const bool press=parry&&should_press(a,Press::Timely,cur,cp05::rate(kind,idx));if(press){cur.pressed=true;cur.press_tick=g.p8.tick;}
  link_execute(press);enemy_tick(a,want,true,L,cur);if(s.token!=last_token)last_token=unsigned(s.token);
 }
 ck(c5.c.kind[unsigned(kind)].completed==1,"combo completes with parries/guards at CP06 speeds");
 unsigned want_parries=0;for(unsigned i=0;i<n;++i)if(mask&(1u<<i))++want_parries;
 ck(L.confirmed==want_parries,"exactly one confirmed parry per selected index, never for guarded ones");
 ck(L.normal==n-want_parries&&L.unblocked==0,"every other index is an ordinary native guard");
 for(unsigned i=0;i<n;++i){const auto& st=c5.c.kind[unsigned(kind)];ck(st.contacts[i]==1,"each index has exactly one Link contact");ck(st.parries[i]==((mask>>i)&1u),"parry attributed to the correct swing index");ck(st.armed[i]==1,"each index opened its own native AT window");}
 for(unsigned i=0;i<L.nserial;++i){ck(L.deltas[i]>=-1&&L.deltas[i]<=7,"inside the unchanged POST08 window");for(unsigned j=0;j<i;++j)ck(L.serials[i]!=L.serials[j],"each parry needs its own native Shield Attack serial");}
 ck(c4.shield.c.native_starts==want_parries,"one genuine native Shield Attack init per parry");
 ck(g_dComIfG_gameInfo.play.item_life==0&&c6.chip.c.armed==0,"no chip machinery outside fury");
}
void combos(){
 const unsigned all[]={0xFu,0xFFu,0x7Fu};unsigned runs=0;
 for(auto kind:{cp05::Kind::Red4,cp05::Kind::Black8,cp05::Kind::Black7}){const unsigned n=cp05::count(kind);
  for(bool clip_c:{false,true})for(bool reverse:{false,true}){
   for(unsigned i=0;i<n;++i){combo_parry_index(kind,1u<<i,clip_c,reverse);++runs;}
   for(unsigned i=0;i+1<n;++i){combo_parry_index(kind,3u<<i,clip_c,reverse);++runs;}
   combo_parry_index(kind,0,clip_c,reverse);++runs;
   (void)all;
  }
 }
 std::printf("CP06_COMBO_PARRY runs=%u per_index_and_consecutive_pairs=ALL clips=B/C orders=both native_game_executed=false\n",runs);
}
void posture_break_mid_combo(){
 // Black8 all eight parried: 20 each -> the fifth crosses 100, legitimate cancellation, no further swing.
 daE_OC_c a;int slot=prepare6(a,cp05::Kind::Black8,true);auto& s=c5.slots[slot];Ledger6 L;Swing sw[9];
 for(unsigned t=0;t<2600&&(t<3||s.active());++t){const unsigned idx=s.phase==cp05::Phase::Swing?s.swing:0;Swing& cur=sw[idx<9?idx:0];link_mod_update();const bool press=idx&&should_press(a,Press::Timely,cur,cp05::rate(cp05::Kind::Black8,idx));if(press)cur.pressed=true;link_execute(press);enemy_tick(a,idx>0,true,L,cur);}
 const auto& st=c5.c.kind[unsigned(cp05::Kind::Black8)];
 ck(L.confirmed==5&&st.breaks==1&&st.starts[5]==0&&st.completed==0,"fifth Black parry breaks at100 and cancels the rest legitimately");
 ck(g.pe.px.entry(unsigned(slot)).posture>=100,"real posture crossing");
}
void held_r_and_late(){
 // Same Shield Attack cannot parry two swings; a late new R+B is logged, never credited.
 {daE_OC_c a;int slot=prepare6(a,cp05::Kind::Red4,false);auto& s=c5.slots[slot];Ledger6 L;Swing sw[9];
  for(unsigned t=0;t<2600&&(t<3||s.active());++t){const unsigned idx=s.phase==cp05::Phase::Swing?s.swing:0;Swing& cur=sw[idx<9?idx:0];link_mod_update();
   bool press=false;if(idx==2)press=should_press(a,Press::Timely,cur,1.35f);if(idx==3)press=should_press(a,Press::Late,cur,1.35f);if(press)cur.pressed=true;
   link_execute(press);enemy_tick(a,idx>0,true,L,cur);cp06_maintenance();}
  ck(L.confirmed==1&&L.normal==3,"swing2 parried, swing3 late press is a native guard, 1/4 ordinary guards");
  ck(c6.late.c.late==1&&c6.late.c.by_delta[0]+c6.late.c.by_delta[1]==1,"late R+B diagnosed once, after the resolving pass");
  ck(c5.c.kind[unsigned(cp05::Kind::Red4)].parries[2]==0&&c5.c.kind[unsigned(cp05::Kind::Red4)].parries[1]==1,"no credit to the late index");}
 {daE_OC_c a;int slot=prepare6(a,cp05::Kind::Black7,true);auto& s=c5.slots[slot];Ledger6 L;Swing sw[9];
  bool again=false;for(unsigned t=0;t<2600&&(t<3||s.active());++t){const unsigned idx=s.phase==cp05::Phase::Swing?s.swing:0;Swing& cur=sw[idx<9?idx:0];link_mod_update();bool press=idx==1&&should_press(a,Press::Early,cur,1.30f);if(press)cur.pressed=true;
   if(!again&&sw[1].contacted&&g.p8.tick==sw[1].contact_tick+2){press=true;again=true;}link_execute(press);enemy_tick(a,idx>0,true,L,cur);cp06_maintenance();}
  ck(L.confirmed==0&&sw[1].result==int(post08::Result::MissedEarly),"press ten steps early leaves the window: never a parry");ck(again&&c6.late.c.late==0,"a MISSED_EARLY contact is never diagnosed as a late press");}
}
// Red Expansion special x2 (actual Random cause from the unmodified source PRNG/cooldowns).
void red_special_x2(Press how){
 bool found=false;
 for(unsigned id=1;id<200&&!found;++id){
  reset();daE_OC_c a;a.id=id;a.mpMorf=&a.morf;int slot=admit(a,false);policy_ports[slot].random=true;judge6=post08::Parry{};judge6.enabled=true;arm6.reset();lsim=LinkSim{};fx={};f2={};c6=Cp06Runtime{};ck(cp06_install(),"install");g.layout=1;g.p8.layout=1;player.mProcID=6;player.actual_clip=false;
  Ledger6 L;Swing sw;bool special=false;
  for(unsigned t=0;t<90;++t){link_mod_update();
   const bool x2=policy_ports[slot].red.blow_cause==devf_red_ai::Cause::Random&&a.mActionMode==4&&(a.mOcState==1||a.mOcState==2);special=special||x2;
   const bool press=x2&&should_press(a,how,sw,2.f);if(press)sw.pressed=true;link_execute(press);
   enemy_tick(a,x2,true,L,sw);cp06_maintenance();if(sw.contacted&&g.p8.tick>sw.contact_tick+10)break;}
  if(!special||!sw.contacted)continue;found=true;
  ck(c5.c.distribution[0].opportunities==0&&!c5.slots[slot].active(),"special x2 never becomes a CP05 combo opportunity");
  if(how==Press::Timely){ck(L.confirmed==1,"timely Shield Attack parries the Expansion x2 special");ck(L.deltas[0]>=-1&&L.deltas[0]<=7,"unchanged window");}
  else{ck(L.confirmed==0&&L.normal==1,"late press against x2: native guard, no parry");if(how==Press::Late)ck(c6.late.c.late==1&&c6.late.c.rate2==1,"late x2 R+B diagnosed");}
 }
 ck(found,"authentic Random x2 reached");
}
// Black fury x2 + Expansion chip: neutralized only for an authenticated committed parry.
struct FuryCase{const char* name;Press press;float foreign=0;int life_change=0;unsigned char type=0xff;bool shield=true;bool expect_parry;float expect_pending;cp06::Chip expect;bool commit_fail=false;};
void fury_case(const FuryCase& k,bool reverse){
 daE_OC_c a;int slot=prepare6(a,cp05::Kind::Normal,true);reverse_registration=reverse;policy_ports[slot].fury=5000;a.atp=6;force_commit_fail=k.commit_fail;
 Ledger6 L;Swing sw;bool contacted=false;unsigned charged_before=0;
 for(unsigned t=0;t<160;++t){link_mod_update();
  const bool blow=a.mActionMode==4&&(a.mOcState==1||a.mOcState==2)&&a.morf.getPlaySpeed()==2.f;
  const bool press=blow&&should_press(a,k.press,sw,2.f);if(press)sw.pressed=true;link_execute(press);
  const bool judge_tick=sw.contacted&&!contacted;if(judge_tick){foreign_pending=k.foreign;foreign_life=k.life_change;foreign_type=k.type;charged_before=expansion_chip_counts.applied;}
  enemy_tick(a,blow,k.shield,L,sw);
  if(judge_tick){foreign_pending=0;foreign_life=0;foreign_type=0xff;contacted=true;}
  if(contacted&&g.p8.tick>sw.contact_tick+4)break;}
 ck(contacted,"fury x2 contact reached");
 ck((L.confirmed==1)==k.expect_parry,k.name);
 if(k.shield)ck(expansion_chip_counts.applied==charged_before+1,"Expansion charged its own fury chip on the engine shield pair");
 const float pending=g_dComIfG_gameInfo.play.item_life;ck(pending==k.expect_pending,k.name);
 const auto& r=c6.chip.c.result;ck(k.expect==cp06::Chip::None?c6.chip.c.armed==0:r[unsigned(k.expect)]==1,k.name);
 ck(c6.compensated==(k.expect==cp06::Chip::Neutralized?1u:0u),"exactly one attributed restore or none");force_commit_fail=false;
}
// Fury begins mid-blow (CP10.6 power 3->6 on a blow Expansion started at x1.15): no x2, no Expansion chip,
// no CP06 window; the legacy gate (rate<1.5) keeps the ordinary parry. Rate is a required fury signal.
void fury_mid_blow(bool reverse){
 daE_OC_c a;int slot=prepare6(a,cp05::Kind::Normal,true);reverse_registration=reverse;Ledger6 L;Swing sw;bool contacted=false;
 for(unsigned t=0;t<160;++t){link_mod_update();const bool blow=a.mActionMode==4&&(a.mOcState==1||a.mOcState==2);if(blow&&a.morf.getFrame()>=8&&!policy_ports[slot].fury)policy_ports[slot].fury=5000;
  const bool press=blow&&should_press(a,Press::Timely,sw,a.morf.getPlaySpeed());if(press)sw.pressed=true;link_execute(press);enemy_tick(a,blow,true,L,sw);if(sw.contacted)contacted=true;if(contacted&&g.p8.tick>sw.contact_tick+4)break;}
 ck(contacted&&a.atp==6&&L.confirmed==1,"atp6 at x1.15: legacy ordinary parry");ck(c6.chip.c.armed==0&&c6.fury_offered==0,"no fury window without Expansion x2");
 ck(g_dComIfG_gameInfo.play.item_life==0&&expansion_chip_counts.applied==0&&expansion_chip_counts.not_fury>=1,"Expansion itself refuses a chip for a blow started outside fury");
}
void fury(){
 const FuryCase cases[]={
  {"perfect fury parry: Expansion chip fully neutralized",Press::Timely,0,0,0xff,true,true,0.f,cp06::Chip::Neutralized},
  {"fury normal guard keeps Expansion's chip",Press::Never,0,0,0xff,true,false,-1.f,cp06::Chip::None},
  {"simultaneous third-party -1 is never absorbed",Press::Timely,-1.f,0,0xff,true,true,-2.f,cp06::Chip::DeltaMismatch},
  {"simultaneous heal +1 is never cancelled",Press::Timely,1.f,0,0xff,true,true,0.f,cp06::Chip::NoChipObserved},
  {"type rewritten by another writer: refused",Press::Timely,0,0,3,true,true,-1.f,cp06::Chip::TypeChanged},
  {"real life change inside the window: refused",Press::Timely,0,-1,0xff,true,true,-1.f,cp06::Chip::LifeChanged},
  {"late press against fury: guard chip, no parry",Press::Late,0,0,0xff,true,false,-1.f,cp06::Chip::None},
  {"failed parry commit: no chip window, chip kept",Press::Timely,0,0,0xff,true,true,-1.f,cp06::Chip::None,true},
 };
 for(bool reverse:{false,true}){for(const auto& k:cases)fury_case(k,reverse);fury_mid_blow(reverse);}
 // Classification negatives: atp6 is required; Black NORMAL at its fastest CP06 rate keeps legacy behaviour.
 post08::Input in;in.e_oc2=true;in.atp=3;in.anim_ok=true;in.anim_rate=2.f;in.fury_verified=false;ck(post08::Parry::fury_chip_possible(in),"atp3 at x2 (fury ended mid-blow) still refused");
 in.anim_rate=cp05::kBlackMaxRate;ck(!post08::Parry::fury_chip_possible(in),"Black NORMAL 1.30 is not mistaken for fury");
 ck(!cp06::fury_parry_authenticated(true,6,true,1.15f,true)&&!cp06::fury_parry_authenticated(true,3,true,2.f,true)&&!cp06::fury_parry_authenticated(true,6,true,2.f,false)&&cp06::fury_parry_authenticated(true,6,true,2.f,true),"all four fury signals required");
 std::printf("CP06_FURY cases=%u orders=2 expansion_chip_source=TRANSCRIBED_fe47c8b_cp19f2 native_game_executed=false\n",unsigned(sizeof(cases)/sizeof(cases[0])));
}
void blue_profile(){
 daE_OC_c a;reset();a=daE_OC_c{};a.mpMorf=&a.morf;a.id=1;ck(cp04::kModerateGap==8&&cp04::kShortGap==2&&cp04::kOpenRate==1.10f&&cp04::kFastRate==1.5f,"approved CP06 Blue profile");
}
int main(){combos();posture_break_mid_combo();held_r_and_late();red_special_x2(Press::Timely);red_special_x2(Press::Late);red_special_x2(Press::Never);fury();blue_profile();
 std::printf("CP06_COMBAT checks=%llu failures=0 RESULT=LAB_PASS native_attack_find_ccset_exact=true expansion_policies_exact=true link_motion_and_contacts=MODELED native_game_executed=false visual=HUMAN_RUNTIME_PENDING\n",checks);}
