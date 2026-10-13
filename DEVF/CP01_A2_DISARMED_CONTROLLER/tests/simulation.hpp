#pragma once
#if !defined(DEVF_A2_LAB_ONLY) || DEVF_A2_LAB_ONLY != 1
#error "Simulation is exclusively an offline laboratory component"
#endif
#include "../src/controller_core.hpp"
namespace devf::a2::lab {
// Backend is a fake only. No production implementation exists or can be selected.
template<class Fake> class LayerScope {
    Fake& f_; Id previous_; bool restored_=false, ok_=false;
public:
    explicit LayerScope(Fake& f):f_(f),previous_(f.current()) {}
    bool select(Id target) { f_.select(target); return f_.current()==target; }
    bool restore() noexcept {
        if (restored_) return ok_;
        restored_=true;
        // No restoring a known-dead context. Report failure; do not dereference anything.
        if (!f_.registered(previous_)) return false;
        try { f_.select(previous_); ok_=f_.current()==previous_; } catch (...) { ok_=false; }
        return ok_;
    }
    ~LayerScope() { restore(); }
};
template<class Fake> void attempt(Controller& c,Fake& f,bool permission,bool synthetic_position) {
    if (c.busy()) { c.fail(Reason::Reentry,true); return; }
    if (c.consumed()) return;
    Owner before{};
    try { before=f.resolve(); }
    catch (...) { c.fail(Reason::Exception,true); return; }
    if (!c.begin_simulation(before,permission,synthetic_position)) return;
    struct Release { Controller& c; ~Release() { c.end_scope(); } } release{c};
    try {
        if (!same_preflight(before,f.resolve())) { c.fail(Reason::Revalidation,true); return; }
        if (f.current()!=before.previous || !f.registered(before.previous)) { c.fail(Reason::Previous,true); return; }
        LayerScope<Fake> scope(f);
        try {
            if (!scope.select(before.child)) c.fail(Reason::Selection,true);
            else if (!same_preflight(before,f.resolve_selected(before.previous))) c.fail(Reason::Revalidation,true);
            else {
                const auto result=f.request(); // ONLY fake has this method; absent from host target.
                if (!result.ok) c.fail(Reason::Service,true);
                else c.requested(result.id);
            }
        } catch (...) { c.fail(Reason::Exception,true); }
        if (!scope.restore()) c.fail(Reason::Restore,true);
    } catch (...) { c.fail(Reason::Exception,true); }
}
}
