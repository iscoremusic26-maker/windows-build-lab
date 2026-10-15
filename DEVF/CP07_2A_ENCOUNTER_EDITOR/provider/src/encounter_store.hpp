#pragma once
#include "encounter_core.hpp"
#include "persistence.hpp"
namespace encounter {
struct Disk {editor7::Kind kind=editor7::Kind::Error;std::string raw;Collection values;};
struct Store {
 editor7::Store io;bool exported=false;const char* message="NOT_STARTED";char backup[80]{};
 const char* filename()const{return exported?"encounters.export.json":"encounters.work.json";}
 struct Listing{
  const char* target;bool done=false,bad=false,exists=false;unsigned count=0;std::array<bool,32> slots{};bool exported;
  static void entry(ModContext*,const FileEntry* e,void* user)noexcept{
   auto& x=*static_cast<Listing*>(user);if(!e){if(x.done)x.bad=true;x.done=true;return;}
   if(x.done||++x.count>512||!e->name){x.bad=true;return;}
   if(editor7::ascii_equal(e->name,x.target)){if(x.exists||e->is_directory)x.bad=true;x.exists=true;}
   for(unsigned i=0;i<32;++i){char n[80];std::snprintf(n,sizeof(n),"encounters.%s.backup-%02u.json",x.exported?"export":"work",i+1);if(editor7::ascii_equal(e->name,n))x.slots[i]=true;}
  }
 };
 bool list(Listing& x){std::string dir;if(!io.dir(dir))return false;return io.files->list(io.ctx,dir.c_str(),Listing::entry,&x)==MOD_OK&&x.done&&!x.bad;}
 bool read(Disk& out)noexcept{
  out=Disk{};if(!io.available()){message="ENCOUNTER_IO_UNAVAILABLE";return false;}
  try{Listing l{filename(),false,false,false,0,{},exported};if(!list(l)){message="ENCOUNTER_LIST_FAILED";return false;}
   if(!l.exists){out.kind=editor7::Kind::Missing;message="ENCOUNTER_FILE_MISSING";return true;}
   if(!io.read_existing(filename(),out.raw)){message="ENCOUNTER_READ_FAILED";return false;}
   Error e;if(!decode(out.raw,out.values,exported,e)){out.kind=editor7::Kind::Invalid;message="ENCOUNTER_INVALID_READ_ONLY";return true;}
   out.kind=editor7::Kind::Valid;message="ENCOUNTER_FILE_VALID";return true;
  }catch(...){message="ENCOUNTER_READ_EXCEPTION";return false;}
 }
 static bool same(const Disk& a,const Disk& b){return a.kind==b.kind&&(a.kind==editor7::Kind::Missing||a.raw==b.raw);}
 bool verify(const char* name,const std::string& raw){
  if(!io.write_existing(name,raw))return false;std::string back;if(!io.read_existing(name,back)||back!=raw)return false;Collection values;Error e;return decode(back,values,exported,e);
 }
 bool save(Disk& baseline,const Collection& values)noexcept{
  if(!io.available()||(baseline.kind!=editor7::Kind::Valid&&baseline.kind!=editor7::Kind::Missing)){message="ENCOUNTER_SAVE_READ_ONLY";return false;}
  bool touched=false;const bool existed=baseline.kind==editor7::Kind::Valid;backup[0]=0;
  try{
   if(!collection_ok(values)){message="ENCOUNTER_VALUES_INVALID";return false;}
   const auto desired=serialize(values,exported);Collection check;Error e;if(!decode(desired,check,exported,e)){message="ENCOUNTER_SERIALIZE_REJECTED";return false;}
   Disk current;if(!read(current)||!same(current,baseline)){message="ENCOUNTER_EXTERNAL_CONFLICT";return false;}
   if(existed){
    Listing l{filename(),false,false,false,0,{},exported};if(!list(l)){message="ENCOUNTER_BACKUP_LIST_FAILED";return false;}
    unsigned slot=0;while(slot<32&&l.slots[slot])++slot;if(slot==32){message="ENCOUNTER_BACKUP_LIMIT";return false;}
    std::snprintf(backup,sizeof(backup),"encounters.%s.backup-%02u.json",exported?"export":"work",slot+1);
    if(!io.create(backup)||!verify(backup,baseline.raw)){message="ENCOUNTER_BACKUP_FAILED_NO_OVERWRITE";return false;}
    Disk again;if(!read(again)||!same(again,baseline)){message="ENCOUNTER_CONFLICT_AFTER_BACKUP";return false;}
   }else if(!io.create(filename())){message="ENCOUNTER_EXCLUSIVE_CREATE_FAILED";return false;}
   touched=true;
   if(!verify(filename(),desired))throw 1;
   baseline.kind=editor7::Kind::Valid;baseline.raw=desired;baseline.values=values;message=exported?"EXPORT_VERIFIED_FUTURE_COMPILATION_ONLY":"WORK_SAVED_VERIFIED_NO_SPAWN";return true;
  }catch(...){
   if(touched){bool restored=false;try{const auto old=existed?baseline.raw:serialize({},exported);restored=verify(filename(),old);if(restored&&!existed){baseline.kind=editor7::Kind::Valid;baseline.raw=old;baseline.values.clear();}}catch(...){}
    if(!restored)baseline.kind=editor7::Kind::Error;
    message=restored?"ENCOUNTER_SAVE_FAILED_PRIOR_OR_EMPTY_RESTORED":"ENCOUNTER_SAVE_FAILED_MANUAL_RECOVERY_REQUIRED";
   }else message="ENCOUNTER_SAVE_FAILED_NO_OVERWRITE";return false;
  }
 }
};
struct Session{
 Model model;Store work,output;Disk baseline,export_baseline;
 void initialize(ModContext* c,const HostService* h,const FileService* file)noexcept{
  for(auto* s:{&work,&output}){s->io.ctx=c;s->io.host=h;s->io.files=file;}output.exported=true;
  work.read(baseline);output.read(export_baseline);model.alive=true;
  if(baseline.kind==editor7::Kind::Valid)model.saved=baseline.values;model.working=model.saved;model.fresh();model.message=work.message;
 }
 bool writable()const{return model.alive&&!model.busy&&work.io.available()&&(baseline.kind==editor7::Kind::Valid||baseline.kind==editor7::Kind::Missing);}
 bool save()noexcept{if(!writable()){model.message="WORK_READ_ONLY";return false;}model.busy=true;const bool ok=work.save(baseline,model.working);if(ok)model.saved=baseline.values;model.message=work.message;model.busy=false;return ok;}
 bool export_file()noexcept{if(!writable()){model.message="WORK_READ_ONLY";return false;}model.busy=true;const bool ok=output.save(export_baseline,model.working);model.message=output.message;model.busy=false;return ok;}
 void shutdown()noexcept{model.alive=false;model.busy=false;}
};
}
