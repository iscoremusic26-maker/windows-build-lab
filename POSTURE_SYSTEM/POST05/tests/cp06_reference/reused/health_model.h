#pragma once
#include <stdint.h>

// Reconstructed from the selected Twilit Essentials 2.6.7 routines.
// No game pointers, native offsets, health writes, or allocations in this layer.
namespace ehz {
enum class Boss : uint8_t {
    None, Diababa, Fyrus, Morpheel, Stallord, Blizzeta, Armogohma,
    Argorok, BeastGanon, Ganondorf, PuppetZelda, Zant, Darknut,
    Aeralfos, DeathSword, KingBulblin, Darkhammer, Ook, DekuToad,
    Dangoro, PhantomZant
};
inline float clamp01(float x) { return !(x > 0) ? 0 : (x < 1 ? x : 1); }
inline int maximum(int a,int b) { return a>b?a:b; }
inline int minimum(int a,int b) { return a<b?a:b; }
struct BossSample {
    Boss kind;
    int health, maximumHealth;
    int action, mode, phase, hits, cycles, damage, damageLimit;
    float healthScale=1.0f, cycleScale=1.0f;
    bool dungeonComplete=false;
};
// This is the source mod's *instantaneous* metric, separate from its phase
// presentation state machine. Phase refill, fade, and selection are not here.
inline float instantaneous(const BossSample& x) {
    float scale=x.healthScale>0?x.healthScale:1.0f;
    float hp=float(maximum(0,x.health));
    switch(x.kind) {
    case Boss::Fyrus: {
        int n=maximum(1,int(3.0f*x.cycleScale+0.5f));
        int down=minimum(n,maximum(0,x.cycles));
        if(down>=n)return x.action==12?0.0f:1.0f/n;
        return clamp01((n-down+clamp01(hp/(50.0f*x.cycleScale)))/n);
    }
    case Boss::Morpheel:
        if(x.action<=99&&!x.phase)return clamp01(hp/(30*scale));
        else {
            int n=maximum(1,int(4*x.cycleScale+0.5f));
            return 1-clamp01(float(n*minimum(3,maximum(0,x.cycles))+
                minimum(n,maximum(0,x.hits)))/(n*3));
        }
    case Boss::Stallord:
        return x.phase?clamp01(hp/(1080*scale)):1-minimum(3,x.hits)/3.0f;
    case Boss::Blizzeta:
        return x.phase<7?1-clamp01((maximum(1,x.phase)-1)/6.0f):
            1-minimum(3,x.hits)/3.0f;
    case Boss::Armogohma:
        return 1-clamp01(float(x.hits)/maximum(1,int(3*scale+0.5f)));
    case Boss::Argorok:
        return x.phase==2?clamp01(hp/(24*scale)):1-clamp01(x.hits*0.5f);
    case Boss::BeastGanon:
        return x.action==9?0:clamp01(hp/(700*scale));
    case Boss::Ganondorf:
        return clamp01(hp/((x.phase?24.0f:(x.action>0&&x.action<7?24.0f:100.0f))*scale));
    case Boss::Zant:
        return !x.phase||x.action==4?1:clamp01(hp/((x.phase==6?600.0f:280.0f)*scale));
    case Boss::Darknut:
        return 1-clamp01(x.action<=7?x.hits/12.0f:
            float(x.damage)/(x.damageLimit>0?x.damageLimit:360));
    case Boss::Aeralfos:
        return clamp01(hp/(x.maximumHealth>0?float(x.maximumHealth):700.0f));
    case Boss::DeathSword:
        return x.action==20||x.dungeonComplete?0:1-clamp01(float(minimum(
            int(800*scale),maximum(0,x.damage)))/(800*scale));
    case Boss::Dangoro:
        return x.cycles<=0?1:1-clamp01(float(3*minimum(3,x.cycles)+
            minimum(2,maximum(0,x.hits))-2)/9);
    default:
        return x.maximumHealth>0?clamp01(hp/x.maximumHealth):(hp>0?1:0);
    }
}

struct Identity { uintptr_t address; uint32_t id; int16_t profile; };
inline bool same(Identity a,Identity b) {
    return a.address==b.address&&a.id==b.id&&a.profile==b.profile;
}
struct HealthTrack {
    Identity identity{};
    uint32_t epoch=0,lastTick=0;
    int maximumHealth=0;
    float fill=1,trail=1,alpha=0,anchorHeight=0;
    bool occupied=false;
    void reset(Identity who,uint32_t scene,uint32_t tick,int hp) {
        identity=who;epoch=scene;lastTick=tick;maximumHealth=maximum(1,hp);
        fill=trail=clamp01(float(hp)/maximumHealth);alpha=0;anchorHeight=0;occupied=true;
    }
    void update(uint32_t tick,int hp,float targetAlpha) {
        if(tick==lastTick)return; // draw can run more often than simulation
        lastTick=tick;
        maximumHealth=maximum(maximumHealth,hp);
        fill=clamp01(float(maximum(0,hp))/maximum(1,maximumHealth));
        trail=fill>=trail?fill:trail+(fill-trail)*0.12f;
        alpha+=(clamp01(targetAlpha)-alpha)*0.2f;
        if(alpha<0.005f)alpha=0;
    }
};
}
