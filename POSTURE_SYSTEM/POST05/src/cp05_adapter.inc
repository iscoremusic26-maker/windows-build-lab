// CP05 production: E_OC2 Red/Black only. No Blue/shield implementation changes.
#if defined(_MSC_VER)
#define CP05_NOINLINE __declspec(noinline)
#else
#define CP05_NOINLINE __attribute__((noinline))
#endif
struct Cp05Runtime {
 bool enabled=false,reentered_call=false;unsigned depth=0,lines=0;
 unsigned log_class_lines[7]{};
 unsigned critical_case_examples[32]{};
 std::uint64_t dropped=0,last_summary_tick=0;cp05::Combo slots[cp05::kSlots]{};cp05::Counters c;
} c5;
void cp05_capture(daE_OC_c* a,cp05::Combo& s,cp05::Site site)noexcept{
 s.site=site;auto& v=s.view;v.mode=a->mActionMode;v.state=a->mOcState;v.timer=a->field_0x6c2;v.speed=a->speedF;
 v.frame=a->mpMorf?a->mpMorf->getFrame():-1;v.rate=a->mpMorf?a->mpMorf->getPlaySpeed():0;v.end=a->mpMorf?a->mpMorf->mFrameCtrl.getEnd():0;
 auto* p=dComIfGp_getLinkPlayer();v.link_proc=-1;v.link_distance=-1;v.held_r=lf.r_held;v.guard=lf.guard_accepted;
 if(p&&p->profname==fpcNm_ALINK_e&&p->state.init_state!=3){v.link_proc=static_cast<daAlink_c*>(p)->mProcID;const float x=a->current.pos.x-p->current.pos.x,z=a->current.pos.z-p->current.pos.z;const float q=x*x+z*z;if(q==q&&q>=0&&q<1.e16f)v.link_distance=q;}
 v.pose_valid=true;
 for(unsigned n=0;n<2;++n){const auto& c=a->mSphs_at[n].GetC();v.centre[n][0]=c.x;v.centre[n][1]=c.y;v.centre[n][2]=c.z;v.radius[n]=a->mSphs_at[n].GetR();
  v.pose_valid=v.pose_valid&&cp04::finite(c.x)&&cp04::finite(c.y)&&cp04::finite(c.z)&&cp04::finite(v.radius[n])&&v.radius[n]>0&&v.radius[n]<=300;
 }
}
// Bounded internal event tokens: no additional CRT string-search dependency.
bool cp05_same(const char* a,const char* b)noexcept{for(unsigned i=0;i<64;++i){if(a[i]!=b[i])return false;if(!a[i])return true;}return false;}
bool cp05_contains(const char* text,const char* part)noexcept{
 for(unsigned i=0;i<64&&text[i];++i){unsigned n=0;while(n<64&&i+n<64&&part[n]&&text[i+n]==part[n])++n;if(n<64&&!part[n])return true;}return false;
}
unsigned cp05_log_class(const char* event)noexcept{
 if(cp05_contains(event,"CANCEL")||cp05_contains(event,"NOT_STARTED")||cp05_contains(event,"FAILED")||cp05_contains(event,"EXCLUDED")||cp05_contains(event,"FOREIGN")||cp05_same(event,"INIT"))return 0;
 if(cp05_contains(event,"SELECTED")||cp05_same(event,"OPPORTUNITY"))return 1;
 if(cp05_contains(event,"HITBOX")||cp05_contains(event,"POSE"))return 2;
 if(cp05_contains(event,"CONTACT")||cp05_contains(event,"PARRY")||cp05_same(event,"GUARD")||cp05_contains(event,"SHIELD"))return 3;
 if(cp05_contains(event,"GAP")||cp05_contains(event,"NEXT"))return 4;
 if(cp05_contains(event,"SPEED")||cp05_contains(event,"SWING"))return 5;return 6;
}
const char* cp05_expected(const cp05::Combo* s)noexcept{
 if(!s)return "NO_ACTOR";if(s->phase==cp05::Phase::Swing)return "ATTACK1_2_3_OR_PROVEN_FULL_NATIVE_EXIT";
 if(s->phase==cp05::Phase::Gap)return s->launch_requested?"ATTACK0_PENDING_FRESH_BC":"FIND0_1_2_3_4_NATIVE_RECOVERY";return "NATIVE_OWNER";
}
void cp05_log(const char* event,cp05::Combo* s,const char* reason="OK",float frame=-1,float rate=999,
 int before=-1,int after=-1,std::uint64_t episode=0,std::uint64_t sa=0,float delta=0)noexcept{
 const unsigned cls=cp05_log_class(event);constexpr unsigned global[]={384,192,384,256,256,256,128},actor[]={32,12,24,24,24,16,12};
 const unsigned critical=s&&cp05_same(event,"CANCEL_REASON")?(s->last_cancel==cp05::Cancel::Priority?unsigned(s->detail):16+unsigned(s->last_cancel)):30;
 const bool reserve=cls==0&&s&&critical<32&&s->critical_examples[critical]<2&&c5.critical_case_examples[critical]<16;
 if(!reserve&&(c5.log_class_lines[cls]>=global[cls]||(s&&s->log_lines[cls]>=actor[cls]))){++c5.dropped;return;}
 if(reserve){++s->critical_examples[critical];++c5.critical_case_examples[critical];}++c5.log_class_lines[cls];if(s)++s->log_lines[cls];++c5.lines;char line[kLogLineMax*6];
 const cp05::View zero{};const auto& v=s?s->view:zero;if(frame<0)frame=v.frame;if(rate==999)rate=v.rate;
 std::snprintf(line,sizeof(line),"CP05_%s version=0.11.11 source=%s actor=%u generation=%llu context_serial=%llu epoch=%u stage=%u room=%d layer=%d variant=%s fury_gate=%u origin=%u tick=%llu local_tick=%llu opportunity_id=%llu draws=%u rejections=%u kind=%s combo_id=%llu swing_index=%u swing_token=%llu bck=%d clip_end=%.3f frame=%.3f rate=%.3f last_early_post_rate=%.3f native_sample_tick=%llu required_normal_handoff=1.150 rate_owner=%s arm_eval_frame=%.3f at=%u target_delta=%u launch_at=%llu enemy_episode=%llu sa_serial=%llu input_serial=%llu parry_delta=%.3f posture_before=%d posture_after=%d reason=%s phase=%s site=%s subreason=%s expected=%s native_pre=%d/%d native_now=%d/%d native_timer_pre=%d native_timer_now=%d expected_rate=%.3f speed=%.3f link_proc=%d held_R=%u manual_guard_accepted=%u distance_xz_squared=%.3f tg_pending=%u pose_valid=%u at0=(%.2f,%.2f,%.2f,r%.2f) at1=(%.2f,%.2f,%.2f,r%.2f) engine_spark_event=NOT_OBSERVED runtime=HUMAN_RUNTIME_PENDING",
 event,POST05_SOURCE_COMMIT,s?s->key.id:0,(unsigned long long)(s?s->key.generation:0),(unsigned long long)(s?s->context_serial:0),s?s->key.epoch:0,s?s->key.stage:0,s?s->key.room:-1,s?s->key.layer:-1,
 s?(s->variant==cp05::Variant::Red?"RED":"BLACK"):"NONE",unsigned(s&&s->fury),s?s->origin:0,(unsigned long long)g.p8.tick,(unsigned long long)(s?s->local_tick:0),
 (unsigned long long)(s?s->opportunity:0),s?s->last_draws:0,s?s->last_rejections:0,s?cp05::name(s->kind):"NONE",(unsigned long long)(s?s->combo_id:0),s?s->swing:0,
 (unsigned long long)(s?s->token:0),s?s->bck:0,s?s->end:0,frame,rate,s?s->raw_rate:0,(unsigned long long)(s?s->raw_tick:0),s&&s->rate_owned?"CP05_FRESH_HANDOFF":"NATIVE_OR_EXPANSION",s?s->attack_frame:0,unsigned(s&&s->at_live),
 s&&s->swing?cp05::target(s->kind,s->swing-1):0,(unsigned long long)(s?s->launch_at:0),(unsigned long long)episode,(unsigned long long)sa,
 (unsigned long long)c4.shield.input_serial,delta,before,after,reason,s?cp05::phase_name(s->phase):"NONE",s?cp05::name(s->site):"NONE",s?cp05::name(s->detail):"NONE",cp05_expected(s),v.pre_mode,v.pre_state,v.mode,v.state,v.pre_timer,v.timer,s?s->rate_value:0,v.speed,v.link_proc,unsigned(v.held_r),unsigned(v.guard),v.link_distance,unsigned(s&&s->tg_pending),unsigned(v.pose_valid),v.centre[0][0],v.centre[0][1],v.centre[0][2],v.radius[0],v.centre[1][0],v.centre[1][1],v.centre[1][2],v.radius[1]);lnk_write(LOG_LEVEL_INFO,line);
}
void cp05_cancel(cp05::Combo& s,cp05::Cancel why,cp05::Detail sub=cp05::Detail::None)noexcept{
 if(!s.active())return;if(s.phase==cp05::Phase::Recovery){s.cancel(why,c5.c);return;}
 if(why==cp05::Cancel::Priority&&sub==cp05::Detail::None){const int m=s.view.mode;sub=m==5?cp05::Detail::DamageMode:m==6?cp05::Detail::BigDamageMode:m==9?cp05::Detail::DeathMode:(m==10||m==13||m==14)?cp05::Detail::WaterFallMode:(m==7||m==8||m==11||m==12)?cp05::Detail::WatchMode:m==4?cp05::Detail::ExternalAttack:cp05::Detail::OtherMode;}
 s.detail=sub;s.last_cancel=why;cp05_log(s.phase==cp05::Phase::Selected?"SELECTED_NOT_STARTED":"CANCEL_REASON",&s,cp05::name(why));s.cancel(why,c5.c,sub,g.p8.tick);
}
cp05::Combo* cp05_actor(daE_OC_c* a,cp05::Site site=cp05::Site::Pre)noexcept{
 const int i=cp04_slot(a);if(i<0){if(a&&a->profname==fpcNm_E_OC_e){unsigned live=0;for(unsigned n=0;n<cp05::kSlots;++n)if(g.eng.obs.ledger.slot(n).id&&g.eng.obs.ledger.slot(n).life==Life::Active)++live;if(live==cp05::kSlots)++c5.c.overflow;else ++c5.c.untracked;}return nullptr;}
 const auto& ledger=g.eng.obs.ledger.slot(unsigned(i));const auto aux=read_aux(a);
 auto& s=c5.slots[i];
 if(!aux.valid||aux.resource!=unsigned(NameKind::E_OC2)){if(s.bound)cp05_cancel(s,cp05::Cancel::Context);return nullptr;}
 const auto profile=postbal01::capture(g.eng.obs.ledger,lg.posture.variants,i,ledger.id,ledger.generation,aux);
 cp04::Key key;key.id=ledger.id;key.generation=ledger.generation;key.epoch=ledger.epoch_last;key.stage=stage_token(dComIfGp_getStartStageName());key.room=fopAcM_GetRoomNo(a);key.layer=dComIfG_play_c::getLayerNo(0);
 const bool red=profile.verified&&profile.variant==postbal01::Variant::Red;
 const bool black=profile.verified&&(profile.variant==postbal01::Variant::Black||profile.variant==postbal01::Variant::BlackFury);
 if(!red&&!black){if(s.bound)cp05_cancel(s,cp05::Cancel::Context);return nullptr;}
 const auto variant=red?cp05::Variant::Red:cp05::Variant::Black;
 if(!s.bound||!(s.key==key)||s.variant!=variant){cp05_cancel(s,cp05::Cancel::Context);s.bind(key,variant);}
 const auto& p=g.pe.px.entry(unsigned(i));s.fury=profile.variant==postbal01::Variant::BlackFury;s.origin=profile.method;
 s.valid=p.bound&&p.id==ledger.id&&p.generation==ledger.generation&&!p.frozen&&!p.dead&&!p.latched&&a->health>1;
 cp05_capture(a,s,site);
 if(s.fury)cp05_cancel(s,cp05::Cancel::Fury);
 else if(!s.valid)cp05_cancel(s,p.latched?cp05::Cancel::Posture100:cp05::Cancel::Priority);
 return &s;
}
bool cp05_safe(daE_OC_c* a,const cp05::Combo& s)noexcept{
 return cp04_sim_safe()&&s.valid&&!s.fury&&!a->field_0x6e3&&!a->mHide;
}
bool cp05_weighted_native_hit(const cp05::Combo& s)noexcept{
 const unsigned slot=unsigned(&s-c5.slots);const auto& h=g.pe.last;
 return slot<cp05::kSlots&&h.slot==int(slot)&&h.generation==s.key.generation&&h.event&&h.event!=s.execute_event&&h.applied&&h.weight>0;
}
// New writer1: only an observed native frame1/rate1 -> Expansion normal1.15
// handoff, classified E_OC2, selected combo. No x2/pending special is retimed.
CP05_NOINLINE bool cp05_native_rate(daE_OC_c* a,cp05::Combo& s)noexcept{
 if(!s.native_fresh||!cp05_safe(a,s)||s.phase!=cp05::Phase::Swing||s.rate_owned||s.rate_value!=cp05::rate(s.kind,s.swing)||!a->mpMorf||
    a->mActionMode!=4||(a->mOcState!=1&&a->mOcState!=2)||cp04_bck(a)!=s.bck||
    !cp04::near(a->mpMorf->getFrame(),1)||a->mpMorf->getPlaySpeed()!=1.15f)return false;
 a->mpMorf->setPlaySpeed(s.rate_value);const bool ok=a->mpMorf->getPlaySpeed()==s.rate_value;
 if(ok){s.rate_owned=true;++c5.c.kind[unsigned(s.kind)].rates;}return ok;
}
// New writer2: compare-before-write16bit lease. PRE runs after Expansion0;
// EARLY POST restores only the still-owned native2->1 value, before Expansion0.
// Other mods see the native timer/decrement, never a long CP05 waiting timer.
CP05_NOINLINE bool cp05_native_timer(daE_OC_c* a,int expected,int value)noexcept{
 if(expected<0||expected>32767||value<0||value>32767||a->field_0x6c2!=expected)return false;
 a->field_0x6c2=static_cast<s16>(value);return a->field_0x6c2==value;
}
// Reuse the already verified native mode writer; the CP05 caller supplies its
// own end/gap ownership gate. Never assign mActionMode/mOcState directly.
CP05_NOINLINE bool cp05_native_mode(daE_OC_c* a,int mode)noexcept{return cp04_native_mode(a,mode);}
HookAction on_eoc_pre_cp05(ModContext*,void* args,void*,void*)noexcept{
 if(!c5.enabled||!args)return HOOK_CONTINUE;
 if(c5.depth++){c5.reentered_call=true;++c5.c.reentered;return HOOK_CONTINUE;}c5.reentered_call=false;
 auto* a=mods::arg<daE_OC_c*>(args,0);auto* s=cp05_actor(a,cp05::Site::Pre);if(!s)return HOOK_CONTINUE;
 if(s->last_execute&&g.p8.tick>s->last_execute+1)cp05_cancel(*s,cp05::Cancel::Pause);
 if(!s->execute(g.p8.tick,c5.c))return HOOK_CONTINUE;
 s->execute_hp=a->health;s->tg_pending=a->mSphs_cc[0].cCcD_ObjHitInf::ChkTgHit()||a->mSphs_cc[1].cCcD_ObjHitInf::ChkTgHit();
 s->execute_event=g.pe.last.event;
 s->view.pre_mode=a->mActionMode;s->view.pre_state=a->mOcState;s->view.pre_timer=a->field_0x6c2;
 if(s->tg_pending&&s->active()){++c5.c.kind[unsigned(s->kind)].tg_deferred;cp05_log("TG_NATIVE_DECISION_PENDING",s,"RAW_TG_IS_NOT_A_COMMITTED_REACTION");}
 if(!cp05_safe(a,*s)){cp05_cancel(*s,s->fury?cp05::Cancel::Fury:cp05::Cancel::Priority,cp05::Detail::UnsafeFlags);return HOOK_CONTINUE;}
 if(s->phase==cp05::Phase::Recovery){s->phase=cp05::Phase::Idle;return HOOK_CONTINUE;}
 if(s->phase==cp05::Phase::Gap){
  if(s->launch_requested){
   if(a->mActionMode!=4||a->mOcState!=0||s->local_tick-s->launch_tick>cp05::kWindupMax)cp05_cancel(*s,cp05::Cancel::Windup);
   return HOOK_CONTINUE;
  }
  if(a->mActionMode!=3){cp05_cancel(*s,cp05::Cancel::Priority);return HOOK_CONTINUE;}
  if(s->gap_timer_known&&a->field_0x6c2!=s->gap_timer_expected){cp05_cancel(*s,cp05::Cancel::Foreign);return HOOK_CONTINUE;}
  if(a->mOcState==4){
   // Native Find/0 gets its genuine20-tick initialization; subsequent Find4
   // executes can wait without hiding that timer from Expansion's PRE/POST.
   s->native_timer=a->field_0x6c2;
   if(cp05_native_timer(a,s->native_timer,2)){s->lease=true;++c5.c.kind[unsigned(s->kind)].lease_acquired;}else cp05_cancel(*s,cp05::Cancel::Readback);
  }else if(a->mOcState<0||a->mOcState>4)cp05_cancel(*s,cp05::Cancel::Priority,cp05::Detail::GapState);
 }else if(s->active()&&a->mActionMode!=4)cp05_cancel(*s,cp05::Cancel::Priority);
 return HOOK_CONTINUE;
}
HookAction on_attack_pre_cp05(ModContext*,void* args,void*,void*)noexcept{
 if(!c5.enabled||!args||c5.depth!=1)return HOOK_CONTINUE;
 auto* a=mods::arg<daE_OC_c*>(args,0);auto* s=cp05_actor(a,cp05::Site::AttackPre);if(!s)return HOOK_CONTINUE;
 // A newly rebound sidecar inside damage_check has no matching EOC PRE.
 // Wait for the next genuine execution; never acquire ownership mid-call.
 if(s->last_execute!=g.p8.tick)return HOOK_CONTINUE;
 if(s->processed||a->mActionMode!=4)return HOOK_CONTINUE;
 // This callback is AFTER damage_check and its complete POST05 transaction.
 // Do not confuse a zero-damage suppressed Shield Attack with a real injury.
 if(a->health<s->execute_hp||cp05_weighted_native_hit(*s)){cp05_cancel(*s,cp05::Cancel::Priority,a->health<s->execute_hp?cp05::Detail::HpLoss:cp05::Detail::WeightedHit);return HOOK_CONTINUE;}
 s->attack_entered=true;s->attack_frame=a->mpMorf?a->mpMorf->getFrame():0;
 s->will_finish=cp05_safe(a,*s)&&s->phase==cp05::Phase::Swing&&a->mOcState>=1&&a->mOcState<=3&&a->mpMorf&&a->mpMorf->isStop();
 if(a->mOcState==0&&cp04::finite(a->speedF)&&cp04::absf(a->speedF)<=1.f&&!a->checkBeforeFloorBg(200.f))s->start_pending=true;
 return HOOK_CONTINUE;
}
void on_eoc_early_post_cp05(ModContext*,void* args,void*,void*)noexcept{
 if(!c5.enabled||!args||c5.depth!=1)return;
 auto* a=mods::arg<daE_OC_c*>(args,0);auto* s=cp05_actor(a,cp05::Site::EarlyPost);if(!s)return;
 if(s->lease){
  const int restore=s->native_timer>0?s->native_timer-1:0;
  // A changed native mode owns its own timer, even when the value happens to1.
  auto& stats=c5.c.kind[unsigned(s->kind)];
  if(a->mActionMode!=3||a->mOcState!=4){++stats.lease_native_mode;cp05_cancel(*s,cp05::Cancel::Priority);}
  else if(!cp05_native_timer(a,1,restore)){++stats.lease_foreign;cp05_cancel(*s,cp05::Cancel::Foreign);}else ++stats.lease_restored;s->lease=false;
 }
 if(a->health<s->execute_hp||cp05_weighted_native_hit(*s)){cp05_cancel(*s,cp05::Cancel::Priority,a->health<s->execute_hp?cp05::Detail::HpLoss:cp05::Detail::WeightedHit);s->native_fresh=false;return;}
 s->native_fresh=!s->processed&&s->start_pending&&s->attack_entered&&a->mpMorf&&a->mActionMode==4&&
  (a->mOcState==1||a->mOcState==2)&&cp04_bck(a)&&cp04::near(a->mpMorf->getFrame(),1)&&a->mpMorf->getPlaySpeed()==1.f;
 s->raw_rate=a->mpMorf?a->mpMorf->getPlaySpeed():0;s->raw_tick=g.p8.tick;
 s->fresh_bck=s->native_fresh?cp04_bck(a):0;s->fresh_end=a->mpMorf?a->mpMorf->mFrameCtrl.getEnd():0;
}
void cp05_observe_other(daE_OC_c* a,cp05::Combo& s)noexcept{
 if(!s.at_live||!s.at_seen||s.other_seen)return;
 const bool hit[2]={a->mSphs_at[0].cCcD_ObjHitInf::ChkAtHit()!=0,a->mSphs_at[1].cCcD_ObjHitInf::ChkAtHit()!=0};
 if(!hit[0]&&!hit[1])return;
 auto* base=dComIfGp_getLinkPlayer();auto* link=base&&base->profname==fpcNm_ALINK_e&&base->state.init_state!=3?static_cast<daAlink_c*>(base):nullptr;
 for(unsigned sphere=0;sphere<2;++sphere){if(!hit[sphere])continue;
  const void* target=static_cast<const void*>(a->mSphs_at[sphere].cCcD_ObjHitInf::GetAtHitObj());bool on_link=false;
  if(link)for(unsigned n=0;n<3;++n)on_link=on_link||target==static_cast<const void*>(static_cast<cCcD_Obj*>(&link->mTgCyls[n]));
  if(target&&!on_link){s.contact(false,false,false,c5.c);cp05_log("CONTACT_OTHER",&s,"FRESH_REGISTERED_TARGET_NOT_LIVE_LINK_TG_ID_UNRESOLVED_NO_DEREFERENCE",a->mpMorf?a->mpMorf->getFrame():0,a->mpMorf?a->mpMorf->getPlaySpeed():0);break;}
 }
}
void on_eoc_post_cp05(ModContext*,void* args,void*,void*)noexcept{
 if(!c5.enabled||!args)return;if(!c5.depth){++c5.c.errors;return;}if(--c5.depth)return;
 auto* a=mods::arg<daE_OC_c*>(args,0);auto* s=cp05_actor(a,cp05::Site::LatePost);if(!s||s->processed)return;s->processed=true;
 if(c5.reentered_call){cp05_cancel(*s,cp05::Cancel::Reentry);return;}
 if(!cp05_safe(a,*s)){if(s->fury&&s->native_fresh){++c5.c.fury;cp05_log("FURY_EXCLUDED",s,"TWO_NATIVE_ATP6_SPHERES_AND_CLASSIFIER_HISTORY");}cp05_cancel(*s,s->fury?cp05::Cancel::Fury:cp05::Cancel::Priority);return;}
 if(s->native_fresh){
  if(!a->mpMorf||a->mActionMode!=4||(a->mOcState!=1&&a->mOcState!=2)||cp04_bck(a)!=s->fresh_bck||
     !cp04::near(a->mpMorf->getFrame(),1)||!cp04::finite(s->fresh_end)||s->fresh_end<=25||s->fresh_end>300||a->mpMorf->mFrameCtrl.getEnd()!=s->fresh_end){cp05_cancel(*s,cp05::Cancel::Unsafe);return;}
  if(a->mpMorf->getPlaySpeed()!=1.15f){
   if(s->variant==cp05::Variant::Red){++c5.c.special;cp05_log("SPECIAL_PRIORITY_EXCLUDED",s,"NO_NORMAL_WRITE_WITNESS_PENDING_X2_OR_UNKNOWN_CAUSE",a->mpMorf->getFrame(),a->mpMorf->getPlaySpeed());}
   cp05_cancel(*s,cp05::Cancel::Special);return;
  }
  if(s->phase==cp05::Phase::Idle){
   if(s->cancelled_this_execute){cp05_log("NATIVE_START_AFTER_CANCEL_SKIPPED",s,"NO_REROLL_IN_THE_CANCELLED_EXECUTION");return;}
   const auto kind=s->decide(g.p8.tick,true,c5.c);cp05_log("OPPORTUNITY",s,"FRESH_NATIVE_BC_RATE1_THEN_EXACT_POLICY_NORMAL115");
   cp05_log(kind==cp05::Kind::Normal?"NORMAL_SELECTED":kind==cp05::Kind::Red4?"RED4_SELECTED":kind==cp05::Kind::Black8?"BLACK8_SELECTED":"BLACK7_SELECTED",s);
   if(kind==cp05::Kind::Normal)return;
  }
  if(s->phase==cp05::Phase::Selected||s->phase==cp05::Phase::Gap){
   if(!s->begin(cp04_bck(a),a->mpMorf->mFrameCtrl.getEnd(),a->mpMorf->getFrame(),a->mpMorf->getPlaySpeed(),c5.c)||!cp05_native_rate(a,*s)){
    ++c5.c.kind[unsigned(s->kind)].foreign;cp05_cancel(*s,cp05::Cancel::Readback);return;
   }
   cp05_log("SPEED_APPLIED_READBACK",s,"OBSERVED_NORMAL_CAUSE_HANDOFF_NO_SPECIAL_OR_FURY",a->mpMorf->getFrame(),a->mpMorf->getPlaySpeed());
   cp05_log(s->swing==1?"START_SWING":"NEXT_SWING_STARTED",s,"NATIVE_BC_SELECTED_BY_ORIGINAL",a->mpMorf->getFrame(),cp05::rate(s->kind,s->swing));return;
  }
 }
 if(s->phase==cp05::Phase::Swing){
  if(!s->will_finish&&a->mActionMode!=4){cp05_cancel(*s,cp05::Cancel::Priority);return;}
  if(s->tg_pending&&a->health==s->execute_hp&&a->mActionMode==4){++c5.c.kind[unsigned(s->kind)].tg_continued;cp05_log("TG_NATIVE_RETURN_CONTINUED",s,"NO_HP_LOSS_NATIVE_CONTINUATION_REVALIDATED_AFTER_DAMAGE_TRANSACTION");}
  cp05_observe_other(a,*s);
  if(s->will_finish){
   if(!s->at_seen){cp05_cancel(*s,cp05::Cancel::NoWindow);return;}
   if(!a->mpMorf||!a->checkBck(28)||a->mSphs_at[0].cCcD_ObjHitInf::ChkAtSet()||a->mSphs_at[1].cCcD_ObjHitInf::ChkAtSet()||
      !((a->mActionMode==3&&a->mOcState==0)||(a->mActionMode==4&&a->mOcState==0))){cp05_cancel(*s,cp05::Cancel::Priority);return;}
   if(!s->contact_seen&&!s->other_seen)cp05_log("MISS",s,"NO_IDENTIFIED_NATIVE_CONTACT_IN_REGISTERED_WINDOW");
   cp05_log("END_SWING",s,"FULL_NATIVE_CLIP_OR_FULL_NEGATIVE_RECOIL");s->finish(c5.c);cp05_log("SPEED_RELEASED",s,"NATIVE_CLIP_EXIT_ALREADY_RESET_OWNERSHIP");
   if(s->phase==cp05::Phase::Gap){
    if(a->mActionMode!=3&&!cp05_native_mode(a,3)){cp05_cancel(*s,cp05::Cancel::Readback);return;}
    s->gap_timer_expected=a->field_0x6c2;s->gap_timer_known=true;
    cp05_log("GAP_BEGIN",s,"SEGMENT_AT_DEADLINE_NATIVE_TIMER_LEASE_ONLY_DURING_EXECUTE");
   }else cp05_log("COMBO_COMPLETE",s,"NATIVE_RECOVERY_AND_LOCKOUT_UNCHANGED");return;
  }
  const float rate=a->mpMorf?a->mpMorf->getPlaySpeed():0;
  const bool stopped=a->mpMorf&&a->mpMorf->isStop()&&rate==0&&(a->mOcState==3?a->mpMorf->getFrame()<=0:a->mpMorf->getFrame()>=s->end-.01f);
  const float frame=a->mpMorf?a->mpMorf->getFrame():-1;
  const float delta=frame-s->last_frame;
  const bool motion=cp04::finite(frame)&&frame>=0&&frame<=s->end&&a->mpMorf&&a->mpMorf->mFrameCtrl.getEnd()==s->end&&
   (a->mOcState==3?(delta<=.01f&&delta>=-1.01f):(delta>=0&&delta<=cp05::rate(s->kind,s->swing)+.01f));
  if(a->mActionMode!=4||!a->mpMorf||cp04_bck(a)!=s->bck||a->mOcState<1||a->mOcState>3||!motion||(!stopped&&(a->mOcState==3?rate!=-1:rate!=cp05::rate(s->kind,s->swing)))){
   ++c5.c.kind[unsigned(s->kind)].foreign;cp05_log("SPEED_FOREIGN_SKIPPED",s,"OWNER_RELEASED_NO_REWRITE",0,rate);cp05_cancel(*s,cp05::Cancel::Foreign);return;
  }
  const bool at=a->mSphs_at[0].cCcD_ObjHitInf::ChkAtSet()||a->mSphs_at[1].cCcD_ObjHitInf::ChkAtSet();
  if(at&&!s->at_seen){s->arm(c5.c);cp05_log("HITBOX_BEGIN",s,"TWO_SPHERES_ONE_EPISODE_NATIVE_ATSET",a->mpMorf->getFrame(),rate);cp05_log(s->view.pose_valid?"AT_SPHERE_POSE_VALID":"AT_SPHERE_POSE_UNAVAILABLE",s,"READ_ONLY_AFTER_NATIVE_CC_SET_NO_CONTACT_OR_SPARK_CLAIM");}
  if(s->at_live&&!at)cp05_log("HITBOX_END",s,"NATIVE_ATSET_OFF",a->mpMorf->getFrame(),rate);
  s->at_live=at;s->last_frame=a->mpMorf->getFrame();
 }else if(s->phase==cp05::Phase::Gap){
  if(s->view.held_r)++c5.c.kind[unsigned(s->kind)].guard_idle_ticks;
  if(s->launch_requested&&a->mActionMode==4&&a->mOcState==0)return;
  if(a->mActionMode!=3){cp05_cancel(*s,cp05::Cancel::Priority);return;}
  s->gap_timer_expected=a->field_0x6c2;s->gap_timer_known=true;
  if(a->mOcState==4&&s->local_tick+1>=s->launch_at){
   if(a->checkBeforeFloorBg(200.f)){cp05_cancel(*s,cp05::Cancel::Unsafe);return;}
   // Publish the entry AFTER this execute, so Expansion's NEXT PRE observes
   // genuine ATTACK/0 before it records its source-proven fresh-start receipt.
   if(!cp05_native_mode(a,4)){cp05_cancel(*s,cp05::Cancel::Readback);return;}
   s->launch_requested=true;s->launch_tick=s->local_tick+1;cp05_log("NEXT_SWING_REQUESTED",s,"POST_ENTRY_VISIBLE_TO_NEXT_EXPANSION_PRE");
   s->gap_timer_known=false;
  }
 }
}
void cp05_contact(const post08::Input& in,const post08::Verdict& v,bool committed,int before,int after)noexcept{
 if(in.slot<0||unsigned(in.slot)>=cp05::kSlots)return;auto& s=c5.slots[in.slot];
 if(!s.bound||s.key.id!=in.id||s.key.generation!=in.generation)return;
 s.site=cp05::Site::Contact;
 if(s.phase==cp05::Phase::Swing&&v.first_contact&&!s.contact_seen){
  const bool link=in.at_on_link[0]||in.at_on_link[1];s.contact(link,committed,v.blocked,c5.c);
  cp05_log(link?"CONTACT_LINK":"CONTACT_OTHER",&s,committed?"PARRY_CONFIRMED":v.blocked?"GUARD":"UNBLOCKED_CONTACT_DAMAGE_NOT_CONFIRMED",in.anim_frame,in.anim_rate,before,after,v.token,in.pass.serial,v.delta);
  if(link&&v.blocked)cp05_log("NATIVE_SHIELD_BLOCK_FLAG",&s,"MATCHED_NATIVE_SHIELD_FLAG_NOT_VISIBLE_SPARK_CERTIFICATION",in.anim_frame,in.anim_rate,before,after,v.token,in.pass.serial,v.delta);
  if(link&&v.result==post08::Result::Confirmed)cp05_log("PARRY_ELIGIBLE",&s,"ACTUAL_POST08_JUDGE_REQUIRES_TRANSACTION_COMMIT",in.anim_frame,in.anim_rate,before,after,v.token,in.pass.serial,v.delta);
  if(link&&!committed)cp05_log("PARRY_REJECTED",&s,post08::name(v.result),in.anim_frame,in.anim_rate,before,after,v.token,in.pass.serial,v.delta);
  if(committed){cp05_log("PARRY_CONFIRMED",&s,"POSTURE_AND_DEFLECTION_TRANSACTION_COMMITTED",in.anim_frame,in.anim_rate,before,after,v.token,in.pass.serial,v.delta);cp05_log("SHIELD_SERIAL_MATCHED",&s,"CP04_CONFIRMED_REAL_NATIVE_ACTION",in.anim_frame,in.anim_rate,before,after,v.token,in.pass.serial,v.delta);}
  else if(v.blocked)cp05_log("GUARD",&s,"NATIVE_SHIELD_FLAG_NOT_A_PARRY",in.anim_frame,in.anim_rate,before,after,v.token,in.pass.serial,v.delta);
 }
 if(after>=100)cp05_cancel(s,cp05::Cancel::Posture100);
}
void cp05_report(const char* reason)noexcept{
 for(unsigned i=0;i<2;++i){const auto& d=c5.c.distribution[i];char line[kLogLineMax*3];
  std::snprintf(line,sizeof(line),"CP05_SUMMARY part=DISTRIBUTION version=0.11.11 source=%s variant=%s opportunities=%llu normal=%llu combo1=%llu combo2=%llu draws=%llu rejections=%llu fury_excluded=%llu non_normal_or_unsupported_excluded=%llu duplicates=%llu reentered=%llu overflow_observations=%llu untracked_observations=%llu errors=%llu detail_lines=%u detail_dropped=%llu reason=%s runtime=HUMAN_RUNTIME_PENDING",
   POST05_SOURCE_COMMIT,i?"BLACK_NORMAL":"RED_NORMAL",(unsigned long long)d.opportunities,(unsigned long long)d.normal,(unsigned long long)d.combo1,(unsigned long long)d.combo2,(unsigned long long)d.draws,(unsigned long long)d.rejections,(unsigned long long)c5.c.fury,(unsigned long long)c5.c.special,(unsigned long long)c5.c.duplicate,(unsigned long long)c5.c.reentered,(unsigned long long)c5.c.overflow,(unsigned long long)c5.c.untracked,(unsigned long long)c5.c.errors,c5.lines,(unsigned long long)c5.dropped,reason);lnk_write(LOG_LEVEL_INFO,line);
 }
 for(unsigned k=1;k<4;++k){const auto& s=c5.c.kind[k];char line[kLogLineMax*3];
  std::snprintf(line,sizeof(line),"CP05_SUMMARY part=KIND version=0.11.11 source=%s kind=%s selected=%llu not_started=%llu completed=%llu cancelled=%llu breaks=%llu rates=%llu foreign=%llu returned=%llu duration_floor_extensions=%llu legitimate_cancellations=%llu unexpected_cancellations=%llu deferred_tg_observations=%llu native_return_continued=%llu held_R_gap_ticks=%llu lease_acquired=%llu lease_restored=%llu lease_foreign=%llu lease_native_mode_changed=%llu reason=%s runtime=HUMAN_RUNTIME_PENDING",
   POST05_SOURCE_COMMIT,cp05::name(cp05::Kind(k)),(unsigned long long)s.selected,(unsigned long long)s.not_started,(unsigned long long)s.completed,(unsigned long long)s.cancelled,(unsigned long long)s.breaks,(unsigned long long)s.rates,(unsigned long long)s.foreign,(unsigned long long)s.returned,(unsigned long long)s.extended,(unsigned long long)s.legitimate,(unsigned long long)s.unexpected,(unsigned long long)s.tg_deferred,(unsigned long long)s.tg_continued,(unsigned long long)s.guard_idle_ticks,(unsigned long long)s.lease_acquired,(unsigned long long)s.lease_restored,(unsigned long long)s.lease_foreign,(unsigned long long)s.lease_native_mode,reason);lnk_write(LOG_LEVEL_INFO,line);
  for(unsigned n=0;n<cp05::count(cp05::Kind(k));++n){const cp05::Range zero{};const bool next=n+1<cp05::count(cp05::Kind(k));const auto& at=next?s.at[n]:zero;const auto& ct=next?s.contact[n]:zero;const auto& gap=next?s.gap[n]:zero;
   std::snprintf(line,sizeof(line),"CP05_SUMMARY part=POSITION version=0.11.11 source=%s kind=%s swing=%u starts=%llu armed=%llu link_contacts=%llu other_contacts=%llu parries=%llu guards=%llu unblocked_contacts_damage_unconfirmed=%llu misses=%llu next_at_n=%llu next_at_min=%llu next_at_max=%llu next_at_sum=%llu next_at_avg=%.3f next_contact_n=%llu next_contact_min=%llu next_contact_max=%llu next_contact_sum=%llu next_contact_avg=%.3f gap_n=%llu gap_min=%llu gap_max=%llu gap_sum=%llu gap_avg=%.3f reason=%s",
    POST05_SOURCE_COMMIT,cp05::name(cp05::Kind(k)),n+1,(unsigned long long)s.starts[n],(unsigned long long)s.armed[n],(unsigned long long)s.contacts[n],(unsigned long long)s.other[n],(unsigned long long)s.parries[n],(unsigned long long)s.guard[n],(unsigned long long)s.unblocked[n],(unsigned long long)s.miss[n],
    (unsigned long long)(n+1<cp05::count(cp05::Kind(k))?at.n:0),(unsigned long long)at.min,(unsigned long long)at.max,(unsigned long long)at.sum,at.n?double(at.sum)/double(at.n):0,
    (unsigned long long)(n+1<cp05::count(cp05::Kind(k))?ct.n:0),(unsigned long long)ct.min,(unsigned long long)ct.max,(unsigned long long)ct.sum,ct.n?double(ct.sum)/double(ct.n):0,
    (unsigned long long)(n+1<cp05::count(cp05::Kind(k))?gap.n:0),(unsigned long long)gap.min,(unsigned long long)gap.max,(unsigned long long)gap.sum,gap.n?double(gap.sum)/double(gap.n):0,reason);lnk_write(LOG_LEVEL_INFO,line);
  }
  for(unsigned n=0;n+1<cp05::count(cp05::Kind(k));++n){const cp05::Range* ranges[]={&s.at[n],&s.contact[n],&s.gap[n]};const char* labels[]={"AT","CONTACT","POST_CLIP_GAP"};for(unsigned q=0;q<3;++q){const auto& h=*ranges[q];
   std::snprintf(line,sizeof(line),"CP05_SUMMARY part=INTERVAL_HIST version=0.11.11 source=%s kind=%s boundary=%u-%u measure=%s bins=lt32/32_39/40_47/48_55/56_63/64_75/76_119/ge120 counts=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu n=%llu reason=%s",POST05_SOURCE_COMMIT,cp05::name(cp05::Kind(k)),n+1,n+2,labels[q],(unsigned long long)h.hist[0],(unsigned long long)h.hist[1],(unsigned long long)h.hist[2],(unsigned long long)h.hist[3],(unsigned long long)h.hist[4],(unsigned long long)h.hist[5],(unsigned long long)h.hist[6],(unsigned long long)h.hist[7],(unsigned long long)h.n,reason);lnk_write(LOG_LEVEL_INFO,line);
  }}
  for(unsigned n=0;n<unsigned(cp05::Detail::Count);++n)if(s.detail[n]){std::snprintf(line,sizeof(line),"CP05_SUMMARY part=CANCEL_DETAIL version=0.11.11 source=%s kind=%s subreason=%s count=%llu reason=%s",POST05_SOURCE_COMMIT,cp05::name(cp05::Kind(k)),cp05::name(cp05::Detail(n)),(unsigned long long)s.detail[n],reason);lnk_write(LOG_LEVEL_INFO,line);}
  for(unsigned r=0;r<unsigned(cp05::Cancel::Count);++r){if(!s.cancel[r])continue;std::snprintf(line,sizeof(line),"CP05_SUMMARY part=CANCEL version=0.11.11 source=%s kind=%s cancel_reason=%s count=%llu reason=%s",POST05_SOURCE_COMMIT,cp05::name(cp05::Kind(k)),cp05::name(cp05::Cancel(r)),(unsigned long long)s.cancel[r],reason);lnk_write(LOG_LEVEL_INFO,line);}
 }
}
void cp05_maintenance()noexcept{
 if(!c5.enabled)return;
 for(unsigned i=0;i<cp05::kSlots;++i){auto& s=c5.slots[i];if(!s.bound)continue;const auto& l=g.eng.obs.ledger.slot(i);
  if(l.id!=s.key.id||l.generation!=s.key.generation||l.life!=Life::Active||l.epoch_last!=s.key.epoch||stage_token(dComIfGp_getStartStageName())!=s.key.stage){s.site=cp05::Site::Maintenance;cp05_cancel(s,cp05::Cancel::Context);s.bound=false;}
  else if(!cp04_sim_safe())cp05_cancel(s,cp05::Cancel::Pause);
 }
 if(g.p8.tick<c5.last_summary_tick)c5.last_summary_tick=g.p8.tick;
 if(g.p8.tick-c5.last_summary_tick>=1800){cp05_report("PERIODIC");c5.last_summary_tick=g.p8.tick;}
}
void cp05_summary()noexcept{c5.enabled=false;for(auto& s:c5.slots){s.site=cp05::Site::Shutdown;cp05_cancel(s,cp05::Cancel::Shutdown);}cp05_report("SHUTDOWN");}
bool cp05_install()noexcept{
 HookOptions late=HOOK_OPTIONS_INIT,early=HOOK_OPTIONS_INIT;late.priority=-200;early.priority=100;
 const ModResult r[]={mods::hook::add_pre<Cp04EocExecute>(on_eoc_pre_cp05,&late),mods::hook::add_pre<AttackObserveFix01>(on_attack_pre_cp05,&late),
  mods::hook::add_post<Cp04EocExecute>(on_eoc_early_post_cp05,&early),mods::hook::add_post<Cp04EocExecute>(on_eoc_post_cp05,&late)};
 bool ok=c4.enabled;for(auto x:r)ok=ok&&x==MOD_OK;c5.enabled=ok;cp05_log("INIT",nullptr,ok?"EOC2_RED_BLACK_NORMAL_ON_BLUE_SHIELD_UNCHANGED":"HOOK_FAILURE_NO_PARTIAL_CHECKPOINT");return ok;
}
#undef CP05_NOINLINE
