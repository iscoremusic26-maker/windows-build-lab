#include "../shared/devf_encounter_api.h"
static_assert(sizeof(DevfNativePlacementService)==24,"Old ABI unchanged");
static_assert(sizeof(DevfEncounterPlacementService)==24,"New ABI");
int encounter_abi_only;
