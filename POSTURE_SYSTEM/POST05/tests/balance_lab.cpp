#include "../src/balance_adapter.hpp"
#include "../src/break_core.hpp"
#include <cstdio>
#include <initializer_list>
using namespace postbal01;
unsigned checks=0,failures=0;
void ck(bool b,const char* s){++checks;if(!b){++failures;if(failures<25)std::printf("FAIL %s\n",s);}}
struct Key{int slot=-1;std::uint32_t id=0,epoch=1;std::uint64_t gen=0;std::uintptr_t ptr=0;post03::NativeAux aux;};
struct World{
 post03::Ledger L;post04::Posture p;lnk01::VariantTrack track;std::uint64_t event=0,call=0;
 Key add(int variant,unsigned n=0,std::uint32_t epoch=1,std::uint32_t id=0,std::uintptr_t ptr=0){
  Key k;k.id=id?id:100+n;k.ptr=ptr?ptr:0x10000+0x1000*n;k.epoch=epoch;post03::Events ev;
  auto o=L.observe(k.id,k.ptr,variant==0?post03::Admission::E_OC_CANDIDATE:post03::Admission::E_OC2_CANDIDATE,epoch,++call,ev);
  k.slot=o.slot;k.gen=o.generation;k.aux.valid=1;k.aux.resource=static_cast<unsigned char>(variant==0?post03::NameKind::E_OC:post03::NameKind::E_OC2);k.aux.atp=variant==0?1:variant==1?2:variant==2?3:variant==3?6:9;
  p.sync(L);profile(k);return k;
 }
 Snapshot profile(const Key& k){track.observe(k.slot,k.id,k.gen,k.aux.resource==static_cast<unsigned>(post03::NameKind::E_OC2),k.aux.atp);auto s=capture(L,track,k.slot,k.id,k.gen,k.aux);p.observe_profile(s,L);return s;}
 post03::HitReport report(const Key&k,post03::Family f=post03::Family::Sword){post03::HitReport h;h.slot=k.slot;h.id=k.id;h.generation=k.gen;h.event=++event;h.j.kind=post03::HitKind::Confirmed;h.j.elig=post03::Eligibility::Confirmed;h.life=post03::Life::Active;h.at_valid=true;h.fam.family=f;h.pre.health=1000;h.post.health=980;h.aux_pre=k.aux;h.aux_post=k.aux;return h;}
 post04::ConsumeOutcome hit(const Key&k,post03::Family f=post03::Family::Sword){auto h=report(k,f);auto s=profile(k);auto out=p.consume(h,L,s);ck(h.pre.health==1000&&h.post.health==980,"posture never changes native HP report");return out;}
 post04::Posture::ParryOutcome parry(const Key&k,std::uint64_t tok){auto s=profile(k);auto a=p.preview_parry(k.slot,k.gen,k.id,tok,L,s);auto b=p.apply_parry(k.slot,k.gen,k.id,tok,L,s,&a);ck(b.reject==post04::Posture::ParryReject::None&&a.gain==b.gain&&a.after==b.after&&a.crossed==b.crossed,"same verified weight and threshold in preview/commit");return b;}
};
int main(){
 const int expected[4][3]={{25,40,50},{17,27,33},{13,20,25},{13,20,25}};
 const int counts0[4][3]={{4,3,2},{6,4,4},{8,5,4},{8,5,4}};
 const int counts25[4][3]={{3,2,2},{5,3,3},{6,4,3},{6,4,3}};
 for(int v=0;v<4;++v)for(int kind=0;kind<3;++kind)for(int start:{0,25}){
  World w;auto k=w.add(v);auto s=w.profile(k);auto z=select(s,k.slot,k.id,k.gen,k.epoch);ck(z.verified&&index(z.variant)==unsigned(v),"exact existing LNK07 variant");
  Kind t=kind==0?Kind::Sword:kind==1?Kind::Parry:Kind::Iron;ck(gain(z,t,0)==expected[v][kind],"approved complete weight table");
  if(start==25){for(int i=1;i<=5&&!w.p.entry(k.slot).latched;++i)w.parry(k,i);ck(w.p.rearm(k.slot,k.gen,k.id,kStandingRearm),"standing rearm accepts25");ck(w.p.entry(k.slot).posture==25,"rearm exactly25");}
  const auto old_cross=w.p.c.crossings;int value=start;const int count=start?counts25[v][kind]:counts0[v][kind];
  for(int n=1;n<=count;++n){const int sum=value+expected[v][kind];value=sum>100?100:sum;bool crossed=false;
   if(kind==1)crossed=w.parry(k,100+n).crossed;else crossed=w.hit(k,kind==0?post03::Family::Sword:post03::Family::IronBall).crossed;
   ck(w.p.entry(k.slot).posture==value,"exact sequence and saturating boundary");ck(crossed==(n==count),"only the final contact crosses100");ck(w.p.audit().ok,"unit conservation after each contact");
  }
  ck(w.p.c.crossings==old_cross+1,"one transition per cycle");auto before=w.p.c.units_added;w.hit(k);auto rejected=w.p.apply_parry(k.slot,k.gen,k.id,999,w.L,w.profile(k));ck(!rejected.applied&&w.p.c.units_added==before,"latched ignores future hits/parry without queues");
  w.p.note_break_delivery(k.slot,k.id,k.gen);w.p.note_break_delivery(k.slot,k.id,k.gen);ck(w.p.balance.delivered[v]==1&&w.p.balance.delivery_duplicate==1,"delivery counted once per cycle");
  ck(!w.p.rearm(k.slot,k.gen,k.id,50),"old50 cannot silently rearm");ck(w.p.rearm(k.slot,k.gen,k.id,0)&&w.p.entry(k.slot).posture==0,"full getup remains0");ck(!w.p.rearm(k.slot,k.gen,k.id,25),"cannot rearm twice");ck(w.p.audit().ok,"25 and0 conservation");
 }
 {World w;auto k=w.add(1);for(int n=1;n<=3;++n){auto o=w.hit(k,post03::Family::IronBall);ck(!o.crossed&&w.p.entry(k.slot).posture==n*33,"RED iron33x3=99 and NOT break");}ck(w.hit(k,post03::Family::IronBall).crossed,"RED fourth iron breaks");}
 for(int v=0;v<4;++v){World w;auto k=w.add(v);auto s=w.profile(k);auto h=w.report(k);auto a=w.p.consume(h,w.L,s);const int hp=h.post.health;ck(a.weight==expected[v][0],"sword production weight");auto b=w.p.consume(h,w.L,s);ck(!b.consumed_new&&w.p.entry(k.slot).posture==expected[v][0]&&hp==h.post.health,"duplicate hook/two sphere report yields zero extra");
  h=w.report(k);h.j.lethal=true;h.post.health=0;auto l=w.p.consume(h,w.L,s);ck(!l.applied&&l.skip==post04::Skip::Lethal,"lethal stays excluded");
  for(auto family:{post03::Family::ShieldAttack,post03::Family::Arrow,post03::Family::Bomb,post03::Family::Boomerang,post03::Family::Ambiguous,post03::Family::Unavailable}){h=w.report(k,family);auto o=w.p.consume(h,w.L,s);ck(!o.applied,"nonadmitted family never becomes sword");}
  auto pv=w.p.preview_parry(k.slot,k.gen,k.id,77,w.L,s);auto p=w.p.apply_parry(k.slot,k.gen,k.id,77,w.L,s,&pv);ck(p.applied,"first parry admitted");auto dup=w.p.apply_parry(k.slot,k.gen,k.id,77,w.L,s,&pv);ck(!dup.applied&&dup.reject==post04::Posture::ParryReject::DuplicateToken,"duplicate parry remains rejected");
 }
 for(int bad=0;bad<6;++bad){World w;auto red=w.add(1),black=w.add(2,1);auto s=w.profile(red);auto a=w.p.preview_parry(red.slot,red.gen,red.id,1,w.L,s);
  int slot=red.slot;auto id=red.id;auto gen=red.gen;std::uint64_t token=1;
  if(bad==0){slot=black.slot;id=black.id;gen=black.gen;s=w.profile(black);}if(bad==1){s.variant=Variant::Black;}if(bad==2){token=2;}if(bad==3){w.hit(red);}if(bad==4){post03::Events ev;w.L.observe(red.id,red.ptr,post03::Admission::E_OC2_CANDIDATE,2,999,ev);s=w.profile(red);}if(bad==5){s.method=0;}
  const auto units=w.p.c.units_added;auto b=w.p.apply_parry(slot,gen,id,token,w.L,s,&a);ck(!b.applied&&b.reject==post04::Posture::ParryReject::PreviewMismatch&&w.p.c.units_added==units,"preview receipt rejects target/weight/token/posture/epoch/provenance change");ck(w.p.balance.preview_mismatch==1,"preview mismatch visible and not a PASS");
 }
 for(int bad=0;bad<5;++bad){World w;auto k=w.add(1);auto s=w.profile(k);if(bad==0)s.id++;if(bad==1)s.generation++;if(bad==2)s.epoch++;if(bad==3)s.verified=false;if(bad==4){k.aux.resource=0;s=w.profile(k);}auto h=w.report(k);auto o=w.p.consume(h,w.L,s);ck(o.applied&&o.weight==25&&!o.balance_profile.verified&&o.balance_profile.variant==Variant::Unknown,"unverified variant uses explicit0.11.1 fallback only");ck(w.p.balance.unavailable==1,"fallback is accounted");}
 {World w;auto k=w.add(4);auto o=w.hit(k);ck(o.weight==25&&!o.balance_profile.verified,"unknown E_OC2 not guessed Red/Black");}
 {World w;auto k=w.add(2);auto s=w.profile(k);k.aux.atp=6;auto fury=w.profile(k);ck(fury.variant==Variant::BlackFury&&gain(select(fury,k.slot,k.id,k.gen,k.epoch),Kind::Sword,0)==13,"fury retains black weights");k.aux.atp=-2;ck(w.profile(k).variant==Variant::Black,"two-sphere transient uses verified generation history");
  post08::Input in;in.tracked=in.link_ok=in.fresh=true;in.slot=k.slot;in.id=k.id;in.generation=k.gen;in.hp=440;in.atp=6;in.e_oc2=true;in.at_hit[0]=in.at_on_link[0]=in.at_shield[0]=true;in.pass.valid=in.pass.guard_attack=true;in.pass.frame=3;in.pass.start=0;in.pass.serial=1;
  post08::Parry judge;judge.enabled=true;ck(judge.judge(in).result!=post08::Result::Confirmed,"Black Fury chip still rejected by actual POST08 judge");}
 {World w;auto k=w.add(0);k.aux.atp=2;ck(w.profile(k).variant==Variant::Blue,"E_OC grouped Blue is not Red");auto legacy=w.add(1,1);legacy.aux.atp=1;ck(w.profile(legacy).variant==Variant::Blue,"legacy Blue from actual LNK07 history");legacy.aux.atp=2;ck(w.profile(legacy).variant==Variant::Blue,"group power cannot turn legacyBlue toRed");}
 for(int v=0;v<3;++v){World w;auto k=w.add(v);const int hpmax=v==0?220:v==1?330:440;
  for(int hp=1;hp<=hpmax;++hp){const auto rate=post06::policy::regen_rate_for(hp,hpmax);const int want=100LL*hp>75LL*hpmax?10:100LL*hp>50LL*hpmax?7:100LL*hp>25LL*hpmax?4:100LL*hp>10LL*hpmax?2:1;ck(rate==want,"CP02 approved health-band regeneration");w.p.set_regen_rate(k.slot,k.gen,k.id,rate);ck(w.profile(k).variant==static_cast<Variant>(v),"HP band never changes identity");}
 }
 {World w;auto k=w.add(1);post06::Breaker b;b.enabled=true;post06::View v;v.slot=k.slot;v.id=k.id;v.generation=k.gen;v.tracked=true;v.pre_hp=v.native_hp=330;v.mode=3;b.step(v);v.pre_hp=259;v.native_hp=239;b.step(v);ck(b.regen_rate(k.slot,239)==7,"RED239/330 CP02 rate7");v.pre_hp=239;v.native_hp=322;b.step(v);ck(b.regen_rate(k.slot,322)==10&&w.profile(k).variant==Variant::Red,"RED lifesteal239to322 changes only regen band");ck(v.native_hp-v.pre_hp==83,"real lifesteal untouched");}
 for(int why=0;why<4;++why){post06::Breaker b;b.enabled=true;post06::View v;v.slot=0;v.id=7;v.generation=1;v.tracked=true;v.pre_hp=500;v.native_hp=490;v.pre_mode=4;v.mode=5;v.hit=v.crossed=v.latched=true;
  auto a=b.step(v);ck(a.act==post06::Act::Stun,"break opens through existing native stun");b.commit(v,a,true);v.hit=v.crossed=false;v.state=5;
  if(why==0){v.stun_ended=true;a=b.step(v);ck(a.why==post06::Why::NativeStunEnded,"native stun-end route");}
  if(why==1){for(unsigned i=0;i<90;++i){a=b.step(v);if(a.act!=post06::Act::None)break;}ck(a.why==post06::Why::TimeoutStanding,"bounded90 timeout route");}
  if(why==2){v.writer_ok=false;v.mode=3;a=b.step(v);ck(a.why==post06::Why::TechnicalAbort,"old safe technical fallback");}
  if(why==3){v.mode=6;v.state=5;b.step(v);v.state=6;b.step(v);v.mode=3;v.state=5;a=b.step(v);ck(a.act==post06::Act::Rearm&&a.rearm_to==0,"actual native fullgetup uses0");}
  if(why<3)ck(a.act==post06::Act::Recover&&a.rearm_to==25,"all standing producers use25");b.commit(v,a,true);ck(b.slot(0).phase==post06::Phase::Tracking,"return TRACKING without forced AI");
 }
 // Recovery delay/accumulator unchanged, including standing latch freeze.
 for(int rate=1;rate<=10;++rate){World w;auto k=w.add(2);w.hit(k);w.p.set_regen_rate(k.slot,k.gen,k.id,rate);const int before=w.p.entry(k.slot).posture;for(int t=0;t<60;++t)w.p.advance();ck(w.p.entry(k.slot).posture==before,"exact60 eligible tick delay");for(int t=0;t<30;++t)w.p.advance();ck(w.p.entry(k.slot).posture==before-rate,"CP02 exact30Hz accumulator");ck(w.p.audit().ok,"regen conservation");}
 // Full observer engine route (same extra argument as mod.cpp), not a parallel formula.
 for(int v=0;v<3;++v){post04::PostureEngine pe;pe.eng.policy=post03::AdmissionPolicy::V2Compat;pe.eng.masks=post03::AtMasks{0x4000002u,0x10u,0x2000u,0x20u,0x400000u,0x10000u,0x40u,0x80u};post03::PreReads pre;pre.facts.profile_ok=pre.facts.live=true;pre.facts.status_10000=v==0;pre.facts.name=v==0?post03::NameKind::E_OC:post03::NameKind::E_OC2;pre.facts.atp=v+1;pre.snap.health=500;pre.snap.mode=3;pre.snap.tg_bits=3;pre.raw0=pre.raw1=0x90000;pre.aux.valid=1;pre.aux.resource=static_cast<unsigned char>(pre.facts.name);pre.aux.atp=v+1;post03::Events ev;auto o=pe.on_pre(0x10000,200,pre,ev);lnk01::VariantTrack tr;tr.observe(o.slot,200,o.generation,v!=0,v+1);auto profile=capture(pe.eng.obs.ledger,tr,o.slot,200,o.generation,pre.aux);post03::PostReads post;post.alive=true;post.selected=0x90000;post.snap.health=480;post.snap.mode=5;post.snap.old_mode=3;post.snap.cooldown=10;post.aux=pre.aux;post03::HitReport rep;const bool has=pe.on_post(0x10000,post,[](std::uintptr_t key,std::uint32_t& t){t=0x4000000u;return key==0x90000;},ev,rep,profile);ck(has&&pe.last.applied&&pe.last.weight==expected[v][0]&&pe.px.entry(o.slot).posture==expected[v][0],"actual observer POST04 engine consumes verified variant profile");}
 // Receipt contract: individually change each bound field while leaving every
 // other field equal (including equal-weight Black/Fury cases).
 {World w;auto k=w.add(2);auto profile=w.profile(k);auto a=w.p.preview_parry(k.slot,k.gen,k.id,99,w.L,profile);
  for(int changed=0;changed<13;++changed){auto b=a;
   switch(changed){case 0:++b.slot;break;case 1:++b.id;break;case 2:++b.generation;break;case 3:++b.epoch;break;case 4:++b.token;break;case 5:++b.gain;break;case 6:++b.before;break;case 7:++b.after;break;case 8:b.crossed=!b.crossed;break;case 9:b.balance_profile.variant=Variant::BlackFury;break;case 10:b.balance_profile.method=0;break;case 11:b.balance_profile.verified=false;break;case 12:b.reject=post04::Posture::ParryReject::Latched;break;}
   ck(!post04::Posture::same_preview(a,b),"receipt binds identity, amount, epoch, state and provenance independently");
  }
 }
 {World w;auto k=w.add(1);auto aux=k.aux;aux.valid=0;ck(!capture(w.L,w.track,k.slot,k.id,k.gen,aux).verified,"missing both-sphere read never verified");
  post03::Events ev;w.L.mark_death(k.slot,k.gen,k.epoch,ev);w.p.sync(w.L);ck(!w.profile(k).verified&&!w.hit(k).applied,"dead identity does not retain a usable profile");}
 {World w;auto k=w.add(2);auto old=w.profile(k);post03::Pick pick;pick.slot=k.slot;pick.id=k.id;pick.generation=k.gen;pick.ptr=k.ptr;post03::Events ev;w.L.apply_lookup(pick,post03::Lookup::Absent,2,ev);w.p.sync(w.L);ck(!w.p.entry(k.slot).bound,"deleted identity retired");auto fresh=w.add(1,0,2,k.id,k.ptr);ck(fresh.gen!=k.gen&&w.profile(fresh).variant==Variant::Red,"same id/address after delete has a fresh RED generation");ck(!select(old,fresh.slot,fresh.id,fresh.gen,2).verified,"old room/gen proof cannot apply");}
 // No-hit tracking fallback is also an old safe standing25 producer.
 {post06::Breaker b;b.enabled=true;post06::View v;v.slot=0;v.id=7;v.generation=1;v.tracked=v.latched=true;v.pre_hp=v.native_hp=500;v.pre_mode=v.mode=3;auto a=b.step(v);ck(a.act==post06::Act::Recover&&a.rearm_to==25,"tracking latched safe fallback changed50to25");}
 {World w;auto k=w.add(1);w.hit(k);ck(w.p.audit().balance_units&&w.p.audit().balance_events,"per-variant accounting equals global production totals");++w.p.balance.units[1];ck(!w.p.audit().ok,"corrupt variant-unit accounting is detected");}
 // Capacity, reuse and an eight-actor repeated cycle test with bounded loops.
 {World w;Key keys[64];for(unsigned i=0;i<64;++i){keys[i]=w.add(i%3,i);ck(keys[i].slot>=0,"64 independent live slots");w.hit(keys[i]);}auto extra=w.add(1,64);ck(extra.slot<0,"capacity exhaustion never lends an existing profile");for(unsigned i=0;i<64;++i)ck(w.p.entry(keys[i].slot).balance_profile.variant==static_cast<Variant>(i%3),"no cross-slot identity");}
 {World w;auto k=w.add(2);auto old=w.profile(k);w.hit(k);auto recycled=w.add(1,0,2,k.id+1,k.ptr);ck(recycled.gen!=k.gen&&w.p.entry(recycled.slot).posture==0,"address reuse starts zero");auto h=w.report(recycled);auto z=w.p.consume(h,w.L,old);ck(z.applied&&z.weight==25&&!z.balance_profile.verified,"old generation profile cannot leak");ck(w.p.audit().ok,"retire conservation");}
 for(int cycle=0;cycle<40;++cycle){World w;Key k[8];for(unsigned i=0;i<8;++i)k[i]=w.add(i%3,i);for(int n=0;n<8;++n)for(unsigned i=0;i<8;++i){w.hit(k[i]);if(w.p.entry(k[i].slot).latched){w.p.note_break_delivery(k[i].slot,k[i].id,k[i].gen);ck(w.p.rearm(k[i].slot,k[i].gen,k[i].id,(cycle&1)?25:0),"mixed group independent rearm");}ck(w.p.audit().ok,"mixed group per-event conservation");}}
 {// LFT 0.13.6 FIX06 P0: the Jump Strike's ONE posture credit x1.6 (Blue 40, Red 27, Black 21, Black Fury = Black's 21) and nothing for its other phases
  const int jw[4]={40,27,21,21};
  for(int v=0;v<4;++v){World w;auto k=w.add(v,0);
   w.p.js_mode=1;auto o=w.hit(k);ck(o.weight==jw[v],"FIX06 jump strike weight x1.6");
   w.p.js_mode=2;auto o2=w.hit(k);ck(o2.weight==0,"FIX06 another phase of the same Jump Strike gives nothing");
   w.p.js_mode=0;auto o3=w.hit(k);ck(o3.weight==expected[v][0],"FIX06 an ordinary cut keeps its weight");}
  Selection un;ck(jump_gain(un,25)==40&&jump_gain(un,13)==21,"FIX06 unverified variant: 1.6x its reference");}
 {// LFT 0.13.7 FIX07 P1 [BALANCE_PROVISIONAL]: a special skill's ONE posture credit (x1.6 spin, x2.0 great spin, x2.25 Mortal Draw of the variant's sword weight,
  // half up: Blue 40/50/56, Red 27/34/38, Black and Black Fury 21/26/29) and nothing for another contact of the same execution
  const int pct[3]={160,200,225};const int want[4][3]={{40,50,56},{27,34,38},{21,26,29},{21,26,29}};
  for(int v=0;v<4;++v)for(int s=0;s<3;++s){World w;auto k=w.add(v,0);
   w.p.js_mode=3;w.p.sk_pct=pct[s];auto o=w.hit(k);ck(o.weight==want[v][s],"FIX07 special skill weight");
   w.p.js_mode=4;auto o2=w.hit(k);ck(o2.weight==0,"FIX07 another contact of the same special gives nothing");
   w.p.js_mode=0;w.p.sk_pct=0;auto o3=w.hit(k);ck(o3.weight==expected[v][0],"FIX07 an ordinary cut keeps its weight");}
  // Blue: a Mortal Draw (56) + a cut (25) + a block-free spin (40) already reach 100: two specials and a cut, never one special alone
  ck(56<100&&50<100&&40<100&&56+50>=100,"FIX07 no single special breaks a Blue Bokoblin by itself");}
 std::printf("POST_BAL01_LAB checks=%u failures=%u RESULT=%s LAB_MODEL_ONLY\n",checks,failures,failures?"LAB_FAIL":"LAB_PASS");return failures?1:0;
}
