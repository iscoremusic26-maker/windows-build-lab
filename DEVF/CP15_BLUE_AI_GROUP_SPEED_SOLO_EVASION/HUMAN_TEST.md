# Prueba humana CP15 — Windows x64 / DirectX 12 (no Vulkan)
Estado: **HUMAN_RUNTIME_PENDING**. Nada de esto está probado en juego todavía.

1. **Respaldo e instalación.** Cierra Dusklight por completo. Respalda `BokoblinExpansion_0.11.0_CP14_TEST.dusk`, el Provider 0.13.0 (CP13), `encounters.json`, `placements.json` y tus partidas. Instala **solo** `BokoblinExpansion_0.12.0_CP15_TEST.dusk` (una única versión del mod `dev.bokoblin.expansion`; el Provider y el editor CP13 no cambian). No toques CISO, ROM ni partidas. Arranque en frío.
2. **Cómo tener compañeros cerca (sin tocar tu JSON validado).** Los campamentos nativos de Campo de Hyrule ya traen varios Bokoblin (E_OC = Blue; E_OC2 = Red). Sirve acercarte a uno de ellos y eliminar compañeros uno a uno. Para un montaje controlado, abre el Map Editor CP13, crea un JSON de prueba **con otro nombre** (no sobrescribas el validado) con 2-4 puntos (separación ≥300, a menos de 650 entre sí, misma sala y capa, R07/L6 o R10/L6) y úsalo solo en esta prueba. **Rollback:** cerrar el juego, restaurar los JSON de respaldo y el mod CP14.
3. **Blue solo.** Un E_OC nativo (o un Normal DEVF) sin compañeros cerca: acércate. Esperado: se gira y **corre alejándose** (no te persigue ni te ataca), sin atravesar paredes, sin saltar a vacío ni al agua. Prueba junto a un muro y junto a un borde. Si te pega, el golpe nativo y la reacción al daño siguen siendo los de siempre.
4. **Pareja.** Blue + otro Blue a ≤650 XZ y ≤250 Y: ambos pasan a nivel 2; sus ataques deben verse algo más rápidos (≈15 %) con el golpe, el sonido y el escudo coherentes.
5. **Blue + Red** y **Blue + Black** como única compañía: también nivel 2 (cualquier variante cuenta).
6. **Grupo.** Blue + dos compañeros de cualquier variante: nivel 3, más rápido que el nivel 2 (≈30 % sobre el nativo). Un cuarto Bokoblin no acelera más.
7. **Eliminar compañeros uno a uno:** 3 → 2 → 1. Con el último compañero muerto, Blue debe pasar a huir. Si vuelven a acercarse compañeros, vuelve a atacar.
8. **Escudo:** bloquea un golpe de un Blue en pareja/grupo: el retroceso nativo debe seguir viéndose y oyéndose igual (no se acelera).
9. **Regresión:** Red CP14 (robo de vida, ventana, dorado) y Black CP13 (furia, ojos/aura rojos, textura negra), aura/ojos cian de Blue, encuentros R07/R10 y multiroom.
10. **Evidencia.** Guarda el log. Envía solo las líneas `DEVF_CP15_BLUE_AI` (`BLUE_AI_ARMED`, `BLUE_LEVEL`, `EVADE_START/END/HOLD/RESUME/RETARGET`, `ATTACK_VETOED`, `ATTACK_RATE_APPLIED/SKIPPED`, `ATTACK_LOCKOUT_SCALED`, `AI_CANARY_FAILED_*`) y las de error, con rutas y usuario redactados. Anota a mano si hubo crash, golpes desincronizados, clipping, sonidos perdidos o animaciones congeladas. Nunca recursos del juego.

**Rollback.** Cierra el juego, retira `BokoblinExpansion_0.12.0_CP15_TEST.dusk` y repón exactamente la Expansion 0.11.0 de CP14 y tus JSON respaldados. El Provider no se toca. No hay hot reload.
