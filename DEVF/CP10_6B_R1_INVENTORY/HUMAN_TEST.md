# CP10.6B-R1 — prueba humana, una sola ronda corta (Windows / Dusklight 2.0.3 / DX12)
**Esto es un diagnóstico: NO verás ninguna aura y eso es lo correcto.** Solo queremos el log.
1. Cierra el juego. Haz copia de `BokoblinExpansion_0.8.2_TEST.dusk` y de `encounters.json`.
2. Retira **solo** `BokoblinExpansion_0.8.2_TEST.dusk` y pon `BokoblinExpansion_0.8.3_DIAG_TEST.dusk`. Mantén el Provider 0.10.0 y el editor 0.10.2. No dupliques `dev.bokoblin.expansion`. No toques saves, CISO ni `encounters.json`.
3. Arranca el juego (comprueba D3D12 y `bokoblin_expansion` 0.8.3 en el log). Entra a `F_SP121`, sala 3, capa 6 y quédate cerca de los Bokoblins (donde ya aparecían) unos 30-60 s. El inventario se lee cuando se ejecuta algún Bokoblin. Opcional: ve también a la sala 1, capa 6 y vuelve a la 3 (cada cambio de sala puede generar otra lectura del banco de escena).
4. No hace falta combatir. La lógica de Blue (grupo), Black (furia) y los ojos púrpura sigue igual si quieres comprobarla, pero no es el objetivo.
5. Cierra el juego y guarda el **LOG COMPLETO**. En él deben aparecer, con el prefijo `DEVF_CP10_6B_R1_INVENTORY`:
   - `INVT_BEGIN`, `INVT_IDS` e `INVT_END` para `bank=0` (común) con `integrity=SHOWN_PLUS_MISSING_EQUALS_UNIQUE` y `truncated=0`;
   - `INVENTORY_READY_COMMON`, `INVENTORY_STATS` y `INVENTORY_SHORTLIST` (puede no haber ninguna);
   - lo mismo para `bank=1` (escena) en cada sala/escena visitada, y `INVENTORY_COMPLETE`;
   - si algo falla: `INVENTORY_PARTIAL` o `INVENTORY_FAILED` con su motivo.
6. Envíame el log completo.
7. **Rollback** ante cualquier problema: cierra el juego, retira el 0.8.3 DIAG y restaura exactamente `BokoblinExpansion_0.8.2_TEST.dusk` (o el 0.8.1). Nunca dejes dos archivos de la Expansion. No depende de editar saves ni `encounters.json`.
