#pragma once
// CP10.2 FIX01: regional recolor of a GX CMPR (BC1-like) texture in the COMPRESSED domain.
// Only the two RGB565 endpoint colors of each 4x4 sub-block are transformed; the 2-bit pixel indices are kept,
// so texel structure, edges and local contrast are preserved exactly (no re-quantization of the image).
// Integer-only math: identical results on MSVC and GCC (golden vectors in tests/texture_lab.cpp).
// Operates on a caller-owned COPY; never on the game's original texture bytes.
#include <array>
#include <cstddef>
#include <cstdint>
namespace devf_tex {

// Bounded, named preset. Units: degrees for hue, 0..255 levels for chroma, per mille (1000 = x1) for gains.
struct Params {
 int hue_center_deg=325;   // E_OC2 skin is mauve/purple (measured locally: endpoint hues cluster at 320-360 deg)
 int hue_full_deg=35;      // full skin weight within +-35 deg
 int hue_feather_deg=20;   // fades to zero over the next 20 deg (orange leather ~20-40 deg and olive cloth ~40-80 deg stay native)
 int chroma_lo=4;          // below this chroma a color is treated as neutral (chain mail, metal, near-black)
 int chroma_hi=13;
 int skin_gain_pm=800;     // skin luminance kept at 80%: dark charcoal, never forced to black
 int tint_r_pm=950,tint_g_pm=1000,tint_b_pm=1120; // cool slate hue; luma-neutral (0.299*950+0.587*1000+0.114*1120 ~= 1000)
 int keep_chroma_pm=120;   // 12% of the original chroma survives inside the skin: subtle natural variation, no flat gray
 int other_gain_pm=920;    // non-skin materials keep their hue and saturation, 8% darker to sit with the darker body
};
inline constexpr Params kCharcoal{};

struct RGB {int r=0,g=0,b=0;};
inline RGB expand565(std::uint16_t v) noexcept {
 const int r=(v>>11)&31,g=(v>>5)&63,b=v&31;
 return {(r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2)};
}
inline std::uint16_t pack565(RGB c) noexcept {
 auto q=[](int v,int max){if(v<0)v=0;if(v>255)v=255;return (v*max+127)/255;};
 return static_cast<std::uint16_t>((q(c.r,31)<<11)|(q(c.g,63)<<5)|q(c.b,31));
}
// smoothstep in Q12 (0..4096), integer only
inline int smooth_q12(int e0,int e1,int x) noexcept {
 if(x<=e0)return 0;
 if(x>=e1)return 4096;
 const std::int64_t t=std::int64_t(x-e0)*4096/(e1-e0);
 return static_cast<int>(t*t/4096*(3*4096-2*t)/4096);
}
// HSV hue in 1/16 degree (0..5759) and chroma (0..255). Truncating integer division (C++ semantics).
inline int hue_x16(RGB c,int& chroma) noexcept {
 const int mx=c.r>c.g?(c.r>c.b?c.r:c.b):(c.g>c.b?c.g:c.b);
 const int mn=c.r<c.g?(c.r<c.b?c.r:c.b):(c.g<c.b?c.g:c.b);
 chroma=mx-mn;if(!chroma)return 0;
 int h;
 if(mx==c.r){h=960*(c.g-c.b)/chroma;if(h<0)h+=5760;}
 else if(mx==c.g)h=960*(c.b-c.r)/chroma+1920;
 else h=960*(c.r-c.g)/chroma+3840;
 return h;
}
inline int skin_weight_q12(RGB c,const Params& p) noexcept {
 int chroma=0;const int h=hue_x16(c,chroma);
 if(!chroma)return 0;
 int d=((h-p.hue_center_deg*16)%5760+5760)%5760;if(d>2880)d=5760-d; // circular distance, 1/16 degree
 const int hue_w=4096-smooth_q12(p.hue_full_deg*16,(p.hue_full_deg+p.hue_feather_deg)*16,d);
 const int chroma_w=smooth_q12(p.chroma_lo,p.chroma_hi,chroma);
 return hue_w*chroma_w/4096;
}
// Albedo transform for one color. All intermediate values in milli-levels (0..255000).
inline RGB recolor(RGB c,const Params& p,int* weight_out=nullptr) noexcept {
 const std::int64_t y=299LL*c.r+587LL*c.g+114LL*c.b;
 const std::int64_t ys=y*p.skin_gain_pm/1000;
 const int w=skin_weight_q12(c,p);if(weight_out)*weight_out=w;
 const int in[3]={c.r,c.g,c.b};const int tint[3]={p.tint_r_pm,p.tint_g_pm,p.tint_b_pm};int out[3];
 for(int i=0;i<3;++i){
  const std::int64_t cm=1000LL*in[i];
  std::int64_t skin=ys*tint[i]/1000;
  if(y>0)skin+=(p.keep_chroma_pm*(cm-y)/1000)*ys/y;
  const std::int64_t other=cm*p.other_gain_pm/1000;
  std::int64_t v=(w*skin+(4096-w)*other)/4096;
  if(v<0)v=0;
  v=(v+500)/1000;out[i]=v>255?255:static_cast<int>(v);
 }
 return {out[0],out[1],out[2]};
}

struct Stats {std::size_t blocks=0,endpoints_changed=0,skin_endpoints=0,swaps=0,nudges=0,three_color=0;};
inline std::uint16_t rd16(const std::uint8_t* b) noexcept {return static_cast<std::uint16_t>((b[0]<<8)|b[1]);}
inline void wr16(std::uint8_t* b,std::uint16_t v) noexcept {b[0]=std::uint8_t(v>>8);b[1]=std::uint8_t(v);}
inline std::uint32_t rd32(const std::uint8_t* b) noexcept {return (std::uint32_t(b[0])<<24)|(std::uint32_t(b[1])<<16)|(std::uint32_t(b[2])<<8)|b[3];}
inline void wr32(std::uint8_t* b,std::uint32_t v) noexcept {b[0]=std::uint8_t(v>>24);b[1]=std::uint8_t(v>>16);b[2]=std::uint8_t(v>>8);b[3]=std::uint8_t(v);}
// Recolor one 8-byte GX CMPR sub-block in place, keeping its mode:
//  four-color (c0>c1) stays four-color; three-color+transparent (c0<=c1) stays three-color with index 3 transparent.
inline void recolor_block(std::uint8_t* b,const Params& p,Stats& s) noexcept {
 const std::uint16_t c0=rd16(b),c1=rd16(b+2);std::uint32_t bits=rd32(b+4);
 int w0=0,w1=0;
 std::uint16_t n0=pack565(recolor(expand565(c0),p,&w0)),n1=pack565(recolor(expand565(c1),p,&w1));
 s.endpoints_changed+=(n0!=c0)+(n1!=c1);s.skin_endpoints+=(w0>2048)+(w1>2048);++s.blocks;
 if(c0>c1){ // four-color mode must keep n0>n1
  if(n0<n1){const auto t=n0;n0=n1;n1=t;bits^=0x55555555u;++s.swaps;} // indices 0<->1, 2<->3 (2/3 c0+1/3 c1 <-> 1/3 c0+2/3 c1)
  else if(n0==n1){if(n0<0xFFFFu)++n0;else --n1;++s.nudges;}
 }else{ // three-color mode must keep n0<=n1; index 2 = midpoint (symmetric), index 3 = transparent
  ++s.three_color;
  if(n0>n1){const auto t=n0;n0=n1;n1=t;
   std::uint32_t out=0;for(int i=0;i<16;++i){std::uint32_t v=(bits>>(30-2*i))&3u;if(v<2)v^=1u;out|=v<<(30-2*i);}bits=out;++s.swaps;}
 }
 wr16(b,n0);wr16(b+2,n1);wr32(b+4,bits);
}
// Whole level(s) of CMPR data (multiple of 8 bytes). Returns false (and changes nothing) on malformed sizes.
inline bool recolor_cmpr(std::uint8_t* data,std::size_t bytes,const Params& p,Stats& s) noexcept {
 if(!data||!bytes||bytes%8||bytes>(1u<<22))return false;
 for(std::size_t o=0;o<bytes;o+=8)recolor_block(data+o,p,s);
 return true;
}
// GX CMPR level size: 8x8 texel tiles of 32 bytes; width/height rounded up to 8.
inline std::size_t cmpr_level_bytes(unsigned w,unsigned h) noexcept {return std::size_t((w+7)/8)*((h+7)/8)*32u;}
inline std::uint64_t fnv1a(const std::uint8_t* p,std::size_t n) noexcept {std::uint64_t h=1469598103934665603ULL;for(std::size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ULL;}return h;}
} // namespace devf_tex
