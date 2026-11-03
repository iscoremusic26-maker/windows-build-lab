#include "ROOT/src/balance_adapter.hpp"
#include "ROOT/src/break_core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <algorithm>
using namespace post03;
struct daE_OC_c{int health=500,mode=3,state=0;};
struct Cur{std::uintptr_t actor=0;bool tracked=true,from_link=false,helm=false;int slot=-1,pre_hp=500,pre_mode=3;std::uint64_t generation=0;std::uint32_t id=0;postbal01::Snapshot balance_profile;};
struct FakeRx{bool disabled=false;bool session_disabled()const{return disabled;}void close_session(){disabled=true;}};
struct FakeAi{void intervention(int,std::uint32_t,std::uint64_t,std::uint64_t,bool,bool){}int expire_one(std::uint64_t){return 0;}};
struct Fx{FakeAi ai;}fx;
struct Global{post04::PostureEngine pe;post03::Engine& eng=pe.eng;post06::Breaker br;Cur cur;int layout=1;FakeRx rx;struct{std::uint64_t tick=0;}p8;post08::AiWatch watch;}g;
struct{bool sim_active=true;}lg;
static bool sim_allowed=true;static std::uint64_t event=0;static unsigned stuns=0,bonuses=0;static unsigned checks=0;
enum{LOG_LEVEL_INFO,LOG_LEVEL_WARN};
void ck(bool b,const char* s){++checks;if(!b){std::fprintf(stderr,"CP0203_ADAPTER_FAIL %s\n",s);std::exit(1);}}
static bool same_live=true;static postbal01::Snapshot profiles[64];static lnk01::VariantTrack variants;
post05::LiveMode read_live(daE_OC_c* a){post05::LiveMode t;t.mode=a->mode;t.state=a->state;return t;}
bool post06_bonus(daE_OC_c* a,const post06::View&,const post06::Action& x){++bonuses;a->health=x.new_hp;return true;}
bool post06_stun(daE_OC_c*,bool){++stuns;return true;}
bool post06_recover(daE_OC_c*){return true;}
void post08_watch_out(const post08::AiWatch::Out&){}void fix01_cleanup_request(){}void lnk_write(int,const char*){}bool postbal_log_take(){return false;}void emit(LogCat,int,const char*){}
void probe_variant_ai_env(){}void post08_update(){++g.p8.tick;}void post08_observe_link(){}void lnk01_update(){lg.sim_active=sim_allowed;}
const char* dComIfGp_getStartStageName(){return "TEST";}Lookup lookup_actor(const Pick&){return Lookup::PresentMatch;}void emit_events(const Events&){}void drain_posture(){}

void cp04_maintenance(){} // CP04 adapter tested independently; old CP02/03 fixture retains only its original scope.
