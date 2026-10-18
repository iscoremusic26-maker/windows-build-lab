# CP12 — prueba humana A/B (Windows / Dusklight 2.0.3 / DX12 / Intel UHD)
Objetivo: (1) ver si los colores nuevos son correctos y (2) **medir** de dónde vienen los bajones en combates de grupo.
1. Cierra el juego. Respalda `BokoblinExpansion_0.8.9_TEST.dusk` y `encounters.json`. Deja **una sola** Expansion: `BokoblinExpansion_0.9.0_TEST.dusk`. Provider 0.10.0 y editor 0.10.2 sin cambios.
2. **Colores** (F_SP121):
   - **Blue** (sala 3, capa 6): solo → ojos nativos y sin aura. En grupo de 2 y de 3+ → el aura igual que en 0.8.9 y los **ojos pasan a azul/cian** poco a poco. Al separarse → vuelven al color nativo suave. Mira de cerca.
   - **Black** (sala 1, capa 6): golpe no letal → aura **roja intensa** (no rosa, no naranja, sin restos morados) y **ojos rojos**. La piel sigue negra. Unos 10 s tras el último golpe, todo vuelve a nativo.
   - **Red:** sin aura y ojos nativos.
   - Comprueba también al morir y al cambiar de sala: no deben quedar ojos teñidos ni líneas.
3. **Rendimiento:** juega la **misma escena** de combate de grupo (varios Bokoblins y varias auras, unos 2 minutos) tres veces y anota la fluidez y, si lo tienes, el contador de FPS:
   - **Ronda A:** arranque normal.
   - **Ronda B:** arranca Dusklight con la opción `--log-level warning` (quita las miles de líneas DEBUG del motor).
   - **Ronda C:** arranca con `--log-level error` (quita además los avisos `CalcDivideInfo` que el motor escribe a disco en cada frame de combate).

   Son opciones de arranque temporales: no cambian ningún archivo, y al volver a arrancar sin ellas todo queda como antes. Si no sabes cómo añadirlas, basta con las rondas A y C, o solo la A.
4. En el log de cada ronda aparecen cada ~10 s líneas `DEVF_CP12 event=PERF_WINDOW` (salen como `[ERROR ...]` aunque no son errores: `level_note=MEASUREMENT_NOT_AN_ERROR`). Ahí van los intervalos entre ticks (p50, p95, p99 y máx), cuántos superan 40/50/67 ms, la CPU del mod y cuántas auras había.
5. Si quieres comparar con 0.8.9: repite la ronda A con 0.8.9 (no tiene `PERF_WINDOW`, solo vale la impresión).
6. Envíame los **logs completos** de cada ronda, tu impresión de fluidez (igual, mejor o peor) y capturas o vídeo de los ojos y del aura roja.
7. **Rollback:** cierra el juego, quita el 0.9.0 y reinstala exactamente `BokoblinExpansion_0.8.9_TEST.dusk`. Nunca dos versiones a la vez; no toques saves, CISO ni JSON.
