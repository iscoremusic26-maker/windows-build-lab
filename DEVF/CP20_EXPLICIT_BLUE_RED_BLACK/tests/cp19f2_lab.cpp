// CP19 FIX02 laboratory: the DLL's own cp19f2_policy.hpp (and the unchanged FIX01 gate of cp19_policy.hpp) driven against PORTS of the engine paths it
// depends on (official source @40457c6): Z2SceneMgr::loadSeWave/getWaveLoadStatus -> JASWaveArc::load(NULL) -> JASWaveArcLoader::getRootHeap() (sAramHeap
// when set, else the fixed audio ARAM heap) -> JASHeap::alloc; DVD completion -> TWaveGroup::onLoadDone -> JASBasicWaveBank::incWaveTable (first group
// wins a wave id); eraseSeWave -> JASHeap::free -> onEraseDone -> decWaveTable (clears every handle pointing at the erased group); Z2SceneMgr room loads
// (_load1stWaveInner_1 / load2ndDynamicWave); dCcS::SetAtTgGObjInf shield marking; daAlink_c::setDamagePoint/damageMagnification (negative control);
// dMeter2 pending life count; daAlink_c::draw damage colour schedule. It is a PORT, not the engine: group sizes and wave ids are lab values, no audio
// device, no collision geometry. The 0.16.1 field case (wave 0x5e refused at status 0) is reproduced as a negative control.
#include "../runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/cp19f2_policy.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <vector>
namespace f2=devf_cp19f2;namespace c1=devf_cp19;
unsigned cases=0;void need(bool q,const char*why){if(!q){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
template<class F>void test(const char*n,F f){f();++cases;std::printf("CASE_PASS %s\n",n);}
struct Events{std::vector<std::string> ev,why;std::vector<int> a,b,c,d;
 void operator()(const char*e,const char*w,int x,int y,int z,int u){ev.emplace_back(e);why.emplace_back(w);a.push_back(x);b.push_back(y);c.push_back(z);d.push_back(u);}
 unsigned count(const char*n)const{return unsigned(std::count(ev.begin(),ev.end(),n));}
 bool has(const char*n,const char*w)const{for(size_t i=0;i<ev.size();++i)if(ev[i]==n&&why[i]==w)return true;return false;}
 int index(const char*n)const{for(size_t i=0;i<ev.size();++i)if(ev[i]==n)return int(i);return -1;}};
// ---------------- engine port: ARAM heaps, wave groups, wave table ----------------
constexpr std::uint32_t MiB=1u<<20;
struct Heap{std::uint32_t size=0;std::map<int,std::uint32_t> children;   // group -> bytes (contiguity: lab heaps are compacted bump heaps)
 std::uint32_t used()const{std::uint32_t u=0;for(auto&kv:children)u+=kv.second;return u;}
 std::uint32_t largest_free()const{return size-used();}
 bool alloc(int g,std::uint32_t bytes){bytes=(bytes+31)&~31u;if(children.count(g)||bytes>largest_free())return false;children[g]=bytes;return true;}
 bool free(int g){return children.erase(g)>0;}};
struct Group{std::uint32_t bytes=0;std::vector<int> ids;bool exists=true;int status=0;Heap* heap=nullptr;};
struct Engine{
 // FIX01 readiness lifecycle
 bool ready=false,transition=false;int init_only=-1;   // init_only: 0 = init flag without audio heap, 1 = audio heap without init flag (lab of the FIX01 gate)
 Heap audio;std::deque<Heap> own_blocks;Heap* own=nullptr;Heap* sAramHeap=nullptr;std::uint32_t graph_free=14*MiB;
 std::map<int,Group> groups;std::map<int,int> handle;   // wave id -> group that registered it
 std::vector<std::pair<int,int>> dvd;                   // queued (group, completes at tick)
 unsigned tick=0;
 // Z2SceneMgr
 int req1=0,loaded1=0,req2=0,loaded2=0;
 // counters
 unsigned status_calls=0,load_calls=0,erase_calls=0,reserve_calls=0,own_alloc=0,illegal=0,native_refusals=0,root_swaps=0;
 Engine(){
  audio.size=10*MiB;
  auto add=[&](int g,std::uint32_t b,std::vector<int> ids){Group x;x.bytes=b;x.ids=ids;groups[g]=x;};
  add(0x00,3*MiB,{1,2,3});add(0x58,1500*1024,{4,5});                     // resident system groups (loadStaticWaves)
  add(0x4a,2*MiB,{400,401,402,51});                                        // field SE 1
  add(0x5e,1200*1024,{100,101,102,103,104,105,106,107,108,109,50,51});     // rooms 2/3/4: carries the E_OC family (ids 100..109) + shared 50/51
  add(0x5f,1600*1024,{300,301,302,50});                                    // rooms 0/5/7
  add(0x60,1800*1024,{200,201,202,203,51});                                // rooms 9/10/11
  add(0x61,1500*1024,{500,501});add(0x4b,1500*1024,{600,601,100,101});     // 0x4b: partial overlap with E_OC ids (lab value)
  add(0x30,1700*1024,{700,701,702});                                       // a dungeon group (other stage, other policy)
  add(0x19,1500*1024,{});                                                  // BGM-sized filler (field BGM wave), occupies the audio heap
  load_native(0x00);load_native(0x58);run_dvd(1000);
 }
 Heap* root(){return sAramHeap?sAramHeap:&audio;}   // JASWaveArcLoader::getRootHeap()
 int status(int g){if(!ready)++illegal;++status_calls;auto it=groups.find(g);return it==groups.end()?0:it->second.status;}   // getWaveLoadStatus
 bool load_native(int g){                                        // Z2SceneMgr::loadSeWave -> JASWaveArc::load(NULL)
  if(!ready&&tick>0)++illegal;++load_calls;auto it=groups.find(g);if(it==groups.end()||!it->second.exists)return false;Group&x=it->second;
  if(x.status!=0)return false;Heap* h=root();if(h==&audio&&sAramHeap)return false;
  if(!h->alloc(g,x.bytes)){if(h==&audio)++native_refusals;return false;}
  x.status=1;x.heap=h;dvd.push_back({g,int(tick)+3});return true;}
 bool erase(int g){if(!ready)++illegal;++erase_calls;auto it=groups.find(g);if(it==groups.end()||!it->second.heap)return false;Group&x=it->second;   // JASHeap::free -> onEraseDone
  x.heap->free(g);x.heap=nullptr;const bool was=x.status==2;x.status=0;
  if(was)for(auto&kv:handle)if(kv.second==g)kv.second=-1;   // decWaveTable: only handles pointing at THIS group, never re-pointed
  for(auto i=handle.begin();i!=handle.end();)if(i->second==-1)i=handle.erase(i);else ++i;return true;}
 void run_dvd(int n=1){for(int k=0;k<n;++k){++tick;for(auto i=dvd.begin();i!=dvd.end();){if(i->second<=int(tick)){Group&x=groups[i->first];if(x.status==1&&x.heap){x.status=2;for(int id:x.ids)if(!handle.count(id))handle[id]=i->first;}i=dvd.erase(i);}else ++i;}}}
 bool playable(int id)const{auto it=handle.find(id);return it!=handle.end()&&groups.at(it->second).status==2;}
 // Scene room change: _load1stWaveInner_1 (se1) + load2ndDynamicWave (se2), native heap only.
 void enter(int se1,int se2){
  req1=se1;req2=se2;
  if(loaded2&&req2!=loaded2){erase(loaded2);loaded2=0;}
  if(req1!=loaded1){if(loaded2&&erase(loaded2))loaded2=0;if(loaded1&&erase(loaded1))loaded1=0;loaded1=req1&&load_native(req1)?req1:0;}
  if(req2!=loaded2){if(loaded2&&erase(loaded2))loaded2=0;loaded2=req2&&load_native(req2)?req2:0;}
  run_dvd(5);
 }
};
// Io over the port: flags from the lifecycle only; audio members count calls and flag any call made while not ready.
struct Io{
 Engine& e;bool foreign_root=false;std::uint32_t keep_seen=0;unsigned bgm_calls=0;
 c1::AudioFlags flags()const{c1::AudioFlags f;f.manager=true;f.init=e.ready||e.init_only==0;f.heap=e.ready||e.init_only==1;f.reset_data=true;return f;}
 bool transition()const{return e.transition;}
 int status(std::uint8_t w){return e.status(w);}
 std::uint32_t own_used()const{return e.own?e.own->used():0u;}
 std::uint32_t game_free()const{return e.audio.largest_free();}
 bool reserve(std::uint32_t bytes,std::uint32_t keep,const char*& why){
  if(!e.ready)++e.illegal;++e.reserve_calls;keep_seen=keep;if(e.own)return true;
  if(e.graph_free<bytes||e.graph_free-bytes<keep){why="GRAPH_ARAM_LARGEST_FREE_BLOCK_BELOW_RESERVATION_PLUS_KEEP";return false;}
  e.own=&e.own_blocks.emplace_back();e.own->size=bytes;e.graph_free-=bytes;++e.own_alloc;return true;}
 bool load_own(std::uint8_t w,const char*& why){
  if(!e.own){why="NO_OWN_ARAM_BLOCK";return false;}
  if(e.sAramHeap!=nullptr){why="FOREIGN_WAVE_LOADER_ROOT_HEAP_ALREADY_SET_NOT_TOUCHED";return false;}
  e.sAramHeap=e.own;++e.root_swaps;const bool ok=e.load_native(w);e.sAramHeap=nullptr;
  if(!ok)why=own_used()==0?"LOADSEWAVE_FALSE_GROUP_ABSENT_ENTRY_INVALID_OR_LARGER_THAN_OWN_BLOCK":"LOADSEWAVE_FALSE_STATUS_NOT_IDLE";return ok;}
 c1::Bgm bgm(){if(!e.ready)++e.illegal;++bgm_calls;c1::Bgm b;b.dist=0;b.seq=0;return b;}
};
struct Rig{Engine e;Io io{e};f2::AudioPass st;Events ev;
 void pass(bool need=true,int n=1){for(int i=0;i<n;++i){f2::audio_pass(st,io,need,ev);e.run_dvd();}}
 void field_room(int room){static const std::map<int,int> se2={{2,0x5e},{3,0x5e},{4,0x5e},{0,0x5f},{5,0x5f},{7,0x5f},{9,0x60},{10,0x60},{11,0x60},{12,0x61},{1,0x4b},{6,0x4b}};e.enter(0x4a,se2.at(room));}
 void boot_to(int room){e.ready=true;e.load_native(0x19);e.run_dvd(5);field_room(room);}
 bool oc_family_audible()const{for(int id=100;id<=109;++id)if(!e.playable(id))return false;return true;}
};
// ---------------- B ports ----------------
// daAlink_c::damageMagnification + setDamagePoint (official @40457c6) for the NEGATIVE control: what 1 unit becomes on the native damage path.
struct NativeDamage{float multiplier=1.0f;bool zora=false,check_zora=false,magic_no_damage=false,double_defense=false;int life=40,rupees=500;
 int apply(int dmg){if(dmg<=0)return 0;float mag=1.0f*multiplier;if(zora&&check_zora)mag*=10.0f;const float m=float(dmg)*mag;int d=int(m);if(int(m*10.0f)%10!=0)++d;
  if(magic_no_damage){rupees-=d*10;return 0;}if(double_defense){d/=2;if(d<=0)d=1;}life=std::max(0,life-d);return d;}};
// dCcS::SetAtTgGObjInf: per (at, tg) hit of one pass: at gets hit + hit obj (+ shield if ChkShield or tg hit mark 8), tg gets hit + hit obj (+ shield only if
// ChkShield). Registered objects are cleared at the start of the pass (cCcS::ChkAtTg); unregistered ones keep their old bits.
struct Obj{bool hit=false,shield=false;const void* obj=nullptr;};
struct Pass{Obj sph[2],cyl[3];
 void clear_at(int j){sph[j]=Obj{};}void clear_tg(int i){cyl[i]=Obj{};}
 void hit(int j,int i,bool chk_shield,bool hitmark8=false){sph[j].hit=true;sph[j].obj=&cyl[i];sph[j].shield=chk_shield||hitmark8;cyl[i].hit=true;cyl[i].obj=&sph[j];cyl[i].shield=chk_shield;}
 bool confirmed()const{for(int j=0;j<2;++j)for(int i=0;i<3;++i)if(f2::shield_confirmed(sph[j].hit,sph[j].shield,sph[j].obj,&cyl[i],cyl[i].hit,cyl[i].shield,cyl[i].obj,&sph[j]))return true;return false;}};
// One Black blow lifecycle against the chip policy: frames of (at_set, contact) with the collision pass running after each POST.
struct Blackish{f2::ChipBlow b;std::uint32_t id=7,gen=1;unsigned seq=0;unsigned fury=0;Pass cc;unsigned charges=0,dups=0,notfury=0,stale=0;
 f2::ChipOp post(bool at_set,bool live=true){f2::ChipView v;v.id=id;v.gen=gen;v.seq=seq;v.fury=fury>0;v.at_set=at_set;v.live=live;v.shield=cc.confirmed();
  const auto op=f2::chip_step(b,v);if(op==f2::ChipOp::Charge)++charges;else if(op==f2::ChipOp::Duplicate)++dups;else if(op==f2::ChipOp::NotFury)++notfury;else if(op==f2::ChipOp::NotRegistered)++stale;return op;}
 // collision pass after a POST: spheres registered only when at_set; cylinders always registered.
 void collide(bool at_set,int contact_cyl,bool shield){if(at_set){cc.clear_at(0);cc.clear_at(1);}for(int i=0;i<3;++i)cc.clear_tg(i);if(at_set&&contact_cyl>=0){cc.hit(0,contact_cyl,shield);cc.hit(1,contact_cyl,shield);cc.cyl[contact_cyl].obj=&cc.sph[1];}}
 // a whole blow: start POST (seq++), 3 frames before the AT window, 5 AT frames with contact, 3 frames after.
 void blow(bool shield,int contact_frames=5){++seq;post(false);collide(false,-1,false);for(int k=0;k<3;++k){post(false);collide(false,-1,false);}
  for(int k=0;k<5;++k){post(true);collide(true,k<contact_frames?1:-1,shield);}for(int k=0;k<3;++k){post(false);collide(false,-1,false);}}
};
int main(){
 // ================= A. universal native SE wave residency =================
 test("a_negative_control_0_16_1_native_load_refused_at_status_0_by_full_audio_aram",[]{Rig r;r.boot_to(10);
  need(r.e.groups[0x5e].status==0&&r.e.loaded2==0x60,"room 10: 0x60 resident, 0x5e idle");
  const unsigned before=r.e.native_refusals;need(!r.e.load_native(0x5e),"0.16.1 path: loadSeWave(0x5e) false");need(r.e.native_refusals==before+1,"cause in the port: JASHeap::alloc on the audio ARAM heap");
  need(r.e.audio.largest_free()<r.e.groups[0x5e].bytes,"largest free block smaller than the group");need(!r.oc_family_audible(),"E_OC family silent in room 10 natively");});
 test("a_own_block_load_ready_room_10_game_audio_heap_untouched",[]{Rig r;r.boot_to(10);const auto audio_used=r.e.audio.used();
  r.pass(true,8);need(r.st.res.st==f2::Res::Ready,"ready");need(r.oc_family_audible(),"E_OC family audible");
  for(int id:{200,201,202,203,51})need(r.e.playable(id),"room 10 own sounds intact");
  need(r.e.audio.used()==audio_used,"game audio ARAM untouched");need(r.e.sAramHeap==nullptr&&r.e.root_swaps==1,"root heap restored after the one call");
  for(const char* n:{"AUDIO_DEPENDENCY_IDENTIFIED","AUDIO_RESERVATION_ACQUIRED","AUDIO_LOAD_REQUESTED","AUDIO_LOAD_ACCEPTED","AUDIO_LOAD_READY","AUDIO_NATIVE_REFUSAL_CAUSE_MEASURED"})need(r.ev.count(n)==1,n);
  need(r.ev.index("AUDIO_LOAD_ACCEPTED")<r.ev.index("AUDIO_LOAD_READY"),"READY only after the engine status 2");
  need(r.ev.has("AUDIO_NATIVE_REFUSAL_CAUSE_MEASURED","GAME_AUDIO_ARAM_LARGEST_FREE_BLOCK_SMALLER_THAN_THE_GROUP"),"cause measured");
  need(r.e.own_alloc==1&&r.e.erase_calls==0,"one reservation, no erase");});
 test("a_ready_never_reported_on_a_request_only",[]{Rig r;r.boot_to(7);f2::audio_pass(r.st,r.io,true,r.ev);
  need(r.ev.count("AUDIO_LOAD_ACCEPTED")==1&&r.ev.count("AUDIO_LOAD_READY")==0&&r.st.res.st==f2::Res::Loading,"accepted, not ready while status 1");
  r.e.run_dvd(1);f2::audio_pass(r.st,r.io,true,r.ev);need(r.ev.count("AUDIO_LOAD_READY")==0,"still loading");r.e.run_dvd(3);r.pass();need(r.ev.count("AUDIO_LOAD_READY")==1,"ready after DVD completion");});
 test("a_no_room_or_stage_list_any_scene_gets_the_dependency",[]{
  for(int room:{0,5,7,9,10,11,12}){Rig r;r.boot_to(room);r.pass(true,8);need(r.oc_family_audible(),"field room without native E_OC");}
  {Rig r;r.e.ready=true;r.e.load_native(0x19);r.e.run_dvd(5);r.e.enter(0x31,0x30);r.pass(true,8);need(r.oc_family_audible()&&r.e.playable(700),"other stage (dungeon group 0x30)");}
  {Rig r;r.e.ready=true;r.e.enter(0,0);r.pass(true,8);need(r.oc_family_audible(),"scene with no SE groups at all");}});
 test("a_scene_holds_the_group_not_claimed_then_own_after_scene_erase",[]{Rig r;r.boot_to(2);r.pass(true,5);
  need(r.ev.count("AUDIO_RESIDENT_NOT_OWNED")==1&&r.e.reserve_calls==0&&r.e.root_swaps==0,"scene's own 0x5e: no reservation, no load");need(r.oc_family_audible(),"native room");
  r.pass(true,50);need(r.ev.count("AUDIO_RESIDENT_NOT_OWNED")==1,"one line");
  r.field_room(10);r.pass(true,8);need(r.st.res.st==f2::Res::Ready&&r.oc_family_audible(),"room 10 after the scene erased its copy: own load");});
 test("a_scene_requests_the_group_while_own_is_resident",[]{Rig r;r.boot_to(10);r.pass(true,8);
  r.field_room(2);need(r.e.loaded2==0,"scene loadSeWave(0x5e) false at status 2: loadedSeWave_2 0 (port of load2ndDynamicWave)");
  need(r.oc_family_audible()&&r.e.playable(50),"room 2 sounds served by the resident own group");
  r.field_room(7);need(r.e.groups[0x5e].status==2&&r.oc_family_audible(),"leaving room 2 erases nothing (loadedSeWave_2 0)");});
 test("a_transitions_never_lose_room_sounds_mod_never_erases",[]{Rig r;r.boot_to(10);r.pass(true,8);
  const std::map<int,std::vector<int>> own={{0x5f,{300,301,302}},{0x60,{200,201,202,203}},{0x61,{500,501}},{0x4b,{600,601}}};
  for(int room:{7,10,9,12,6,7,3,10,0}){r.field_room(room);r.pass(true,6);const int g=r.e.loaded2;if(own.count(g))for(int id:own.at(g))need(r.e.playable(id),"room-own sound present after transition");
   need(r.oc_family_audible(),"E_OC family audible in every room");need(r.e.playable(51)||r.e.groups[0x4a].status==2,"field SE");}
  need(r.e.erase_calls>0&&r.e.own_alloc==1&&r.st.res.lost==0,"only the scene erased; own block once; never lost");});
 test("a_negative_control_release_on_leave_would_silence_a_shared_room_id",[]{
  // Why FIX02 keeps the group: own 0x5e loaded first in room 10 (claims id 50), room 7 then loads 0x5f (also id 50, handle stays on 0x5e).
  Rig r;r.boot_to(10);r.pass(true,8);r.field_room(7);need(r.e.playable(50),"room 7 id 50 audible through the resident group");
  Engine& e=r.e;e.erase(0x5e);need(!e.playable(50)&&e.groups[0x5f].status==2,"erasing the own group clears id 50 although 0x5f (loaded) carries it: decWaveTable");});
 test("a_gate_first_zero_audio_calls_before_ready_fix01_preserved",[]{Rig r;r.e.ready=false;
  for(int i=0;i<500;++i)f2::audio_pass(r.st,r.io,true,r.ev);
  need(r.e.status_calls==0&&r.e.reserve_calls==0&&r.e.root_swaps==0&&r.io.bgm_calls==0&&r.e.illegal==0,"no audio call while not ready");
  need(r.ev.count("CP19_AUDIO_NOT_READY_DEFERRED")==1&&r.st.gate.deferred_passes==500,"one bounded line");
  r.boot_to(10);r.pass(true,8);need(r.ev.count("AUDIO_READY")==1&&r.st.res.st==f2::Res::Ready&&r.e.illegal==0,"after ready");});
 test("a_init_flag_and_audio_heap_both_required",[]{for(int which:{0,1}){Rig r;r.e.init_only=which;for(int i=0;i<200;++i)f2::audio_pass(r.st,r.io,true,r.ev);
  need(r.e.status_calls==0&&r.e.reserve_calls==0&&r.io.bgm_calls==0&&r.ev.count("AUDIO_READY")==0,"one readiness fact alone is not readiness");}});
 test("a_scene_loading_the_group_itself_is_never_touched",[]{Rig r;r.e.ready=true;r.e.load_native(0x19);r.e.run_dvd(5);r.e.req1=0x4a;r.e.loaded1=r.e.load_native(0x4a)?0x4a:0;
  need(r.e.load_native(0x5e)&&r.e.groups[0x5e].status==1,"scene load in flight");const unsigned loads=r.e.load_calls;
  for(int i=0;i<3;++i)f2::audio_pass(r.st,r.io,true,r.ev);need(r.e.root_swaps==0&&r.e.load_calls==loads&&r.e.reserve_calls==0,"status 1 of a scene load: no reservation, no load call");
  r.e.run_dvd(5);r.pass(true,3);need(r.ev.count("AUDIO_RESIDENT_NOT_OWNED")==1&&r.e.root_swaps==0,"then reported as not owned");
  Rig q;q.boot_to(10);q.pass(true,8);const unsigned swaps=q.e.root_swaps;q.e.erase(0x5e);q.e.erase(0x60);q.e.loaded2=0;q.e.load_native(0x5e);need(q.e.groups[0x5e].status==1&&q.e.own->used()==0,"own block empty, scene reloading the group itself");
  for(int i=0;i<500;++i)f2::audio_pass(q.st,q.io,true,q.ev);need(q.e.root_swaps==swaps,"own block present: still no load call while the scene's load is in flight");
  need(q.ev.count("AUDIO_RESIDENT_LOST_ENGINE_ERASED")==1&&q.st.res.st!=f2::Res::Ready,"no stale READY once the group left the own block");});
 test("a_reset_and_return_to_menu_suspend_then_resume_without_second_load",[]{Rig r;r.boot_to(10);r.pass(true,8);const unsigned swaps=r.e.root_swaps,calls=r.e.status_calls;
  for(int ep=0;ep<4;++ep){r.e.ready=false;for(int i=0;i<40;++i)f2::audio_pass(r.st,r.io,true,r.ev);need(r.e.status_calls==calls+unsigned(ep)*10u,"no call while suspended");r.e.ready=true;r.pass(true,10);}
  need(r.e.root_swaps==swaps&&r.e.own_alloc==1&&r.ev.count("AUDIO_READY_AGAIN")==4&&r.e.illegal==0,"resident across suspensions, no second load");});
 test("a_graph_aram_too_small_bounded_refusals_then_give_up",[]{Rig r;r.e.graph_free=6*MiB;r.boot_to(10);
  r.pass(true,2000);need(r.ev.count("AUDIO_RESERVATION_REFUSED")==3&&r.ev.count("AUDIO_LOAD_GAVE_UP")==1,"three spaced refusals then give up");
  need(r.io.keep_seen==f2::kGraphKeep&&r.e.reserve_calls==3&&r.e.root_swaps==0,"keep enforced, no load without a block");
  int prev=-1;for(size_t i=0;i<r.ev.ev.size();++i)if(r.ev.ev[i]=="AUDIO_RESERVATION_REFUSED"){need(r.ev.c[i]>prev,"attempt counter");prev=r.ev.c[i];}});
 test("a_foreign_root_heap_never_touched",[]{Rig r;r.boot_to(10);Heap other;other.size=MiB;r.e.sAramHeap=&other;r.pass(true,3);
  need(r.ev.has("AUDIO_LOAD_REFUSED","FOREIGN_WAVE_LOADER_ROOT_HEAP_ALREADY_SET_NOT_TOUCHED")&&r.e.sAramHeap==&other&&r.e.root_swaps==0,"left as found");
  r.e.sAramHeap=nullptr;r.pass(true,400);need(r.st.res.st==f2::Res::Ready,"later, once free");});
 test("a_group_larger_than_block_refused_retry_spaced",[]{Rig r;r.e.groups[0x5e].bytes=5*MiB;r.boot_to(10);r.pass(true,10);
  need(r.ev.has("AUDIO_LOAD_REFUSED","LOADSEWAVE_FALSE_GROUP_ABSENT_ENTRY_INVALID_OR_LARGER_THAN_OWN_BLOCK")&&r.ev.count("AUDIO_LOAD_REFUSED")==1,"one refusal, spaced retry");
  r.pass(true,1000);need(r.ev.count("AUDIO_LOAD_REFUSED")==3&&r.ev.count("AUDIO_LOAD_GAVE_UP")==1&&r.e.own_alloc==1,"bounded, one reservation");});
 test("a_absent_group_refused",[]{Rig r;r.e.groups[0x5e].exists=false;r.boot_to(10);r.pass(true,5);need(r.ev.count("AUDIO_LOAD_REFUSED")==1&&!r.oc_family_audible(),"no fake ready");});
 test("a_transition_defers_load",[]{Rig r;r.boot_to(10);r.e.transition=true;r.pass(true,20);need(r.e.root_swaps==0&&r.e.reserve_calls==0,"nothing during a stage transition");
  r.e.transition=false;r.pass(true,8);need(r.st.res.st==f2::Res::Ready,"then loaded");});
 test("a_need_false_no_calls_beyond_the_gate",[]{Rig r;r.boot_to(10);r.pass(false,100);need(r.e.status_calls==0&&r.e.reserve_calls==0&&r.ev.count("AUDIO_DEPENDENCY_IDENTIFIED")==0,"no DEVF Bokoblin, no encounters: nothing");});
 test("a_shutdown_no_audio_call_idempotent_and_kept",[]{Rig r;r.boot_to(10);r.pass(true,8);const unsigned s=r.e.status_calls,e=r.e.erase_calls;
  f2::audio_shutdown(r.st,r.ev);f2::audio_shutdown(r.st,r.ev);need(r.ev.count("AUDIO_RESIDENT_KEPT_NO_RELEASE")==1&&r.e.status_calls==s&&r.e.erase_calls==e,"no call, one line");
  need(r.e.groups[0x5e].status==2&&r.oc_family_audible(),"group stays resident after unload");
  Io again{r.e};again.e.own=nullptr;f2::AudioPass st2;Events ev2;for(int i=0;i<5;++i)f2::audio_pass(st2,again,true,ev2);
  need(ev2.count("AUDIO_RESIDENT_NOT_OWNED")==1&&ev2.count("AUDIO_LOAD_REQUESTED")==0,"a later instance does not reload or claim it");});
 test("a_slow_dvd_reported_once",[]{Rig r;r.boot_to(10);f2::audio_pass(r.st,r.io,true,r.ev);r.e.dvd.clear();for(int i=0;i<1300;++i)f2::audio_pass(r.st,r.io,true,r.ev);
  need(r.ev.count("AUDIO_LOAD_NOT_READY_YET")==1&&r.ev.count("AUDIO_LOAD_READY")==0,"one line, no fake ready");});
 test("a_lost_after_engine_erase_bounded_reload",[]{Rig r;r.boot_to(10);r.pass(true,8);r.e.erase(0x5e);r.pass(true,2);
  need(r.ev.count("AUDIO_RESIDENT_LOST_ENGINE_ERASED")==1,"reported");r.pass(true,400);need(r.st.res.st==f2::Res::Ready&&r.e.own_alloc==1,"reloaded into the same block");});
 test("a_bgm_diagnostic_kept_bounded",[]{Rig r;r.boot_to(10);r.pass(true,50);need(r.ev.count("BGM_BATTLE_STATE")==1,"one line for an unchanged state");});
 // ================= B. Black fury shield chip =================
 test("b_shield_confirmation_needs_both_directions",[]{Pass p;p.hit(0,1,true);need(p.confirmed(),"shield pair");
  Pass q;q.hit(0,1,false);need(!q.confirmed(),"body hit");Pass m;m.hit(0,1,false,true);need(!m.confirmed(),"at shield bit from hit mark 8 alone is not a shield block");
  Pass o;o.hit(0,1,true);o.cyl[1].obj=&o.cyl[0];need(!o.confirmed(),"tg hit object of another attacker");Pass n;n.hit(0,1,true);n.sph[0].obj=&n.cyl[2];need(!n.confirmed(),"at hit object another cylinder");});
 test("b_normal_blow_blocked_no_chip",[]{Blackish k;k.blow(true);need(k.charges==0&&k.notfury>=1,"x1.15 blow: no chip");});
 test("b_fury_blow_blocked_exactly_one_two_spheres_persisting_flags",[]{Blackish k;k.fury=200;k.blow(true);need(k.charges==1&&k.dups>=4,"one charge, the other sphere and frames are duplicates");});
 test("b_two_chained_fury_blows_two_units",[]{Blackish k;k.fury=300;k.blow(true);k.blow(true);need(k.charges==2,"one per blow");});
 test("b_two_blacks_one_each",[]{Blackish a,b;a.fury=b.fury=300;b.id=8;a.blow(true);b.blow(true);need(a.charges==1&&b.charges==1,"per actor");});
 test("b_fury_captured_at_blow_start",[]{
  {Blackish k;k.fury=10;++k.seq;k.post(false);k.collide(false,-1,false);k.fury=0;for(int i=0;i<3;++i){k.post(false);k.collide(false,-1,false);}for(int i=0;i<5;++i){k.post(true);k.collide(true,1,true);}k.post(false);
   need(k.charges==1,"fury ending during the blow keeps the fury blow");}
  {Blackish k;k.fury=0;++k.seq;k.post(false);k.collide(false,-1,false);k.fury=300;for(int i=0;i<3;++i){k.post(false);k.collide(false,-1,false);}for(int i=0;i<5;++i){k.post(true);k.collide(true,1,true);}k.post(false);
   need(k.charges==0,"fury starting during a normal blow: no chip");}});
 test("b_stale_flags_before_registration_ignored",[]{Blackish k;k.fury=300;k.blow(true);
  // next blow: no AT yet, flags of the previous blow persist on the unregistered spheres, cylinders re-hit by a stale pair would need tg obj == sphere
  ++k.seq;k.post(false);k.cc.cyl[1].hit=true;k.cc.cyl[1].shield=true;k.cc.cyl[1].obj=&k.cc.sph[0];k.post(false);need(k.charges==1&&k.stale>=1,"not charged to the new blow");});
 test("b_flags_read_at_the_next_blow_start_belong_to_the_previous_blow",[]{Blackish k;k.fury=300;++k.seq;k.post(false);k.collide(false,-1,false);k.post(true);k.collide(true,1,true);
  ++k.seq;need(k.post(false)==f2::ChipOp::Charge&&k.charges==1,"judged against the blow before the sequence change");
  k.collide(false,-1,false);for(int i=0;i<3;++i)k.post(false);need(k.charges==1,"new blow not charged by it");});
 test("b_body_hit_and_miss_never_charge",[]{Blackish k;k.fury=300;k.blow(false);need(k.charges==0,"body hit");Blackish m;m.fury=300;m.blow(true,0);need(m.charges==0,"no contact");});
 test("b_generation_change_no_inheritance",[]{Blackish k;k.fury=300;k.blow(true);k.gen=2;k.seq=1;k.post(false);k.cc=Pass{};k.cc.hit(0,1,true);need(k.post(false)!=f2::ChipOp::Charge,"new generation: not registered yet");});
 test("b_new_generation_same_sequence_number_starts_clean",[]{Blackish k;k.fury=300;k.blow(true,0);need(k.charges==0&&k.b.registered,"generation 1: registered fury blow, no contact");
  k.gen=2;k.cc=Pass{};k.cc.hit(0,1,true);need(k.post(false)!=f2::ChipOp::Charge&&k.charges==0,"generation 2 with the same blow number inherits nothing");});
 test("b_dead_or_deleting_black_never_charges",[]{Blackish k;k.fury=300;++k.seq;k.post(false);k.collide(false,-1,false);k.post(true);k.collide(true,1,true);need(k.post(false,false)==f2::ChipOp::None&&k.charges==0,"live only");});
 test("b_exactly_one_unit_any_multiplier_any_armour_vs_native_path",[]{
  for(float mult:{1.0f,2.0f,5.0f,8.0f,10.0f})for(int armour=0;armour<4;++armour){
   const float before=0.0f;const float after=f2::chip_pending_after(before);need(f2::chip_readback_ok(before,after)&&after==-1.0f,"pending -1");
   need(f2::meter_apply(40,after,80)==39,"meter: exactly one unit");
   NativeDamage n;n.multiplier=mult;n.zora=armour==1;n.check_zora=false;n.magic_no_damage=armour==2;n.double_defense=armour==3;const int d=n.apply(1);
   if(mult>=2.0f&&armour==0)need(d==int(mult),"negative control: the native path would multiply (x2/x5/x8/x10)");
   if(armour==2)need(d==0&&n.rupees<500,"negative control: magic armour turns it into rupees");}
  NativeDamage z;z.zora=true;z.check_zora=true;need(z.apply(1)==10,"negative control: Zora fire/ice x10");});
 test("b_life_one_reaches_zero_never_negative",[]{need(f2::meter_apply(1,-1.0f,80)==0&&f2::meter_apply(0,-1.0f,80)==0,"clamped at 0; native death decides");});
 test("b_pending_sum_with_other_changes_kept",[]{const float other=-3.0f;const float after=f2::chip_pending_after(other);need(after==-4.0f&&f2::chip_readback_ok(other,after),"accumulates, never absorbs");
  need(f2::meter_apply(40,after+8.0f,80)==44,"a heal in the same frame survives");});
 // ================= B. flash =================
 test("b_flash_native_schedule_30_ticks_colour_and_pulse",[]{f2::Flash f;f2::flash_start(f);need(f.dmg==30&&f.col==18,"setDamageColorTime 32-(30%16)");
  unsigned on=0;int maxr=0;while(f2::flash_on(f)&&on<100){const auto c=f2::flash_color(f);const int t=f.dmg+f.col;const std::uint8_t base=t>32?15:t>16?10:5;
   const double ref=base*std::fabs(std::sin(double(std::int16_t(std::uint16_t((t*0x800)&0xffff)))*3.14159265358979/32768.0));need(std::fabs(double(c.r)-ref)<=1.0&&c.g==0&&c.b==0,"daAlink_c::draw formula");
   maxr=std::max(maxr,int(c.r));++on;f2::flash_tick(f);}
  need(on==30&&maxr>=14,"30 ticks, native peak R0");});
 test("b_flash_merge_never_lowers_native_and_restart_refreshes",[]{f2::Rgb n;n.r=12;n.g=3;f2::Rgb o;o.r=5;const auto c=f2::flash_merge(n,o);need(c.r==12&&c.g==3&&c.b==0,"max");
  f2::Flash f;f2::flash_start(f);for(int i=0;i<20;++i)f2::flash_tick(f);f2::flash_start(f);need(f.dmg==30&&f.started==2,"second chip restarts");});
 test("b_flash_is_off_without_a_chip",[]{f2::Flash f;need(!f2::flash_on(f)&&f2::flash_color(f).r==0,"nothing to write");for(int i=0;i<5;++i)f2::flash_tick(f);need(f.ticks==0,"no ticks counted");});
 std::printf("STATE_BYTES %zu %zu %zu\n",sizeof(f2::AudioPass),sizeof(f2::ChipBlow),sizeof(f2::Flash));
 std::printf("CP19F2_LAB_PASS cases=%u\n",cases);return 0;
}
