#pragma once
// CP12 Phase-A instrument (pure, no game headers): what the user's 0.8.9 log could not tell, measured in the game itself.
// mod_update runs once per SIMULATION tick (fapGm_Execute -> duskExecute -> ModLoader::tick, f_ap_game.cpp 807; m_Do_main.cpp runs
// game_clock::numSimTicks of them per presented frame). The wall-clock interval between two calls is therefore ~33.3 ms at a steady 30 Hz
// simulation; a hitch of the whole frame (CPU or GPU-bound present, I/O, logging, streaming) shows up as a LONG interval, often followed by
// short catch-up intervals. This is NOT a GPU timer and not the presented-frame time; it is an honest, cheap stutter detector.
// The mod's own CPU time (PRE + POST hooks + mod_update) is accumulated separately, so the share of the mod in any hitch is visible.
// One summary line per kWindowTicks; nothing per frame.
#include <cstdint>
namespace devf_pw {
constexpr unsigned kWindowTicks=300;              // ~10 s of game time, same cadence as Dusklight's own "fapGm_Execute frame=N" markers
constexpr unsigned kBucketUs=500,kBuckets=400;    // 0.5 ms buckets up to 200 ms; longer intervals go to the overflow bucket
constexpr std::uint64_t kPauseUs=1000000;         // >= 1 s between two sim ticks: load / pause / window drag, reported apart, not a hitch
struct Window {
 std::uint16_t hist[kBuckets+1]{};unsigned n=0,over40=0,over50=0,over67=0,catchup=0,paused=0;
 std::uint64_t sum_us=0,max_us=0;
 std::uint64_t mod_ticks=0,mod_tick_max_ticks=0,cur_tick_ticks=0;unsigned hook_calls=0;   // raw counter ticks, converted once per window
 unsigned aura_max=0,created=0,eye_swaps=0,tracked_max=0;std::uint64_t aura_sum=0;unsigned ticks=0;
 void add_interval(std::uint64_t us) noexcept {
  if(us>=kPauseUs){++paused;return;}
  ++n;sum_us+=us;if(us>max_us)max_us=us;
  over40+=us>=40000;over50+=us>=50000;over67+=us>=66700;catchup+=us<8000;
  const std::uint64_t b=us/kBucketUs;++hist[b<kBuckets?b:kBuckets];
 }
 void add_mod(std::uint64_t ticks) noexcept {mod_ticks+=ticks;cur_tick_ticks+=ticks;}
 void end_tick(unsigned auras,unsigned tracked) noexcept {
  ++ticks;aura_sum+=auras;if(auras>aura_max)aura_max=auras;if(tracked>tracked_max)tracked_max=tracked;
  mod_tick_max_ticks=cur_tick_ticks>mod_tick_max_ticks?cur_tick_ticks:mod_tick_max_ticks;cur_tick_ticks=0;
 }
 // Upper edge (us) of the bucket holding the p-th per-mille interval; exact to the 0.5 ms bucket width.
 std::uint64_t percentile_us(unsigned pm) const noexcept {
  if(!n)return 0;
  const std::uint64_t rank=(std::uint64_t(n)*pm+999)/1000;   // 1-based nearest-rank
  std::uint64_t seen=0;
  for(unsigned b=0;b<=kBuckets;++b){seen+=hist[b];if(seen>=rank){const std::uint64_t edge=b<kBuckets?std::uint64_t(b+1)*kBucketUs:max_us;return edge<max_us?edge:max_us;}}   // never above the observed max
  return max_us;
 }
 bool full() const noexcept {return ticks>=kWindowTicks;}
};
// Converts performance-counter ticks to microseconds with a frequency calibrated against the system clock (no extra import needed).
struct Clock {
 std::uint64_t qpc0=0,ft0=0,hz=10000000;bool started=false;
 void calibrate(std::uint64_t qpc,std::uint64_t ft100ns) noexcept {
  if(!started){qpc0=qpc;ft0=ft100ns;started=true;return;}
  const std::uint64_t dft=ft100ns-ft0,dq=qpc-qpc0;
  if(ft100ns>ft0&&dft>=5000000ull&&dq)hz=static_cast<std::uint64_t>(double(dq)*1.0e7/double(dft));   // >= 0.5 s of system time; double: no overflow for GHz counters
 }
 std::uint64_t us(std::uint64_t ticks) const noexcept {return hz?ticks*1000000ull/hz:0;}
};
} // namespace devf_pw
