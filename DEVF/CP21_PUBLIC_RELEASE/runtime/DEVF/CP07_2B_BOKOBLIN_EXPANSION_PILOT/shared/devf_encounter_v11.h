#pragma once
#include "devf_encounter_api.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Additional private service: V1 remains published, unchanged, on its old ID.
 * Explicit resource is decoded into the actor record BEFORE StageService.add_actor.
 * Neither parameters nor V1 reserved fields carry extension data. Receipt is an edit,
 * never an ActorID. Cold bootstrap, owner attribution and rollback follow V1. */
#define DEVF_ENCOUNTER_V11_SERVICE_ID "dev.devf.encounter_placement.v11"
#define DEVF_ENCOUNTER_V11_MAJOR 1u
#define DEVF_ENCOUNTER_V11_MINOR 1u
#define DEVF_ENCOUNTER_V2_REQUEST_VERSION 2u
#define DEVF_ENCOUNTER_RESOURCE_E_OC 1u
#define DEVF_ENCOUNTER_RESOURCE_E_OC2 2u
#define DEVF_ENCOUNTER_CAP_EXPLICIT_RESOURCE 1u
typedef struct DevfEncounterPlacementRequestV2 {
 uint32_t struct_size;
 uint32_t request_version;
 uint32_t native_resource;
 uint32_t reserved; /* must be zero */
 DevfEncounterPlacementRequest placement; /* unchanged 80-byte V1 record */
} DevfEncounterPlacementRequestV2;
typedef struct DevfEncounterPlacementServiceV11 {
 ServiceHeader header;
 uint64_t capabilities;
 ModResult (*request_placement_v2)(ModContext*,const DevfEncounterPlacementRequestV2*,DevfEncounterPlacementReceipt*);
 ModResult (*remove_placement)(ModContext*,uint64_t);
} DevfEncounterPlacementServiceV11;
static_assert(sizeof(DevfEncounterPlacementRequestV2)==96,"V2 request ABI");
static_assert(offsetof(DevfEncounterPlacementRequestV2,placement)==16,"V1 record offset");
static_assert(sizeof(DevfEncounterPlacementServiceV11)==32,"Additional V1.1 service ABI");
static_assert(offsetof(DevfEncounterPlacementServiceV11,capabilities)==8,"Capabilities slot");
static_assert(offsetof(DevfEncounterPlacementServiceV11,request_placement_v2)==16,"V2 request slot");
static_assert(offsetof(DevfEncounterPlacementServiceV11,remove_placement)==24,"V1.1 removal slot");
#ifdef __cplusplus
}
#endif
MOD_DECLARE_SERVICE(DevfEncounterPlacementServiceV11,svc_devf_encounter_v11,DEVF_ENCOUNTER_V11_SERVICE_ID,DEVF_ENCOUNTER_V11_MAJOR,DEVF_ENCOUNTER_V11_MINOR);
