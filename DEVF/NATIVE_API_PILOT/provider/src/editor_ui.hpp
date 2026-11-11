#pragma once
#include "persistence.hpp"
#include <mods/svc/ui.h>
namespace editor7 {
struct Ui {
 ModContext* ctx=nullptr;const UiService* api=nullptr;Editor editor;
 UiMenuTabHandle menu=0;UiWindowHandle window=0;UiElementHandle status=0;
 bool alive=false,built=false;ModResult cleanup_result=MOD_OK;const char* reason="UI_NOT_STARTED";
 struct Toggle {Ui* owner=nullptr;unsigned site=0;};Toggle controls[2]{{this,0},{this,1}};
 static Ui* owner(ModContext* c,void* p) noexcept {auto* x=static_cast<Ui*>(p);return x&&x->alive&&x->ctx==c?x:nullptr;}
 static bool enabled(const UiService* s) noexcept {
  return native_qa::compatible(s,2,3)&&s->register_menu_tab&&s->unregister_menu_tab&&s->window_push&&s->window_close&&s->pane_add_text&&s->pane_add_control&&s->elem_set_text;
 }
 void toast(const char* text) noexcept {
  if(!alive||!api||!api->push_toast)return;
  UiToastDesc t=UI_TOAST_DESC_INIT;t.title_rml="DEVF Editor";t.body_rml=text;
  try{api->push_toast(ctx,&t);}catch(...){}
 }
 void refresh() noexcept {
  if(!alive||!built||!status)return;
  const auto saved=editor.saved.kind==Kind::Valid?editor.saved.config.active():std::array<bool,2>{};
  const char* state=editor.saved.kind==Kind::Valid?"Archivo valido":editor.saved.kind==Kind::Missing?"Archivo ausente: Guardar crea explicitamente":editor.saved.kind==Kind::Invalid?"JSON invalido: solo lectura":"Error de acceso: solo lectura";
  const char* action=std::strcmp(editor.message,"SAVED_VERIFIED_RESTART_REQUIRED")==0?"Guardado y verificado. Cierre completamente y reinicie Dusklight.":std::strcmp(editor.message,"DRAFT_DISCARDED")==0?"Cambios descartados.":std::strncmp(editor.message,"SAVE_FAILED",11)==0?"ERROR al guardar: revise respaldo y no reinicie antes de recuperar.":"Los cambios provisionales no alteran enemigos en esta sesion.";
  char text[1024]{};
  std::snprintf(text,sizeof(text),"%s\nGuardado: FIELD=%s NORTH=%s\nProvisional: FIELD=%s NORTH=%s\n%s\n%s\nEstado=%s detalle=%s\nRespaldo=%s",
   state,saved[0]?"SI":"NO",saved[1]?"SI":"NO",editor.draft[0]?"SI":"NO",editor.draft[1]?"SI":"NO",
   editor.dirty()?"MODIFICACIONES PENDIENTES":"Sin cambios pendientes",action,editor.message,editor.store.detail,editor.store.backup_name[0]?editor.store.backup_name:"NINGUNO");
  try{if(api->elem_set_text(ctx,status,text)!=MOD_OK){status=0;reason="UI_TEXT_UPDATE_FAILED";toast("No se pudo actualizar el estado del editor.");}}catch(...){status=0;reason="UI_TEXT_EXCEPTION";}
 }
 static void get(ModContext* c,void* p,UiControlValue* value) noexcept {
  auto* t=static_cast<Toggle*>(p);if(!value||value->struct_size<sizeof(UiControlValue))return;
  value->bool_value=t&&t->owner&&t->owner->alive&&t->owner->ctx==c&&t->site<2?t->owner->editor.draft[t->site]:false;
 }
 static void set(ModContext* c,void* p,const UiControlValue* value) noexcept {
  auto* t=static_cast<Toggle*>(p);
  if(t&&t->owner&&t->owner->alive&&t->owner->built&&t->owner->ctx==c&&value&&value->struct_size>=sizeof(UiControlValue)){
   t->owner->editor.toggle(t->site,value->bool_value);t->owner->refresh();
  }
 }
 static bool disabled(ModContext* c,void* p) noexcept {
  auto* t=static_cast<Toggle*>(p);return !t||!t->owner||!t->owner->alive||!t->owner->built||t->owner->ctx!=c||!t->owner->editor.editable();
 }
 static bool modified(ModContext* c,void* p) noexcept {
  auto* t=static_cast<Toggle*>(p);if(!t||!t->owner||!t->owner->alive||t->owner->ctx!=c||t->site>1)return false;
  const auto& e=t->owner->editor;const auto flags=e.saved.kind==Kind::Valid?e.saved.config.active():std::array<bool,2>{};
  return e.draft[t->site]!=flags[t->site];
 }
 static bool save_disabled(ModContext* c,void* p) noexcept {
  auto* x=owner(c,p);return !x||!x->built||!x->editor.editable()||(!x->editor.dirty()&&x->editor.saved.kind!=Kind::Missing);
 }
 static bool discard_disabled(ModContext* c,void* p) noexcept {auto* x=owner(c,p);return !x||!x->built||x->editor.busy||!x->editor.dirty();}
 static void save_pressed(ModContext* c,void* p) noexcept {
  auto* x=owner(c,p);if(!x||save_disabled(c,p))return;
  const bool ok=x->editor.save();x->refresh();
  x->toast(ok?"Configuracion guardada y verificada. Cierre y reinicie Dusklight.":"No se guardo la configuracion. Revise el estado y el respaldo del editor.");
 }
 static void discard_pressed(ModContext* c,void* p) noexcept {auto* x=owner(c,p);if(x&&x->built){x->editor.discard();x->refresh();}}
 static void closed(ModContext* c,UiWindowHandle w,void* p) noexcept {
  auto* x=owner(c,p);if(x&&x->window==w){x->window=0;x->status=0;x->built=false;} // draft survives close/reopen
 }
 static ModResult build(ModContext* c,UiWindowHandle w,UiElementHandle left,UiElementHandle,void* p,ModError*) noexcept {
  auto* x=owner(c,p);if(!x)return MOD_OK;
  x->window=w;x->status=0;x->built=false;
  try{
   auto text=[&](const char* s){return x->api->pane_add_text(c,left,s,nullptr)==MOD_OK;};
   bool ok=text("DEVF Editor - dos Bokoblins nativos. Solo configuracion para el siguiente arranque.")&&
    text("FIELD: (-1250, -6304.00830078125, 60750)")&&text("NORTH: (-1250, -6322.6005859375, 61950)")&&
    text("IMPORTANTE: cerrar completamente y reiniciar Dusklight para aplicar cambios. No hot reload.");
   for(unsigned i=0;i<2&&ok;++i){
    UiControlDesc t=UI_CONTROL_DESC_INIT;t.kind=UI_CONTROL_TOGGLE;t.label=i?"Bokoblin NORTH":"Bokoblin FIELD";
    t.get=get;t.set=set;t.is_disabled=disabled;t.is_modified=modified;t.user_data=&x->controls[i];
    ok=x->api->pane_add_control(c,left,&t,nullptr)==MOD_OK;
   }
   if(ok){UiControlDesc b=UI_CONTROL_DESC_INIT;b.label="Guardar configuración";b.on_pressed=save_pressed;b.is_disabled=save_disabled;b.user_data=x;ok=x->api->pane_add_control(c,left,&b,nullptr)==MOD_OK;}
   if(ok){UiControlDesc b=UI_CONTROL_DESC_INIT;b.label="Descartar cambios";b.on_pressed=discard_pressed;b.is_disabled=discard_disabled;b.user_data=x;ok=x->api->pane_add_control(c,left,&b,nullptr)==MOD_OK;}
   if(ok)ok=x->api->pane_add_text(c,left,"Estado", &x->status)==MOD_OK&&x->status;
   x->built=ok;x->reason=ok?"UI_EDITOR_BUILT":"UI_BUILD_FAILED_CORE_PRESERVED";
   if(ok)x->refresh();else{x->status=0;x->toast("Error creando el editor; nucleo CP06 conservado.");}
  }catch(...){x->built=false;x->status=0;x->reason="UI_BUILD_EXCEPTION_CORE_PRESERVED";}
  return MOD_OK; // optional editor failure must not disable working CP06
 }
 static void selected(ModContext* c,void* p) noexcept {
  auto* x=owner(c,p);if(!x||x->window)return;
  UiTabDesc tab=UI_TAB_DESC_INIT;tab.title="DEVF Editor";tab.build=build;tab.user_data=x;
  UiWindowDesc desc=UI_WINDOW_DESC_INIT;desc.tabs=&tab;desc.tab_count=1;desc.on_closed=closed;desc.user_data=x;
  UiWindowHandle out=0;
  try{
   const auto result=x->api->window_push(c,&desc,&out);
   if(result!=MOD_OK||!out){
    const auto partial=out?out:x->window;if(partial){try{x->api->window_close(c,partial);}catch(...){}}
    x->window=0;x->status=0;x->built=false;x->reason="UI_WINDOW_OPEN_FAILED";x->toast("No se pudo abrir DEVF Editor.");return;
   }
   x->window=out;
  }catch(...){x->reason="UI_WINDOW_EXCEPTION";if(x->window){try{x->api->window_close(c,x->window);}catch(...){}}x->window=0;x->status=0;x->built=false;}
 }
 bool initialize(ModContext* c,const HostService* host,const FileService* file,const UiService* ui) noexcept {
  if(alive||menu)return false;ctx=c;api=ui;
  if(!c||!enabled(ui)||!native_qa::compatible(host,2,2)||!host->version||(std::strcmp(host->version,"2.0.3")&&std::strcmp(host->version,"2.0.3.0"))){reason="UI_UNAVAILABLE_CORE_PRESERVED";return false;}
  alive=true;editor.initialize(c,host,file);
  UiMenuTabDesc t=UI_MENU_TAB_DESC_INIT;t.label="DEVF Editor";t.on_selected=selected;t.user_data=this;
  try{
   const auto r=ui->register_menu_tab(c,&t,&menu);
   if(r==MOD_OK&&menu){reason="UI_MENU_REGISTERED";return true;}
  }catch(...){}
  if(menu){try{ui->unregister_menu_tab(c,menu);}catch(...){}menu=0;}
  alive=false;editor.shutdown();reason="UI_MENU_FAILED_CORE_PRESERVED";return false;
 }
 ModResult shutdown() noexcept {
  alive=false;built=false;status=0;editor.shutdown();ModResult failure=MOD_OK;
  if(api){if(menu){try{if(api->unregister_menu_tab(ctx,menu)!=MOD_OK)failure=MOD_ERROR;}catch(...){failure=MOD_ERROR;}menu=0;}
   if(window){try{if(api->window_close(ctx,window)!=MOD_OK)failure=MOD_ERROR;}catch(...){failure=MOD_ERROR;}window=0;}}
  cleanup_result=failure;return failure; // host UiService.modDetached removes all residual owned slots
 }
};
}
