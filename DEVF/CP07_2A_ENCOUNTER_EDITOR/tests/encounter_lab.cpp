// Reuse unchanged real File/UI adapters and assertions from editor55; do not invoke its main.
#define main inherited_editor_suite_main
#include "../provider/tests/editor_lab.cpp"
#undef main
#include "../provider/src/encounter_ui.hpp"
#include <iostream>
#include <limits>
namespace encounter {
Sample native_value;bool native_ok=true;unsigned native_calls=0;
bool read_native(uint64_t tick,Sample& out,const char*& reason)noexcept{++native_calls;out=native_value;out.tick=tick;reason=native_ok?"LAB_NORMALIZED_NATIVE_INPUT":"LAB_NATIVE_UNAVAILABLE";return native_ok;}
}
namespace extra{
std::string operation,target;bool persistent=false;unsigned hit=0,nth=1;UiTabDesc tabs[2]{};UiListDesc listing{};std::vector<std::string> names;
bool fail(const char* op,const std::string& n){return operation==op&&(target.empty()||n==target||target=="backup"&&n.find(".backup-")!=std::string::npos)&&++hit>=nth&&(persistent||hit==nth);}
ModResult open(ModContext* c,const char* p,FileOpenMode m,FileStreamHandle* out){if(fail(m==FILE_OPEN_READ?"read_open":"write_open",fs::path(p).filename().string()))return MOD_ERROR;return io::open(c,p,m,out);}
ModResult write(ModContext* c,FileStreamHandle h,const void* p,uint64_t n){const auto name=io::streams.at(h)->name;if(fail("write",name)){io::streams.at(h)->file.write(static_cast<const char*>(p),std::streamsize(n/2));return MOD_ERROR;}return io::write(c,h,p,n);}
ModResult flush(ModContext* c,FileStreamHandle h){if(fail("flush",io::streams.at(h)->name))return MOD_ERROR;return io::flush(c,h);}
ModResult close(ModContext* c,FileStreamHandle h){const bool bad=fail("close",io::streams.at(h)->name);const auto r=io::close(c,h);return bad?MOD_ERROR:r;}
ModResult read(ModContext* c,FileStreamHandle h,void* p,uint64_t n,uint64_t* got){if(fail("read",io::streams.at(h)->name))return MOD_ERROR;return io::read(c,h,p,n,got);}
ModResult create(ModContext* c,const char* p,const char* n,const char** out){if(fail("create",n))return MOD_ERROR;return io::create(c,p,n,out);}
ModResult push(ModContext* c,const UiWindowDesc* w,UiWindowHandle* h){check(w->tab_count==2);tabs[0]=w->tabs[0];tabs[1]=w->tabs[1];return gui::push(c,w,h);}
ModResult list(ModContext*,UiElementHandle,const UiListDesc* d,UiListHandle* h){listing=*d;*h=900;return MOD_OK;}
ModResult items(ModContext*,UiListHandle h,const UiListItem* p,size_t n){check(h==900);names.clear();for(size_t i=0;i<n;++i){check(p[i].key==i+1);names.push_back(p[i].label);}return MOD_OK;}
void reset(){operation.clear();target.clear();persistent=false;hit=0;nth=1;listing={};names.clear();io::files.open=open;io::files.write=write;io::files.flush=flush;io::files.close=close;io::files.read=read;io::files.create_child=create;gui::api.window_push=push;gui::api.pane_add_list=list;gui::api.list_set_items=items;encounter::native_ok=true;encounter::native_calls=0;encounter::native_value={"F_SP121",3,6,0,-1234.,-6300.,60800.,1,"link_readonly_v203",true};}
}
encounter::Entry entry(unsigned n=1){encounter::Entry e;e.id="enc-"+std::to_string(n);e.name=n==1?"Bosque \"A\" \\ prueba":"Lugar "+std::to_string(n);e.sample=encounter::native_value;e.sample.tick=2;e.yaw=-100;e.quantity=2;e.spacing=200;return e;}
template<class F>void enc(const char* name,F fn){run(name,[&]{extra::reset();fn();});}
void session(encounter::Session& s){s.initialize(ctx,&io::host,&io::files);}
void collection_file(bool exported=false){auto c=encounter::Collection{entry()};io::put(exported?"encounters.export.json":"encounters.work.json",encounter::serialize(c,exported));}
void make_ui(encounter::Ui& u){check(u.initialize(ctx,&io::host,&io::files,&gui::api));gui::open();gui::controls.clear();ModError err{};check(extra::tabs[1].build(ctx,2,100,101,extra::tabs[1].user_data,&err)==MOD_OK);check(u.active);}
int main(int argc,char** argv){
 if(argc!=3)return 2;io::folder=argv[2];io::both=[](const fs::path& p){std::ifstream in(p,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});}(fs::path(argv[1])/"both-enabled.json");
 enc("collection-empty",[&]{encounter::Collection out;native_config::Error er;check(encounter::decode(encounter::serialize({}),out,false,er)&&out.empty());});
 enc("multi-stage-roundtrip",[&]{auto a=entry(),b=entry(2);b.sample.stage="F_SP118";b.sample.room=1;b.sample.layer=0;b.sample.x=999;encounter::Collection c{a,b},o;native_config::Error e;auto raw=encounter::serialize(c,true);check(encounter::decode(raw,o,true,e)&&o.size()==2&&o[1].sample.stage=="F_SP118"&&encounter::serialize(o,true)==raw);});
 enc("utf8-escapes",[&]{auto e=entry();e.name="Encuentro \"árbol\" \\ 日本";encounter::Collection c{e},o;native_config::Error er;check(encounter::decode(encounter::serialize(c,true),o,true,er)&&o[0].name==e.name);});
 for(const auto* bad:{"\xC0\x80","\xED\xA0\x80","\xF4\x90\x80\x80","a\nb"})enc("utf8-reject",[&]{auto e=entry();e.name=bad;check(!encounter::entry_ok(e));});
 enc("unicode-escape-input",[&]{auto raw=encounter::serialize({entry()},true);auto p=raw.find("Bosque");raw.replace(p,6,"\\u00c1rbol");encounter::Collection o;native_config::Error e;check(encounter::decode(raw,o,true,e));});
 enc("duplicate-name",[&]{auto a=entry(),b=entry(2);b.name=a.name;check(!encounter::collection_ok({a,b}));});
 enc("duplicate-id",[&]{auto a=entry(),b=entry(2);b.id=a.id;check(!encounter::collection_ok({a,b}));});
 enc("unknown-document-no-placements-schema",[&]{encounter::Collection o;native_config::Error e;check(!encounter::decode(io::both,o,false,e));});
 enc("unknown-key",[&]{auto raw=encounter::serialize({entry()},true);raw.insert(1,"\"unknown\":0,");encounter::Collection o;native_config::Error e;check(!encounter::decode(raw,o,true,e));});
 enc("duplicate-key",[&]{auto raw=encounter::serialize({entry()},true);raw.insert(1,"\"schema_version\":1,");encounter::Collection o;native_config::Error e;check(!encounter::decode(raw,o,true,e));});
 enc("BOM",[&]{encounter::Collection o;native_config::Error e;check(!encounter::decode("\xEF\xBB\xBF"+encounter::serialize({},true),o,true,e));});
 enc("oversize",[&]{encounter::Collection o;native_config::Error e;check(!encounter::decode(std::string(16385,' '),o,true,e));});
 enc("quantity-limits",[&]{auto e=entry();e.quantity=0;check(!encounter::entry_ok(e));e.quantity=5;check(!encounter::entry_ok(e));e.quantity=4;check(encounter::entry_ok(e));});
 enc("max-16-collection",[&]{encounter::Collection c;for(unsigned n=1;n<=16;++n)c.push_back(entry(n));check(encounter::collection_ok(c));encounter::Collection out;native_config::Error e;check(encounter::decode(encounter::serialize(c),out,false,e));c.push_back(entry(17));check(!encounter::collection_ok(c));});
 for(double n:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),1000001.})enc("nonfinite-range",[&]{auto e=entry();e.sample.x=n;check(!encounter::entry_ok(e));});
 enc("stage-traversal",[&]{auto e=entry();e.sample.stage="../";check(!encounter::entry_ok(e));e.id="../bad";check(!encounter::entry_ok(e));});
 enc("room-layer-yaw",[&]{auto e=entry();e.sample.room=64;check(!encounter::entry_ok(e));e=entry();e.sample.layer=16;check(!encounter::entry_ok(e));e=entry();e.yaw=32768;check(!encounter::entry_ok(e));});
 enc("no-fake-fixed-capture",[&]{auto e=entry();e.sample.origin="FIELD";check(!encounter::entry_ok(e));e=entry();e.sample.valid=false;check(!encounter::entry_ok(e));});
 for(unsigned k=0;k<6;++k)enc("capture-negative-gate",[&]{auto s=entry().sample;encounter::CaptureCheck g{true,false,false,true,1,2};if(k==0)g.same_scene=false;if(k==1)g.transition=true;if(k==2)g.event=true;if(k==3)g.member_ready=false;if(k==4)g.requested_tick=3;if(k==5)g.read_tick=4;check(!encounter::capture_gate(s,g));});
 enc("capture-fresh",[&]{auto s=entry().sample;check(encounter::capture_gate(s,{true,false,false,true,1,2}));});
 enc("model-add-edit-select-discard-delete",[&]{encounter::Session s;session(s);s.model.draft=entry();check(s.model.apply());s.model.fresh();s.model.draft=entry(2);check(s.model.apply()&&s.model.working.size()==2);s.model.select(0);s.model.draft.quantity=4;s.model.discard_draft();check(s.model.draft.quantity==2);s.model.draft.quantity=3;check(s.model.apply()&&s.model.working[0].quantity==3);check(!s.model.erase());s.model.delete_confirmed=true;check(s.model.erase()&&s.model.working.size()==1);s.shutdown();});
 enc("save-restart-export-multiple",[&]{encounter::Session s;session(s);s.model.working={entry(),entry(2)};check(s.save());check(s.export_file());encounter::Session next;session(next);check(next.model.working.size()==2);encounter::Collection c;native_config::Error e;check(encounter::decode(io::bytes("encounters.export.json"),c,true,e)&&c.size()==2);check(!fs::exists(fs::path(io::folder)/"placements.json"));});
 enc("work-invalid-read-only",[&]{io::put("encounters.work.json","bad");encounter::Session s;session(s);check(!s.writable()&&!s.save()&&!s.export_file());check(io::bytes("encounters.work.json")=="bad");});
 enc("export-invalid-not-overwritten",[&]{io::put("encounters.export.json","bad");encounter::Session s;session(s);s.model.working={entry()};check(!s.export_file());check(io::bytes("encounters.export.json")=="bad");});
 enc("external-conflict",[&]{collection_file();encounter::Session s;session(s);io::put("encounters.work.json",encounter::serialize({entry(2)}));check(!s.save());check(io::bytes("encounters.work.json")==encounter::serialize({entry(2)}));});
 for(const auto* op:{"create","write","flush","close","read","write_open","read_open"})enc("backup-failure-no-overwrite",[&]{collection_file();auto prior=io::bytes("encounters.work.json");encounter::Session s;session(s);s.model.working={entry(2)};extra::operation=op;extra::target="backup";check(!s.save());check(io::bytes("encounters.work.json")==prior);});
 for(const auto* op:{"write","flush","close","read","write_open"})enc("target-failure-restores-backup",[&]{collection_file();auto prior=io::bytes("encounters.work.json");encounter::Session s;session(s);s.model.working={entry(2)};extra::operation=op;extra::target="encounters.work.json";if(std::string(op)=="read")extra::nth=5;else if(std::string(op)=="close")extra::nth=3;check(!s.save());check(io::bytes("encounters.work.json")==prior);check(fs::exists(fs::path(io::folder)/"encounters.work.backup-01.json"));});
 enc("persistent-failure-keeps-backup",[&]{collection_file();auto prior=io::bytes("encounters.work.json");encounter::Session s;session(s);s.model.working={entry(2)};extra::operation="write";extra::target="encounters.work.json";extra::persistent=true;check(!s.save());check(io::bytes("encounters.work.backup-01.json")==prior&&!s.writable());});
 enc("exclusive-backup-slots",[&]{collection_file();encounter::Session s;session(s);for(unsigned i=1;i<=32;++i){char n[80];std::snprintf(n,sizeof(n),"encounters.work.backup-%02u.json",i);io::put(n,"reserved");}auto prior=io::bytes("encounters.work.json");check(!s.save()&&io::bytes("encounters.work.json")==prior);});
 enc("missing-io-service",[&]{io::files.flush=nullptr;encounter::Session s;session(s);check(!s.writable()&&!s.save());});
 enc("editor-ui-capture-is-deferred",[&]{encounter::Ui u;make_ui(u);check(gui::controls.size()==16);gui::button(8);check(u.pending&&encounter::native_calls==0);u.update();check(encounter::native_calls==1&&u.session.model.draft.sample.valid);u.shutdown();});
 enc("ui-null-native",[&]{encounter::Ui u;make_ui(u);encounter::native_ok=false;gui::button(8);u.update();check(!u.session.model.draft.sample.valid);u.shutdown();});
 enc("ui-close-cancels-capture",[&]{encounter::Ui u;make_ui(u);gui::button(8);gui::close(ctx,2);u.update();check(encounter::native_calls==0&&!u.pending);u.shutdown();});
 enc("ui-edit-save-export-select-delete",[&]{encounter::Ui u;make_ui(u);gui::button(8);u.update();gui::button(10);check(u.session.model.working.size()==1&&extra::names.size()==1);gui::button(13);check(fs::exists(fs::path(io::folder)/"encounters.work.json"));gui::button(15);check(fs::exists(fs::path(io::folder)/"encounters.export.json"));extra::listing.on_pressed(ctx,900,1,extra::listing.user_data);gui::toggle(6,true);gui::button(12);check(u.session.model.working.empty());gui::button(14);check(u.session.model.working.size()==1);u.shutdown();});
 enc("ui-reopen-keeps-draft",[&]{encounter::Ui u;make_ui(u);u.session.model.draft.name="Pendiente";gui::close(ctx,2);gui::open();gui::controls.clear();ModError e{};extra::tabs[1].build(ctx,2,100,101,extra::tabs[1].user_data,&e);check(u.session.model.draft.name=="Pendiente");u.shutdown();});
 enc("ui-service-absent-core-independent",[&]{encounter::Ui u;check(!u.initialize(ctx,&io::host,&io::files,nullptr));check(!u.legacy.alive);});
 enc("no-io-update-without-click",[&]{encounter::Ui u;make_ui(u);auto n=io::operations;for(unsigned i=0;i<100;++i)u.update();check(io::operations==n&&encounter::native_calls==0);u.shutdown();});
 enc("ui-new-draft-cancels-pending",[&]{encounter::Ui u;make_ui(u);gui::button(8);gui::button(9);u.update();check(encounter::native_calls==0&&!u.pending);u.shutdown();});
 enc("ui-metadata-edit-cancels-pending",[&]{encounter::Ui u;make_ui(u);gui::button(8);UiControlValue v=UI_CONTROL_VALUE_INIT;v.int_value=3;gui::controls[2].set(ctx,gui::controls[2].user_data,&v);u.update();check(encounter::native_calls==0&&u.session.model.draft.quantity==3);u.shutdown();});
 enc("draft-capture-copy-preserves-other-entries",[&]{encounter::Session s;session(s);s.model.working={entry(),entry(2)};auto prior=encounter::serialize(s.model.working);s.model.select(0);auto v=entry().sample;v.x=77;check(s.model.capture(v));check(encounter::serialize(s.model.working)==prior);s.model.discard_draft();check(s.model.draft.sample.x!=77);});
 enc("export-external-conflict-preserves-file",[&]{collection_file(true);encounter::Session s;session(s);s.model.working={entry()};auto changed=encounter::serialize({entry(2)},true);io::put("encounters.export.json",changed);check(!s.export_file()&&io::bytes("encounters.export.json")==changed);});
 enc("independent-work-export-backups",[&]{encounter::Session s;session(s);s.model.working={entry()};check(s.save()&&s.export_file());s.model.working.push_back(entry(2));check(s.save()&&s.export_file());check(fs::exists(fs::path(io::folder)/"encounters.work.backup-01.json")&&fs::exists(fs::path(io::folder)/"encounters.export.backup-01.json"));});
 std::cout<<"ENCOUNTER_LAB_PASS cases="<<cases<<"\n";return 0;
}
