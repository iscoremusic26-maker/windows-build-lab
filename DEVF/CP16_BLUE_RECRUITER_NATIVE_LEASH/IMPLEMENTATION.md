# Implementación CP16

Base runtime realmente compilada CP15: `9fddfd9d445ade5d1ae5b4cc6f7d8380fc99f336`; rama nueva desciende de su cierre documental `933f249bbf422606d35dd8b55bf1afcbe30722f3`. El delta entre ambos solo contiene TEST_RESULTS y VERIFIED_PACKAGE. CI deriva de `ea52ba92248a150dda0a74fe3d4b43f014bd27e4`. No cambia PR71/56, Provider, editor, SDK o JSON.

## Contrato nativo exacto

Dusklight `40457c6adb381928e4b5fef6ed459ed291edd5e2`, `src/d/actor/d_a_e_oc.cpp`:

- searchPlayer307–330: detección nativa propia, orientación/distancia/LOS; excepción nativa muy cercana por Link rápido. No se llama searchPlayer2 desde el adaptador: sus puentes y switches no son detección directa.
- setActionMode584–595: actualiza old/mode/state, borra c0/c2, restituye bits nativos de colisión/cabeza. Se invoca el método tipado, nunca se imitan parcialmente sus escrituras.
- executeFind1138–1290: en estado4 la ofensiva requiere c2==0. PRE puede prestar c2=2 cuando era0/1; execute lo decrementa a1. POST solo devuelve el original decrementado si siguen modo/estado/valor esperados. No hay veto POST ni reversión de ATTACK.
- executeWatch1726–1775: entrar en WATCH3 produce BCK7+VOICE_CRY; WATCH4 retorna a FIND2 al parar el clip. No simula un compañero muerto. Tope120 ejecuciones propias si sigue exactamente WATCH4/BCK7 y estado seguro.
- executeSoundWatch1778–1829: alerta nativa60–89 ticks, locomoción hacia mWatchPos, luego WAIT; puede pasar antes a FIND por searchPlayer2. Su temporizador de carrera60 se mantiene≥2 solo bajo orden consumida propia y limitada. Colisión, giro y movimiento los ejecuta el juego, sin escrituras speedF/posición.
- action2410+: daño, agua y caída preceden al dispatch; radio home/mMoveRange permanece intacto. Cero omite el control radial nativo, pero no nuestros filtros de contexto. MOVE_OUT, ataque/escudo, daño, demos/eventos/transiciones nunca se fuerzan a reunión.

Imports nuevos revisados: `?setActionMode@daE_OC_c@@QEAAXHH@Z`, `?searchPlayer@daE_OC_c@@QEAA_NXZ`, `?dComIfGp_isEnableNextStage@@YAHXZ`. Firma MSVC x64 según clase tipada oficial; membresía en importlib oficial, link MSVC e IAT PE deben verificarse por CI antes de entregar. Se eliminan los imports de evasión checkBeforeBg/checkBeforeFloorBg/gndCheck/mGroundY. Un solo hook execute PRE/POST, ningún hook nuevo.

## Estado y límites

Tabla64 por id/generación, sin nuevos punteros de actores. Guarda valores propios, contexto exacto de stage (no solo hash), room y `getLayerNo(0)` en la ejecución propia. La capa es el contexto activo del motor, no una capa DZR inventada. Frescura usa el presupuesto existente de pares execute del ledger. Lookup autoritativo de id/dirección/generación al consumir y cada15 ejecuciones receptoras; contexto, frescura y correa se revisan cada PRE. Despacho mediante snapshots, no escritura de otro actor. Primer convocante válido conserva su orden; Blue con orden entrante no propaga automáticamente.

Llamada inmediata por detección propia y repetición cada150 ejecuciones de ese Blue, sin límite de rondas. Primer grito una vez; calma de150 ejecuciones sin detección, FIND/ATTACK/FIND_STAY ni orden reinicia el combate. El culling no avanza estos relojes; la frescura global evita retener a Blue por un compañero que dejó de ejecutar. No se mide tiempo de pared.

Orden180 ticks propios; extensión a240 solo con progreso reciente medido. Alerta inicial no se confunde con atasco. Una vez en carrera o combate nativo, 60 ticks sin avance de50 unidades hacia el objetivo abortan; backoff90. Se informa APPROACH_STARTED solo con desplazamiento≥5 y ARRIVED solo por posiciones reales≤650/250. Entrar FIND/ATTACK es ENGAGED_NATIVE_MODE, no llegada ni daño; se conserva observación acotada para que Blue no abandone la espera solo por un cambio de modo.

Espera Blue≤240, gracia de oferta no consumida2 ejecuciones del receptor; no aliado válido→fallback y descanso150. FIND3/4 usa el préstamo PRE del temporizador; FIND_STAY seguro usa WATCH0 alerta, no grito. Ataques iniciados, guardia, eventos, daño, caída/agua y MOVE_OUT tienen prioridad. Esto no detiene defensas ni obliga a atravesar obstáculos. Bonus y velocidad usan su filtro previo independiente650/250; una orden jamás concede bonus.

## Lista cerrada de escrituras nuevas

- Solo actor ejecutando, clasificado y seguro: mWatchPos.xyz con respaldo/valor esperado, más setActionMode(SOUND_WATCH,0). Revalidar objetivo/correa antes de refrescarlo cada15 ticks.
- c0≥2 solo SOUND_WATCH2 de orden propia; cancelación segura por setActionMode(WAIT,0), nunca por asignación de mode/old/state.
- Blue seguro: setActionMode(WATCH,3) y watch target de Link leído, o WATCH0 para espera FIND_STAY; fallback visual FIND2 solo si sigue clip propio.
- c2 PRE préstamo2 / POST restauración condicional del valor esperado; no reset POST de acciones/flags/animación.
- Tasas de animación y lockout CP15 conservados literalmente. Las cuatro escrituras HP anteriores no se modifican.

Al salir de su acción, se restaura watch target solo si sigue siendo el escrito. No se restituye un target mientras el nativo todavía lo consume en SOUND_WATCH. Detach frío no cambia modos; un SoundWatch nativo residual termina por su temporizador, sin punteros ni estado persistido. Hot unload no está soportado. Limpieza4 slots/pase, sin censo global; tabla de política12288 bytes, leases/velocidad acotados; logs transición≤1200. No se afirma FPS.

El gate comprueba22 archivos runtime idénticos a CP15/CP14 y todas las líneas fuera del adaptador Blue idénticas salvo versión/include/calls y reemplazo explícito de telemetría inicial. Las suites históricas de evasión se ejecutan sobre CP15 histórico; no se presentan como comportamiento CP16. El port de FSM usa mundo sintético: no demuestra navegación, clips ni sonido del juego.
