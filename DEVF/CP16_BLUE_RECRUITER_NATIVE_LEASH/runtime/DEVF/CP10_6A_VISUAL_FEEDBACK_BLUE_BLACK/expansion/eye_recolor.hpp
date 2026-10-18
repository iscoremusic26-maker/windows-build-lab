#pragma once
// CP10.6A pure eye recolor (no game headers): green -> purple on a caller-owned COPY of the E_OC2 CMPR atlas, in the compressed domain
// (RGB565 endpoints only, indices kept, block mode preserved), exactly like the approved FIX01 body recolor but with its own bounded
// parameters and an interpolation amount `t` (per mille). Integer math only: identical results on MSVC and GCC.
//
// WHY THE WHOLE ATLAS MAY BE PROCESSED: the three E_OC2 materials share ONE 128x128 atlas, but the eye material samples only its own
// region, and the private variant produced here is bound ONLY to the eye material packet of ONE Black model (the body keeps the FIX01
// private texture). Any other green in the atlas is therefore never drawn through this variant. Non-green colors are left exactly as is.
#include "../../CP10_2_FIX01_BLACK_TEXTURE/expansion/cmpr_recolor.hpp"
namespace devf_eye {
using devf_tex::RGB;
constexpr unsigned kVariants=8;   // index 0 = the native texture (never built); 1..7 = t = k/7; 7 = full purple
struct Params {
 int hue_lo_deg=70,hue_hi_deg=170,feather_deg=25;   // greens: full weight on [lo,hi], feathered outside
 int chroma_lo=12,chroma_hi=32;                     // near-neutral colors (pupil highlight, dark frame) keep their color
 int tint_r=158,tint_g=60,tint_b=255;               // purple direction; luminance of the original is preserved
};
inline constexpr Params kPurple{};
// Per-mille interpolation amount of variant k (k in 1..kVariants-1).
constexpr int amount_pm(unsigned k) noexcept {return k>=kVariants-1?1000:int(k*1000u/(kVariants-1));}
inline int weight_q12(RGB c,const Params& p) noexcept {
 int chroma=0;const int h=devf_tex::hue_x16(c,chroma);
 if(!chroma)return 0;
 int d=0;
 if(h<p.hue_lo_deg*16)d=p.hue_lo_deg*16-h;else if(h>p.hue_hi_deg*16)d=h-p.hue_hi_deg*16;
 if(d>2880)d=5760-d;                                                // circular distance, 1/16 degree
 const int hue_w=4096-devf_tex::smooth_q12(0,p.feather_deg*16,d);
 const int chroma_w=devf_tex::smooth_q12(p.chroma_lo,p.chroma_hi,chroma);
 return hue_w*chroma_w/4096;
}
inline RGB recolor(RGB c,int t_pm,const Params& p,int* weight_out=nullptr) noexcept {
 const int w=weight_q12(c,p);if(weight_out)*weight_out=w;
 if(!w||t_pm<=0)return c;
 if(t_pm>1000)t_pm=1000;
 const std::int64_t y=299LL*c.r+587LL*c.g+114LL*c.b;                   // milli-levels
 const std::int64_t py=299LL*p.tint_r+587LL*p.tint_g+114LL*p.tint_b;
 const int in[3]={c.r,c.g,c.b},dir[3]={p.tint_r,p.tint_g,p.tint_b};int out[3];
 for(int i=0;i<3;++i){
  std::int64_t target=py?std::int64_t(dir[i])*y*1000/py:0;              // milli-levels: purple direction scaled to the original luminance
  if(target>255000)target=255000;
  const std::int64_t base=1000LL*in[i];
  const std::int64_t v=base+(target-base)*w*t_pm/(4096LL*1000);
  const std::int64_t r=(v+500)/1000;out[i]=r<0?0:r>255?255:static_cast<int>(r);
 }
 return {out[0],out[1],out[2]};
}
struct Stats {std::size_t blocks=0,endpoints_changed=0,green_endpoints=0,swaps=0,nudges=0;};
// Same block handling as devf_tex::recolor_block (mode preserved), with the eye color map.
inline void recolor_block(std::uint8_t* b,int t_pm,const Params& p,Stats& s) noexcept {
 using namespace devf_tex;
 const std::uint16_t c0=rd16(b),c1=rd16(b+2);std::uint32_t bits=rd32(b+4);
 int w0=0,w1=0;
 std::uint16_t n0=pack565(recolor(expand565(c0),t_pm,p,&w0)),n1=pack565(recolor(expand565(c1),t_pm,p,&w1));
 s.endpoints_changed+=(n0!=c0)+(n1!=c1);s.green_endpoints+=(w0>2048)+(w1>2048);++s.blocks;
 if(n0==c0&&n1==c1)return;                                          // untouched block: bytes identical
 if(c0>c1){
  if(n0<n1){const auto t=n0;n0=n1;n1=t;bits^=0x55555555u;++s.swaps;}
  else if(n0==n1){if(n0<0xFFFFu)++n0;else --n1;++s.nudges;}
 }else{
  if(n0>n1){const auto t=n0;n0=n1;n1=t;
   std::uint32_t out=0;for(int i=0;i<16;++i){std::uint32_t v=(bits>>(30-2*i))&3u;if(v<2)v^=1u;out|=v<<(30-2*i);}bits=out;++s.swaps;}
 }
 wr16(b,n0);wr16(b+2,n1);wr32(b+4,bits);
}
inline bool recolor_cmpr(std::uint8_t* data,std::size_t bytes,int t_pm,const Params& p,Stats& s) noexcept {
 if(!data||!bytes||bytes%8||bytes>(1u<<22))return false;
 for(std::size_t o=0;o<bytes;o+=8)recolor_block(data+o,t_pm,p,s);
 return true;
}
} // namespace devf_eye
