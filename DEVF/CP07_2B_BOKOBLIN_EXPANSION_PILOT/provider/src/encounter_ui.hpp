#pragma once
#include "encounter_store.hpp"
#include "editor_ui.hpp"
namespace encounter {
struct Ui {
 editor7::Ui legacy;Session session;bool active=false,pending=false;uint64_t tick=0;
 void(*event)(const char*,ModResult) noexcept=nullptr;
 UiElementHandle status=0;UiListHandle list=0;const char* capture_reason="CAPTURE_NOT_REQUESTED";
 struct Binding{Ui* owner;unsigned field;};std::array<Binding,8> fields{{{this,0},{this,1},{this,2},{this,3},{this,4},{this,5},{this,6},{this,7}}};
 static Ui* owner(ModContext* c,void* p){auto* x=static_cast<Ui*>(p);return x&&x->legacy.alive&&x->legacy.ctx==c?x:nullptr;}
 bool writable()const{return active&&legacy.alive&&session.writable();}
 void toast(const char* s)noexcept{legacy.toast(s);}
 void refresh()noexcept{
  if(!active||!status||!legacy.alive)return;
  try{
   const auto& m=session.model;const auto& s=m.draft.sample;
   char b[1600];std::snprintf(b,sizeof(b),"DISENO Y EXPORTACION PARA FUTURA COMPILACION. No crea enemigos. API@1.0 conserva solo FIELD/NORTH.\nColeccion: %zu/%u; %s. Seleccion=%s\nBorrador: %s\nCaptura: %s; origen=%s tick=%llu\nStage=%s room=%d layer=%d XYZ=(%.17g, %.17g, %.17g) yaw capturado=%d\nTerreno/grupo NO verificados. Respawn es solo metadato, NO aplicado.\nEstado=%s\nArchivo: encounters.work.json; exportacion: encounters.export.json\nRespaldo trabajo=%s; export=%s",
    m.working.size(),MaxEncounters,m.dirty()?"CAMBIOS DE COLECCION PENDIENTES":"Coleccion guardada",m.selected<0?"NUEVO":m.draft.id.c_str(),m.draft.name.c_str(),capture_reason,s.origin.c_str(),static_cast<unsigned long long>(s.tick),s.stage.c_str(),s.room,s.layer,s.x,s.y,s.z,s.yaw,m.message,session.work.backup[0]?session.work.backup:"NINGUNO",session.output.backup[0]?session.output.backup:"NINGUNO");
   if(legacy.api->elem_set_text(legacy.ctx,status,b)!=MOD_OK){status=0;active=false;return;}
   if(list){std::vector<UiListItem> items;for(size_t i=0;i<m.working.size();++i){UiListItem row=UI_LIST_ITEM_INIT;row.key=i+1;row.label=m.working[i].name.c_str();items.push_back(row);}if(legacy.api->list_set_items(legacy.ctx,list,items.data(),items.size())!=MOD_OK){list=0;active=false;}}
  }catch(...){active=false;status=0;}
 }
 static void get(ModContext* c,void* p,UiControlValue* out)noexcept{
  auto* b=static_cast<Binding*>(p);if(!b||!out||out->struct_size<sizeof(*out))return;auto* x=b->owner;if(!x||!owner(c,x))return;const auto& e=x->session.model.draft;
  switch(b->field){case 0:out->string_value=e.id.c_str();break;case 1:out->string_value=e.name.c_str();break;case 2:out->int_value=e.quantity;break;case 3:out->int_value=e.yaw;break;case 4:out->int_value=e.spacing;break;case 5:out->int_value=e.respawn;break;case 6:out->bool_value=x->session.model.delete_confirmed;break;case 7:out->int_value=0;break;}
 }
 static bool disabled(ModContext* c,void* p)noexcept{auto* b=static_cast<Binding*>(p);return !b||!b->owner||!owner(c,b->owner)||!b->owner->writable()||(b->field==0&&b->owner->session.model.selected>=0);}
 static void set(ModContext* c,void* p,const UiControlValue* in)noexcept{
  auto* b=static_cast<Binding*>(p);if(!b||!in||in->struct_size<sizeof(*in)||disabled(c,p))return;auto& x=*b->owner;auto& e=x.session.model.draft;
  try{x.pending=false;switch(b->field){
   case 0:case 1:{if(!in->string_value)return;size_t n=0,limit=b->field?96:32;while(n<=limit&&in->string_value[n])++n;if(n>limit){x.session.model.message="TEXT_TOO_LONG";break;}auto s=std::string(in->string_value,n);if(b->field)e.name=std::move(s);else e.id=std::move(s);break;}
   case 2:if(in->int_value>=1&&in->int_value<=MaxQuantity)e.quantity=unsigned(in->int_value);break;
   case 3:if(in->int_value>=-32768&&in->int_value<=32767)e.yaw=int(in->int_value);break;
   case 4:if(in->int_value>=0&&in->int_value<=1000)e.spacing=unsigned(in->int_value);break;
   case 5:if(in->int_value>=0&&in->int_value<=1)e.respawn=unsigned(in->int_value);break;
   case 6:x.session.model.delete_confirmed=in->bool_value;break;
   case 7:if(in->int_value!=0)x.session.model.message="NATIVE_CATALOG_ONLY";break;
  }x.refresh();}catch(...){x.session.model.message="UI_EDIT_ALLOCATION_FAILED";}
 }
 static bool action_disabled(ModContext* c,void* p)noexcept{auto* x=owner(c,p);return !x||!x->writable();}
 static void perform(ModContext* c,void* p,unsigned action)noexcept{
  auto* x=owner(c,p);if(!x||!x->writable())return;try{
   auto& m=x->session.model;bool action_ok=true;if(action!=0)x->pending=false;
   switch(action){
    case 0:x->pending=true;x->capture_reason="CAPTURE_PENDING_NEXT_VALIDATED_UPDATE";m.message=x->capture_reason;break;
    case 1:m.fresh();break;case 2:m.apply();break;case 3:m.discard_draft();break;case 4:m.erase();break;
    case 5:action_ok=x->session.save();break;case 6:m.discard_collection();break;case 7:action_ok=x->session.export_file();break;
   }x->refresh();if(action==5||action==7){x->toast(m.message);if(x->event)x->event(m.message,action_ok?MOD_OK:MOD_ERROR);}
  }catch(...){x->session.model.message="EDITOR_ACTION_EXCEPTION_NO_SPAWN";x->refresh();}
 }
 static void capture(ModContext* c,void* p)noexcept{perform(c,p,0);}static void fresh(ModContext* c,void* p)noexcept{perform(c,p,1);}
 static void apply(ModContext* c,void* p)noexcept{perform(c,p,2);}static void discard(ModContext* c,void* p)noexcept{perform(c,p,3);}
 static void erase(ModContext* c,void* p)noexcept{perform(c,p,4);}static void save(ModContext* c,void* p)noexcept{perform(c,p,5);}
 static void reset(ModContext* c,void* p)noexcept{perform(c,p,6);}static void export_file(ModContext* c,void* p)noexcept{perform(c,p,7);}
 static void chosen(ModContext* c,UiListHandle h,uint64_t key,void* p)noexcept{auto* x=owner(c,p);if(x&&x->writable()&&h==x->list&&key&&key<=x->session.model.working.size()){try{x->pending=false;x->session.model.select(unsigned(key-1));x->refresh();}catch(...){x->session.model.message="SELECTION_FAILED";}}}
 static ModResult old_build(ModContext* c,UiWindowHandle w,UiElementHandle a,UiElementHandle b,void* p,ModError* e)noexcept{
  auto* x=owner(c,p);if(!x)return MOD_OK;x->active=false;x->pending=false;x->list=0;x->status=0;return editor7::Ui::build(c,w,a,b,&x->legacy,e);
 }
 static ModResult build(ModContext* c,UiWindowHandle w,UiElementHandle pane,UiElementHandle,void* p,ModError*)noexcept{
  auto* x=owner(c,p);if(!x)return MOD_OK;x->legacy.window=w;x->legacy.built=false;x->legacy.status=0;x->status=0;x->list=0;x->active=false;
  try{
   const auto* api=x->legacy.api;bool ok=api->pane_add_text(c,pane,"Encuentros: captura, coleccion y exportacion para futura compilacion. NO coloca enemigos ni aplica respawn.",nullptr)==MOD_OK;
   if(!api->pane_add_list||!api->list_set_items){x->toast("Encuentros requiere lista de UiService2.3; editor CP07.1 conservado.");return MOD_OK;}
   UiListDesc ld=UI_LIST_DESC_INIT;ld.on_pressed=chosen;ld.user_data=x;ok=ok&&api->pane_add_list(c,pane,&ld,&x->list)==MOD_OK&&x->list;
   static const char* respawn[]{"Al reentrar a la zona (metadato)","Sin reaparicion durante la sesion (metadato)"};
   static const char* catalog[]{"Bokoblin E_OC2 nativo (solo diseno)"};
   const char* labels[]{"ID estable (nuevo encuentro)","Nombre del encuentro","Cantidad (solo metadato)","Orientacion inicial s16","Separacion en unidades (solo metadato)","Politica de reaparicion NO aplicada","Confirmo eliminar el encuentro seleccionado","Catalogo"};
   for(unsigned i=0;i<8&&ok;++i){UiControlDesc q=UI_CONTROL_DESC_INIT;q.label=labels[i];q.user_data=&x->fields[i];q.get=get;q.set=set;q.is_disabled=disabled;
    q.kind=i<2?UI_CONTROL_STRING:i==5||i==7?UI_CONTROL_SELECT:i==6?UI_CONTROL_TOGGLE:UI_CONTROL_NUMBER;
    q.max_length=i?96:32;q.min=i==2?1:i==3?-32768:0;q.max=i==2?MaxQuantity:i==3?32767:1000;q.step=1;
    if(i==5){q.options=respawn;q.option_count=2;}if(i==7){q.options=catalog;q.option_count=1;}
    ok=api->pane_add_control(c,pane,&q,nullptr)==MOD_OK;
   }
   struct Action{const char* label;UiPressedFn fn;};const Action actions[]{
    {"Capturar posicion de Link",capture},{"Nuevo encuentro / limpiar borrador",fresh},{"Anadir / actualizar borrador en coleccion",apply},{"Descartar cambios del borrador",discard},
    {"Eliminar seleccionado (requiere confirmacion)",erase},{"Guardar coleccion de trabajo",save},{"Deshacer coleccion hasta ultimo guardado",reset},{"Exportar para Bokoblin Expansion (futura compilacion)",export_file}};
   for(const auto& a:actions)if(ok){UiControlDesc q=UI_CONTROL_DESC_INIT;q.label=a.label;q.on_pressed=a.fn;q.user_data=x;q.is_disabled=action_disabled;ok=api->pane_add_control(c,pane,&q,nullptr)==MOD_OK;}
   if(ok)ok=api->pane_add_text(c,pane,"Variantes futuras: NO DISPONIBLES. Ningun dato exportado es ejecutable por API@1.0 fuera de FIELD/NORTH.",nullptr)==MOD_OK;
   if(ok)ok=api->pane_add_text(c,pane,"Estado",&x->status)==MOD_OK&&x->status;
   x->active=ok;if(ok)x->refresh();else x->toast("Error construyendo encuentros; editor CP07.1 y API conservados.");
  }catch(...){x->active=false;x->status=0;x->list=0;}return MOD_OK;
 }
 static void closed(ModContext* c,UiWindowHandle w,void* p)noexcept{auto* x=owner(c,p);if(x&&x->legacy.window==w){x->legacy.window=0;x->legacy.built=false;x->legacy.status=0;x->active=false;x->pending=false;x->status=0;x->list=0;}}
 static void selected(ModContext* c,void* p)noexcept{
  auto* x=owner(c,p);if(!x||x->legacy.window)return;
  UiTabDesc tabs[2]{UI_TAB_DESC_INIT,UI_TAB_DESC_INIT};tabs[0].title="FIELD / NORTH";tabs[0].build=old_build;tabs[0].user_data=x;tabs[1].title="Diseno de encuentros";tabs[1].build=build;tabs[1].user_data=x;
  UiWindowDesc desc=UI_WINDOW_DESC_INIT;desc.tabs=tabs;desc.tab_count=2;desc.on_closed=closed;desc.user_data=x;UiWindowHandle out=0;
  try{const auto r=x->legacy.api->window_push(c,&desc,&out);if(r==MOD_OK&&out){x->legacy.window=out;return;}if(out)x->legacy.api->window_close(c,out);}catch(...){}
  if(x->legacy.window){try{x->legacy.api->window_close(c,x->legacy.window);}catch(...){}}x->legacy.window=0;x->active=false;x->status=0;x->pending=false;x->toast("No se pudo abrir DEVF Editor.");
 }
 bool initialize(ModContext* c,const HostService* h,const FileService* f,const UiService* api)noexcept{
  if(legacy.alive)return false;legacy.ctx=c;legacy.api=api;
  if(!c||!editor7::Ui::enabled(api)||!native_qa::compatible(h,2,2)||!h->version||(std::strcmp(h->version,"2.0.3")&&std::strcmp(h->version,"2.0.3.0"))){legacy.reason="UI_UNAVAILABLE_CORE_PRESERVED";return false;}
  try{legacy.alive=true;legacy.editor.initialize(c,h,f);session.initialize(c,h,f);UiMenuTabDesc tab=UI_MENU_TAB_DESC_INIT;tab.label="DEVF Editor";tab.on_selected=selected;tab.user_data=this;
   if(api->register_menu_tab(c,&tab,&legacy.menu)==MOD_OK&&legacy.menu){legacy.reason="UI_MENU_REGISTERED";return true;}
  }catch(...){}shutdown();legacy.reason="UI_MENU_FAILED_CORE_PRESERVED";return false;
 }
 void update()noexcept{
  if(tick<9007199254740990ULL)++tick;
  if(!pending)return;pending=false;if(!legacy.alive||!active||!legacy.window){capture_reason="CAPTURE_CANCELED_UI_CLOSED";return;}
  try{Sample sample;if(read_native(tick,sample,capture_reason))session.model.capture(sample);else session.model.message=capture_reason;if(event)event(capture_reason,session.model.draft.sample.valid&&std::strcmp(capture_reason,"LINK_CURRENT_READ_ONLY_SNAPSHOT")==0?MOD_OK:MOD_UNAVAILABLE);refresh();}catch(...){capture_reason="CAPTURE_EXCEPTION";session.model.message=capture_reason;}
 }
 ModResult shutdown()noexcept{active=false;pending=false;status=0;list=0;session.shutdown();event=nullptr;return legacy.shutdown();}
};
}
