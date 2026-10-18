# CP10.6D-FIX02 — prueba humana (Windows / Dusklight 2.0.3 / DX12)
1. Cierra el juego. Respalda `BokoblinExpansion_0.8.7_TEST.dusk` (y `encounters.json`). Retira **todas** las versiones anteriores de `dev.bokoblin.expansion` y deja **solo** `BokoblinExpansion_0.8.8_TEST.dusk`.
2. Mantén Provider 0.10.0 y editor 0.10.2. No toques saves, CISO ni JSON.
3. En el log: `version=0.8.8`, `route=SHARED_BODY_HUGGING_STROKES` para Blue y Black, y en `TUNING_APPLIED` `centre=TORSO_COLLIDERS` y `base_prm_effective=YES` (si dice `NO_RESOURCE_ANIMATES_PRM`, avísame).
4. **Blue** (F_SP121, sala 3, capa 6): solo → nada. Con otro Bokoblin cerca (`BLUE_GROUP_ON`) → trazos **azul/cian**.
5. **Black** (F_SP121, sala 1, capa 6): golpe **no letal** (`BLACK_FURY_START`) → trazos **violeta/púrpura** + **ojos púrpura**.
6. Mira de frente, de lado y por detrás:
   - ¿El efecto está **más pegado al cuerpo** (sobre el torso, los costados y la espalda) en vez de un aro suelto alrededor?
   - ¿Tiene **más presencia** y se ve menos tenue, sin llegar a resplandor gigante ni a tapar al enemigo?
   - ¿Siguen siendo **líneas finas casi verticales**?
   - ¿Blue y Black tienen la **misma forma** con distinto color?
7. Limpieza: al terminar el grupo o la furia, al morir y al cambiar de sala no debe quedar ninguna línea. Red: nada.
8. Envíame el **log completo** y capturas o vídeo de **ambas** variantes (idealmente comparando con 0.8.7).
9. **Rollback:** cierra el juego, quita el 0.8.8 y reinstala exactamente `BokoblinExpansion_0.8.7_TEST.dusk`. Nunca dos versiones a la vez; no toques save/CISO/JSON.
