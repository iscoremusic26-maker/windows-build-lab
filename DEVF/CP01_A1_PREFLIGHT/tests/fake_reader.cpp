#include "../src/core.hpp"
namespace devf::a1 {
unsigned capture_calls = 0;
Snapshot fixture;
Snapshot capture_native() { ++capture_calls; return fixture; }
}
