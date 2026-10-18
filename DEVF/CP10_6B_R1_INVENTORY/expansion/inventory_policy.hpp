#pragma once
// CP10.6B-R1 pure policy (no game headers): READ-ONLY inventory of the JPA resources the game really registered.
//
// ENGINE FACTS (Dusklight 40457c6, read from source):
//  * dPa_control_c::createCommon registers the resident JPAResourceManager in JPAEmitterManager::pResMgrAry[0]; createRoomScene registers the scene one in [1];
//    JPAEmitterManager::clearResourceManager sets pResMgrAry[id] = NULL BEFORE the scene heap is destroyed (JPAEmitterManager.cpp L153-167), and a manager is
//    fully built (its constructor loads every resource) before entryResourceManager publishes it (d_particle.cpp L1231-1240). Reading the CURRENT array entry
//    synchronously inside a game callback therefore never sees a destroyed manager.
//  * JPAResourceManager has PUBLIC resRegNum / resMaxNum / pResAry; JPAResource::getUsrIdx() is the user index that dPa_control_c::set looks up (linear scan).
//    Scene-bank resources carry the 0x8000 bit in their user index; common-bank ones do not. An id is "present" only if it appears here.
// Nothing in this header (or in the mod that uses it) calls dComIfGp_particle_set, creates or destroys an emitter, or writes any engine object: every
// accessor takes a const pointer.
//
// All templates run unchanged in the mod (real JPA types) and in the lab (fakes with the same public member names).
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
namespace devf_inv {
constexpr unsigned kMaxResources=8192,kMaxChunks=40,kChunkChars=700,kMaxShortlist=20,kMaxSceneSnapshots=6,kMaxLines=160;
enum class Why:std::uint8_t {None,NullManager,NullArray,EmptyBank,CountOutOfRange,BankIndexOutOfRange};
inline const char* why_text(Why w) noexcept {
 switch(w){case Why::NullManager:return "MANAGER_NULL_BANK_NOT_REGISTERED";case Why::NullArray:return "RESOURCE_ARRAY_NULL";case Why::EmptyBank:return "BANK_HAS_NO_RESOURCES";
  case Why::CountOutOfRange:return "RESOURCE_COUNT_OUT_OF_RANGE_NOT_READ";case Why::BankIndexOutOfRange:return "BANK_INDEX_OUT_OF_RANGE";default:return "NONE";}
}
struct Snapshot {
 std::array<std::uint16_t,kMaxResources> ids{};   // sorted unique user indices, first `unique` entries
 unsigned entries=0,nulls=0,listed=0,unique=0,dups=0,res_reg=0,res_max=0;std::uint32_t checksum=0;Why why=Why::None;
};
inline std::uint32_t checksum_of(const std::uint16_t* ids,unsigned n) noexcept {
 std::uint32_t h=2166136261u;auto mix=[&](std::uint8_t b){h^=b;h*=16777619u;};
 mix(std::uint8_t(n>>8));mix(std::uint8_t(n));
 for(unsigned i=0;i<n;++i){mix(std::uint8_t(ids[i]>>8));mix(std::uint8_t(ids[i]));}
 return h;
}
// Reads m->resRegNum / resMaxNum / pResAry[i]->getUsrIdx() ONLY. The array is not touched when the counts are inconsistent.
template<class Mgr> inline Why collect(const Mgr* m,Snapshot& s) noexcept {
 s=Snapshot{};
 if(!m)return s.why=Why::NullManager;
 const unsigned reg=m->resRegNum,mx=m->resMaxNum;s.res_reg=reg;s.res_max=mx;s.entries=reg;
 if(reg==0)return s.why=m->pResAry?Why::EmptyBank:Why::NullArray;
 if(!m->pResAry)return s.why=Why::NullArray;
 if(reg>mx||reg>kMaxResources)return s.why=Why::CountOutOfRange;
 unsigned n=0;
 for(unsigned i=0;i<reg;++i){const auto* r=m->pResAry[i];if(!r){++s.nulls;continue;}s.ids[n++]=static_cast<std::uint16_t>(r->getUsrIdx());}
 s.listed=n;std::sort(s.ids.begin(),s.ids.begin()+n);
 unsigned u=0;for(unsigned i=0;i<n;++i)if(i==0||s.ids[i]!=s.ids[i-1])s.ids[u++]=s.ids[i];
 s.unique=u;s.dups=n-u;s.checksum=checksum_of(s.ids.data(),u);
 return s.why=Why::None;
}
// One bank of the emitter manager, re-reading the array entry NOW (never a remembered pointer).
template<class EM> inline Why collect_bank(const EM* em,unsigned bank,Snapshot& s) noexcept {
 s=Snapshot{};
 if(!em||!em->pResMgrAry)return s.why=Why::NullManager;
 if(bank>=em->ridMax)return s.why=Why::BankIndexOutOfRange;
 return collect(em->pResMgrAry[bank],s);
}
// ---------------- serialisation: compact exact ranges, bounded chunks, explicit truncation ----------------
struct Rendered {
 unsigned chunks=0,ids_shown=0,ids_missing=0;
 std::array<std::array<char,kChunkChars+1>,kMaxChunks> text{};
 std::array<unsigned,kMaxChunks> first_index{},id_count{};
};
inline void render(const Snapshot& s,Rendered& r) noexcept {
 r.chunks=0;r.ids_shown=0;r.ids_missing=0;
 unsigned i=0;
 while(i<s.unique){
  if(r.chunks>=kMaxChunks){r.ids_missing=s.unique-i;break;}                           // explicit, never silent
  auto& buf=r.text[r.chunks];unsigned len=0;buf[0]=0;r.first_index[r.chunks]=i;unsigned in_chunk=0;
  while(i<s.unique){
   unsigned j=i;while(j+1<s.unique&&s.ids[j+1]==s.ids[j]+1)++j;                        // maximal run starting at i
   char tok[16];int tl=j>i?std::snprintf(tok,sizeof(tok),"%04X-%04X",s.ids[i],s.ids[j]):std::snprintf(tok,sizeof(tok),"%04X",s.ids[i]);
   const unsigned need=unsigned(tl)+(len?1u:0u);
   if(len+need>kChunkChars)break;                                                      // a run is at most 9 chars: an empty chunk always fits one
   if(len){buf[len++]=',';}
   std::memcpy(&buf[len],tok,unsigned(tl));len+=unsigned(tl);buf[len]=0;
   in_chunk+=j-i+1;i=j+1;
  }
  if(in_chunk==0){r.ids_missing=s.unique-i;break;}                                      // cannot happen; guarantees termination
  r.id_count[r.chunks]=in_chunk;r.ids_shown+=in_chunk;++r.chunks;
 }
}
#ifdef DEVF_INVENTORY_TEST_HELPERS   // lab only: sscanf must not become a CRT import of the DLL (the mod builds with /Zc:inline-)
// Test/QA helper: decode chunk text back to ids (exact inverse of render).
inline unsigned parse_chunk(const char* text,std::uint16_t* out,unsigned cap) noexcept {
 unsigned n=0;const char* p=text;
 while(*p){
  unsigned a=0,b=0;int used=0;
  if(std::sscanf(p,"%4x-%4x%n",&a,&b,&used)==2&&used>0){for(unsigned v=a;v<=b&&n<cap;++v)out[n++]=std::uint16_t(v);}
  else if(std::sscanf(p,"%4x%n",&a,&used)==1&&used>0){if(n<cap)out[n++]=std::uint16_t(a);}
  else break;
  p+=used;if(*p==',')++p;
 }
 return n;
}
#endif
// ---------------- when to snapshot (pure scheduling, rate-limited, stability-checked) ----------------
struct Sig {std::uint64_t token=0;unsigned res_reg=0,res_max=0;std::uint32_t stage=0;
 bool operator==(const Sig& o) const noexcept {return token==o.token&&res_reg==o.res_reg&&res_max==o.res_max&&stage==o.stage;}};
enum class Act:std::uint8_t {None,SnapCommon,CommonNotReady,SnapScene,SceneAbsent,SceneCapReached};
struct Tracker {
 bool common_done=false,common_partial_logged=false,scene_present=false,absent_logged=false,cap_logged=false;
 unsigned common_ready_polls=0,common_polls=0,scene_stable=0,scene_snaps=0,absent_polls=0;
 Sig seen{},logged{};bool have_logged=false;
 static constexpr unsigned kCommonStable=2,kSceneStable=2,kCommonGiveUp=40,kAbsentPolls=60;
 // One poll (the mod calls it every 15th execute pair). `common_ready`: bank 0 registered and non-empty. `scene_present`: bank 1 registered now.
 Act poll(bool mgr_ok,bool common_ready,bool scene_present_now,const Sig& sig) noexcept {
  if(!mgr_ok)return Act::None;
  if(!common_done){
   ++common_polls;
   if(common_ready){if(++common_ready_polls>=kCommonStable){common_done=true;return Act::SnapCommon;}}
   else{common_ready_polls=0;if(common_polls>=kCommonGiveUp&&!common_partial_logged){common_partial_logged=true;return Act::CommonNotReady;}}
  }
  if(!scene_present_now){
   scene_present=false;scene_stable=0;seen=Sig{};
   if(++absent_polls>=kAbsentPolls&&!absent_logged&&scene_snaps==0){absent_logged=true;return Act::SceneAbsent;}
   return Act::None;
  }
  absent_polls=0;scene_present=true;
  if(!(sig==seen)){seen=sig;scene_stable=1;return Act::None;}                          // new manager instance / content: wait until it is stable
  if(scene_stable<kSceneStable)++scene_stable;
  if(scene_stable>=kSceneStable&&!(have_logged&&sig==logged)){
   if(scene_snaps>=kMaxSceneSnapshots){if(!cap_logged){cap_logged=true;logged=sig;have_logged=true;return Act::SceneCapReached;}return Act::None;}
   logged=sig;have_logged=true;++scene_snaps;return Act::SnapScene;
  }
  return Act::None;
 }
};
// Log-line budget for the whole session.
struct LineBudget {unsigned used=0;bool exhausted_reported=false;bool take() noexcept {if(used>=kMaxLines)return false;++used;return true;}};
// ---------------- technical shortlist (heuristics, NOT visual proof) ----------------
struct Meta {
 std::uint16_t id=0;unsigned shape=0,dir=0,rot=0,volume_type=0,volume_size=0,clr_anm=0,texs=0,keys=0,fields=0;int max_frame=0,start=0,life=0;
 float sx=0,sy=0,rate=0,rate_rndm=0,vel_dir=0,vel_omni=0,vel_axis=0,edir[3]{0,0,0};std::uint32_t dyn_flags=0,user_work=0;
 std::uint8_t prm[4]{0,0,0,0},env[3]{0,0,0};bool child=false,extra=false,extex=false;
};
// Feature letters: E elongated (line/stripe shape or aspect >= 3), V emission axis vertical, P persistent (maxFrame 0 or >= 60, life >= 10),
// S soft emission (0.1 <= rate <= 5), N simple (no key animation, no child shape).
constexpr unsigned F_E=1,F_V=2,F_P=4,F_S=8,F_N=16;
inline float aspect_of(const Meta& m) noexcept {const float a=m.sx<0?-m.sx:m.sx,b=m.sy<0?-m.sy:m.sy;const float lo=a<b?a:b,hi=a<b?b:a;return lo>0.0001f?hi/lo:(hi>0.0001f?1000.0f:0.0f);}
inline unsigned features(const Meta& m) noexcept {
 unsigned f=0;
 const bool shape_line=m.shape==1||m.shape==5||m.shape==6;
 if(shape_line||aspect_of(m)>=3.0f)f|=F_E;
 const float len2=m.edir[0]*m.edir[0]+m.edir[1]*m.edir[1]+m.edir[2]*m.edir[2];   // squared: no CRT sqrt import
 if(len2>0.00000001f&&m.edir[1]*m.edir[1]>=0.49f*len2)f|=F_V;
 if((m.max_frame==0||m.max_frame>=60)&&m.life>=10)f|=F_P;
 if(m.rate>=0.1f&&m.rate<=5.0f)f|=F_S;
 if(m.keys==0&&!m.child)f|=F_N;
 return f;
}
inline int score_of(unsigned f) noexcept {return ((f&F_E)?2:0)+((f&F_V)?1:0)+((f&F_P)?1:0)+((f&F_S)?1:0)+((f&F_N)?1:0);}
inline bool eligible(unsigned f) noexcept {return (f&F_E)!=0;}                    // strokes first: a non-elongated resource never enters the shortlist
struct Candidate {Meta meta;unsigned feat=0;int score=0;};
struct Shortlist {
 std::array<Candidate,kMaxShortlist> top{};unsigned n=0;unsigned offered=0,eligible_count=0;
 static bool better(const Candidate& a,const Candidate& b) noexcept {return a.score!=b.score?a.score>b.score:a.meta.id<b.meta.id;}   // total order: independent of input order
 void offer(const Meta& m) noexcept {
  ++offered;const unsigned f=features(m);if(!eligible(f))return;++eligible_count;
  Candidate c;c.meta=m;c.feat=f;c.score=score_of(f);
  unsigned pos=n;while(pos>0&&better(c,top[pos-1]))--pos;
  if(pos>=kMaxShortlist)return;
  if(n<kMaxShortlist)++n;
  for(unsigned k=n-1;k>pos;--k)top[k]=top[k-1];
  top[pos]=c;
 }
};
struct Stats {unsigned total=0,by_shape[12]{},elongated=0,persistent=0,keyed=0,with_child=0,no_meta=0;
 void add(const Meta& m) noexcept {++total;++by_shape[m.shape<11?m.shape:11];const unsigned f=features(m);if(f&F_E)++elongated;if(f&F_P)++persistent;if(m.keys)++keyed;if(m.child)++with_child;}};
} // namespace devf_inv
