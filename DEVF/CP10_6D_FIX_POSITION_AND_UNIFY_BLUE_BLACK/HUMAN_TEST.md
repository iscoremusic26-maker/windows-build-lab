# CP10.6D — prueba humana (Windows / Dusklight 2.0.3 / DX12)
1. Cierra el juego. Respalda `BokoblinExpansion_0.8.5_TEST.dusk` (y `encounters.json`). Retira **todas** las versiones anteriores de `dev.bokoblin.expansion` (0.8.1–0.8.5, 0.8.3 DIAG) y deja **solo** `BokoblinExpansion_0.8.6_TEST.dusk`.
2. Mantén Provider 0.10.0 y editor 0.10.2. No toques saves, CISO ni JSON.
3. Arranca. En el log: `DEVF_CP10_6D event=RESOURCE_FOUND resource=0x0B9D... used_by=BLUE_AND_BLACK`, y una línea `RESOURCE_GEOMETRY`.
4. **Blue** (F_SP121, sala 3, capa 6): solo → nada. Con otro Bokoblin cerca (`BLUE_GROUP_ON`) → el **mismo efecto que el Black pero azul/cian**. Log: `TUNING_APPLIED variant=BLUE ... route=SHARED_CANONICAL_RING`.
5. **Black** (F_SP121, sala 1, capa 6): golpe **no letal** (`BLACK_FURY_START`) → el efecto en **violeta/púrpura** + **ojos púrpura**. Log: `TUNING_APPLIED variant=BLACK ... route=SHARED_CANONICAL_RING` con la misma `geometry=(...)` que el Blue.
6. Mira de frente, de lado y por detrás: ¿el efecto **rodea el torso** (espalda y costados) o sigue **amontonado en la cintura / espalda baja**? ¿Blue y Black tienen la **misma forma**, distinto color? ¿Tamaño y brillo razonables (ni lluvia ni resplandor gigante)?
7. Limpieza: al acabar el grupo o la furia, al morir y al cambiar de sala no debe quedar ninguna línea; un Bokoblin nuevo no nace con ellas. Red: nada.
8. Envíame el **log completo** y capturas o un vídeo corto (Blue y Black, varios ángulos).
9. **Rollback:** cierra el juego, quita el 0.8.6 y reinstala exactamente el `BokoblinExpansion_0.8.5_TEST.dusk`. No toques save/CISO/JSON.
