# CP15 — IA Blue: evasión en solitario y agresividad escalonada en grupo

Expansion **0.12.0** = Expansion 0.11.0 (CP14) + la primera IA Blue. Provider 0.13.0 y editor 0.13.0 (CP13) no cambian y no se entregan; la CI los recompila solo para regresión.
Motor: Dusklight 2.0.3, `TwilitRealm/dusklight@40457c6`. Etiquetas: SOURCE (código), LAB (modelo de la fuente, no el motor), WINDOWS_MSVC, INDEPENDENT_DOWNLOAD, HUMAN.

## 1. Qué hace
| Nivel | Condición | Comportamiento de Blue |
|---|---|---|
| 1 solo | 0 compañeros elegibles | **Evasivo**: no se acerca a Link y corre alejándose por un rumbo comprobado como seguro; no empieza ataques nuevos. |
| 2 pareja | 1 compañero | Ataques B y C a **1,15x**; bloqueo posterior al ataque 20 → **17** ticks. |
| 3 grupo | 2 o más compañeros | Ataques B y C a **1,30x**; bloqueo 20 → **15** ticks. Cuatro o más no suman. |

- **Compañero:** cualquier Bokoblin vivo clasificado por Expansion (Blue, Red o Black) que cumpla el filtro CP10.6 **sin cambios** (`devf_dyn::eligible`: ≤650 XZ euclídeo, |Y|≤250, misma sala y escenario, snapshot reciente, vivo, no terminal). Cuenta como máximo 2.
- **Se conserva:** el bono de daño de Blue (1 solo / 2 en grupo), `BLUE_GROUP_ON/OFF`, aura y ojos cian. El nivel nuevo no los sustituye ni duplica. Nada de HP, daño por impacto, esferas, regen ni CP14 Red cambia. Red y Black no reciben ninguna acción de la IA (`applies_to` es solo Blue).

## 2. Lectura de la fuente exacta (`src/d/actor/d_a_e_oc.cpp`)
| Hecho | Línea | Consecuencia para el diseño |
|---|---|---|
| `executeFind` estado 4: persigue con `speedF` → 20, gira `shape_angle.y` hacia Link `clamp(diff/4, 0x100, 0x800)` por tick, ataca si 200<dist<400 y `abs(shape - angulo_a_Link) < 0x1000`. | 1222-1238 | Si el cuerpo mira en sentido contrario, el ataque nativo **no puede arrancar** en ese tick. |
| `current.angle.y = field_0x6da` mientras `field_0x6ce != 0` (desvío nativo de pared). | 1215-1219 | Se reutiliza para mover al Blue en el rumbo exacto sin tocar posición. |
| FIND → WAIT solo si `field_0x6c0 == 0` y `searchPlayer2()` falla; `searchPlayer()` exige mirar a Link (<0x4000). | 1309-1313, 307-329 | De espaldas, el Blue "perdería" a Link: se mantiene `field_0x6c0 ≥ 2` mientras huye o gira (ver §4). |
| `speedF` se ajusta con `cLib_chaseF` (paso 1.0) **después** de la escritura de PRE. | 1226-1250 | Se escribe el valor compensado para que el resultado sea exacto (LAB, 144 combinaciones). |
| `checkBeforeBg()` (línea de pared de 300 desde y+100) y `checkBeforeFloorBg(arg)` (agua profunda a `arg`) miran en `shape_angle.y`. **No existe ninguna comprobación nativa de precipicio.** | 441-525 | Se reutilizan con el rumbo candidato; los precipicios se sondean aparte (altura de suelo, §3). |
| `executeAttack` estado 0 desacelera `speedF` a 0, elige B (`setBck(5,0,5,1.0)`) o C (`setBck(6,...)`) y fija estado 1/2; en 1/2 `OnAtSetBit` si `14 ≤ frame ≤ 22`, sonidos con `checkFrame`, avance por tabla acumulativa `oc_attack{b,c}_trans`, y el retroceso por escudo escribe `setPlaySpeed(-1.0)`. | 1379-1530 (retroceso: 1478-1482) | La tasa se fija **una vez**, justo tras `setBck`; no se vuelve a escribir. |
| `J3DFrameCtrl::update` suma `mRate` por tick y `checkPass` usa `mRate`. | J3DAnimation.cpp 23-48, 141-157 | Los sonidos por fotograma disparan una vez con cualquier tasa; la ventana [14,22] contiene ticks; el desplazamiento acumulado no depende de la tasa (LAB). |
| `executeFind` estado 0 tras un golpe fija `field_0x6c2 = 20` (bloqueo de ataque). | 1153-1156 | Es el único temporizador de "pasividad" que se escala; no se tocan recuperaciones ni animaciones. |
| `mMoveRange = 100 * (home.angle.x & 0xFF)` (correa). Los Bokoblin DEVF suelen tener 600. | 2733 | La huida respeta la correa; si el nativo ya está fuera de ella (MOVE_OUT) la IA no actúa. |

Hook: **uno** (`daE_OC_c::execute`, PRE+POST), el mismo de CP14. No hacen falta hooks nuevos: todo se resuelve con escrituras acotadas a campos nativos del propio actor dentro de su PRE/POST.

## 3. Evasión del Blue solitario (`blue_ai_policy.hpp`, `step_evade`)
**Cuándo actúa:** nivel 1 (tracker), modo nativo `FIND` estado 4, sin evento en curso, dentro de la correa, vivo y no oculto. Cualquier otro modo (daño, muerte, caída, agua, demo, ataque en curso, espera…) conserva su prioridad nativa: la IA no escribe nada.
**Qué hace por tick:**
- **Rumbo:** elige un rumbo alrededor de "alejarse de Link" (0, ±30°, ±60°, ±90°; nunca más de 90°, es decir nunca con componente hacia Link). Mantiene el anterior mientras sea seguro (evita oscilaciones). Reevalúa cada 3 ticks.
- **Sondeo de cada rumbo** (solo lecturas del motor): pared con `checkBeforeBg()` (cerca <100 = bloqueado; 100-300 = "flojo"), agua profunda con `checkBeforeFloorBg(200/400)`, altura de suelo con `fopAcM_gc_c::gndCheck` a 150/300/450 (sin suelo o un escalón de más de 100 = precipicio), y la correa. Se elige el primer rumbo "limpio"; si solo hay "flojos", el primero; si no hay ninguno, **retiene** la posición.
- **Escrituras:** cuerpo hacia el rumbo con giro acotado (0x2000/tick; el nativo contra-gira como mucho 0x800), `field_0x6ce=2` y `field_0x6da=rumbo` (el movimiento sigue el rumbo exacto), `field_0x6c0 ≥ 2` (que el cuerpo de espaldas no dispare la regla nativa FIND→WAIT) y `speedF` compensado hacia 20 con rampa de +2 por tick (20 es la velocidad nativa de persecución; no es más rápido).
- **El giro del cuerpo evita el agua:** la prueba nativa de agua mira en `shape_angle.y`; un giro de 180° que pase por una dirección con agua a 200 convertiría la persecución en `FIND_STAY`. El sentido del giro se cambia si el siguiente paso miraría al agua; si ambos están mal, no se gira ese tick (el movimiento lo gobierna el desvío, no la orientación).
- **Sin ruta segura:** `speedF` se compensa a 0 exacto (sin avanzar), no gira y mantiene la alerta. **Es la salida a prueba de fallos; no se avanza hacia Link ni se atraviesa nada.**
- **Link lejos (≥1200, con histéresis +300 mientras corre):** no se acerca (la persecución nativa lo haría) ni huye; mantiene la alerta solo mientras da la vuelta para volver a mirar a Link. El radio 1200 es el nativo `battle_participation_radius` (L102) = `plyr_srch_max_radius_S` (L99).
- **Fin de la evasión por llegar un compañero:** si el nativo sigue en FIND/4, se mantiene la alerta hasta 30 ticks mientras el Blue gira para volver a mirar a Link (si no, caería a WAIT de espaldas).
- **Ataques:** no se empieza ninguno. En el POST, si la ejecución nativa pasó a `ATTACK` estado 0 desde otro modo, se restauran `mActionMode/mOldActionMode/mOcState` y los dos temporizadores (veto). **Un golpe ya en curso no se toca nunca.**

**Rutas descartadas:** reducir `mPlayerRange` (no huye; ya hecho en CP05); sustituir `action()` o saltarse el original (viola "sin sustitución masiva"); escribir posición (teletransporte, prohibido); hookear `executeFind` (más superficie sin necesidad); caminar hacia atrás mirando a Link a −15 como `FIND_STAY` (el nativo no comprueba el suelo a la espalda).

## 4. Velocidad de ataque
- **Tasas:** pareja **1,15x**, grupo **1,30x** (candidatos del encargo, validados por LAB; no se promete más).
- **Cuándo:** en el POST de la ejecución que arrancó el golpe (`mode=ATTACK`, estado 1/2, `bck 5/6`, tasa exactamente 1.0, fotograma 1.0). Se escribe `setPlaySpeed` una vez. Si la tasa no es 1.0 (retroceso por escudo −1.0, otro mod) o el golpe ya corre, no se escribe (SKIP + log).
- **Cambio de grupo a mitad de golpe:** no cambia la tasa de ese golpe; el siguiente usa el nivel vigente (LAB).
- **Resultados LAB (animación de 46/60/80 fotogramas; la longitud real no figura en la fuente):**

| Golpe | Ticks nativo | Ticks pareja | Ticks grupo | Ticks de contacto (nativo / pareja / grupo) |
|---|---|---|---|---|
| B o C, 60 fotogramas | 60 | 53 | 47 | 9 / 7 / 7 |
| B o C, 46 fotogramas | 46 | 41 | 36 | 9 / 7 / 7 |
| B o C, 80 fotogramas | 80 | 70 | 62 | 9 / 7 / 7 |

  Los sonidos por fotograma (B: 10, 12,5, 19; C: 7, 11,5) disparan exactamente una vez y en el mismo orden; el desplazamiento acumulado del golpe es idéntico (±1e-3); los ticks de contacto son contiguos. La ventana de contacto dura 7 ticks en lugar de 9: es la consecuencia directa de "golpear más rápido" y el contacto sigue en los fotogramas 14-22.
- **Bloqueo posterior al ataque** (`field_0x6c2`): 20 → 17 (pareja) / 15 (grupo) = 20/tasa redondeado. Solo se reescribe el valor exacto 20 en el tick que ejecuta FIND estado 0 justo tras un golpe. No se tocan el tiempo de desaceleración previo al golpe (`speedF` 0 → hasta 20 ticks), las recuperaciones, la orientación ni la seguridad de suelo.
- **Cadencia (LAB):** intervalo medio entre golpes nativo > pareja > grupo.

## 5. Grupo y niveles
`scan_group` recorre el ledger CP10.6 una vez y cuenta hasta 2 compañeros distintos. Los candidatos nuevos sobre los ya contados se verifican con la comprobación autoritativa (id sigue resolviendo al mismo actor vivo), como hace CP10.6 al encender el bono. Transiciones: subir o cambiar entre 2 y 3 es inmediato; bajar a solo exige 3 ticks solitarios consecutivos (0,1 s): absorbe un snapshot transitorio y retrasa una muerte real como mucho 3 ticks. Un Blue nace solo. El estado es por actor y generación y no se hereda.

## 6. Qué cambió y qué no
- **Nuevo:** `blue_ai_policy.hpp` (política pura, lógica del mod y del lab) y el adaptador en `mod.cpp` (2 líneas de CP14 sustituidas: la inclusión de `red_policy.hpp` y la versión; todo el resto de `mod.cpp` de CP14 sigue presente y en orden, más 144 líneas nuevas; gate de CI).
- **Idéntico a CP14 byte a byte (gate de CI):** los otros 22 ficheros de `runtime/` (incluido `red_policy.hpp` y las tres cabeceras compartidas con los parches de Red).
- **Lista cerrada de escrituras al actor** (gate de CI): `health` (4, CP14), `shape_angle.y` (5), `field_0x6ce`, `field_0x6da`, `speedF`, `mActionMode`, `mOldActionMode`, `mOcState`, `field_0x6c0` (2), `field_0x6c2` (2) y `setPlaySpeed` (3). Sin posición, sin `setActionMode/setBck`, sin tocar `mPlayerRange/mMoveRange`/HIO.
- **Imports de juego nuevos** (todos de solo lectura o consulta; verificados en la `windows-amd64.lib` fijada y en el audit de PE): `daE_OC_c::checkBeforeBg`, `daE_OC_c::checkBeforeFloorBg`, `daE_OC_c::checkBck`, `fopAcM_gc_c::gndCheck`, `fopAcM_gc_c::mGroundY` (dato), `dComIfGp_event_runCheck`.
- **Sin cambios:** hook (1, PRE+POST), Provider, editor, JSON de encuentros, schema, servicios, capacidad 32, presupuesto de 8 auras, CP14 Red, CP12/CP13.

## 7. Límites y riesgos (honestos)
- **LAB ≠ motor:** el lab ejecuta la política contra un **port** de la fuente (executeFind, executeAttack, J3DFrameCtrl, cLib), no contra el juego. Longitudes de animación, geometría real, sonido, imagen y FPS **no** se han observado. Todo el comportamiento es `HUMAN_RUNTIME_PENDING`.
- **Aprovecha detalles de la FSM nativa:** `field_0x6ce/6da`, `field_0x6c0`, `field_0x6c2` y la estructura de `executeFind` salen de la fuente exacta 40457c6; otro build los invalidaría. Hay un canario (`ai_sane`) que desactiva la IA para un actor con campos fuera de rango (`AI_CANARY_FAILED`).
- **Sondeo de agua en 2 puntos (200/400):** una franja de agua estrecha podría escaparse del muestreo. El agua profunda real del juego tiene extensión y el rumbo se reevalúa cada 3 ticks.
- **Sin precipicio nativo:** el sondeo de suelo (150/300/450) es lo único que evita saltar al vacío; un hueco más corto que el espaciado podría no verse. El rumbo "flojo" (pared a 100-300) se acepta solo si no hay uno limpio.
- **Estado compartido del motor:** `fopAcM_gc_c::gndCheck` reutiliza el comprobador estático global; las demás llamadas del motor lo reescriben justo antes de usarlo, así que no hay interferencia, pero es estado compartido.
- **Visual:** el Blue corre de espaldas con la cabeza girada hacia Link por el contra-giro nativo; animaciones: las nativas (carrera de persecución). En "sin ruta segura" puede quedar la animación de andar sin avanzar (el nativo solo cambia a reposo en una de sus ramas).
- **Veto de ataque:** restaura 5 campos tras `setActionMode(ATTACK)`; `offHeadLockFlg` y los bits de colisión quedan como los dejó el nativo (ya activos).
- **Correa:** con `mMoveRange > 0` el Blue solo huye dentro del círculo; cerca del borde puede quedarse en retención.
- **Sin pathfinding:** rodea obstáculos solo con los 7 rumbos locales; puede quedar atrapado en una esquina cóncava (retención).
