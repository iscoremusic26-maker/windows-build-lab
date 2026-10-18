#pragma once
#include "devf_native_api.h"
#ifdef __cplusplus
extern "C" {
#endif
#define DEVF_ENCOUNTER_SERVICE_ID "dev.devf.encounter_placement"
#define DEVF_ENCOUNTER_MAJOR 1u
#define DEVF_ENCOUNTER_MINOR 0u
#define DEVF_ENCOUNTER_EDIT_REGISTERED 1u
/* Same 80-byte data layout, distinct service and semantics: finite float32 XYZ,
 * native E_OC2 profile510/F_SP121/3/6, yaw s16, paramsFFFF0100.
 * This API does NOT certify terrain or expose an actor ID. */
typedef DevfNativePlacementRequest DevfEncounterPlacementRequest;
typedef struct DevfEncounterPlacementReceipt {
 uint32_t struct_size;
 uint32_t flags; /* EDIT_REGISTERED, NOT a fixed-site index */
 uint64_t placement_token;
 uint32_t registered_edits;
 uint32_t reserved;
} DevfEncounterPlacementReceipt;
typedef struct DevfEncounterPlacementService {
 ServiceHeader header;
 ModResult (*request_placement)(ModContext*,const DevfEncounterPlacementRequest*,DevfEncounterPlacementReceipt*);
 ModResult (*remove_placement)(ModContext*,uint64_t);
} DevfEncounterPlacementService;
static_assert(sizeof(DevfEncounterPlacementRequest)==80,"Encounter request ABI");
static_assert(sizeof(DevfEncounterPlacementReceipt)==24,"Encounter receipt ABI");
static_assert(sizeof(DevfEncounterPlacementService)==24,"Encounter service ABI");
static_assert(offsetof(DevfEncounterPlacementService,request_placement)==8,"Request slot");
static_assert(offsetof(DevfEncounterPlacementService,remove_placement)==16,"Remove slot");
#ifdef __cplusplus
}
#endif
MOD_DECLARE_SERVICE(DevfEncounterPlacementService,svc_devf_encounter,DEVF_ENCOUNTER_SERVICE_ID,DEVF_ENCOUNTER_MAJOR,DEVF_ENCOUNTER_MINOR);
