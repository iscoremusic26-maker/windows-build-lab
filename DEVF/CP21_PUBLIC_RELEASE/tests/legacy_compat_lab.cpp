#include "../../CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/runtime/DEVF/CP10_2_BLACK_BOKOBLIN/shared/config.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2)return 1;unsigned cases=0;
 for(const char* name:{"legacy-absent","legacy-normal","existing-black","black","blue","red","mixed","schema1-legacy","schema1-no-extension"}){
  std::ifstream f(std::filesystem::path(argv[1])/(std::string(name)+".json"),std::ios::binary);if(!f)return 2;
  std::string raw(std::istreambuf_iterator<char>(f),{});multi_config::Config c;const char* why;
  const bool expected=std::string(name)!="blue"&&std::string(name)!="red"&&std::string(name)!="mixed"&&std::string(name)!="schema1-no-extension";
  const bool actual=multi_config::decode(raw,c,why);
  if(actual!=expected||(!actual&&c.count)){std::cerr<<name<<" "<<why<<"\n";return 3;}
  std::cout<<"CASE_PASS expansion_0_16_2_"<<name<<" accepted="<<actual<<"\n";++cases;
 }
 std::cout<<"CP20_LEGACY_COMPAT_PASS cases="<<cases<<" GAME_EXECUTED=false\n";return 0;
}
