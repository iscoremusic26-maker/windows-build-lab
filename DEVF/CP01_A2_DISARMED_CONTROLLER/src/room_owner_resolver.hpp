#pragma once
#include "controller_core.hpp"
namespace devf::a2 {
// Entry only from verified mod_update; no public runtime arming or request interface.
Owner capture_owner(bool host_verified, bool update_context);
}
