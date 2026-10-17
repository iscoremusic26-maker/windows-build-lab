// CP10.2 FIX01 offline lab: CMPR compressed-domain recolor + per-instance binding. Synthetic fixtures only:
// no game texture, no game memory, no GPU. Integer math => identical results on MSVC (CI) and GCC.
#include "../expansion/cmpr_recolor.hpp"
#include "../expansion/texture_binding.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace devf_tex;
// Golden values produced by this integer implementation (must be identical on MSVC x64 and GCC x86-64).
constexpr std::uint64_t GOLDEN_SRC=0x923196cf9a7bedffULL,GOLDEN_OUT=0x1f01aafe6dd92478ULL;
constexpr std::size_t GOLDEN_SKIN=628,GOLDEN_CHANGED=2048;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
int luma(RGB c){return (299*c.r+587*c.g+114*c.b+500)/1000;}
int chroma(RGB c){int ch=0;hue_x16(c,ch);return ch;}
std::uint64_t lcg=0x0DEF1A2B3C4D5E6FULL;std::uint32_t rnd(){lcg=lcg*6364136223846793005ULL+1442695040888963407ULL;return std::uint32_t(lcg>>33);}
// decode one GX CMPR sub-block to 16 RGBA (alpha 0/255)
std::array<std::array<int,4>,16> decode(const std::uint8_t* b){
 const auto c0=rd16(b),c1=rd16(b+2);const auto bits=rd32(b+4);const RGB a=expand565(c0),e=expand565(c1);
 std::array<std::array<int,4>,4> pal;
 pal[0]={a.r,a.g,a.b,255};pal[1]={e.r,e.g,e.b,255};
 if(c0>c1){pal[2]={(2*a.r+e.r)/3,(2*a.g+e.g)/3,(2*a.b+e.b)/3,255};pal[3]={(a.r+2*e.r)/3,(a.g+2*e.g)/3,(a.b+2*e.b)/3,255};}
 else{pal[2]={(a.r+e.r)/2,(a.g+e.g)/2,(a.b+e.b)/2,255};pal[3]={0,0,0,0};}
 std::array<std::array<int,4>,16> out;for(int i=0;i<16;++i)out[i]=pal[(bits>>(30-2*i))&3u];return out;
}
void put(std::uint8_t* b,std::uint16_t c0,std::uint16_t c1,std::uint32_t bits){wr16(b,c0);wr16(b+2,c1);wr32(b+4,bits);}
// ---- fake packets for the binding tests ----
struct Tex {int id;};
struct Packet {Tex* texture;};
auto get=[](Packet* p){return p->texture;};
auto set=[](Packet* p,Tex* t){p->texture=t;};
struct Entry {const void* source;unsigned long long source_hash;Tex* texture;};
int main(){
 const Params& P=kCharcoal;
 {for(unsigned v=0;v<65536;++v)require(pack565(expand565(std::uint16_t(v)))==v,"565 roundtrip");pass("rgb565_expand_pack_roundtrip_all_65536");}
 {const RGB g{128,128,128};int w=-1;const RGB o=recolor(g,P,&w);require(w==0&&o.r==118&&o.g==118&&o.b==118,"neutral");pass("neutral_metal_keeps_hue_8pct_darker");}
 {const RGB skin{115,72,104};int w=0;const RGB o=recolor(skin,P,&w);
  require(w==4096,"mauve skin full weight");const int y0=luma(skin),y1=luma(o);
  require(y1*1000>=y0*760&&y1*1000<=y0*840,"skin luminance ~80% (dark, not black)");
  require(chroma(o)*100<=chroma(skin)*35,"skin chroma strongly reduced");require(o.b>=o.r&&o.b>=o.g,"cool slate");
  require(y1>=40,"not crushed black");pass("mauve_skin_to_cool_charcoal_luma_preserving");}
 {for(RGB c:{RGB{60,160,60},RGB{190,105,45},RGB{120,115,70},RGB{200,200,205},RGB{20,20,22}}){int w=-1;const RGB o=recolor(c,P,&w);require(w==0,"non-skin weight 0");
   require(o.r==(c.r*920+500)/1000&&o.g==(c.g*920+500)/1000&&o.b==(c.b*920+500)/1000,"non-skin exact gain");}
  pass("eye_green_leather_cloth_metal_black_keep_hue");}
 {int prev=4096;for(int deg=0;deg<=90;deg+=5){int hue_r=0;(void)hue_r;
   // purple family around 325 deg: build a color at hue 325+deg with fixed chroma via HSV-ish construction
   const int h=(325+deg)%360;const double hd=h/60.0;const int i=int(hd);const double f=hd-i;const int v=200,m=100,c=v-m;
   const int x=int(m+c*(i%2?1-f:f));RGB col;switch(i%6){case 0:col={v,x,m};break;case 1:col={x,v,m};break;case 2:col={m,v,x};break;case 3:col={m,x,v};break;case 4:col={x,m,v};break;default:col={v,m,x};}
   int w=0;recolor(col,P,&w);require(w<=prev,"weight monotonic with hue distance");prev=w;
   if(deg<=34)require(w==4096,"full band");
   if(deg>=56)require(w==0,"outside feather");}
  pass("hue_weight_band_feather_monotonic");}
 {int w=0;recolor(RGB{102,100,101},P,&w);require(w==0,"chroma below lo is neutral");pass("low_chroma_never_treated_as_skin");}
 // ---- block semantics ----
 {std::uint8_t b[8];int found=0;
  for(unsigned t=0;t<200000&&found<50;++t){const std::uint16_t c0=std::uint16_t(rnd()),c1=std::uint16_t(rnd());if(c0<=c1)continue;
   put(b,c0,c1,rnd());const auto before=decode(b);Stats s;recolor_block(b,P,s);require(rd16(b)>rd16(b+2),"four-color mode kept");
   const auto after=decode(b);for(int i=0;i<16;++i)require(after[i][3]==255,"no transparency introduced");
   // endpoint texels map to the recolored endpoint colors exactly (after a possible swap)
   const auto e0=pack565(recolor(expand565(c0),P)),e1=pack565(recolor(expand565(c1),P));
   const RGB r0=expand565(rd16(b)),r1=expand565(rd16(b+2));
   if(s.swaps)require(rd16(b)==e1&&rd16(b+2)==e0,"swap uses recolored endpoints");else if(!s.nudges)require(rd16(b)==e0&&rd16(b+2)==e1,"endpoints recolored");
   (void)before;(void)r0;(void)r1;if(s.swaps)++found;}
  require(found>0,"swap cases exercised");pass("four_color_mode_preserved_with_swaps");}
 {std::uint8_t b[8];bool nudged=false;
  for(unsigned c0=1;c0<65536&&!nudged;++c0){const std::uint16_t c1=std::uint16_t(c0-1);
   if(pack565(recolor(expand565(std::uint16_t(c0)),P))!=pack565(recolor(expand565(c1),P)))continue;
   put(b,std::uint16_t(c0),c1,0x1B1B1B1Bu);Stats s;recolor_block(b,P,s);require(s.nudges==1&&rd16(b)>rd16(b+2),"collision nudged");nudged=true;}
  require(nudged,"collision case exists");pass("four_color_endpoint_collision_nudged_not_transparent");}
 {std::uint8_t b[8];int swaps=0;
  for(unsigned t=0;t<200000;++t){std::uint16_t c0=std::uint16_t(rnd()),c1=std::uint16_t(rnd());if(c0>c1){auto x=c0;c0=c1;c1=x;}
   std::uint32_t bits=rnd()|0xC0000003u; // pixel 0 and 15 transparent
   put(b,c0,c1,bits);const auto before=decode(b);Stats s;recolor_block(b,P,s);require(rd16(b)<=rd16(b+2),"three-color mode kept");
   const auto after=decode(b);for(int i=0;i<16;++i)require((before[i][3]==0)==(after[i][3]==0),"transparency positions kept");
   swaps+=int(s.swaps);}
  require(swaps>0,"three-color swaps exercised");pass("three_color_transparency_preserved");}
 {std::uint8_t b[8];put(b,0x8000,0x0001,0x00FF00FFu);std::uint8_t c[8];std::memcpy(c,b,8);Stats s;recolor_block(b,P,s);
  const std::uint32_t x=rd32(b+4),o=rd32(c+4);require(x==o||x==(o^0x55555555u),"indices kept or documented remap only");pass("indices_never_requantized");}
 {std::vector<std::uint8_t> v(24,0x11);const auto copy=v;Stats s;require(!recolor_cmpr(v.data(),7,P,s)&&!recolor_cmpr(nullptr,8,P,s)&&!recolor_cmpr(v.data(),0,P,s),"bad sizes");
  require(v==copy&&s.blocks==0,"rejected input untouched");pass("malformed_sizes_rejected_untouched");}
 {require(cmpr_level_bytes(128,128)==8192&&cmpr_level_bytes(8,8)==32&&cmpr_level_bytes(4,4)==32&&cmpr_level_bytes(130,8)==17*32,"level size");pass("cmpr_level_size");}
 // ---- golden: deterministic synthetic 128x128 atlas (skin / cloth / metal / eye-like / transparent blocks) ----
 {std::vector<std::uint8_t> src(8192);lcg=0x5EEDCAFE12345678ULL;
  const RGB families[6]={{118,70,110},{90,52,84},{120,115,70},{196,196,200},{60,160,60},{190,105,45}};
  for(std::size_t o=0;o<src.size();o+=8){const RGB f=families[rnd()%6];auto j=[&](int x){x+=int(rnd()%41)-20;return x<0?0:x>255?255:x;};
   std::uint16_t c0=pack565({j(f.r),j(f.g),j(f.b)}),c1=pack565({j(f.r),j(f.g),j(f.b)});
   if(rnd()%16==0){if(c0>c1){auto x=c0;c0=c1;c1=x;}}else if(c0<=c1){auto x=c0;c0=c1;c1=x;if(c0==c1)++c0;}
   put(&src[o],c0,c1,rnd());}
  const auto original=src;const std::uint64_t src_hash=fnv1a(src.data(),src.size());
  std::vector<std::uint8_t> priv=src;Stats s;require(recolor_cmpr(priv.data(),priv.size(),P,s),"recolor ok");
  require(src==original&&fnv1a(src.data(),src.size())==src_hash,"original buffer untouched");
  const std::uint64_t out=fnv1a(priv.data(),priv.size());
  std::printf("GOLDEN src=%016llx out=%016llx blocks=%zu changed=%zu skin=%zu swaps=%zu nudges=%zu three=%zu\n",(unsigned long long)src_hash,(unsigned long long)out,s.blocks,s.endpoints_changed,s.skin_endpoints,s.swaps,s.nudges,s.three_color);
  require(s.blocks==1024,"blocks");
  require(src_hash==GOLDEN_SRC&&out==GOLDEN_OUT,"golden hashes (integer math must match on every compiler)");
  require(s.skin_endpoints==GOLDEN_SKIN&&s.endpoints_changed==GOLDEN_CHANGED,"golden stats");
  pass("golden_synthetic_atlas_bit_exact");}
 // ---- per-instance binding: shared model data texture, one Black model, one Normal model ----
 {Tex original{1},priv{2};Packet a0{&original},a1{&original},ae{&original},n0{&original},n1{&original},ne{&original};
  require(devf_bind::bind_body(&a0,&a1,&ae,&original,&priv,get,set)==nullptr,"bind");
  require(a0.texture==&priv&&a1.texture==&priv&&ae.texture==&original,"black body private, eye native");
  require(n0.texture==&original&&n1.texture==&original&&ne.texture==&original,"normal model untouched");
  require(original.id==1,"shared texture object never written");pass("black_and_normal_same_resource_isolated");
  require(devf_bind::bind_body(&a0,&a1,&ae,&original,&priv,get,set)!=nullptr,"double bind rejected");pass("double_bind_rejected");
  require(devf_bind::restore_body(&a0,&a1,&original,&priv,get,set)==2&&a0.texture==&original&&a1.texture==&original,"restore");pass("shutdown_restore_to_native");
  Packet b0{&original},b1{&original},be{&original};require(!devf_bind::bind_body(&a0,&a1,&ae,&original,&priv,get,set)&&!devf_bind::bind_body(&b0,&b1,&be,&original,&priv,get,set),"two black models");
  require(a0.texture==&priv&&b0.texture==&priv&&n0.texture==&original,"two black + one normal simultaneously");pass("two_black_models_share_private_normal_native");}
 {Tex original{1},priv{2},foreign{3};Packet p0{&original},p1{&original},pe{&foreign};
  require(devf_bind::bind_body(&p0,&p1,&pe,&original,&priv,get,set)!=nullptr&&p0.texture==&original&&p1.texture==&original,"unexpected eye texture => no change");
  Packet q0{&foreign},q1{&original},qe{&original};require(devf_bind::bind_body(&q0,&q1,&qe,&original,&priv,get,set)!=nullptr&&q1.texture==&original,"other mod already rebound => fail closed");
  require(devf_bind::bind_body(&p0,&p0,&pe,&original,&priv,get,set)!=nullptr,"alias");require(devf_bind::bind_body(&p0,&p1,&pe,&original,&original,get,set)!=nullptr,"priv==orig");
  require(devf_bind::bind_body<Packet,Tex>(nullptr,&p1,&pe,&original,&priv,get,set)!=nullptr,"null");pass("unexpected_packet_state_fail_closed_untouched");}
 {Tex t1{1},t2{2};const int src1=0,src2=0;Entry e[2]={{&src1,10,&t1},{&src2,20,&t2}};
  require(devf_bind::find_cached(e,2,&src1,10)==0&&devf_bind::find_cached(e,2,&src2,20)==1,"hit");
  require(devf_bind::find_cached(e,2,&src1,11)==-1&&devf_bind::find_cached(e,2,&src2,10)==-1,"same pointer, different content => rebuild");
  e[0].texture=nullptr;require(devf_bind::find_cached(e,2,&src1,10)==-1,"incomplete entry never reused");pass("private_texture_cache_keyed_by_pointer_and_content");}
 {Tex original{1},priv{2};Packet a0{&original},a1{&original},ae{&original};devf_bind::bind_body(&a0,&a1,&ae,&original,&priv,get,set);
  // room exit: model + packets destroyed by the game; a new generation gets fresh native packets and binds again
  Packet r0{&original},r1{&original},re{&original};require(!devf_bind::bind_body(&r0,&r1,&re,&original,&priv,get,set)&&r0.texture==&priv,"rebind new generation");
  require(devf_bind::restore_body(&r0,&r1,&original,&priv,get,set)==2&&devf_bind::restore_body(&r0,&r1,&original,&priv,get,set)==0,"restore idempotent");pass("reentry_new_generation_and_idempotent_restore");}
 std::printf("TEXTURE_LAB_PASS cases=%u OFFLINE_ONLY; SYNTHETIC_FIXTURES; GAME_TEXTURE_AND_GPU_NOT_USED\n",cases);
}
