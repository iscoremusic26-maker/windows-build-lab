# Bokoblin Black: por qué no está y qué haría falta

## Bloqueo (Bokoblin Expansion 0.17.0, fuente `fe47c8b`)

- Black no es un perfil nativo: es un E_OC2 que Expansion admite como Black solo si coincide **exactamente** con un registro de sus encuentros empaquetados (`typed_registration.hpp`, `black_policy.hpp`). Se comprueban perfil 510, parámetros, coordenadas iniciales, habitación, capa, home, identidad, primer Execute y que no haya otro actor vivo en ese registro.
- Esa tabla se construye una vez en `mod_initialize`, desde `packaged_config`. Expansion no publica ningún servicio: no hay manera pública de añadir un registro después.
- El servicio V1.1 del Framework (`dev.devf.encounter_placement.v11`) crea ediciones de escena para la carga de la escena. No da identidad Black a un actor que ya se está creando junto a Link.
- Un E_OC2 creado por este mod es Red para Expansion (`make_plan` con `Registration::None`). Pintarlo, ponerle 440 HP o copiar coordenadas de un registro no lo convierte en Black. Este mod no hace nada de eso.

## API mínima que tendría que implementar Bokoblin Expansion

Un servicio propio, por ejemplo `dev.bokoblin.expansion.variant_spawn` v1:

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

Reglas que mantendrían a Expansion como único dueño: un ticket por actor, expiración corta, admisión solo en el primer Execute de esa generación, firma nativa intacta (E_OC2, 220), y HP, textura, IA y furia aplicados solo por Expansion.

## Qué cambiaría en este spawner

La entrada Black existe ya y está bloqueada. Con ese servicio presente, la ruta haría `reserve` → `create_actor` en la capa de Link → `bind` → observar `query`. Todo lo demás (capa, suelo, límite, seguimiento) ya está aquí. Hasta entonces: **Black: integración no disponible**.
