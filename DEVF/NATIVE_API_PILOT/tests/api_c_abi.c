#include "../shared/devf_native_api.h"
_Static_assert(sizeof(DevfNativePlacementRequest)==80,"C request");
_Static_assert(sizeof(DevfNativePlacementService)==24,"C service");
int devf_c_abi_compile_only(void){return DEVF_NATIVE_MAJOR;}
