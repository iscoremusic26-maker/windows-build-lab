# Local capture schema and interpretation

C is observation-only F_SP121 / room3 / logical6, double XYZ(-4806.7346,-6729.480961460304,65135.8281). No spawner parameters contain C. Radius501 is the prior conditional resource union used only for initial query/screen; it is neither actor volume nor navigation radius. `query_f32` records actual engine probe coordinates after float conversion: x=C.x, y=C.y+501, z=C.z.

A monotonically increasing `id` correlates one synchronous update capture. Own scalar `Capture`, including copied A2 Owner, is trivially copyable. No pointers, borrowed strings, actors, resources, polygons or scene objects survive in the record. A value-only sidecar is reset before each adapted G2 read and copied when the unchanged A2 resolver returns. No cross-frame aggregation.

| Group | Fields and interpretation |
|---|---|
| Host | Version16-byte own buffer; build <=32 bytes copied as hex65-byte buffer, otherwise NOT_VERIFIED. No personal paths. |
| Context | stage, room/stay, logical layer, next_room, next_stage, base G2 outcome; `owner_result` is existing A2 Reason, enum in controller_core.hpp. root, previous/current_at_preflight, link_owner, ROOM_SCENE ID/profile/init/room parameter, child registered; PLAY_SCENE ID/profile/init, play_layer, parent, direct binding; child/parent create/delete counts, retiring, status2e mask and read flag. Actual layer IDs are observed, never fixed5/6. |
| Ground and water_group_probe | Status, highest acceptable Y strictly below probe, BG index, polygon index, room, ground/special/poly-color raw codes, BG actor-ID and moving flag. NO_HIT_AT_PROBE is native -1e9 sentinel. Codes are not an invented material admission policy. One surface at x/z does not prove full disk support or volume occupancy. |
| Dynamic BG | At most8 moving registered ready candidate AABBs, index, actor-ID, room, lock, copied bounds, nearest AABB distance, owner_valid. Validate owner against initialized actor queue, exact BG actor pointer+ID and G2 registered owner. Absent owner_valid means NOT_VERIFIED, not safe/unsafe actor verdict. Not a shape intersection query. |
| Actors | Existing G2 player-membership pass and SAME bounded census; at most12 local records. Included reason bit1=Link regardless of distance, bit2=center within501, bit4=BG-owner ID. ID/profile/current room/process owner/init, position, 3D distance. Native physical bound always unknown. Far centers alone never show physical separation. |
| Incomplete process | If creation/init/actor-init fails before safe derived reads, incomplete_actor/profile/init preserve process-header evidence only. Position is not accessed. Existing G2 outcome INCOMPLETE_CENSUS propagates into A2 owner rejection. |
| Resources | Independent query read flag; E_OC/E_ocb registry presence; READY always NOT_VERIFIED. No load/sync. |
| Session | stable, event_read, event_active; one event.runCheck flag and scene/transitions/retirement/counters only. Coach/escort-specific flags NOT_VERIFIED. |
| Completion | visits<=256 (257 means limit stop), bg_slots<=256, actors_complete/truncated, bg_scan_complete/truncated, capture_valid, partial coverage. Exact BG shape, universal transform and water-volume coverage unavailable. `no_location_admission=1` always. |

## Status mappings

* Surface Status0=NOT_VERIFIED;1=OBSERVED;2=NO_HIT_AT_PROBE;3=INVALID;4=UNAVAILABLE_WITH_SAFE_API.
* Session0=SESSION_CONTEXT_INCOMPLETE;1=SESSION_CONTEXT_COMPATIBLE_OBSERVED;2=SESSION_CONTEXT_INCOMPATIBLE_OBSERVED. Compatibility covers only the listed signals and is never a spawn authorization.
* Occupancy0=OVERLAP_NOT_VERIFIED;1=PROXIMATE_CENTER (physical overlap still not verified);2=SEPARATED_UNDER_PROVEN_BOUND;3=OVERLAP_UNDER_PROVEN_BOUND;4=INVALID. States2/3 are exercised by synthetic fixtures; the native reader does not claim a proven bound for any actor.
* Unknown Id=4294967295; room/index/code unknown=-1. Negative/unknown values are not absence claims. G2 Observation numeric mapping is preserved in historical core.hpp.

`capture_valid` certifies bounded coherent basic scalar observation only: owner validates, context unchanged, event queried/inactive, no malformed local scalar or duplicate included ID, complete untruncated actor/BG scans, available valid-or-no-hit surface probes, budget not exceeded. No-hit/support/material and water-group findings remain raw evidence; no medium-admission rule is asserted. Partial exact-intersection/volume coverage remains even with valid=1. All captures remain invalid **for location admission** in this stage, expressed independently by no_location_admission=1 and gates.

## Limits

16 diagnostic slots over the loaded module lifetime, cadence120 mod_update invocations, <=8192-byte own output buffer, one correlated line per due update. Each error consumes a slot. Shutdown/reinitialization in the same loaded DLL cannot reset the budget. C++ capture/log exception exhausts budget and stops reads. The budget resets if a completely new DLL image is loaded; this is not permission for hot reload. Reentrant update returns without a capture. Format overflow yields one bounded CAPTURE_INCOMPLETE=LOG_TRUNCATION line.

Native geometry query performs its existing spatial traversal; API does not expose an interrupt/work counter. Two vertical queries/capture, finite registered BG count, spatial filtering in official geometry and low cadence constrain calls but do not guarantee a hard CPU-time upper bound. No worker/cancel hook introduced. Stop future session through the official host UI if performance changes; no runtime performance measured here.
