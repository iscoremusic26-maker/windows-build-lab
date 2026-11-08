#pragma once
// CP06 FIX01 0.11.12: value-only core. No game headers, addresses, RNG or native
// writes here; cp06f1_adapter.inc samples native state and applies the bounded
// results through three audited writer functions.
//
// A. Skipped-native-pose contact coverage. daE_OC_c::executeAttack arms both AT
//    spheres when the PRE-play frame p is in [14,22]; cc_set places them on the
//    POST-play pose p+r. Vanilla (r=1) therefore registers every pose 15..23.
//    At r>1 one tick spans (p,p+r]: the poses in between are never registered,
//    yet Dusklight's presentation interpolation draws them. FIX01 evaluates the
//    EXACT native poses vanilla would register in that span (pre+1, plus the window
//    start/end on the first/last armed tick) with the engine's own model calc, with
//    the current native pose (in case a foreign sweep displaced it), and only when
//    one of them reaches Link's REAL registered Tg cylinders while the registered
//    spheres do not, moves the native sphere of that slot (native radius) onto it
//    for this pass. The engine then resolves guard/parry/damage/i-frames/no-
//    continuous-hit/sparks itself.
// B. Causal flash suppression. Bokoblin Expansion 0.17.0 CP19F2 restarts ONE
//    global render-only red flash on every chip it charges. A chip that CP06
//    neutralized (authenticated fury parry) still restarted it. FIX01 mirrors the
//    documented schedule twice: E = what Expansion shows (every chip), L = the
//    counterfactual flash of legitimate chips only. Around Link's modelDraw it
//    replaces Expansion's write ONLY when the observed value equals Expansion's
//    exact expected write, by the value Expansion would show without the
//    neutralized chips. Anything else is left untouched.
#include <cstdint>
namespace cp06f1 {
inline constexpr const char* kVersion="0.11.12";
inline constexpr unsigned kSlots=64;
// ---------------------------------------------------------------- A. geometry
struct V3{float x=0,y=0,z=0;};
struct Sph{V3 c{};float r=0;};
struct Cyl{V3 c{};float r=0,h=0;bool on=false;};   // c = bottom centre (cM3dGCyl), on = Link Tg bit set (setCollision)
// cM3d_Cross_CylSph (c_m3d.cpp:1143): `cyl.r+sph.r < sqrt(Len2dSq)` misses, else
// `sph.y+sph.r >= cyl.y && sph.y-sph.r <= cyl.y+cyl.h`. Squared form, no sqrt, with
// a signed slack: slack>0 demands the native test passes with that margin (used
// before WRITING a sphere), slack<0 also accepts near misses (used to KEEP the
// engine's own spheres). slack==0 is the native predicate up to the last ulp.
inline bool cross(const Cyl& k,const Sph& s,float slack)noexcept{
 const float rs=k.r+s.r-slack;if(!(rs>=0.0f))return false;
 const float dx=s.c.x-k.c.x,dz=s.c.z-k.c.z;if(!(dx*dx+dz*dz<=rs*rs))return false;
 const float rv=s.r-slack;return s.c.y+rv>=k.c.y&&s.c.y-rv<=k.c.y+k.h;
}
inline constexpr float kWriteSlack=0.5f,kKeepSlack=-0.5f;
inline bool touches(const Cyl k[3],const Sph& s,float slack,bool require_on)noexcept{
 for(int i=0;i<3;++i)if((k[i].on||!require_on)&&cross(k[i],s,slack))return true;return false;
}
inline bool any_on(const Cyl k[3])noexcept{return k[0].on||k[1].on||k[2].on;}
// executeAttack / cc_set constants (d_a_e_oc.cpp:1452, 2575-2581) and the attack
// lunge tables E_OC_n::oc_attackb_trans / oc_attackc_trans (d_a_e_oc.cpp:61-69).
inline constexpr float kAtFirst=14.0f,kAtLast=22.0f,kPoseFirst=15.0f,kPoseLast=23.0f;
inline constexpr float kTipZ=70.0f,kHiltZ=20.0f,kTipR=45.0f,kHiltR=30.0f;
inline constexpr float kMaxRate=2.0f,kMinFastRate=1.0001f;
inline constexpr float kLungeB[10]={0.0f,0.0f,0.0f,0.0f,15.3f,38.4f,63.3f,86.2f,103.3f,112.0f};
inline constexpr float kLungeC[10]={0.0f,31.3f,72.7f,91.5f,91.5f,91.5f,91.5f,91.5f,91.5f,91.5f};
inline float lunge(bool clip_b,float pre)noexcept{int k=int(pre-9.0f);if(k<0)return 0.0f;if(k>=9)k=9;return clip_b?kLungeB[k]:kLungeC[k];}
// The tick that registered pose `post` started at `pre`. Vanilla poses to evaluate
// so that every pose of the vanilla window [15,23] lies within half a vanilla step
// (0.5 frame) of a registered or evaluated pose (never coarser than x1). Greedy
// fill anchored on exact poses:
//  * pre+1 is always taken (the vanilla tick whose PRE frame is `pre`: same root
//    lunge; at x2 it is the skipped integer pose), `post` is registered;
//  * the span starts at pre+0.5 when pose `pre` was registered AND decided against
//    Link last tick, at `pre` when it was armed but undecided, else at 15 (first
//    armed tick: earlier vanilla poses were skipped by an unarmed tick);
//  * gaps wider than 0.5 before an anchor get points at min(cur+0.5, anchor-0.5);
//  * the window end 23 when the engine will not arm again (next PRE frame = post >
//    22, the exact native test) and 23 is more than 0.5 past `post`.
inline constexpr unsigned kMaxProbes=3;
inline unsigned probe_frames(float pre,float post,bool prev_armed,bool prev_covered,float out[kMaxProbes])noexcept{
 const float r=post-pre;if(!(r>=kMinFastRate&&r<=kMaxRate+1e-4f))return 0;
 if(!(pre>=kAtFirst&&pre<=kAtLast))return 0;   // the engine armed this tick on exactly this test
 unsigned n=0;float cur=prev_armed&&prev_covered?pre+0.5f:prev_armed?pre:kPoseFirst;
 const float one=pre+1.0f;const bool use_one=one<post&&one>=kPoseFirst;
 const float anchors[2]={use_one?one:post,post};
 for(unsigned a=use_one?0u:1u;a<2;++a){
  const float at=anchors[a];
  while(at-cur>0.5f+1e-4f&&n<kMaxProbes){float p=cur+0.5f;if(p>at-0.5f)p=at-0.5f;if(p>kPoseLast)p=kPoseLast;if(n&&p<=out[n-1])break;out[n++]=p;cur=p+0.5f;}
  if(a==0){if(n<kMaxProbes&&(!n||out[n-1]<one))out[n++]=one;else if(n>=kMaxProbes)break;}
  if(at+0.5f>cur)cur=at+0.5f;
 }
 if(n<kMaxProbes&&post>kAtLast&&kPoseLast-post>0.5f&&(!n||out[n-1]<kPoseLast))out[n++]=kPoseLast;
 return n;
}
// Vanilla registers pose m on the tick whose PRE frame is m-1, after executeAttack
// moved the root by lunge(m-1) (d_a_e_oc.cpp:1491-1493: pos += (lunge-prev)*
// (cM_ssin,cM_scos)(shape_angle.y)). The engine pose is evaluated with THIS tick's
// base matrix (root at lunge(pre)), so the vanilla centre is offset by
// (lunge(m-1)-lunge(pre)) along the facing. Zero for m=pre+1, for the window end
// 23 (flat table there) and for the flat clip-C table (frame>=12).
// s16 binary angle -> sin/cos, |error| < 3e-7 (the engine's table differs by < 1e-4).
inline void sincos16(std::int16_t a,float& sn,float& cs)noexcept{
 const int x=int(a);const int q=((x+0x2000)>>14)&3;const float t=float(x-((x+0x2000)&~0x3fff))*(3.14159265358979f/32768.0f);
 const float t2=t*t;const float s=t*(1.0f-t2/6.0f*(1.0f-t2/20.0f*(1.0f-t2/42.0f*(1.0f-t2/72.0f))));const float c=1.0f-t2/2.0f*(1.0f-t2/12.0f*(1.0f-t2/30.0f*(1.0f-t2/56.0f)));
 if(q==0){sn=s;cs=c;}else if(q==1){sn=c;cs=-s;}else if(q==2){sn=-s;cs=-c;}else{sn=-c;cs=s;}
}
inline V3 lunge_offset(bool clip_b,float pre,float m,float sn,float cs)noexcept{const float d=lunge(clip_b,m-1.0f)-lunge(clip_b,pre);return V3{d*sn,0.0f,d*cs};}
// cc_set: centre = AnmMtx(0xc) * (0,0,z). Row-major 3x4 (PSMTXMultVec).
inline V3 bone_point(const float m[3][4],float z)noexcept{V3 v;v.x=m[0][0]*0.0f+m[0][1]*0.0f+m[0][2]*z+m[0][3];v.y=m[1][0]*0.0f+m[1][1]*0.0f+m[1][2]*z+m[1][3];v.z=m[2][0]*0.0f+m[2][1]*0.0f+m[2][2]*z+m[2][3];return v;}
inline Sph tip(const float m[3][4])noexcept{Sph s;s.c=bone_point(m,kTipZ);s.r=kTipR;return s;}
inline Sph hilt(const float m[3][4])noexcept{Sph s;s.c=bone_point(m,kHiltZ);s.r=kHiltR;return s;}
inline bool same(const V3& a,const V3& b)noexcept{return a.x==b.x&&a.y==b.y&&a.z==b.z;}
inline Sph moved(Sph s,V3 o)noexcept{s.c.x+=o.x;s.c.y+=o.y;s.c.z+=o.z;return s;}
enum class Why:std::uint8_t{None,Written,AlreadyTouching,NoMidContact,LinkInvulnerable,NotFast,NotConsecutive,WindowEdge,MorphActive,LinkNotCurrent,LinkMissing,ProbeRefused,ProbeMismatch,OutOfReach,NotAttack,Count};
inline const char* name(Why w)noexcept{
 const char* n[]={"NONE","WRITTEN_EXACT_NATIVE_POSE","ALREADY_TOUCHING_ENGINE_DECIDES","NO_EVALUATED_POSE_REACHES_LINK","LINK_TG_OFF_NATIVE_INVULNERABILITY","RATE_NOT_ABOVE_1","NOT_CONSECUTIVE_SAME_CLIP","NO_SKIPPED_POSE_IN_VANILLA_WINDOW","MORPH_BLEND_ACTIVE","LINK_COLLISION_NOT_SET_THIS_TICK","LINK_MISSING","POSE_PROBE_REFUSED","POSE_PROBE_RESTORE_MISMATCH","OUT_OF_REACH","NOT_AN_ARMED_ATTACK"};
 return n[unsigned(w)<unsigned(Why::Count)?unsigned(w):0];
}
// Candidate exact native poses for this pass: [0] = the current native pose (what
// cc_set placed before any foreign sweep), then the evaluated vanilla poses with
// their vanilla root.
struct Cand{Sph s[2]{};};
struct Decision{Why why=Why::None;bool write[2]{},geom[2]{};unsigned from[2]{};Sph place[2]{};};
// cur = spheres as registered for this pass (native cc_set or Expansion's sweep).
// A registered sphere that crosses an ON cylinder with the write margin already
// guarantees the engine contact: nothing is written. Otherwise each slot takes the
// first candidate (in order) that crosses with that margin; a slot without one is
// left exactly as registered.
inline Decision decide(const Cyl k[3],const Sph cur[2],const Cand* c,unsigned n)noexcept{
 Decision d;
 if(touches(k,cur[0],kWriteSlack,true)||touches(k,cur[1],kWriteSlack,true)){d.why=Why::AlreadyTouching;return d;}
 for(int j=0;j<2;++j)for(unsigned i=0;i<n&&!d.write[j];++i){
  const Sph& t=c[i].s[j];if(!(t.r>0.0f))continue;
  if(!touches(k,t,kWriteSlack,false))continue;d.geom[j]=true;
  if(touches(k,t,kWriteSlack,true)){d.write[j]=true;d.place[j]=t;d.from[j]=i;}
 }
 d.why=d.write[0]||d.write[1]?Why::Written:d.geom[0]||d.geom[1]?Why::LinkInvulnerable:Why::NoMidContact;return d;
}
// Cheap pre-gate before any probe: can an evaluated pose reach Link at all? Every
// evaluated pose lies within [pre-1, post+1] frames of this tick, so its hand (bone
// 0xc origin) is within the hand's travel this tick plus slack of the current hand;
// the tip sphere reaches kTipZ+kTipR around the hand. Performance gate only: a
// false "out of reach" loses coverage (0.11.11 behaviour), never creates a contact.
inline constexpr float kReachSlack=60.0f;
inline float sqrt_upper(float x)noexcept{   // Newton from above, no CRT; >= sqrt(x) for x>=0
 if(!(x>0.0f))return 0.0f;float r=x>1.0f?x:1.0f;for(int i=0;i<40;++i){const float n=0.5f*(r+x/r);if(!(n<r))break;r=n;}return r*1.0001f+1e-3f;
}
inline bool reachable(const Cyl k[3],V3 hand,V3 prev_hand)noexcept{
 const float mx=hand.x-prev_hand.x,my=hand.y-prev_hand.y,mz=hand.z-prev_hand.z;const float move=sqrt_upper(mx*mx+my*my+mz*mz);
 for(int i=0;i<3;++i){const float lim=kTipZ+kTipR+k[i].r+kReachSlack+move;const float dx=hand.x-k[i].c.x,dz=hand.z-k[i].c.z;
  if(dx*dx+dz*dz<=lim*lim&&hand.y+lim>=k[i].c.y&&hand.y-lim<=k[i].c.y+k[i].h)return true;}
 return false;
}
// --------------------------------------------------------- A. swing episodes
enum class Kind:std::uint8_t{Blue,Red,Black,BlackFury,RedSpecialX2,Unknown,Count};
inline const char* name(Kind k)noexcept{const char* n[]={"BLUE","RED","BLACK","BLACK_FURY_X2","RED_SPECIAL_X2","UNKNOWN"};return n[unsigned(k)<unsigned(Kind::Count)?unsigned(k):5];}
enum class Result:std::uint8_t{None,ValidHitBody,ValidShieldBlock,ValidParry,LegitimateMiss,LegitimateInvulnerability,NativeNoContinuousHit,NativeBlockedOther,ContactLostConfirmed,GeometryUndetermined,CleanupStale,Count};
inline const char* name(Result r)noexcept{
 const char* n[]={"NONE","VALID_HIT_BODY","VALID_SHIELD_BLOCK","VALID_PARRY","LEGITIMATE_MISS","LEGITIMATE_INVULNERABILITY","NATIVE_NO_CONTINUOUS_HIT","NATIVE_BLOCKED_OTHER","CONTACT_LOST_CONFIRMED","GEOMETRY_UNDETERMINED","CLEANUP_STALE"};
 return n[unsigned(r)<unsigned(Result::Count)?unsigned(r):0];
}
// What the previous collision pass recorded on this actor's spheres (read in the
// next POST: ChkAtTg clears registered At records at the start of each pass).
struct PassView{bool registered=false,link=false,shield=false,other=false;};
struct Tick{bool armed=false,link_current=false,link_any_on=false,geom_touch=false,expect=false,probe_refused=false,guard_attack=false;Why why=Why::None;unsigned written=0;float pre=0,post=0,rate=0;int link_proc=-1,damage_timer=0;};
struct Swing{
 bool open=false;std::uint32_t id=0;std::uint64_t generation=0,seq=0,start_tick=0,last_tick=0,contact_tick=0;std::uint8_t clip=0;Kind kind=Kind::Unknown;
 float rate=0,first_pre=-1,last_pre=-1;unsigned at_ticks=0,geom_ticks=0,off_touch_ticks=0,expect_ticks=0,lost_ticks=0,nocon_ticks=0,undetermined_ticks=0,backfill_ticks=0,backfill_slots=0,stale=0;
 bool contact=false,shield=false,parry=false,other=false,backfilled_contact=false;int link_proc=-1,damage_timer=0;
 // pending facts of the last armed tick (its pass is judged in the next POST) and
 // whether this attacker crossed Link in the pass before it (engine TgOldApid rule)
 bool exp_pending=false,written_pending=false,ga_pending=false,crossed_before=false;
};
inline Result classify(const Swing& s)noexcept{
 if(s.contact)return s.shield?(s.parry?Result::ValidParry:Result::ValidShieldBlock):Result::ValidHitBody;
 if(s.other)return Result::NativeBlockedOther;
 if(s.lost_ticks)return Result::ContactLostConfirmed;
 if(s.nocon_ticks)return Result::NativeNoContinuousHit;
 if(s.off_touch_ticks)return Result::LegitimateInvulnerability;
 if(s.undetermined_ticks)return Result::GeometryUndetermined;
 if(s.stale&&!s.at_ticks)return Result::CleanupStale;
 return Result::LegitimateMiss;
}
// Result of the pass that followed the last armed tick (pv.registered) or stale
// flags of an unregistered sphere (FIX01 lease cleans those natively).
inline void feed_pass(Swing& s,const PassView& pv,std::uint64_t tick)noexcept{
 if(!s.open)return;
 if(!pv.registered){if(pv.link||pv.other)++s.stale;s.exp_pending=s.written_pending=s.ga_pending=false;s.crossed_before=false;return;}
 bool crossed=false;
 if(pv.link){crossed=true;if(!s.contact){s.contact=true;s.shield=pv.shield;s.parry=pv.shield&&s.ga_pending;s.contact_tick=tick;s.backfilled_contact=s.written_pending;}}
 else if(pv.other)s.other=true;
 else if(s.exp_pending){crossed=true;if(s.crossed_before)++s.nocon_ticks;else ++s.lost_ticks;}
 s.crossed_before=crossed;s.exp_pending=s.written_pending=s.ga_pending=false;
}
inline void feed_tick(Swing& s,const Tick& t)noexcept{
 if(!s.open||!t.armed)return;
 ++s.at_ticks;if(s.first_pre<0)s.first_pre=t.pre;s.last_pre=t.pre;s.rate=t.rate;s.link_proc=t.link_proc;s.damage_timer=t.damage_timer;
 if(s.kind==Kind::Red&&t.rate==kMaxRate)s.kind=Kind::RedSpecialX2;
 if(t.geom_touch){++s.geom_ticks;if(!t.link_any_on)++s.off_touch_ticks;}
 if(t.probe_refused)++s.undetermined_ticks;
 if(t.written){++s.backfill_ticks;s.backfill_slots+=t.written;}
 // Expected = a sphere of THIS pass reaches an ON cylinder of a Link registered this tick, with the write margin.
 s.exp_pending=t.expect&&t.link_current;if(s.exp_pending)++s.expect_ticks;s.written_pending=t.written!=0;s.ga_pending=t.guard_attack;
}
// ------------------------------------------------- B. Expansion flash mirror
// Transcription of devf_cp19f2 (cp19f2_policy.hpp, fe47c8b): flash_start sets
// dmg=30, col=32-(30%16)=18; flash_tick once per management pass decrements both;
// colour t=dmg+col, base 15/10/5 (t>32 / t>16 / else), r=uint8(base*|sin16[t&15]|),
// g=b=0; merge = per-channel max with the native register value.
inline constexpr int kFlashTimer=30;
inline constexpr std::uint8_t kR0=15,kR1=10,kR2=5;
inline constexpr float kAbsSin16[16]={0.0f,0.19509032f,0.38268343f,0.55557023f,0.70710678f,0.83146961f,0.92387953f,0.98078528f,1.0f,0.98078528f,0.92387953f,0.83146961f,0.70710678f,0.55557023f,0.38268343f,0.19509032f};
struct Rgb{std::uint8_t r=0,g=0,b=0;};
inline bool operator==(const Rgb& a,const Rgb& b)noexcept{return a.r==b.r&&a.g==b.g&&a.b==b.b;}
inline bool operator!=(const Rgb& a,const Rgb& b)noexcept{return !(a==b);}
struct Flash{int dmg=0,col=0;};
inline void start(Flash& f)noexcept{f.dmg=kFlashTimer;f.col=32-(kFlashTimer%16);}
inline bool on(const Flash& f)noexcept{return f.dmg>0;}
inline void tick(Flash& f)noexcept{if(f.dmg>0){if(f.col>0)--f.col;--f.dmg;}}
inline Rgb color(const Flash& f)noexcept{Rgb c;if(f.dmg<=0)return c;const int t=f.dmg+f.col;const std::uint8_t base=t>32?kR0:t>16?kR1:kR2;c.r=std::uint8_t(float(base)*kAbsSin16[unsigned(t)&15u]);return c;}
inline Rgb merge(Rgb n,Rgb o)noexcept{Rgb c;c.r=n.r>o.r?n.r:o.r;c.g=n.g>o.g?n.g:o.g;c.b=n.b>o.b?n.b:o.b;return c;}
// Chip observation around Expansion's E_OC execute POST (priority 0): pending life
// count and CP06's neutralization counter captured at POST(+91) and compared at
// POST(-251), i.e. after CP06's own decide (-250) of the SAME execute.
enum class ChipSeen:std::uint8_t{None,ParryNeutralized,Legit,Foreign,Count};
inline ChipSeen observe(float before,float after,std::uint64_t comp_before,std::uint64_t comp_after)noexcept{
 if(comp_after==comp_before+1&&after==before)return ChipSeen::ParryNeutralized;
 if(comp_after==comp_before&&after==before-1.0f)return ChipSeen::Legit;
 if(comp_after!=comp_before||after!=before)return ChipSeen::Foreign;
 return ChipSeen::None;
}
enum class Draw:std::uint8_t{Untouched,NoMirroredWrite,Counterfactual,Mismatch,Uncertain,Count};
inline const char* name(Draw d)noexcept{const char* n[]={"UNTOUCHED","EXPANSION_DID_NOT_WRITE","COUNTERFACTUAL_WRITTEN","FOREIGN_OR_MISALIGNED_VALUE","UNCERTAIN_WINDOW"};return n[unsigned(d)<unsigned(Draw::Count)?unsigned(d):0];}
class FlashMirror{
 public:
 struct Counters{std::uint64_t parry_starts=0,legit_starts=0,foreign=0,ticks=0,draws=0,expansion_writes=0,counterfactual=0,red_removed=0,red_kept_legit=0,mismatch=0,uncertain_draws=0,restored_by_expansion=0,restored_by_us=0,restore_foreign=0,restore_not_needed=0;}c{};
 void chip(ChipSeen s)noexcept{
  if(s==ChipSeen::ParryNeutralized){start(e_);++c.parry_starts;}
  else if(s==ChipSeen::Legit){start(e_);start(l_);++c.legit_starts;}
  else if(s==ChipSeen::Foreign){uncertain_=true;++c.foreign;}
 }
 void pass()noexcept{tick(e_);tick(l_);++c.ticks;if(!on(e_)&&!on(l_))uncertain_=false;}
 // PRE(-250), after Expansion's PRE(0): orig = value seen at PRE(+90), now = current.
 // Writes `out` only for an exact Expansion write that differs from the counterfactual.
 Draw decide(Rgb orig,Rgb now,Rgb& out)noexcept{
  ++c.draws;out=now;
  if(!on(e_))return Draw::Untouched;   // nothing of Expansion's to attribute: never touch a foreign value
  const Rgb expected=merge(orig,color(e_));
  if(now==orig&&expected!=orig)return Draw::NoMirroredWrite;
  if(now!=expected){++c.mismatch;uncertain_=true;return Draw::Mismatch;}
  if(now!=orig)++c.expansion_writes;
  if(uncertain_){++c.uncertain_draws;return Draw::Uncertain;}
  const Rgb target=merge(orig,on(l_)?color(l_):Rgb{});
  if(target==now)return Draw::Untouched;
  out=target;++c.counterfactual;if(target==orig)++c.red_removed;else ++c.red_kept_legit;return Draw::Counterfactual;
 }
 bool suppressing()const noexcept{return on(e_)&&!uncertain_;}
 bool uncertain()const noexcept{return uncertain_;}
 const Flash& expansion()const noexcept{return e_;}
 const Flash& legit()const noexcept{return l_;}
 void reset()noexcept{e_=Flash{};l_=Flash{};uncertain_=false;}
 private:
 Flash e_{},l_{};bool uncertain_=false;
};
}
