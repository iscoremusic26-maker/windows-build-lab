#include "experiment_core.hpp"
namespace devf::first {
// Sealed in this offline delivery: no config/UI/export can grant location/session permission.
// A future independently authorized source revision must supply a real reviewed admission.
__declspec(noinline) Admission current_admission() noexcept { return {}; }
}
