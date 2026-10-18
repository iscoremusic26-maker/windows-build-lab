# CP11 — prueba humana A/B (Windows / Dusklight 2.0.3 / DX12)
Objetivo: comparar 0.8.8 y 0.8.9 en **las mismas condiciones**. Debe verse y jugarse **igual**; lo único que puede cambiar es la fluidez.
1. Cierra el juego. Respalda `BokoblinExpansion_0.8.8_TEST.dusk` y `encounters.json`. En la carpeta de mods deja **una sola** Expansion (retira también la 0.8.7 que estaba "shadowed"). Provider 0.10.0 y editor 0.10.2 sin cambios.
2. **Ronda A (0.8.8)** y **ronda B (0.8.9)**, misma partida, mismo sitio, misma cámara y misma duración (~2 min cada escenario):
   - Blue solo (F_SP121, sala 3, capa 6) → sin aura.
   - Grupo de 2 y de 3+ Blue (aura ON), sepáralos y júntalos varias veces (ON/OFF).
   - Black (F_SP121, sala 1, capa 6) en furia con golpes no letales, y si puedes 2+ auras a la vez.
   - Caminar, combatir y cambiar de sala con auras activas.
3. Fluidez: anota si notas tirones al **aparecer o desaparecer** las auras. Si tienes un contador de FPS o de frametime (por ejemplo el de Dusklight), apunta mínimos y caídas de cada ronda; si no, basta con tu impresión.
4. Comprueba que **nada cambió**: el mismo aspecto de las líneas (cerca del cuerpo, finas, Blue cian, Black violeta, ojos púrpura), los mismos fades y la misma limpieza al morir o cambiar de sala. Red sin aura; vida, daño y regeneración iguales.
5. Log 0.8.9: debe decir `version=0.8.9`. Las dos primeras activaciones por variante salen completas (`TUNING_APPLIED`, `DRAW_BASIS_CHECK`), luego aparece `TELEMETRY_BUDGET` y después líneas `detail=COMPACT`. Es lo esperado.
6. Si dudas de si un tirón es del mod: repite la misma escena **sin auras activas** (Blue solos). Si el tirón sigue, no viene de las partículas. No hace falta tocar los otros mods ni las texturas; si quieres aislar más, retíralos temporalmente y vuelve a ponerlos después.
7. Envíame el log completo de cada ronda y tu comparación (igual, mejor o peor), con capturas o vídeo si puedes.
8. **Rollback:** cierra el juego, quita el 0.8.9 y reinstala exactamente `BokoblinExpansion_0.8.8_TEST.dusk`. Nunca dos versiones a la vez; no toques saves, CISO ni JSON.
