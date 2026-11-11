# Bokoblin Black: why it is not available and what would be needed

## Blocker (Bokoblin Expansion 0.17.0, source `fe47c8b`)

- Black is not a native profile. It is an E_OC2 that Expansion admits as Black only when it matches **exactly** a record of its packaged encounters (`typed_registration.hpp`, `black_policy.hpp`): profile 510, parameters, initial coordinates, room, layer, home, identity, first Execute, and no other live actor on that record.
- That table is built once in `mod_initialize`, from `packaged_config`. Expansion publishes no service, so there is no public way to add a record later.
- The framework's V1.1 service (`dev.devf.encounter_placement.v11`) creates scene edits for when a scene loads. It does not give Black identity to an actor being created next to Link.
- An E_OC2 created by this mod is Red for Expansion (`make_plan` with `Registration::None`). Painting it, giving it 440 HP or copying a record's coordinates would not make it a Black. This mod does none of that.

## Minimal API Bokoblin Expansion would have to implement

A service of its own, for example `dev.bokoblin.expansion.variant_spawn` v1:

```c
typedef struct ExpansionVariantSpawnService {
  ServiceHeader header;
  /* Reserve the identity of the NEXT Bokoblin created with these exact params by the caller. */
  ModResult (*reserve)(ModContext* ctx, uint32_t variant /* 1 = BLACK */, const ActorSpawnParams* params,
                       uint32_t expire_ticks, uint64_t* out_ticket);
  /* Tell Expansion the ActorId the host returned for that ticket, before its first Execute. */
  ModResult (*bind)(ModContext* ctx, uint64_t ticket, ActorId id);
  /* Read the outcome: PENDING, ADMITTED (id, generation, hp), REJECTED (reason), EXPIRED. */
  ModResult (*query)(ModContext* ctx, uint64_t ticket, uint32_t* out_state, uint32_t* out_reason);
  ModResult (*cancel)(ModContext* ctx, uint64_t ticket);
} ExpansionVariantSpawnService;
```

Rules that keep Expansion the only owner: one ticket per actor, a short expiry, admission only at the first Execute of that generation, an intact native signature (E_OC2, 220), and HP, texture, AI and fury applied only by Expansion.

## What would change in this spawner

The Black entry already exists and is blocked. With that service present, its route would be `reserve` → `create_actor` in Link's layer → `bind` → observe `query`. Everything else (layer, ground, limit, tracking) is already here. Until then: **Bokoblin Black: integration unavailable**.
