#include "../src/capture.hpp"
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/game.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
extern unsigned reads;extern bool fail_capture;
unsigned checks=0,writes=0;bool nested=false,fail_log=false;
void check(bool v,const char* n){if(!v){std::cerr<<"FAIL "<<n<<'\n';std::exit(1);}++checks;std::cout<<"PASS "<<n<<'\n';}
void write(ModContext*,LogLevel,const char* text){++writes;check(std::strstr(text,"DISARMED_NO_REGISTRATION") && std::strstr(text,"no_location_admission=1"),"scalar_disarmed_line");if(nested){ModError e=MOD_ERROR_INIT;mod_update(&e);}if(fail_log)throw std::runtime_error("fake log error");}
int main(int argc,char** argv){
 ModError e=MOD_ERROR_INIT;HostService h{};h.header=SERVICE_HEADER(HostService,2,2);h.version="2.0.3";
 LogService l{};l.header=SERVICE_HEADER(LogService,1,0);l.write=write;
 GameService g{};g.header=SERVICE_HEADER(GameService,2,0);svc_host=&h;svc_log=&l;svc_game=&g;
 if(argc>1){check(mod_initialize(&e)==MOD_OK,"error_case_initialize");fail_capture=!std::strcmp(argv[1],"capture_error");fail_log=!fail_capture;
 check(mod_update(&e)==MOD_ERROR,"exception_reports_error");for(unsigned i=0;i<4000;++i)mod_update(&e);
 check(reads==1,"exception_stops_further_native_reads");check(mod_initialize(&e)==MOD_OK,"reinitialize_disarmed_error");mod_update(&e);check(reads==1,"exception_budget_cannot_reset");
 }else{
 check(mod_update(&e)==MOD_OK && reads==0,"inactive_no_reads");svc_host=nullptr;check(mod_initialize(&e)==MOD_UNSUPPORTED,"missing_host");svc_host=&h;
 h.version="4.0.3";check(mod_initialize(&e)==MOD_UNSUPPORTED,"wrong_version");h.version="2.0.3";
 h.header.struct_size=sizeof(ServiceHeader);check(mod_initialize(&e)==MOD_UNSUPPORTED,"short_host");h.header.struct_size=sizeof(h);
 g.header.major_version=1;check(mod_initialize(&e)==MOD_UNSUPPORTED,"wrong_game_epoch");g.header.major_version=2;
 l.write=nullptr;check(mod_initialize(&e)==MOD_UNSUPPORTED,"missing_log_callback");l.write=write;
 check(mod_initialize(&e)==MOD_OK,"initialize");check(mod_initialize(&e)==MOD_CONFLICT,"double_initialize");
 nested=true;mod_update(&e);check(reads==1 && writes==1,"no_reentrant_capture");nested=false;
 for(unsigned i=0;i<119;++i)mod_update(&e);check(reads==1,"cadence_120");mod_update(&e);check(reads==2,"next_cadence");
 for(unsigned i=0;i<4000;++i)mod_update(&e);check(reads==16 && writes==16,"bounded_real_lifecycle_fake_capture");
 mod_shutdown(&e);mod_update(&e);check(reads==16,"shutdown_no_reads");check(mod_initialize(&e)==MOD_OK,"reactivation");mod_update(&e);check(reads==16,"reactivation_preserves_budget");
 }
 std::cout<<"SUMMARY "<<checks<<" PASS 0 FAIL; real observer lifecycle with fake capture; no engine\n";
}
