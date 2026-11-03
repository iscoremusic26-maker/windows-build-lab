/* Enemy Health Multiplier 2.0.3 -- OPTIONAL VITALS SERVICE, contract v1.0 (HM203_CP04).
 *
 * SHARED, BYTE-IDENTICAL header: the provider (Enemy Health Multiplier) and the consumer (Enemy Health Bars) each carry an identical copy
 * (tools/check_contract_copies.sh compares their SHA-256). Nothing here depends on game headers.
 *
 * Semantics (full text: HM203_CP04_OPTIONAL_SERVICE_ABI.md, HM203_CP04_EFFECTIVE_VITALS_MODEL.md):
 *  - READ ONLY. The call copies a few integers into `out`; it never writes to the actor, the bank, the damage state or any other game state, never
 *    synchronises a budget, never allocates and never logs per call. The provider does its own work (damage, budgets) without ever being queried.
 *  - `actor` is a borrowed pointer valid only during the call; the provider verifies `expected_actor_id` resolves to it and that `expected_profile`
 *    equals the actor's profile name BEFORE reading any field, and returns no pointer into its own state.
 *  - Any status other than HM_VITALS_OK means: use the actor's native health. USE_NATIVE is an expected, silent answer; UNAVAILABLE and INVALID are
 *    contract/transient problems the consumer counts (bounded logging).
 *  - Units: `current`/`maximum` are in "effective units" = 1/multiplier of a native HP (see the model). With multiplier 1 they equal native HP.
 */
#ifndef HM_VITALS_API_H
#define HM_VITALS_API_H

#include <stdint.h>
#include "mods/api.h"

#define HM_VITALS_SERVICE_ID "com.dusklight.enemy_health_multiplier_203.vitals"
#define HM_VITALS_SERVICE_MAJOR 1u
#define HM_VITALS_SERVICE_MINOR 0u

/* Return values of get_vitals (plain uint32_t on the wire). */
enum {
    HM_VITALS_OK = 0,           /* `out` is fully filled and valid */
    HM_VITALS_USE_NATIVE = 1,   /* expected: this actor has no effective representation now; `out->reason` says why; use native health */
    HM_VITALS_UNAVAILABLE = 2,  /* transient or unverifiable (not ready, call in flight, wrong thread, identity mismatch, out of range); use native health */
    HM_VITALS_INVALID = 3       /* bad arguments (null pointer, wrong struct_size); `out` untouched; use native health */
};

/* Meaning of HmVitals::kind (valid when status == OK). */
enum {
    HM_VITALS_KIND_EFFECTIVE_HP = 1, /* current = native_hp * multiplier - fractional_bank (exact remaining raw damage); maximum = 0: unknown, the consumer keeps its own running maximum IN THE SAME UNITS */
    HM_VITALS_KIND_BUDGET = 2        /* exact HP budget with a known maximum (Zant mask: native health / marker; armoured Helmasaur: armour pool * multiplier - bank over budget * multiplier) */
};

/* HmVitals::reason (always written for statuses 1..2; 0 for OK). 1..15 = USE_NATIVE (expected), 16..31 = UNAVAILABLE, 32.. = INVALID. */
enum {
    HM_VR_NONE = 0,
    HM_VR_FACTOR_ONE = 1,      /* factor x1: the multiplier is not active, native health IS the effective health */
    HM_VR_BREAKER = 2,         /* the provider closed its safety breaker: nothing is scaled any more */
    HM_VR_PROFILE_NATIVE = 3,  /* profile excluded / native policy / native-HP family / unresolved family / unknown: no effective representation */
    HM_VR_PROFILE_RULE = 4,    /* actor is not eligible (group != enemy, or the profile rule such as the Bulblin ordinary-soldier rule fails) */
    HM_VR_NOT_LIVING = 5,      /* health <= 0 */
    HM_VR_NO_BUDGET = 6,       /* Zant mask still at its retail budget (no multiplier marker) */
    HM_VR_NOT_READY = 16,      /* provider not registered / not yet running a tick */
    HM_VR_BUSY = 17,           /* a damage call is in flight: a temporary health value could be visible, so nothing is reported */
    HM_VR_WRONG_THREAD = 18,   /* called from a thread other than the game thread */
    HM_VR_IDENTITY = 19,       /* expected id/profile do not resolve to this actor, or its class is inconsistent */
    HM_VR_RANGE = 20,          /* the effective value is outside the representable/consistent range */
    HM_VR_UNREADABLE = 21,     /* a related actor (Helmasaur armour) could not be resolved */
    HM_VR_BAD_ARGS = 32,
    HM_VR_BAD_SIZE = 33
};

typedef struct HmVitals {
    uint32_t struct_size;   /* IN: sizeof(HmVitals) as compiled by the caller (must be >= the v1.0 size); unchanged on return */
    uint32_t actor_id;      /* OUT (OK): echo of expected_actor_id */
    uint16_t profile;       /* OUT (OK): echo of expected_profile */
    uint16_t kind;          /* OUT (OK): HM_VITALS_KIND_* */
    uint16_t multiplier;    /* OUT (OK): 1..8 (factor in force for this actor); the unit of current/maximum is 1/multiplier native HP */
    uint16_t reason;        /* OUT (always except INVALID): HM_VR_* */
    uint32_t current;       /* OUT (OK): effective units remaining */
    uint32_t maximum;       /* OUT (OK): effective units total, or 0 when unknown (see KIND_EFFECTIVE_HP) */
} HmVitals;

typedef struct HmVitalsService {
    ServiceHeader header;
    /* `caller` identifies the importing mod (SDK convention). Returns HM_VITALS_*; fills `out` as documented above. */
    uint32_t (*get_vitals)(ModContext* caller, const void* actor, uint32_t expected_actor_id, uint16_t expected_profile, HmVitals* out);
} HmVitalsService;

#ifdef __cplusplus
#include "mods/service.hpp"
static_assert(sizeof(HmVitals) == 24, "HmVitals v1.0 is 24 bytes");
template <>
struct mods::ServiceTraits<HmVitalsService> {
    static constexpr const char* id = HM_VITALS_SERVICE_ID;
    static constexpr uint16_t major_version = HM_VITALS_SERVICE_MAJOR;
    static constexpr uint16_t minor_version = HM_VITALS_SERVICE_MINOR;
};
#endif

#endif /* HM_VITALS_API_H */
