# Scoped safety and lifecycle audit

Scope: this observer under official Dusklight2.0.3 mod_update contract; no runtime safety PASS asserted. See READ_API_CONTRACTS for every new query and its direct/callee effects. Compile-time guards reject absent/zero read-only definitions and spawn/mutation/armed macros. There is no configuration to arm this observer.

## Native surface

Only SDK Host, Log and Game imports; no Actor/Stage/Hook/Resource service. No create/delete/register/add/cancel native API, layer setter, HIO/scene/flag/save write, roomReLoader, load/sync overlay, detour, patch or writable host pointer dereference assignment. Own query objects receive writes via constructors/SetPos/base GroundCross, and own records/budget receive writes. LogService is the explicit bounded diagnostic-output exception.

BG registry slots are borrowed synchronously. Used/null/index/ready checks precede BG virtual reads; finite ordered AABB is copied only for moving candidate screen. BG actor pointer is compared with initialized actor queue membership and ID; it is not dereferenced from the BG slot. Scene pointers use the existing public scene lookup. G2 Registry validates actor owners. All borrowed objects die on the stack. Only the value sidecar, host strings, capture counter and budget persist; no native resource/actor/polygon ownership is acquired.

Synchronous visitor callbacks are passed to fopAcIt_Judge, which traverses and returns in the call; nothing is registered for future execution. No worker/timer subscription. Logging occurs after all native loans have ended. Nested mod_update is blocked while busy. Constructor/destructor of private ground-query objects does not create engine processes or register BG.

Shutdown stops new observation and frees no game objects. Exception closes observation budget; fake C++ exceptions test this policy but do not establish recovery from access violations or host memory corruption. No observed snapshot is a handle valid in future updates. The independent **custom-DLL unload blocker remains BLOCKED**; this task neither fixes nor re-audits it. No claims about general safe host DLL unload arise from observer shutdown.

## Evidence requirements for binary PASS

The final text result must identify the exact native DLL SHA256/bytes, MSVC x64/SDK provenance and exact own-source commit. PE AMD64/PE32+, four ABI/service metadata records (ABI1, Host2.2/Log1/Game2), exactly five exported symbols, exact reviewed game IAT, no forbidden mutation/system imports, no fake sources or request/setter symbols in module graph/map. Correlate entry call graph and virtual BG reads against source and disassembly; IAT names alone cannot prove read-only effects. Shared DEBUG GroundCross wrapper and shared-scratch SphChk must remain absent. Runtime-specific data remain NOT_TESTED.

## CI scope incident

The initial CI PR8 was mistakenly opened against main, whose accumulated source diff triggered historical G2/A2 workflows as well as G4B. It was immediately retargeted to ci/devf-cp01-a2-disarmed; subsequent revisions trigger only G4B. Cancellation via configured gh API returned HTTP403 for historical runs38348059071/38348058990; the connector exposes no cancellation action. Their outputs are excluded from G4B evidence. The historical G2 workflow can generate its previous observer package in the private inputs repository; it is **not** a G4B deliverable, was not downloaded or installed here, and is not a new observer package authorization. G4B workflow generates no .dusk and uploads only text. Preserve this distinction rather than claiming the incidental executions did not happen.
