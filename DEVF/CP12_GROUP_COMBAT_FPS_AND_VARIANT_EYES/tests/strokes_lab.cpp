// CP12 offline lab (0.9.0): Black aura colour intense red; unbudgeted wait for the first draw (tevStr). (CP10.6D-FIX02, 0.8.8:) body-hugging band, torso centre and presence on the same shared route. (FIX01, 0.8.7:) ONE corrected strokes route with a non-degenerate draw basis. (CP10.6D:) ONE corrected strokes route (0x0B9D, canonical emitter frame, shared geometry) used by Blue and Black; only the colour differs.
// The REAL devf_vis::step (CP10.6A, unchanged) is driven through the REAL devf_stk::AuraPart against a fake engine that models the contract read from the
// source: dPa_control_c::set returns 0 when the tevStr is not ready or the pool is exhausted, an emitter nobody refreshes during a tick is removed by the
// engine, colour/alpha/scale/frame are per-emitter, and a resource id exists in the bank only if the fake says so. A small COVERAGE MODEL ports the exact
// birth formulas (JPAVolumeCircle, JPAResource::calcWorkData_c, JPABaseParticle::init_p) to show what the canonical frame changes. It proves policy and
// geometry arithmetic; it does NOT prove what is drawn on screen (human Windows round).
#include "../expansion/strokes_policy.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>
namespace vv=devf_vis;
namespace sk=devf_stk;
using vv::Variant;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
// Authored (sword-frame) values a resource may carry; a fresh emitter copies them (JPABaseEmitter::init) until configure() writes the canonical frame.
struct Frame {float tx=0,ty=0,tz=0;int rx=0,ry=0,rz=0;float sx=1,sy=1,sz=1,dx=0,dy=1,dz=0,sweep=1,min_rad=0;};
struct Em {bool alive=false,refreshed=false,stopped=false,continuous=false;sk::Rgba base{255,255,195,150};unsigned rate_step=1;int alpha=0,r=0,g=0,b=0,er=0,eg=0,eb=0;float x=0,y=0,z=0,vscale=1,wscale=1,lscale=1;Frame f;unsigned refreshes=0,configures=0;std::uint16_t id=0;};
struct FakeEngine {
 bool bank_ok=true,pool_full=false,lookup_fail=false,color_mismatch=false,geo_mismatch=false,presence_mismatch=false;unsigned not_ready=0;std::set<std::uint16_t> missing;Frame authored;
 std::vector<Em> ems{Em{}};std::uint32_t next=1;std::vector<std::uint16_t> set_ids,probe_ids;unsigned set_calls=0,probe_calls=0,created=0,stops=0,orphans=0;int max_frame=0;
 bool probe(std::uint16_t id,const char*& why){++probe_calls;probe_ids.push_back(id);if(!bank_ok){why="COMMON_PARTICLE_BANK_NOT_LOADED";return false;}if(missing.count(id)){why="RESOURCE_MISSING_FROM_COMMON_BANK";return false;}return true;}
 bool ready(){if(not_ready){--not_ready;return false;}return true;}
 std::uint32_t set(std::uint16_t id,std::uint32_t key,float x,float y,float z){
  ++set_calls;set_ids.push_back(id);
  if(key&&key<ems.size()&&ems[key].alive){Em& m=ems[key];m.refreshed=true;m.x=x;m.y=y;m.z=z;++m.refreshes;return key;}
  if(pool_full)return 0;
  const std::uint32_t k=next++;ems.resize(k+1);Em& m=ems[k];m.alive=true;m.refreshed=true;m.x=x;m.y=y;m.z=z;m.id=id;m.f=authored;++created;return k;
 }
 bool configure(std::uint32_t key,const sk::Geometry& g,const sk::Colour& c,int alpha,bool created_now,sk::Readback& rb){
  if(lookup_fail||key>=ems.size()||!ems[key].alive)return false;
  Em& m=ems[key];++m.configures;
  m.f.tx=m.f.ty=m.f.tz=0;m.f.rx=m.f.ry=m.f.rz=0;m.f.sx=m.f.sy=m.f.sz=1;m.f.dx=g.dir_x;m.f.dy=g.dir_y;m.f.dz=g.dir_z;m.f.sweep=g.sweep;m.f.min_rad=float(g.min_rad_pm)/1000.0f;   // the mod's canonical frame
  m.vscale=g.volume_scale;m.wscale=g.width_scale;m.lscale=g.length_scale;m.r=c.r;m.g=c.g;m.b=c.b;m.er=c.er;m.eg=c.eg;m.eb=c.eb;m.alpha=alpha;if(created_now&&max_frame!=0)m.continuous=true;
  rb.ok=!color_mismatch;rb.geo_ok=!geo_mismatch;rb.min_rad=m.f.min_rad;rb.sweep=m.f.sweep;rb.dir=sk::V3{m.f.dx,m.f.dy,m.f.dz};rb.basis=sk::draw_basis(sk::V3{0,1,0},rb.dir);/* as the mod: identity global rotation read back */m.base=sk::kBasePrm;m.rate_step=sk::kRateStep;rb.base_prm=m.base;rb.rate_step=m.rate_step;rb.presence_ok=!presence_mismatch;rb.r=m.r;rb.g=m.g;rb.b=m.b;rb.a=m.alpha;return true;
 }
 void stop(std::uint32_t key){if(key<ems.size()){ems[key].stopped=true;ems[key].alpha=0;}++stops;}
 void engine_tick(){for(auto& m:ems){if(m.alive&&!m.refreshed){m.alive=false;++orphans;}m.refreshed=false;}}
 unsigned live()const{unsigned n=0;for(auto& m:ems)if(m.alive)++n;return n;}
};
struct FakeEye {unsigned tex=0;bool foreign=false;unsigned writes=0;};
struct Fx {
 sk::AuraPart<FakeEngine>& ap;FakeEye& eyes;const char* last="NONE";
 bool aura(vv::Entry& e,int l,int a){const bool ok=ap.aura(e,l,a);last=ap.why;return ok;}
 void aura_release(vv::Entry& e){ap.aura_release(e);}
 bool eye(vv::Entry&,unsigned idx){if(eyes.foreign){last="EYE_PACKET_TEXTURE_NOT_OURS";return false;}eyes.tex=idx;++eyes.writes;return true;}
 const char* reason()const{return last;}
};
struct Run {
 FakeEngine own;FakeEngine* en;vv::Ledger L;std::array<sk::Retry,vv::kCapacity> retries{};FakeEye eyes;vv::Entry* e=nullptr;std::uint32_t id=1,gen=1;std::vector<sk::Note> notes;std::vector<vv::Event> events;float px=10,py=20,pz=30,eye_dy=200;bool torso_ok=false;float c0x=0,c0z=0,c1x=0,c1z=0;
 explicit Run(Variant v,std::uint32_t id_=1,FakeEngine* shared=nullptr):en(shared?shared:&own){id=id_;e=L.record(id,this,gen,v);require(e!=nullptr,"record");}
 void tick(bool aura,bool eyes_want=false){
  sk::Retry& rt=sk::retry_for(retries,std::size_t(e-L.entries.data()),e->id,e->generation);
  sk::AuraPart<FakeEngine> ap(*en,rt);ap.fx=px;ap.fy=py;ap.fz=pz;ap.eye_dy=eye_dy;ap.torso_ok=torso_ok;ap.c0x=c0x;ap.c0z=c0z;ap.c1x=c1x;ap.c1z=c1z;Fx fx{ap,eyes};
  const auto out=vv::step(L,*e,fx,aura,eyes_want);
  for(unsigned i=0;i<out.n;++i){events.push_back(out.ev[i]);}
  for(unsigned i=0;i<ap.notes.count;++i){notes.push_back(ap.notes.n[i]);}
  en->engine_tick();
 }
 void run(unsigned n,bool a,bool ey=false){for(unsigned i=0;i<n;++i)tick(a,ey);}
 unsigned count(vv::Ev k){unsigned n=0;for(auto& x:events)if(x.kind==k)++n;return n;}
 unsigned notes_of(sk::Note::Kind k){unsigned n=0;for(auto& x:notes)if(x.kind==k)++n;return n;}
 const Em& em()const{return en->ems[e->key];}
};
// ---- coverage model: exact birth arithmetic of a circle-volume particle (no drawing, no texture, no curves) ----
struct Rng {std::uint32_t s=0x1234567u;std::uint32_t u(){s=s*1664525u+1013904223u;return s;}std::int16_t ss(){return std::int16_t(u()>>16);}float f(){return float(u()>>8)/16777216.0f;}};
struct Birth {float x,y,z;};
// JPAVolumeCircle: theta = sweep x s16, distance = size x (minRad + r(1-minRad)); local pos (d sin, 0, d cos).
// calcWorkData_c: SR = globalRot(identity) x localRot x localScale; mGlobalPos = trs + globalScale x localTrs. init_p: pos = mGlobalPos + SR(local) x globalScale.
Birth birth(const Frame& f,float volume_size,float gscale,float ox,float oy,float oz,Rng& rng){
 const float theta=float(std::int16_t(f.sweep*float(rng.ss())))*(6.28318530718f/65536.0f);
 const float d=volume_size*(f.min_rad+rng.f()*(1.0f-f.min_rad));
 float lx=d*std::sin(theta),ly=0.0f,lz=d*std::cos(theta);
 lx*=f.sx;ly*=f.sy;lz*=f.sz;                                                     // local scale
 const float ax=float(f.rx)*0.0174533f;const float cy=std::cos(ax),sy=std::sin(ax);   // local rotation about X (enough to model a tilted ring)
 const float ry=ly*cy-lz*sy,rz=ly*sy+lz*cy;ly=ry;lz=rz;
 return Birth{ox+gscale*f.tx+lx*gscale,oy+gscale*f.ty+ly*gscale,oz+gscale*f.tz+lz*gscale};
}
struct Coverage {unsigned sectors=0;float min_r=1e9f,max_r=0,max_dy=0;};
Coverage coverage(const Frame& f,float gscale,float outside=sk::kTorsoColliderRadius){Rng rng;Coverage c;bool hit[8]{};
 for(int i=0;i<2000;++i){const Birth b=birth(f,24.0f,gscale,0,0,0,rng);const float r=std::sqrt(b.x*b.x+b.z*b.z);
  if(r>c.max_r)c.max_r=r;
  if(r<c.min_r)c.min_r=r;
  if(std::fabs(b.y)>c.max_dy)c.max_dy=std::fabs(b.y);
  const float a=std::atan2(b.x,b.z);int s=int(std::floor((a+3.14159265f)/(6.28318530718f/8.0f)));if(s<0)s=0;if(s>7)s=7;
  if(r>outside)hit[s]=true;}                                                    // a sector counts only if particles are born beyond `outside` there
 for(bool h:hit)if(h)++c.sectors;
 return c;}
// ---- engine port of the draw basis (independent of devf_stk::draw_basis; doubles of the exact float rules) ----
// init_p: mBaseAxis = Y column of work->mGlobalRot (= global x local rotation). JPADrawDirection/JPADrawRotDirection: axisY = p_direction[dir type 3] =
// mGlobalEmtrDir = global x local x mLocalDir; if axisY^2 <= eps return; normalize; axisZ = cross(mBaseAxis, axisY); if axisZ^2 <= eps return;
// normalize; mBaseAxis = normalize(cross(axisY, axisZ)) -- written to the REAL particle only on non-presentation frames (dusk interp draws a copy).
struct D3 {double x,y,z;};
D3 cr(D3 a,D3 b){return D3{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double dt(D3 a,D3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
D3 nz(D3 a){const double l=std::sqrt(dt(a,a));return D3{a.x/l,a.y/l,a.z/l};}
const double kEps=32.0*1.1920928955078125e-7;
struct Drawn {bool drawn=false;double cross_len=0;D3 axis_y{},width{};};
Drawn engine_draw(D3& base,D3 dir_world,bool write_back){
 Drawn d;D3 ay=dir_world;if(dt(ay,ay)<=kEps)return d;ay=nz(ay);D3 z=cr(base,ay);if(dt(z,z)<=kEps)return d;d.cross_len=std::sqrt(dt(z,z));z=nz(z);
 const D3 nb=nz(cr(ay,z));if(write_back)base=nb;d.drawn=true;d.axis_y=ay;d.width=nb;return d;}
// Draw `frames` frames of one particle born with the identity global rotation and local rotation 0 (base = (0,1,0)) for a stroke direction.
struct Run2 {unsigned drawn=0;double min_cross=1e9;Drawn last;};
Run2 engine_frames(D3 dir,unsigned frames,bool presentation_copy){Run2 r;D3 base{0,1,0};
 for(unsigned i=0;i<frames;++i){D3 b=base;Drawn d=engine_draw(presentation_copy?b:base,dir,true);if(d.drawn){++r.drawn;if(d.cross_len<r.min_cross)r.min_cross=d.cross_len;r.last=d;}}return r;}
// JPAGetDirMtx column 2 (the velocity direction with spread 0, JPABaseParticle::init_p velDir) -- exact port of JPAMath.cpp.
D3 dir_mtx_col2(D3 p){double lx=p.y,ly=-p.x;const double len=std::sqrt(lx*lx+ly*ly);if(len<=kEps){lx=ly=0;}else{lx/=len;ly/=len;}return D3{-(ly*len),lx*len,p.z};}
int main(){
 // ---------- resource selection: one shared resource ----------
 {static_assert(sk::kResource==0x0B9D&&sk::kBank==0&&sk::kEmittersPerActor==1,"shared route table");
  require(sk::resource_for(Variant::Blue)==0x0B9D&&sk::resource_for(Variant::Black)==0x0B9D&&sk::resource_for(Variant::Red)==0&&sk::resource_for(Variant::None)==0,"Blue AND Black use 0x0B9D, nothing for the rest");
  for(std::uint16_t bad:{0x0A58,0x0A59,0x0046,0x8046,0x072F}){require(sk::never_used(bad),"forbidden ids, including Blue's old 0x0A58");}
  require(!sk::never_used(0x0B9D),"the shared id is not forbidden");
  Run b(Variant::Blue);b.run(40,true);b.run(40,false);require(!b.en->set_ids.empty(),"engine was called");for(auto id:b.en->set_ids)require(id==0x0B9D,"Blue only ever passes 0x0B9D");for(auto id:b.en->probe_ids)require(id==0x0B9D,"Blue probes only 0x0B9D");
  Run k(Variant::Black);k.run(20,true,true);for(auto id:k.en->set_ids)require(id==0x0B9D,"Black only ever passes 0x0B9D");for(auto id:k.en->probe_ids)require(id==0x0B9D,"Black probes only 0x0B9D");
  for(const Run* r:{&b,&k}){for(auto id:r->en->set_ids)require(id!=0x0A58&&id!=0x0A59&&id!=0x0046&&id!=0x8046&&id!=0x072F,"never SWL_A, SWL_B, WindLine, 0x8046 nor the retired glow");}
  pass("blue_and_black_use_only_0x0B9D_blue_never_touches_its_old_0x0A58_route");}
 {Run r(Variant::Blue);vv::Entry fake=*r.e;fake.variant=Variant::Red;sk::Retry rt;sk::AuraPart<FakeEngine> ap(*r.en,rt);require(!ap.aura(fake,10,100)&&r.en->set_calls==0&&r.en->probe_calls==0,"a variant without a resource is refused before any engine call");
  vv::Ledger L;int d=0;require(L.record(1,&d,1,Variant::Red)==nullptr&&L.record(2,&d,1,Variant::None)==nullptr,"Red and unclassified never get a visual record");
  pass("red_and_unclassified_never_request_an_emitter");}
 // ---------- the unification itself ----------
 {require(&sk::geometry_for(Variant::Blue)==&sk::geometry_for(Variant::Black)&&&sk::geometry_for(Variant::Blue)==&sk::kGeometry,"ONE geometry object for both variants");
  Run b(Variant::Blue,1);Run k(Variant::Black,2);b.run(30,true);k.run(30,true,true);const Em& eb=b.em();const Em& ek=k.em();
  require(eb.id==ek.id&&eb.vscale==ek.vscale&&eb.wscale==ek.wscale&&eb.lscale==ek.lscale&&eb.alpha==ek.alpha&&eb.y==ek.y&&eb.x==ek.x&&eb.z==ek.z,"same resource, scales, alpha and placement");
  require(eb.f.sweep==ek.f.sweep&&eb.f.min_rad==ek.f.min_rad&&eb.f.tx==ek.f.tx&&eb.f.rx==ek.f.rx&&eb.f.sx==ek.f.sx&&eb.f.sz==ek.f.sz&&eb.f.dx==ek.f.dx&&eb.f.dy==ek.f.dy&&eb.f.dz==ek.f.dz,"same canonical frame and stroke direction");
  require(eb.r!=ek.r&&eb.g!=ek.g,"only the colour differs");
  require(eb.r==80&&eb.g==190&&eb.b==255&&ek.r==255&&ek.g==32&&ek.b==24,"Blue blue/cyan (80,190,255) unchanged, Black intense red (255,32,24)");
  require(eb.base.a==ek.base.a&&eb.base.r==ek.base.r&&eb.rate_step==ek.rate_step,"same presence settings (base colour, rate step)");
  require(eb.b>eb.r&&eb.g>eb.r&&ek.r>=4*ek.g&&ek.r>=4*ek.b&&ek.g>=ek.b&&ek.g<=48,"Blue leans blue-green (cyan); Black red-dominant: not pink (blue <= green), not orange (green small)");
  pass("blue_and_black_emitters_are_identical_except_colour");}
 // ---------- Blue lifecycle ----------
 {Run r(Variant::Blue);r.run(5,false);require(r.en->set_calls==0&&r.en->probe_calls==0&&r.en->live()==0&&r.events.empty(),"inactive: engine untouched, nothing probed");
  r.run(12,true);require(r.e->aura_level==100&&r.en->live()==1&&r.en->created==1,"faded in over 10 ticks with ONE emitter");
  const Em& em=r.em();require(em.id==0x0B9D&&em.vscale==2.4f&&em.wscale==0.70f&&em.lscale==0.60f&&em.f.min_rad==0.72f&&em.f.sweep==1.0f&&em.alpha==255,"shared body-hugging geometry and alpha cap 255");
  require(r.notes_of(sk::Note::Requested)==1&&r.notes_of(sk::Note::Acquired)==1&&r.notes_of(sk::Note::ColorApplied)==1&&r.notes_of(sk::Note::ColorUnavailable)==0&&r.notes_of(sk::Note::TuningApplied)==1,"requested once, acquired once, colour once, tuning once");
  require(r.count(vv::Ev::AuraOn)==1,"one ACTOR_STATE_ON");r.run(20,true);require(r.en->created==1&&r.em().refreshes>=30,"refreshed every tick, never recreated");
  unsigned ticks=0;while(r.e->aura_level>0&&ticks<40){r.tick(false);++ticks;}require(ticks==15,"15 ticks out");require(r.count(vv::Ev::AuraOff)==1&&r.e->key==0&&r.en->stops==1,"released once");
  r.run(3,false);require(r.en->live()==0&&r.en->orphans==1,"the engine removed the emitter nobody refreshes");
  pass("blue_fade_in_10_out_15_one_emitter_then_released");}
 {Run r(Variant::Blue);r.en->not_ready=4;r.run(2,true);
  require(r.count(vv::Ev::AuraOn)==1&&r.en->created==0&&r.notes_of(sk::Note::Acquired)==0,"ACTOR_STATE_ON is logged while no emitter exists: state is not an emitter");
  r.run(10,true);require(r.en->created==1&&r.notes_of(sk::Note::Acquired)==1,"the emitter is acquired later: a separate event");
  pass("actor_state_on_is_distinct_from_emitter_acquired");}
 {Run r(Variant::Blue);r.run(30,true);r.run(7,false);const int mid=r.e->aura_level;require(mid>0&&mid<100,"mid fade-out");const unsigned created=r.en->created;r.run(2,true);require(r.en->created==created&&r.e->aura_level>mid-14,"resume from the current level, same emitter");
  r.run(20,true);require(r.e->aura_level==100&&r.count(vv::Ev::AuraOff)==0,"never released in between");pass("blue_reactivation_during_fade_out_resumes_same_emitter");}
 // ---------- Black lifecycle ----------
 {Run r(Variant::Black);r.run(15,true,true);const Em& em=r.em();require(em.id==0x0B9D&&em.r==255&&em.g==32&&em.b==24&&em.er==130&&em.eg==0&&em.eb==6&&em.alpha==255&&r.eyes.tex==7,"Black intense red (prm 255,32,24 / env 130,0,6), alpha 255, eye variant 7 at full fury");
  const unsigned created=r.en->created,ev=r.events.size(),nn=r.notes.size();r.run(600,true,true);
  require(r.en->created==created&&r.events.size()==ev&&r.notes.size()==nn&&r.eyes.tex==7&&r.en->stops==0,"refresh: no new emitter, no event, no note, no stop, no flicker");
  r.run(40,false,false);require(r.e->aura_level==0&&r.eyes.tex==0&&r.en->stops==1&&r.count(vv::Ev::EyesOff)==1,"fury over: aura stopped, eyes native");r.run(3,false);require(r.en->live()==0,"no residue");
  pass("black_strokes_and_eyes_on_with_fury_stable_through_refresh_then_clean_off");}
 // ---------- transient engine refusals ----------
 {Run r(Variant::Blue);r.en->not_ready=5;r.run(3,true);require(r.en->live()==0&&r.e->aura_fault==false&&r.e->aura_live,"tevStr not ready: no emitter, no fault, logic stays on");
  r.run(10,true);require(r.en->live()==1&&r.notes_of(sk::Note::Waiting)==1&&r.notes_of(sk::Note::RetryStart)==0&&r.notes_of(sk::Note::Acquired)==1&&r.notes_of(sk::Note::Requested)==1&&r.count(vv::Ev::AuraRejected)==0,"waited for the first draw (one PENDING), then acquired");
  bool late=false;for(auto& n:r.notes)if(n.kind==sk::Note::Acquired&&std::string(n.why)=="ACQUIRED_AFTER_FIRST_DRAW"&&n.fails==0)late=true;require(late,"the acquisition records that it came after the first draw, without consuming the retry budget");
  pass("tevstr_not_ready_waits_for_the_first_draw_not_a_fault");}
 {Run r(Variant::Blue);r.en->not_ready=1000;const unsigned calls0=r.en->set_calls;r.run(900,true);
  require(r.en->live()==0&&!r.e->aura_fault&&r.count(vv::Ev::AuraRejected)==0&&r.en->set_calls==calls0,"an actor never drawn for 30 s: still pending, no rejection, set() never called (0.8.9 gave up after 60 ticks)");
  require(r.notes_of(sk::Note::Waiting)==1&&r.notes_of(sk::Note::Requested)==1,"one REQUESTED and one PENDING line for the whole wait: no spam");
  r.run(200,true);require(r.en->live()==1&&r.notes_of(sk::Note::Acquired)==1&&r.e->aura_level==100,"the moment it is drawn, the aura appears (the 454/455/772/773/1574 case of the 0.8.9 log)");
  Run q(Variant::Black);q.en->not_ready=100;q.en->pool_full=true;q.run(400,true,true);
  require(q.count(vv::Ev::AuraRejected)==1&&q.e->aura_fault&&q.en->set_calls==sk::kRetryTicks+1,"engine refusals AFTER the first draw stay bounded exactly as before (61 set() calls, one rejection)");
  pass("waiting_for_the_first_draw_is_unbudgeted_engine_refusals_stay_bounded");}
 {Run r(Variant::Black);r.en->pool_full=true;r.run(100,true,true);
  require(r.count(vv::Ev::AuraRejected)==1&&r.e->aura_fault&&r.e->aura_level==0&&r.en->live()==0,"pool exhausted for good: rejected once, level 0");
  require(r.en->set_calls==sk::kRetryTicks+1&&r.notes_of(sk::Note::Exhausted)==1,"exactly kRetryTicks+1 attempts, then no more calls");
  const unsigned calls=r.en->set_calls;r.run(200,true,true);require(r.en->set_calls==calls&&r.count(vv::Ev::AuraRejected)==1,"no retries after the rejection, no log spam");
  pass("pool_exhaustion_is_bounded_then_rejected_once_for_the_generation");}
 {Run r(Variant::Blue);r.en->pool_full=true;r.run(30,true);require(r.en->live()==0&&!r.e->aura_fault,"still retrying");r.en->pool_full=false;r.run(3,true);
  require(r.en->live()==1&&r.notes_of(sk::Note::Acquired)==1&&r.count(vv::Ev::AuraRejected)==0&&sk::retry_for(r.retries,std::size_t(r.e-r.L.entries.data()),1,1).fails==0,"recovered, retry count reset");
  pass("recovery_when_the_pool_frees_up_within_the_retry_budget");}
 {Run r(Variant::Blue);r.en->bank_ok=false;r.run(100,true);require(r.en->set_calls==0,"bank not loaded: set() is never called");require(r.count(vv::Ev::AuraRejected)==1&&r.e->aura_fault,"rejected after the budget");
  Run q(Variant::Blue);q.en->bank_ok=false;q.run(10,true);q.en->bank_ok=true;q.run(5,true);require(q.en->live()==1,"bank appeared in time: works");
  pass("missing_bank_never_reaches_set_and_is_rejected_cleanly_or_recovers");}
 {FakeEngine shared;shared.missing.insert(0x0B9D);Run blue(Variant::Blue,1,&shared);Run black(Variant::Black,2,&shared);blue.run(100,true);black.run(100,true,true);
  require(blue.count(vv::Ev::AuraRejected)==1&&black.count(vv::Ev::AuraRejected)==1&&shared.created==0&&shared.set_calls==0,"shared resource missing: both auras rejected once, set() never called");
  require(black.eyes.tex==7&&!black.e->eye_fault,"the Black's purple eyes do not depend on the strokes resource");
  for(auto id:shared.probe_ids)require(id==0x0B9D,"no fallback to another id");
  pass("a_missing_shared_resource_rejects_both_auras_cleanly_without_fallback_and_keeps_the_eyes");}
 {Run r(Variant::Blue);r.en->lookup_fail=true;r.run(100,true);require(r.count(vv::Ev::AuraRejected)==1&&r.e->aura_fault,"lookup always failing: bounded, rejected once");
  Run q(Variant::Blue);q.en->color_mismatch=true;q.run(15,true);require(q.notes_of(sk::Note::ColorUnavailable)==1&&q.notes_of(sk::Note::ColorApplied)==0&&q.en->live()==1,"colour readback mismatch reported, emitter still works");
  pass("lookup_failure_bounded_and_colour_unavailable_reported_honestly");}
 {Run r(Variant::Blue);r.run(20,true);const auto k=r.e->key;r.en->ems[k].alive=false;
  r.run(3,true);require(r.e->key!=k&&r.en->live()==1&&r.en->created==2&&r.notes_of(sk::Note::ColorApplied)==2&&r.notes_of(sk::Note::TuningApplied)==2&&r.notes_of(sk::Note::Acquired)==2&&r.notes_of(sk::Note::Requested)==1,"expired handle: exactly one new emitter, never two alive");
  require(r.em().f.sweep==1.0f&&r.em().f.sz==1.0f,"the replacement emitter gets the canonical frame too");
  pass("expired_emitter_is_recreated_once_never_duplicated");}
 // ---------- ownership / cleanup / bounds / isolation ----------
 {std::array<sk::Retry,4> t{};auto& x=sk::retry_for(t,0,7,1);x.fails=9;require(sk::retry_for(t,0,7,1).fails==9,"same generation keeps it");require(sk::retry_for(t,0,7,2).fails==0&&sk::retry_for(t,0,8,2).fails==0,"new generation or id starts clean");
  pass("retry_state_is_stamped_by_id_and_generation");}
 {FakeEngine shared;vv::Ledger L;std::array<sk::Retry,vv::kCapacity> rt{};FakeEye eyes;unsigned rejected=0;std::vector<vv::Entry*> es;
  for(unsigned i=0;i<10;++i){static int dummy[16];es.push_back(L.record(100+i,&dummy[i],1,i%2?Variant::Black:Variant::Blue));require(es.back(),"record");}
  for(unsigned t=0;t<40;++t){for(auto* e:es){sk::AuraPart<FakeEngine> ap(shared,sk::retry_for(rt,std::size_t(e-L.entries.data()),e->id,e->generation));Fx fx{ap,eyes};const auto o=vv::step(L,*e,fx,true,false);for(unsigned i=0;i<o.n;++i)if(o.ev[i].kind==vv::Ev::AuraRejected)++rejected;}shared.engine_tick();}
  require(shared.live()==vv::kMaxAuras&&rejected==10-vv::kMaxAuras,"at most 8 emitters, the rest rejected cleanly");
  std::set<std::uint32_t> owners;for(auto& m:shared.ems)if(m.alive)owners.insert(m.id);require(owners.size()==1&&*owners.begin()==0x0B9D,"all live emitters are the one shared resource");
  pass("emitter_capacity_is_bounded_and_explicit_one_emitter_per_actor");}
 {Run r(Variant::Black);r.en->pool_full=true;r.run(100,true,true);require(r.e->aura_fault&&r.eyes.tex==7&&r.count(vv::Ev::EyesRejected)==0,"strokes failure: the eyes are not touched");r.run(40,false,false);require(r.eyes.tex==0&&r.count(vv::Ev::EyesOff)==1,"and still return to native");
  Run q(Variant::Black);q.eyes.foreign=true;q.run(30,true,true);require(q.count(vv::Ev::EyesRejected)==1&&q.e->eye_fault&&q.en->live()==1&&!q.e->aura_fault,"eye failure: the aura keeps working");
  pass("aura_and_eye_faults_are_independent");}
 {Run r(Variant::Blue);r.run(20,true);require(r.en->live()==1,"live");r.en->engine_tick();r.en->engine_tick();require(r.en->live()==0&&r.en->orphans==1,"actor gone (death/room change): the engine removes the emitter within one tick");
  Run s(Variant::Black);s.run(20,true,true);sk::Retry& rt=s.retries[0];FakeEye& ey=s.eyes;sk::AuraPart<FakeEngine> ap(*s.en,rt);Fx fx{ap,ey};vv::cleanup(*s.e,fx,true);require(s.en->stops==1&&s.e->key==0&&ey.tex==0,"shutdown cleanup: aura stopped, eyes native");s.en->engine_tick();s.en->engine_tick();require(s.en->live()==0,"nothing left");
  pass("actor_gone_and_shutdown_leave_no_emitter");}
 {Run g(Variant::Blue,1);g.gen=2;g.run(3,false);require(g.en->set_calls==0&&g.e->aura_level==0&&g.e->key==0,"a fresh record starts inactive with no handle");
  pass("new_generation_starts_clean");}
 {require(sk::kGeometry.volume_scale==2.4f&&sk::kGeometry.width_scale==0.70f&&sk::kGeometry.length_scale==0.60f&&sk::kGeometry.sweep==1.0f&&sk::kGeometry.min_rad_pm==720&&sk::kGeometry.height_pm==650&&sk::kAlphaCap==255,"the one table (FIX02 body-hugging values)");
  require(sk::kBasePrm.r==255&&sk::kBasePrm.g==255&&sk::kBasePrm.b==255&&sk::kBasePrm.a==255&&sk::kRateStep==0,"FIX02 presence table");
  require(sk::kGeometry.dir_x==0.10f&&sk::kGeometry.dir_y==0.99498744f&&sk::kGeometry.dir_z==0.0f,"FIX01 stroke direction");
  require(sk::kBlueColour.b==255&&sk::kBlueColour.g>sk::kBlueColour.r&&sk::kBlackColour.r==255&&sk::kBlackColour.g==32&&sk::kBlackColour.b==24,"cyan-blue vs intense red");
  pass("shared_table_documented_values");}
 // ---------- geometry / placement / frame / coverage ----------
 {const float R=sk::nominal_radius(24,sk::kGeometry),in=sk::nominal_inner(24,sk::kGeometry);
  require(R>57.5f&&R<57.7f&&in>41.4f&&in<41.5f,"nominal ring 57.6, spawn band from 41.5");
  require(in<sk::kTorsoColliderRadius&&R>sk::kTorsoColliderRadius&&R<=sk::kTorsoColliderRadius*1.16f&&in>=sk::kTorsoColliderRadius*0.82f,"band straddles the torso collider surface (0.83 .. 1.15 x 50): hugging, not orbiting");
  const float R087=24.0f*3.8f,band087=R087*(1.0f-0.64f);require(R<0.65f*R087&&(R-in)<0.5f*band087,"vs 0.8.7: radius -37 %, band thickness -51 %");
  const float w=20.0f*sk::kGeometry.width_scale,l=300.0f*sk::kGeometry.length_scale;require(w>13.9f&&w<14.1f&&l>179.9f&&l<180.1f&&l>w*12.0f,"thin quads 14 x 180 nominal");
  pass("nominal_geometry_hugs_the_torso_surface_not_a_loose_ring");}
 {Run r(Variant::Black);r.px=100;r.py=-50;r.pz=7;r.eye_dy=200;r.run(12,true,true);
  require(r.em().x==100&&r.em().z==7&&r.em().y>-50.0f+129.9f&&r.em().y<-50.0f+130.1f,"ring centre at feet + 0.65 x measured eye offset (200 -> 130), on the actor axis");
  r.px=140;r.py=0;r.pz=-33;r.eye_dy=260;r.tick(true,true);require(r.em().x==140&&r.em().z==-33&&r.em().y>168.9f&&r.em().y<169.1f&&r.en->created==1,"actor moved: the SAME emitter follows on the next tick");
  Run lo(Variant::Blue);lo.eye_dy=-3000;lo.run(12,true);require(lo.em().y>20.0f+38.9f&&lo.em().y<20.0f+39.1f,"glitched eye offset clamped to 60 -> 39");
  Run hi(Variant::Blue);hi.eye_dy=99999;hi.run(12,true);require(hi.em().y>20.0f+324.9f&&hi.em().y<20.0f+325.1f,"absurd eye offset clamped to 500 -> 325");
  pass("ring_centre_on_the_actor_axis_at_chest_height_follows_every_tick_with_sane_clamps");}
 {FakeEngine fe;fe.authored.sweep=0.15f;fe.authored.sz=0.1f;fe.authored.tz=40;fe.authored.rx=90;fe.authored.dx=0;fe.authored.dy=0;fe.authored.dz=1;Run r(Variant::Black,1,&fe);r.run(12,true,true);
  const Em& m=r.em();require(m.f.sweep==1.0f&&m.f.tx==0&&m.f.ty==0&&m.f.tz==0&&m.f.rx==0&&m.f.ry==0&&m.f.rz==0&&m.f.sx==1&&m.f.sy==1&&m.f.sz==1&&m.f.dx==sk::kGeometry.dir_x&&m.f.dy==sk::kGeometry.dir_y&&m.f.dz==sk::kGeometry.dir_z&&m.f.min_rad==0.72f,"a sword-authored frame is replaced by the canonical frame on the emitter");
  fe.ems[r.e->key].f=fe.authored;r.tick(true,true);require(r.em().f.sweep==1.0f&&r.em().f.sz==1.0f&&r.em().f.rx==0,"if anything rewrote the frame, the next tick restores it (applied every tick)");
  bool good=false;for(auto& n:r.notes)if(n.kind==sk::Note::TuningApplied&&n.color.geo_ok&&n.color.sweep==1.0f&&std::string(n.why)=="CANONICAL_FRAME_SHARED_GEOMETRY_AND_PRESENCE_READBACK_MATCHES")good=true;require(good,"TUNING_APPLIED reports the canonical readback");
  pass("canonical_emitter_frame_replaces_the_authored_sword_frame_every_tick");}
 {Frame canon;canon.sweep=1.0f;canon.min_rad=0.72f;const Coverage c=coverage(canon,2.4f,41.0f);
  require(c.sectors==8,"canonical: births hugging the body in all 8 sectors around it");
  require(c.min_r>=41.4f&&c.max_r<=57.7f&&c.max_dy<0.01f,"canonical: every birth between 41.5 and 57.6 from the torso centre, in the horizontal plane through the chest");
  Frame arc;arc.sweep=0.15f;const Coverage ca=coverage(arc,3.8f);require(ca.sectors<=2,"authored narrow sweep: a fixed sector (the clump), whatever the radius");
  Frame squash;squash.sz=0.1f;const Coverage cs=coverage(squash,3.8f);require(cs.sectors<=4&&cs.min_r<20.0f,"authored anisotropic scale: a squashed line, most births inside the body");
  Frame off;off.tz=40;const Coverage co=coverage(off,3.8f);require(co.sectors<8,"authored local translation x scale: ring pushed to one side");
  Frame tilt;tilt.rx=90;const Coverage ct=coverage(tilt,3.8f);require(ct.max_dy>50.0f,"authored rotation: tilted (vertical) ring plane");
  Frame wide;wide.sweep=0.15f;const Coverage cw=coverage(wide,8.0f);require(cw.sectors<=2,"a bigger radius alone does NOT fix a narrow sweep (why 0.8.5 still clumped if this was the cause)");
  pass("coverage_model_canonical_frame_surrounds_the_body_authored_sword_frames_clump");}
 {for(Variant v:{Variant::Blue,Variant::Black}){Run r(v);int prev=0;std::vector<int> al;for(int i=0;i<12;++i){r.tick(true,v==Variant::Black);if(r.e->key)al.push_back(r.em().alpha);}
   require(al.size()==12&&al[0]==25&&al[9]==255&&al[11]==255,"first fade-in sample 25, peak 255 at tick 10, for both variants");for(int a:al){require(a>=prev,"monotonic in");prev=a;}
   int down=255;unsigned n=0;while(r.e->aura_level>0&&n<40){r.tick(false,false);if(r.e->key){require(r.em().alpha<=down,"monotonic out");down=r.em().alpha;}++n;}require(n==15,"15 ticks out");}
  pass("one_alpha_curve_for_both_variants_25_first_sample_255_peak_monotonic");}
 {Run r(Variant::Blue);r.en->geo_mismatch=true;r.run(15,true);bool mism=false;for(auto& n:r.notes)if(n.kind==sk::Note::TuningApplied&&!n.color.geo_ok&&std::string(n.why)=="GEOMETRY_OR_PRESENCE_READBACK_MISMATCH")mism=true;
  require(mism&&r.en->live()==1&&!r.e->aura_fault,"a geometry readback mismatch is REPORTED and does not kill the emitter");
  Run q(Variant::Black);q.run(15,true,true);bool good=false;for(auto& n:q.notes)if(n.kind==sk::Note::TuningApplied&&n.color.geo_ok&&n.applied.y_off>129.9f&&n.applied.y_off<130.1f&&n.applied.radius>57.5f&&n.applied.radius<57.7f&&n.applied.inner>41.4f&&n.applied.inner<41.5f)good=true;require(good,"applied numbers carried to the log: height 130, radius 57.6, inner 41.5");
  pass("tuning_applied_reports_the_real_readback_honestly");}
 {Run bk(Variant::Black);bk.run(100,false,false);require(bk.en->set_calls==0&&bk.en->probe_calls==0&&bk.notes.empty()&&bk.events.empty(),"Black without fury: nothing");
  Run bl(Variant::Blue);bl.run(100,false);require(bl.en->set_calls==0&&bl.en->live()==0&&bl.notes.empty(),"Blue without a group: nothing");
  pass("inactive_states_spawn_and_log_nothing");}
 {Run r(Variant::Blue);r.run(20,true);const auto k=r.e->key;vv::Entry* e=r.e;sk::Retry& rt=r.retries[0];sk::AuraPart<FakeEngine> ap(*r.en,rt);FakeEye ey;Fx fx{ap,ey};vv::cleanup(*e,fx,true);
  r.en->engine_tick();r.en->engine_tick();require(r.en->live()==0&&k!=0,"room change / unload: emitter stopped and gone");
  FakeEngine fresh;Run again(Variant::Blue,1,&fresh);again.gen=2;again.run(12,true);require(fresh.created==1&&fresh.live()==1&&again.em().f.sweep==1.0f,"re-entry: exactly one new emitter, canonical again");
  pass("reentry_after_cleanup_creates_exactly_one_new_emitter");}

 // ---------- FIX01: draw basis against the exact engine rule ----------
 {const D3 old_dir{0,1,0};const Run2 a=engine_frames(old_dir,300,false),b=engine_frames(old_dir,300,true);
  require(a.drawn==0&&b.drawn==0,"0.8.6 frame (rot I, local rot 0, dir (0,1,0)): base // axisY, cross = 0, the engine draws NOTHING in any frame");
  require(!sk::draw_basis(sk::V3{0,1,0},sk::V3{0,1,0}).drawable,"the policy model agrees: degenerate");
  sk::Geometry g086=sk::kGeometry;g086.dir_x=0;g086.dir_y=1;g086.dir_z=0;require(!sk::configured_basis(g086).drawable,"the 0.8.6 geometry fails the drawable check used by FIX01");
  pass("negative_0_8_6_basis_is_degenerate_and_the_engine_would_not_draw");}
 {for(Variant v:{Variant::Blue,Variant::Black}){Run r(v);r.run(12,true,v==Variant::Black);const Em& m=r.em();const D3 dir{m.f.dx,m.f.dy,m.f.dz};   // what configure() actually wrote
   const Run2 kept=engine_frames(dir,300,false),copy=engine_frames(dir,300,true);
   require(kept.drawn==300&&copy.drawn==300,"configured direction: drawn every frame, with and without the base written back");
   require(copy.min_cross>0.0999&&copy.min_cross<0.1001&&kept.min_cross>0.0999,"|cross| >= 0.10 every frame (presentation copies keep base (0,1,0))");
   require(copy.min_cross*copy.min_cross>2000.0*kEps,"margin > 2000 x the engine threshold");
   bool noted=false;for(auto& n:r.notes)if(n.kind==sk::Note::DrawBasisCheck&&n.color.basis.drawable&&std::string(n.why)=="NON_DEGENERATE"&&std::fabs(n.color.basis.cross_sq-0.01f)<1e-5f&&std::fabs(n.color.basis.cross_len-0.10f)<1e-5f)noted=true;
   require(noted,"DRAW_BASIS_CHECK carries the same numbers as the engine port (cross 0.10, cross^2 0.01)");}
  pass("fixed_basis_is_drawn_every_frame_for_blue_and_black_by_the_engine_rule");}
 {Frame tilted;tilted.sweep=1.0f;tilted.min_rad=float(sk::kGeometry.min_rad_pm)/1000.0f;tilted.dx=sk::kGeometry.dir_x;tilted.dy=sk::kGeometry.dir_y;tilted.dz=sk::kGeometry.dir_z;Frame upright=tilted;upright.dx=0;upright.dy=1;
  const Coverage ct=coverage(tilted,sk::kGeometry.volume_scale,41.0f),cu=coverage(upright,sk::kGeometry.volume_scale,41.0f);
  require(ct.sectors==8&&ct.min_r>=41.4f&&ct.max_r<=57.7f&&ct.max_dy<0.01f,"tilted direction: full ring, 8 of 8 sectors, band 41.5-57.6, horizontal plane");
  require(ct.sectors==cu.sectors&&ct.min_r==cu.min_r&&ct.max_r==cu.max_r&&ct.max_dy==cu.max_dy,"birth positions identical to 0.8.6: the direction is not a term of the birth formula (volume offset = rotation x scale only)");
  const D3 drift=dir_mtx_col2(D3{tilted.dx,tilted.dy,tilted.dz});require(std::fabs(drift.x-0.10)<1e-6&&std::fabs(drift.y-0.99498744)<1e-6&&std::fabs(drift.z)<1e-9,"velocity direction (JPAGetDirMtx col 2, spread 0) = the stroke direction: upward drift, 5.7 deg");
  pass("tilted_direction_keeps_the_full_ring_and_birth_positions_unchanged");}
 {const D3 dir{sk::kGeometry.dir_x,sk::kGeometry.dir_y,sk::kGeometry.dir_z};const Run2 r=engine_frames(dir,1,false);
  const double tilt_deg=std::acos(r.last.axis_y.y)*57.29577951;require(tilt_deg>5.6&&tilt_deg<5.8,"quad length axis 5.7 degrees from vertical");
  require(std::fabs(r.last.width.y)<0.11&&std::fabs(r.last.width.x)>0.99,"quad width axis horizontal: a vertical stroke, not a horizontal beam");
  const double w=20.0*sk::kGeometry.width_scale,l=300.0*sk::kGeometry.length_scale;require(l/w>12.0,"still thin: 14 x 180 nominal");
  require(sk::colour_for(Variant::Blue).b==255&&sk::colour_for(Variant::Blue).g>sk::colour_for(Variant::Blue).r&&sk::colour_for(Variant::Black).r>sk::colour_for(Variant::Black).g&&sk::colour_for(Variant::Black).b<sk::colour_for(Variant::Black).g,"Blue blue/cyan, Black violet");
  pass("quad_stays_predominantly_vertical_thin_and_coloured_per_variant");}

 // ---------- FIX02: centre on the body, hugging band, presence ----------
 {Run r(Variant::Black);r.px=100;r.pz=0;r.torso_ok=true;r.c0x=110;r.c0z=0;r.c1x=150;r.c1z=10;r.run(12,true,true);
  require(r.em().x==130.0f&&r.em().z==5.0f,"ring centred on the midpoint of the torso and head collision spheres (hunched body), not on the feet");
  r.c0x=120;r.c1x=160;r.tick(true,true);require(r.em().x==140.0f&&r.en->created==1,"the same emitter follows the body every tick");
  Run far(Variant::Blue);far.px=0;far.pz=0;far.torso_ok=true;far.c0x=200;far.c1x=220;far.run(12,true);require(far.em().x==0.0f&&far.em().z==0.0f,"implausible collider offset (>80): feet fallback");
  Run none(Variant::Blue);none.px=7;none.pz=9;none.torso_ok=false;none.c0x=30;none.c1x=30;none.run(12,true);require(none.em().x==7.0f&&none.em().z==9.0f,"colliders unusable (radius 0): feet fallback");
  bool logged=false;for(auto& n:r.notes)if(n.kind==sk::Note::TuningApplied&&n.applied.torso&&n.applied.cdx==30.0f&&n.applied.cdz==5.0f)logged=true;require(logged,"TUNING_APPLIED reports centre=TORSO_COLLIDERS and the offset from the feet");
  bool fb=false;for(auto& n:far.notes)if(n.kind==sk::Note::TuningApplied&&!n.applied.torso)fb=true;require(fb,"and FEET_FALLBACK when it falls back");
  pass("ring_centre_follows_the_hunched_torso_with_a_feet_fallback");}
 {Frame canon;canon.sweep=1.0f;canon.min_rad=0.72f;Rng rng;unsigned inside=0,outside=0;
  for(int i=0;i<2000;++i){const Birth b=birth(canon,24.0f,2.4f,0,0,0,rng);const float rr=std::sqrt(b.x*b.x+b.z*b.z);if(rr<sk::kTorsoColliderRadius)++inside;else ++outside;}
  require(inside>300&&outside>300,"births on BOTH sides of the 50-unit collider surface: strokes overlap the silhouette instead of orbiting it");
  Frame old;old.sweep=1.0f;old.min_rad=0.64f;Rng r2;float nearest=1e9f;for(int i=0;i<2000;++i){const Birth b=birth(old,24.0f,3.8f,0,0,0,r2);const float rr=std::sqrt(b.x*b.x+b.z*b.z);if(rr<nearest)nearest=rr;}
  require(nearest>58.0f,"0.8.7 never placed a stroke closer than 58.4: the loose ring the user saw");
  pass("hugging_band_straddles_the_body_surface_unlike_the_0_8_7_ring");}
 {for(Variant v:{Variant::Blue,Variant::Black}){Run r(v);r.run(12,true,v==Variant::Black);const Em& m=r.em();
   require(m.alpha==255&&m.base.r==255&&m.base.g==255&&m.base.b==255&&m.base.a==255&&m.rate_step==0,"presence on the emitter: global alpha 255, base colour opaque white (resource 255,255,195,150), rate step 0");
   bool ok=false;for(auto& n:r.notes)if(n.kind==sk::Note::TuningApplied&&n.color.presence_ok&&n.color.base_prm.a==255&&n.color.rate_step==0)ok=true;require(ok,"presence read back and reported");}
  Run q(Variant::Blue);q.en->presence_mismatch=true;q.run(12,true);bool mism=false;for(auto& n:q.notes)if(n.kind==sk::Note::TuningApplied&&std::string(n.why)=="GEOMETRY_OR_PRESENCE_READBACK_MISMATCH")mism=true;require(mism&&q.en->live()==1,"a presence mismatch is reported, the emitter keeps working");
  require(sk::kBlueColour.r>=70&&sk::kBlueColour.g>=170&&sk::kBlackColour.r==255,"Blue not darker than 0.8.7; Black red channel at its maximum");
  const double opacity087=(150.0/255.0)*(234.0/255.0),opacity088=1.0*1.0;require(opacity088/opacity087>1.8,"nominal opacity factor x1.85 vs 0.8.7 (before the resource's own texture and alpha animation)");
  pass("presence_raised_on_the_emitter_only_and_reported_honestly");}
 std::printf("STROKES_LAB_PASS cases=%u POLICY_AND_GEOMETRY_MODEL_ONLY; STROKES_LOOK_NOT_VERIFIED; ENGINE_AND_PIXELS_NOT_EXECUTED\n",cases);
}
