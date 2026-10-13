#include "native_backend.hpp"
#include <d/d_com_inf_game.h>
#include <f_pc/f_pc_layer.h>
#include <f_pc/f_pc_manager.h>
#include <f_pc/f_pc_create_iter.h>
#include <f_pc/f_pc_create_req.h>
#include <f_op/f_op_actor_iter.h>
#include <f_op/f_op_actor_mng.h>
#include <f_op/f_op_scene_mng.h>
#include <cstring>
#include <stdexcept>
namespace devf::first {
namespace {
// Minimal live registry for mutation scope/monitor; original G2 census is not duplicated.
// Every pointer below belongs to THIS stack only; current() treats unmatched pointer as opaque.
struct Layers {
 std::array<layer_class*,128> values{};unsigned count=0;bool complete=false;
 Layers(){auto* p=fpcLy_RootLayer();while(p){if(count==values.size())return;
  for(unsigned i=0;i<count;++i)if(values[i]==p||values[i]->layer_id==p->layer_id)return;
  values[count++]=p;p=reinterpret_cast<layer_class*>(p->node.mpNextNode);}complete=count!=0;}
 layer_class* find(Id id)const {if(!complete||id==unknown)return nullptr;for(unsigned i=0;i<count;++i)if(values[i]->layer_id==id)return values[i];return nullptr;}
 Id id_of(const layer_class* p)const {if(!complete)return unknown;for(unsigned i=0;i<count;++i)if(values[i]==p)return values[i]->layer_id;return unknown;}
};
Frame monitor_context(const Layers& layers){
 Frame f;auto& o=f.owner;o.host_ok=o.thread_ok=true;const char* stage=dComIfGp_getStartStageName();
 if(stage)for(unsigned i=0;i<8&&stage[i];++i)o.stage[i]=stage[i];
 o.room=dStage_roomControl_c::getStayNo();o.next_room=dStage_roomControl_c::getNextStayNo();o.logical=dComIfG_play_c::getLayerNo(0);
 o.root=layers.id_of(fpcLy_RootLayer());o.previous=layers.id_of(fpcLy_CurrentLayer());o.registry_ok=layers.complete;o.previous_registered=o.previous!=unknown;
 if(o.room<0||o.room>=64)return f;
 o.node=dStage_roomControl_c::getStatusProcID(o.room);o.play=dStage_roomControl_c::getProcID();
 auto* room=fopScnM_SearchByID(o.node);auto* play=fopScnM_SearchByID(o.play);o.node_present=room!=nullptr;
 if(!room||!play)return f;
 o.node_profile=room->base.base.profname==fpcNm_ROOM_SCENE_e;
 o.node_id_ok=room->base.base.id==o.node&&room->base.base.parameters==unsigned(o.room);
 o.node_complete=room->base.base.state.init_state==2&&!fpcM_IsCreating(o.node);
 o.play_ok=play->base.base.id==o.play&&play->base.base.profname==fpcNm_PLAY_SCENE_e&&play->base.base.state.init_state==2&&!fpcM_IsCreating(o.play);
 if(!o.node_profile||!o.node_id_ok||!o.node_complete||!o.play_ok)return f; // no scene-derived loans before complete
 o.child=layers.id_of(&room->base.layer);o.parent=layers.id_of(room->base.base.layer_tag.layer);o.play_layer=layers.id_of(&play->base.layer);
 o.child_registered=o.child!=unknown;
 o.binding_ok=o.child_registered&&o.parent==o.play_layer&&room->base.base.layer_tag.layer==&play->base.layer&&room->base.layer.process_node==&room->base&&play->base.layer.process_node==&play->base;
 o.creates=room->base.layer.counts.create_count;o.deletes=room->base.layer.counts.delete_count;o.parent_creates=play->base.layer.counts.create_count;o.parent_deletes=play->base.layer.counts.delete_count;
 o.transition=dComIfGp_isEnableNextStage()!=0||o.next_room!=o.room;
 o.retiring=dComIfGp_roomControl_checkStatusFlag(o.room,0x2e)!=0||fpcLy_IsDeletingMesg(&room->base.layer)||fpcLy_IsDeletingMesg(&play->base.layer);
 f.event_active=dComIfGp_event_runCheck()!=0;f.event_read=true;
 f.registered_eoc=dComIfG_getObjectResInfo("E_OC")!=nullptr;f.registered_weapon=dComIfG_getObjectResInfo("E_ocb")!=nullptr;
 f.stable=dStage_roomControl_c::getStayNo()==o.room&&dStage_roomControl_c::getStatusProcID(o.room)==o.node&&dStage_roomControl_c::getProcID()==o.play&&dComIfG_play_c::getLayerNo(0)==o.logical;
 return f;
}
struct Requests {Observation& out;const Layers& layers;unsigned visits=0,matches=0;};
void* request_match(void* ptr,void* data){auto& c=*static_cast<Requests*>(data);
 if(!ptr||++c.visits>256){c.out.request_scan_complete=false;return data;}
 const auto* q=static_cast<const create_request*>(ptr);
 if(q->id==c.out.id){if(++c.matches>1)c.out.ambiguous=true;c.out.request_present=true;c.out.request_owner=c.layers.id_of(q->layer);c.out.request_cancelled=q->is_cancel!=0;}
 return nullptr;
}
struct Actors {Observation& out;const Layers& layers;const void* link;unsigned visits=0,matches=0,link_matches=0;};
void* actor_match(void* ptr,void* data){auto& c=*static_cast<Actors*>(data);
 if(!ptr||++c.visits>256){c.out.actor_scan_complete=false;return data;}
 const auto* base=static_cast<const base_process_class*>(ptr);const bool target=base->id==c.out.id;const bool player=ptr==c.link;
 if(!target&&!player)return nullptr;
 if(target){if(++c.matches>1)c.out.ambiguous=true;c.out.actor_present=true;c.out.profile=base->profname;c.out.init=base->state.init_state;}
 // Existing queue membership establishes header; no actor fields until init2 + no pending.
 if(base->state.init_state!=2||fpcM_IsCreating(base->id))return nullptr;
 const auto* actor=static_cast<const fopAc_ac_c*>(ptr);const auto owner=c.layers.id_of(actor->layer_tag.layer);
 if(player){++c.link_matches;c.out.frame.owner.player_ok=c.link_matches==1&&base->profname==fpcNm_ALINK_e&&(actor->actor_condition&fopAcCnd_INIT_e)&&owner!=unknown;c.out.frame.owner.link_owner=owner;}
 if(target){c.out.derived_read=true;c.out.actor_init=(actor->actor_condition&fopAcCnd_INIT_e)!=0;c.out.owner=owner;
  // Room SDK search is unsafe for incomplete actors; call ONLY after this checked membership/phase.
  c.out.room=actor->current.roomNo;
 }
 return nullptr;
}
}
bool NativeBackend::service_available()const {
 return ctx_&&svc_actor&&svc_actor->header.struct_size>=sizeof(ActorService)&&svc_actor->header.major_version==1&&svc_actor->create_actor&&svc_actor->get_actor_room_num;
}
Id NativeBackend::current(){Layers r;return r.id_of(fpcLy_CurrentLayer());}
bool NativeBackend::registered(Id id){Layers r;return r.find(id)!=nullptr;}
void NativeBackend::select(Id id){Layers r;auto* p=r.find(id);if(!p)throw std::runtime_error("registered layer unavailable");fpcLy_SetCurrentLayer(p);}
RequestResult NativeBackend::request(const Preset& p){
 static_assert(fpcNm_E_OC_e==0x1fe);if(!service_available())return {};
 ActorSpawnParams native{};native.parameters=p.parameters;native.argument=static_cast<int8_t>(p.argument);native.room_num=static_cast<int8_t>(p.room);
 native.position={p.xyz[0],p.xyz[1],p.xyz[2]};native.angle={p.angle[0],p.angle[1],p.angle[2]};native.scale={p.scale[0],p.scale[1],p.scale[2]};native.create_function=nullptr;
 ActorId id=unknown;const auto result=svc_actor->create_actor(ctx_,static_cast<ProfileName>(p.profile),&native,&id); // ONLY official native create_actor call site
 return {static_cast<int>(result),id};
}
Observation NativeBackend::observe(Id id){
 Observation o;o.id=id;if(!service_available())return o;Layers r;o.frame=monitor_context(r);if(!r.complete)return o;
 o.creating=fpcM_IsCreating(id)!=0;o.request_scan_complete=true;Requests requests{o,r};fpcCtIt_Judge(request_match,&requests);
 o.actor_scan_complete=true;Actors actors{o,r,dComIfGp_getPlayer(0)};fopAcIt_Judge(actor_match,&actors);
 // Only a unique complete bounded membership scan permits the SDK's by-ID room search.
 if(o.actor_scan_complete&&!o.ambiguous&&actors.matches==1&&o.derived_read&&o.actor_init&&o.profile==0x1fe){int8_t room=-1;
  const auto result=svc_actor->get_actor_room_num(ctx_,id,&room);o.room_api_ok=result==MOD_OK&&room==o.room;}
 o.resources_registered=o.frame.registered_eoc&&o.frame.registered_weapon;
 if(o.frame.owner.room<0||o.frame.owner.room>=64){o.frame.stable=false;return o;}
 o.frame.stable=o.frame.stable&&dStage_roomControl_c::getStayNo()==o.frame.owner.room&&dStage_roomControl_c::getStatusProcID(o.frame.owner.room)==o.frame.owner.node&&dStage_roomControl_c::getProcID()==o.frame.owner.play&&r.id_of(fpcLy_CurrentLayer())==o.frame.owner.previous&&!dComIfGp_isEnableNextStage()&&dStage_roomControl_c::getNextStayNo()==o.frame.owner.next_room&&dComIfG_play_c::getLayerNo(0)==o.frame.owner.logical&&(dComIfGp_event_runCheck()!=0)==o.frame.event_active;
 return o;
}
}
extern "C" __declspec(noinline) void devf_first_request_link_anchor(devf::first::Controller& c,devf::first::NativeBackend& b,const devf::first::Admission& p){c.attempt(b,p);}
