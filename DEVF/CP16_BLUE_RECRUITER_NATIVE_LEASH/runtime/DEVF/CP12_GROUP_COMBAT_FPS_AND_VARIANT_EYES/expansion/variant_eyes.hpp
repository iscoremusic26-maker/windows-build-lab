#pragma once
// CP12 pure eye policy (no game headers): variant-matched eye colours on the EYE material packet only.
//   BLACK: intense red during the fury (replaces the CP10.6A purple), same 7-variant fade, same detection (green eye endpoints of E_OC2).
//   BLUE : the aura's blue/cyan during the group bonus, NEW. The E_OC atlas is not the E_OC2 one and its eye colour is not verified
//          from here, so the detection is chosen per atlas at build time: the proven green rule first; if the atlas has no green eye
//          endpoints, every CHROMATIC endpoint (neutral pupil/highlight kept) of the copy. That is safe because the copy is bound only to
//          the eye material packet of one model, and that material samples only its own atlas region (same argument as CP10.6A).
//   RED  : never.
// Colours are applied in the CMPR compressed domain by devf_eye::recolor_cmpr (endpoints only, indices kept, block mode preserved,
// luminance of the original preserved), so pupils, shading and native detail survive.
#include "../../CP10_6A_VISUAL_FEEDBACK_BLUE_BLACK/expansion/eye_recolor.hpp"
#include "../../CP10_6A_VISUAL_FEEDBACK_BLUE_BLACK/expansion/visual_policy.hpp"
#include "strokes_policy.hpp"
namespace devf_eyes12 {
using devf_variant::Variant;
enum class Scheme:std::uint8_t {None,BlackRed,BlueCyan};
constexpr Scheme scheme_for(Variant v) noexcept {return v==Variant::Black?Scheme::BlackRed:v==Variant::Blue?Scheme::BlueCyan:Scheme::None;}
// Same tint as the aura's global prm colour of the variant (strokes_policy.hpp), so eyes and aura share the apparent hue.
inline constexpr devf_eye::Params kRedEyes{70,170,25,12,32,devf_stk::kBlackColour.r,devf_stk::kBlackColour.g,devf_stk::kBlackColour.b};
inline constexpr devf_eye::Params kCyanEyesGreen{70,170,25,12,32,devf_stk::kBlueColour.r,devf_stk::kBlueColour.g,devf_stk::kBlueColour.b};
inline constexpr devf_eye::Params kCyanEyesAnyHue{0,360,0,12,32,devf_stk::kBlueColour.r,devf_stk::kBlueColour.g,devf_stk::kBlueColour.b};
constexpr std::size_t kMinEyeEndpoints=8;   // fewer endpoints than this under a rule = the rule does not see the eyes of this atlas
// Read-only count of endpoints a rule would recolour at full strength (weight > 1/2), without touching the atlas.
inline std::size_t count_endpoints(const std::uint8_t* data,std::size_t bytes,const devf_eye::Params& p) noexcept {
 if(!data||!bytes||bytes%8)return 0;
 std::size_t n=0;
 for(std::size_t o=0;o<bytes;o+=8){
  const std::uint16_t c0=devf_tex::rd16(data+o),c1=devf_tex::rd16(data+o+2);
  n+=(devf_eye::weight_q12(devf_tex::expand565(c0),p)>2048)+(devf_eye::weight_q12(devf_tex::expand565(c1),p)>2048);
 }
 return n;
}
struct Choice {const devf_eye::Params* params=nullptr;bool fallback=false;std::size_t endpoints=0;const char* why="NONE";};
inline Choice choose(Scheme s,const std::uint8_t* atlas,std::size_t bytes) noexcept {
 Choice c;
 if(s==Scheme::BlackRed){c.params=&kRedEyes;c.endpoints=count_endpoints(atlas,bytes,kRedEyes);if(c.endpoints<kMinEyeEndpoints){c.params=nullptr;c.why="NO_GREEN_EYE_ENDPOINTS";}return c;}
 if(s!=Scheme::BlueCyan){c.why="NO_EYE_SCHEME_FOR_VARIANT";return c;}
 c.endpoints=count_endpoints(atlas,bytes,kCyanEyesGreen);
 if(c.endpoints>=kMinEyeEndpoints){c.params=&kCyanEyesGreen;return c;}
 c.endpoints=count_endpoints(atlas,bytes,kCyanEyesAnyHue);
 if(c.endpoints>=kMinEyeEndpoints){c.params=&kCyanEyesAnyHue;c.fallback=true;return c;}
 c.why="NO_CHROMATIC_EYE_ENDPOINTS";return c;
}
// Eye fade of ONE entry: the exact eye block of devf_vis::step (CP10.6A, 10 in / 15 out, 7 variants, resume from the current level,
// best-effort return to native on a fault) without its Black-only guard. devf_vis::step keeps driving the Black eyes unchanged; this drives Blue.
template<class Fx> inline devf_vis::Events step_eyes(devf_vis::Entry& e,Fx& fx,bool want_eyes) noexcept {
 devf_vis::Events out;
 if(!e.eye_fault&&(want_eyes||e.eye_level>0)){
  const int level=devf_vis::next_level(e.eye_level,want_eyes);const unsigned idx=devf_vis::eye_index(level);
  e.eye_level=level;e.eye_target=want_eyes;
  if(idx!=e.eye_idx){
   if(!fx.eye(e,idx)){
    e.eye_fault=true;const char* why=fx.reason();
    if(e.eye_idx>0)fx.eye(e,0);
    e.eye_idx=0;e.eye_level=0;devf_vis::Event r;r.kind=devf_vis::Ev::EyesRejected;r.reason=why;r.eye_idx=idx;out.add(r);
   }else{
    const unsigned previous=e.eye_idx;e.eye_idx=idx;
    if(previous==0&&idx>0){devf_vis::Event o;o.kind=devf_vis::Ev::EyesOn;o.reason="LOGIC_STATE_ON";o.level=level;o.eye_idx=idx;out.add(o);}
    else if(idx==0){devf_vis::Event o;o.kind=devf_vis::Ev::EyesOff;o.reason="FADE_OUT_COMPLETE_NATIVE_TEXTURE_RESTORED";o.level=level;o.eye_idx=0;out.add(o);}
   }
  }
 }
 return out;
}
// Which channel drives the eyes of a variant: Black through devf_vis::step (unchanged), Blue through step_eyes, Red never.
constexpr bool eyes_via_policy_step(Variant v) noexcept {return v==Variant::Black;}
constexpr bool eyes_via_step_eyes(Variant v) noexcept {return v==Variant::Blue;}
// Per visual-ledger slot ownership of a Blue eye binding, stamped with id+generation (a new generation never inherits it).
struct EyeOwner {std::uint32_t id=0,generation=0;const void* model=nullptr;int set=-1;bool bound=false;};
template<std::size_t N> inline EyeOwner& owner_for(std::array<EyeOwner,N>& table,std::size_t index,std::uint32_t id,std::uint32_t generation) noexcept {
 EyeOwner& o=table[index<N?index:0];if(o.id!=id||o.generation!=generation){o=EyeOwner{};o.id=id;o.generation=generation;}return o;
}
} // namespace devf_eyes12
