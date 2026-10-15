#pragma once
#include "runtime.hpp"
#include <cstdio>
namespace editor7 {
using native_config::Config;
enum class Kind {Valid,Missing,Invalid,Error};
struct Snapshot {Kind kind=Kind::Error;Config config;std::string raw;};
inline bool ascii_equal(const char* a,const char* b) noexcept {
 if(!a||!b)return false;
 for(;*a&&*b;++a,++b){char x=*a,y=*b;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return false;}
 return !*a&&!*b;
}
struct Inventory {
 bool done=false,bad=false,config=false;unsigned count=0;std::array<bool,32> backups{};
 static void entry(ModContext*,const FileEntry* e,void* data) noexcept {
  auto& x=*static_cast<Inventory*>(data);
  if(!e){if(x.done)x.bad=true;x.done=true;return;}
  if(x.done||++x.count>256||!e->name){x.bad=true;return;}
  if(ascii_equal(e->name,"placements.json")){if(x.config||e->is_directory)x.bad=true;x.config=true;}
  for(unsigned i=0;i<32;++i){char n[64]{};std::snprintf(n,sizeof(n),"placements.cp07-backup-%02u.json",i+1);if(ascii_equal(e->name,n))x.backups[i]=true;}
 }
};
struct Store {
 ModContext* ctx=nullptr;const HostService* host=nullptr;const FileService* files=nullptr;
 const char* reason="NOT_STARTED";const char* detail="NONE";char backup_name[64]{};
 bool available() const noexcept {
  return ctx&&native_qa::compatible(host,2,2)&&native_qa::compatible(files,1,0)&&host->data_dir&&
   files->list&&files->join&&files->create_child&&files->open&&files->size&&files->read&&files->write&&files->flush&&files->close;
 }
 bool fail(const char* why) noexcept {detail=why;return false;}
 bool dir(std::string& out){
  const char* p=nullptr;if(host->data_dir(ctx,&p)!=MOD_OK||!p||!*p)return fail("DATA_DIR_FAILED");out=p;return true;
 }
 bool inventory(Inventory& x){
  std::string folder;if(!dir(folder))return false;
  if(files->list(ctx,folder.c_str(),Inventory::entry,&x)!=MOD_OK||!x.done||x.bad)return fail("FILE_LIST_FAILED");
  return true;
 }
 bool location(const char* name,std::string& out){
  std::string folder;if(!dir(folder))return false;const char* p=nullptr;
  if(files->join(ctx,folder.c_str(),name,&p)!=MOD_OK||!p||!*p)return fail("FILE_JOIN_FAILED");out=p;return true;
 }
 bool read_existing(const char* name,std::string& raw){
  raw.clear();std::string path;if(!location(name,path))return false;FileStreamHandle h=0;
  try{
   const auto opened=files->open(ctx,path.c_str(),FILE_OPEN_READ,&h);
   if(opened!=MOD_OK||!h){if(h){files->close(ctx,h);h=0;}return fail("FILE_READ_OPEN_FAILED");}
   uint64_t size=0,n=0;
   bool ok=files->size(ctx,h,&size)==MOD_OK&&size&&size<=native_config::kMaxBytes;
   if(ok){raw.resize(size);ok=files->read(ctx,h,raw.data(),size,&n)==MOD_OK&&n==size;char tail=0;uint64_t extra=0;
    if(ok)ok=files->read(ctx,h,&tail,1,&extra)==MOD_OK&&extra==0;}
   const auto close=files->close(ctx,h);h=0;
   if(!ok||close!=MOD_OK){raw.clear();return fail("FILE_READ_SIZE_OR_CLOSE_FAILED");}
   return true;
  }catch(...){if(h){try{files->close(ctx,h);}catch(...){}}raw.clear();return fail("FILE_READ_EXCEPTION");}
 }
 bool current(Snapshot& out) noexcept {
  out=Snapshot{};
  if(!available()){reason="PERSISTENCE_SERVICE_UNAVAILABLE";return false;}
  try{
   Inventory inv;if(!inventory(inv)){reason="CONFIG_ACCESS_FAILED";return false;}
   if(!inv.config){out.kind=Kind::Missing;reason="CONFIG_MISSING_NO_DEFAULT_SPAWN";return true;}
   if(!read_existing("placements.json",out.raw)){reason="CONFIG_READ_FAILED";return false;}
   native_config::Error error;
   if(!native_config::decode(out.raw,out.config,error)){out.kind=Kind::Invalid;reason="CONFIG_INVALID_NOT_OVERWRITTEN";detail=error.code;return true;}
   out.kind=Kind::Valid;reason="CONFIG_VALID";detail="NONE";return true;
  }catch(...){reason="CONFIG_SNAPSHOT_EXCEPTION";return false;}
 }
 bool create(const char* name){
  std::string folder;if(!dir(folder))return false;const char* p=nullptr;
  if(files->create_child(ctx,folder.c_str(),name,&p)!=MOD_OK||!p||!*p)return fail("FILE_CREATE_FAILED");
  return true; // exclusive, never replace an existing child; re-join for the next operation
 }
 bool write_existing(const char* name,const std::string& raw){
  std::string path;if(!location(name,path))return false;FileStreamHandle h=0;
  try{
   const auto opened=files->open(ctx,path.c_str(),FILE_OPEN_TRUNCATE,&h);
   if(opened!=MOD_OK||!h){if(h){files->close(ctx,h);h=0;}return fail("FILE_WRITE_OPEN_FAILED");}
   const bool wrote=files->write(ctx,h,raw.data(),raw.size())==MOD_OK;
   const bool flushed=files->flush(ctx,h)==MOD_OK;
   const bool closed=files->close(ctx,h)==MOD_OK;h=0;
   if(!wrote)return fail("FILE_WRITE_FAILED");if(!flushed)return fail("FILE_FLUSH_FAILED");if(!closed)return fail("FILE_CLOSE_FAILED");
   return true;
  }catch(...){if(h){try{files->close(ctx,h);}catch(...){}}return fail("FILE_WRITE_EXCEPTION");}
 }
 bool write_verified(const char* name,const std::string& raw){
  if(!write_existing(name,raw))return false;
  std::string back;if(!read_existing(name,back)||back!=raw)return fail("FILE_READBACK_MISMATCH");
  Config c;native_config::Error e;if(!native_config::decode(back,c,e))return fail("FILE_READBACK_INVALID");
  return true;
 }
 bool backup(const std::string& raw){
  Inventory inv;if(!inventory(inv))return false;
  unsigned slot=0;while(slot<32&&inv.backups[slot])++slot;
  if(slot==32)return fail("BACKUP_SLOTS_FULL");
  std::snprintf(backup_name,sizeof(backup_name),"placements.cp07-backup-%02u.json",slot+1);
  if(!create(backup_name))return false;
  return write_verified(backup_name,raw);
 }
 static bool same(const Snapshot& a,const Snapshot& b){return a.kind==b.kind&&(a.kind==Kind::Missing||a.raw==b.raw);}
 bool save(Snapshot& baseline,const std::string& desired) noexcept {
  reason="SAVE_NOT_STARTED";detail="NONE";backup_name[0]=0;
  if(!available()){reason="PERSISTENCE_SERVICE_UNAVAILABLE";return false;}
  if(baseline.kind!=Kind::Valid&&baseline.kind!=Kind::Missing){reason="INVALID_CONFIG_READ_ONLY";return false;}
  bool target_may_change=false;const bool existed=baseline.kind==Kind::Valid;
  try{
   Config validated;native_config::Error error;
   if(!native_config::decode(desired,validated,error)){reason="SERIALIZED_CONFIG_INVALID";detail=error.code;return false;}
   Snapshot now;
   if(!current(now)||!same(now,baseline)){reason="CONFIG_CHANGED_OR_UNREADABLE";return false;}
   if(existed){
    if(!backup(baseline.raw)){reason="BACKUP_NOT_VERIFIED_NO_OVERWRITE";return false;}
    Snapshot again;if(!current(again)||!same(again,baseline)){reason="CONFIG_CHANGED_AFTER_BACKUP";return false;}
   }else if(!create("placements.json")){reason="CONFIG_CREATE_FAILED";return false;}
   target_may_change=true; // truncate/open can fail after touching a file
   if(write_verified("placements.json",desired)){
    baseline.kind=Kind::Valid;baseline.raw=desired;baseline.config=std::move(validated);
    reason="SAVED_VERIFIED_RESTART_REQUIRED";detail="NONE";return true;
   }
  }catch(...){detail="SAVE_EXCEPTION";}
  const char* original_error=detail;
  if(target_may_change){
   bool restored=false;
   try{
    if(existed)restored=write_verified("placements.json",baseline.raw);
    else {
     // No remove API: on failed explicit creation, try to leave a valid disabled file.
     const std::string empty=R"json({"schema_version":1,"variants":[{"variant_id":"native_elite","enemy":"E_OC2","native_profile":510,"hp":220,"appearance":"native","ai":"native"}],"placements":[]})json";
     restored=write_verified("placements.json",empty);
     if(restored){baseline.kind=Kind::Valid;baseline.raw=empty;native_config::Error e;native_config::decode(empty,baseline.config,e);}
    }
   }catch(...){}
   reason=restored?"SAVE_FAILED_ORIGINAL_OR_DISABLED_RESTORED":"SAVE_FAILED_MANUAL_RECOVERY_REQUIRED";
   if(!restored)baseline.kind=Kind::Error;
  }else reason="SAVE_FAILED_NO_OVERWRITE";
  detail=original_error;return false;
 }
};
inline std::string serialize(const Config& prior,const std::array<bool,2>& flags){
 std::array<std::string,2> ids;
 for(unsigned i=0;i<prior.count;++i)ids[prior.placements[i].north?1:0]=prior.placements[i].id;
 for(unsigned i=0;i<2;++i)if(ids[i].empty()){
  ids[i]=i?"north_native":"field_native";
  if(ids[i]==ids[1-i])ids[i]=i?"cp07_north":"cp07_field";
 }
 std::string out="{\n  \"schema_version\": 1,\n  \"variants\": [{\"variant_id\":\"native_elite\",\"enemy\":\"E_OC2\",\"native_profile\":510,\"hp\":220,\"appearance\":\"native\",\"ai\":\"native\"}],\n  \"placements\": [\n";
 for(unsigned i=0;i<2;++i){
  out+="    {\"placement_id\":\""+ids[i]+"\",\"variant_id\":\"native_elite\",\"enabled\":"+(flags[i]?"true":"false")+
   ",\"stage\":\"F_SP121\",\"room\":3,\"layer\":6,\"x\":-1250,\"y\":"+(i?"-6322.6005859375":"-6304.00830078125")+
   ",\"z\":"+(i?"61950":"60750")+",\"yaw_s16\":0,\"parameters\":4294902016}"+(i==0?",\n":"\n");
 }
 out+="  ]\n}\n";return out;
}
struct Editor {
 Store store;Snapshot saved;std::array<bool,2> draft{};bool initialized=false,busy=false,alive=false;
 const char* message="EDITOR_NOT_INITIALIZED";
 void initialize(ModContext* c,const HostService* h,const FileService* f) noexcept {
  if(initialized)return;initialized=alive=true;store.ctx=c;store.host=h;store.files=f;
  store.current(saved);draft=saved.kind==Kind::Valid?saved.config.active():std::array<bool,2>{};message=store.reason;
 }
 bool editable() const noexcept {return alive&&!busy&&store.available()&&(saved.kind==Kind::Valid||saved.kind==Kind::Missing);}
 bool dirty() const {return draft!=(saved.kind==Kind::Valid?saved.config.active():std::array<bool,2>{});}
 void toggle(unsigned site,bool on) noexcept {if(site<2&&editable()){draft[site]=on;message="DRAFT_ONLY_RESTART_AFTER_SAVE";}}
 void discard() noexcept {if(alive&&!busy){draft=saved.kind==Kind::Valid?saved.config.active():std::array<bool,2>{};message="DRAFT_DISCARDED";}}
 bool save() noexcept {
  if(!editable()){message="SAVE_DISABLED";return false;}busy=true;bool ok=false;
  try{const auto bytes=serialize(saved.config,draft);ok=store.save(saved,bytes);}catch(...){store.reason="SERIALIZE_OR_SAVE_EXCEPTION";}
  if(ok)draft=saved.config.active();message=store.reason;busy=false;return ok;
 }
 void shutdown() noexcept {alive=false;}
};
}
