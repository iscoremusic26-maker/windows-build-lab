// LAB REFERENCE (not shipped): Bokoblin Expansion 0.17.0 CP19 FIX02 B shield chip.
// Source: iscoremusic26-maker/Mods-Duskligh fe47c8b828939829914d466a425cb1e2c0223cd9,
// DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/mod.cpp
// (file SHA-256 2cfb0ea974b9631f8d4c228d2c71f140cb7ef5f10a7cc81a3b0ea11993a84a5e), lines 1682-1720:
// cp19f2_shield_pair() and cp19f2_chip_post(). Statement order, predicate, chip_step() call (vendored
// cp19f2_policy.hpp, same commit) and the single pending-life write are kept; logging, flash rendering
// and the dyn-ledger indexing are replaced by a slot-indexed array. Executed in POST priority 0.
std::array<devf_cp19f2::ChipBlow,64> expansion_chip_blows{};
struct ExpansionChipCounts{unsigned applied=0,duplicates=0,not_fury=0,stale=0,no_blow=0,skipped=0,readback_failed=0;}expansion_chip_counts;
bool expansion_shield_pair(daE_OC_c* a,daAlink_c* k,int& cyl,int& sph){
 for(int j=0;j<2;++j){
  auto& s=a->mSphs_at[j];const bool at_hit=s.cCcD_ObjHitInf::ChkAtHit()!=0,at_shield=at_hit&&s.ChkAtShieldHit();
  const void* at_obj=at_hit?static_cast<const void*>(s.cCcD_ObjHitInf::GetAtHitObj()):nullptr;const void* sphere=static_cast<const void*>(static_cast<cCcD_Obj*>(&s));
  for(int i=0;i<3;++i){
   auto& c=k->mTgCyls[i];const bool tg_hit=c.cCcD_ObjHitInf::ChkTgHit()!=0,tg_shield=tg_hit&&c.ChkTgShieldHit();
   const void* tg_obj=tg_hit?static_cast<const void*>(c.cCcD_ObjHitInf::GetTgHitObj()):nullptr;
   if(devf_cp19f2::shield_confirmed(at_hit,at_shield,at_obj,static_cast<const void*>(static_cast<cCcD_Obj*>(&c)),tg_hit,tg_shield,tg_obj,sphere)){cyl=i;sph=j;return true;}
  }
 }
 return false;
}
void expansion_chip_post(daE_OC_c* a,unsigned index,const devf_black_ai::State& s,unsigned fury,const devf_black_ai::Actor& f){
 daAlink_c* k=dComIfGp_getLinkPlayer();
 int cyl=-1,sph=-1;devf_cp19f2::ChipView v;v.id=f.id;v.gen=f.gen;v.seq=s.seq;v.fury=fury>0;v.at_set=f.at_set;v.live=f.health>0;
 v.shield=k&&expansion_shield_pair(a,k,cyl,sph);
 auto& b=expansion_chip_blows[index];
 const auto op=devf_cp19f2::chip_step(b,v);
 using devf_cp19f2::ChipOp;
 if(op==ChipOp::None)return;
 auto& c=expansion_chip_counts;
 if(op!=ChipOp::Charge){if(op==ChipOp::Duplicate)++c.duplicates;else if(op==ChipOp::NotFury)++c.not_fury;else if(op==ChipOp::NotRegistered)++c.stale;else ++c.no_blow;return;}
 if(!k||dComIfGs_getLife()==0||dComIfGp_event_runCheck()){++c.skipped;return;}
 const float before=dComIfGp_getItemLifeCount();const unsigned char type=dComIfGp_getItemLifeCountType();
 dComIfGp_setItemLifeCount(-float(devf_cp19f2::kChipLifeUnits),type);
 const float after=dComIfGp_getItemLifeCount();
 if(!devf_cp19f2::chip_readback_ok(before,after)){++c.readback_failed;return;}
 ++c.applied;
}
