int terminal_setup(daE_OC_c& a,int variant,int state,bool c,int hp,int posture,unsigned actor_id=0){
 reset();a=daE_OC_c{};a.mpMorf=&a.morf;a.id=actor_id?actor_id:812+unsigned(variant);a.health=hp;a.blue=variant==0;a.mName=a.blue?"E_OC":"E_OC2";a.atp=variant==0?1:variant==1?2:variant==2?3:6;
 post03::Events ev;const auto adm=a.blue?Admission::E_OC_CANDIDATE:Admission::E_OC2_CANDIDATE;auto o=g.eng.obs.ledger.observe(a.id,reinterpret_cast<std::uintptr_t>(&a),adm,g.eng.obs.epoch.epoch,1,ev);g.pe.px.sync(g.eng.obs.ledger);
 lg.posture.variants.observe(o.slot,a.id,o.generation,!a.blue,a.atp);
 const auto profile=postbal01::capture(g.eng.obs.ledger,lg.posture.variants,o.slot,a.id,o.generation,read_aux(&a));g.pe.px.observe_profile(profile,g.eng.obs.ledger);
 auto& initial=const_cast<post04::Entry&>(g.pe.px.entry(unsigned(o.slot)));ck(initial.bound&&!initial.frozen&&!initial.dead,"valid live posture sidecar before fixture snapshot");initial.posture=static_cast<unsigned char>(posture);
 a.setBck(c?6:5,0,0,0);a.mOcState=state;a.morf.mFrameCtrl.frame=state==3?0:float(c?47:42)-.001f;a.morf.stopped=true;
 incoming_type=AT_TYPE_MASTER_SWORD|AT_TYPE_NORMAL_SWORD;incoming_power=40;incoming_obj.type=incoming_type;a.mSphs_cc[0].tg=a.mSphs_cc[1].tg=true;a.field_0x6cc=0;suppression_enabled=true;return o.slot;
}
void observed_five(){
 const int states[]={3,1,3,2,3},before[]={54,0,27,0,44},gain[]={17,17,17,13,17},hp[]={330,330,330,335,295};const unsigned ids[]={812,813,826,1168,2411},gen[]={27,28,23,40,84},event_id[]={29,37,59,181,282};
 for(unsigned n=0;n<5;++n)for(bool fixed:{false,true}){
  daE_OC_c a;const int slot=terminal_setup(a,n==3?3:1,states[n],n==3,hp[n],before[n],ids[n]);use_fix02=fixed;if(n==1)a.mSphs_cc[0].tg=false;const auto count=suppression_commits;native_damage_transaction(a);
  ck(a.health==hp[n]-40,"exact confirmed native HP damage retained");ck(g.pe.px.entry(slot).posture==before[n]+gain[n],"exact observed variant posture increment retained");
  if(fixed){ck(suppression_commits==count+1&&a.mActionMode==4&&a.mOcState==states[n],"FIX02 restores only the native terminal continuation");a.executeAttack();ck(a.mActionMode==3||a.mActionMode==4&&a.mOcState==0,"original Attack performs its native terminal exit");ck(a.checkBck(28),"native exit selects its own wait clip");ck(!a.mSphs_at[0].on&&!a.mSphs_at[1].on,"terminal continuation never rearms an expired hitbox");}
  else ck(a.mActionMode==5&&suppression_commits==count,"0.11.9 reproduces each retained premature damage reaction");
  std::printf("FIX02_TERMINAL_OBSERVED source_actor=%u source_generation=%u source_event=%u fixed=%u posture=%d>%d native_hp=%d>%d terminal_state=%d native_game_executed=false\n",ids[n],gen[n],event_id[n],unsigned(fixed),before[n],before[n]+gain[n],hp[n],hp[n]-40,states[n]);
 }
}
void terminal_variants(){
 for(int variant=0;variant<4;++variant)for(bool c:{false,true})for(int state:{c?2:1,3})for(int points:{0,17,27,44,54,71,86,99})for(bool fixed:{false,true}){
  daE_OC_c a;int slot=terminal_setup(a,variant,state,c,330,points);use_fix02=fixed;const unsigned old=suppression_commits;const int weight=variant==0?25:variant==1?17:13;native_damage_transaction(a);
  const int after=std::min(100,points+weight);ck(g.pe.px.entry(slot).posture==after,"all variants retain exact normalized posture");ck(a.health==290,"all native ordinary HP damage retained");
  if(fixed&&after<100)ck(suppression_commits==old+1&&a.mActionMode==4,"eligible stopped/recoil no premature stagger");
  else ck(a.mActionMode==5,"old exclusion or genuine posture100 remains native");
 }
 for(bool c:{false,true})for(int state:{c?2:1,3})for(int variant=0;variant<4;++variant){
  daE_OC_c a;int slot=terminal_setup(a,variant,state,c,30,0);use_fix02=true;const auto old=suppression_commits;native_damage_transaction(a);ck(a.health<=1&&suppression_commits==old,"native lethal outcome never suppressed");(void)slot;
 }
}

void terminal_receipt_negatives(){
 for(int fault=0;fault<23;++fault){
  daE_OC_c a;terminal_setup(a,1,1,false,330,0);use_fix02=true;native_damage_transaction(a);auto post=last_native_post;auto report=last_native_report;
  ck(fix02_terminal_proof(&a,post,report),"fresh complete production terminal receipt control");
  if(fault==0)g.cur.actor=0;if(fault==1)g.cur.tracked=false;if(fault==2)g.cur.slot=-1;if(fault==3)++report.generation;if(fault==4)++report.id;if(fault==5)++report.slot;if(fault==6)g.cur.from_link=false;
  if(fault==7)report.fam.family=post03::Family::ShieldAttack;if(fault==8)report.j.lethal=true;if(fault==9)report.post.health=1;if(fault==10)report.post.mode=6;if(fault==11)report.post.state=3;if(fault==12)report.post.cooldown=20;if(fault==13)report.post.cutdown=1;
  if(fault==14)a.morf.clip=28;if(fault==15)a.morf.mFrameCtrl.frame=13;if(fault==16)a.morf.mFrameCtrl.rate=1.15f;if(fault==17)a.speedF=1;if(fault==18)a.morf.stopped=false;if(fault==19)post.aux.anim_attr=2;if(fault==20)pause=true;if(fault==21)event=true;if(fault==22)g.rx.off=true;
  ck(!fix02_terminal_proof(&a,post,report),"real helper refuses stale identity, foreign animation, nonordinary/lethal or unsafe lifecycle");
 }
}

void regenerated_and_duplicate(){
 for(int variant=0;variant<4;++variant)for(bool c:{false,true})for(int state:{c?2:1,3})for(bool held:{false,true}){
  daE_OC_c a;const int slot=terminal_setup(a,variant,state,c,330,86);use_fix02=true;player_guard=held;lf.r_held=lf.guard_accepted=held;
  for(unsigned tick=0;tick<90;++tick)g.pe.px.advance();const int before=g.pe.px.entry(slot).posture;ck(before<86,"original CP02 regeneration is active before terminal sword contact");
  native_damage_transaction(a);const int after=g.pe.px.entry(slot).posture;const int weight=variant==0?25:variant==1?17:13;
  ck(after==std::min(100,before+weight)&&a.health==290,"regeneration does not invalidate exact native hit/variant posture");
  const auto health=a.health;native_damage_transaction(a);ck(a.health==health&&g.pe.px.entry(slot).posture==after,"two hitboxes and immediate duplicate call never charge HP or posture twice");
 }
}
int main(){observed_five();terminal_variants();terminal_receipt_negatives();regenerated_and_duplicate();std::printf("CP05_FIX02_NATIVE checks=%llu failures=0 RESULT=LAB_PASS native_game_executed=false visual=HUMAN_RUNTIME_PENDING\n",checks);}
