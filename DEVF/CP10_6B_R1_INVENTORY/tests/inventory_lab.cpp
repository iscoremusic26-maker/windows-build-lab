// CP10.6B-R1 offline lab: the READ-ONLY JPA inventory (collection, serialisation, scheduling, shortlist). Pure logic, no game, no GPU.
// The collection templates run on fake managers with the same public member names as the engine's JPAResourceManager / JPAEmitterManager, so the code
// under test is the code the mod runs. It proves exactness, bounds and the read-only contract; it does NOT prove what the user's game really holds:
// that is the human round (the log).
#define _CRT_SECURE_NO_WARNINGS 1   // sscanf in the lab-only parser (MSVC C4996)
#define DEVF_INVENTORY_TEST_HELPERS 1
#include "../expansion/inventory_policy.hpp"
#include <cstdio>
#include <cstdlib>
#include <set>
#include <vector>
using namespace devf_inv;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
unsigned g_reads=0;
struct FakeRes {std::uint16_t usr;std::uint16_t getUsrIdx() const {++g_reads;return usr;}};
struct FakeMgr {unsigned resRegNum=0,resMaxNum=0;FakeRes** pResAry=nullptr;};
struct FakeEM {FakeMgr** pResMgrAry=nullptr;unsigned char ridMax=2;};
struct Holder {
 std::vector<FakeRes> res;std::vector<FakeRes*> ptrs;FakeMgr mgr;
 explicit Holder(const std::vector<unsigned>& ids,unsigned max_extra=8){res.resize(ids.size());ptrs.resize(ids.size());for(size_t i=0;i<ids.size();++i){res[i].usr=std::uint16_t(ids[i]);ptrs[i]=&res[i];}
  mgr.resRegNum=unsigned(ids.size());mgr.resMaxNum=unsigned(ids.size())+max_extra;mgr.pResAry=ptrs.empty()?reinterpret_cast<FakeRes**>(&mgr):ptrs.data();}
};
std::uint64_t lcg=0x1234ABCD5678EF01ULL;std::uint32_t rnd(){lcg=lcg*6364136223846793005ULL+1442695040888963407ULL;return std::uint32_t(lcg>>33);}
std::vector<std::uint16_t> decode_all(const Rendered& r){std::vector<std::uint16_t> out;for(unsigned c=0;c<r.chunks;++c){std::uint16_t tmp[kMaxResources];const unsigned n=parse_chunk(r.text[c].data(),tmp,kMaxResources);require(n==r.id_count[c],"chunk count matches its text");out.insert(out.end(),tmp,tmp+n);}return out;}
Rendered R;Snapshot S;
int main(){
 // ===================== collection =====================
 {std::vector<unsigned> ids{0x72F,0x0E6,0x8001,0x0E7,0x0E8,0x9D3,0x0046+1,0xFFFF,0x8000,0x0000};Holder h(ids);require(collect(&h.mgr,S)==Why::None,"collect ok");
  require(S.unique==ids.size()&&S.listed==ids.size()&&S.dups==0&&S.nulls==0,"counts");for(unsigned i=1;i<S.unique;++i)require(S.ids[i]>S.ids[i-1],"sorted unique");
  std::set<unsigned> want(ids.begin(),ids.end());for(unsigned i=0;i<S.unique;++i)require(want.count(S.ids[i])==1,"exactly the registered ids");require(S.ids[0]==0&&S.ids[S.unique-1]==0xFFFF,"extremes kept");
  pass("collect_returns_exactly_the_registered_user_indices_sorted");}
 {Holder h({5,3,3,9,5,5,1});require(collect(&h.mgr,S)==Why::None&&S.listed==7&&S.unique==4&&S.dups==3,"duplicates counted, listed once");
  Holder g({1,2,3,4});g.ptrs[1]=nullptr;require(collect(&g.mgr,S)==Why::None&&S.nulls==1&&S.listed==3&&S.unique==3&&S.entries==4,"null entries skipped and counted");
  pass("duplicates_and_null_entries_are_counted_not_hidden");}
 {FakeMgr empty;empty.pResAry=reinterpret_cast<FakeRes**>(&empty);require(collect(&empty,S)==Why::EmptyBank&&S.unique==0,"empty bank");
  FakeMgr noarr;noarr.resRegNum=5;noarr.resMaxNum=8;require(collect(&noarr,S)==Why::NullArray,"array null with count");
  require(collect(static_cast<const FakeMgr*>(nullptr),S)==Why::NullManager,"absent manager");
  FakeEM em;require(collect_bank(&em,0,S)==Why::NullManager,"no manager array");FakeMgr* arr[2]{nullptr,nullptr};em.pResMgrAry=arr;require(collect_bank(&em,1,S)==Why::NullManager,"bank slot empty");require(collect_bank(&em,2,S)==Why::BankIndexOutOfRange,"bank index beyond ridMax");
  pass("absent_empty_and_unregistered_banks_are_reported_with_a_reason");}
 {Holder h({1,2,3});h.mgr.resRegNum=9;h.mgr.resMaxNum=4;g_reads=0;require(collect(&h.mgr,S)==Why::CountOutOfRange&&g_reads==0,"count > max: the array is NOT read");
  std::vector<unsigned> many(kMaxResources+10);for(unsigned i=0;i<many.size();++i)many[i]=i&0xFFFF;Holder b(many,50);g_reads=0;require(collect(&b.mgr,S)==Why::CountOutOfRange&&g_reads==0,"count above the bound: not read");
  Holder ok(std::vector<unsigned>(kMaxResources,7));require(collect(&ok.mgr,S)==Why::None&&S.unique==1&&S.dups==kMaxResources-1,"exactly at the bound is read");
  pass("inconsistent_or_overflowing_counts_never_touch_the_array");}
 {Holder h({10,20,30,40});unsigned char before[sizeof(FakeMgr)];std::memcpy(before,&h.mgr,sizeof(FakeMgr));std::vector<std::uint16_t> idsb;for(auto& r:h.res)idsb.push_back(r.usr);
  const FakeMgr* cm=&h.mgr;collect(cm,S);require(std::memcmp(before,&h.mgr,sizeof(FakeMgr))==0,"manager bytes unchanged");for(size_t i=0;i<h.res.size();++i)require(h.res[i].usr==idsb[i],"resources unchanged");
  pass("read_only_contract_const_access_and_no_mutation");}
 {// order independence and checksum
  std::vector<unsigned> a{9,8,7,100,101,102,0x8010};std::vector<unsigned> b(a.rbegin(),a.rend());Holder ha(a),hb(b);collect(&ha.mgr,S);Snapshot S2;collect(&hb.mgr,S2);require(S.checksum==S2.checksum&&S.unique==S2.unique,"same set, same checksum, any order");
  std::vector<unsigned> c=a;c[3]=103;Holder hc(c);Snapshot S3;collect(&hc.mgr,S3);require(S3.checksum!=S.checksum,"one different id changes the checksum");
  std::vector<unsigned> d=a;d.push_back(1);Holder hd(d);Snapshot S4;collect(&hd.mgr,S4);require(S4.checksum!=S.checksum,"one more id changes it");
  pass("checksum_depends_on_the_exact_set_not_the_order");}
 // ===================== serialisation =====================
 {for(int round=0;round<40;++round){std::set<unsigned> set;const unsigned target=round<10?round:round%2?unsigned(rnd()%3000):unsigned(rnd()%7000);while(set.size()<target){const unsigned base=(rnd()%2)?0x8000u:0u;set.insert((base+rnd()%0x0F00)&0xFFFF);}
   std::vector<unsigned> ids(set.begin(),set.end());Holder h(ids);require(collect(&h.mgr,S)==(ids.empty()?Why::EmptyBank:Why::None),"collect");render(S,R);
   if(R.ids_missing==0){const auto back=decode_all(R);require(back.size()==S.unique,"every id present");for(unsigned i=0;i<S.unique;++i)require(back[i]==S.ids[i],"exact, ordered");}
   require(R.ids_shown+R.ids_missing==S.unique,"shown + missing == unique: nothing silently dropped");
   for(unsigned c=0;c<R.chunks;++c)require(std::strlen(R.text[c].data())<=kChunkChars,"chunk within the line budget");}
  pass("render_roundtrips_exact_ids_for_random_sets_and_never_exceeds_the_chunk_budget");}
 {std::vector<unsigned> evens;for(unsigned i=0;i<kMaxResources;++i)evens.push_back((i*2)&0xFFFF);Holder h(evens);collect(&h.mgr,S);render(S,R);
  require(R.chunks==kMaxChunks&&R.ids_missing>0&&R.ids_shown+R.ids_missing==S.unique,"worst case (no two consecutive ids) is explicitly truncated with an exact missing count");
  const auto shown=decode_all(R);require(shown.size()==R.ids_shown,"what is shown is exact");for(size_t i=0;i<shown.size();++i)require(shown[i]==S.ids[i],"shown ids are the first ones in order");
  std::vector<unsigned> typical;for(unsigned i=0;i<1500;++i)typical.push_back(0x0E6+i+(i>700?200:0)+(i>1200?500:0));Holder t(typical);collect(&t.mgr,S);render(S,R);require(R.ids_missing==0&&R.chunks<=2,"a realistic bank (a few runs) fits in a couple of lines");
  pass("truncation_is_explicit_and_exact_and_realistic_banks_fit_in_two_lines");}
 {std::vector<unsigned> run;for(unsigned i=0;i<5000;++i)run.push_back(i);Holder h(run);collect(&h.mgr,S);render(S,R);require(R.chunks==1&&R.ids_missing==0&&std::strcmp(R.text[0].data(),"0000-1387")==0,"5000 consecutive ids = one token");
  Holder e(std::vector<unsigned>{});collect(&e.mgr,S);render(S,R);require(R.chunks==0&&R.ids_shown==0,"empty set renders nothing");
  Holder one(std::vector<unsigned>{0x8046});collect(&one.mgr,S);render(S,R);require(R.chunks==1&&std::strcmp(R.text[0].data(),"8046")==0,"bank-1 id keeps its 0x8000 bit, 4 upper-case hex digits");
  Holder m(std::vector<unsigned>{0x0046,0x0047,0x0049,0x004A,0x004B,0x8046});collect(&m.mgr,S);render(S,R);require(std::strcmp(R.text[0].data(),"0046-0047,0049-004B,8046")==0,"runs and singles");
  pass("ranges_singles_empty_and_bank_bit_ids_render_exactly");}
 // ===================== scheduling =====================
 {Tracker t;Sig a{1,100,120,7};require(t.poll(false,false,false,a)==Act::None,"no manager: nothing");
  require(t.poll(true,false,false,a)==Act::None,"common not ready");require(t.poll(true,true,false,a)==Act::None,"first ready poll: not yet");require(t.poll(true,true,false,a)==Act::SnapCommon,"stable for 2 polls: snapshot common");
  for(int i=0;i<200;++i)require(t.poll(true,true,false,a)!=Act::SnapCommon,"the common bank is never snapshotted twice");
  Tracker u;for(unsigned i=0;i<Tracker::kCommonGiveUp-1;++i)require(u.poll(true,false,false,a)==Act::None,"waiting");require(u.poll(true,false,false,a)==Act::CommonNotReady,"gives up once, with a reason");for(int i=0;i<50;++i)require(u.poll(true,false,false,a)!=Act::CommonNotReady,"and does not repeat");
  pass("common_bank_is_snapshotted_once_when_stable_and_a_late_bank_is_reported_once");}
 {Tracker t;Sig a{1000,300,320,5},b{2000,300,320,5},c{2000,300,320,6};unsigned snaps=0;t.poll(true,true,false,a);require(t.poll(true,true,false,a)==Act::SnapCommon,"common first (a poll that snapshots the common bank does not advance the scene logic)");auto drive=[&](const Sig& s,int n,bool present=true){for(int i=0;i<n;++i)if(t.poll(true,true,present,s)==Act::SnapScene)++snaps;};
  drive(a,1);require(snaps==0,"first sight of an instance: wait");drive(a,1);require(snaps==1,"stable: one snapshot");drive(a,100);require(snaps==1,"no repeat while nothing changes");
  drive(b,1);drive(b,1);require(snaps==2,"new manager instance (token changed): new snapshot");drive(b,50);require(snaps==2,"stable again");
  drive(c,2);require(snaps==3,"same pointer but a different stage tag: new snapshot");
  drive(a,0,false);for(int i=0;i<3;++i)t.poll(true,true,false,a);drive(a,2);require(snaps==4,"bank vanished then came back (room change): new snapshot");
  pass("scene_bank_snapshots_follow_new_instances_stably_and_never_repeat_on_their_own");}
 {Tracker t;unsigned snaps=0,caps=0,absent=0;t.poll(true,true,false,Sig{});t.poll(true,true,false,Sig{});for(unsigned k=0;k<20;++k){Sig s{500+k,10,12,1};for(int i=0;i<3;++i){auto a=t.poll(true,true,true,s);if(a==Act::SnapScene)++snaps;if(a==Act::SceneCapReached)++caps;}}
  require(snaps==kMaxSceneSnapshots&&caps==1,"hard cap on scene snapshots, reported once");
  Tracker n;for(unsigned i=0;i<Tracker::kAbsentPolls+5;++i)if(n.poll(true,true,false,Sig{})==Act::SceneAbsent)++absent;require(absent==1,"a scene bank that never appears is reported once");
  pass("scene_snapshot_cap_and_absent_scene_bank_are_reported_once");}
 {Tracker t;Sig a{7,1,2,3};t.poll(true,true,false,a);t.poll(true,true,false,a);t.poll(true,true,true,a);t.poll(true,true,true,a);t.poll(true,true,true,a);require(t.scene_snaps==1,"snap");
  // the bank pointer is nulled by the engine: the scheduler never needs (or keeps) a pointer, only a token
  t.poll(true,true,false,a);require(!t.scene_present&&t.scene_stable==0&&t.seen.token==0,"vanished: no remembered signature to compare against a dead pointer");
  pass("pointer_invalidation_resets_the_scheduler_state");}
 {LineBudget b;unsigned ok=0;for(unsigned i=0;i<kMaxLines+50;++i)if(b.take())++ok;require(ok==kMaxLines&&!b.take(),"line budget is hard");pass("log_line_budget_is_hard");}
 // ===================== shortlist =====================
 {Meta thin;thin.id=0x100;thin.shape=5;thin.sx=1;thin.sy=1;thin.edir[1]=1;thin.max_frame=0;thin.life=30;thin.rate=0.5f;
  Meta wide;wide.id=0x101;wide.shape=2;wide.sx=1;wide.sy=10;wide.edir[1]=1;wide.life=30;wide.rate=0.5f;wide.max_frame=120;
  Meta blob;blob.id=0x102;blob.shape=2;blob.sx=4;blob.sy=4;blob.edir[1]=1;blob.life=30;blob.rate=1;
  Meta point;point.id=0x103;point.shape=0;
  const unsigned ft=features(thin),fw=features(wide),fb=features(blob),fp=features(point);
  require((ft&F_E)&&(ft&F_V)&&(ft&F_P)&&(ft&F_S)&&(ft&F_N)&&score_of(ft)==6,"stripe, vertical, persistent, soft, simple = top score");
  require((fw&F_E)&&aspect_of(wide)==10.0f,"narrow billboard is elongated by aspect");require(!(fb&F_E),"square sprite is not elongated");require(!eligible(fb)&&!eligible(fp),"non-elongated resources never enter the shortlist");
  Shortlist s;for(const Meta* m:{&blob,&point,&wide,&thin})s.offer(*m);require(s.n==2&&s.top[0].meta.id==0x100&&s.top[1].meta.id==0x101&&s.offered==4&&s.eligible_count==2,"ordered by score, strokes only");
  pass("shortlist_features_score_and_eligibility");}
 {std::vector<Meta> pool;for(unsigned i=0;i<200;++i){Meta m;m.id=std::uint16_t(0x200+i);m.shape=(i%4==0)?1:(i%4==1)?5:2;m.sx=1;m.sy=(i%3==0)?8.f:1.f;m.edir[1]=(i%5==0)?1.f:0.f;m.edir[0]=(i%5==0)?0.f:1.f;m.life=(i%7==0)?5:30;m.rate=(i%11==0)?9.f:0.5f;m.keys=i%13==0;pool.push_back(m);}
  Shortlist a;for(auto& m:pool)a.offer(m);require(a.n==kMaxShortlist,"capped");for(unsigned i=1;i<a.n;++i)require(!Shortlist::better(a.top[i],a.top[i-1]),"sorted by the total order");
  std::vector<Meta> rev(pool.rbegin(),pool.rend());Shortlist b;for(auto& m:rev)b.offer(m);for(unsigned i=0;i<a.n;++i)require(a.top[i].meta.id==b.top[i].meta.id,"independent of the iteration order");
  Meta none;none.shape=2;none.sx=3;none.sy=3;Shortlist e;for(int i=0;i<50;++i){none.id=std::uint16_t(i);e.offer(none);}require(e.n==0&&e.offered==50,"an empty shortlist is a valid result");
  pass("shortlist_is_capped_deterministic_order_independent_and_may_be_empty");}
 {Stats st;Meta m;m.shape=5;m.keys=1;st.add(m);m.shape=2;m.keys=0;m.child=true;st.add(m);m.shape=99;st.add(m);require(st.total==3&&st.by_shape[5]==1&&st.by_shape[2]==1&&st.by_shape[11]==1&&st.keyed==1&&st.with_child==2,"histogram");pass("stats_histogram");}
 std::printf("INVENTORY_LAB_PASS cases=%u POLICY_ONLY; NO_ENGINE_NO_GAME; REAL_INVENTORY_NOT_READ\n",cases);
}
