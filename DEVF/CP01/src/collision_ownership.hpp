#pragma once

namespace devf::cp01 {
// cBgS's exact table has 256 slots. Never use an actor's stale BG index to unlink another object.
template<class Ops> bool release_owned_collision(Ops& ops) {
    if (!ops.present()) return false;
    int found = -1;
    for (int index = 0; index < 256; ++index) {
        if (!ops.references_own_at(index)) continue;
        if (found != -1) return false; // duplicated world reference cannot be silently released
        found = index;
    }
    if (found == -1) {
        // Scene teardown already dropped every world reference. Only reset our own local ID.
        ops.retire_local();
        return true;
    }
    if (!ops.used() || ops.id() != found || !ops.owner_matches(found)) return false;
    return ops.release_world();
}
}
