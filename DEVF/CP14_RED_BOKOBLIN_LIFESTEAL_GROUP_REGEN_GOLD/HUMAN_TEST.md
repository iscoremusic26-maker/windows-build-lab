# Prueba humana CP14 — Windows x64 / DirectX 12 (no Vulkan)
Estado: **HUMAN_RUNTIME_PENDING**. Nada de esto está probado en juego todavía.

1. **Respaldo.** Cierra Dusklight por completo. Respalda `BokoblinExpansion_0.10.0_CP13_TEST.dusk`, `encounters.json`, `placements.json` y tus partidas. No instales nada con el juego abierto.
2. **Instalación.** Sustituye **solo** la Expansion 0.10.0 por `BokoblinExpansion_0.11.0_CP14_TEST.dusk`. Mantén el Provider 0.13.0 de CP13 (`DEVF_Provider_0.13.0_CP13_TEST.dusk`, SHA256 `3a3c874e…7634e`) y el editor CP13. La pareja CP14 es Provider 0.13.0 + Expansion 0.11.0. Una sola versión por ID; no toques los JSON.
3. **Zona.** Arranca en DX12 y ve a una zona válida de Campo de Hyrule con un Bokoblin E_OC2 nativo (el del machete, no el del garrote). Es Red salvo que esté registrado como Black/Normal en tu JSON. Si puedes, que haya otro Bokoblin cerca (Blue, Black o Red).
4. **Identidad.** Comprueba en el log `DEVF_CP10_4_VARIANT … classified_variant=RED effective_max_hp=330` y `DEVF_CP14_RED event=LINK_LAYOUT_CANARY_PASS`. Si aparece `…CANARY_FAILED…`, detente y envía esa línea.
5. **Golpe real.** Deja que el Red te golpee **sin escudo** y que pierdas vida. Esperado:
   - `RED_HIT_CLAIMED` → `RED_HIT_CONFIRMED` → `RED_LIFESTEAL_APPLIED heal=82` (o 83) y `RED_WINDOW_START`;
   - la cura se ve en el log (`hp_before`/`hp_after`); el juego no muestra la vida de los Bokoblins;
   - durante unos 4 s el Red muestra **aura dorada y ojos dorados**, que luego se apagan.
6. **Escudo.** Bloquea golpes con el escudo. Esperado: **sin** curación ni dorado, línea `RED_HIT_REJECTED reason=BLOCKED_BY_SHIELD_…`.
7. **Bonus grupal.** Con otro Bokoblin a menos de unos 650 de distancia y un Red que te acaba de golpear, busca `RED_GROUP_REGEN_X2_ON` del aliado y del Red, y `REGEN_APPLIED … X2_RED_WINDOW`. Al alejarse o al terminar los 4 s: `RED_GROUP_REGEN_X2_OFF`. El aliado **no** se vuelve dorado (es lo acordado).
8. **Regresión.** Blue: aura y ojos cian en grupo, como en CP12. Black: furia, aura y ojos rojos, textura negra, 440 HP, como en CP12/CP13. Encuentros R07/L6 y R10/L6, enable/disable y multiroom, como en CP13.
9. **Limpieza.** Mata a un Red con la ventana abierta y cambia de sala con un Red dorado. Ni el aura ni los ojos deben quedar colgados.
10. **Evidencia.** Guarda el log. Envía solo las líneas `DEVF_CP14_RED`, `DEVF_CP14_EYES`, `DEVF_CP10_4_VARIANT`, `DEVF_CP10_6D … variant=RED`, `REGEN_APPLIED` y las de error, con rutas y usuario redactados. Nunca recursos del juego, ROM ni partidas.

**Rollback.** Cierra el juego, retira `BokoblinExpansion_0.11.0_CP14_TEST.dusk` y repón exactamente la 0.10.0 de CP13 con los JSON respaldados. El Provider no se toca. No hay hot reload.
