# QA humana CP19 FIX01 0.16.1 — Windows/DX12

Instrucciones, no resultados.

0. Cerrar Dusklight. **Retirar** `BokoblinExpansion_0.16.0_CP19_BUGFIX_TEST.dusk` (crashea) y poner solo `BokoblinExpansion_0.16.1_CP19_FIX01_STARTUP_AUDIO_READINESS_TEST.dusk`. Provider/editor 0.13.0 y JSON iguales. Una sola Expansion. Para aislar, conviene una primera prueba sin otros mods (p. ej. Zora Swim) y luego con ellos.
1. **Arranque frío** hasta el menú: sin crash. En el log, las primeras líneas CP19: `CP19_AUDIO_NOT_READY_DEFERRED reason=MDOAUD_INITFLAG_OFF_SECTION_HEAP_NOT_CREATED_YET` y después `AUDIO_READY`.
2. Cargar partida, entrar en Hyrule Field salas 7 y 10: `FIELD_SE_WAVE_LOAD_REQUESTED`; comprobar si se oyen los Bokoblin (A).
3. Cambiar de sala (7 → 3 → 10), volver al menú (`CP19_AUDIO_NOT_READY_DEFERRED reason=RETURN_TO_MENU` y luego `AUDIO_READY_AGAIN`), recargar partida, cerrar el juego y volver a abrir.
4. Repetir las comprobaciones A–G de CP19 (`DEVF/CP19_BUGFIX/HUMAN_TEST.md`).
5. **Si hay otro crash:** no repetir; cerrar, guardar el log completo y volver a 0.15.0 (RECOVERY).

Separar lo observado en juego de lo que dice el log. Estado: HUMAN_BOOT_RUNTIME_PENDING y HUMAN_A-G_PENDING.
