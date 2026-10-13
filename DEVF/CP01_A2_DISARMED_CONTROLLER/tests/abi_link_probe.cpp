#include <f_pc/f_pc_layer.h>
#include <f_pc/f_pc_manager.h>
#include <mods/svc/actor.h>
#include <type_traits>
static_assert(sizeof(void*)==8);
static_assert(std::is_same_v<decltype(ActorSpawnParams::create_function),int(*)(void*)>);
// Address retention only; no calls. Kept OUT of host module and all installable packages.
extern "C" __declspec(dllexport) const void* a2_offline_abi_addresses[] = {
    reinterpret_cast<const void*>(&fpcLy_CurrentLayer),reinterpret_cast<const void*>(&fpcLy_SetCurrentLayer),
    reinterpret_cast<const void*>(&fpcLy_RootLayer),reinterpret_cast<const void*>(&fpcLy_IsDeletingMesg),
    reinterpret_cast<const void*>(&fpcLy_Search),reinterpret_cast<const void*>(&fpcM_IsCreating)
};
