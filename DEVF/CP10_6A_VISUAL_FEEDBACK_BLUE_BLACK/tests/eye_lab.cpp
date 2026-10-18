// CP10.6A offline lab: Black eye recolor (green -> purple) in the CMPR compressed domain. Synthetic fixtures only: no game texture,
// no game memory, no GPU. Integer math => identical results on MSVC (CI) and GCC.
#include "../expansion/eye_recolor.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace devf_eye;
using devf_tex::RGB;using devf_tex::expand565;using devf_tex::pack565;using devf_tex::rd16;using devf_tex::rd32;using devf_tex::wr16;using devf_tex::wr32;
unsigned cases=0;
void require(bool v,const char* m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
void pass(const char* n){++cases;std::printf("CASE_PASS %s\n",n);}
int luma(RGB c){return (299*c.r+587*c.g+114*c.b+500)/1000;}
int hue_deg(RGB c){int ch=0;return devf_tex::hue_x16(c,ch)/16;}
int chroma(RGB c){int ch=0;devf_tex::hue_x16(c,ch);return ch;}
std::uint64_t lcg=0x0EEE5EED1234567ULL;std::uint32_t rnd(){lcg=lcg*6364136223846793005ULL+1442695040888963407ULL;return std::uint32_t(lcg>>33);}
std::array<std::array<int,4>,16> decode(const std::uint8_t* b){
 const auto c0=rd16(b),c1=rd16(b+2);const auto bits=rd32(b+4);const RGB a=expand565(c0),e=expand565(c1);
 std::array<std::array<int,4>,4> pal;pal[0]={a.r,a.g,a.b,255};pal[1]={e.r,e.g,e.b,255};
 if(c0>c1){pal[2]={(2*a.r+e.r)/3,(2*a.g+e.g)/3,(2*a.b+e.b)/3,255};pal[3]={(a.r+2*e.r)/3,(a.g+2*e.g)/3,(a.b+2*e.b)/3,255};}
 else{pal[2]={(a.r+e.r)/2,(a.g+e.g)/2,(a.b+e.b)/2,255};pal[3]={0,0,0,0};}
 std::array<std::array<int,4>,16> out;for(int i=0;i<16;++i)out[i]=pal[(bits>>(30-2*i))&3u];return out;
}
std::uint64_t fnv(const std::uint8_t* p,std::size_t n){return devf_tex::fnv1a(p,n);}
int main(){
 const Params& P=kPurple;
 static_assert(kVariants==8&&amount_pm(1)==142&&amount_pm(4)==571&&amount_pm(7)==1000&&amount_pm(9)==1000,"variant amounts");
 {for(unsigned k=2;k<kVariants;++k)require(amount_pm(k)>amount_pm(k-1),"strictly increasing");require(amount_pm(kVariants-1)==1000,"last is full");pass("eight_variant_table_monotonic_to_full_purple");}
 {unsigned changed=0;for(unsigned v=0;v<65536;++v){const RGB c=expand565(std::uint16_t(v));int w=0;const RGB o=recolor(c,0,P,&w);require(o.r==c.r&&o.g==c.g&&o.b==c.b,"t=0 identity");(void)changed;}
  pass("amount_zero_is_identity_for_all_65536_colors");}
 {unsigned green=0,other=0;for(unsigned v=0;v<65536;++v){const RGB c=expand565(std::uint16_t(v));int w=0;const RGB o=recolor(c,1000,P,&w);
   if(w==0){require(o.r==c.r&&o.g==c.g&&o.b==c.b,"zero weight untouched exactly");++other;}else ++green;}
  require(green>0&&other>green,"only a minority of colors (greens) is touched");pass("non_green_colors_are_left_bit_exact");}
 {struct G{RGB c;const char* n;};const G greens[]{{{60,200,70},"vivid green"},{{30,150,40},"mid green"},{{90,220,110},"light green"},{{20,90,25},"dark green"},{{110,200,60},"yellow green"},{{40,180,150},"teal green edge"}};
  for(const auto& g:greens){const RGB o=recolor(g.c,1000,P);const int h=hue_deg(o);int w=0;recolor(g.c,1000,P,&w);
   if(w>3500){require(h>=250&&h<=310,"full purple hue");require(luma(o)*100>=luma(g.c)*85-100&&luma(o)*100<=luma(g.c)*115+100,"luminance preserved (+-15%)");}
   else{require(w>0,"still partially shifted");}
   (void)g.n;}
  pass("green_pixels_become_purple_keeping_their_luminance");}
 {const RGB g{70,200,80};RGB prev=g;int last=0;for(unsigned k=1;k<kVariants;++k){const RGB o=recolor(g,amount_pm(k),P);const int d=std::abs(o.r-g.r)+std::abs(o.g-g.g)+std::abs(o.b-g.b);require(d>=last,"distance from the original never decreases");last=d;prev=o;}
  require(last>150,"the last step is far from the original");(void)prev;pass("interpolation_distance_monotonic_per_step");}
 {// white pupil highlight and near-neutral dark frame keep their color
  const RGB neutral[]{{255,255,255},{230,235,230},{20,22,20},{0,0,0},{128,128,128}};for(const auto& c:neutral){const RGB o=recolor(c,1000,P);require(o.r==c.r&&o.g==c.g&&o.b==c.b,"neutral untouched");}
  pass("pupil_highlights_and_neutrals_untouched");}
 {// block handling: modes and transparency survive, ordering invariants hold, untouched blocks identical, indices only swap in pairs
  for(int it=0;it<20000;++it){std::uint8_t b[8];std::uint16_t c0,c1;
   if(it%3==0){c0=pack565({int(rnd()%60),int(150+rnd()%100),int(rnd()%80)});c1=pack565({int(rnd()%60),int(60+rnd()%100),int(rnd()%60)});}
   else{c0=std::uint16_t(rnd());c1=std::uint16_t(rnd());}
   if(it%2){if(c0<c1){const auto t=c0;c0=c1;c1=t;}}                // mix of 4-color and 3-color blocks
   wr16(b,c0);wr16(b+2,c1);wr32(b+4,rnd());const std::uint8_t orig[8]{b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7]};
   const auto before=decode(b);Stats s;recolor_block(b,1000,P,s);const auto after=decode(b);
   const bool mode4_before=c0>c1,mode4_after=rd16(b)>rd16(b+2);
   if(!(rd16(b)==c0&&rd16(b+2)==c1&&std::memcmp(b,orig,8)==0)){
    require(mode4_before==mode4_after||(!mode4_before&&rd16(b)==rd16(b+2)),"block mode kept");
    for(int i=0;i<16;++i)require(before[i][3]==after[i][3],"transparency map identical");}
   else{for(int i=0;i<16;++i)require(before[i]==after[i],"untouched block decodes identically");}
   if(mode4_before)require(rd16(b)>rd16(b+2)||s.nudges,"four-color order kept");}
  pass("cmpr_block_modes_transparency_and_ordering_preserved_20000_random_blocks");}
 {// synthetic 128x128 atlas (16384 px = 1024 blocks of 4x4 = 4096*... bytes = 8192): only green blocks change; table independent of any game data
  const std::size_t bytes=devf_tex::cmpr_level_bytes(128,128);require(bytes==8192,"atlas size");
  std::vector<std::uint8_t> src(bytes);for(std::size_t o=0;o<bytes;o+=8){RGB a,b;const unsigned kind=(o/8)%4;
   if(kind==0){a={50+int(rnd()%40),170+int(rnd()%60),50+int(rnd()%40)};b={20,90,25};}      // green block
   else if(kind==1){a={150,int(120+rnd()%20),160};b={60,40,70};}                             // mauve skin
   else if(kind==2){a={160,110,50};b={90,60,30};}                                             // orange leather
   else {a={230,230,225};b={30,30,30};}                                                       // neutral
   std::uint16_t c0=pack565(a),c1=pack565(b);if(c0<c1){const auto t=c0;c0=c1;c1=t;}
   wr16(&src[o],c0);wr16(&src[o+2],c1);wr32(&src[o+4],rnd());}
  std::vector<std::uint8_t> v(src);Stats s;require(recolor_cmpr(v.data(),bytes,1000,P,s)&&s.blocks==bytes/8,"processed");
  unsigned same=0,diff=0;for(std::size_t o=0;o<bytes;o+=8)if(std::memcmp(&src[o],&v[o],8)==0)++same;else ++diff;
  require(diff==bytes/8/4&&same==bytes/8-diff,"exactly the green blocks changed (one in four)");require(s.green_endpoints>=diff,"green endpoints counted");
  std::vector<std::uint8_t> v0(src);Stats s0;require(recolor_cmpr(v0.data(),bytes,0,P,s0)&&v0==src&&s0.endpoints_changed==0,"amount 0 leaves the atlas bit-identical");
  std::vector<std::uint8_t> a(src),b(src);Stats sa,sb;recolor_cmpr(a.data(),bytes,amount_pm(3),P,sa);recolor_cmpr(b.data(),bytes,amount_pm(3),P,sb);require(a==b&&fnv(a.data(),bytes)==fnv(b.data(),bytes),"deterministic");
  std::uint8_t tiny[7]{};Stats st;require(!recolor_cmpr(tiny,7,1000,P,st)&&!recolor_cmpr(nullptr,8,1000,P,st)&&!recolor_cmpr(tiny,0,1000,P,st),"malformed sizes refused, nothing changed");
  std::printf("GOLDEN eye src=%016llx k3=%016llx k7=%016llx changed=%zu\n",(unsigned long long)fnv(src.data(),bytes),(unsigned long long)fnv(a.data(),bytes),(unsigned long long)fnv(v.data(),bytes),s.endpoints_changed);
  require(fnv(src.data(),bytes)==0x4e2aca9ce8c7b418ULL&&fnv(a.data(),bytes)==0x5946d35e2ec31823ULL&&fnv(v.data(),bytes)==0xbd0ea494a67fb8ebULL&&s.endpoints_changed==512,"golden vectors (must be identical on MSVC and GCC)");
  pass("synthetic_atlas_only_green_blocks_change_zero_is_bit_identical_deterministic_malformed_refused");}
 {std::vector<std::uint8_t> src(8192);for(std::size_t o=0;o<src.size();o+=8){wr16(&src[o],pack565({60,200,70}));wr16(&src[o+2],pack565({20,90,25}));wr32(&src[o+4],0x1B1B1B1Bu);}
  std::array<std::uint64_t,kVariants> hashes{};hashes[0]=fnv(src.data(),src.size());for(unsigned k=1;k<kVariants;++k){std::vector<std::uint8_t> v(src);Stats s;recolor_cmpr(v.data(),v.size(),amount_pm(k),P,s);hashes[k]=fnv(v.data(),v.size());}
  for(unsigned i=0;i<kVariants;++i)for(unsigned j=i+1;j<kVariants;++j)require(hashes[i]!=hashes[j],"all eight variants are distinct textures");
  pass("all_eight_variants_distinct");}
 std::printf("EYE_LAB_PASS cases=%u SYNTHETIC_ONLY; NO_GAME_TEXTURE_READ; PIXEL_LOOK_NOT_VERIFIED\n",cases);
}
