#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/stage.h>
#include <d/d_stage.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
extern "C" ModResult mod_initialize(ModError*);
extern "C" ModResult mod_update(ModError*);
extern "C" ModResult mod_shutdown(ModError*);
namespace {
unsigned calls=0,logs=0;bool payload_ok=false,forbidden=false;const char* scenario="success";
void log_write(ModContext*,LogLevel,const char* s){++logs;if(!std::strstr(s,"runtime_spawn=NOT_VERIFIED"))forbidden=true;}
ModResult add(ModContext* ctx,const char* stage,uint8_t room,int8_t layer,const void* record,size_t size,StageActorHandle* handle){
 ++calls;
 if(!std::strcmp(scenario,"throw"))throw std::runtime_error("own-fake");
 const auto& a=*static_cast<const stage_actor_data_class*>(record);const auto& pos=a.base.position;const auto& angle=a.base.angle;
 const unsigned char* raw=static_cast<const unsigned char*>(record);
 payload_ok=ctx==mod_ctx&&!std::strcmp(stage,"F_SP121")&&room==3&&layer==6&&size==32&&
  std::memcmp(a.name,"E_oc\0\0\0\0",8)==0&&u32(a.base.parameters)==0xFFFF0000u&&
  pos.x==-1250.f&&pos.y==-6304.0083f&&pos.z==60750.f&&angle.x==6&&angle.y==0&&angle.z==0&&u16(a.base.setID)==0xFFFF&&
  raw[8]==0xFF&&raw[9]==0xFF&&raw[10]==0&&raw[11]==0&&raw[24]==0&&raw[25]==6&&raw[30]==0xFF&&raw[31]==0xFF;
 if(!std::strcmp(scenario,"reentry")&&mod_initialize(nullptr)!=MOD_CONFLICT)forbidden=true;
 if(!std::strcmp(scenario,"reject")){*handle=0;return MOD_INVALID_ARGUMENT;}
 *handle=!std::strcmp(scenario,"zero-handle")?0:123;return MOD_OK;
}
ModResult forbidden_edit(ModContext*,StageActorHandle){forbidden=true;return MOD_ERROR;}
}
// Test-only resolution for an emitted, unreferenced SDK inline destructor.
// If reached at ANY time (including CRT teardown), fail immediately. No GPU stub in production.
extern "C" void GXDestroyTexObj(GXTexObj*) {
 std::fputs("FAIL unexpected GXDestroyTexObj in isolated StageService test\n",stderr);std::abort();
}
int main(int argc,char** argv){
 if(argc!=2)return 2;scenario=argv[1];HostService host{};host.header=SERVICE_HEADER(HostService,2,2);host.version="2.0.3";
 LogService log{};log.header=SERVICE_HEADER(LogService,1,0);log.write=log_write;
 StageService stage{};stage.header=SERVICE_HEADER(StageService,1,0);stage.add_actor=add;stage.remove_actor_edit=forbidden_edit;
 svc_host=&host;svc_log=&log;svc_stage=&stage;mod_ctx=reinterpret_cast<ModContext*>(1);
 const bool incompatible=!std::strcmp(scenario,"missing")||!std::strcmp(scenario,"wrong-host")||!std::strcmp(scenario,"small-stage")||!std::strcmp(scenario,"null-add")||!std::strcmp(scenario,"wrong-stage-major")||!std::strcmp(scenario,"null-context");
 if(!std::strcmp(scenario,"missing"))svc_stage=nullptr;
 if(!std::strcmp(scenario,"wrong-host"))host.version="1.3.1";
 if(!std::strcmp(scenario,"small-stage"))stage.header.struct_size=sizeof(ServiceHeader);
 if(!std::strcmp(scenario,"null-add"))stage.add_actor=nullptr;
 if(!std::strcmp(scenario,"wrong-stage-major"))stage.header.major_version=2;
 if(!std::strcmp(scenario,"null-context"))mod_ctx=nullptr;
 const ModResult wanted=incompatible?MOD_UNSUPPORTED:!std::strcmp(scenario,"reject")?MOD_INVALID_ARGUMENT:(!std::strcmp(scenario,"zero-handle")||!std::strcmp(scenario,"throw"))?MOD_ERROR:MOD_OK;
 const auto result=mod_initialize(nullptr);bool ok=result==wanted&&calls==(incompatible?0u:1u)&&(!calls||!std::strcmp(scenario,"throw")||payload_ok);
 for(unsigned i=0;i<1000;++i)ok&=mod_update(nullptr)==MOD_OK;
 ok&=mod_initialize(nullptr)==MOD_CONFLICT&&calls==(incompatible?0u:1u);
 mod_shutdown(nullptr);const auto before=logs;mod_shutdown(nullptr);ok&=logs==before&&!forbidden&&calls==(incompatible?0u:1u);
 std::printf("%s StageService_fake=%s calls=%u no_update_retry=%s payload32_be_checked=%s game=NOT_EXECUTED\n",ok?"PASS":"FAIL",scenario,calls,ok?"PASS":"FAIL",payload_ok?"PASS":"NO_REGISTRATION_OR_THROW");return ok?0:1;
}
