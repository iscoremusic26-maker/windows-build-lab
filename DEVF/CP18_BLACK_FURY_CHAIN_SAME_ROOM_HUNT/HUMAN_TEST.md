# QA humana CP18 0.15.0 — Windows/DX12, arranques fríos

Prueba privada. Nada de esto se ha observado todavía en el juego: son instrucciones, no resultados.

0. **Preparación.** Cerrar Dusklight por completo. Guardar copia de `BokoblinExpansion_0.14.1_CP17_FIX01_RED_X2_JUMP_INTERCEPT_TEST.dusk`, de los JSON y de las partidas. Sustituir **solo** la Expansion por `BokoblinExpansion_0.15.0_CP18_BLACK_FURY_CHAIN_SAME_ROOM_HUNT_TEST.dusk`. Provider y editor 0.13.0 y los JSON no cambian. Nunca dos Expansion a la vez ni hot reload. En el log debe aparecer `BLACK_AI_ARMED version=0.15.0`.

A. **Black sin provocar.** Acercarse sin golpearle. Esperado: golpes B/C algo más rápidos (x1,15), con sus pausas normales de espera entre golpes; HP, aura y ojos como en 0.14.1; Blue y Red iguales. Un Blue que grita puede seguir llamando a este Black. Log: `BLACK_NATIVE_ATTACK_STARTED reason=NATIVE_BC_X1_15…`, `BLACK_RATE_APPLIED_READBACK owned_rate=1.15`.

B. **Primer golpe y furia.** Quitarle HP de verdad (espada, flecha…). Esperado: aura/ojos de furia y ATP 6 como siempre durante 300 tics (~10 s) desde el último daño. Durante la furia: golpes B/C a x2, **uno tras otro**, sin la animación ni la voz de espera intermedia y sin la pausa de 20 tics. Probar a corta distancia y moviéndose; con escudo levantado debe rebotar (retroceso nativo completo) y volver a atacar. Escuchar si cada golpe conserva su voz y su sonido de impacto. Log: `BLACK_DAMAGE_RECOGNIZED_EXISTING_CP10_6`, `BLACK_HUNT_LOCKED_FIRST_DAMAGE`, `BLACK_CHAIN_NEXT_ATTACK_STARTED`, `BLACK_NATIVE_WAIT_TRANSITION_AVOIDED`, `BLACK_RATE_APPLIED_READBACK owned_rate=2.00`, `BLACK_NATIVE_CONTACT_LINK`, `BLACK_NATIVE_SHIELD_BLOCK`. Anotar: ¿se oye alguna voz de espera entre golpes encadenados? ¿algún golpe atraviesa pared o daña sin contacto visual? ¿se corta alguna animación?

C. **Fin de la furia.** Sin volver a dañarle, esperar a que se apague el aura. Esperado: vuelve a x1,15 con pausas nativas, ATP 3, ojos normales; **sigue persiguiéndote** (Hunt). Log: `BLACK_FURY_END` (CP10.6) sin `BLACK_HUNT_RESET…`.

D. **Hunt en la misma sala.** Huir al otro extremo de la sala, salir de su campo de visión, rodear obstáculos transitables. Esperado: no vuelve a su puesto ni se queda esperando; sigue acercándose a distancia de golpe, sin teletransportes ni aceleraciones raras. Si el terreno lo impide (barranco, pared), puede quedarse quieto mirando o pegado al obstáculo: eso se registra (`BLACK_HUNT_PURSUIT_BLOCKED_BY_GEOMETRY`), no es éxito. Log: `BLACK_HUNT_NATIVE_ABANDON_PREVENTED`, `BLACK_HUNT_PURSUIT_MEASURED`.

E. **Cambio de sala.** Salir de la sala A a la B: Black **no** debe seguirte. Volver a A (si es posible, A→B→A sin detenerse en B): no debe recordar el aggro; se comporta como antes del daño hasta que vuelvas a quitarle HP. Log: `BLACK_HUNT_RESET_PLAYER_ROOM_CHANGED` y, con daño nuevo, otro `BLACK_HUNT_LOCKED_FIRST_DAMAGE`. En Hyrule Field cada sala del mapa cuenta como sala distinta.

F. **Integración.** Blue llamando compañeros antes y después de dañar al Black (tras el daño, el Black provocado no acude a la llamada: `BLACK_HUNT_LOCKED_SAME_ROOM_PRIORITY_OVER_RECRUIT_ORDER`). Red CP17 FIX01 igual que en 0.14.1. Dos Black a la vez: solo el dañado persigue. Ojos, aura, regeneración y daño normales. 1080p DX12: anotar fluidez subjetiva. `PERF_WINDOW … MEASUREMENT_NOT_AN_ERROR` no es un error.

G. **Parada segura.** Ante crash, caída fuera del mapa, bloqueo, daño sin contacto visual, golpes a través de paredes, falta de audio nativo, impactos dobles, pérdida clara de FPS o persecución a otra sala: cerrar el juego, conservar el log, rollback frío según RECOVERY y anotar evento, hora y `actor_id`.

Separar siempre lo que se ve y oye en el juego de lo que dice el log. Ningún contador sustituye la observación. Estado: HUMAN_RUNTIME_PENDING.
