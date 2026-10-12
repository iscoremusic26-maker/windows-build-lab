// Own test driver. Actual layouts/queue APIs come from the exact host checkout.
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include "f_pc/f_pc_stdcreate_req.h"
#include "f_pc/f_pc_layer.h"
#include "f_pc/f_pc_deletor.h"
#include "SSystem/SComponent/c_phase.h"
#include "SSystem/SComponent/c_malloc.h"
static int ownerA,ownerB;static bool processDeleteOK=true;static unsigned freed=0;
namespace dusk::mods::svc::actor_impl { void* profile_owner(s16 n){return n==30000?&ownerB:nullptr;} }
struct standard_create_request_class;
int fpcSCtRq_Handler(standard_create_request_class*){return 0;} // Identity only; phase engine not invoked.
void fpcLy_CreatedMesg(layer_class*){} // Explicit layer bookkeeping adapter.
void fpcLy_CreatingMesg(layer_class*){}
void fpcLy_CancelQTo(process_method_tag_class*){}
int fpcLy_ToCancelQ(layer_class*,process_method_tag_class*){return 1;}
int fpcDt_Delete(void*){return processDeleteOK?TRUE:FALSE;} // Explicit process-delete adapter.
void cMl::free(void* p){++freed;std::free(p);} // Explicit heap adapter, actually frees allocated requests.
static int deleteCalls=0;static bool failDelete=false;static delete_tag_class* removeDuringCallback=nullptr;
int fpcDt_deleteMethod(base_process_class*){++deleteCalls;if(removeDuringCallback){fpcDtTg_DeleteQTo(removeDuringCallback);removeDuringCallback=nullptr;}return failDelete?0:1;} // Controlled destructor edge; no heap/process destruction.
#include "real_functions.inc"
static int good(void*){return 1;}static int bad(void*){return 0;}
static create_request_method_class standardMethods{reinterpret_cast<fpcCtRq_phase_handler>(fpcSCtRq_Handler),good,good};
static unsigned checks=0;
static void check(bool x,const char* name){if(!x){std::printf("FAIL %s\n",name);std::exit(1);}++checks;std::printf("PASS %s\n",name);}
static standard_create_request_class* make(void* owner,s16 profile=100){
 auto* r=static_cast<standard_create_request_class*>(std::calloc(1,sizeof(standard_create_request_class)));if(!r)std::abort();
 r->base.methods=&standardMethods;r->mod_owner=owner;r->process_name=profile;fpcCtTg_Init(&r->base.base,r);fpcCtRq_ToCreateQ(&r->base);return r;
}
int main(){
 static_assert(sizeof(void*)==8);static_assert(offsetof(standard_create_request_class,base)==0);static_assert(offsetof(create_request,base)==0);static_assert(offsetof(create_tag,base)==0);static_assert(offsetof(create_tag_class,mpNode)==0);
 std::printf("LAYOUT pointer=%zu create_request=%zu standard_request=%zu owner_offset=%zu tag_data_offset=%zu\n",sizeof(void*),sizeof(create_request),sizeof(standard_create_request_class),offsetof(standard_create_request_class,mod_owner),offsetof(create_tag_class,mpTagData));
 check(!fpcSCtRq_HasPendingForOwner(&ownerA),"empty real queue");
 auto* a=make(&ownerA);auto* b=make(&ownerB);check(g_fpcCtTg_Queue.mSize==2,"real tag insertion two requests");check(fpcSCtRq_HasPendingForOwner(&ownerA),"submission owner before first callback");
 check(fpcCtRq_Cancel(&a->base)==TRUE&&freed==1&&g_fpcCtTg_Queue.mSize==1,"actual cancel removes/frees first node");check(!fpcSCtRq_HasPendingForOwner(&ownerA)&&fpcSCtRq_HasPendingForOwner(&ownerB),"scanner follows retained second node");
 check(fpcCtRq_Cancel(&b->base)==TRUE&&g_fpcCtTg_Queue.mSize==0,"actual cancel removes/frees final node");
 a=make(&ownerA,30000);check(fpcSCtRq_HasPendingForOwner(&ownerB),"profile owner distinct from submitting owner");fpcCtRq_Cancel(&a->base);
 a=make(&ownerA);create_request_method_class opaque{nullptr,good,good};a->base.methods=&opaque;check(fpcSCtRq_HasPendingForOwner(&ownerB),"opaque real queue entry conservatively blocks");fpcCtRq_Cancel(&a->base);
 a=make(&ownerA);base_process_class proc{};proc.create_req=&a->base;a->base.process=&proc;processDeleteOK=false;
 check(fpcCtRq_Cancel(&a->base)==FALSE&&a->base.is_cancel&&g_fpcCtTg_Queue.mSize==1,"process cancel failure retains request and sticky flag");
 check(fpcCtRq_Cancel(&a->base)==TRUE&&fpcSCtRq_HasPendingForOwner(&ownerA),"sticky cancel success does not prove actual queue drained");processDeleteOK=true;fpcCtRq_Delete(&a->base);check(proc.create_req==nullptr&&g_fpcCtTg_Queue.mSize==0,"delete clears real process request backlink");
 a=make(&ownerA);create_request_method_class cancelFail{reinterpret_cast<fpcCtRq_phase_handler>(fpcSCtRq_Handler),bad,good};a->base.methods=&cancelFail;
 check(fpcCtRq_Cancel(&a->base)==FALSE&&fpcSCtRq_HasPendingForOwner(&ownerA),"cancel callback failure retains scanned request");fpcCtRq_Delete(&a->base);
 a=make(&ownerA);create_request_method_class deleteFail{reinterpret_cast<fpcCtRq_phase_handler>(fpcSCtRq_Handler),good,bad};a->base.methods=&deleteFail;unsigned before=freed;
 check(fpcCtRq_Delete(&a->base)==FALSE&&freed==before&&g_fpcCtTg_Queue.mSize==0,"LIMIT original delete callback failure removes queue before failure");
 // This behavior is outside the standard request contract: scanner alone cannot prove generic resources.
 a->base.methods=&standardMethods;cMl::free(a);
 check(g_fpcCtTg_Queue.mpHead==nullptr&&g_fpcCtTg_Queue.mpTail==nullptr,"real queue head/tail empty after cleanup");
 base_process_class first{},second{};first.name=second.name=30000;
 fpcDtTg_Init(&first.delete_tag,&first);fpcDtTg_Init(&second.delete_tag,&second);
 fpcDtTg_ToDeleteQ(&first.delete_tag);failDelete=true;deleteCalls=0;
 check(!finish_delete_all_actors_of_name(30000)&&g_fpcDtTg_Queue.mSize==1&&deleteCalls==1,"real delete queue requeues callback failure");
 failDelete=false;check(finish_delete_all_actors_of_name(30000)&&g_fpcDtTg_Queue.mSize==0,"real delete queue drains after callback success");
 fpcDtTg_Init(&first.delete_tag,&first);fpcDtTg_Init(&second.delete_tag,&second);fpcDtTg_ToDeleteQ(&first.delete_tag);fpcDtTg_ToDeleteQ(&second.delete_tag);deleteCalls=0;removeDuringCallback=&second.delete_tag;
 check(finish_delete_all_actors_of_name(30000)&&deleteCalls==2,"BLOCKED detached next node revisited by exact traversal");
 // Objects remain alive deliberately; real fpcBs_Delete can free them. This demonstrates
 // the stale traversal without intentionally executing undefined use-after-free.
 std::printf("SAFETY_BLOCKED reentrant_next_node_revisit=observed objects_not_freed=yes runtime=NOT_TESTED\n");
 std::printf("INTEGRATION_TEST_PASS checks=%u game_runtime=NOT_TESTED adapters=profile_owner,phase,layer,heap,process_delete\n",checks);return 0;
}
