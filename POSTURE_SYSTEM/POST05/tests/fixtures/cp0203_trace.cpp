#include "ROOT/src/break_core.hpp"
#include <cstdio>
#include <initializer_list>
int main(){for(int rate:{10,7,4,2,1,0}){
 post03::Ledger L;post03::Events ev;auto k=L.observe(7,0x10000,post03::Admission::E_OC_CANDIDATE,1,1,ev);post04::Posture p;p.sync(L);post03::HitReport h;h.slot=k.slot;h.id=7;h.generation=k.generation;h.life=post03::Life::Active;h.j.kind=post03::HitKind::Confirmed;h.j.elig=post03::Eligibility::Confirmed;h.at_valid=true;h.fam.family=post03::Family::Sword;h.post.health=500;for(int i=1;i<=3;++i){h.event=i;p.consume(h,L);}
 for(int tick=1;tick<=480;++tick){const int rates[]={10,7,4,2,1};const int wanted=rate?rate:rates[(tick/17)%5];const bool ok=p.set_regen_rate(k.slot,k.generation,7,static_cast<unsigned char>(wanted));p.advance();std::printf("T,%d,%d,%d,%d,%d,%d\n",rate,tick,wanted,int(ok),int(p.entry(k.slot).posture),int(p.entry(k.slot).regen_acc));}
 }
 for(int hp:{1,10,11,25,26,50,51,75,76,100})std::printf("B,%d,%d\n",hp,int(post06::policy::regen_rate_for(hp,100)));
 for(float frame:{0.f,1.f,8.f,8.0001f,9.f,10.f})for(unsigned age:{9u,10u,11u}){post08::LinkView v;v.valid=v.guard_attack=true;v.start=1;v.frame=frame;v.age_ticks=age;std::printf("P,%.7g,%u,%d\n",frame,age,int(post08::Parry::in_window(v)));}
}
