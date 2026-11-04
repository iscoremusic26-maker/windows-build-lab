#pragma once
// CP05 FIX02: additional value-only evidence. Legacy parry/continuation policy
// remains unchanged; only the ordinary Link sword POST05 transaction may use it.
#include "fix01_core.hpp"
#include "cp04_combo_core.hpp"
namespace cp05fix02 {
enum class Terminal:std::uint8_t{None,ForwardEnd,RecoilEnd};
inline const char* name(Terminal t)noexcept{return t==Terminal::ForwardEnd?"NATIVE_BC_STOPPED_AT_END":t==Terminal::RecoilEnd?"NATIVE_RECOIL_STOPPED_AT_ZERO":"NO_TERMINAL_PROOF";}
inline Terminal terminal(int mode,int state,const post03::NativeAux& a,const post03::NativeAux& b,bool bck5,bool bck6,bool stopped)noexcept{
 if(mode!=4||state<1||state>3||!a.valid||!b.valid||!post05::same_anim(a,b)||!a.anim_ok||a.anim_attr!=0||!stopped||a.anim_rate!=0||
    !lnks1fix::finite(a.anim_frame)||a.anim_frame<0||!lnks1fix::finite(a.speed)||bck5==bck6)return Terminal::None;
 const int end=bck5?42:47;if(a.anim_end!=end||a.anim_frame>float(end))return Terminal::None;
 if(state==3)return a.anim_frame<=.01f?Terminal::RecoilEnd:Terminal::None;
 if((state==1)!=bck5)return Terminal::None;
 return a.anim_frame>=float(end)-.01f?Terminal::ForwardEnd:Terminal::None;
}
inline bool ordinary_link_sword(const post03::HitReport& r,bool from_link)noexcept{
 return from_link&&r.at_valid&&r.fam.family==post03::Family::Sword&&r.j.kind==post03::HitKind::Confirmed&&!r.j.lethal&&r.post.health>1&&
  r.pre.mode==4&&r.post.mode==5&&r.post.state>=0&&r.post.state<=2&&r.pre.cooldown==0&&r.post.cooldown==10&&!r.pre.cutdown&&!r.post.cutdown;
}
// This asks for the EXISTING paired cc_set registration lease; it never clears
// a hit flag or enables a hitbox for the collision pass. Normal guard and real
// nonblocked contacts need the same final-pass cleanup as a confirmed parry.
inline bool cleanup_contact(const post08::Input& p,const post08::Verdict& v)noexcept{
 return p.fresh&&p.tracked&&!p.dead&&!p.frozen&&p.hp>1&&!p.event_running&&p.link_ok&&p.pass.valid&&v.first_contact&&
  p.mode==4&&(p.state==1||p.state==2)&&((p.at_hit[0]&&p.at_on_link[0])||(p.at_hit[1]&&p.at_on_link[1]));
}
}
