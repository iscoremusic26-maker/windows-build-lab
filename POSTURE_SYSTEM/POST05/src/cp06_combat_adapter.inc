// CP06 0.11.11 production adapter. Included once inside mod.cpp's anonymous
// namespace after FIX02. Fully hash-audited by ci/cp06_audit.py.
#if defined(_MSC_VER)
#define CP06_NOINLINE __declspec(noinline)
#else
#define CP06_NOINLINE __attribute__((noinline))
#endif
struct Cp06Runtime {
 bool enabled=false;cp06::ChipGuard chip;cp06::LateInput late;
 std::uint64_t fury_offered=0,fury_refused_unverified=0,last_input_serial=0,last_summary_tick=0,compensated=0,compensate_failed=0;
 unsigned chip_lines=0,late_lines=0,fury_lines=0;std::uint64_t chip_dropped=0,late_dropped=0,fury_dropped=0;
} c6;
// POST08 Input: the fury bit is offered only for an E_OC2 sphere pair at power6,
// Expansion's exact fury rate and a verified per-generation LNK07 BLACK_FURY.
bool cp06_fury_verified(const PreReads& r)noexcept{
 if(!c6.enabled||!g.cur.tracked||g.cur.slot<0)return false;
 const bool e2=r.aux.resource==static_cast<std::uint8_t>(NameKind::E_OC2);
 if(!e2||r.aux.atp!=cp06::kFuryAtp)return false;  // O(1) for every non-fury actor and tick
 if(!g.eng.obs.ledger.valid(g.cur.slot,g.cur.generation)){++c6.fury_refused_unverified;return false;}
 const auto profile=postbal01::capture(g.eng.obs.ledger,lg.posture.variants,g.cur.slot,g.cur.id,g.cur.generation,r.aux);
 const bool fury=profile.verified&&profile.variant==postbal01::Variant::BlackFury;
 const bool ok=cp06::fury_parry_authenticated(e2,r.aux.atp,r.aux.anim_ok!=0,r.aux.anim_rate,fury);
 if(!ok)++c6.fury_refused_unverified;
 return ok;
}
// Same engine predicate Expansion CP19F2 uses for its chip (both directions of
// one dCcS::SetAtTgGObjInf call), read-only, on THIS actor and the live Link.
bool cp06_shield_pair(daE_OC_c* a,daAlink_c* k)noexcept{
 for(int j=0;j<2;++j){
  auto& s=a->mSphs_at[j];const bool at_hit=s.cCcD_ObjHitInf::ChkAtHit()!=0;if(!at_hit||!s.ChkAtShieldHit())continue;
  const void* at_obj=static_cast<const void*>(s.cCcD_ObjHitInf::GetAtHitObj());const void* sphere=static_cast<const void*>(static_cast<cCcD_Obj*>(&s));
  for(int i=0;i<3;++i){auto& c=k->mTgCyls[i];if(at_obj!=static_cast<const void*>(static_cast<cCcD_Obj*>(&c)))continue;
   if(c.cCcD_ObjHitInf::ChkTgHit()&&c.ChkTgShieldHit()&&static_cast<const void*>(c.cCcD_ObjHitInf::GetTgHitObj())==sphere)return true;}
 }
 return false;
}
cp06::ChipView cp06_chip_view(daE_OC_c* a)noexcept{
 cp06::ChipView v;auto* p=dComIfGp_getLinkPlayer();
 v.pending=g_dComIfG_gameInfo.play.getItemLifeCount();v.type=g_dComIfG_gameInfo.play.getItemLifeCountType();
 v.life=static_cast<int>(g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife());
 v.link_ok=p&&p->profname==fpcNm_ALINK_e&&p->state.init_state!=3&&v.life>0&&!g_dComIfG_gameInfo.play.getEvent()->runCheck();
 v.shield_pair=v.link_ok&&a&&cp06_shield_pair(a,static_cast<daAlink_c*>(p));return v;
}
void cp06_chip_log(const char* event,unsigned slot,const cp06::ChipGuard::Row& r,const cp06::ChipView& before,const cp06::ChipView& now,cp06::Chip result,float written)noexcept{
 if(!lnk_take(c6.chip_lines,c6.chip_dropped,96))return;char line[kLogLineMax*2];
 std::snprintf(line,sizeof(line),"CP06_FURY_CHIP event=%s version=%s slot=%u actor=%u generation=%llu token=%llu tick=%llu result=%s pending_before=%.3f pending_after_expansion=%.3f written=%.3f type=%u/%u life=%d/%d shield_pair=%u window=OWN_POST90_TO_POST-250_SAME_EXECUTE attribution=EXPANSION_CP19F2_POST0_EXACT_ONE_UNIT_AFTER_COMMITTED_FURY_PARRY expansion_flash=NOT_CONTROLLED runtime=HUMAN_RUNTIME_PENDING",
 event,cp06::kVersion,slot,r.id,(unsigned long long)r.generation,(unsigned long long)r.token,(unsigned long long)g.p8.tick,cp06::name(result),before.pending,now.pending,written,unsigned(before.type),unsigned(now.type),before.life,now.life,unsigned(before.shield_pair));lnk_write(result==cp06::Chip::Neutralized||result==cp06::Chip::NoChipObserved?LOG_LEVEL_INFO:LOG_LEVEL_WARN,line);
}
// Native mutation (only CP06 writer): returns the single attributed Expansion
// fury chip unit to the meter's pending life count in the SAME execute that
// charged it, before any consumer (dMeter2) runs. Readback must be exact.
CP06_NOINLINE bool cp06_native_chip_restore(float units,std::uint8_t type,float expected)noexcept{
 if(units!=cp06::kChipUnits)return false;
 g_dComIfG_gameInfo.play.setItemLifeCount(units,type);
 return g_dComIfG_gameInfo.play.getItemLifeCount()==expected&&g_dComIfG_gameInfo.play.getItemLifeCountType()==type;
}
void cp06_parry_committed(const post08::Input& in,const post08::Verdict& v,bool ok)noexcept{
 if(!c6.enabled||!ok||!in.fury_verified||in.slot<0||unsigned(in.slot)>=cp06::kSlots)return;
 if(c6.chip.arm(unsigned(in.slot),in.id,in.generation,v.token,g.p8.tick)){++c6.fury_offered;
  if(lnk_take(c6.fury_lines,c6.fury_dropped,64)){char line[kLogLineMax];std::snprintf(line,sizeof(line),"CP06_FURY_PARRY event=COMMITTED_CHIP_WINDOW_ARMED version=%s actor=%u generation=%llu token=%llu tick=%llu atp=%d rate=%.3f frame=%.3f sa_serial=%llu delta=%.2f runtime=HUMAN_RUNTIME_PENDING",cp06::kVersion,in.id,(unsigned long long)in.generation,(unsigned long long)v.token,(unsigned long long)g.p8.tick,in.atp,in.anim_rate,in.anim_frame,(unsigned long long)in.pass.serial,v.delta);lnk_write(LOG_LEVEL_INFO,line);}
 }
}
void cp06_contact(const post08::Input& in,const post08::Verdict& v)noexcept{
 if(!c6.enabled||!v.first_contact||!in.fresh||!in.link_ok)return;
 if(v.result!=post08::Result::NormalGuard&&v.result!=post08::Result::NotBlocked)return;
 if(!(in.at_on_link[0]||in.at_on_link[1]))return;
 const auto s=postbal01::select(g.cur.balance_profile,in.slot,in.id,in.generation,g.cur.balance_profile.epoch);
 c6.late.contact(in.id,in.generation,v.token,g.p8.tick,s.verified?int(s.variant):4,int(v.result),in.anim_rate,in.anim_frame);
}
void on_eoc_post_cp06_capture(ModContext*,void* args,void*,void*)noexcept{
 if(!c6.enabled||!args||!c6.chip.armed_count())return;auto* a=mods::arg<daE_OC_c*>(args,0);const int slot=cp04_slot(a);if(slot<0)return;
 const auto r=c6.chip.row(unsigned(slot));if(!r.armed)return;const auto& l=g.eng.obs.ledger.slot(unsigned(slot));
 const auto v=cp06_chip_view(a);
 if(!c6.chip.capture(unsigned(slot),l.id,l.generation,g.p8.tick,v))cp06_chip_log("CAPTURE_REFUSED",unsigned(slot),r,v,v,cp06::Chip::NotCaptured,0);
}
void on_eoc_post_cp06_chip(ModContext*,void* args,void*,void*)noexcept{
 if(!c6.enabled||!args||!c6.chip.armed_count())return;auto* a=mods::arg<daE_OC_c*>(args,0);const int slot=cp04_slot(a);if(slot<0)return;
 const auto row=c6.chip.row(unsigned(slot));if(!row.armed)return;const auto& l=g.eng.obs.ledger.slot(unsigned(slot));
 const auto now=cp06_chip_view(a);float units=0;std::uint8_t type=0;
 auto result=c6.chip.decide(unsigned(slot),l.id,l.generation,g.p8.tick,now,units,type);
 if(result==cp06::Chip::Neutralized){
  if(g.p8.layout!=1||!cp06_native_chip_restore(units,type,row.before.pending)){result=c6.chip.record(cp06::Chip::ReadbackFailed);++c6.compensate_failed;}
  else{c6.chip.record(cp06::Chip::Neutralized);++c6.compensated;}
 }
 cp06_chip_log("DECIDED",unsigned(slot),row,row.before,now,result,result==cp06::Chip::Neutralized?units:0);
}
void cp06_late_log(const cp06::LateInput::Event& e,unsigned d)noexcept{
 if(!lnk_take(c6.late_lines,c6.late_dropped,64))return;char line[kLogLineMax];
 std::snprintf(line,sizeof(line),"CP06_PARRY_TIMING event=NEW_R_B_AFTER_RESOLVING_PASS version=%s actor=%u generation=%llu token=%llu variant=%s verdict=%s enemy_rate=%.3f enemy_frame=%.3f judged_tick=%llu input_ticks_after_judge=%u input_ticks_after_pass=%u meaning=PRESS_ARRIVED_AFTER_NATIVE_BLOCK_OR_HIT_NOT_A_PARRY runtime=HUMAN_RUNTIME_PENDING",
 cp06::kVersion,e.id,(unsigned long long)e.generation,(unsigned long long)e.token,postbal01::name(postbal01::Variant(e.variant)),post08::name(post08::Result(e.result)),e.rate,e.frame,(unsigned long long)e.tick,d,d+1);lnk_write(LOG_LEVEL_INFO,line);
}
void cp06_report(const char* reason)noexcept{
 const auto& k=c6.chip.c;const auto& t=c6.late.c;char line[kLogLineMax*3];
 std::snprintf(line,sizeof(line),"CP06_SUMMARY reason=%s version=%s source=%s fury_parry_offered=%llu fury_unverified_atp6_refused=%llu chip_windows_armed=%llu captured=%llu neutralized=%llu no_chip_observed=%llu not_captured=%llu context_changed=%llu delta_mismatch=%llu type_changed=%llu life_changed=%llu link_invalid=%llu readback_failed=%llu duplicate_arm=%llu shield_pair_missing=%llu late_contacts=%llu late_inputs=%llu late_by_ticks_after_judge=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu late_by_variant(blue/red/black/fury/unknown)=%llu/%llu/%llu/%llu/%llu late_rate2=%llu lines(chip/late/fury)=%u+%llu/%u+%llu/%u+%llu runtime=HUMAN_RUNTIME_PENDING",
 reason,cp06::kVersion,POST05_SOURCE_COMMIT,(unsigned long long)c6.fury_offered,(unsigned long long)c6.fury_refused_unverified,(unsigned long long)k.armed,(unsigned long long)k.captured,
 (unsigned long long)k.result[1],(unsigned long long)k.result[2],(unsigned long long)k.result[4],(unsigned long long)k.result[5],(unsigned long long)k.result[6],(unsigned long long)k.result[7],(unsigned long long)k.result[8],(unsigned long long)k.result[9],(unsigned long long)k.result[10],(unsigned long long)k.duplicate_arm,(unsigned long long)k.shield_pair_missing,
 (unsigned long long)t.contacts,(unsigned long long)t.late,(unsigned long long)t.by_delta[0],(unsigned long long)t.by_delta[1],(unsigned long long)t.by_delta[2],(unsigned long long)t.by_delta[3],(unsigned long long)t.by_delta[4],(unsigned long long)t.by_delta[5],(unsigned long long)t.by_delta[6],(unsigned long long)t.by_delta[7],(unsigned long long)t.by_delta[8],
 (unsigned long long)t.by_variant[0],(unsigned long long)t.by_variant[1],(unsigned long long)t.by_variant[2],(unsigned long long)t.by_variant[3],(unsigned long long)t.by_variant[4],(unsigned long long)t.rate2,c6.chip_lines,(unsigned long long)c6.chip_dropped,c6.late_lines,(unsigned long long)c6.late_dropped,c6.fury_lines,(unsigned long long)c6.fury_dropped);
 lnk_write(LOG_LEVEL_INFO,line);
}
void cp06_maintenance()noexcept{
 if(!c6.enabled)return;
 if(c4.shield.input_serial!=c6.last_input_serial){c6.last_input_serial=c4.shield.input_serial;c6.late.input(c4.shield.input_tick,[](const cp06::LateInput::Event& e,unsigned d){cp06_late_log(e,d);});}
 if(g.p8.tick<c6.last_summary_tick)c6.last_summary_tick=g.p8.tick;
 if(g.p8.tick-c6.last_summary_tick>=1800){cp06_report("PERIODIC");c6.last_summary_tick=g.p8.tick;}
}
void cp06_summary()noexcept{c6.enabled=false;cp06_report("SHUTDOWN");c6.chip.reset();c6.late.reset();}
bool cp06_install()noexcept{
 HookOptions before=HOOK_OPTIONS_INIT,after=HOOK_OPTIONS_INIT;before.priority=90;after.priority=-250;
 const auto capture=mods::hook::add_post<Cp04EocExecute>(on_eoc_post_cp06_capture,&before),decide=mods::hook::add_post<Cp04EocExecute>(on_eoc_post_cp06_chip,&after);
 c6.enabled=c5.enabled&&capture==MOD_OK&&decide==MOD_OK;
 char line[kLogLineMax];std::snprintf(line,sizeof(line),"CP06_INIT version=%s source=%s enabled=%u red4=1.05/1.35/1.35/1.05 black8=1.05/1.05/1.30x5/1.05 black7=1.30/1.30/1.05/1.05/1.20/1.20/1.20 blue=1.10/1.10/1.50/1.50 blue_gaps=8/8/2 at_targets(red4)=60/36/62 at_targets(black8)=62/62/38/38/38/38/64 at_targets(black7)=38/62/64/47/47/47 fury_parry=ATP6_LNK07_FURY_RATE2_ONLY chip=ATTRIBUTED_EXACT_ONE_UNIT_SAME_EXECUTE parry_window=UNCHANGED_-1_+7_AGE9 new_hook_targets=0 runtime=HUMAN_RUNTIME_PENDING",cp06::kVersion,POST05_SOURCE_COMMIT,unsigned(c6.enabled));lnk_write(LOG_LEVEL_INFO,line);
 return c6.enabled;
}
#undef CP06_NOINLINE
