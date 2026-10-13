#include "experiment_core.hpp"
namespace devf::first {
// Technical arming of this fixed build, NOT human permission to load/run it.
// No config/UI/network input. Controller still enforces live F_SP121/3/6,
// exact candidate conversion/preset and every A/B/C guard before mutation.
Admission current_admission() noexcept {
 Admission a; a.location_admitted=true; a.session_authorized=true;
 a.bind_context_live=true; a.xyz=candidate; return a;
}
}
