#pragma once
// CP10.2 FIX01: per-instance texture binding. The same logic runs on the real J3DMatPacket (mod.cpp) and on
// fake packets (tests/texture_lab.cpp). Only the two BODY material packets of ONE model are re-pointed to a
// private texture; the eye packet, the shared J3DModelData and every other model keep the original texture.
#include <initializer_list>
namespace devf_bind {
template<class Packet,class Texture,class Get,class Set>
const char* bind_body(Packet* body0,Packet* body1,Packet* eye,Texture* original,Texture* priv,Get get,Set set) noexcept {
 if(!body0||!body1||!eye||!original||!priv||priv==original)return "BIND_ARGUMENTS";
 if(body0==body1||body0==eye||body1==eye)return "BIND_PACKET_ALIAS";
 if(get(body0)!=original||get(body1)!=original||get(eye)!=original)return "MAT_PACKET_TEXTURE_NOT_NATIVE";
 set(body0,priv);set(body1,priv);
 return nullptr;
}
// Private-texture cache lookup: an entry is reused only for the SAME original atlas pointer AND content hash.
template<class Entry>
int find_cached(const Entry* entries,unsigned count,const void* source,unsigned long long hash) noexcept {
 for(unsigned i=0;i<count;++i)if(entries[i].source==source&&entries[i].source_hash==hash&&entries[i].texture)return static_cast<int>(i);
 return -1;
}
// Restores only packets that still point to our private texture; returns how many were restored.
template<class Packet,class Texture,class Get,class Set>
int restore_body(Packet* body0,Packet* body1,Texture* original,Texture* priv,Get get,Set set) noexcept {
 int n=0;
 for(Packet* p:{body0,body1})if(p&&original&&priv&&get(p)==priv){set(p,original);++n;}
 return n;
}
}
