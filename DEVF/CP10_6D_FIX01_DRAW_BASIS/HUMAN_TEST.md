# CP10.6D-FIX01 — prueba humana (Windows / Dusklight 2.0.3 / DX12)
1. Cierra el juego. Respalda `BokoblinExpansion_0.8.6_TEST.dusk` (y `encounters.json`). Retira **todas** las versiones anteriores de `dev.bokoblin.expansion` y deja **solo** `BokoblinExpansion_0.8.7_TEST.dusk`.
2. Mantén Provider 0.10.0 y editor 0.10.2. No toques saves, CISO ni JSON.
3. Arranca. En el log deben aparecer `version=0.8.7`, `RESOURCE_FOUND resource=0x0B9D... used_by=BLUE_AND_BLACK` y, por cada efecto, `DRAW_BASIS_CHECK ... status=NON_DEGENERATE` (esto es cálculo, no prueba de píxeles).
4. **Blue** (F_SP121, sala 3, capa 6): solo → nada. Con otro Bokoblin cerca (`BLUE_GROUP_ON`) → trazos **azul/cian**.
5. **Black** (F_SP121, sala 1, capa 6): golpe **no letal** (`BLACK_FURY_START`) → trazos **violeta/púrpura** + **ojos púrpura**.
6. Mira de **frente, de lado y por detrás**: ¿se ven los trazos? ¿rodean el torso y los costados, o se amontonan en la cintura? ¿son finos y casi verticales (≈6° de inclinación es lo esperado)? ¿Blue y Black tienen la misma forma con distinto color?
7. Fades y limpieza: entrada/salida suaves; al terminar el grupo o la furia, al morir y al cambiar de sala no queda ninguna línea. Red: nada.
8. Envíame el **log completo** y capturas o vídeo de **ambas** variantes.
9. Si los trazos siguen **sin verse**, márcalo como **FAIL** (y adjunta el log); no hace falta más.
10. **Rollback:** cierra el juego, quita el 0.8.7 y reinstala exactamente `BokoblinExpansion_0.8.6_TEST.dusk`. Nunca dos versiones a la vez; no toques save/CISO/JSON.
