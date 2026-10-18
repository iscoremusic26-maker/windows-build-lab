// CP11/CP12 local microbenchmark + parity driver (included at the end of the generated bench TU). Measures the MOD's own CPU work in
// visual_post with fake engine calls and checks parity per variant:
//  * Blue: FNV of everything the mod writes to its emitter / passes to set() every tick (must equal the previous version);
//  * Black: the same FNV WITHOUT the colour fields (geometry, frame, alpha, presence, position must be equal), plus the colour it writes;
//  * Blue eyes (CP12): a fake E_OC model with a green-eyed CMPR atlas; the eye packet texture is traced through ON -> OFF.
// It says nothing about the real engine, the GPU or in-game FPS.
#include <algorithm>
#include <chrono>
#include <string>
#include <vector>
#include <cstdlib>
namespace bench {
struct Sink {unsigned long long lines=0,bytes=0;std::string copy;};
Sink g;bool print=false;
void write(void*,int,const char* s){++g.lines;const std::size_t n=std::strlen(s);g.bytes+=n;g.copy.assign(s,n);if(print)std::puts(s);}   // the host copies every line into a std::string
struct Fnv {std::uint64_t h=1469598103934665603ull;void mix(const void* p,std::size_t n){const auto* b=static_cast<const unsigned char*>(p);for(std::size_t i=0;i<n;++i){h^=b[i];h*=1099511628211ull;}}};
void trace(Fnv& f,bool colour){JPABaseEmitter* e=dComIfGp_particle_getEmitter(1);
 f.mix(&e->ds,sizeof e->ds);f.mix(&e->ps,sizeof e->ps);f.mix(&e->mLocalDir,sizeof e->mLocalDir);f.mix(&e->mLocalTrs,sizeof e->mLocalTrs);f.mix(&e->mLocalScl,sizeof e->mLocalScl);
 f.mix(&e->mLocalRot,sizeof e->mLocalRot);f.mix(&e->mVolumeSweep,sizeof e->mVolumeSweep);f.mix(&e->mVolumeMinRad,sizeof e->mVolumeMinRad);f.mix(&e->mPrmClr,sizeof e->mPrmClr);
 f.mix(&e->mRateStep,sizeof e->mRateStep);f.mix(&e->prmc.a,sizeof e->prmc.a);if(colour){f.mix(&e->prmc.r,3);}f.mix(&g_last_pos,sizeof g_last_pos);}
double ns(std::chrono::steady_clock::time_point a,std::chrono::steady_clock::time_point b){return double(std::chrono::duration_cast<std::chrono::nanoseconds>(b-a).count());}
void stats(const char* name,std::vector<double>& v){std::sort(v.begin(),v.end());auto q=[&](double p){return v[std::size_t(p*(v.size()-1))];};
 std::printf("BENCH %-26s n=%zu median_ns=%.0f p95_ns=%.0f p99_ns=%.0f\n",name,v.size(),q(0.5),q(0.95),q(0.99));}
// A fake E_OC-like model: 3 materials, eye material "oc_eye_mat", one 128x128 CMPR atlas whose first 64 blocks are green "eyes".
struct FakeModel {
 alignas(32) unsigned char buf[3*sizeof(ResTIMG)+64+8192]{};J3DModelData data;J3DModel model;Morf morf;J3DTexture* tex=nullptr;
 FakeModel(){
  auto* res=reinterpret_cast<ResTIMG*>(buf);for(int i=0;i<3;++i)new(&res[i]) ResTIMG{};
  unsigned char* img=buf+((3*sizeof(ResTIMG)+31)&~std::size_t(31));
  for(int i=0;i<3;++i)res[i].imageOffset=s32(img-reinterpret_cast<unsigned char*>(&res[i]));
  for(std::size_t o=0,b=0;o<8192;o+=8,++b){const bool eye=b<64;const devf_tex::RGB c0=eye?devf_tex::RGB{70,190,60}:devf_tex::RGB{120,90,70},c1=eye?devf_tex::RGB{30,120,40}:devf_tex::RGB{80,60,45};
   devf_tex::wr16(img+o,devf_tex::pack565(c0));devf_tex::wr16(img+o+2,devf_tex::pack565(c1));devf_tex::wr32(img+o+4,0x1B1B1B1Bu);}
  tex=new J3DTexture(3,res);data.t=tex;model.d=&data;for(int i=0;i<3;++i){model.p[i].m=&data.mat[i];model.p[i].mpTexture=tex;}
  data.mat[0].mMaterialName="oc_body_mat01";data.mat[1].mMaterialName="oc_body_mat02";morf.m=&model;
 }
};
}
int main(int argc,char** argv){
 const unsigned cycles=argc>1?unsigned(std::atoi(argv[1])):2000;
 g_svc.write=bench::write;bench::print=argc>2;
 std::vector<double> act,steady,idle;unsigned long long lines_cycle=0,bytes_cycle=0;
 bench::Fnv blue_full,black_geo;int black_rgb[3]{-1,-1,-1};unsigned eye_on_ok=0,eye_off_ok=0,eye_checks=0;
 for(devf_variant::Variant var:{devf_variant::Variant::Blue,devf_variant::Variant::Black}){
  bench::FakeModel fm;
  daE_OC_c a;a.eyePos=cXyz(0,166,0);a.mSphs_cc[0].r=50;a.mSphs_cc[1].r=45;a.mpMorf=&fm.morf;
  devf_variant::Entry e;e.used=true;e.id=7;e.variant=var;e.source=devf_variant::Source::NativeEOC;
  auto* d=ledger.record(7,&a,1,var,var==devf_variant::Variant::Black?3:1);d->owned=true;
  daE_OC_c idle_a;devf_variant::Entry ie;ie.used=true;ie.id=9;ie.variant=devf_variant::Variant::Red;auto* idd=ledger.record(9,&idle_a,1,devf_variant::Variant::Red,2);
  for(unsigned c=0;c<cycles;++c){
   visual_logs=0;if(auto* v=vledger.find(7))v->events=0;
   const auto l0=bench::g.lines,b0=bench::g.bytes;
   for(unsigned t=0;t<80;++t){
    const bool on=t<60;if(var==devf_variant::Variant::Black)d->fury=on?100u:0u;else d->group=on;
    a.current.pos=cXyz(float(t%7),0.0f,float(t%5));a.mSphs_cc[0].cx=12.0f+float(t%3);a.mSphs_cc[1].cx=40.0f;
    const auto s=std::chrono::steady_clock::now();visual_post(&a,7,&e,d);const auto f=std::chrono::steady_clock::now();
    if(var==devf_variant::Variant::Blue)bench::trace(blue_full,true);else{bench::trace(black_geo,false);if(on&&t>12){auto* em=dComIfGp_particle_getEmitter(1);black_rgb[0]=em->prmc.r;black_rgb[1]=em->prmc.g;black_rgb[2]=em->prmc.b;}}
    if(var==devf_variant::Variant::Blue&&c<20){                          // eye packet: cyan variant while on, the ORIGINAL texture after the fade-out
     J3DTexture* now=fm.model.p[2].mpTexture;
     if(t==20){++eye_checks;if(now!=fm.tex)++eye_on_ok;}
     if(t==79){++eye_checks;if(now==fm.tex)++eye_off_ok;}
    }
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
 std::printf("PARITY blue_emitter_trace_fnv=%016llx black_emitter_trace_without_colour_fnv=%016llx black_global_prm_rgb=(%d,%d,%d)\n",
  static_cast<unsigned long long>(blue_full.h),static_cast<unsigned long long>(black_geo.h),black_rgb[0],black_rgb[1],black_rgb[2]);
 std::printf("EYES blue_eye_packet_cyan_while_on=%u/%u blue_eye_packet_native_after_off=%u/%u\n",eye_on_ok,eye_checks/2,eye_off_ok,eye_checks/2);
}
