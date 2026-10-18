// CP11 local microbenchmark + parity driver (included at the end of the generated bench TU). Measures the MOD's own CPU work in visual_post
// with fake engine calls: activation tick (emitter created + its log lines), steady active tick, inactive actor. Prints median / p95 / p99,
// log lines and bytes per ON->OFF cycle, and an FNV hash of everything the mod writes to the emitter / passes to set() every tick (parity).
#include <algorithm>
#include <chrono>
#include <string>
#include <vector>
#include <cstdlib>
namespace bench {
struct Sink {unsigned long long lines=0,bytes=0;std::string copy;};
Sink g;
bool print=false;
void write(void*,int,const char* s){++g.lines;const std::size_t n=std::strlen(s);g.bytes+=n;g.copy.assign(s,n);if(print)std::puts(s);}   // the host copies every line into a std::string
std::uint64_t h=1469598103934665603ull;
void mix(const void* p,std::size_t n){const auto* b=static_cast<const unsigned char*>(p);for(std::size_t i=0;i<n;++i){h^=b[i];h*=1099511628211ull;}}
void trace(){JPABaseEmitter* e=dComIfGp_particle_getEmitter(1);
 mix(&e->ds,sizeof e->ds);mix(&e->ps,sizeof e->ps);mix(&e->mLocalDir,sizeof e->mLocalDir);mix(&e->mLocalTrs,sizeof e->mLocalTrs);mix(&e->mLocalScl,sizeof e->mLocalScl);
 mix(&e->mLocalRot,sizeof e->mLocalRot);mix(&e->mVolumeSweep,sizeof e->mVolumeSweep);mix(&e->mVolumeMinRad,sizeof e->mVolumeMinRad);mix(&e->mPrmClr,sizeof e->mPrmClr);
 mix(&e->mRateStep,sizeof e->mRateStep);mix(&e->prmc,sizeof e->prmc);mix(&g_last_pos,sizeof g_last_pos);mix(&g_set_calls,sizeof g_set_calls);}
double ns(std::chrono::steady_clock::time_point a,std::chrono::steady_clock::time_point b){return double(std::chrono::duration_cast<std::chrono::nanoseconds>(b-a).count());}
void stats(const char* name,std::vector<double>& v){std::sort(v.begin(),v.end());auto q=[&](double p){return v[std::size_t(p*(v.size()-1))];};
 std::printf("BENCH %-26s n=%zu median_ns=%.0f p95_ns=%.0f p99_ns=%.0f\n",name,v.size(),q(0.5),q(0.95),q(0.99));}
}
int main(int argc,char** argv){
 const unsigned cycles=argc>1?unsigned(std::atoi(argv[1])):2000;
 g_svc.write=bench::write;bench::print=argc>2;   // any 2nd argument: print the log lines
 std::vector<double> act,steady,idle;unsigned long long lines_cycle=0,bytes_cycle=0;
 for(devf_variant::Variant var:{devf_variant::Variant::Blue,devf_variant::Variant::Black}){
  daE_OC_c a;a.eyePos=cXyz(0,166,0);a.mSphs_cc[0].r=50;a.mSphs_cc[1].r=45;
  devf_variant::Entry e;e.used=true;e.id=7;e.variant=var;e.source=devf_variant::Source::NativeEOC;
  auto* d=ledger.record(7,&a,1,var,var==devf_variant::Variant::Black?3:1);d->owned=true;
  daE_OC_c idle_a;devf_variant::Entry ie;ie.used=true;ie.id=9;ie.variant=devf_variant::Variant::Red;auto* idd=ledger.record(9,&idle_a,1,devf_variant::Variant::Red,2);
  for(unsigned c=0;c<cycles;++c){
   visual_logs=0;if(auto* v=vledger.find(7))v->events=0;                  // representative of an early-session activation (logs not yet capped)
   const auto l0=bench::g.lines,b0=bench::g.bytes;
   for(unsigned t=0;t<80;++t){
    const bool on=t<60;if(var==devf_variant::Variant::Black)d->fury=on?100u:0u;else d->group=on;
    a.current.pos=cXyz(float(t%7),0.0f,float(t%5));a.mSphs_cc[0].cx=12.0f+float(t%3);a.mSphs_cc[1].cx=40.0f;   // the actor moves: the ring must follow
    const auto s=std::chrono::steady_clock::now();visual_post(&a,7,&e,d);const auto f=std::chrono::steady_clock::now();
    bench::trace();
    if(c>=10){if(t==0)act.push_back(bench::ns(s,f));else if(on)steady.push_back(bench::ns(s,f));}
    const auto s2=std::chrono::steady_clock::now();visual_post(&idle_a,9,&ie,idd);const auto f2=std::chrono::steady_clock::now();
    if(c>=10)idle.push_back(bench::ns(s2,f2));
   }
   if(c>=10){lines_cycle+=bench::g.lines-l0;bytes_cycle+=bench::g.bytes-b0;}
  }
  visual_drop(7,"x");ledger.release(*d);ledger.release(*idd);
 }
 const unsigned measured=2*(cycles-10);
 bench::stats("activation_tick",act);bench::stats("steady_active_tick",steady);bench::stats("inactive_actor_tick",idle);
 std::printf("BENCH log_lines_per_on_off_cycle=%.2f log_bytes_per_on_off_cycle=%.1f\n",double(lines_cycle)/measured,double(bytes_cycle)/measured);
 std::printf("PARITY emitter_state_trace_fnv=%016llx set_calls=%u\n",static_cast<unsigned long long>(bench::h),g_set_calls);
}
