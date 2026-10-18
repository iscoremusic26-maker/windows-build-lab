# CP10.6B-R1 — implementation and evidence (Dusklight 40457c6)

## 1. Gate 0 — why a read-only enumeration from the mod is safe (read from the exact source)
| Question | Evidence |
|---|---|
| Are the types public? | `JPAResourceManager` has public `resRegNum`, `resMaxNum`, `pResAry` (`JPAResourceManager.h` L26-33); `JPAEmitterManager` has public `pResMgrAry`, `gidMax`, `ridMax` (`JPAEmitterManager.h` L47-53); `JPAResource::getUsrIdx()` and the dynamics/shape getters are header inlines. No offsets are invented and no new import is needed (the only game data symbol touched is `dPa_control_c::mEmitterMng`, already reviewed). |
| Which slot is which? | `createCommon`: `entryResourceManager(mCommonResMng, 0)` (`d_particle.cpp` L1200-1230); `createRoomScene`: `entryResourceManager(mSceneResMng, 1)` (L1231-1252). `getRM_ID` is the 0x8000 bit (L1196). |
| Can a manager be half-built or dead when read? | A manager is fully constructed (its constructor loads every resource) before `entryResourceManager` publishes it. `clearResourceManager` sets `pResMgrAry[id] = NULL` **before** the scene heap is destroyed (`removeRoomScene`, L1290-1307; `JPAEmitterManager.cpp` L153-167). Reading the CURRENT array entry synchronously in a game callback therefore never dereferences a destroyed manager. |
| What does `getUsrIdx` mean? | `getResource(u16)` is a linear scan comparing `getUsrIdx()` with the id passed to `set()` (`JPAResourceManager.cpp` L20-25). Scene-bank resources carry the 0x8000 bit in that index, common-bank ones do not. An id is "present" only if it appears in this list. |
| Thread / context | The reading happens inside the existing `daE_OC_c::execute` POST callback (game thread), synchronously, no yield. |
Verdict: feasible and safe within these bounds, so the build was produced (no `STOP_DIAGNOSTIC_NOT_FEASIBLE`).

## 2. What the mod reads (and nothing else)
`collect()` (pure, `inventory_policy.hpp`, templates over the public member names, the same code in the lab with fakes): `resRegNum`, `resMaxNum`, `pResAry[i]->getUsrIdx()`. Bounds: the array is **not touched** if `resRegNum == 0`, `pResAry == NULL`, `resRegNum > resMaxNum` or `resRegNum > 8192`. Null entries are skipped and counted; duplicates are counted and listed once. The ids are sorted, de-duplicated, checksummed (FNV-1a over the sorted ids and the count). Every accessor takes a `const` pointer. For the shortlist it also reads scalar metadata through header getters (shape/dir/rot type, base size, volume, max frame, life, rate, velocities, emitter direction, flags, colours, key/field/texture counts, child/extra shape presence); a resource without dynamics/shape data is counted as `no_metadata`.

## 3. When it reads (bounded)
Polled every 16th execute pair of any Bokoblin (a counter; no per-frame work). `Tracker` (pure, tested):
- **Common bank (slot 0):** registered and non-empty on 2 consecutive polls -> one snapshot, never repeated; if it is still not ready after 40 polls, one `INVENTORY_PARTIAL`.
- **Scene bank (slot 1):** the array entry is re-read each poll; a numeric **token** of the manager address plus `resRegNum`, `resMaxNum` and the stage tag form a signature. The token is only compared, never dereferenced. A new signature must be stable for 2 polls before a snapshot (stabilisation). If the bank vanishes (room change) the remembered signature is dropped, so its return, even at the same address, is a new instance. Hard cap of 6 scene snapshots (one `INVENTORY_PARTIAL` on reaching it); a scene bank that never appears is reported once.
- If a new scene snapshot has the same checksum and count as the previous one, only `INVT_BEGIN`/`INVT_END` are written (`same_as_snapshot=N`), not the id list again.

## 4. Log format (`DEVF_CP10_6B_R1_INVENTORY`, hard budget of 160 lines per session, lines <= ~1.1 kB, no per-frame output)
- `INVT_BEGIN`: snapshot, bank, role (COMMON/SCENE), manager state, stage tag, entries, listed, unique, dups, nulls, `res_max`, `ridMax`, `gidMax`, `platform_detected=NOT_AVAILABLE_TO_MOD` (the SDK exposes no game version) and the two possible virtual common-bank paths (`/res/Particle/common.jpc` on GameCube, `common-r.jpc` on Wii: virtual disc paths, not claims about files on Windows).
- `INVT_IDS`: `chunk=i/n`, `first_index`, `ids_in_chunk`, `data=` sorted exact ids as 4-digit upper-case hex in compact ranges (`00E6-00EF,00F1-017B,...`); bank-1 ids keep the 0x8000 bit. Lines are <= 700 data characters.
- `INVT_END`: count, checksum, chunks, `ids_shown`, `ids_missing`, `truncated`, `same_as_snapshot`, `integrity=SHOWN_PLUS_MISSING_EQUALS_UNIQUE`. Truncation (only if more than 40 lines would be needed, i.e. a pathological non-contiguous bank) is explicit with an exact missing count; it is never silent.
- `INVENTORY_READY_COMMON` / `INVENTORY_READY_SCENE`, `INVENTORY_STATS` (shape-type histogram, how many are elongated / persistent / keyed / with child), `INVENTORY_SHORTLIST` (<= 20 per snapshot, one per line), `INVENTORY_COMPLETE`, `INVENTORY_PARTIAL` / `INVENTORY_FAILED` with a reason. No `EMITTER_ACQUIRED`, no `HUMAN_VISUAL_ACCEPTED` is ever emitted.
- Catalogue names are **not** printed: the mod cannot verify the id -> name pairing; resolve names offline from the logged `usr_index` and `d_particle_name.h`, and treat that as a catalogue claim.

## 5. Shortlist (technical, NOT visual)
Eligible only if **elongated**: shape type line (1) or stripe (5, 6), or base-size aspect >= 3. Then a score: elongated 2, emission axis vertical (|y| >= 70 % of the emitter direction) 1, persistent (max frame 0 or >= 60 and life >= 10) 1, soft emission (0.1 <= rate <= 5) 1, simple (no key animation, no child) 1. Top 20, a total order (score, then id) so the result does not depend on iteration order. **These are heuristics and correlations; none is a visual demonstration**, and a stored colour/alpha is not the final on-screen colour. The shortlist may be empty, and then the inventory is the result.

## 6. What changed relative to 0.8.2
Removed: the whole aura emitter path (`dComIfGp_particle_set`, `getEmitter`, `getResource`, colour/alpha/scale setters, retries, probe). The visual controller still runs, with the aura channel never requested (`want_aura = false`), for the Black eyes. Added: `inventory_policy.hpp`, the poll and the logs. Imports: the reviewed game import set shrinks by three symbols (the two particle functions and `JPAResourceManager::getResource`). `sscanf` (lab helper) is compiled out of the DLL.

## 7. Evidence classes
| Class | What | Covers |
|---|---|---|
| Source reading | section 1 | safety of the read, meaning of banks and user indices |
| `inventory_lab` (17 cases, ASan/UBSan clean locally) | the real collection/render/scheduler/shortlist code against fake managers | exact ids, duplicates/nulls, empty/absent/unregistered banks, count overflow never touches the array, const contract, order-independent checksum, exact round trip of the serialisation, bounded chunks, explicit truncation, scheduling (once, stable, new instance, vanished pointer, cap, absent), line budget, shortlist order/cap/empty |
| Mod-level dry run (not committed) | the mod's own `inventory_poll` against a fake emitter manager, including a vanishing/returning scene bank | the log lines and `same_as_snapshot` |
| Regression (unchanged, reused) | CP10.6 28, CP10.5 22, CP10.4 30, CP10.6A visual 22 + eye 9, runtime 66, policy 26, texture 20, editor 42/71/110/25 | everything else |
| CI static gate | no emitter-creating/modifying code of any kind in the source, one hook | "no aura particle_set" |
| PE/ABI audit + Windows CI + independent download | imports (reduced set), one hook, services, negatives, SHA256 | the delivered bytes |
| **Not available** | what the user's game actually registers | the human round (the log) |

## 8. Not verified
The real contents of the user's common bank and scene banks; whether the 16-poll cadence catches every room (the scene bank is only read while a Bokoblin executes); behaviour with other mods that register particle resources; the platform (GameCube `common.jpc` vs Wii `common-r.jpc`) since the mod cannot detect it. The disc id `GZ2E01` is the GameCube USA id in Dusklight's own asset folders (Wii USA is `RZDE01_*`): the logged paths are only the two possibilities.
