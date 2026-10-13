#include <mods/svc/actor.h>
#include <type_traits>
#include <cstddef>
#include <iostream>
using Create=ModResult(*)(ModContext*,ProfileName,const ActorSpawnParams*,ActorId*);
using Room=ModResult(*)(ModContext*,ActorId,int8_t*);
static_assert(std::is_same_v<decltype(ActorService::create_actor),Create>);
static_assert(std::is_same_v<decltype(ActorService::get_actor_room_num),Room>);
static_assert(sizeof(ActorSpawnParams)==48&&sizeof(ActorService)==80&&sizeof(ServiceHeader)==8&&sizeof(ProfileName)==2&&sizeof(ActorId)==4&&sizeof(ModContext*)==8);
static_assert(offsetof(ActorService,create_actor)==32&&offsetof(ActorService,get_actor_room_num)==64);
static_assert(offsetof(ActorSpawnParams,position)==8&&offsetof(ActorSpawnParams,angle)==20&&offsetof(ActorSpawnParams,scale)==28&&offsetof(ActorSpawnParams,create_function)==40);
int main(){std::cout<<"PASS ABI actual official headers: ActorSpawnParams=48 ActorService=80 ServiceHeader=8 ProfileName=2 ActorId=4 ModContext*=8 opaque_ModContext_no_size_claim; create_offset=32 room_offset=64; C++20 Winx64; no native service called\n";}
