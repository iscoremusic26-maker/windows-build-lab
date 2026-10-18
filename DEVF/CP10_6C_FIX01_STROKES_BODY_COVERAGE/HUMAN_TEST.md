# CP10.6C-FIX01 — prueba humana, una sola ronda (Windows / Dusklight 2.0.3 / DX12)
Candidato técnico, no una versión "buena" garantizada. Solo tú puedes decidir si el aspecto es el buscado.
1. Cierra el juego. Respalda `BokoblinExpansion_0.8.4_TEST.dusk` (y `encounters.json`). Retira **cualquier** otra versión de `dev.bokoblin.expansion` (0.8.1, 0.8.2, 0.8.3 DIAG, 0.8.4) y deja **solo** `BokoblinExpansion_0.8.5_TEST.dusk`.
2. Mantén Provider 0.10.0 y editor 0.10.2 sin cambios. No toques saves, CISO/ISO ni JSON.
3. Arranca y comprueba `bokoblin_expansion` 0.8.5 en el log. Deben salir `DEVF_CP10_6C_FIX01 event=RESOURCE_PROBE ... ok=1` (y `RESOURCE_GEOMETRY`, `CURVE_PROBE`) para `0x0A58` y `0x0B9D`.
4. **Blue** (F_SP121, sala 3, capa 6): solo → sin líneas. Con otro Bokoblin vivo cerca (`BLUE_GROUP_ON`) → líneas **cian/azules** alrededor del cuerpo; mira por **ambos lados** y de frente.
5. **Black** (F_SP121, sala 1, capa 6): golpe **no letal** (`BLACK_FURY_START`) → líneas **violetas** por torso y flancos (no solo la espalda) + ojos púrpura. Gira la cámara: de frente, de costado y por detrás.
6. Observa: ancho, longitud, densidad, dirección (¿verticales?), brillo, estabilidad al caminar y girar, duración, fade de entrada/salida y qué pasa al cambiar de sala. Comprueba muerte y furia caducada (~10 s tras el último golpe): **sin líneas huérfanas**, y un Bokoblin nuevo no nace con ellas. Red: nada.
7. Envíame el **log completo** y capturas o un vídeo corto comparando 0.8.4 y 0.8.5: qué mejoró y qué empeoró (cobertura, tamaño, brillo, estelas de espada, objetos flotando lejos del cuerpo).
8. **Rollback:** cierra el juego, retira solo el 0.8.5 y reinstala exactamente `BokoblinExpansion_0.8.4_TEST.dusk`. No toques save/CISO/JSON.
