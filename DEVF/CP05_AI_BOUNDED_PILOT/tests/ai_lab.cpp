#include "../src/ai_core.hpp"
#include <stdexcept>
#include <cstdio>
#include <limits>
using namespace devf_ai;
void need(bool ok){if(!ok)throw std::runtime_error("assertion");}
struct Shared {int model=7,texture=9,hio=5000;};
struct Actor {float range=kNative;Shared* shared;};
View good(){View v;v.id=147;v.generation=1;v.epoch=1;v.key=0x123;v.identity=v.context=v.eoc2=v.created=true;v.action=0;v.health=150;v.range=kNative;v.move=600;v.player2=1000.f*1000;v.home2=0;return v;}
Pilot fresh(){Pilot p;p.admit(147,1);return p;}
bool apply(Pilot& p,View v,Actor& a){p.push();v.range=a.range;float x=0;if(!p.begin(v,x))return false;a.range=x;return true;}
void restore(Pilot& p,View v,Actor& a){v.range=a.range;float x=0;auto e=p.end(v,x);if(e==Pilot::End::Restore)a.range=x;}
int main(){
 unsigned n=0;const char* name="";
 auto test=[&](const char* label,auto fn){name=label;fn();++n;std::printf("PASS %s\n",label);};
 try{
 test("native_same_resources_unchanged",[]{Shared s;Actor dev{kNative,&s},retail{kNative,&s};auto p=fresh();auto v=good();need(apply(p,v,dev));need(dev.range==1200&&retail.range==5000);v.action=3;restore(p,v,dev);need(dev.range==5000&&retail.range==5000&&s.hio==5000&&s.model==7&&s.texture==9);need(p.phase==Phase::Interest);});
 test("retail_id_rejected",[]{auto p=fresh();auto v=good();v.id=9;p.push();float x;need(!p.begin(v,x));need(p.end(v,x)==Pilot::End::Ignored);});
 test("wrong_record",[]{auto p=fresh();auto v=good();v.identity=false;p.push();float x;need(!p.begin(v,x));});
 test("normal_EOC",[]{auto p=fresh();auto v=good();v.eoc2=false;p.push();float x;need(!p.begin(v,x));});
 test("stage_room_layer_epoch",[]{for(int i=0;i<3;++i){auto p=fresh();auto v=good();if(i==0)v.context=false;if(i==1)v.epoch=2;if(i==2)v.generation=2;p.push();float x;need(!p.begin(v,x));}});
 test("create_not_ready",[]{auto p=fresh();auto v=good();v.created=false;p.push();float x;need(!p.begin(v,x));});
 test("death_priority",[]{auto p=fresh();auto v=good();v.health=0;p.push();float x;need(!p.begin(v,x));});
 test("attack_damage_stun_fall_demo_priority",[]{for(int a:{4,5,6,9,10,11,12,13,14,15,16}){auto p=fresh();auto v=good();v.action=a;p.push();float x;need(!p.begin(v,x));}});
 test("hit_and_cinematic_priority",[]{for(int i=0;i<2;++i){auto p=fresh();auto v=good();v.hit=i==0;v.event=i==1;p.push();float x;need(!p.begin(v,x));}});
 test("strict_native_range_move_contract",[]{for(int i=0;i<2;++i){auto p=fresh();auto v=good();if(i==0)v.range=1200;else v.move=0;p.push();float x;need(!p.begin(v,x)&&p.fault);}});
 test("enter_1200_vs_native_5000",[]{need(1300.f<kNative&&!(1300.f<kEnter));auto p=fresh();auto v=good();v.player2=kEnter*kEnter;p.push();float x;need(p.begin(v,x)&&x==1200);v.action=3;v.range=x;need(p.end(v,x)==Pilot::End::Restore&&p.phase==Phase::Guard);});
 test("interest_retains_1400",[]{auto p=fresh();p.phase=Phase::Interest;auto v=good();v.action=3;v.player2=1300.f*1300;p.push();float x;need(p.begin(v,x)&&x==1400&&p.outside==0);});
 test("exit_30_updates_saturating",[]{auto p=fresh();p.phase=Phase::Interest;auto v=good();v.action=3;v.home2=700.f*700;v.player2=kExit*kExit;for(unsigned i=1;i<=35;++i){p.push();float x;need(p.begin(v,x));need(x==(i<30?5000:1200));v.range=x;need(p.end(v,x)==Pilot::End::Restore);v.range=5000;}need(p.phase==Phase::ReturnNative&&p.outside==30);});
 test("grace_reset_reentry",[]{auto p=fresh();p.phase=Phase::Interest;p.outside=29;auto v=good();v.action=3;p.push();float x;need(p.begin(v,x)&&p.outside==0&&x==1400);});
 test("return_home_native_only",[]{auto p=fresh();p.phase=Phase::ReturnNative;auto v=good();v.home2=500.f*500;v.player2=2000.f*2000;p.push();float x;need(p.begin(v,x)&&p.phase==Phase::ReturnNative);v.range=x;p.end(v,x);v.range=5000;v.home2=499.f*499;p.push();need(p.begin(v,x)&&p.phase==Phase::Guard);});
 test("damage_during_original_restores",[]{Shared s;Actor a{kNative,&s};auto p=fresh();auto v=good();need(apply(p,v,a));v.health=110;v.action=5;restore(p,v,a);need(a.range==5000&&p.phase==Phase::ReturnNative);});
 test("interleaved_retail_nested",[]{Shared s;Actor a{kNative,&s},b{kNative,&s};auto p=fresh();auto v=good();need(apply(p,v,a));auto w=v;w.id=33;w.key=0x456;need(!apply(p,w,b));restore(p,w,b);need(a.range==1200&&b.range==5000);restore(p,v,a);need(a.range==5000&&!p.depth);});
 test("nested_selected_rejected_outer_restore",[]{Shared s;Actor a{kNative,&s};auto p=fresh();auto v=good();need(apply(p,v,a));need(!apply(p,v,a)&&p.fault);restore(p,v,a);restore(p,v,a);need(a.range==5000&&!p.depth);});
 test("stack_overflow_preserves_outer",[]{Shared s;Actor a{kNative,&s};auto p=fresh();auto v=good();need(apply(p,v,a));for(int i=0;i<9;++i)p.push();float x;for(int i=0;i<9;++i)p.end(View{},x);restore(p,v,a);need(a.range==5000&&!p.depth&&!p.overflow&&p.fault);});
 test("missing_post_safe_recovery",[]{Shared s;Actor a{kNative,&s};auto p=fresh();auto v=good();need(apply(p,v,a));need(p.selected());v.range=a.range;float x;need(p.end(v,x)==Pilot::End::Restore);a.range=x;p.reject("CALLBACK_SCOPE_INCOMPLETE");need(a.range==5000&&p.fault);});
 test("wrong_post_no_foreign_write_then_recover",[]{Shared s;Actor a{kNative,&s};auto p=fresh();auto v=good();need(apply(p,v,a));auto w=v;w.id=33;float x;need(p.end(w,x)==Pilot::End::Reject&&p.depth==1);v.range=a.range;need(p.end(v,x)==Pilot::End::Restore);a.range=x;need(a.range==5000);});
 test("other_writer_never_overwritten",[]{auto p=fresh();auto v=good();p.push();float x;need(p.begin(v,x));v.range=777;need(p.end(v,x)==Pilot::End::Reject&&p.fault&&v.range==777);});
 test("generation_recycled_and_release",[]{auto p=fresh();p.released();auto v=good();p.push();float x;need(!p.begin(v,x));p.end(v,x);p.admit(563,2);v.id=147;need(!p.eligible(v));v.id=563;v.generation=2;v.epoch=2;need(p.eligible(v));});
 test("first_admission_inside_ignored_pre_frame",[]{Pilot p;p.push();p.admit(147,1);need(!p.fault&&p.id==147&&p.depth==1);auto v=good();float x;need(p.begin(v,x));v.range=x;need(p.end(v,x)==Pilot::End::Restore&&!p.depth);});
 test("missing_post_with_ignored_nested_sentinel",[]{Shared s;Actor a{kNative,&s};auto p=fresh();auto v=good();need(apply(p,v,a));p.push();while(p.depth){const auto prior=p.depth;auto w=p.frames[p.depth-1].selected?v:View{};w.range=a.range;float x;if(p.end(w,x)==Pilot::End::Restore)a.range=x;else if(p.depth==prior)p.frames[--p.depth]=Frame{};}need(a.range==5000&&!p.depth);});
 test("native_wait_ends_interest",[]{auto p=fresh();p.phase=Phase::Interest;auto v=good();p.push();float x;need(p.begin(v,x));v.range=x;need(p.end(v,x)==Pilot::End::Restore&&p.phase==Phase::ReturnNative);});
 test("null_hook_or_callback",[]{auto p=fresh();float x;need(p.end(View{},x)==Pilot::End::Reject);p=fresh();p.push();need(!p.begin(View{},x));need(p.end(View{},x)==Pilot::End::Ignored);});
 test("nan_and_no_allocation_core",[]{for(float f:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-1.f}){auto p=fresh();auto v=good();v.player2=f;p.push();float x;need(!p.begin(v,x)&&p.fault);}need(sizeof(Pilot)<1024);});
 test("scene_change_restores_and_cancels",[]{Shared s;Actor a{kNative,&s};auto p=fresh();p.phase=Phase::Interest;auto v=good();need(apply(p,v,a));v.context=false;restore(p,v,a);need(a.range==5000);p.push();float x;need(!p.begin(v,x)&&p.phase==Phase::ReturnNative);});
 std::printf("LAB_PASS cases=%u OFFLINE_ONLY game_executed=false\n",n);
 }catch(...){std::printf("LAB_FAIL case=%s completed=%u\n",name,n);return 1;}
}
