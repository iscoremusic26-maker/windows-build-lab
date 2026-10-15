#pragma once
#include "config_core.hpp"
#include <mods/svc/host.h>
#include <mods/svc/file.h>
namespace cp6 {
inline bool load(ModContext* ctx,const HostService* host,const FileService* files,Config& out,Error& error){
 out=Config{};const char* dir=nullptr;const char* path=nullptr;FileStreamHandle stream=0;
 auto bad=[&](const char* why){error.code=why;out=Config{};return false;};
 if(!host||!files||!host->data_dir||!files->join||!files->open||!files->size||!files->read||!files->close)return bad("FILE_SERVICE_INCOMPLETE");
 try{
  if(host->data_dir(ctx,&dir)!=MOD_OK||!dir||!*dir)return bad("DATA_DIR_UNAVAILABLE");
  if(files->join(ctx,dir,"placements.json",&path)!=MOD_OK||!path)return bad("CONFIG_MISSING_OR_INACCESSIBLE");
  if(files->open(ctx,path,FILE_OPEN_READ,&stream)!=MOD_OK||!stream)return bad("CONFIG_OPEN_FAILED");
  std::uint64_t size=0,read=0;bool ok=files->size(ctx,stream,&size)==MOD_OK&&size&&size<=kMaxBytes;
  std::string raw;
  if(ok){raw.resize(static_cast<std::size_t>(size));ok=files->read(ctx,stream,raw.data(),size,&read)==MOD_OK&&read==size;
   // Detect growth as well as truncation; never read an unbounded file.
   char extra=0;std::uint64_t tail=0;if(ok)ok=files->read(ctx,stream,&extra,1,&tail)==MOD_OK&&tail==0;}
  const auto closed=files->close(ctx,stream);stream=0;
  if(!ok||closed!=MOD_OK)return bad("CONFIG_SIZE_READ_OR_CLOSE_FAILED");
  return decode(raw,out,error);
 }catch(...){if(stream){try{files->close(ctx,stream);}catch(...){}}return bad("CONFIG_IO_EXCEPTION");}
}
}
