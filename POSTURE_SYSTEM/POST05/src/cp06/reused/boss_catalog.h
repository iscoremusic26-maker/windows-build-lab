#pragma once
namespace ehz {
struct BossDescriptor {int profile;const char*name;bool miniboss,primary;};
constexpr BossDescriptor bosses[]={
{524,"Diababa",false,true},
{523,"Diababa",false,false},
{475,"Diababa",false,false},
{471,"Fyrus",false,true},
{243,"Morpheel",false,true},
{210,"Morpheel",false,false},
{528,"Morpheel",false,false},
{246,"Stallord",false,true},
{529,"Blizzeta",false,true},
{525,"Armogohma",false,true},
{247,"Argorok",false,true},
{533,"Argorok",false,false},
{534,"Dark Beast Ganon",false,true},
{526,"Ganondorf",false,true},
{459,"Puppet Zelda",false,true},
{249,"Zant",false,true},
{248,"Zant",false,false},
{250,"Zant",false,false},
{125,"Zant",false,false},
{531,"Darknut",true,true},
{532,"Aeralfos",true,true},
{520,"Death Sword",true,true},
{469,"King Bulblin",true,true},
{450,"Darkhammer",true,true},
{476,"Ook",true,true},
{512,"Deku Toad",true,true},
{433,"Dangoro",true,true},
{484,"Phantom Zant",true,true},
};
inline const BossDescriptor* boss_descriptor(int p){for(auto&d:bosses)if(d.profile==p)return &d;return nullptr;}
}
