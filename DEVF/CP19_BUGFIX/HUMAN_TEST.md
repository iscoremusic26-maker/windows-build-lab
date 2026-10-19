# QA humana CP19 0.16.0 — Windows/DX12, arranques fríos

Prueba privada. Son instrucciones, no resultados: nada de esto se ha observado aún en el juego.

0. **Preparación.** Cerrar Dusklight. Guardar copia de `BokoblinExpansion_0.15.0_CP18_BLACK_FURY_CHAIN_SAME_ROOM_HUNT_TEST.dusk`, de los JSON y de las partidas. Sustituir **solo** la Expansion por `BokoblinExpansion_0.16.0_CP19_BUGFIX_TEST.dusk`; Provider/editor 0.13.0 y JSON iguales. Nunca dos Expansion ni hot reload. Mismos encuentros que en el log del 19-10 (Hyrule Field, salas 7 y 10).

1. **A — audio.** En las salas 7 y 10 (antes mudas): ¿se oyen voces, pasos, ataques, daño, caída/muerte y el choque con el escudo de los Bokoblin añadidos? Probar uno y varios, cerca y lejos. Pasar a una sala con Bokoblin de fábrica (2/3/6) y volver. Log: `FIELD_SE_WAVE_LOAD_REQUESTED`, `FIELD_SE_WAVE_RELEASED`; si aparece `FIELD_SE_WAVE_LOAD_REFUSED` o siguen mudos, anotarlo (sería STOP_BLOCKED_AUDIO).
2. **B — Z-target.** Combatir largo; derribarlos y dejar que se levanten; dejarlos con poca vida y esperar a que regeneren. ¿Alguno vivo deja de poder fijarse? Log: `ATTENTION_BATTLE_RESTORED`.
3. **C — música.** Pelear con varios Bokoblin, rematar con Jump Attack (y con otros golpes) y quedarse en la sala sin cambiar. ¿La música vuelve a su volumen normal? Log: `ZERO_HP_LIVE_MODE_COMPLETED_NATIVE_DEATH_PATH`, `RESIDUAL_LASTHIT_FLAG_CLEARED`, `BGM_BATTLE_STATE` (estado 3 = sin batalla).
4. **D — Blue.** Con aliados lejos de su posición inicial: que un Blue los llame antes y después de morir. Deben acudir andando y pelear sin volver a casa; tras morir el Blue, siguen mientras peleen contigo y se calman si dejas de pelear. Log: `BLUE_CALL_LEASH_PERMISSION_GRANTED/NATIVE_FIND/ENDED/BLOCKED_BY_GEOMETRY`.
5. **E — Red.** Cargar Spin y Jump Strike a 250–400 de un Red: debe acercarse y golpear desde cerca; si te alejas, no debe golpear al aire. Log: `CLOSE_IN_STARTED`, `CLOSE_IN_MEASURED`, `NATIVE_ATTACK_FORCED`, `CLOSE_IN_UNREACHABLE`.
6. **F — Helm Splitter.** Contra Bokoblin con más de 100 HP (Black/Red/Blue según tus encuentros): con la Espada Maestra cada Helm Splitter quita 100 HP, con la de Ordon 50, y el enemigo sobrevive con derribo; con 100 HP o menos (50 con Ordon) muere como siempre. Log: `HELM_SPLITTER_EXACT_DAMAGE_APPLIED value_a=antes value_b=después`, `HELM_SPLITTER_NATIVE_LETHAL`. Jump Attack, Jump Strike, giros y bombas no deben cambiar.
7. **G — golpes x2.** Black en furia y Red reaccionando: ¿siguen pareciendo atravesar a Link o el escudo? ¿suena el choque con el escudo? Log: `BLACK_STRIKE_STARTED`, `BLACK_NATIVE_ATTACK_ENDED … active_min_dist=`.
8. **Regresión.** Black furia/caza CP18, Red CP17 FIX01, Blue FIX01, aura/ojos/regeneración y FPS a 1080p como en 0.15.0.
9. **Parada segura.** Ante crash, cuelgue de audio, actores fuera del mapa, golpes a través de paredes, daño sin contacto, logros falsos o FPS claramente peores: cerrar, guardar el log, rollback frío a 0.15.0 (RECOVERY) y anotar evento y `actor_id`.

Separar lo visto/oído en el juego de lo que dice el log. Estado: HUMAN_RUNTIME_PENDING.
