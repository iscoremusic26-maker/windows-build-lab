#if defined(_MSC_VER)
#define CP04_NOINLINE __declspec(noinline)
#else
#define CP04_NOINLINE __attribute__((noinline))
#endif
// CP04 production adapter. Included once inside mod.cpp's anonymous namespace.
// This ENTIRE file, each native writer and the integration deltas are audited
// by ci/cp04_audit.py. It is not an exemption from the legacy write barrier.
struct Cp04Runtime {
 bool enabled=false;cp04::Combo slots[cp04::kSlots]{};cp04::Counters c{};shieldchain::Chain shield;
 bool reentered_call=false;std::uint64_t motion_logged=0,last_summary_tick=0;unsigned depth=0,lines=0;std::uint64_t dropped=0,reentered=0,source_errors=0;
} c4;
void cp04_log(const char* event,const cp04::Combo* s=nullptr,const char* reason="OK",float frame=0,float rate=0,int detail=0,int pb=-1,int pa=-1,std::uint64_t event_token=0,std::uint64_t post08_serial=0)noexcept{
 if(c4.lines>=640){++c4.dropped;return;}++c4.lines;
 char line[kLogLineMax*3];
 std::snprintf(line,sizeof(line),"CP04_%s version=0.11.7 source=%s actor=%u generation=%llu epoch=%llu stage=%u room=%d layer=%d tick=%llu local_tick=%llu opportunity_id=%llu combo_id=%llu swing_index=%u swing_token=%llu seed=%llu bck=%d clip_end=%.2f frame=%.2f rate=%.3f arm_eval_frame=%.2f incoming_group_hint=%u origin=%u posture_before=%d posture_after=%d enemy_episode_token=%llu post08_sa_serial=%llu sa_serial=%llu input_serial=%llu detail=%d reason=%s evidence=NATIVE_SOURCE_READBACK visual=HUMAN_PENDING",
 event,POST05_SOURCE_COMMIT,s?s->key.id:c4.shield.id,(unsigned long long)(s?s->key.generation:0),(unsigned long long)(s?s->key.epoch:c4.shield.epoch),s?s->key.stage:c4.shield.stage,s?s->key.room:-1,s?s->key.layer:-1,
 (unsigned long long)g.p8.tick,(unsigned long long)(s?s->local_tick:0),(unsigned long long)(s?s->opportunity:0),(unsigned long long)(s?s->combo_id:0),s?s->swing:0,
 (unsigned long long)(s?s->token:0),(unsigned long long)(s?s->seed:0),s?s->bck:0,s?s->end:0.,frame,rate,s?s->attack_frame:0.,s?s->group_hint:0,s?s->origin:0,pb,pa,(unsigned long long)event_token,(unsigned long long)post08_serial,(unsigned long long)c4.shield.pass_serial,(unsigned long long)c4.shield.input_serial,detail,reason);
 lnk_write(LOG_LEVEL_INFO,line);
}
bool cp04_world_safe()noexcept{
 return c4.enabled&&!g.eng.obs.disabled&&
  !g_dComIfG_gameInfo.play.isPauseFlag()&&dMeter2Info_getWindowStatus()==0&&
  !g_dComIfG_gameInfo.play.getEvent()->runCheck()&&
  g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife()>0;
}
bool cp04_sim_safe()noexcept{return cp04_world_safe()&&!g.rx.session_disabled()&&g.layout==1;}
int cp04_slot(daE_OC_c* a)noexcept{
 if(!a||a->profname!=fpcNm_E_OC_e||a->state.init_state==3)return -1;
 const auto id=static_cast<std::uint32_t>(fopAcM_GetID(a));
 for(unsigned i=0;i<cp04::kSlots;++i){const auto& l=g.eng.obs.ledger.slot(i);
  if(l.id==id&&l.ptr==reinterpret_cast<std::uintptr_t>(a)&&l.life==Life::Active&&is_candidate(l.adm))return int(i);
 }return -1;
}
cp04::Combo* cp04_actor(daE_OC_c* a)noexcept{
 const int i=cp04_slot(a);if(i<0)return nullptr;
 const auto& l=g.eng.obs.ledger.slot(unsigned(i));
 const auto& p=g.pe.px.entry(unsigned(i));
 const auto profile=postbal01::capture(g.eng.obs.ledger,lg.posture.variants,i,l.id,l.generation,read_aux(a));
 if(!profile.verified||profile.variant!=postbal01::Variant::Blue)return nullptr;
 cp04::Key k;k.id=l.id;k.generation=l.generation;k.epoch=l.epoch_last;k.stage=stage_token(dComIfGp_getStartStageName());
 k.room=fopAcM_GetRoomNo(a);k.layer=dComIfG_play_c::getLayerNo(0);
 auto& s=c4.slots[i];
 if(!s.bound||!(s.key==k)){if(s.active()){s.cancel(c4.c);++c4.c.context;cp04_log("COMBO_CANCEL",&s,"ACTOR_CONTEXT");}s.bind(k);}
 if(!p.bound||p.id!=l.id||p.generation!=l.generation||p.frozen||p.dead||p.latched||a->health<=1){
  if(s.active()){const bool broken=p.latched;s.cancel(c4.c,broken);cp04_log("COMBO_CANCEL",&s,broken?"POSTURE_100":"LETHAL_OR_UNVERIFIED");}return nullptr;
 }
 s.origin=profile.method;return &s;
}
void cp04_cancel(cp04::Combo& s,const char* why)noexcept{
 if(s.active()){cp04_log(s.phase==cp04::Phase::Selected?"SELECTED_NOT_STARTED":"COMBO_CANCEL",&s,why);s.cancel(c4.c);}
}
int cp04_bck(daE_OC_c* a)noexcept{
 return a&&a->mpMorf?(a->checkBck(5)?5:a->checkBck(6)?6:0):0;
}
// Native mutation 1: one fresh B/C clip only, AFTER Expansion's priority0 POST.
// Accept only its documented 1/1.15/1.30 hand-off; never fight a foreign speed.
CP04_NOINLINE bool cp04_native_rate(daE_OC_c* a,cp04::Combo& s)noexcept{
 if(!a->mpMorf||a->mActionMode!=4||(a->mOcState!=1&&a->mOcState!=2)||
    cp04_bck(a)!=s.bck||!cp04::near(a->mpMorf->getFrame(),1.f)||
    !cp04::normal_rate(a->mpMorf->getPlaySpeed())||s.rate_owned)return false;
 a->mpMorf->setPlaySpeed(s.rate);
 const bool ok=a->mpMorf->getPlaySpeed()==s.rate;
 if(ok){s.rate_owned=true;++c4.c.rates;}return ok;
}
// Native mutation 2: typed action transition, never a mode/state/old restore.
// Caller must hold a selected, live, unbroken Blue at a native clip exit or a
// completed gap. Original execute/action (damage, water, fall, recruitment) runs.
CP04_NOINLINE bool cp04_native_mode(daE_OC_c* a,int mode)noexcept{
 a->setActionMode(mode,0);return a->mActionMode==mode&&a->mOcState==0;
}
// Native mutation 3: own gap only; Find/0 may initialize native20 exactly once.
CP04_NOINLINE bool cp04_native_gap(daE_OC_c* a,unsigned ticks)noexcept{
 if(a->mActionMode!=3||ticks>cp04::kModerateGap)return false;
 a->field_0x6c2=static_cast<s16>(ticks);return a->field_0x6c2==static_cast<s16>(ticks);
}
HookAction on_eoc_pre_cp04(ModContext*,void* args,void*,void*)noexcept{
 if(!c4.enabled||!args)return HOOK_CONTINUE;
 if(c4.depth++){c4.reentered_call=true;++c4.reentered;return HOOK_CONTINUE;}
 c4.reentered_call=false;
 auto* a=mods::arg<daE_OC_c*>(args,0);auto* s=cp04_actor(a);if(!s)return HOOK_CONTINUE;
 if(!cp04_sim_safe()){++c4.c.paused;cp04_cancel(*s,"NATIVE_PRIORITY_OR_PAUSE");return HOOK_CONTINUE;}
 if(!s->execute(g.p8.tick))return HOOK_CONTINUE;
 s->attack_entered=s->will_finish=false;
 if(s->phase==cp04::Phase::Recovery){s->phase=cp04::Phase::Idle;return HOOK_CONTINUE;}
 if(s->phase==cp04::Phase::Gap){
  if(s->launch_requested){
   if(a->mActionMode!=4||a->mOcState!=0||a->field_0x6e3||s->local_tick-s->launch_tick>cp04::kWindupMax)cp04_cancel(*s,"NATIVE_WINDUP_PRIORITY_OR_TIMEOUT");
   return HOOK_CONTINUE;
  }
  if(a->mActionMode!=3||a->field_0x6e3){cp04_cancel(*s,"NATIVE_PRIORITY");return HOOK_CONTINUE;}
  if(!s->remaining()){
   if(a->checkBeforeFloorBg(200.f)){cp04_cancel(*s,"UNSAFE_TERRAIN");return HOOK_CONTINUE;}
   if(!cp04_native_mode(a,4)){cp04_cancel(*s,"MODE_READBACK");return HOOK_CONTINUE;}
   s->launch_requested=true;s->launch_tick=s->local_tick;
   cp04_log("NEXT_SWING_REQUESTED",s,"NATIVE_CHASEF_MAY_STILL_NEED_BRAKING",0,a->speedF,int(s->remaining()));
  }
 }else if(s->active()&&(a->mActionMode!=4||a->field_0x6e3))cp04_cancel(*s,"NATIVE_PRIORITY");
 return HOOK_CONTINUE;
}
HookAction on_attack_pre_cp04(ModContext*,void* args,void*,void*)noexcept{
 if(!c4.enabled||!args||c4.depth!=1)return HOOK_CONTINUE;
 auto* a=mods::arg<daE_OC_c*>(args,0);auto* s=cp04_actor(a);if(!s)return HOOK_CONTINUE;
 // A newly admitted actor may first acquire its sidecar inside damage_check.
 (void)s->execute(g.p8.tick);
 if(!cp04_sim_safe()||a->mActionMode!=4||a->field_0x6e3){cp04_cancel(*s,"NATIVE_PRIORITY");return HOOK_CONTINUE;}
 s->attack_entered=true;s->attack_frame=a->mpMorf?a->mpMorf->getFrame():0.f;
 s->will_finish=s->phase==cp04::Phase::Swing&&a->mOcState>=1&&a->mOcState<=3&&a->mpMorf&&a->mpMorf->isStop();
 if(a->mOcState==0&&cp04::finite(a->speedF)&&cp04::absf(a->speedF)<=1.f){
  if(s->phase==cp04::Phase::Idle){
   // Native ATTACK/0 has already survived action's damage/water/fall gates.
   // Never count waiting chaseF ticks or recruiter WATCH3 as opportunities.
   if(a->checkBeforeFloorBg(200.f))return HOOK_CONTINUE;
   const auto before=s->opportunity;(void)s->decide(g.p8.tick,true,c4.c);
   if(s->opportunity!=before){cp04_log("BLUE_OPPORTUNITY",s,"NATIVE_ATTACK0_CHASE_REACHES_ZERO");cp04_log(s->phase==cp04::Phase::Selected?"COMBO_SELECTED":"NORMAL_SELECTED",s);}
  }
  if(s->phase==cp04::Phase::Selected||s->phase==cp04::Phase::Gap)s->start_pending=true;
 }return HOOK_CONTINUE;
}
void on_eoc_post_cp04(ModContext*,void* args,void*,void*)noexcept{
 if(!c4.enabled||!args)return;
 if(!c4.depth){++c4.source_errors;return;}if(--c4.depth)return;
 auto* a=mods::arg<daE_OC_c*>(args,0);auto* s=cp04_actor(a);if(!s||!s->active())return;
 if(c4.reentered_call||!cp04_sim_safe()||a->field_0x6e3){cp04_cancel(*s,"NATIVE_PRIORITY");return;}
 if(s->start_pending&&s->attack_entered){
  const int clip=cp04_bck(a);
  if(a->mActionMode!=4||(a->mOcState!=1&&a->mOcState!=2)||!a->mpMorf||
     !s->begin(clip,a->mpMorf->mFrameCtrl.getEnd(),a->mpMorf->getFrame(),a->mpMorf->getPlaySpeed(),c4.c)){
   if(a->mpMorf&&!cp04::normal_rate(a->mpMorf->getPlaySpeed())){++c4.c.foreign;cp04_log("SPEED_SKIPPED_FOREIGN",s);}
   cp04_cancel(*s,"SELECTED_NATIVE_START_UNAVAILABLE");return;
  }
  if(!cp04_native_rate(a,*s)){++c4.c.foreign;cp04_cancel(*s,"SPEED_READBACK");return;}
  cp04_log("SPEED_APPLIED_READBACK",s,"FRESH_CLIP_HANDOFF_PRIORITY_MINUS100",a->mpMorf->getFrame(),a->mpMorf->getPlaySpeed());
  cp04_log(s->swing==1?"SWING_START":"NEXT_SWING_STARTED",s,"NATIVE_BC",a->mpMorf->getFrame(),s->rate,int(s->local_tick-s->gap_tick));
  return;
 }
 if(s->phase==cp04::Phase::Swing){
  if(s->will_finish){
   // The ORIGINAL has selected idle BCK28 and released the weapon window.
   if(!a->mpMorf||!a->checkBck(28)||a->mSphs_at[0].cCcD_ObjHitInf::ChkAtSet()||a->mSphs_at[1].cCcD_ObjHitInf::ChkAtSet()||
      !((a->mActionMode==3&&a->mOcState==0)||(a->mActionMode==4&&a->mOcState==0))){cp04_cancel(*s,"EXIT_NOT_OWNED");return;}
   cp04_log("SWING_END",s,"FULL_NATIVE_CLIP_OR_SHIELD_RECOIL");s->finish(c4.c);cp04_log("SPEED_RELEASED",s,"NATIVE_IDLE_RESET");
   if(a->mActionMode!=3&&!cp04_native_mode(a,3)){cp04_cancel(*s,"EXIT_MODE_READBACK");return;}
   if(s->phase==cp04::Phase::Recovery){cp04_log("COMBO_COMPLETE",s,"NATIVE_FIND_RECOVERY20_SCALED_BY_EXPANSION");return;}
   if(!cp04_native_gap(a,s->remaining())){cp04_cancel(*s,"GAP_READBACK");return;}
   cp04_log("PAUSE_BEGIN",s,"SIMULATION_EXECUTES_ONLY",0,0,int(s->gap));return;
  }
  const int clip=cp04_bck(a);const float rate=a->mpMorf?a->mpMorf->getPlaySpeed():0;
  const bool native_stop=a->mpMorf&&a->mpMorf->isStop()&&cp04::near(rate,0.f)&&
   (a->mOcState==3?a->mpMorf->getFrame()<=0.f:a->mpMorf->getFrame()>=s->end-.01f);
  if(a->mActionMode!=4||clip!=s->bck||(a->mOcState!=1&&a->mOcState!=2&&a->mOcState!=3)||
     (!native_stop&&(a->mOcState==3?rate!=-1.f:rate!=s->rate))){cp04_cancel(*s,"UNSAFE_ANIMATION_OR_FOREIGN_OWNER");return;}
  const bool at=a->mSphs_at[0].cCcD_ObjHitInf::ChkAtSet()||a->mSphs_at[1].cCcD_ObjHitInf::ChkAtSet();
  if(at&&!s->at_seen){s->at_seen=true;++c4.c.armed[s->swing-1];
   if(s->swing>=2&&s->swing<=4&&s->previous_arm_swing+1==s->swing){unsigned i=s->swing-2;auto delta=s->local_tick-s->previous_arm_tick;++c4.c.at_n[i];if(c4.c.at_n[i]==1||delta<c4.c.at_min[i])c4.c.at_min[i]=delta;if(delta>c4.c.at_max[i])c4.c.at_max[i]=delta;}
   s->previous_arm_tick=s->local_tick;s->previous_arm_swing=s->swing;cp04_log("SWING_HITBOX_BEGIN",s,"ACTUAL_NATIVE_ATSET",a->mpMorf->getFrame(),rate);}
  if(s->at_live&&!at)cp04_log("SWING_HITBOX_END",s,"NATIVE_ATSET_OFF",a->mpMorf->getFrame(),rate);
  s->at_live=at;s->last_frame=a->mpMorf->getFrame();
 }else if(s->phase==cp04::Phase::Gap){
  if(s->launch_requested&&a->mActionMode==4&&a->mOcState==0)return; // original chaseF has not reached zero yet; never fake a B/C start
  if(a->mActionMode!=3){cp04_cancel(*s,"NATIVE_PRIORITY_IN_GAP");return;}
  if(!s->gap_initialized){
   // Original Find/0 just initialized20; Expansion may have made it17/15.
   if(a->field_0x6c2!=20&&a->field_0x6c2!=17&&a->field_0x6c2!=15){cp04_cancel(*s,"FOREIGN_GAP_TIMER");return;}
   if(!cp04_native_gap(a,s->remaining())){cp04_cancel(*s,"GAP_READBACK");return;}s->gap_initialized=true;
  }
 }else if(s->phase==cp04::Phase::Selected)cp04_cancel(*s,"SELECTED_NOT_STARTED");
}
bool cp04_link_safe(daAlink_c* link)noexcept{
 std::uint32_t id=0;
 return cp04_world_safe()&&lf.enabled&&lnkfx_link(link,id)&&link->mShieldModel&&link->checkShieldDraw()&&
  link->mDamageTimer==0&&lf.def.phase()==lnkfx::Phase::Ready&&!lb.m.hold()&&
  (link->mModeFlg&(lnk05::policy::kModeBlockedMask|lnk05::policy::kModeSpecialMask))==0;
}
void on_stick_post_cp04(ModContext*,void* args,void*,void*)noexcept{
 if(!c4.enabled||!args)return;auto* link=mods::arg<daAlink_c*>(args,0);std::uint32_t id=0;
 if(!lnkfx_link(link,id)||!lf.sample_ok||lf.sample_tick!=g.p8.tick||lf.key!=reinterpret_cast<std::uintptr_t>(link)){c4.shield.cancel();return;}
 c4.shield.context(id,lg.posture.epoch(),stage_token(dComIfGp_getStartStageName()));
 const auto n=c4.shield.c.input_new,b=c4.shield.c.buffered;
 // The frozen LNK callback already sampled the REAL edge before consuming B
 // and processed R release/rearm. Reading its stamped own snapshot avoids
 // losing a simultaneous R+B on the very tick that defence becomes Ready.
 const int proc=link->mProcID;const bool origin=(proc>=3&&proc<=9)||proc==0x1b||proc==daAlink_c::PROC_GUARD_ATTACK||proc==daAlink_c::PROC_GUARD_SLIP;
 if(!origin||link->mEquipItem!=0x103)c4.shield.cancel();
 c4.shield.sample(g.p8.tick,lf.r_held,lf.b_trig,cp04_link_safe(link));
 if(c4.shield.c.input_new!=n)cp04_log("INPUT_NEW",nullptr,"OWN_LNK_SNAPSHOT_PRECONSUMPTION_AFTER_REARM");
 if(c4.shield.c.buffered!=b)cp04_log("BUFFERED",nullptr,"NEW_PRESS_AFTER_CONFIRMED_CONTACT");
}
void on_guard_init_post_cp04(ModContext*,void* args,void*,void*)noexcept{
 if(!c4.enabled||!args)return;auto* link=mods::arg<daAlink_c*>(args,0);
 const bool safe=cp04_link_safe(link);
 if(safe)c4.shield.context(static_cast<std::uint32_t>(fopAcM_GetID(link)),lg.posture.epoch(),stage_token(dComIfGp_getStartStageName()));
 const bool clip=safe&&link->mProcID==daAlink_c::PROC_GUARD_ATTACK&&link->checkUnderMove0BckNoArc(daAlink_c::ANM_SHIELD_ATTACK)&&
  link->mUnderFrameCtrl[0].getRate()==1.f&&link->field_0x3478==1.f&&link->field_0x347c==12.f&&
  link->mUnderFrameCtrl[0].getFrame()>=0.f&&link->mUnderFrameCtrl[0].getFrame()<=1.f;
 if(c4.shield.native_start(g.p8.tick,clip?link->mUnderFrameCtrl[0].getFrame():0.f,clip,safe)){
  g.link_track.native_start();cp04_log("NATIVE_ANIMATION_STARTED",nullptr,"REAL_PROC_INIT_NATIVE_ANM_SHIELD_ATTACK",link->mUnderFrameCtrl[0].getFrame(),1.f);
 }
}
void cp04_observe_motion(daAlink_c* link)noexcept{
 if(!c4.enabled)return;
 bool safe=cp04_link_safe(link);
 if(safe)c4.shield.context(static_cast<std::uint32_t>(fopAcM_GetID(link)),lg.posture.epoch(),stage_token(dComIfGp_getStartStageName()));
 const bool clip=safe&&link->mProcID==daAlink_c::PROC_GUARD_ATTACK&&link->checkUnderMove0BckNoArc(daAlink_c::ANM_SHIELD_ATTACK);
 c4.shield.observe(g.p8.tick,clip,clip?link->mUnderFrameCtrl[0].getFrame():0.f,clip?link->mUnderFrameCtrl[0].getRate():0.f,safe);
 if(c4.shield.pass_valid&&c4.motion_logged!=c4.shield.pass_serial){c4.motion_logged=c4.shield.pass_serial;cp04_log("VISIBLE_CLIP_ACTIVE",nullptr,"NATIVE_CLIP_PROVENANCE_NOT_VISUAL_CERTIFICATION",link->mUnderFrameCtrl[0].getFrame(),link->mUnderFrameCtrl[0].getRate());}
}
bool cp04_guard_pass()noexcept{return !c4.enabled||c4.shield.permits(g.p8.tick);}
// Native mutation 4: the entire native init (clip/voice/shield collider reset),
// not a proc flag, frame reset or a virtual parry. All damage checks ran first.
CP04_NOINLINE bool cp04_native_shield_restart(daAlink_c* link)noexcept{
 const auto serial=c4.shield.native_serial,starts=c4.shield.c.native_starts;
 (void)link->procGuardAttackInit();
 return c4.shield.native_serial==serial+1&&c4.shield.c.native_starts==starts+1&&link->mProcID==daAlink_c::PROC_GUARD_ATTACK&&
  link->checkUnderMove0BckNoArc(daAlink_c::ANM_SHIELD_ATTACK);
}
void on_link_damage_post_cp04(ModContext*,void* args,void*,void*)noexcept{
 if(!c4.enabled||!args)return;auto* link=mods::arg<daAlink_c*>(args,0);
 const bool safe=cp04_link_safe(link)&&link->mEquipItem==0x103;if(!safe){c4.shield.cancel();return;}
 const int proc=link->mProcID;const float frame=link->mUnderFrameCtrl[0].getFrame();
 // Never cut the offensive shield arc. A shield-slip additionally finishes
 // its native body interpolation and braking before its recovery can rearm.
 const bool attack_recovery=proc==daAlink_c::PROC_GUARD_ATTACK&&link->checkUnderMove0BckNoArc(daAlink_c::ANM_SHIELD_ATTACK)&&frame>=link->field_0x347c;
 const bool slip_recovery=proc==daAlink_c::PROC_GUARD_SLIP&&link->mProcVar1.field_0x300a==0&&
  cp04::absf(link->mNormalSpeed)<=2.5f&&g.p8.tick>=c4.shield.permit_tick+12;
 const bool free_recovery=((proc>=3&&proc<=9)||proc==0x1b)&&
  g.p8.tick>=c4.shield.permit_tick+12;
 if(c4.shield.can_restart(g.p8.tick,safe,attack_recovery||slip_recovery||free_recovery)){
  if(cp04_native_shield_restart(link)){++c4.shield.c.rearms;cp04_log("SHIELD_CHAIN_ARMED",nullptr,"NATIVE_RESTART_READBACK");}
  else{++c4.shield.c.failed_native;c4.shield.cancel();cp04_log("CHAIN_CANCELLED",nullptr,"NATIVE_ANIMATION_START_FAILED");}
 }
}
void cp04_contact(const post08::Input& in,const post08::Verdict& v,bool committed,int before,int after)noexcept{
 if(!c4.enabled)return;
 if(committed){if(!c4.shield.confirm(g.p8.tick)){++c4.source_errors;return;}cp04_log("PARRY_MATCHED",nullptr,"REAL_FRESH_BLOCKED_CONTACT_COMMITTED",in.pass.frame,in.anim_rate,after,before,after,v.token,in.pass.serial);}
 if(!committed&&v.first_contact&&c4.shield.pass_valid)c4.shield.cancel();
 if(in.slot<0||unsigned(in.slot)>=cp04::kSlots)return;auto& s=c4.slots[in.slot];
 if(!s.bound||s.key.id!=in.id||s.key.generation!=in.generation||s.phase!=cp04::Phase::Swing)return;
 if(v.first_contact&&!s.contact_seen){s.contact_seen=true;s.contact_tick=s.local_tick;++c4.c.contacts[s.swing-1];
  if(committed)++c4.c.parries[s.swing-1];else if(v.blocked)++c4.c.blocked;else ++c4.c.damaged;
  cp04_log("SWING_CONTACT",&s,committed?"PARRY_CONFIRMED":v.blocked?"GUARD":"DAMAGE",in.anim_frame,in.anim_rate,after-before,before,after,v.token,in.pass.serial);
 }
 if(after>=100){s.cancel(c4.c,true);cp04_log("COMBO_CANCEL",&s,"POSTURE_100_NO_NEXT_SWING");}
}
void cp04_report_summary(const char* reason)noexcept;
void cp04_maintenance()noexcept{
 if(!c4.enabled)return;
 for(unsigned i=0;i<cp04::kSlots;++i){auto& s=c4.slots[i];if(!s.bound)continue;const auto& l=g.eng.obs.ledger.slot(i);
  if(l.id!=s.key.id||l.generation!=s.key.generation||l.life!=Life::Active||l.epoch_last!=s.key.epoch||
     stage_token(dComIfGp_getStartStageName())!=s.key.stage){cp04_cancel(s,"ACTOR_CONTEXT_OR_DESPAWN");s.bound=false;}
 }
 if(!cp04_sim_safe()){c4.shield.cancel();for(auto& s:c4.slots)cp04_cancel(s,"PAUSE_OR_EVENT");}
 if(g.p8.tick<c4.last_summary_tick)c4.last_summary_tick=g.p8.tick;
 if(g.p8.tick-c4.last_summary_tick>=1800){cp04_report_summary("PERIODIC");c4.last_summary_tick=g.p8.tick;}
}
void cp04_report_summary(const char* reason)noexcept{
 const auto& c=c4.c;const auto& l=c4.shield.c;char line[kLogLineMax*8];
 std::snprintf(line,sizeof(line),"CP04_SUMMARY source=%s opportunities=%llu selected=%llu normal=%llu selected_not_started=%llu completed=%llu cancelled=%llu breaks=%llu starts=%llu/%llu/%llu/%llu armed=%llu/%llu/%llu/%llu contacts=%llu/%llu/%llu/%llu parries=%llu/%llu/%llu/%llu missed=%llu blocked=%llu damaged=%llu rate_readbacks=%llu foreign=%llu released=%llu duplicate=%llu context=%llu paused=%llu gap_samples=%llu/%llu/%llu gap_min=%llu/%llu/%llu gap_max=%llu/%llu/%llu at_interval_samples=%llu/%llu/%llu at_interval_min=%llu/%llu/%llu at_interval_max=%llu/%llu/%llu input_new=%llu buffered=%llu native_starts=%llu untrusted=%llu clip_motion=%llu matched=%llu rearms=%llu chain_duplicates=%llu motion_rejected=%llu expired=%llu chain_cancelled=%llu native_failed=%llu errors=%llu reentered=%llu log_lines=%u log_dropped=%llu summary_reason=%s runtime=HUMAN_PENDING visual=VISUAL_SHIELD_MOTION_HUMAN_PENDING",
 POST05_SOURCE_COMMIT,(unsigned long long)c.opportunities,(unsigned long long)c.selected,(unsigned long long)c.normal,(unsigned long long)c.not_started,(unsigned long long)c.completed,(unsigned long long)c.cancelled,(unsigned long long)c.breaks,
 (unsigned long long)c.starts[0],(unsigned long long)c.starts[1],(unsigned long long)c.starts[2],(unsigned long long)c.starts[3],(unsigned long long)c.armed[0],(unsigned long long)c.armed[1],(unsigned long long)c.armed[2],(unsigned long long)c.armed[3],
 (unsigned long long)c.contacts[0],(unsigned long long)c.contacts[1],(unsigned long long)c.contacts[2],(unsigned long long)c.contacts[3],(unsigned long long)c.parries[0],(unsigned long long)c.parries[1],(unsigned long long)c.parries[2],(unsigned long long)c.parries[3],
 (unsigned long long)c.missed,(unsigned long long)c.blocked,(unsigned long long)c.damaged,(unsigned long long)c.rates,(unsigned long long)c.foreign,(unsigned long long)c.released,(unsigned long long)c.duplicate,(unsigned long long)c.context,(unsigned long long)c.paused,
 (unsigned long long)c.gap_n[0],(unsigned long long)c.gap_n[1],(unsigned long long)c.gap_n[2],(unsigned long long)c.gap_min[0],(unsigned long long)c.gap_min[1],(unsigned long long)c.gap_min[2],(unsigned long long)c.gap_max[0],(unsigned long long)c.gap_max[1],(unsigned long long)c.gap_max[2],
 (unsigned long long)c.at_n[0],(unsigned long long)c.at_n[1],(unsigned long long)c.at_n[2],(unsigned long long)c.at_min[0],(unsigned long long)c.at_min[1],(unsigned long long)c.at_min[2],(unsigned long long)c.at_max[0],(unsigned long long)c.at_max[1],(unsigned long long)c.at_max[2],
 (unsigned long long)l.input_new,(unsigned long long)l.buffered,(unsigned long long)l.native_starts,(unsigned long long)l.untrusted,(unsigned long long)l.motion,(unsigned long long)l.matched,(unsigned long long)l.rearms,
 (unsigned long long)l.duplicate,(unsigned long long)l.rejected_motion,(unsigned long long)l.expired,(unsigned long long)l.cancelled,(unsigned long long)l.failed_native,(unsigned long long)c4.source_errors,(unsigned long long)c4.reentered,c4.lines,(unsigned long long)c4.dropped,reason);
 lnk_write(LOG_LEVEL_INFO,line);
}
void cp04_summary()noexcept{
 c4.enabled=false;c4.shield.cancel();for(auto& s:c4.slots)cp04_cancel(s,"SHUTDOWN");
 cp04_report_summary("SHUTDOWN");
}
bool cp04_install()noexcept{
 HookOptions late=HOOK_OPTIONS_INIT;late.priority=-100;
 const ModResult r[]={mods::hook::add_pre<Cp04EocExecute>(on_eoc_pre_cp04,&late),mods::hook::add_post<Cp04EocExecute>(on_eoc_post_cp04,&late),
  mods::hook::add_pre<AttackObserveFix01>(on_attack_pre_cp04,&late),mods::hook::add_post<LinkStickFx>(on_stick_post_cp04,&late),
  mods::hook::add_post<Cp04GuardInit>(on_guard_init_post_cp04,&late),mods::hook::add_post<LinkDamageFix01>(on_link_damage_post_cp04,&late)};
 bool ok=lf.enabled;for(auto x:r)ok=ok&&x==MOD_OK;c4.enabled=ok;
 cp04_log("INIT",nullptr,ok?"BLUE2P2_AND_REUSABLE_NATIVE_SHIELD_CHAIN_ON":"HOOK_FAILURE_NO_PARTIAL_CHECKPOINT");return ok;
}

#undef CP04_NOINLINE
