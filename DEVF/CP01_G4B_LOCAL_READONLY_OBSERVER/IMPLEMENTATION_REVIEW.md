# Implemented architecture

New scope is only this directory; historical G2, A2, owner/location reports and official SDK/host remain untouched. Base8911403f2432b7117fa9e6009d809a5c3501cd7c. Independent branch implementation/devf-cp01-g4b-readonly.

* `src/capture.hpp/.cpp`: owning scalars, finite positions/distances, AABB candidate screen, scoped completion/context classification, explicit unknown-volume occupancy, bounded formatting/budget. No admission/spawn method.
* `src/extended_g2_reader.cpp`: mechanical extension of historical G2 native_reader.cpp. Identical Registry/layer_id/complete/player membership and established scene/resource/E_OC checks are retained. The existing census callback additionally copies local initialized actor positions and BG-owner identity into a pointer-free sidecar. The historical native_reader.cpp is preserved byte-for-byte and **not also compiled**, avoiding duplicate native symbol/read paths. No second census or new registry introduced. Two fopAcIt_Judge calls remain: original player membership and original bounded census. Inherited A2 itself has its existing second layer registry preflight; no new one is added here.
* `src/local_queries.cpp`: bounded public BG slot screen, caller-owned object/water-group ground queries via qualified base GroundCross, safe poly metadata, event.runCheck and post-read scene/stage/room/layer/counter/retirement recheck. BG malformed/unready scan causes geometry queries to be skipped; independent event/context diagnostics still run when A2 owner prerequisites hold. Owner rejection does not fabricate medium results. Copied resource results already obtained remain visible independently of later census failure.
* `src/mod.cpp`: historical service/version validation shape and conservative16/120 policy, reentry guard, owned host/build strings, capture and LogService only. No A2 simulation/control method reachable from lifecycle.
* `tests/`: pure scalar capture fixtures plus real mod lifecycle using fake capture/service. New semantic mutants target only new evaluator. Synthetic proven actor bounds have no native counterpart. These tests do not emulate game collision or DX12.
* `ci/`: existing private SDK-input Windows pipeline adapted to build only this DLL and two new labs; selected exact CP00 import library extracted inside ephemeral runner; never package target. Historical PE parser reused for metadata/IAT. Own/reused source SHA256 ledger, compile-unit graph, map, disassembly and text logs distinguish fixtures from the native DLL.

## Graph

mod_initialize -> Host2.2 / Log1.0 / Game2 + version check; mod_update -> capture_native -> unchanged A2 capture_owner -> adapted G2 read -> local surface/event/context scalars -> evaluate -> bounded log; mod_shutdown -> own active=false only.

Native DLL compilation includes four G4B implementation units, historical G2 core.cpp, historical A2 controller_core.cpp/room_owner_resolver.cpp, original SDK game_feature.cpp. The controller translation unit supplies validate_owner; unused synthetic controller methods have no lifecycle caller and no native request backend. Tests/fake_capture.cpp is exclusively an executable-lab source. No permanent native pointer/handle cache, async callback, worker, hook, resource loading or game write.

Review exposed and corrected namespace/header declaration issues and the real non-const parameter of read-only fpcLy_IsDeletingMesg. This does not change official source or replace an API signature. Native actor scan also diagnoses incomplete process headers without reading unsafe derived fields.
