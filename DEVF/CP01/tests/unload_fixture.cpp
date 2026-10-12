#include <mods/api.h>
extern "C" {
MOD_EXPORT ModResult mod_shutdown(ModError*) { return MOD_UNAVAILABLE; }
MOD_EXPORT int fixture_callback() { return 17; }
}
