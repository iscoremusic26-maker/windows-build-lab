#include "../src/room_owner_resolver.hpp"
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/game.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
namespace { unsigned checks=0,reads=0,writes=0;bool nested=false; }
namespace devf::a2 {
Owner capture_owner(bool host,bool update) { ++reads; if(!host || !update) std::abort(); return {}; }
}
void check(bool b,const char* name) { if(!b){std::cerr<<"FAIL "<<name<<'\n';std::exit(1);}++checks;std::cout<<"PASS "<<name<<'\n'; }
void write(ModContext*,LogLevel,const char* text) {
    ++writes; check(std::strstr(text,"DISARMED_NO_REGISTRATION") && std::strstr(text,"requests=0"),"permanent_disarm_log");
    if(nested) {ModError e=MOD_ERROR_INIT;mod_update(&e);}
}
int main() {
    ModError e=MOD_ERROR_INIT;
    HostService h{};h.header=SERVICE_HEADER(HostService,2,2);h.version="2.0.3";
    LogService l{};l.header=SERVICE_HEADER(LogService,1,0);l.write=write;
    GameService g{};g.header=SERVICE_HEADER(GameService,2,0);
    svc_host=&h;svc_log=&l;svc_game=&g;
    check(mod_update(&e)==MOD_OK && reads==0,"not_initialized_no_read");
    svc_host=nullptr;check(mod_initialize(&e)==MOD_UNSUPPORTED,"missing_host");svc_host=&h;
    h.version="4.0.3";check(mod_initialize(&e)==MOD_UNSUPPORTED,"wrong_host_version");h.version="2.0.3";
    g.header.major_version=1;check(mod_initialize(&e)==MOD_UNSUPPORTED,"wrong_epoch");g.header.major_version=2;
    l.write=nullptr;check(mod_initialize(&e)==MOD_UNSUPPORTED,"missing_log");l.write=write;
    h.header.struct_size=sizeof(ServiceHeader);check(mod_initialize(&e)==MOD_UNSUPPORTED,"short_host");h.header.struct_size=sizeof(h);
    check(mod_initialize(&e)==MOD_OK,"initialize_disarmed");
    check(mod_initialize(&e)==MOD_CONFLICT,"double_initialize");
    nested=true;mod_update(&e);check(reads==1 && writes==1,"reentrant_update_no_read");nested=false;
    for(unsigned i=0;i<3000;++i)mod_update(&e);
    check(reads==16 && writes==16,"bounded_diagnostic_reads_only");
    mod_shutdown(&e);auto before=reads;mod_update(&e);check(reads==before,"shutdown_no_read");
    check(mod_initialize(&e)==MOD_OK,"reactivation_stays_disarmed");mod_update(&e);check(reads==before,"reactivation_no_rearm");
    std::cout<<"SUMMARY "<<checks<<" PASS 0 FAIL; actual disarmed lifecycle with fake resolver; no engine\n";
}
