#include "../provider/src/encounter_core.hpp"
#include <fstream>
#include <iostream>
int main(int argc,char** argv){if(argc!=2){std::cerr<<"Uso: encounter_validate encounters.export.json\n";return 2;}std::ifstream in(argv[1],std::ios::binary);if(!in)return 2;std::string raw(16385,' ');in.read(raw.data(),raw.size());raw.resize(size_t(in.gcount()));encounter::Collection out;native_config::Error e;if(!encounter::decode(raw,out,true,e)){std::cerr<<"REJECTED "<<e.code<<"\n";return 1;}std::cout<<"ENCOUNTER_EXPORT_VALID count="<<out.size()<<" runtime_applied=false\n";return 0;}
