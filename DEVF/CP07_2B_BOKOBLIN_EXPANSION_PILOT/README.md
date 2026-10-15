# DEVF CP07.2B y Bokoblin Expansion 0.1.0 — piloto privado

**PRIVATE_TEST_READY / HUMAN_RUNTIME_PENDING.** Hay dos .dusk independientes y verificados: proveedor DEVF0.7.3 y consumidor Bokoblin Expansion0.1.0. El juego no fue ejecutado por Codex.

El proveedor conserva el ID `dev.devf.cp06_native_config_dual`, el editor FIELD/NORTH, la colección/exportación y `dev.devf.native_placement@1.0`. Publica además `dev.devf.encounter_placement@1.0`. El consumidor `dev.bokoblin.expansion` importa obligatoriamente este servicio nuevo. No importa StageService ni accede a los archivos del proveedor.

## Capacidad de este corte

Solo DEVF registra records nativos E_oc/E_OC2 mediante StageService. El nuevo contrato acepta dos solicitudes distintas del mismo consumidor, devuelve tickets y valida F_SP121/room3/layer6, profile510, XYZ float32 finitos, yaw s16 y parámetrosFFFF0100. El límite global es DOS ediciones, contando JSON legado y ambos servicios. No se aplica HP, color ni IA personalizada.

El recibo antiguo conserva site0/1=FIELD/NORTH. El nuevo usa flags/ticket/count y no redefine esos sitios. Se rechazan IDs o posiciones exactas duplicados, conflictos con JSON, propietarios desconocidos, exceso de capacidad y solicitudes posteriores al primer update. La atribución ModContext sigue el contrato cooperativo del SDK; no autentica código nativo hostil.

| Encuentro del usuario | XYZ sin corrección vertical | Yaw |
| --- | --- | ---: |
| encounter-1 / A | (5394.84716796875,-5603.69873046875,51442.3046875) | -2421 |
| encounter-2 / B | (3528.7822265625,-5850.8798828125,51201.09375) | -18402 |

Ambos son capturas reales comunicadas por el usuario, con quantity1 y terrain_verified=false. **No son puntos de spawn acreditados.** El proveedor no compila estas coordenadas; solo el consumidor incluye el header generado. No hay una lista A/B rígida dentro de la API.

## Configuración del consumidor

Su archivo propio es `qa.json`, normalmente en:
`%APPDATA%\TwilitRealm\Dusklight\mod_data\dev.bokoblin.expansion\qa.json`.
Usar el data_dir activo si la instalación tiene una ubicación personalizada.

```json
{"schema_version":1,"mode":"A_ONLY"}
```

Modos: NONE=0, A_ONLY=1, BOTH=2. Los ejemplos están bajo examples/expansion en fuente y examples en el ZIP del consumidor. Copiar el elegido como qa.json con el juego cerrado. Si falta, el consumidor no solicita nada; JSON inválido o error de lectura falla limpiamente sin solicitudes. Nunca se escriben defaults activados ni archivos del proveedor.

Para aislar esta QA usar placements.json del proveedor en NONE, conservando respaldo. El proveedor sigue funcionando sin el consumidor. Capturar, guardar o exportar en el editor no crea enemigos.

## Alcance y recuperación

StageActorHandle significa edición, no ActorId vivo. El consumidor hace rollback inverso de sus tickets; el proveedor mantiene ownership y watch de detach. remove_actor_edit no destruye actores vivos. No hot reload: cambios solo con cierre completo y nuevo arranque.

No se implementan grupos, separación aplicada, política de respawn personalizada, estadísticas especiales ni seis variantes. El motor puede recrear las ediciones en cargas posteriores; la reentrada de estas ubicaciones aún no fue observada.

La entrada está reconstruida del JSON completo del prompt y tiene un hash propio. El SHA del adjunto original se conserva como USER_REPORTED; no se simula acceso a sus bytes. Para regenerar datos: ejecutar tools/generate_expansion.py con un nuevo export, revisar el header y compilar un consumidor NUEVO. Este corte requiere exactamente dos capturas con quantity1 en F_SP121/3/6.

MSVC:346 casos C++,20 comprobaciones del generador,28 negativos PE/ZIP y descarga real independiente PASS. Véanse TEST_RESULTS, BUILD_AND_PACKAGE, HUMAN_TEST y RECOVERY. SDK y biblioteca originales intactos; no cambios al host, ROM, saves, renderer u otros mods. Sin merge.
