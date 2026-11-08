// CP06 FIX01 0.11.12 production adapter. Included once inside mod.cpp's anonymous
// namespace after the CP06 adapter. Fully hash-audited by ci/cp06f1_audit.py.
// Three native writer functions, each noinline, scoped and read back:
//   cp06f1_native_sphere_place  centre+radius of ONE own AT sphere of THIS E_OC for THIS pass
//                               (the engine setters cc_set/Expansion use; cc_set rewrites both
//                               next execute), only onto an exact skipped native pose.
//   cp06f1_native_pose_frame    morf frame field of THIS actor + the engine's own model calc; used by
//                               the pose probe for the vanilla frames and then the exact frame back
//                               (every joint matrix and the frame are verified equal afterwards).
//   cp06f1_native_kcolor        Link tevStr.TevKColor r/g/b between Expansion's PRE and the model
//                               draw, only when the value is Expansion's exact expected flash write.
#if defined(_MSC_VER)
#define CP06F1_NOINLINE __declspec(noinline)
#else
#define CP06F1_NOINLINE __attribute__((noinline))
#endif
inline constexpr unsigned kF1Joints=64,kF1Kinds=unsigned(cp06f1::Kind::Count),kF1Results=unsigned(cp06f1::Result::Count);
struct F1Slot{std::uint32_t id=0;std::uint64_t generation=0,tick=0;bool armed=false,valid=false,covered=false;float frame=0;int state=-1;cp06f1::V3 hand{};cp06f1::Swing swing{};};
struct F1Draw{bool active=false,wrote=false;std::uintptr_t link=0,model=0;cp06f1::Rgb orig{},written{};};
struct F1Runtime{
 bool enabled=false,coverage=false,flash_on=false;
 std::uint64_t link_set_tick=~0ull;std::uintptr_t link_set_key=0;
 F1Slot slots[cp06f1::kSlots]{};std::uint64_t seq=0;
 std::uint64_t armed_ticks=0,why[unsigned(cp06f1::Why::Count)]{},result[kF1Kinds][kF1Results]{},backfilled_swings[kF1Kinds]{},backfill_contacts[kF1Kinds]{};
 std::uint64_t probes=0,probe_mismatch=0,place_writes=0,place_failed=0,native_formula=0,foreign_spheres=0,swings=0;
 cp06f1::FlashMirror flash;struct{bool armed=false;std::uintptr_t key=0;std::uint64_t tick=0,comp=0;float pending=0;}chip;
 F1Draw draw[4]{};unsigned depth=0,overflow_open=0,open_swings=0;std::uint64_t kcolor_failed=0,draw_overflow=0;
 unsigned swing_lines=0,backfill_lines=0,flash_lines=0;std::uint64_t swing_dropped=0,backfill_dropped=0,flash_dropped=0,last_summary_tick=0;
} f1;
float f1_snap[kF1Joints][3][4];
daAlink_c* f1_link()noexcept{
 auto* p=dComIfGp_getLinkPlayer();
 if(!p||p->profname!=fpcNm_ALINK_e||p->state.init_state==3)return nullptr;return static_cast<daAlink_c*>(p);
}
// ---------------------------------------------------------------- writers
// Writer 1/3: one own AT sphere of the executing E_OC, engine setters, exact readback.
CP06F1_NOINLINE bool cp06f1_native_sphere_place(daE_OC_c* a,int j,float x,float y,float z,float r)noexcept{
 if(j!=0&&j!=1)return false;
 a->mSphs_at[j].SetC(cXyz(x,y,z));a->mSphs_at[j].SetR(r);
 const cXyz& c=a->mSphs_at[j].GetC();return c.x==x&&c.y==y&&c.z==z&&a->mSphs_at[j].GetR()==r;
}
// Writer 2/3: the morf frame FIELD of THIS actor followed by the engine's own pose calc
// (mDoExt_McaMorfSO::modelCalc). Field write exactly like the console J3DFrameCtrl::setFrame: no
// Dusk material-interpolation reset of the joint controller (the frame is back before anything
// consumes it); modelCalc sets the anm frame itself. Called only by cp06f1_pose_probe.
CP06F1_NOINLINE void cp06f1_native_pose_frame(mDoExt_McaMorfSO* m,float frame)noexcept{m->mFrameCtrl.mFrame=frame;m->modelCalc();}
// Pose probe (no native write of its own): snapshot every joint matrix, evaluate up to three vanilla
// frames, put the exact frame back and verify every joint matrix and the frame read back equal.
// Pure calc path only (morph done, checked by the caller).
bool cp06f1_pose_probe(daE_OC_c* a,const float* frames,unsigned count,float restore,float out[cp06f1::kMaxProbes][3][4])noexcept{
 mDoExt_McaMorfSO* m=a->mpMorf;J3DModel* model=m?m->getModel():nullptr;if(!model||count<1||count>cp06f1::kMaxProbes)return false;
 const unsigned n=model->getModelData()->getJointNum();if(n>kF1Joints||n<=0xcu)return false;
 for(unsigned i=0;i<n;++i)std::memcpy(f1_snap[i],model->getAnmMtx(int(i)),sizeof(f1_snap[i]));
 for(unsigned i=0;i<count;++i){cp06f1_native_pose_frame(m,frames[i]);std::memcpy(out[i],model->getAnmMtx(0xc),sizeof(f1_snap[0]));}
 cp06f1_native_pose_frame(m,restore);
 bool same=m->getFrame()==restore;
 for(unsigned i=0;i<n&&same;++i){const MtxP q=model->getAnmMtx(int(i));for(int r=0;r<3&&same;++r)for(int c=0;c<4&&same;++c)same=q[r][c]==f1_snap[i][r][c];}
 return same;
}
// Writer 3/3: Link's constant colour register between Expansion's PRE and the model draw.
CP06F1_NOINLINE bool cp06f1_native_kcolor(daAlink_c* k,std::uint8_t r,std::uint8_t g,std::uint8_t b)noexcept{
 GXColor& kc=k->tevStr.TevKColor;kc.r=r;kc.g=g;kc.b=b;return kc.r==r&&kc.g==g&&kc.b==b;
}
// ---------------------------------------------------------------- A. coverage
cp06f1::Kind f1_kind(daE_OC_c* a,int slot,const post03::Instance& l)noexcept{
 const auto p=postbal01::capture(g.eng.obs.ledger,lg.posture.variants,slot,l.id,l.generation,read_aux(a));
 if(!p.verified)return cp06f1::Kind::Unknown;
 switch(p.variant){case postbal01::Variant::Blue:return cp06f1::Kind::Blue;case postbal01::Variant::Red:return cp06f1::Kind::Red;
  case postbal01::Variant::Black:return cp06f1::Kind::Black;case postbal01::Variant::BlackFury:return cp06f1::Kind::BlackFury;default:return cp06f1::Kind::Unknown;}
}
void f1_cyls(daAlink_c* k,cp06f1::Cyl cy[3])noexcept{
 for(int i=0;i<3;++i){const auto& c=k->mTgCyls[i];const cXyz& o=c.GetC();cy[i].c={o.x,o.y,o.z};cy[i].r=c.GetR();cy[i].h=c.GetH();cy[i].on=c.ChkTgSet()!=0;}
}
// Previous pass on this actor's own spheres: At record on one of the live Link's Tg cylinders (pointer identity), shield flag.
void f1_read_pass(daE_OC_c* a,daAlink_c* k,cp06f1::PassView& pv)noexcept{
 for(int j=0;j<2;++j){auto& s=a->mSphs_at[j];if(!s.cCcD_ObjHitInf::ChkAtHit())continue;
  const void* obj=static_cast<const void*>(s.cCcD_ObjHitInf::GetAtHitObj());bool link=false;
  for(int i=0;i<3&&k;++i)link=link||obj==static_cast<const void*>(static_cast<cCcD_Obj*>(&k->mTgCyls[i]));
  if(link){pv.link=true;pv.shield=pv.shield||s.ChkAtShieldHit();}else pv.other=true;
 }
}
void f1_swing_log(const F1Slot& S,cp06f1::Result r,const char* why)noexcept{
 const auto& s=S.swing;const bool anomaly=r==cp06f1::Result::ContactLostConfirmed||r==cp06f1::Result::GeometryUndetermined;
 if(!(anomaly||s.backfill_ticks||s.kind==cp06f1::Kind::BlackFury||s.kind==cp06f1::Kind::RedSpecialX2||f1.swings<=48))return;
 if(!lnk_take(f1.swing_lines,f1.swing_dropped,anomaly?400u:300u))return;char line[kLogLineMax*2];
 std::snprintf(line,sizeof(line),"CP06F1_SWING version=%s actor=%u generation=%llu swing=%llu kind=%s clip=%s rate=%.3f start_tick=%llu end_tick=%llu at_ticks=%u pre_first=%.3f pre_last=%.3f result=%s contact_tick=%llu shield=%u parry_proc=%u backfill_ticks=%u backfill_slots=%u backfilled_contact=%u expect_ticks=%u geom_ticks=%u invulnerable_touch_ticks=%u lost=%u no_continuous=%u undetermined=%u link_proc=%d link_damage_timer=%d close=%s anomaly=%u runtime=HUMAN_RUNTIME_PENDING",
  cp06f1::kVersion,s.id,(unsigned long long)s.generation,(unsigned long long)s.seq,cp06f1::name(s.kind),s.clip==1?"B":s.clip==2?"C":"?",s.rate,(unsigned long long)s.start_tick,(unsigned long long)s.last_tick,s.at_ticks,s.first_pre,s.last_pre,cp06f1::name(r),(unsigned long long)s.contact_tick,unsigned(s.shield),unsigned(s.parry),s.backfill_ticks,s.backfill_slots,unsigned(s.backfilled_contact),s.expect_ticks,s.geom_ticks,s.off_touch_ticks,s.lost_ticks,s.nocon_ticks,s.undetermined_ticks,s.link_proc,s.damage_timer,why,unsigned(anomaly));
 lnk_write(anomaly?LOG_LEVEL_WARN:LOG_LEVEL_INFO,line);
}
void f1_close(F1Slot& S,const char* why)noexcept{
 auto& s=S.swing;if(!s.open)return;s.open=false;if(f1.open_swings)--f1.open_swings;const auto r=cp06f1::classify(s);const unsigned k=unsigned(s.kind)<kF1Kinds?unsigned(s.kind):kF1Kinds-1;
 ++f1.swings;++f1.result[k][unsigned(r)];if(s.backfill_ticks)++f1.backfilled_swings[k];if(s.backfilled_contact)++f1.backfill_contacts[k];
 f1_swing_log(S,r,why);
}
void f1_backfill_log(daE_OC_c* a,daAlink_c* k,const cp06f1::Tick& t,const float* fr,unsigned np,const cp06f1::Decision& d,const F1Slot& S,bool native_cur)noexcept{
 if(!lnk_take(f1.backfill_lines,f1.backfill_dropped,240))return;char line[kLogLineMax*2];
 const float dist=cXyz(a->current.pos.x-k->current.pos.x,0.0f,a->current.pos.z-k->current.pos.z).abs();
 auto src=[&](int j)->float{return !d.write[j]?-1.0f:d.from[j]==0?t.post:fr[d.from[j]-1<np?d.from[j]-1:0];};
 std::snprintf(line,sizeof(line),"CP06F1_BACKFILL version=%s actor=%u generation=%llu swing=%llu kind=%s tick=%llu pre=%.3f post=%.3f rate=%.3f probes=%u tip_pose=%.3f hilt_pose=%.3f tip=(%.1f,%.1f,%.1f) hilt=(%.1f,%.1f,%.1f) registered_were=%s link_dist_xz=%.1f link_proc=%d damage_timer=%d meaning=EXACT_NATIVE_POSE_REACHES_LINK_REGISTERED_FOR_THIS_PASS engine_decides=GUARD_PARRY_DAMAGE_IFRAMES runtime=HUMAN_RUNTIME_PENDING",
  cp06f1::kVersion,S.swing.id,(unsigned long long)S.swing.generation,(unsigned long long)S.swing.seq,cp06f1::name(S.swing.kind),(unsigned long long)g.p8.tick,t.pre,t.post,t.rate,np,src(0),src(1),
  d.place[0].c.x,d.place[0].c.y,d.place[0].c.z,d.place[1].c.x,d.place[1].c.y,d.place[1].c.z,native_cur?"NATIVE_CC_SET":"FOREIGN_SWEEP",dist,t.link_proc,t.damage_timer);
 lnk_write(LOG_LEVEL_INFO,line);
}
// One armed tick. `covered` = this tick's registered/native pose was decided against Link (the next
// tick may then treat pose `pre` as already covered).
void f1_sample(daE_OC_c* a,daAlink_c* k,F1Slot& S,cp06f1::Tick& t,bool consecutive,bool clip_b,const float now[3][4],bool& covered)noexcept{
 using namespace cp06f1;covered=false;Sph cur[2];
 for(int j=0;j<2;++j){const cXyz& c=a->mSphs_at[j].GetC();cur[j].c={c.x,c.y,c.z};cur[j].r=a->mSphs_at[j].GetR();}
 Cand c[1+kMaxProbes];unsigned n=0;c[n].s[0]=tip(now);c[n].s[1]=hilt(now);++n;   // [0] exact current native pose (cc_set formula)
 const bool native_cur=same(c[0].s[0].c,cur[0].c)&&same(c[0].s[1].c,cur[1].c)&&cur[0].r==kTipR&&cur[1].r==kHiltR;
 if(native_cur)++f1.native_formula;else ++f1.foreign_spheres;
 if(!k){t.why=Why::LinkMissing;return;}
 Cyl cy[3];f1_cyls(k,cy);t.link_any_on=any_on(cy);
 t.link_current=f1.link_set_tick==g.p8.tick&&f1.link_set_key==reinterpret_cast<std::uintptr_t>(k);
 t.link_proc=k->mProcID;t.damage_timer=k->mDamageTimer;t.guard_attack=k->mProcID==daAlink_c::PROC_GUARD_ATTACK;
 t.geom_touch=touches(cy,cur[0],kKeepSlack,false)||touches(cy,cur[1],kKeepSlack,false);
 t.expect=touches(cy,cur[0],kWriteSlack,true)||touches(cy,cur[1],kWriteSlack,true);
 if(!t.link_current){t.why=Why::LinkNotCurrent;return;}
 if(t.expect){t.why=Why::AlreadyTouching;covered=true;return;}   // a registered sphere already crosses an ON cylinder with margin
 Decision d=decide(cy,cur,c,n);covered=true;   // pose `post` (next tick's `pre`) is now decided against Link
 float fr[kMaxProbes]{};unsigned np=0;
 if(!t.link_any_on){t.why=t.geom_touch?Why::LinkInvulnerable:Why::NoMidContact;}   // native i-frames: no contact possible, no probe spent
 else if(!(d.write[0]&&d.write[1])&&t.rate>=kMinFastRate){
  const V3 hand{now[0][3],now[1][3],now[2][3]};Cyl any[3]={cy[0],cy[1],cy[2]};for(auto& x:any)x.on=true;
  const bool near=S.valid&&reachable(any,hand,S.hand);
  mDoExt_McaMorfSO* m=a->mpMorf;
  if(!consecutive){t.why=d.why==Why::Written?Why::Written:Why::NotConsecutive;t.probe_refused=near;}
  else if(!(np=probe_frames(t.pre,t.post,S.armed,S.armed&&S.covered,fr))){if(d.why!=Why::Written)t.why=Why::WindowEdge;}
  else if(!(m->getMorfRatio()>=1.0f||!m->getOldTransInfo()||!m->getOldQuaternion())){t.why=d.why==Why::Written?Why::Written:Why::MorphActive;t.probe_refused=near;np=0;}
  else if(!near){if(d.why!=Why::Written)t.why=Why::OutOfReach;np=0;}
  else{
   float pm[kMaxProbes][3][4];++f1.probes;
   if(!cp06f1_pose_probe(a,fr,np,t.post,pm)){
    ++f1.probe_mismatch;f1.coverage=false;t.why=Why::ProbeMismatch;t.probe_refused=true;
    char line[kLogLineMax];std::snprintf(line,sizeof(line),"CP06F1_CRITICAL event=POSE_PROBE_RESTORE_MISMATCH version=%s actor=%u generation=%llu frame=%.3f probes=%u action=COVERAGE_DISABLED_FOR_SESSION runtime=HUMAN_RUNTIME_PENDING",cp06f1::kVersion,S.swing.id,(unsigned long long)S.swing.generation,t.post,np);lnk_write(LOG_LEVEL_ERROR,line);return;
   }
   float sn=0,cs=1;sincos16(a->shape_angle.y,sn,cs);
   for(unsigned i=0;i<np;++i){const V3 o=lunge_offset(clip_b,t.pre,fr[i],sn,cs);c[n].s[0]=moved(tip(pm[i]),o);c[n].s[1]=moved(hilt(pm[i]),o);++n;}
   d=decide(cy,cur,c,n);t.why=d.why;
  }
 }else{t.why=d.why==Why::Written?Why::Written:t.rate>=kMinFastRate?d.why:Why::NotFast;}
 if(d.why==Why::LinkInvulnerable)t.geom_touch=true;   // an exact native pose reached Link while its Tg was off
 for(int j=0;j<2;++j){if(!d.write[j])continue;
  if(cp06f1_native_sphere_place(a,j,d.place[j].c.x,d.place[j].c.y,d.place[j].c.z,d.place[j].r)){++t.written;++f1.place_writes;}else ++f1.place_failed;}
 if(t.written){t.expect=true;f1_backfill_log(a,k,t,fr,np,d,S,native_cur);}
}
void on_eoc_post_cp06f1_coverage(ModContext*,void* args,void*,void*)noexcept{
 if(!f1.enabled||!args||g.eng.obs.disabled)return;
 auto* a=mods::arg<daE_OC_c*>(args,0);if(!a||(a->mActionMode!=4&&!f1.open_swings))return;   // O(1) outside attacks
 const int slot=cp04_slot(a);if(slot<0)return;
 const auto& l=g.eng.obs.ledger.slot(unsigned(slot));auto& S=f1.slots[slot];const std::uint64_t tick=g.p8.tick;
 if(S.id!=l.id||S.generation!=l.generation){f1_close(S,"IDENTITY_OR_GENERATION_CHANGED");S=F1Slot{};S.id=l.id;S.generation=l.generation;}
 daAlink_c* k=f1_link();
 mDoExt_McaMorfSO* m=a->mpMorf;J3DModel* model=m?m->getModel():nullptr;
 if(!model){f1_close(S,"NO_MODEL");S.valid=false;S.armed=false;return;}
 cp06f1::PassView pv;pv.registered=S.armed&&S.tick+1==tick;
 if(k)f1_read_pass(a,k,pv);
 cp06f1::feed_pass(S.swing,pv,tick);
 const int mode=a->mActionMode,state=a->mOcState;const float f=m->getFrame();
 const bool attack=mode==4&&(state==1||state==2),blow=mode==4&&state>=1&&state<=3;
 const bool consecutive=S.valid&&S.tick+1==tick&&S.state==state;
 if(S.swing.open&&(!blow||S.swing.last_tick+2<tick||(attack&&consecutive&&f+0.001f<S.frame)||(attack&&S.valid&&S.state!=state)))f1_close(S,!blow?"LEFT_ATTACK":"NEW_CLIP_OR_GAP");
 const bool reopened=attack&&!S.swing.open;
 if(reopened){++f1.open_swings;auto& s=S.swing;s=cp06f1::Swing{};s.open=true;s.id=l.id;s.generation=l.generation;s.seq=++f1.seq;s.start_tick=tick;s.last_tick=tick;s.clip=std::uint8_t(state);s.kind=f1_kind(a,slot,l);}
 const bool armed=attack&&f1.coverage&&a->mSphs_at[0].ChkAtSet()&&a->mSphs_at[1].ChkAtSet();
 const bool pose_consecutive=consecutive&&!reopened;   // a reopened swing never inherits the previous clip frame
 float now[3][4];std::memcpy(now,model->getAnmMtx(0xc),sizeof(now));
 if(armed){
  cp06f1::Tick t;t.armed=true;t.post=f;t.pre=pose_consecutive?S.frame:f-m->getPlaySpeed();t.rate=t.post-t.pre;++f1.armed_ticks;
  bool covered=false;f1_sample(a,k,S,t,pose_consecutive,state==1,now,covered);S.covered=covered;++f1.why[unsigned(t.why)<unsigned(cp06f1::Why::Count)?unsigned(t.why):0];
  cp06f1::feed_tick(S.swing,t);
 }
 if(S.swing.open&&blow)S.swing.last_tick=tick;
 if(!armed)S.covered=false;
 S.armed=armed;S.tick=tick;S.valid=true;S.frame=f;S.state=state;S.hand={now[0][3],now[1][3],now[2][3]};
}
void on_collision_post_cp06f1(ModContext*,void* args,void*,void*)noexcept{
 if(!f1.enabled||!args)return;auto* k=mods::arg<daAlink_c*>(args,0);
 if(k&&k==f1_link()){f1.link_set_tick=g.p8.tick;f1.link_set_key=reinterpret_cast<std::uintptr_t>(k);}
}
// ---------------------------------------------------------------- B. flash
void f1_flash_log(const char* event,const char* why)noexcept{
 if(!lnk_take(f1.flash_lines,f1.flash_dropped,160))return;char line[kLogLineMax];const auto& e=f1.flash.expansion();const auto& l=f1.flash.legit();
 std::snprintf(line,sizeof(line),"CP06F1_FLASH event=%s version=%s tick=%llu expansion_flash=%d/%d legit_flash=%d/%d uncertain=%u reason=%s scope=EXPANSION_CP19F2_RENDER_ONLY runtime=HUMAN_RUNTIME_PENDING",
  event,cp06f1::kVersion,(unsigned long long)g.p8.tick,e.dmg,e.col,l.dmg,l.col,unsigned(f1.flash.uncertain()),why);lnk_write(LOG_LEVEL_INFO,line);
}
void on_eoc_post_cp06f1_chip_before(ModContext*,void* args,void*,void*)noexcept{
 f1.chip.armed=false;if(!f1.enabled||!f1.flash_on||!args)return;auto* a=mods::arg<daE_OC_c*>(args,0);if(!a||a->profname!=fpcNm_E_OC_e)return;
 f1.chip.armed=true;f1.chip.key=reinterpret_cast<std::uintptr_t>(a);f1.chip.tick=g.p8.tick;f1.chip.comp=c6.compensated;f1.chip.pending=g_dComIfG_gameInfo.play.getItemLifeCount();
}
void on_eoc_post_cp06f1_chip_after(ModContext*,void* args,void*,void*)noexcept{
 if(!f1.chip.armed)return;f1.chip.armed=false;if(!f1.enabled||!f1.flash_on||!args)return;auto* a=mods::arg<daE_OC_c*>(args,0);
 if(reinterpret_cast<std::uintptr_t>(a)!=f1.chip.key||f1.chip.tick!=g.p8.tick)return;
 const auto seen=cp06f1::observe(f1.chip.pending,g_dComIfG_gameInfo.play.getItemLifeCount(),f1.chip.comp,c6.compensated);
 if(seen==cp06f1::ChipSeen::None)return;f1.flash.chip(seen);
 f1_flash_log(seen==cp06f1::ChipSeen::ParryNeutralized?"EXPANSION_FLASH_RESTARTED_BY_NEUTRALIZED_PARRY_CHIP":seen==cp06f1::ChipSeen::Legit?"LEGIT_EXPANSION_CHIP_FLASH_KEPT":"FOREIGN_PENDING_LIFE_CHANGE_UNCERTAIN",
  seen==cp06f1::ChipSeen::ParryNeutralized?"CP06_NEUTRALIZED_EXACT_ONE_UNIT_IN_THIS_EXECUTE":seen==cp06f1::ChipSeen::Legit?"EXACT_MINUS_ONE_NOT_NEUTRALIZED":"NOT_ATTRIBUTABLE_NO_SUPPRESSION_UNTIL_BOTH_EXPIRE");
}
bool f1_flash_model(const daAlink_c* k,const J3DModel* m)noexcept{
 // Exactly Expansion CP19F2's draw scope (cp19f2_flash_model): body/face/hat/hand and the demo face/hand models.
 return m==k->mpLinkModel||m==k->mpLinkFaceModel||m==k->mpLinkHatModel||m==k->mpLinkHandModel||m==k->mpDemoFCBlendModel||m==k->mpDemoFCTongueModel||m==k->mpDemoHLTmpModel||m==k->mpDemoHRTmpModel;
}
HookAction on_model_draw_pre_cp06f1_capture(ModContext*,void* args,void*,void*)noexcept{
 if(f1.depth>=4){++f1.draw_overflow;++f1.overflow_open;return HOOK_CONTINUE;}F1Draw& d=f1.draw[f1.depth++];d=F1Draw{};
 if(!f1.enabled||!f1.flash_on||!args||g.eng.obs.disabled)return HOOK_CONTINUE;
 auto* k=mods::arg<daAlink_c*>(args,0);J3DModel* m=mods::arg<J3DModel*>(args,1);
 if(!k||!m||k!=f1_link()||!f1_flash_model(k,m))return HOOK_CONTINUE;
 const GXColor& kc=k->tevStr.TevKColor;d.active=true;d.link=reinterpret_cast<std::uintptr_t>(k);d.model=reinterpret_cast<std::uintptr_t>(m);d.orig={kc.r,kc.g,kc.b};
 return HOOK_CONTINUE;
}
HookAction on_model_draw_pre_cp06f1_apply(ModContext*,void* args,void*,void*)noexcept{
 if(!f1.depth||!args)return HOOK_CONTINUE;F1Draw& d=f1.draw[f1.depth-1];if(!d.active)return HOOK_CONTINUE;
 auto* k=mods::arg<daAlink_c*>(args,0);if(reinterpret_cast<std::uintptr_t>(k)!=d.link||reinterpret_cast<std::uintptr_t>(mods::arg<J3DModel*>(args,1))!=d.model){d.active=false;return HOOK_CONTINUE;}
 const GXColor& kc=k->tevStr.TevKColor;const cp06f1::Rgb now{kc.r,kc.g,kc.b};cp06f1::Rgb out{};
 const auto r=f1.flash.decide(d.orig,now,out);
 if(r==cp06f1::Draw::Counterfactual){if(cp06f1_native_kcolor(k,out.r,out.g,out.b)){d.wrote=true;d.written=out;}else ++f1.kcolor_failed;}
 return HOOK_CONTINUE;
}
void on_model_draw_post_cp06f1(ModContext*,void* args,void*,void*)noexcept{
 if(f1.overflow_open){--f1.overflow_open;return;}   // innermost frames were never pushed
 if(!f1.depth)return;const F1Draw d=f1.draw[--f1.depth];if(!d.active||!d.wrote||!args)return;
 auto* k=mods::arg<daAlink_c*>(args,0);if(reinterpret_cast<std::uintptr_t>(k)!=d.link)return;
 // Expansion's POST(0) restores its own saved value (== orig) before this POST(-250). Restore only our exact value, never a foreign one.
 const GXColor& kc=k->tevStr.TevKColor;const cp06f1::Rgb now{kc.r,kc.g,kc.b};
 if(now==d.orig){if(d.written==d.orig)++f1.flash.c.restore_not_needed;else ++f1.flash.c.restored_by_expansion;}
 else if(now==d.written){if(cp06f1_native_kcolor(k,d.orig.r,d.orig.g,d.orig.b))++f1.flash.c.restored_by_us;else ++f1.kcolor_failed;}
 else ++f1.flash.c.restore_foreign;
}
// ---------------------------------------------------------------- lifecycle
void cp06f1_report(const char* reason)noexcept{
 char line[kLogLineMax*4];int n=std::snprintf(line,sizeof(line),"CP06F1_SUMMARY reason=%s version=%s source=%s coverage=%u flash=%u armed_ticks=%llu probes=%llu probe_mismatch=%llu writes=%llu write_failed=%llu native_formula_spheres=%llu foreign_sweep_spheres=%llu swings=%llu why=",
  reason,cp06f1::kVersion,POST05_SOURCE_COMMIT,unsigned(f1.coverage),unsigned(f1.flash_on),(unsigned long long)f1.armed_ticks,(unsigned long long)f1.probes,(unsigned long long)f1.probe_mismatch,(unsigned long long)f1.place_writes,(unsigned long long)f1.place_failed,(unsigned long long)f1.native_formula,(unsigned long long)f1.foreign_spheres,(unsigned long long)f1.swings);
 for(unsigned w=1;w<unsigned(cp06f1::Why::Count)&&n>0&&n<int(sizeof(line))-64;++w)if(f1.why[w])n+=std::snprintf(line+n,sizeof(line)-unsigned(n),"%s:%llu,",cp06f1::name(cp06f1::Why(w)),(unsigned long long)f1.why[w]);
 for(unsigned k=0;k<kF1Kinds&&n>0&&n<int(sizeof(line))-96;++k){bool any=false;for(unsigned r=1;r<kF1Results;++r)any=any||f1.result[k][r];if(!any)continue;
  n+=std::snprintf(line+n,sizeof(line)-unsigned(n)," %s[backfilled_swings=%llu backfill_contacts=%llu",cp06f1::name(cp06f1::Kind(k)),(unsigned long long)f1.backfilled_swings[k],(unsigned long long)f1.backfill_contacts[k]);
  for(unsigned r=1;r<kF1Results&&n>0&&n<int(sizeof(line))-48;++r)if(f1.result[k][r])n+=std::snprintf(line+n,sizeof(line)-unsigned(n)," %s=%llu",cp06f1::name(cp06f1::Result(r)),(unsigned long long)f1.result[k][r]);
  if(n>0&&n<int(sizeof(line))-2)n+=std::snprintf(line+n,sizeof(line)-unsigned(n),"]");}
 lnk_write(LOG_LEVEL_INFO,line);
 const auto& c=f1.flash.c;char fl[kLogLineMax*2];
 std::snprintf(fl,sizeof(fl),"CP06F1_SUMMARY part=FLASH reason=%s version=%s parry_restarts=%llu legit_chips=%llu foreign_changes=%llu passes=%llu draws=%llu expansion_writes=%llu counterfactual_writes=%llu red_removed=%llu legit_red_kept=%llu mismatch=%llu uncertain_draws=%llu restored_by_expansion=%llu restored_by_us=%llu restore_foreign=%llu restore_not_needed=%llu kcolor_failed=%llu draw_overflow=%llu lines(swing/backfill/flash)=%u+%llu/%u+%llu/%u+%llu coverage_limit=ZERO_COUNTER_IS_NOT_VISUAL_PROOF runtime=HUMAN_RUNTIME_PENDING",
  reason,cp06f1::kVersion,(unsigned long long)c.parry_starts,(unsigned long long)c.legit_starts,(unsigned long long)c.foreign,(unsigned long long)c.ticks,(unsigned long long)c.draws,(unsigned long long)c.expansion_writes,(unsigned long long)c.counterfactual,(unsigned long long)c.red_removed,(unsigned long long)c.red_kept_legit,(unsigned long long)c.mismatch,(unsigned long long)c.uncertain_draws,(unsigned long long)c.restored_by_expansion,(unsigned long long)c.restored_by_us,(unsigned long long)c.restore_foreign,(unsigned long long)c.restore_not_needed,(unsigned long long)f1.kcolor_failed,(unsigned long long)f1.draw_overflow,
  f1.swing_lines,(unsigned long long)f1.swing_dropped,f1.backfill_lines,(unsigned long long)f1.backfill_dropped,f1.flash_lines,(unsigned long long)f1.flash_dropped);
 lnk_write(LOG_LEVEL_INFO,fl);
}
void cp06f1_maintenance()noexcept{
 if(!f1.enabled)return;
 f1.depth=0;f1.overflow_open=0;   // no model draw is in progress during mod_update
 f1.flash.pass();   // Expansion's cp19f2_poll ticks its flash once per mod_update as well
 if(f1.open_swings&&g.p8.tick%30==0)for(auto& S:f1.slots)if(S.swing.open&&S.swing.last_tick+2<g.p8.tick)f1_close(S,"STALE_NO_EXECUTE");   // actor deleted or culled mid-swing
 if(g.p8.tick<f1.last_summary_tick)f1.last_summary_tick=g.p8.tick;
 if(g.p8.tick-f1.last_summary_tick>=1800){cp06f1_report("PERIODIC");f1.last_summary_tick=g.p8.tick;}
}
void cp06f1_summary()noexcept{for(auto& s:f1.slots)f1_close(s,"SHUTDOWN");cp06f1_report("SHUTDOWN");f1.enabled=false;f1.flash.reset();}
bool cp06f1_install()noexcept{
 HookOptions cov=HOOK_OPTIONS_INIT,cb=HOOK_OPTIONS_INIT,ca=HOOK_OPTIONS_INIT,dc=HOOK_OPTIONS_INIT,da=HOOK_OPTIONS_INIT,dv=HOOK_OPTIONS_INIT,lc=HOOK_OPTIONS_INIT;
 cov.priority=-300;cb.priority=91;ca.priority=-251;dc.priority=90;da.priority=-250;dv.priority=-250;lc.priority=-300;
 // POST before PRE on the draw target: a PRE write can never exist without its verifying POST.
 const ModResult r[]={mods::hook::add_post<LinkModelDrawF1>(on_model_draw_post_cp06f1,&dv),mods::hook::add_pre<LinkModelDrawF1>(on_model_draw_pre_cp06f1_capture,&dc),mods::hook::add_pre<LinkModelDrawF1>(on_model_draw_pre_cp06f1_apply,&da),
  mods::hook::add_post<Cp04EocExecute>(on_eoc_post_cp06f1_chip_before,&cb),mods::hook::add_post<Cp04EocExecute>(on_eoc_post_cp06f1_chip_after,&ca),
  mods::hook::add_post<LinkCollisionFx>(on_collision_post_cp06f1,&lc),mods::hook::add_post<Cp04EocExecute>(on_eoc_post_cp06f1_coverage,&cov)};
 bool ok=c6.enabled;for(auto x:r)ok=ok&&x==MOD_OK;
 f1.enabled=ok;f1.coverage=ok;f1.flash_on=ok;
 char line[kLogLineMax];std::snprintf(line,sizeof(line),"CP06F1_INIT version=%s source=%s enabled=%u coverage=EXACT_SKIPPED_NATIVE_POSE_ENGINE_MODELCALC_RESTORE_VERIFIED window=VANILLA_POSES_15_23 rates=GT1_LE2 write_margin=%.1f flash=EXPANSION_SCHEDULE_MIRROR_E_L_EXACT_VALUE_ONLY priorities=exec_post(+91/-251/-300) draw_pre(+90/-250) draw_post(-250) new_hook_targets=1 writers=3 runtime=HUMAN_RUNTIME_PENDING",
  cp06f1::kVersion,POST05_SOURCE_COMMIT,unsigned(ok),double(cp06f1::kWriteSlack));lnk_write(ok?LOG_LEVEL_INFO:LOG_LEVEL_WARN,line);
 return ok;
}
#undef CP06F1_NOINLINE
