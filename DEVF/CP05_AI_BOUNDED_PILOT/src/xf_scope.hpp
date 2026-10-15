#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace devf_black {
using Command=std::array<std::uint8_t,9>;
constexpr std::uint32_t rgb_multiplier=0x08080A00u;
inline std::uint32_t be32(const std::uint8_t* p) noexcept {
 return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3];
}
inline Command xf(std::uint32_t rgba) noexcept {
 return {0x10,0x00,0x00,0x10,0x0C,std::uint8_t(rgba>>24),std::uint8_t(rgba>>16),std::uint8_t(rgba>>8),std::uint8_t(rgba)};
}
inline std::uint32_t charcoal(std::uint32_t base) noexcept {
 const auto r=((base>>24)&255u)*8u/255u,g=((base>>16)&255u)*8u/255u,b=((base>>8)&255u)*10u/255u;
 return (r<<24)|(g<<16)|(b<<8)|(base&255u);
}
// The reviewed native ColorBlock::load records an exact two-word XF batch at this offset.
// No search/scan and no pointer is retained. Caller establishes native DL ownership and range.
inline bool read_base(const std::uint8_t* bytes,std::size_t size,std::size_t offset,std::uint32_t& rgba) noexcept {
 if(!bytes||size>16384u||offset>size||size-offset<13u)return false;
 const auto* p=bytes+offset;
 if(p[0]!=0x10||be32(p+1)!=0x0001100Cu)return false;
 rgba=be32(p+5);return true;
}
struct Owner {
 std::uint32_t id=0,admitted_id=0;
 std::uint64_t generation=0;
 bool record=false,context=false,eoc2=false,created=false,model_matches=false,packet_matches=false;
 unsigned shape=99,material=99;
 bool eligible() const noexcept {
  return id!=0&&id==admitted_id&&generation!=0&&record&&context&&eoc2&&created&&model_matches&&packet_matches&&
   ((shape==0&&material==0)||(shape==2&&material==1));
 }
};
// Own scalar words only. No allocator, native asset, model pointer, retained FIFO address,
// virtual calculator, copied model graph or callback is installed in actor/model resources.
struct Scope {
 bool active=false,fault=false;
 std::uintptr_t call_key=0;
 std::uint32_t id=0,baseline=0,applied_word=0;
 std::uint64_t generation=0;
 const char* reason="NONE";
 template<class Emit> bool begin(const Owner& owner,std::uintptr_t key,std::uint32_t rgba,Emit emit) noexcept {
  if(active){abort(emit,"NESTED_PACKET_CALL");return false;}
  if(fault||!owner.eligible())return false;
  if(!key){fault=true;reason="INVALID_CALL_KEY";return false;}
  active=true;call_key=key;id=owner.id;generation=owner.generation;baseline=rgba;
  const auto command=xf(charcoal(rgba));applied_word=be32(command.data()+5);emit(command);return true;
 }
 template<class Emit> bool end(std::uintptr_t key,Emit emit) noexcept {
  if(!active)return false;
  if(key!=call_key){abort(emit,"POST_CALL_IDENTITY_MISMATCH");return false;}
  const auto command=xf(baseline);emit(command);active=false;call_key=0;return true;
 }
 template<class Emit> void abort(Emit emit,const char* why) noexcept {
  if(active){const auto command=xf(baseline);emit(command);}
  active=false;call_key=0;fault=true;reason=why;
 }
};
} // namespace devf_black
