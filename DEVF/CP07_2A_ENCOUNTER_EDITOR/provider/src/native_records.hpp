#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
namespace native_qa {
using Record=std::array<std::uint8_t,32>;
constexpr float north_floor=-6327.600450915229f;
constexpr float north_spawn=-6322.600450915229f; // floor +5 before float32 rounding; human QA pending.
constexpr float site1_spawn=-6304.00830078125f;
constexpr void be16(Record& r,std::size_t p,std::uint16_t v){r[p]=std::uint8_t(v>>8);r[p+1]=std::uint8_t(v);}
constexpr void be32(Record& r,std::size_t p,std::uint32_t v){for(unsigned i=0;i<4;++i)r[p+i]=std::uint8_t(v>>(24-8*i));}
constexpr Record record(bool north){
 Record r{};r[0]='E';r[1]='_';r[2]='o';r[3]='c';
 be32(r,8,0xFFFF0100u);be32(r,12,std::bit_cast<std::uint32_t>(-1250.f));
 be32(r,16,std::bit_cast<std::uint32_t>(north?north_spawn:site1_spawn));
 be32(r,20,std::bit_cast<std::uint32_t>(north?61950.f:60750.f));
 be16(r,24,0x0106);be16(r,26,0);be16(r,28,0);be16(r,30,0xFFFF);return r;
}
static_assert(sizeof(Record)==0x20);
static_assert(record(true)!=record(false));
constexpr const char* stage="F_SP121";
constexpr std::uint8_t room=3;
constexpr std::int8_t layer=6;
}
