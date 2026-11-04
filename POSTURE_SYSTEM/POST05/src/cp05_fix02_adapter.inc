// FIX02 adds no native writer. Terminal restoration uses the unchanged POST05
// compare/readback transaction; cleanup uses the unchanged paired cc_set lease.
struct Cp05Fix02Runtime {
 std::uint64_t terminal_seen=0,terminal_applied=0,terminal_rejected=0,cleanup_requested=0,terminal_refused=0;const char* last_terminal_gate="NOT_EVALUATED";
 bool enabled=false;cp05fix02::GuardMonitor monitor;cp05fix02::Sample pre[64]{};
 unsigned guard_lines=0,critical_guard_lines=0;std::uint64_t guard_dropped=0,last_summary_tick=0;
 unsigned reaction_lines=0,contact_lines=0;std::uint64_t reaction_dropped=0,contact_dropped=0;
} f2;
bool fix02_terminal_proof(daE_OC_c* a,const PostReads& r,const HitReport& rep)noexcept{
 f2.last_terminal_gate="IDENTITY_OR_LIFECYCLE";
 if(!r.alive||!a||g.reentered||g.layout!=1||g.rx.session_disabled()||!cp04_world_safe()||g.cur.actor!=reinterpret_cast<std::uintptr_t>(a)||!g.cur.tracked||g.cur.slot<0||
    !g.eng.obs.ledger.valid(g.cur.slot,g.cur.generation)||rep.id!=g.cur.id||rep.generation!=g.cur.generation||rep.slot!=g.cur.slot)return false;
 f2.last_terminal_gate="ORDINARY_LINK_SWORD_RECEIPT";
 if(!cp05fix02::ordinary_link_sword(rep,g.cur.from_link))return false;
 f2.last_terminal_gate="ANIMATION_CHANGED_OR_UNAVAILABLE";
 if(!a->mpMorf||!post05::same_anim(g.cur.pre_aux,r.aux))return false;
 f2.last_terminal_gate="LIVE_ANIMATION_READBACK_CHANGED";
 if(a->mpMorf->getFrame()!=r.aux.anim_frame||a->mpMorf->getPlaySpeed()!=r.aux.anim_rate||a->mpMorf->mFrameCtrl.getEnd()!=r.aux.anim_end||a->speedF!=r.aux.speed)return false;
 const auto proof=cp05fix02::terminal(g.cur.pre_live.mode,g.cur.pre_live.state,g.cur.pre_aux,r.aux,a->checkBck(5),a->checkBck(6),a->mpMorf->isStop());
 f2.last_terminal_gate=cp05fix02::name(proof);if(proof==cp05fix02::Terminal::None)return false;++f2.terminal_seen;return true;
}
void fix02_terminal_result(const HitReport& rep,lnks1fix::Continuation old,bool proved,const post05::DecisionResult& d,post05::Apply ap)noexcept{
 if(!proved){if(old==lnks1fix::Continuation::Safe||rep.fam.family!=Family::Sword||rep.j.lethal)return;++f2.terminal_refused;}else if(ap==post05::Apply::Applied)++f2.terminal_applied;else ++f2.terminal_rejected;
 if(!lnk_take(f2.reaction_lines,f2.reaction_dropped,96))return;char line[kLogLineMax*2];
 std::snprintf(line,sizeof(line),"ANTI_STAGGER event=TERMINAL_CONTINUATION proved=%u gate=%s actor=%u generation=%llu hit_event=%llu tick=%llu pre=%d/%d post=%d/%d frame=%.3f end=%d rate=%.3f native_stop=1 legacy_reason=%s decision=%s reason=%s apply=%s hp=%d>%d posture=%d>%d exact_pre_restored=%u runtime=HUMAN_RUNTIME_PENDING",unsigned(proved),f2.last_terminal_gate,rep.id,(unsigned long long)rep.generation,(unsigned long long)rep.event,(unsigned long long)g.p8.tick,rep.pre.mode,rep.pre.state,rep.post.mode,rep.post.state,rep.aux_pre.anim_frame,int(rep.aux_pre.anim_end),rep.aux_pre.anim_rate,lnks1fix::name(old),post05::name(d.decision),post05::name(d.reason),post05::name(ap),rep.pre.health,rep.post.health,g.pe.last.before,g.pe.last.after,unsigned(ap==post05::Apply::Applied));lnk_write(LOG_LEVEL_INFO,line);
}
void fix02_guard_contact()noexcept{
 if(!cp04_world_safe())return;
 if(!cp05fix02::cleanup_contact(g.cur.pin,g.cur.pv))return;
 const auto before=fx.cleanup.c.requested;fix01_cleanup_request();if(fx.cleanup.c.requested==before)return;++f2.cleanup_requested;
 if(!lnk_take(f2.contact_lines,f2.contact_dropped,96))return;char line[kLogLineMax*2];const auto&p=g.cur.pin;
 std::snprintf(line,sizeof(line),"GUARD_AI event=FRESH_CONTACT_CLEANUP_REQUEST actor=%u generation=%llu tick=%llu token=%llu mode=%d/%d frame=%.3f rate=%.3f blocked=%u verdict=%s held_R=%u guard_accepted=%u source=ARMTRACK_PREVIOUS_PASS_AND_NATIVE_LINK_CONTACT writer=EXISTING_PAIRED_CC_SET_REGISTRATION_LEASE native_hit_flag_write=0 runtime=HUMAN_RUNTIME_PENDING",p.id,(unsigned long long)p.generation,(unsigned long long)g.p8.tick,(unsigned long long)g.cur.pv.token,p.mode,p.state,p.anim_frame,p.anim_rate,unsigned(g.cur.pv.blocked),post08::name(g.cur.pv.result),unsigned(lf.r_held),unsigned(lf.guard_accepted));lnk_write(LOG_LEVEL_INFO,line);
}
void fix02_summary()noexcept{
 f2.monitor.shutdown();fix02_guard_summary("SHUTDOWN");
 char line[kLogLineMax];std::snprintf(line,sizeof(line),"CP05_FIX02_SUMMARY reason=SHUTDOWN version=0.11.10 terminal_proved=%llu terminal_applied=%llu terminal_rejected_by_transaction=%llu terminal_receipt_refused=%llu fresh_contact_cleanup=%llu reaction_lines=%u reaction_dropped=%llu contact_lines=%u contact_dropped=%llu new_native_writers=0 runtime=HUMAN_RUNTIME_PENDING",(unsigned long long)f2.terminal_seen,(unsigned long long)f2.terminal_applied,(unsigned long long)f2.terminal_rejected,(unsigned long long)f2.terminal_refused,(unsigned long long)f2.cleanup_requested,f2.reaction_lines,(unsigned long long)f2.reaction_dropped,f2.contact_lines,(unsigned long long)f2.contact_dropped);lnk_write(LOG_LEVEL_INFO,line);
}
cp05fix02::Sample fix02_guard_sample(daE_OC_c* a,int slot)noexcept{
 cp05fix02::Sample s;if(slot<0||slot>=64||!a||a->state.init_state==3)return s;const auto& l=g.eng.obs.ledger.slot(unsigned(slot));
 if(l.life!=Life::Active||l.id!=fopAcM_GetID(a)||l.ptr!=reinterpret_cast<std::uintptr_t>(a))return s;
 s.valid=true;s.tick=g.p8.tick;s.key.id=l.id;s.key.generation=l.generation;s.key.epoch=l.epoch_last;s.key.stage=stage_token(dComIfGp_getStartStageName());s.key.room=fopAcM_GetRoomNo(a);s.key.layer=dComIfG_play_c::getLayerNo(0);
 s.mode=a->mActionMode;s.state=a->mOcState;s.old=a->mOldActionMode;s.t0=a->field_0x6c0;s.t2=a->field_0x6c2;s.hp=a->health;s.speed=a->speedF;s.battle=a->mBattleOn;s.sim=cp04_world_safe();s.penalty=lf.def.phase()!=lnkfx::Phase::Ready||lb.m.hold();
 if(a->mpMorf){s.frame=a->mpMorf->getFrame();s.rate=a->mpMorf->getPlaySpeed();s.end=a->mpMorf->mFrameCtrl.getEnd();s.stop=a->mpMorf->isStop();s.bck=a->checkBck(5)?5:a->checkBck(6)?6:a->checkBck(28)?28:0;}
 const auto aux=read_aux(a);s.atp=aux.atp;const auto profile=postbal01::capture(g.eng.obs.ledger,lg.posture.variants,slot,l.id,l.generation,aux);s.variant=profile.verified?int(profile.variant):4;
 if(g.cur.actor==reinterpret_cast<std::uintptr_t>(a)&&g.cur.id==l.id&&g.cur.generation==l.generation){s.contact=g.cur.pin.fresh&&post08::Parry::any_contact(g.cur.pin)&&g.cur.pv.first_contact;s.parry=g.cur.balance_parry_gain>0;}
 s.at=a->mSphs_at[0].ChkAtSet()||a->mSphs_at[1].ChkAtSet();s.facing=int(s16(a->shape_angle.y-a->mPrevShapeAngle));
 auto* p=dComIfGp_getLinkPlayer();
 if(p&&p->profname==fpcNm_ALINK_e&&p->state.init_state!=3&&!p->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF)){
  s.target=g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife()>0&&fopAcM_GetRoomNo(a)==p->current.roomNo;s.link_proc=static_cast<daAlink_c*>(p)->mProcID;
  s.input=lf.sample_ok&&lf.sample_tick==g.p8.tick&&lf.key==reinterpret_cast<std::uintptr_t>(p);s.held=s.input&&lf.r_held;s.guard=lf.guard_accepted;
  const float x=a->current.pos.x-p->current.pos.x,z=a->current.pos.z-p->current.pos.z;s.range2=x*x+z*z;s.height=p->current.pos.y-a->current.pos.y;
  if(!cp04::finite(s.range2)||!cp04::finite(s.height)){s.target=false;s.range2=-1;}
  auto* link=static_cast<daAlink_c*>(p);bool on_link=false;
  for(unsigned n=0;n<2;++n)if(a->mSphs_at[n].ChkAtHit()){const void* hit=a->mSphs_at[n].GetAtHitObj();for(unsigned k=0;k<3;++k)on_link=on_link||hit==static_cast<const void*>(static_cast<cCcD_Obj*>(&link->mTgCyls[k]));}
  s.stale_link_hit=on_link&&!g.arm.fresh(slot,l.id,l.generation,g.p8.tick);
 }return s;
}
void fix02_guard_log(unsigned slot,cp05fix02::GuardEvent event,const cp05fix02::Sample& s)noexcept{
 if(event==cp05fix02::GuardEvent::None)return;const auto& r=f2.monitor.row(slot);const bool critical=event==cp05fix02::GuardEvent::EarlyRebound&&f2.critical_guard_lines<32;
 if(!critical&&(f2.guard_lines>=240||r.lines>=8)){++f2.guard_dropped;return;}if(critical)++f2.critical_guard_lines;++f2.guard_lines;f2.monitor.logged(slot);
 char line[kLogLineMax*3];std::snprintf(line,sizeof(line),"GUARD_AI event=%s actor=%u generation=%llu epoch=%u stage=%u room=%d layer=%d tick=%llu own_tick=%llu variant=%d mode=%d/%d old=%d timer6c0=%d timer6c2=%d bck=%d frame=%.3f end=%.1f rate=%.3f isStop=%u speed=%.3f held_R=%u input_fresh=%u accepted_guard=%u Link_proc=%d Link_penalty=%u target_present=%u battle=%u range_xz_squared=%.3f height=%.3f native_prev_target_yaw_error=%d at=%u stale_Link_contact=%u last_attempt_own_tick=%llu last_AT_own_tick=%llu last_contact_tick=%llu last_committed_parry_tick=%llu episode_attempts=%u episode_AT_samples=%u transitions=%u timer_progress=%u early_rebounds=%u reason=%s observer_native_writes=0 runtime=HUMAN_RUNTIME_PENDING",cp05fix02::name(event),s.key.id,(unsigned long long)s.key.generation,s.key.epoch,s.key.stage,s.key.room,s.key.layer,(unsigned long long)s.tick,(unsigned long long)r.own,s.variant,s.mode,s.state,s.old,s.t0,s.t2,s.bck,s.frame,s.end,s.rate,unsigned(s.stop),s.speed,unsigned(s.held),unsigned(s.input),unsigned(s.guard),s.link_proc,unsigned(s.penalty),unsigned(s.target),unsigned(s.battle),s.range2,s.height,s.facing,unsigned(s.at),unsigned(s.stale_link_hit),(unsigned long long)r.last_attempt,(unsigned long long)r.last_at,(unsigned long long)r.last_contact_tick,(unsigned long long)r.last_parry_tick,r.attempts,r.arms,r.transitions,r.timer_progress,r.early,cp05fix02::wait_reason(s));lnk_write(LOG_LEVEL_INFO,line);
}
HookAction on_eoc_pre_fix02_guard(ModContext*,void* args,void*,void*)noexcept{
 if(!f2.enabled||!args)return HOOK_CONTINUE;auto* a=mods::arg<daE_OC_c*>(args,0);const int slot=cp04_slot(a);if(slot>=0)f2.pre[slot]=fix02_guard_sample(a,slot);return HOOK_CONTINUE;
}
void on_eoc_post_fix02_guard(ModContext*,void* args,void*,void*)noexcept{
 if(!f2.enabled||!args)return;auto* a=mods::arg<daE_OC_c*>(args,0);const int slot=cp04_slot(a);if(slot<0)return;const auto s=fix02_guard_sample(a,slot);fix02_guard_log(unsigned(slot),f2.monitor.feed(unsigned(slot),f2.pre[slot],s),s);
}
void fix02_guard_summary(const char* reason)noexcept{
 const auto& c=f2.monitor.c;char line[kLogLineMax*2];std::snprintf(line,sizeof(line),"GUARD_AI_SUMMARY reason=%s samples=%llu duplicate_samples=%llu guard_episodes=%llu ends=%llu context_ends=%llu expired_inconclusive=%llu persistent_reports=%llu stale_early_rebounds=%llu attempts=%llu actual_AT_samples=%llu by_blue_red_black_fury_unknown=%llu/%llu/%llu/%llu/%llu duration_hist_lt30_lt60_lt120_lt240_lt480_lt960_lt1920_ge1920=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu detail_lines=%u detail_dropped=%llu observation_only=1 forced_attacks=0 runtime=HUMAN_RUNTIME_PENDING",reason,(unsigned long long)c.samples,(unsigned long long)c.duplicates,(unsigned long long)c.starts,(unsigned long long)c.ends,(unsigned long long)c.contexts,(unsigned long long)c.expired,(unsigned long long)c.persistent,(unsigned long long)c.early,(unsigned long long)c.attempts,(unsigned long long)c.arms,(unsigned long long)c.by_variant[0],(unsigned long long)c.by_variant[1],(unsigned long long)c.by_variant[2],(unsigned long long)c.by_variant[3],(unsigned long long)c.by_variant[4],(unsigned long long)c.duration_hist[0],(unsigned long long)c.duration_hist[1],(unsigned long long)c.duration_hist[2],(unsigned long long)c.duration_hist[3],(unsigned long long)c.duration_hist[4],(unsigned long long)c.duration_hist[5],(unsigned long long)c.duration_hist[6],(unsigned long long)c.duration_hist[7],f2.guard_lines,(unsigned long long)f2.guard_dropped);lnk_write(LOG_LEVEL_INFO,line);
}
void fix02_maintenance()noexcept{
 if(!f2.enabled)return;for(unsigned n=0;n<64;++n)if(f2.monitor.expire(n,g.p8.tick))fix02_guard_log(n,cp05fix02::GuardEvent::Expired,f2.monitor.row(n).last);
 if(g.p8.tick<f2.last_summary_tick)f2.last_summary_tick=g.p8.tick;if(g.p8.tick-f2.last_summary_tick>=1800){fix02_guard_summary("PERIODIC");f2.last_summary_tick=g.p8.tick;}
}
bool fix02_install()noexcept{
 HookOptions last=HOOK_OPTIONS_INIT;last.priority=-300;
 const auto pre=mods::hook::add_pre<Cp04EocExecute>(on_eoc_pre_fix02_guard,&last),post=mods::hook::add_post<Cp04EocExecute>(on_eoc_post_fix02_guard,&last);f2.enabled=pre==MOD_OK&&post==MOD_OK;return f2.enabled;
}
