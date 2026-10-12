#pragma once

namespace devf::cp01 {
// Shared by the real actor and the Windows laboratory. Engine operations are supplied by Ops.
// A pending resource request is not a completed actor. Solid-heap memory belongs to the engine.
class ResourceLifecycle {
    bool requested_ = false;
    bool heap_ready_ = false;
    bool collider_registered_ = false;
    bool ready_ = false;
    bool released_ = false;
    bool failed_ = false;
public:
    bool ready() const { return ready_; }
    template<class Ops> int advance(Ops& ops) {
        if (released_ || failed_) return ops.error_step;
        if (ready_) return ops.complete_step;
        requested_ = true;
        const int phase = ops.load_archive();
        if (phase != ops.complete_step) {
            if (phase == ops.error_step || phase == ops.cancelled_step) failed_ = true;
            return phase;
        }
        if (!heap_ready_) {
            if (!ops.create_heap()) { failed_ = true; return ops.error_step; }
            heap_ready_ = true;
        }
        if (!collider_registered_) {
            if (!ops.register_collision()) { failed_ = true; return ops.error_step; }
            collider_registered_ = true;
        }
        if (!ops.initialize_scene()) { failed_ = true; return ops.error_step; }
        ready_ = true;
        return ops.complete_step;
    }
    template<class Ops> bool release(Ops& ops) {
        if (released_) return true;
        // Remove world references before releasing archive data and actor heap references.
        if (collider_registered_) {
            if (!ops.release_collision()) return false;
            collider_registered_ = false;
        }
        if (requested_) ops.release_archive();
        ops.clear_heap_references();
        requested_ = heap_ready_ = collider_registered_ = ready_ = false;
        released_ = true;
        return true;
    }
};
}
