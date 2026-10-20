// CP14 offline lab: the RED abilities on the REAL CP14 runtime headers (red_policy.hpp and the three patched shared headers). No game, no GPU.
//  * identity: native E_OC2 -> Red 330 / 3 HP/s / atp 2; DEVF Black and DEVF Normal registrations keep their variants;
//  * lifesteal: 25 % of the maximum with the milli-HP accumulator (82, 83, ...), cap, terminal Red;
//  * attribution: claim (shield on either side, invulnerability, damage status, non-Red collider) and confirmation (life drop + timer restart);
//  * group window: 120 ticks, renew without stacking, terminal end, x2 for the Red and for Blue/Red/Black in range, out of range at once, re-entry,
//    two Reds never x4, other room / dead / stale companions excluded;
//  * visuals: same strokes route in gold for Red, Blue/Black colours/route/geometry unchanged, gold eyes 0 -> 7 -> 0, cleanup;
//  * regression of the patched shared headers (regeneration step with mult 1 == the CP10.3 rule, record() rules, colours).
#include "../runtime/DEVF/CP14_RED_BOKOBLIN_LIFESTEAL_GROUP_REGEN_GOLD/expansion/red_policy.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
namespace rd=devf_red;namespace vv=devf_vis;namespace dy=devf_dyn;namespace sk=devf_stk;using devf_variant::Variant;using devf_tex::RGB;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
// The CP10.3/CP10.4 regeneration rule exactly as shipped before CP14 (reference for the mult=1 parity check).
devf_variant::Step ref_step(devf_variant::Entry& e,std::int16_t health,int action){
 devf_variant::Step s;s.new_health=health;++e.ticks;
 if(e.rejected||e.variant==Variant::None){s.skip="REJECTED";return s;}
 if(health<=0){e.acc=0;s.skip="DEAD";return s;}
 if(devf_variant::blocks_regen(action)){e.acc=0;s.skip="IRREVERSIBLE_OR_SCRIPTED_ACTION";return s;}
 if(health>=e.max_hp){e.acc=0;s.skip=health==e.max_hp?"FULL":"ABOVE_MAX_UNTOUCHED";return s;}
 unsigned acc=e.acc+e.rate;const unsigned heal=acc/devf_variant::kTicksPerSecond;e.acc=static_cast<std::uint8_t>(acc%devf_variant::kTicksPerSecond);
 if(!heal){s.skip="FRACTION_ACCUMULATED";return s;}
 int target=int(health)+int(heal);if(target>e.max_hp)target=e.max_hp;
 s.heal=target-health;s.new_health=static_cast<std::int16_t>(target);
 const std::uint32_t before=e.healed;e.healed+=static_cast<std::uint32_t>(s.heal);
 s.milestone=before==0||(before/devf_variant::kHealLogEveryHp)!=(e.healed/devf_variant::kHealLogEveryHp);return s;
}
devf_variant::Entry entry_for(Variant v){
 devf_variant::Entry e;e.used=true;e.classified=true;e.variant=v;
 e.max_hp=v==Variant::Blue?devf_variant::kMaxHpBlue:v==Variant::Red?devf_variant::kMaxHpRed:440;
 e.rate=v==Variant::Blue?devf_variant::kRateBlue:v==Variant::Red?devf_variant::kRateRed:devf_variant::kRateBlack;return e;
}
// HP regenerated in `ticks` ticks from `hp` with a constant multiplier.
int regen(Variant v,int hp,unsigned ticks,unsigned mult){auto e=entry_for(v);std::int16_t h=std::int16_t(hp);for(unsigned i=0;i<ticks;++i){auto s=devf_variant::step(e,h,0,mult);h=s.new_health;}return h-hp;}
// ---- a little world of Bokoblins on the REAL CP10.6 ledger + CP14 side table ----
struct World {
 dy::Ledger L;std::array<rd::State,dy::kCapacity> S{};std::uint32_t seq=0;int ids=0;
 struct Io{std::uint8_t v[2]{2,2};std::uint8_t get(unsigned i)const{return v[i];}void set(unsigned i,std::uint8_t x){v[i]=x;}};
 std::vector<Io> io=std::vector<Io>(dy::kCapacity);
 dy::Entry* add(Variant v,float x,float z,int room=1){
  static int addr[dy::kCapacity];const int i=ids++;
  auto* e=L.record(std::uint32_t(100+i),&addr[i],std::uint32_t(1+i),v,devf_damage::target_for(v));require(e,"ledger record");
  e->owned=false;post(e,x,z,room);return e;
 }
 void post(dy::Entry* e,float x,float z,int room=1,int health=100,int action=0){
  ++seq;Io& d=io[std::size_t(e-L.entries.data())];dy::Post in;in.post_health=health;in.action=action;in.room=room;in.x=x;in.y=0;in.z=z;in.stage=7;in.seq=seq;in.window=dy::window_for(L.used);
  dy::post_update(L,*e,d,in,[](const dy::Entry&){return true;});
 }
 rd::State& st(dy::Entry* e){return rd::state_for(S,std::size_t(e-L.entries.data()),e->id,e->generation);}
 unsigned mult(dy::Entry* e){const bool own=e->variant==Variant::Red&&rd::window_open(st(e));const auto* src=rd::ally_source(L,S,*e,seq,dy::window_for(L.used));return rd::regen_multiplier(own,src!=nullptr);}
 void open(dy::Entry* red){auto& s=st(red);s.pending=1;rd::red_post(s,200,330,0);}
};
// ---- attribution fixtures: addresses only, as in the mod ----
int link_obj,cyl_obj[3],red_sph[2][2],other_obj;
rd::LinkSnap link_hit(int red,int sphere,int cyl=0){
 rd::LinkSnap L;L.present=true;L.canary=true;L.link=&link_obj;L.id=7;L.timer=0;L.life=40;L.pending=0;L.dmg=2;
 for(int i=0;i<3;++i)L.cyl[i].self=&cyl_obj[i];
 L.cyl[cyl].raw_hit=true;L.cyl[cyl].hit_obj=&red_sph[red][sphere];return L;
}
rd::RedSnap red_snap(int red,int cyl=0){rd::RedSnap r;r.live=true;r.sph[0]=&red_sph[red][0];r.sph[1]=&red_sph[red][1];r.at_hit[0]=true;r.at_obj[0]=&cyl_obj[cyl];return r;}
// ---- visual fakes ----
struct FakeEn {
 std::vector<std::uint16_t> ids;std::vector<sk::Colour> colours;const sk::Geometry* geo=nullptr;unsigned sets=0,stops=0;std::uint32_t next=1;
 bool probe(std::uint16_t,const char*&){return true;}bool ready(){return true;}
 std::uint32_t set(std::uint16_t id,std::uint32_t key,float,float,float){++sets;ids.push_back(id);return key?key:next++;}
 bool configure(std::uint32_t,const sk::Geometry& g,const sk::Colour& c,int alpha,bool,sk::Readback& rb){geo=&g;colours.push_back(c);rb.ok=true;rb.geo_ok=true;rb.presence_ok=true;rb.a=alpha;return true;}
 void stop(std::uint32_t){++stops;}
};
struct EyeFx {
 sk::AuraPart<FakeEn>* part;unsigned tex=0;std::vector<unsigned> seq;const char* last="NONE";
 bool aura(vv::Entry& e,int l,int a){const bool ok=part->aura(e,l,a);last=part->why;return ok;}void aura_release(vv::Entry& e){part->aura_release(e);}
 bool eye(vv::Entry&,unsigned idx){tex=idx;seq.push_back(idx);return true;}const char* reason()const{return last;}
};
std::vector<std::uint8_t> atlas(RGB e0,RGB e1,unsigned eye_blocks){
 std::vector<std::uint8_t> a(devf_tex::cmpr_level_bytes(128,128));const RGB s0{120,90,70},s1{80,60,45};
 for(std::size_t o=0,i=0;o<a.size();o+=8,++i){const bool eye=i<eye_blocks;std::uint16_t p0=devf_tex::pack565(eye?e0:s0),p1=devf_tex::pack565(eye?e1:s1);if(p0<=p1){const auto t=p0;p0=p1;p1=t;}if(p0==p1)++p0;
  devf_tex::wr16(a.data()+o,p0);devf_tex::wr16(a.data()+o+2,p1);devf_tex::wr32(a.data()+o+4,0x1B1B1B1Bu);}
 return a;
}
int main(){
 // ---------- 1. identity ----------
 {using namespace devf_variant;
  auto p=make_plan("E_OC2",kNativeHpE_OC2,kNativeHpE_OC2,true,Registration::None,0);
  require(p.variant==Variant::Red&&p.source==Source::NativeEOC2&&p.max_hp==330&&p.initial_hp==330&&p.rate==3&&p.write_hp,"native E_OC2 -> RED 330 HP, 3 HP/s");
  require(devf_damage::target_for(Variant::Red)==2&&dy::kAtpRed==2,"Red base attack stays 2 (CP10.5 / CP10.6, never buffed)");
  auto b=make_plan("E_OC2",kNativeHpE_OC2,kNativeHpE_OC2,true,Registration::Black,440);require(b.variant==Variant::Black&&b.max_hp==440&&b.rate==4,"exact DEVF Black registration stays BLACK 440 / 4 HP/s");
  auto n=make_plan("E_OC2",kNativeHpE_OC2,kNativeHpE_OC2,true,Registration::Normal,0);require(n.variant==Variant::Blue&&n.max_hp==220&&n.rate==2,"exact DEVF Normal registration stays BLUE");
  auto o=make_plan("E_OC",kNativeHpE_OC,kNativeHpE_OC,true,Registration::None,0);require(o.variant==Variant::Blue,"native E_OC stays BLUE");
  auto f=make_plan("E_OC2",kNativeHpE_OC2,kNativeHpE_OC2,true,Registration::BlackAdmissionFailed,0);require(f.variant==Variant::None,"failed Black admission is never turned into Red");
  pass("identity_native_e_oc2_is_red_devf_exceptions_respected");}
 // ---------- 2. lifesteal ----------
 {std::uint16_t acc=0;
  auto h=rd::lifesteal(acc,100,330);require(h.heal==82&&h.new_health==182&&acc==500&&!h.capped,"first hit: 82 (82.5, remainder 0.5 carried)");
  h=rd::lifesteal(acc,100,330);require(h.heal==83&&h.new_health==183&&acc==0,"second hit: 83 (0.5 + 82.5 = 83.0)");
  unsigned total=0;std::uint16_t a2=0;for(int i=0;i<1000;++i){auto x=rd::lifesteal(a2,0+1,330);total+=x.whole;}require(total==82500,"1000 hits = 82500 HP exactly: 82.5 per hit on average, no drift");
  std::uint16_t a3=0;h=rd::lifesteal(a3,300,330);require(h.heal==30&&h.new_health==330&&h.capped&&a3==500,"cap at the 330 maximum; remainder still carried");
  std::uint16_t a4=0;h=rd::lifesteal(a4,330,330);require(h.heal==0&&h.new_health==330&&h.capped,"full HP: nothing to heal");
  std::uint16_t a5=0;h=rd::lifesteal(a5,400,330);require(h.heal==0&&h.new_health==400,"an HP above the maximum (someone else's) is never lowered");
  require(rd::kLifestealPm*330%1000==500,"330 x 25 % = 82.5: the fraction is real and documented");
  pass("lifesteal_25pct_of_max_milli_accumulator_and_cap");}
 // ---------- 3. attribution ----------
 {rd::Claim c;auto L=link_hit(0,0);auto r=red_snap(0);
  require(rd::make_claim(L,true,r,c)==rd::Why::Claimed&&c.armed&&c.e0==40.0f&&c.cyl==0&&c.sphere==0,"Red sphere on Link's selected cylinder, non-shield, timer 0: claimed");
  auto now=L;now.timer=30;now.life=36;require(rd::confirm(c,now)==rd::Why::Confirmed,"life 40 -> 36 and invulnerability restarted: CONFIRMED");
  now=L;now.timer=30;now.life=40;now.pending=-4.0f;require(rd::confirm(c,now)==rd::Why::Confirmed,"pending -4 not yet moved by METER2: still a real drop");
  now=L;now.timer=30;now.life=40;require(rd::confirm(c,now)==rd::Why::NoLifeLoss,"timer restarted but no life lost (magic armour pays in rupees): NOT credited");
  now=L;now.timer=0;now.life=36;require(rd::confirm(c,now)==rd::Why::NotProcessedByLink,"life dropped but Link did not run setDamagePoint for a tg hit: NOT credited");
  now=L;now.link=&other_obj;now.timer=30;now.life=36;require(rd::confirm(c,now)==rd::Why::LinkChanged,"another Link object: NOT credited");
  pass("claim_and_confirm_only_on_real_life_loss");}
 {rd::Claim c;auto L=link_hit(0,0);auto r=red_snap(0);
  L.cyl[0].shield=true;require(rd::make_claim(L,true,r,c)==rd::Why::LinkShield&&!c.armed,"shield bit on Link's cylinder: blocked, no claim");
  L=link_hit(0,0);r.at_shield[0]=true;require(rd::make_claim(L,true,r,c)==rd::Why::RedSideShield&&!c.armed,"shield bit on the Red's sphere: blocked, no claim");
  r=red_snap(0);r.at_hit[0]=false;require(rd::make_claim(L,true,r,c)==rd::Why::RedSideMismatch,"no reverse hit on the Red's side: no claim");
  r=red_snap(0);L.timer=5;require(rd::make_claim(L,true,r,c)==rd::Why::LinkInvulnerable,"Link invulnerable (timer 5): the engine ignores the hit, no claim");
  L.timer=2;require(rd::make_claim(L,true,r,c)==rd::Why::LinkInvulnerable,"timer 2 is still running after Link decrements it: no claim");
  L.timer=1;require(rd::make_claim(L,true,r,c)==rd::Why::Claimed&&c.timer0==1,"timer 1 ends in this execute: claimed");
  auto now=L;now.timer=1;now.life=36;require(rd::confirm(c,now)==rd::Why::NotProcessedByLink,"timer 1 -> 1 (not restarted): NOT credited");
  now.timer=30;require(rd::confirm(c,now)==rd::Why::Confirmed,"timer 1 -> 30 + life drop: credited");
  L=link_hit(0,0);L.dmg=0;require(rd::make_claim(L,true,r,c)==rd::Why::NoDamageStatus,"damage status 0: no claim");
  L=link_hit(0,0);L.pending=4.0f;require(rd::make_claim(L,true,r,c)==rd::Why::PositivePending,"a heart in flight could hide or fake a drop: no claim");
  L=link_hit(0,0);L.life=0;require(rd::make_claim(L,true,r,c)==rd::Why::LinkLifeZero,"Link at 0: no claim");
  L=link_hit(0,0);L.canary=false;require(rd::make_claim(L,true,r,c)==rd::Why::LayoutCanary,"layout canary failed: never claims");
  L=link_hit(0,0);L.cyl_mask=1;require(rd::make_claim(L,true,r,c)==rd::Why::CylinderMask,"engine skips the cylinders: no claim");
  L=link_hit(0,0);r.live=false;require(rd::make_claim(L,true,r,c)==rd::Why::RedNotLive,"Red gone or replaced: no claim");
  pass("shield_invulnerability_and_no_damage_never_claimed");}
 {rd::Claim c;auto L=link_hit(1,0);auto r=red_snap(0);
  require(rd::make_claim(L,true,r,c)==rd::Why::NotARedCollider,"the hit object is another Red's sphere: not this Red");
  L=link_hit(0,1);L.cyl[0].hit_obj=&other_obj;require(rd::make_claim(L,false,r,c)==rd::Why::NotARedCollider,"a non-Red collider (other enemy) is never credited to a Red");
  L=link_hit(0,1,0);L.cyl[0].hit_obj=&other_obj;L.cyl[1].raw_hit=true;L.cyl[1].hit_obj=&red_sph[0][0];
  require(rd::selected_cylinder(L)==0&&rd::make_claim(L,true,r,c)==rd::Why::NotARedCollider,"Link uses the FIRST hit cylinder (another enemy): the Red on cylinder 1 is not credited");
  L=link_hit(0,1,2);r=red_snap(0,2);require(rd::make_claim(L,true,r,c)==rd::Why::Claimed&&c.sphere==1&&c.cyl==2,"second sphere on cylinder 2: claimed with its indices");
  pass("attribution_uses_the_engine_selected_cylinder_and_collider");}
 // ---------- 4. window ----------
 {rd::State s;s.pending=1;auto o=rd::red_post(s,200,330,0);
  require(o.applied==1&&o.hp_after==282&&o.window_ev==rd::Ev::WindowStart&&s.window==rd::kWindowTicks,"hit applied: +82 and the window opens at 120");
  unsigned bonus=1;rd::Ev last=rd::Ev::None;unsigned t=0;
  while(true){auto x=rd::red_post(s,282,330,0);++t;if(rd::window_open(s))++bonus;else{last=x.window_ev;break;}}
  require(bonus==120&&t==120&&last==rd::Ev::WindowEnd,"exactly 120 bonus ticks (0..119) = 4.000 s, closed on tick 120");
  s.pending=1;rd::red_post(s,282,330,0);for(int i=0;i<60;++i)rd::red_post(s,282,330,0);require(s.window==60,"60 ticks later: 60 left");
  s.pending=1;o=rd::red_post(s,282,330,0);require(o.window_ev==rd::Ev::WindowRenew&&s.window==120,"a new effective hit renews to 120, no stacking");
  s.pending=1;o=rd::red_post(s,0,330,0);require(o.window_ev==rd::Ev::WindowEnd&&s.window==0&&o.discarded==1&&o.applied==0&&o.hp_after==0,"dead Red: window closed, pending heal discarded, nothing written");
  rd::State t2;t2.window=50;o=rd::red_post(t2,100,330,devf_variant::A_FALL_DEAD);require(o.window_ev==rd::Ev::WindowEnd&&t2.window==0,"terminal action closes the window at once");
  pass("window_120_ticks_renew_without_stacking_terminal_end");}
 // ---------- 5. group regeneration ----------
 {require(regen(Variant::Red,100,30,1)==3&&regen(Variant::Red,100,30,2)==6,"Red 3 -> 6 HP/s");
  require(regen(Variant::Blue,100,30,1)==2&&regen(Variant::Blue,100,30,2)==4,"Blue 2 -> 4 HP/s");
  require(regen(Variant::Black,100,30,1)==4&&regen(Variant::Black,100,30,2)==8,"Black 4 -> 8 HP/s");
  require(regen(Variant::Red,100,120,2)==24,"a full 4 s window regenerates the Red 24 HP (6 x 4)");
  pass("regeneration_rates_doubled_exactly");}
 {World w;auto* red=w.add(Variant::Red,0,0);auto* blue=w.add(Variant::Blue,5000,0);
  w.open(red);w.post(red,0,0);
  require(w.mult(red)==2&&w.mult(blue)==1,"Red alone: x2 only for the Red (window open with no companion in range)");
  pass("red_alone_window_benefits_only_the_red");}
 {World w;auto* red=w.add(Variant::Red,0,0);auto* blue=w.add(Variant::Blue,100,0);auto* black=w.add(Variant::Black,0,300);
  require(w.mult(blue)==1&&w.mult(black)==1&&w.mult(red)==1,"no window: everybody x1");
  w.open(red);w.post(red,0,0);w.post(blue,100,0);w.post(black,0,300);
  require(w.mult(blue)==2&&w.mult(black)==2&&w.mult(red)==2,"window open: Red, Blue (100) and Black (300) all x2 of their own rate");
  w.post(blue,800,0);require(w.mult(blue)==1,"Blue leaves the 650 XZ range: x1 on the very next tick");
  w.post(blue,640,0);require(w.mult(blue)==2,"Blue comes back while the window is open: x2 again");
  w.post(black,0,300,2);require(w.mult(black)==1,"another room: no bonus");
  w.post(black,0,300,1,0);require(w.mult(black)==1,"dead companion: no bonus");
  w.post(blue,100,0);w.st(red).window=0;require(w.mult(blue)==1&&w.mult(red)==1,"window closed: everybody back to x1");
  pass("allies_in_range_x2_leave_and_reenter_room_and_death");}
 {World w;auto* r1=w.add(Variant::Red,0,0);auto* r2=w.add(Variant::Red,50,0);auto* blue=w.add(Variant::Blue,0,50);
  w.open(r1);w.open(r2);w.post(r1,0,0);w.post(r2,50,0);w.post(blue,0,50);
  require(w.mult(blue)==2&&w.mult(r1)==2&&w.mult(r2)==2,"two Reds with open windows: still x2 (never x4) for everybody");
  require(regen(Variant::Blue,100,30,w.mult(blue))==4,"Blue in range of two Reds regenerates 4 HP/s, not 8");
  pass("two_reds_never_stack");}
 {World w;auto* red=w.add(Variant::Red,0,0);auto* blue=w.add(Variant::Blue,100,0);w.open(red);w.post(red,0,0);
  for(unsigned i=0;i<dy::window_for(w.L.used)+2;++i)w.post(blue,100,0);
  require(w.mult(blue)==1,"a Red that stopped executing (culled / gone) gives no bonus once its snapshot is stale");
  auto& s=w.st(red);const std::uint32_t gen=red->generation;rd::State& again=rd::state_for(w.S,std::size_t(red-w.L.entries.data()),red->id,gen+1);
  require(again.window==0&&again.pending==0&&again.acc_milli==0&&!rd::window_open(again)&&s.window==0,"a new generation in the same slot inherits nothing");
  pass("stale_red_and_new_generation_inherit_nothing");}
 // ---------- 6. visuals ----------
 {require(sk::resource_for(Variant::Red)==sk::kResource&&sk::resource_for(Variant::Blue)==sk::kResource&&sk::resource_for(Variant::Black)==sk::kResource&&sk::resource_for(Variant::None)==0,"Red uses the shared 0x0B9D route; unclassified never");
  const auto& g=sk::colour_for(Variant::Red);require(g.r==255&&g.g==190&&g.b==30&&g.b<g.g&&g.g<g.r,"Red aura is gold (255,190,30)");
  const auto& bl=sk::colour_for(Variant::Blue);const auto& bk=sk::colour_for(Variant::Black);
  require(bl.r==80&&bl.g==190&&bl.b==255&&bl.er==30&&bl.eg==90&&bl.eb==170&&bk.r==255&&bk.g==32&&bk.b==24&&bk.er==130&&bk.eg==0&&bk.eb==6,"Blue and Black colours unchanged (CP12)");
  require(&sk::geometry_for(Variant::Red)==&sk::kGeometry&&&sk::geometry_for(Variant::Blue)==&sk::kGeometry,"one shared geometry object for every variant");
  vv::Ledger L;int a=0,b=0,c=0;require(L.record(1,&a,1,Variant::Red)&&L.record(2,&b,1,Variant::Blue)&&L.record(3,&c,1,Variant::Black)&&!L.record(4,&c,1,Variant::None),"visual ledger: Red accepted, unclassified still refused");
  pass("gold_route_and_unchanged_blue_black");}
 {vv::Ledger L;int a=0;auto* v=L.record(1,&a,1,Variant::Red);FakeEn en;sk::Retry rt;sk::AuraPart<FakeEn> part(en,rt);EyeFx fx{&part};
  rd::State s;s.pending=1;rd::red_post(s,200,330,0);
  unsigned on_ticks=0;bool saw_full=false;
  for(int t=0;t<200;++t){
   if(t>0)rd::red_post(s,200,330,0);
   const bool win=rd::window_open(s);
   vv::step(L,*v,fx,win,false);devf_eyes12::step_eyes(*v,fx,win);
   if(v->aura_live)++on_ticks;if(fx.tex==devf_vis::kEyeVariants-1)saw_full=true;
  }
  require(!en.ids.empty()&&en.ids.front()==sk::kResource,"aura requested on 0x0B9D");
  bool all_gold=true;for(const auto& col:en.colours)all_gold=all_gold&&col.r==255&&col.g==190&&col.b==30;require(all_gold,"every configure of the Red's emitter uses gold");
  require(saw_full&&fx.seq.front()==1&&fx.tex==0&&!v->aura_live&&v->aura_level==0&&en.stops==1,"gold eyes fade in to variant 7, back to the native texture; aura faded out and released");
  require(on_ticks>=120&&on_ticks<=120+15,"aura lives for the window plus its 15-tick fade-out");
  pass("red_gold_aura_and_eyes_follow_the_window_and_clean_up");}
 {vv::Ledger L;int a=0;auto* v=L.record(1,&a,1,Variant::Red);FakeEn en;sk::Retry rt;sk::AuraPart<FakeEn> part(en,rt);EyeFx fx{&part};
  for(int t=0;t<20;++t){vv::step(L,*v,fx,true,false);devf_eyes12::step_eyes(*v,fx,true);}
  require(v->aura_live&&fx.tex==7,"mid-window");
  vv::cleanup(*v,fx,true);require(!v->aura_live&&fx.tex==0&&en.stops==1,"unload / room change with a live Red: emitter stopped, native eye texture restored");
  auto* w2=L.record(2,&a,1,Variant::Red);EyeFx fx2{&part};for(int t=0;t<5;++t){vv::step(L,*w2,fx2,true,false);devf_eyes12::step_eyes(*w2,fx2,true);}
  const unsigned writes=unsigned(fx2.seq.size());vv::cleanup(*w2,fx2,false);require(fx2.seq.size()==writes&&!w2->aura_live,"actor already gone: nothing touched on its model");
  pass("cleanup_on_unload_and_actor_gone");}
 {auto a=atlas(RGB{70,190,60},RGB{30,120,40},40);auto c=rd::choose_gold(a.data(),a.size());
  require(c.params==&rd::kGoldEyes&&c.endpoints==80,"E_OC2-like green eyes: the gold rule sees the 80 eye endpoints");
  auto r=a;devf_eye::Stats st;require(devf_eye::recolor_cmpr(r.data(),r.size(),1000,rd::kGoldEyes,st)&&st.endpoints_changed==80,"only the eye endpoints change");
  const RGB e=devf_tex::expand565(devf_tex::rd16(r.data()));require(e.r>e.g&&e.g>e.b&&e.r>=150,"eye endpoint becomes gold (r > g > b)");
  for(std::size_t o=40*8;o<r.size();o+=8)require(devf_tex::rd32(r.data()+o)==devf_tex::rd32(a.data()+o)&&devf_tex::rd16(r.data()+o)==devf_tex::rd16(a.data()+o),"non-eye blocks byte-identical");
  auto n=atlas(RGB{4,4,4},RGB{250,250,250},40);require(!rd::choose_gold(n.data(),n.size()).params,"no green eyes: refused (no fake gold)");
  require(rd::kGoldEyes.tint_r==sk::kRedGoldColour.r&&rd::kGoldEyes.tint_g==sk::kRedGoldColour.g&&rd::kGoldEyes.tint_b==sk::kRedGoldColour.b,"eyes share the aura's gold");
  pass("gold_eyes_compressed_domain_green_rule_only");}
 // ---------- 7. regression of the patched shared headers ----------
 {std::uint32_t seed=0x2545F491u;auto rnd=[&]{seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
  for(int run=0;run<4000;++run){
   const Variant v=run%3==0?Variant::Blue:run%3==1?Variant::Red:Variant::Black;auto a=entry_for(v),b=a;std::int16_t h=std::int16_t(rnd()%460)-20;
   for(int t=0;t<90;++t){const int act=rnd()%23==0?devf_variant::A_DEATH:rnd()%11==0?devf_variant::A_DAMAGE:0;if(rnd()%17==0)h=std::int16_t(h-std::int16_t(rnd()%30));
    const auto x=devf_variant::step(a,h,act),y=ref_step(b,h,act);
    require(x.heal==y.heal&&x.new_health==y.new_health&&x.milestone==y.milestone&&std::string(x.skip)==y.skip&&a.acc==b.acc&&a.ticks==b.ticks&&a.healed==b.healed,"step(..., mult=1) == the CP10.3 rule");h=x.new_health;}
  }
  require(devf_eyes12::scheme_for(Variant::Red)==devf_eyes12::Scheme::None&&devf_eyes12::eyes_via_step_eyes(Variant::Blue)&&devf_eyes12::eyes_via_policy_step(Variant::Black),"CP12 eye policy file untouched: Blue via step_eyes, Black via devf_vis::step");
  World w;auto* blue=w.add(Variant::Blue,0,0);auto* other=w.add(Variant::Black,100,0);w.post(blue,0,0);w.post(other,100,0);
  require(blue->group==false&&other->fury==0,"not owned in this fixture: CP10.6 never manages colliders it does not own");
  pass("patched_headers_preserve_blue_black_and_regeneration");}
 std::printf("CP14_LAB_PASS cases=%u RED_POLICY_ONLY; LINK_DAMAGE_AND_PIXELS_NOT_OBSERVED; GAME_NOT_EXECUTED\n",cases);
}
