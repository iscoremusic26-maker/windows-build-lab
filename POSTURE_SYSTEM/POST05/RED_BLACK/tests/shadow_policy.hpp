#pragma once
// OFFLINE research policy. No actor pointers, SDK, writer, or runtime integration.
// A phase candidate is never permission to write. Release remains closed until
// profile authority and full contact/animation interaction have been validated.
#include <cstdint>
namespace rb_shadow {
enum class Gate { CommonUnverified, AttackRateInitPending, Frame12RateUnverified,
 AnimationUnknown, PowerChanged, Power3Posthook, ProfileUnverified, MaturePhaseCandidate };
struct Input {
 bool common_verified=false, profile_verified=false, clip_b=false, clip_c=false, stopped=false;
 int state=0, mode=4, playmode=0, power_pre=2,power_post=2;
 float frame=0, rate=1, end=0;
};
inline Gate phase(const Input& x) {
 if (!x.common_verified) return Gate::CommonUnverified;
 if (x.mode!=4 || x.state<0 || x.state>2) return Gate::AnimationUnknown;
 if (x.state==0) return Gate::AttackRateInitPending;
 if (!(x.frame>2)) return Gate::Frame12RateUnverified;
 if (x.stopped || x.playmode!=0 || !((x.state==1&&x.clip_b&&!x.clip_c)||(x.state==2&&x.clip_c&&!x.clip_b)) ||
     !(x.end>25&&x.end<1000&&x.frame<x.end&&x.frame+x.rate<x.end) ||
     !(x.rate==1.0f||x.rate==1.15f||x.rate==1.30f||x.rate==2.0f)) return Gate::AnimationUnknown;
 if (x.power_pre!=x.power_post) return Gate::PowerChanged;
 if (x.power_pre==3) return Gate::Power3Posthook;
 if (!(x.power_pre==1||x.power_pre==2||x.power_pre==6) || !x.profile_verified) return Gate::ProfileUnverified;
 return Gate::MaturePhaseCandidate;
}
inline constexpr bool release_enabled=false;
}
