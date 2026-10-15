#include "encounter_core.hpp"
#include <d/d_com_inf_game.h>
#include <f_op/f_op_actor_iter.h>
#include <f_op/f_op_actor_mng.h>
#include <f_op/f_op_scene_mng.h>
#include <f_pc/f_pc_layer.h>
#include <f_pc/f_pc_manager.h>
namespace encounter {
namespace{
struct Membership{const void* target;unsigned visited=0;bool found=false;};
void* member(void* actor,void* data){auto& m=*static_cast<Membership*>(data);if(!actor)return data;if(actor==m.target){m.found=true;return actor;}if(++m.visited>=512)return actor;return nullptr;}
bool complete(const base_process_class& p){return p.state.init_state==2&&!fpcM_IsCreating(p.id);}
bool registered_layer(const layer_class* target){if(!target)return false;std::array<const layer_class*,256> seen{};unsigned n=0;for(auto* p=fpcLy_RootLayer();p;p=reinterpret_cast<layer_class*>(p->node.mpNextNode)){if(n==seen.size())return false;for(unsigned i=0;i<n;++i)if(seen[i]==p)return false;seen[n++]=p;if(p==target)return true;}return false;}
}
bool read_native(uint64_t tick,Sample& out,const char*& reason)noexcept{
 out=Sample{};reason="CAPTURE_NATIVE_UNAVAILABLE";try{
  if(!tick||dComIfGp_isEnableNextStage()){reason="CAPTURE_TRANSITION";return false;}
  if(dComIfGp_event_runCheck()){reason="CAPTURE_EVENT_ACTIVE";return false;}
  const char* name=dComIfGp_getStartStageName();if(!name){reason="CAPTURE_NO_STAGE";return false;}
  for(unsigned i=0;i<8;++i){if(!name[i])break;out.stage+=name[i];}
  if(!stage_ok(out.stage)){reason="CAPTURE_STAGE_INVALID";return false;}
  const int room=dStage_roomControl_c::getStayNo();const int layer=dComIfG_play_c::getLayerNo(0);
  if(room<0||room>=64||layer<0||layer>=16){reason="CAPTURE_ROOM_LAYER_UNKNOWN";return false;}
  auto* p=dComIfGp_getPlayer(0);Membership membership{p};if(!p){reason="CAPTURE_NO_LINK";return false;}
  fopAcIt_Judge(member,&membership);
  if(!membership.found||p->profname!=fpcNm_ALINK_e||!complete(*p)){reason="CAPTURE_LINK_NOT_READY";return false;}
  if(!(p->actor_condition&fopAcCnd_INIT_e)||p->current.roomNo!=room||!registered_layer(p->layer_tag.layer)){reason="CAPTURE_LINK_CONTEXT";return false;}
  const auto room_id=dStage_roomControl_c::getStatusProcID(room);const auto* scene=fopScnM_SearchByID(room_id);
  if(!scene||scene->base.base.id!=room_id||scene->base.base.profname!=fpcNm_ROOM_SCENE_e||!complete(scene->base.base)||!registered_layer(&scene->base.layer)||scene->base.layer.counts.create_count||scene->base.layer.counts.delete_count||fpcLy_IsDeletingMesg(const_cast<layer_class*>(&scene->base.layer))){reason="CAPTURE_ROOM_UNSTABLE";return false;}
  out.room=room;out.layer=layer;out.x=p->current.pos.x;out.y=p->current.pos.y;out.z=p->current.pos.z;out.yaw=p->current.angle.y;out.tick=tick;out.origin="link_readonly_v203";out.valid=true;
  // All borrows stay on this synchronous stack; reject changed context before publishing a value.
  const char* again=dComIfGp_getStartStageName();
  if(dComIfGp_isEnableNextStage()||dComIfGp_event_runCheck()||dComIfGp_getPlayer(0)!=p||dStage_roomControl_c::getStayNo()!=room||dComIfG_play_c::getLayerNo(0)!=layer||!again||std::strncmp(again,out.stage.c_str(),8)||dStage_roomControl_c::getStatusProcID(room)!=room_id||!capture_gate(out,{true,false,false,true,tick,tick})){out=Sample{};reason="CAPTURE_CONTEXT_CHANGED_OR_INVALID";return false;}
  reason="LINK_CURRENT_READ_ONLY_SNAPSHOT";return true;
 }catch(...){out=Sample{};reason="CAPTURE_NATIVE_EXCEPTION";return false;}
}
}
