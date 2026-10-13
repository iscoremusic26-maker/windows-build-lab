#pragma once
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <mods/svc/game.h>
#include <mods/svc/actor.h>
#include <cstring>
namespace devf::package {
template<class T> bool compatible(const T* p,unsigned major,unsigned minor) noexcept {
 return p&&p->header.struct_size>=sizeof(T)&&p->header.major_version==major&&p->header.minor_version>=minor;
}
inline bool contracts_available(const HostService* h,const LogService* l,const GameService* g,const ActorService* a) noexcept {
 return compatible(h,2,2)&&h->version&&(!std::strcmp(h->version,"2.0.3")||!std::strcmp(h->version,"2.0.3.0"))&&
 compatible(l,1,0)&&l->write&&compatible(g,2,0)&&compatible(a,1,0)&&a->create_actor&&a->get_actor_room_num;
}
}
