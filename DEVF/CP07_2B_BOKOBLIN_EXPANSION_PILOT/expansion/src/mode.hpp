#pragma once
#include "../../provider/src/config_core.hpp"
#include <mods/svc/host.h>
#include <mods/svc/file.h>
namespace expansion {
enum class Mode{None,One,Both};
inline bool decode_mode(std::string_view raw,Mode& out)noexcept{
 out=Mode::None;try{native_config::Parser p{raw};native_config::Node n;if(raw.size()>256||!p.parse(n)||!native_config::keys(n,{"schema_version","mode"})||!native_config::integer(native_config::get(n,"schema_version"),1,1))return false;
 const auto* m=native_config::get(n,"mode");if(native_config::text(m,"NONE"))return true;if(native_config::text(m,"A_ONLY")){out=Mode::One;return true;}if(native_config::text(m,"BOTH")){out=Mode::Both;return true;}return false;}catch(...){out=Mode::None;return false;}
}
template<class T>inline bool service(const T* s,unsigned major,unsigned minor){return s&&s->header.struct_size>=sizeof(T)&&s->header.major_version==major&&s->header.minor_version>=minor;}
inline ModResult load_mode(ModContext* ctx,const HostService* host,const FileService* files,Mode& out,const char*& reason)noexcept{
 out=Mode::None;reason="MODE_IO_UNAVAILABLE";if(!ctx||!service(host,2,2)||!service(files,1,0)||!host->data_dir||!files->join||!files->open||!files->size||!files->read||!files->close)return MOD_UNAVAILABLE;
 FileStreamHandle h=0;try{
  const char* folder=nullptr;if(host->data_dir(ctx,&folder)!=MOD_OK||!folder||!*folder)return MOD_UNAVAILABLE;std::string dir=folder;const char* rawpath=nullptr;
  const auto joined=files->join(ctx,dir.c_str(),"qa.json",&rawpath);if(joined==MOD_UNAVAILABLE){reason="MODE_MISSING_NONE_NO_REQUEST";return MOD_OK;}if(joined!=MOD_OK||!rawpath||!*rawpath)return MOD_ERROR;std::string path=rawpath;
  const auto opened=files->open(ctx,path.c_str(),FILE_OPEN_READ,&h);if(opened!=MOD_OK||!h){if(h){files->close(ctx,h);h=0;}return MOD_ERROR;}
  uint64_t size=0,n=0;bool good=files->size(ctx,h,&size)==MOD_OK&&size&&size<=256;std::string bytes;
  if(good){bytes.resize(size);good=files->read(ctx,h,bytes.data(),size,&n)==MOD_OK&&n==size;char c=0;uint64_t extra=0;if(good)good=files->read(ctx,h,&c,1,&extra)==MOD_OK&&!extra;}
  const auto closed=files->close(ctx,h);h=0;if(!good||closed!=MOD_OK){reason="MODE_READ_SIZE_OR_CLOSE_FAILED";return MOD_ERROR;}
  if(!decode_mode(bytes,out)){reason="MODE_INVALID_ZERO_REQUESTS";return MOD_INVALID_ARGUMENT;}reason="MODE_VALID_COLD_START";return MOD_OK;
 }catch(...){if(h){try{files->close(ctx,h);}catch(...){}}out=Mode::None;reason="MODE_IO_EXCEPTION_ZERO_REQUESTS";return MOD_ERROR;}
}
}
