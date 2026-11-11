#pragma once
#include <array>
#include <cstdint>
namespace expansion {
struct Captured {const char* id;const char* stage;int room,layer;float x,y,z;int yaw;unsigned profile,quantity;};
inline constexpr std::array<Captured,2> captured{{
 {"encounter-1","F_SP121",3,6,5394.84716796875f,-5603.69873046875f,51442.3046875f,-2421,510,1},
 {"encounter-2","F_SP121",3,6,3528.7822265625f,-5850.8798828125f,51201.09375f,-18402,510,1}
}};
}
