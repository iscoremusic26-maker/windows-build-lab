post03::Snapshot damage_snapshot(const daE_OC_c& a){post03::Snapshot s;s.health=a.health;s.mode=a.mActionMode;s.state=a.mOcState;s.old_mode=a.mOldActionMode;s.cooldown=a.field_0x6cc;s.cutdown=a.cutdown;s.tg_bits=(a.mSphs_cc[0].tg?1:0)|(a.mSphs_cc[1].tg?2:0);return s;}
post03::NativeAux damage_aux(const daE_OC_c& a){auto x=read_aux(&a);x.t6c0=a.field_0x6c0;x.t6c2=a.field_0x6c2;x.set_bits=(a.mSphs_cc[0].tg_set?1:0)|(a.mSphs_cc[0].co_set?2:0)|(a.mSphs_cc[1].tg_set?4:0)|(a.mSphs_cc[1].co_set?8:0);x.head_lock=a.head_lock;x.anim_ok=1;x.anim_attr=0;x.anim_state=0;x.anim_end=short(a.mpMorf->mFrameCtrl.getEnd());x.anim_rate=a.mpMorf->getPlaySpeed();x.anim_frame=a.mpMorf->getFrame();x.speed=a.speedF;return x;}
post05::LiveMode damage_live(const daE_OC_c& a){return {a.mActionMode,a.mOcState,a.mOldActionMode,short(a.field_0x6c0),short(a.field_0x6c2),static_cast<unsigned char>(a.head_lock),damage_aux(a).set_bits};}
void native_damage_transaction(daE_OC_c& a){
 post03::PreReads pre;pre.facts.profile_ok=pre.facts.live=true;pre.facts.status_10000=false;pre.facts.name=post03::NameKind::E_OC2;pre.facts.atp=a.atp;pre.snap=damage_snapshot(a);pre.aux=damage_aux(a);pre.raw0=a.mSphs_cc[0].tg?reinterpret_cast<std::uintptr_t>(&incoming_obj):0;pre.raw1=a.mSphs_cc[1].tg?reinterpret_cast<std::uintptr_t>(&incoming_obj):0;
 g.eng.policy=post03::AdmissionPolicy::V2Compat;g.eng.masks={0x4000002u,0x10u,0x2000u,0x20u,0x400000u,0x10000u,0x40u,0x80u};
 post03::Events events;g.pe.on_pre(reinterpret_cast<std::uintptr_t>(&a),a.id,pre,events);++native_damage_calls;a.damage_check();
 post03::PostReads post;post.alive=true;post.snap=damage_snapshot(a);post.aux=damage_aux(a);post.selected=reinterpret_cast<std::uintptr_t>(a.mAtInfo.mpCollider);post03::HitReport report;post03::Events after;
 const int slot=slot_of(a);const auto& l=g.eng.obs.ledger.slot(unsigned(slot));const auto profile=postbal01::capture(g.eng.obs.ledger,lg.posture.variants,slot,l.id,l.generation,read_aux(&a));
 bool hit=g.pe.on_post(reinterpret_cast<std::uintptr_t>(&a),post,[&](std::uintptr_t ptr,std::uint32_t& type){if(ptr!=reinterpret_cast<std::uintptr_t>(&incoming_obj))return false;type=incoming_type;return true;},after,report,profile);
 if(hit&&suppression_enabled){post05::Reaction rx;rx.stagger_policy=post05::StaggerPolicy::General;rx.e2_policy=post05::E2Policy::MatureBC;rx.e2_env=post05::E2Env::Possible;post05::Apply ap;
  auto d=post05::process(rx,g.pe,report,[&]{return damage_live(a);},[&](const post05::LiveMode& v){a.mActionMode=v.mode;a.mOcState=v.state;a.mOldActionMode=v.old_mode;a.field_0x6c0=v.t6c0;a.field_0x6c2=v.t6c2;a.head_lock=v.head_lock;a.mSphs_cc[0].tg_set=(v.set_bits&1)!=0;a.mSphs_cc[0].co_set=(v.set_bits&2)!=0;a.mSphs_cc[1].tg_set=(v.set_bits&4)!=0;a.mSphs_cc[1].co_set=(v.set_bits&8)!=0;},ap,lnks1fix::continuation(pre.snap.mode,pre.snap.state,pre.aux,post.aux)==lnks1fix::Continuation::Safe);
  if(ap==post05::Apply::Applied)++suppression_commits;
 }
 post04::PItem item;while(g.pe.px.pop(item)){}
}
void guard_controller_ab(){
 for(auto kind:{cp05::Kind::Red4,cp05::Kind::Black8,cp05::Kind::Black7})for(int control:{0,1,2})for(bool release:{false,true}){
  daE_OC_c a;int slot=prepare(a,kind);auto& s=c5.slots[slot];if(control<2)c5.enabled=false;if(control==0)c4.enabled=false;incoming_type=0;incoming_power=0;suppression_enabled=true;player_guard=true;lf.r_held=lf.guard_accepted=true;variant_c=false;std::uint64_t at[2]{};unsigned windows=0,gap_run=0,max_gap=0;bool last=false;
  for(unsigned t=0;t<240&&windows<2;++t){player_guard=!(release&&t>=50);lf.r_held=lf.guard_accepted=player_guard;policy_ports[slot].link.motion=player_guard?devf_red_ai::Motion::Guard:devf_red_ai::Motion::Quiet;native_tick(a);
   a.mSphs_at[0].hit=false;const bool armed=a.mSphs_at[0].on||a.mSphs_at[1].on;
   if(armed&&!last){at[windows++]=g.p8.tick;a.mSphs_at[0].hit=true;}last=armed;
   if(s.phase==cp05::Phase::Gap){++gap_run;max_gap=std::max(max_gap,gap_run);}else gap_run=0;
  }
  ck(windows==2,"guard-held and guard-released controllers progress to a second genuine native window");
  if(control<2)ck(c5.c.distribution[0].opportunities+c5.c.distribution[1].opportunities==0,"native and CP04 controls have no CP05 decisions");
  std::printf("CP05_FIX01_GUARD_AB source=%s controller=%s kind=%s release_R_at=%s first_AT=%llu second_AT=%llu interval=%llu max_CP05_gap_ticks=%u native_game_executed=false\n",OLD_SOURCE?"0.11.8":"FIX01",control==0?"NATIVE_PLUS_EXPANSION":control==1?"CP04_PLUS_EXPANSION":"CP05_PLUS_EXPANSION",cp05::name(kind),release?"50":"NEVER",(unsigned long long)at[0],(unsigned long long)at[1],(unsigned long long)(at[1]-at[0]),max_gap);
 }
 incoming_type=0;incoming_power=0;
}
#if !OLD_SOURCE
void complete_native_body_cases(){
 for(auto kind:{cp05::Kind::Red4,cp05::Kind::Black8,cp05::Kind::Black7})for(bool reverse:{false,true})for(bool guard:{false,true})for(bool clip_c:{false,true}){
  daE_OC_c a;int slot=prepare(a,kind);reverse_registration=reverse;variant_c=clip_c;player_guard=guard;lf.r_held=guard;lf.guard_accepted=guard;incoming_power=0;incoming_type=0;suppression_enabled=true;native_damage_calls=suppression_commits=0;auto& s=c5.slots[slot];
  // A valid external special cooldown is part of this no-interruption case.
  // Its ready form is tested below and MUST still preempt the normal combo.
  policy_ports[slot].link.motion=guard?devf_red_ai::Motion::Guard:devf_red_ai::Motion::Offensive;
  unsigned impact_swing=0,blocked=0;bool cooldown_set=false;const unsigned shock_before=native_shocks;
  for(unsigned t=0;t<900&&c5.c.kind[unsigned(kind)].completed==0;++t){
   if(policy_ports[slot].red.id&&!cooldown_set){policy_ports[slot].red.next_passive=300;cooldown_set=true;}
   native_tick(a);
   if(a.mSphs_at[0].hit){ck(a.mOcState==3&&a.morf.getPlaySpeed()==-1,"native shield block really enters negative recoil");++blocked;}
   a.mSphs_at[0].hit=a.mSphs_at[1].hit=false;a.mSphs_cc[0].tg=a.mSphs_cc[1].tg=false;incoming_type=0;
   ck(s.active()||c5.c.kind[unsigned(kind)].completed==1,"no premature cancellation through full native damage transaction");
   if(s.phase==cp05::Phase::Swing&&s.at_seen&&impact_swing!=s.swing){impact_swing=s.swing;incoming_type=AT_TYPE_SHIELD_ATTACK;a.mSphs_cc[0].tg=a.mSphs_cc[1].tg=true;if(guard)a.mSphs_at[0].hit=true;}
  }
  const auto& stats=c5.c.kind[unsigned(kind)];ck(stats.completed==1&&stats.cancelled==0&&s.token==cp05::count(kind),"all genuine native swings complete under zero HP body impacts");
  ck(suppression_commits==cp05::count(kind),"one actual POST05 suppression transaction per swing");
  if(guard)ck(blocked==cp05::count(kind)&&native_shocks-shock_before==cp05::count(kind),"one original native shock and complete recoil for each shield block");
  for(unsigned n=0;n<cp05::count(kind);++n)ck(stats.armed[n]==1,"two native spheres remain one window per swing");
 }
}
void genuine_native_priority_cases(){
 for(auto kind:{cp05::Kind::Red4,cp05::Kind::Black8,cp05::Kind::Black7})for(unsigned n=1;n<=cp05::count(kind);++n)for(bool suppress:{false,true}){
  daE_OC_c a;int slot=prepare(a,kind);auto& s=c5.slots[slot];incoming_type=0;incoming_power=0;suppression_enabled=suppress;for(unsigned t=0;t<900&&(s.swing<n||!s.at_seen);++t)native_tick(a);
  ck(s.swing==n&&s.at_seen,"specific real swing reached before genuine injury");const auto token=s.token;const int hp=a.health;incoming_type=AT_TYPE_NORMAL_SWORD;incoming_power=10;a.mSphs_cc[0].tg=true;native_tick(a);
  ck(a.health==hp-10&&!s.active()&&s.token==token,"committed native HP loss cancels remaining scheduler immediately");
  incoming_type=0;a.mSphs_cc[0].tg=false;incoming_power=0;for(unsigned t=0;t<3;++t)native_tick(a);ck(s.token==token,"no extra scheduled hit after genuine injury");
 }
 // Zero HP does NOT make a retained native shield reaction safe to override.
 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Black7);auto& s=c5.slots[slot];suppression_enabled=false;incoming_type=0;for(unsigned t=0;t<80&&!s.at_seen;++t)native_tick(a);incoming_type=AT_TYPE_SHIELD_ATTACK;incoming_power=0;a.mSphs_cc[0].tg=true;native_tick(a);ck(a.mActionMode==5&&!s.active()&&c5.c.kind[3].legitimate==1,"unsuppressed native zero HP Damage mode retains priority");}
 incoming_type=0;incoming_power=0;suppression_enabled=true;
}
void native_transition_and_lease_cases(){
 // Ordinary Find1/2/3 are native search/recovery transitions, not a priority
 // by themselves. The original must reach Find4 before another attack entry.
 for(int state:{1,2,3}){daE_OC_c a;int slot=prepare(a,cp05::Kind::Black8);auto& s=c5.slots[slot];incoming_type=0;incoming_power=0;suppression_enabled=true;for(unsigned t=0;t<120&&s.phase!=cp05::Phase::Gap;++t)native_tick(a);a.mOcState=state;a.field_0x6c0=4;auto token=s.token;
  for(unsigned t=0;t<3;++t){native_tick(a);ck(s.active()&&s.token==token,"ordinary native Find search/recovery doesn't cancel or force an early attack");}
  for(unsigned t=0;t<700&&c5.c.kind[2].completed==0;++t)native_tick(a);ck(c5.c.kind[2].completed==1,"original Find recovery eventually resumes the complete combo");
 }
 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Red4);auto& s=c5.slots[slot];for(unsigned t=0;t<100&&s.phase!=cp05::Phase::Gap;++t)native_tick(a);native_tick(a);ck(a.mActionMode==3&&a.mOcState==4,"native Find4 before lease takeover test");
  ++g.p8.tick;auto* self=&a;void* args[]={&self};on_eoc_pre_cp05(nullptr,args,nullptr,nullptr);ck(s.lease&&a.field_0x6c2==2,"bounded native lease acquired");a.setActionMode(5,0);a.field_0x6c2=1;on_eoc_early_post_cp05(nullptr,args,nullptr,nullptr);on_eoc_post_cp05(nullptr,args,nullptr,nullptr);
  ck(a.field_0x6c2==1&&!s.active()&&!s.lease,"changed native mode owns even a coincident timer1; never restore an old mode's timer");
 }
 // An actual committed weighted hit remains a cancellation even if another
 // system leaves the net HP unchanged. A raw Tg bit alone still isn't one.
 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Red4);auto& s=c5.slots[slot];incoming_type=0;for(unsigned t=0;t<90&&!s.at_seen;++t)native_tick(a);incoming_type=AT_TYPE_NORMAL_SWORD;incoming_power=0;a.mSphs_cc[0].tg=true;native_tick(a);ck(!s.active()&&s.detail==cp05::Detail::WeightedHit,"confirmed weighted native report retains priority independently of net HP");incoming_type=0;a.mSphs_cc[0].tg=false;}
 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Black7);auto& s=c5.slots[slot];incoming_type=0;for(unsigned t=0;t<80&&!s.at_seen;++t)native_tick(a);incoming_type=AT_TYPE_SHIELD_ATTACK;incoming_power=1;a.mSphs_cc[0].tg=true;native_tick(a);ck(!s.active()&&s.detail==cp05::Detail::HpLoss,"actual unweighted native HP loss is still a priority");incoming_type=0;incoming_power=0;a.mSphs_cc[0].tg=false;}
 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Black8);native_tick(a);auto& s=c5.slots[slot];auto& h=g.pe.last;s.execute_event=40;h.event=s.execute_event+1;h.slot=slot;h.generation=s.key.generation;h.applied=true;h.weight=13;ck(cp05_weighted_native_hit(s),"new matching committed native report is observable");++h.generation;ck(!cp05_weighted_native_hit(s),"foreign generation report cannot cancel this actor");h.generation=s.key.generation;h.event=s.execute_event;ck(!cp05_weighted_native_hit(s),"previous event cannot cancel a fresh execution");h.event=s.execute_event+1;h.applied=false;ck(!cp05_weighted_native_hit(s),"uncommitted report cannot cancel a fresh execution");h.applied=true;h.weight=0;ck(!cp05_weighted_native_hit(s),"zero-weight shield report does not imply a genuine priority");}
 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Red4);native_tick(a);auto& s=c5.slots[slot];c5.log_class_lines[0]=384;s.log_lines[0]=32;auto lines=c5.lines;cp05_cancel(s,cp05::Cancel::Readback);ck(c5.lines==lines+1,"new critical cancellation case retains an independent example reserve after ordinary quotas");}
 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Red4);++g.p8.tick;auto* self=&a;void* args[]={&self};on_eoc_pre_cp05(nullptr,args,nullptr,nullptr);stage="MIDCALL";on_attack_pre_cp05(nullptr,args,nullptr,nullptr);a.executeAttack();native_update(a.morf);on_eoc_early_post_cp05(nullptr,args,nullptr,nullptr);a.morf.setPlaySpeed(1.15f);on_eoc_post_cp05(nullptr,args,nullptr,nullptr);ck(c5.c.distribution[0].opportunities==0&&c5.slots[slot].token==0,"a rebound context without matching EOC PRE cannot acquire ownership mid-call");}
 {daE_OC_c a;int slot=prepare(a,cp05::Kind::Red4);auto& s=c5.slots[slot];for(unsigned t=0;t<100&&s.phase!=cp05::Phase::Gap;++t)native_tick(a);native_tick(a);auto opportunities=c5.c.distribution[0].opportunities;auto token=s.token;a.field_0x6c2=7;
  ++g.p8.tick;auto* self=&a;void* args[]={&self};on_eoc_pre_cp05(nullptr,args,nullptr,nullptr);ck(!s.active()&&!s.lease&&a.field_0x6c2==7,"a changed timer BEFORE the lease belongs to another writer and is never borrowed");
  a.setActionMode(4,0);on_attack_pre_cp05(nullptr,args,nullptr,nullptr);a.executeAttack();native_update(a.morf);on_eoc_early_post_cp05(nullptr,args,nullptr,nullptr);a.morf.setPlaySpeed(1.15f);on_eoc_post_cp05(nullptr,args,nullptr,nullptr);
  ck(c5.c.distribution[0].opportunities==opportunities&&s.token==token,"a cancellation followed by another fresh start in the same execution never rerolls");
 }
}
#endif
int main(){
 unsigned reproduced=0,continued=0;
 for(auto kind:{cp05::Kind::Red4,cp05::Kind::Black8,cp05::Kind::Black7}){
  daE_OC_c a;int slot=prepare(a,kind);native_damage_calls=suppression_commits=0;incoming_type=0;incoming_power=0;suppression_enabled=true;auto& s=c5.slots[slot];
  for(unsigned t=0;t<100;++t){native_tick(a);if(s.at_seen)break;}
  ck(s.phase==cp05::Phase::Swing&&s.at_seen,"real native attack window before body impact");
  const auto token=s.token;a.mSphs_cc[0].tg=a.mSphs_cc[1].tg=true;incoming_type=AT_TYPE_SHIELD_ATTACK;incoming_power=0;
  native_tick(a);ck(native_damage_calls>0&&suppression_commits==1,"exact native ShieldAttack damage path + real POST05 suppression committed");ck(a.health==220&&a.mActionMode==4,"zero HP shield reaction genuinely restored to native attack");
  if(OLD_SOURCE){ck(!s.active(),"old 0.11.8 reproduces premature raw Tg cancellation");++reproduced;}
  else{ck(s.active()&&s.token==token,"FIX01 preserves zero HP suppressed ShieldAttack episode");++continued;}
  a.mSphs_cc[0].tg=a.mSphs_cc[1].tg=false;incoming_type=0;
 }
 #if !OLD_SOURCE
 complete_native_body_cases();genuine_native_priority_cases();native_transition_and_lease_cases();
 #endif
 guard_controller_ab();
 std::printf("CP05_FIX01_NATIVE checks=%llu failures=0 RESULT=LAB_PASS old_source=%u premature_cancel_reproduced=%u zero_hp_native_continuations=%u native_game_executed=false visual=HUMAN_RUNTIME_PENDING\n",checks,unsigned(OLD_SOURCE),reproduced,continued);
}
