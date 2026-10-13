#pragma once
#include "experiment_core.hpp"
#include <mods/svc/actor.h>
namespace devf::first {
class NativeBackend final:public Backend {
 ModContext* ctx_; // official module context, NOT actor/layer pointers
public:
 explicit NativeBackend(ModContext* ctx):ctx_(ctx){}
 bool service_available()const override;
 Frame preflight()override{return read_only_preflight();}
 Id current()override;
 bool registered(Id)override;
 void select(Id)override;
 RequestResult request(const Preset&)override;
 Observation observe(Id)override;
};
}
extern "C" void devf_first_request_link_anchor(devf::first::Controller&,devf::first::NativeBackend&,const devf::first::Admission&);
