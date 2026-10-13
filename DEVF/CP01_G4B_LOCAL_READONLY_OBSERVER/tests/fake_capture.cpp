#include "../src/capture.hpp"
#include <stdexcept>
unsigned reads=0;bool fail_capture=false;
namespace devf::g4b { Capture capture_native(Id id){++reads;if(fail_capture)throw std::runtime_error("synthetic failure");Capture c;c.id=id;return c;} }
