#include "registration_cases.hpp"
#include "../src/editor_ui.hpp"
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <vector>
namespace fs=std::filesystem;
namespace io {
std::string folder,joined,op,target;unsigned nth=1,hits=0,operations=0,writes=0,creates=0;bool forever=false,no_end=false,change_after_backup=false;
struct Stream {std::fstream file;std::string name;bool writable=false;};
std::map<uint64_t,std::unique_ptr<Stream>> streams;uint64_t next=10;
HostService host{};FileService files{};
std::string both,field,disabled;
bool fail(const char* operation,const std::string& name){
 ++operations;const bool matches=op==operation&&(target.empty()||(target=="backup"?name.find("cp07-backup")!=std::string::npos:name=="placements.json"));
 return matches&&++hits>=nth&&(forever||hits==nth);
}
void put(const std::string& name,const std::string& bytes){std::ofstream x(fs::path(folder)/name,std::ios::binary|std::ios::trunc);x<<bytes;check(bool(x));}
std::string bytes(const std::string& name){std::ifstream x(fs::path(folder)/name,std::ios::binary);return std::string(std::istreambuf_iterator<char>(x),{});}
ModResult dir(ModContext* c,const char** out){check(c==ctx);if(fail("dir",""))return MOD_UNAVAILABLE;*out=folder.c_str();return MOD_OK;}
ModResult list(ModContext* c,const char*,FileListFn fn,void* u){
 check(c==ctx);if(fail("list",""))return MOD_ERROR;
 for(const auto& item:fs::directory_iterator(folder)){const auto name=item.path().filename().string();const auto location=item.path().string();FileEntry e{name.c_str(),location.c_str(),item.is_directory()};fn(c,&e,u);}
 if(!no_end)fn(c,nullptr,u);return MOD_OK;
}
ModResult join(ModContext* c,const char*,const char* child,const char** out){
 check(c==ctx);if(fail("join",child))return MOD_ERROR;
 if(!fs::exists(fs::path(folder)/child))return MOD_UNAVAILABLE;joined=(fs::path(folder)/child).string();*out=joined.c_str();return MOD_OK;
}
ModResult create(ModContext* c,const char*,const char* name,const char** out){
 check(c==ctx);if(fail("create",name))return MOD_ERROR;
 if(fs::exists(fs::path(folder)/name))return MOD_CONFLICT;++creates;put(name,"");joined=(fs::path(folder)/name).string();*out=joined.c_str();return MOD_OK;
}
ModResult open(ModContext* c,const char* p,FileOpenMode mode,FileStreamHandle* h){
 check(c==ctx);const auto name=fs::path(p).filename().string();const bool wr=mode!=FILE_OPEN_READ;
 if(fail(wr?"write_open":"read_open",name))return MOD_ERROR;
 auto s=std::make_unique<Stream>();s->name=name;s->writable=wr;
 s->file.open(p,std::ios::binary|std::ios::in|(wr?std::ios::out|std::ios::trunc:std::ios::openmode(0)));
 if(!s->file)return MOD_UNAVAILABLE;*h=++next;streams.emplace(*h,std::move(s));
 if(fail("open_throw",name))throw 1;return MOD_OK;
}
ModResult size(ModContext*,FileStreamHandle h,uint64_t* n){
 const auto& s=*streams.at(h);if(fail("size",s.name))return MOD_ERROR;*n=fs::file_size(fs::path(folder)/s.name);return MOD_OK;
}
ModResult read(ModContext*,FileStreamHandle h,void* p,uint64_t n,uint64_t* got){
 auto& s=*streams.at(h);if(fail("read",s.name))return MOD_ERROR;
 s.file.read(static_cast<char*>(p),std::streamsize(n));*got=uint64_t(s.file.gcount());
 if(n>1&&fail("read_corrupt",s.name)&&*got)static_cast<char*>(p)[0]='!';
 if(n>1&&fail("short",s.name)&&*got)--*got;return MOD_OK;
}
ModResult write(ModContext*,FileStreamHandle h,const void* p,uint64_t n){
 auto& s=*streams.at(h);++writes;
 if(fail("write_throw",s.name))throw 1;
 if(fail("write",s.name)){s.file.write(static_cast<const char*>(p),std::streamsize(n/2));return MOD_ERROR;}
 s.file.write(static_cast<const char*>(p),std::streamsize(n));return s.file?MOD_OK:MOD_ERROR;
}
ModResult flush(ModContext*,FileStreamHandle h){auto& s=*streams.at(h);s.file.flush();return fail("flush",s.name)?MOD_ERROR:MOD_OK;}
ModResult close(ModContext*,FileStreamHandle h){
 auto it=streams.find(h);check(it!=streams.end());auto& s=*it->second;
 const auto name=s.name;const bool wr=s.writable;const bool error=fail(wr?"write_close":"read_close",name);
 if(fail("close_throw",name))throw 1;s.file.close();streams.erase(it);
 if(change_after_backup&&!wr&&name.find("cp07-backup")!=std::string::npos){put("placements.json",field);change_after_backup=false;}
 return error?MOD_ERROR:MOD_OK;
}
void inject(const std::string& operation,const std::string& which="",unsigned n=1,bool all=false){op=operation;target=which;nth=n;hits=0;forever=all;}
void reset(){
 streams.clear();fs::remove_all(folder);fs::create_directories(folder);joined.clear();op.clear();target.clear();nth=1;hits=operations=writes=creates=0;forever=no_end=change_after_backup=false;next=10;
 host={};host.header={sizeof(HostService),2,2};host.version="2.0.3";host.data_dir=dir;
 files={};files.header={sizeof(FileService),1,0};files.list=list;files.join=join;files.create_child=create;files.open=open;files.size=size;files.read=read;files.write=write;files.flush=flush;files.close=close;
}
}
namespace gui {
UiService api{};UiMenuTabDesc menu{};UiWindowDesc window{};UiTabDesc tab{};
std::vector<UiControlDesc> controls;UiElementHandle status=0;std::string text,last_toast;
unsigned registrations=0,unregistrations=0,pushes=0,closes=0,textcalls=0,control_calls=0;
bool fail_register=false,zero_register=false,fail_push=false,zero_push=false,fail_control=false,fail_text=false,fail_remove=false;
ModResult register_tab(ModContext* c,const UiMenuTabDesc* d,UiMenuTabHandle* h){check(c==ctx&&std::string(d->label)=="DEVF Editor");++registrations;menu=*d;if(fail_register)return MOD_ERROR;*h=zero_register?0:1;return MOD_OK;}
ModResult unregister_tab(ModContext*,UiMenuTabHandle h){check(h==1);++unregistrations;return fail_remove?MOD_ERROR:MOD_OK;}
ModResult push(ModContext* c,const UiWindowDesc* w,UiWindowHandle* h){
 ++pushes;window=*w;tab=w->tabs[0];controls.clear();control_calls=0;
 if(fail_push)return MOD_ERROR;
 ModError e{};e.struct_size=sizeof(e);check(tab.build(c,2,100,101,tab.user_data,&e)==MOD_OK);
 *h=zero_push?0:2;return MOD_OK;
}
ModResult close(ModContext* c,UiWindowHandle w){check(w==2);++closes;if(window.on_closed)window.on_closed(c,w,window.user_data);return fail_remove?MOD_ERROR:MOD_OK;}
ModResult add_text(ModContext*,UiElementHandle,const char*,UiElementHandle* out){++textcalls;if(fail_text)return MOD_ERROR;if(out)*out=++status;return MOD_OK;}
ModResult set_text(ModContext*,UiElementHandle,const char* p){text=p;return MOD_OK;}
ModResult add_control(ModContext*,UiElementHandle,const UiControlDesc* p,UiElementHandle*){++control_calls;if(fail_control)return MOD_ERROR;controls.push_back(*p);return MOD_OK;}
ModResult toast(ModContext*,const UiToastDesc* t){last_toast=t->body_rml;return MOD_OK;}
void reset(){
 api={};api.header={sizeof(UiService),2,3};api.register_menu_tab=register_tab;api.unregister_menu_tab=unregister_tab;api.window_push=push;api.window_close=close;api.pane_add_text=add_text;api.elem_set_text=set_text;api.pane_add_control=add_control;api.push_toast=toast;
 menu={};window={};tab={};controls.clear();status=0;text.clear();last_toast.clear();
 registrations=unregistrations=pushes=closes=textcalls=control_calls=0;
 fail_register=zero_register=fail_push=zero_push=fail_control=fail_text=fail_remove=false;
}
void open(){check(menu.on_selected);menu.on_selected(ctx,menu.user_data);}
void toggle(unsigned i,bool b){check(i<controls.size());UiControlValue v=UI_CONTROL_VALUE_INIT;v.bool_value=b;controls[i].set(ctx,controls[i].user_data,&v);}
bool value(unsigned i){UiControlValue v=UI_CONTROL_VALUE_INIT;controls[i].get(ctx,controls[i].user_data,&v);return v.bool_value;}
void button(unsigned i){check(i<controls.size()&&controls[i].on_pressed);controls[i].on_pressed(ctx,controls[i].user_data);}
}
template<class F>void run(const char* name,F fn){test(name,[&]{io::reset();gui::reset();fn();check(io::streams.empty());});}
void init(editor7::Editor& e){e.initialize(ctx,&io::host,&io::files);}
void load_both(editor7::Editor& e){io::put("placements.json",io::both);init(e);check(e.editable()&&e.draft[0]&&e.draft[1]);}
void verify_flags(bool field,bool north){native_config::Config c;native_config::Error e;check(native_config::decode(io::bytes("placements.json"),c,e));check(c.active()==std::array<bool,2>{field,north});}
int main(int argc,char** argv){
 if(argc!=3)return 2;io::folder=argv[2];
 auto input=[&](const char* n){std::ifstream x(fs::path(argv[1])/n,std::ios::binary);check(bool(x));return std::string(std::istreambuf_iterator<char>(x),{});};
 io::both=input("both-enabled.json");io::field=input("field-only.json");io::disabled=input("disabled.json");
 try{
 run("ui_two_controls_saved_vs_draft",[]{io::put("placements.json",io::both);editor7::Ui u;check(u.initialize(ctx,&io::host,&io::files,&gui::api));gui::open();check(u.built&&gui::controls.size()==4&&gui::value(0)&&gui::value(1));const auto ops=io::operations;gui::toggle(1,false);check(gui::value(0)&&!gui::value(1)&&u.editor.dirty()&&io::bytes("placements.json")==io::both&&io::operations==ops&&fake.adds==0);check(gui::text.find("PENDIENTES")!=std::string::npos);u.shutdown();});
 run("ui_save_backup_and_restart_field",[]{io::put("placements.json",io::both);editor7::Ui u;check(u.initialize(ctx,&io::host,&io::files,&gui::api));gui::open();gui::toggle(1,false);gui::button(2);check(!u.editor.dirty()&&std::string(u.editor.message)=="SAVED_VERIFIED_RESTART_REQUIRED"&&io::bytes(u.editor.store.backup_name)==io::both);verify_flags(true,false);u.shutdown();editor7::Editor next;init(next);check(next.draft[0]&&!next.draft[1]);});
 run("ui_reenable_north_save_both",[]{io::put("placements.json",io::field);editor7::Ui u;u.initialize(ctx,&io::host,&io::files,&gui::api);gui::open();gui::toggle(1,true);gui::button(2);verify_flags(true,true);u.shutdown();});
 run("ui_discard_no_file_write",[]{io::put("placements.json",io::both);editor7::Ui u;u.initialize(ctx,&io::host,&io::files,&gui::api);gui::open();gui::toggle(0,false);gui::button(3);check(gui::value(0)&&!u.editor.dirty()&&io::writes==0);u.shutdown();});
 run("ui_close_reopen_retains_draft_no_io",[]{io::put("placements.json",io::both);editor7::Ui u;u.initialize(ctx,&io::host,&io::files,&gui::api);gui::open();gui::toggle(0,false);const auto n=io::operations;gui::close(ctx,2);check(!u.window);gui::open();check(!gui::value(0)&&gui::value(1)&&io::operations==n);u.shutdown();});
 run("ui_polled_callbacks_no_io_or_spawn",[]{io::put("placements.json",io::both);editor7::Ui u;u.initialize(ctx,&io::host,&io::files,&gui::api);gui::open();const auto n=io::operations;for(unsigned i=0;i<1000;++i)for(unsigned j=0;j<2;++j){gui::value(j);gui::controls[j].is_disabled(ctx,gui::controls[j].user_data);gui::controls[j].is_modified(ctx,gui::controls[j].user_data);}check(io::operations==n&&fake.adds==0);u.shutdown();});
 run("ui_absent_core_still_two",[]{io::put("placements.json",io::both);native_config::Runtime c;check(c.initialize(ctx,&io::host,&io::files,&service)==MOD_OK&&fake.adds==2);editor7::Ui u;check(!u.initialize(ctx,&io::host,&io::files,nullptr));c.shutdown(ctx,&service);});
 run("ui_wrong_version_optional",[]{gui::api.header.minor_version=2;editor7::Ui u;check(!u.initialize(ctx,&io::host,&io::files,&gui::api)&&gui::registrations==0&&io::operations==0);});
 run("ui_missing_window_api",[]{gui::api.window_push=nullptr;editor7::Ui u;check(!u.initialize(ctx,&io::host,&io::files,&gui::api));});
 run("ui_menu_registration_failure",[]{gui::fail_register=true;editor7::Ui u;check(!u.initialize(ctx,&io::host,&io::files,&gui::api));u.shutdown();});
 run("ui_zero_menu_handle",[]{gui::zero_register=true;editor7::Ui u;check(!u.initialize(ctx,&io::host,&io::files,&gui::api));u.shutdown();});
 run("ui_window_push_failure",[]{editor7::Ui u;u.initialize(ctx,&io::host,&io::files,&gui::api);gui::fail_push=true;gui::open();check(!u.window&&!u.built);u.shutdown();});
 run("ui_window_zero_handle_cleans_partial",[]{editor7::Ui u;u.initialize(ctx,&io::host,&io::files,&gui::api);gui::zero_push=true;gui::open();check(!u.window&&!u.built&&gui::closes==1);u.shutdown();});
 run("ui_partial_build_disables_controls",[]{editor7::Ui u;u.initialize(ctx,&io::host,&io::files,&gui::api);gui::fail_control=true;gui::open();check(!u.built);u.shutdown();});
 run("ui_text_build_failure",[]{editor7::Ui u;u.initialize(ctx,&io::host,&io::files,&gui::api);gui::fail_text=true;gui::open();check(!u.built);u.shutdown();});
 run("ui_duplicate_open_prevented",[]{editor7::Ui u;u.initialize(ctx,&io::host,&io::files,&gui::api);gui::open();gui::open();check(gui::pushes==1);u.shutdown();});
 run("ui_shutdown_stale_callbacks_and_handles",[]{io::put("placements.json",io::both);editor7::Ui u;u.initialize(ctx,&io::host,&io::files,&gui::api);gui::open();auto t=gui::controls[0];auto save=gui::controls[2];u.shutdown();const auto n=io::operations;UiControlValue v=UI_CONTROL_VALUE_INIT;v.bool_value=false;t.set(ctx,t.user_data,&v);save.on_pressed(ctx,save.user_data);gui::open();check(io::operations==n&&gui::unregistrations==1&&gui::closes==1);u.shutdown();check(gui::unregistrations==1);});
 run("ui_cleanup_failure_detach_fallback",[]{editor7::Ui u;u.initialize(ctx,&io::host,&io::files,&gui::api);gui::open();gui::fail_remove=true;check(u.shutdown()==MOD_ERROR&&!u.alive&&!u.editor.alive);});
 run("missing_stays_zero_until_explicit_save",[]{editor7::Editor e;init(e);check(e.saved.kind==editor7::Kind::Missing&&!e.draft[0]&&!e.draft[1]&&io::writes==0&&io::creates==0);e.toggle(0,true);check(!fs::exists(fs::path(io::folder)/"placements.json"));check(e.save());verify_flags(true,false);});
 run("missing_explicit_disabled_creation",[]{editor7::Editor e;init(e);check(e.save());verify_flags(false,false);});
 run("invalid_json_never_overwritten",[]{io::put("placements.json","{bad");editor7::Editor e;init(e);e.toggle(0,true);check(!e.editable()&&!e.save()&&io::bytes("placements.json")=="{bad"&&io::writes==0);});
 run("empty_corrupt_file_read_only",[]{io::put("placements.json","");editor7::Editor e;init(e);check(!e.editable()&&!e.save()&&io::writes==0);});
 run("permission_error_read_only",[]{io::put("placements.json",io::both);io::inject("read_open","config");editor7::Editor e;init(e);check(!e.editable()&&!e.save()&&io::writes==0);});
 run("directory_list_failure_not_missing",[]{io::inject("list");editor7::Editor e;init(e);check(e.saved.kind==editor7::Kind::Error&&!e.editable());});
 run("directory_missing_end_rejected",[]{io::no_end=true;editor7::Editor e;init(e);check(!e.editable());});
 run("write_api_unavailable_preserves_file",[]{io::put("placements.json",io::both);io::files.flush=nullptr;editor7::Editor e;init(e);check(!e.editable()&&!e.save()&&io::bytes("placements.json")==io::both);});
 for(const auto* op:{"create","write_open","write","flush","write_close","read_open","read","read_corrupt","read_close","write_throw","open_throw","close_throw"}){
  const std::string name=std::string("backup_fault_")+op;
  run(name.c_str(),[&]{editor7::Editor e;load_both(e);e.toggle(1,false);io::inject(op,"backup");check(!e.save()&&io::bytes("placements.json")==io::both);});
 }
 for(const auto* op:{"write_open","write","flush","write_close","write_throw"}){
  const std::string name=std::string("config_fault_restore_")+op;
  run(name.c_str(),[&]{editor7::Editor e;load_both(e);e.toggle(1,false);io::inject(op,"config");check(!e.save()&&io::bytes("placements.json")==io::both&&io::bytes(e.store.backup_name)==io::both&&std::string(e.message)=="SAVE_FAILED_ORIGINAL_OR_DISABLED_RESTORED");});
 }
 run("config_readback_corrupt_restores",[]{editor7::Editor e;load_both(e);e.toggle(1,false);io::inject("read_corrupt","config",3);check(!e.save()&&io::bytes("placements.json")==io::both&&std::string(e.message)=="SAVE_FAILED_ORIGINAL_OR_DISABLED_RESTORED");});
 run("config_restore_failure_keeps_backup_and_locks",[]{editor7::Editor e;load_both(e);e.toggle(1,false);io::inject("write","config",1,true);check(!e.save()&&io::bytes(e.store.backup_name)==io::both&&!e.editable()&&std::string(e.message)=="SAVE_FAILED_MANUAL_RECOVERY_REQUIRED");});
 run("missing_failed_write_leaves_disabled",[]{editor7::Editor e;init(e);e.toggle(0,true);io::inject("write","config");check(!e.save());verify_flags(false,false);});
 run("external_change_before_save_rejected",[]{editor7::Editor e;load_both(e);e.toggle(1,false);io::put("placements.json",io::field);check(!e.save()&&io::bytes("placements.json")==io::field&&io::writes==0);});
 run("external_change_after_backup_rejected",[]{editor7::Editor e;load_both(e);e.toggle(1,false);io::change_after_backup=true;check(!e.save()&&io::bytes("placements.json")==io::field&&io::bytes(e.store.backup_name)==io::both);});
 run("backup_existing_never_overwritten",[]{editor7::Editor e;load_both(e);io::put("placements.cp07-backup-01.json","previous");e.toggle(1,false);check(e.save()&&io::bytes("placements.cp07-backup-01.json")=="previous"&&std::string(e.store.backup_name)=="placements.cp07-backup-02.json");});
 run("backup_capacity_fail_before_original_write",[]{editor7::Editor e;load_both(e);for(unsigned i=1;i<=32;++i){char n[64]{};std::snprintf(n,sizeof(n),"placements.cp07-backup-%02u.json",i);io::put(n,"old");}e.toggle(1,false);check(!e.save()&&io::bytes("placements.json")==io::both&&io::writes==0);});
 run("preserve_existing_custom_ids",[]{auto raw=io::both;auto p=raw.find("field_native");raw.replace(p,12,"custom_field");p=raw.find("north_native");raw.replace(p,12,"custom_north");io::put("placements.json",raw);editor7::Editor e;init(e);e.toggle(1,false);check(e.save());check(io::bytes("placements.json").find("custom_field")!=std::string::npos&&io::bytes("placements.json").find("custom_north")!=std::string::npos);});
 run("missing_site_default_id_collision_avoided",[]{auto raw=io::field;auto p=raw.find("field_native");raw.replace(p,12,"north_native");io::put("placements.json",raw);editor7::Editor e;init(e);e.toggle(1,true);check(e.save());verify_flags(true,true);});
 run("four_modes_serialization_and_restart",[]{for(const auto flags:{std::array<bool,2>{true,true},{true,false},{false,true},{false,false}}){io::put("placements.json",io::both);editor7::Editor e;init(e);e.toggle(0,flags[0]);e.toggle(1,flags[1]);check(e.save());editor7::Editor again;init(again);check(again.draft==flags);}});
 run("ui_invalid_save_disabled",[]{io::put("placements.json","bad");editor7::Ui u;u.initialize(ctx,&io::host,&io::files,&gui::api);gui::open();check(gui::controls[0].is_disabled(ctx,gui::controls[0].user_data)&&gui::controls[2].is_disabled(ctx,gui::controls[2].user_data));gui::button(2);check(io::writes==0);u.shutdown();});
 run("no_ui_write_on_shutdown",[]{io::put("placements.json",io::both);editor7::Ui u;u.initialize(ctx,&io::host,&io::files,&gui::api);gui::open();gui::toggle(0,false);const auto ops=io::operations;u.shutdown();check(io::operations==ops&&io::bytes("placements.json")==io::both);});
 std::printf("CP07_EDITOR_LAB_PASS cases=%u game_executed=false\n",cases);return 0;
 }catch(...){std::printf("CP07_EDITOR_LAB_FAIL completed=%u\n",cases);return 1;}
}
