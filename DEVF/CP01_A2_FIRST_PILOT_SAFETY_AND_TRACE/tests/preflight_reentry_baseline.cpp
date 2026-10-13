#include "../../CP01_A2_FIRST_NATIVE_REQUEST_OFFLINE/tests/fake.hpp"
#include <iostream>
// Focused known-defect reproducer only, not the historical93-check suite.
int main(){unsigned observed=0;for(int phase:{1,2}){
 devf::first::Controller c;lab::Fake b;
 b.before_read=[&](lab::Fake& x){if(x.reads==phase)c.attempt(b,lab::permit(b.frame));};
 c.attempt(b,lab::permit(b.frame));
 const bool defect=b.calls==1&&c.state()==devf::first::State::Requested&&b.layer==0;
 std::cout<<"BASELINE_DEFECT_"<<(defect?"OBSERVED":"NOT_REPRODUCED")<<" preflight="<<phase<<" fake_requests="<<b.calls<<" old_Stop_overwritten="<<defect<<" game=NOT_EXECUTED\n";
 observed+=defect;
 }std::cout<<"SUMMARY focused-baseline-reproducer="<<observed<<"/2 expected-defects; fake-only\n";return observed==2?0:1;}
