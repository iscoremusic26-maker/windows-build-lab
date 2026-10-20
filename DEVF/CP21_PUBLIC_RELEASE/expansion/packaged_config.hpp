#pragma once
#include "../runtime/DEVF/CP10_2_BLACK_BOKOBLIN/expansion/config_io.hpp"
#include <cp21_frozen_encounters.hpp>
namespace packaged_config {
inline ModResult load(multi_config::Config& out,const char*& reason) noexcept {
 out={};
 try {
  if(!multi_config::decode(std::string(cp21_frozen::json,sizeof(cp21_frozen::json)-1),out,reason))return MOD_INVALID_ARGUMENT;
  if(out.count!=32){out={};reason="PACKAGED_COUNT_MISMATCH_ZERO_REQUESTS";return MOD_INVALID_ARGUMENT;}
  reason="PACKAGED_ENCOUNTERS_VALID_COLD_START";return MOD_OK;
 }catch(...){out={};reason="PACKAGED_DECODE_EXCEPTION_ZERO_REQUESTS";return MOD_ERROR;}
}
}
