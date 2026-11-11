#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "../src/registration.hpp"
using namespace native_qa;
namespace {
int context_value;auto* ctx=reinterpret_cast<ModContext*>(&context_value);
struct Fake {
 unsigned adds=0,removes=0,fail_at=0,throw_at=0,zero_at=0,duplicate_at=0,error_handle_at=0,throw_handle_at=0;
 bool remove_fail=false,remove_throw=false;Record records[2]{};StageActorHandle live[2]{},removed[8]{};
} fake;
void check(bool c){if(!c)throw std::runtime_error("LAB_ASSERT");}
ModResult add(ModContext* c,const char* s,uint8_t r,int8_t l,const void* bytes,size_t n,StageActorHandle* out){
 check(c==ctx&&std::strcmp(s,"F_SP121")==0&&r==3&&l==6&&n==32&&bytes&&out);
 const auto k=++fake.adds;check(k<=2);std::memcpy(fake.records[k-1].data(),bytes,32);
 if(fake.throw_at==k)throw std::runtime_error("FAKE_ADD");
 if(fake.fail_at==k)return MOD_ERROR;
 *out=fake.zero_at==k?0:fake.duplicate_at==k?101:100+k;
 if(*out)fake.live[k-1]=*out;
 if(fake.throw_handle_at==k)throw std::runtime_error("FAKE_THROW_HANDLE");
 if(fake.error_handle_at==k)return MOD_ERROR;
 return MOD_OK;
}
ModResult remove(ModContext* c,StageActorHandle h){
 check(c==ctx&&h>=101&&h<=102);fake.removed[fake.removes++]=h;
 if(fake.remove_throw)throw std::runtime_error("FAKE_REMOVE");
 if(fake.remove_fail)return MOD_ERROR;
 for(auto& a:fake.live)if(a==h)a=0;return MOD_OK;
}
StageService service{{sizeof(StageService),1,0},nullptr,nullptr,add,remove};
unsigned cases=0;
template<class F>void test(const char* name,F f){fake={};service={{sizeof(StageService),1,0},nullptr,nullptr,add,remove};f();++cases;std::printf("CASE_PASS %s\n",name);}
}
int run_registration_cases(){
 try {
 test("A_one_north",[]{Registration r;check(r.initialize(ctx,&service,std::array<bool,2>{false,true})==MOD_OK&&r.expected==1&&fake.adds==1&&fake.records[0]==record(true));check(r.cleanup(ctx,&service)==MOD_OK&&fake.removes==1&&!fake.live[0]);});
 test("B_two_distinct",[]{Registration r;check(r.initialize(ctx,&service,std::array<bool,2>{true,true})==MOD_OK&&fake.adds==2&&r.handles[0]!=r.handles[1]&&fake.records[0]==record(false)&&fake.records[1]==record(true)&&fake.records[0]!=fake.records[1]);r.cleanup(ctx,&service);check(fake.removes==2&&fake.removed[0]==102&&fake.removed[1]==101);});
 test("record_native_contract",[]{auto b=record(true);check(!std::memcmp(b.data(),"E_oc\0\0\0\0",8)&&b[8]==255&&b[9]==255&&b[10]==1&&b[11]==0&&b[24]==1&&b[25]==6&&b[30]==255&&b[31]==255);});
 test("A_no_reinjection",[]{Registration r;check(r.initialize(ctx,&service,std::array<bool,2>{false,true})==MOD_OK);check(r.initialize(ctx,&service,std::array<bool,2>{false,true})==MOD_CONFLICT&&fake.adds==1);r.cleanup(ctx,&service);});
 test("B_no_reinjection",[]{Registration r;check(r.initialize(ctx,&service,std::array<bool,2>{true,true})==MOD_OK);check(r.initialize(ctx,&service,std::array<bool,2>{true,true})==MOD_CONFLICT&&fake.adds==2);r.cleanup(ctx,&service);});
 test("fail_first",[]{Registration r;fake.fail_at=1;check(r.initialize(ctx,&service,std::array<bool,2>{true,true})!=MOD_OK&&fake.adds==1&&fake.removes==0&&!r.ready);});
 test("fail_second_rollback_first",[]{Registration r;fake.fail_at=2;check(r.initialize(ctx,&service,std::array<bool,2>{true,true})!=MOD_OK&&fake.adds==2&&fake.removes==1&&fake.removed[0]==101&&!fake.live[0]);});
 test("throw_first",[]{Registration r;fake.throw_at=1;check(r.initialize(ctx,&service,std::array<bool,2>{false,true})==MOD_ERROR&&!r.ready);});
 test("throw_second_rollback",[]{Registration r;fake.throw_at=2;check(r.initialize(ctx,&service,std::array<bool,2>{true,true})==MOD_ERROR&&fake.removes==1&&!fake.live[0]);});
 test("zero_first",[]{Registration r;fake.zero_at=1;check(r.initialize(ctx,&service,std::array<bool,2>{false,true})==MOD_ERROR&&fake.removes==0);});
 test("zero_second",[]{Registration r;fake.zero_at=2;check(r.initialize(ctx,&service,std::array<bool,2>{true,true})==MOD_ERROR&&fake.removes==1);});
 test("duplicate_handle",[]{Registration r;fake.duplicate_at=2;check(r.initialize(ctx,&service,std::array<bool,2>{true,true})==MOD_ERROR&&fake.removes==1);});
 test("error_with_returned_handle",[]{Registration r;fake.error_handle_at=2;check(r.initialize(ctx,&service,std::array<bool,2>{true,true})==MOD_ERROR&&fake.removes==2);});
 test("throw_after_handle_rollback",[]{Registration r;fake.throw_handle_at=2;check(r.initialize(ctx,&service,std::array<bool,2>{true,true})==MOD_ERROR&&fake.removes==2&&!fake.live[0]&&!fake.live[1]);});
 test("remove_fail_retry_own_only",[]{Registration r;r.initialize(ctx,&service,std::array<bool,2>{true,true});fake.remove_fail=true;check(r.cleanup(ctx,&service)!=MOD_OK&&r.handles[0]==101&&r.handles[1]==102);fake.remove_fail=false;check(r.cleanup(ctx,&service)==MOD_OK&&!r.handles[0]&&!r.handles[1]);});
 test("remove_exception",[]{Registration r;r.initialize(ctx,&service,std::array<bool,2>{false,true});fake.remove_throw=true;check(r.cleanup(ctx,&service)==MOD_ERROR&&r.handles[0]==101);fake.remove_throw=false;r.cleanup(ctx,&service);});
 test("cleanup_idempotent",[]{Registration r;r.initialize(ctx,&service,std::array<bool,2>{false,true});r.cleanup(ctx,&service);r.cleanup(ctx,&service);check(fake.removes==1);});
 test("cleanup_without_init",[]{Registration r;check(r.cleanup(ctx,&service)==MOD_OK&&fake.removes==0);});
 test("null_context",[]{Registration r;check(r.initialize(nullptr,&service,std::array<bool,2>{false,true})==MOD_UNSUPPORTED&&fake.adds==0);});
 test("null_service",[]{Registration r;check(r.initialize(ctx,nullptr,std::array<bool,2>{false,true})==MOD_UNSUPPORTED&&fake.adds==0);});
 test("short_service",[]{Registration r;service.header.struct_size=8;check(r.initialize(ctx,&service,std::array<bool,2>{false,true})==MOD_UNSUPPORTED&&fake.adds==0);});
 test("wrong_version",[]{Registration r;service.header.major_version=2;check(r.initialize(ctx,&service,std::array<bool,2>{false,true})==MOD_UNSUPPORTED&&fake.adds==0);});
 test("null_add",[]{Registration r;service.add_actor=nullptr;check(r.initialize(ctx,&service,std::array<bool,2>{false,true})==MOD_UNSUPPORTED&&fake.adds==0);});
 test("null_remove",[]{Registration r;service.remove_actor_edit=nullptr;check(r.initialize(ctx,&service,std::array<bool,2>{false,true})==MOD_UNSUPPORTED&&fake.adds==0);});
 for(bool north:{false,true}){const auto b=record(north);std::printf("RECORD_%s=",north?"NORTH":"SITE1");for(auto v:b)std::printf("%02x",v);std::printf("\n");}
 std::printf("NATIVE_QA_LAB_PASS cases=%u\n",cases);return 0;
 }catch(...){std::printf("NATIVE_QA_LAB_FAIL\n");return 1;}
}
