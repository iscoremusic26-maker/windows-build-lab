# Atribución y procedencia

**Origen.** El Enemy Spawner de Dawnlight, de BeZide93.
- Repositorio: https://github.com/BeZide93/dawnlight
- Commit leído: `b5e9046cd98bdd8593ab2cf219d72a0f61ef7213` (2026-11-01)
- Licencia: CC0 1.0 Universal (`LICENSE.md` del mismo commit)

**Qué se reutilizó, adaptado:**
- `src/enemy_spawner.hpp`/`.cpp`: la lista de enemigos (`kEnemySpawnerProfileLabels`, `kEnemySpawnerProfiles`, `kEnemySpawnerParameters`), el argumento de Helmasaurus, el centinela 0xFF de angle.z de Stalfos/Kargarok, la altura de los voladores, la distancia de 300 y la sonda de suelo desde 500 por encima.
- `create_test_actor_in_player_layer`: la capa de Link activa durante la creación y la capa anterior restaurada siempre. Es la corrección de propiedad de escena que BeZide93 confirmó en el juego.
- Las adaptaciones de Toadpoli de agua y de lava.

**Qué no se reutilizó:** el gancho de `fpcMtd_Method` y de `fopAcM_createChild`, el seguimiento de hijos, la puerta de arena del Boss Rush, el efecto de fuego importado de Dodongo, el Staltroop con IA sustituida, el Goron, la UI de Dawnlight y todo lo demás. No se copia ningún arte ni recurso.

**Diferencias de esta adaptación:**
- Las solicitudes se procesan en `mod_update`, al principio del frame, nunca dentro del callback de la UI.
- Comprobaciones de suelo: escalón, pendiente, agua o lava, y pared entre Link y el punto.
- Límite de 12 enemigos vivos creados por el mod.
- Lizalfos y Dynalfos usan el byte bajo 0xFF en lugar de 0, que en el código nativo es el estado inmóvil `ACTION_STAY`.
- Las entradas de Bokoblin se separan por recurso y por la clasificación de Expansion.

Las modificaciones nuevas no son de BeZide93 y no implican su aval.

**Validado contra:** Dusklight 2.0.3 (`TwilitRealm/dusklight@40457c6adb381928e4b5fef6ed459ed291edd5e2`), Bokoblin Expansion 0.17.0 y Enemy Variant Framework 0.15.0 (fuente `fe47c8b828939829914d466a425cb1e2c0223cd9`). Es inspección de código y laboratorio, no ejecución en el juego.
