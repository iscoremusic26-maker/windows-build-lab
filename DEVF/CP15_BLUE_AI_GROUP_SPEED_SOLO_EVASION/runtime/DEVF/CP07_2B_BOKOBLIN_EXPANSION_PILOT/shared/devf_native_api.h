#pragma once
#include <mods/api.h>
#ifdef __cplusplus
#include <mods/service.hpp>
extern "C" {
#endif
#define DEVF_NATIVE_SERVICE_ID "dev.devf.native_placement"
#define DEVF_NATIVE_MAJOR 1u
#define DEVF_NATIVE_MINOR 0u
#define DEVF_SITE_FIELD 0u
#define DEVF_SITE_NORTH 1u
/* Caller supplies its own SDK context, on game thread, during bootstrap before
 * provider's first update. Context attribution is the SDK contract, not authentication
 * against hostile native code. No actor pointer/ActorId is exposed. */
typedef struct DevfNativePlacementRequest {
 uint32_t struct_size;
 uint32_t native_profile;
 char placement_id[32]; /* ASCII id, 1..31 chars, NUL terminated */
 char stage[8]; /* "F_SP121" */
 int32_t room;
 int32_t layer;
 float x,y,z;
 int32_t yaw_s16;
 uint32_t parameters;
 uint32_t reserved; /* zero */
} DevfNativePlacementRequest;
typedef struct DevfNativePlacementReceipt {
 uint32_t struct_size;
 uint32_t site;
 uint64_t placement_token; /* provider lifetime ticket, NOT StageActorHandle/ActorId */
 uint32_t registered_edits; /* 1 on acceptance, never proof of live actor */
 uint32_t reserved;
} DevfNativePlacementReceipt;
typedef struct DevfNativePlacementService {
 ServiceHeader header;
 ModResult (*request_placement)(ModContext* caller,const DevfNativePlacementRequest* request,DevfNativePlacementReceipt* out_receipt);
 ModResult (*remove_placement)(ModContext* caller,uint64_t placement_token);
} DevfNativePlacementService;
static_assert(sizeof(DevfNativePlacementRequest)==80,"Native request ABI");
static_assert(sizeof(DevfNativePlacementReceipt)==24,"Native receipt ABI");
static_assert(sizeof(DevfNativePlacementService)==24,"Windows x64 C ABI");
static_assert(offsetof(DevfNativePlacementService,request_placement)==8,"Function offset");
static_assert(offsetof(DevfNativePlacementService,remove_placement)==16,"Function offset");
#ifdef __cplusplus
}
#endif
MOD_DECLARE_SERVICE(DevfNativePlacementService,svc_devf_native,DEVF_NATIVE_SERVICE_ID,DEVF_NATIVE_MAJOR,DEVF_NATIVE_MINOR);
