# Implementación CP17 FIX01 — Red x2, contacto, cargas, aproximación e intercepción de saltos

Base: CP17 `feature/devf-cp17-red-aggressive-ai` @ `7b8a7c5` (cabecera documental; el binario CP17 0.14.0 probado por el usuario se compiló desde `8fbaea7`). Evidencia humana: rama `evidence/devf-cp17-human-runtime-20261018-202114` @ `6ccb76f`, log reconstruido con SHA-256 `186616fa…f9b8` (6 755 líneas), sin modificar ni fusionar.

## Diagnóstico de 0.14.0 a partir del log y del código

- Telemetría: 672 líneas de detalle, todas `LINK_REACTIVE_SAMPLE_OBSERVED`, y 40 555 suprimidas. La condición `ticks==0 || link_id!=id` se evaluaba antes de la política; la política solo guardaba `link_id` tras el retorno temprano de los modos no combativos. Mientras un Red estaba fuera de combate, cada tic emitía la línea hasta agotar su cupo de 48 (14 Reds × 48 = 672). El laboratorio reproduce el predicado antiguo: 1 000 líneas en 1 000 tics, frente a 1 con la corrección.
- Contacto x3: el bit AT nativo se decide con el fotograma previo a `play` (14..22). A x3 solo se registraban las poses 19, 22 y 25 (3 de las 9 nativas, 15..23); las poses 15..18 nunca existían para la colisión. Además, el contador `native_contact_flags=17` solo contaba contactos con el golpe todavía en estado 1/2: el retroceso por escudo (estado 3) los excluía, aunque CP14 registró 88 `BLOCKED_BY_SHIELD_LINK_SIDE` y 15 golpes confirmados.
- Cargas: una única tirada del 70 % por episodio, consumida también cuando fallaban radio, cooldown o LOS. El cooldown era compartido con el aleatorio y la aproximación era el FIND nativo, sin aceleración.

## Tasas: x1.15 normal, x2 rápido y fase par

`kNormal=1.15f`, `kBurst=2.0f`. El 3.0 desaparece de toda ruta de juego; solo se menciona como historia de 0.14.0, y un test reproduce su defecto de SFX.

Hay una sola decisión por golpe B/C fresco, en el fotograma 1, con tasa 1 y el modo ATTACK/0 observado en PRE. Para x2 esa decisión se **aplica** en el fotograma 2: es la misma decisión, no un nuevo retiming, y si entre medias cambian el clip, la tasa o el fotograma, no se escribe (`RATE_SKIPPED`).

- Con fase par, los fotogramas previos a `play` son 2, 4, …, 14, 16, 18, 20, 22. Eso da 5 registros AT (poses 16..24) en lugar de 4 (17..23). El primer tic activo sigue siendo t+8 desde el inicio.
- Los SFX `else-if` se verifican a x2 para cualquier fase (B: 10/12.5/19; C: 7/11.5), así que la compensación de SFX de 0.14.0 se elimina junto con su import `startCreatureSound`.
- La traslación nativa acumulada no cambia: B 112, C 91.5.

## Contacto y escudo (A)

Opciones evaluadas sobre el código oficial:

1. **Orden de actualización / fase.** Adoptado como se describe arriba.
2. **Colisión continua real.** `dCcD_Sph::MoveCAt` solo fija `AtVec`, que se usa como dirección de impacto en `d_cc_s.cpp` y no como barrido. E_OC no tiene `Cps`. Crear colisionadores propios del mod exigiría objetos nativos nuevos (vtables, ciclo de vida, registro en `dCcS`) y además la reacción nativa de escudo (`mSphs_at[i].ChkAtHit` en `executeAttack`) y la atribución CP14 no los verían. Rechazado.
3. **Uso temporal y acotado de los colisionadores nativos.** Adoptado. En POST, ya ejecutado `cc_set` y antes de `dCcS::Move`, se colocan las dos esferas AT nativas, durante una única pasada, sobre poses equivalentes a las nativas. El siguiente `cc_set` reescribe centro y radio, así que la restauración es automática. Se usan los setters nativos `cM3dGSph::SetC`/`SetR`, los dos únicos imports nuevos.

Regla por tic de un golpe x2 propio y registrado:

- Candidatas: punta y empuñadura actuales (exactas) y punta y empuñadura de la pose saltada. Estas se calculan como el punto medio de la empuñadura más el vector medio de la hoja, con el radio reducido por una cota de error de interpolación (segunda diferencia / 8; mínimo 33).
- Primero se eligen las que alcanzan el cilindro de Link (r35, h180; las exactas tienen prioridad); después, la punta actual y la punta saltada.
- En el tic posterior a la ventana nativa (pose 24) solo se registra la pose 23, o la pose 22 exacta como respaldo. Así nunca queda registrada la pose 24.
- Se omite la colocación si hay contacto con pared, salto de pose (>150 o >100°), longitud de hoja incoherente o radios nativos no observados.

Escudo y daño siguen siendo nativos: Link bloquea, E_OC hace retroceso con su propio `ChkAtHit` (`setPlaySpeed(-1)`, estado 3) y CP14 atribuye el golpe por la identidad del puntero de la esfera nativa. No se escriben AtHit, HP ni ATP.

Laboratorio (geometría sintética, 18 columpios, rejilla de 10; **no es colisión del motor**):

| Variante | Celdas nativas no cubiertas |
|---|---|
| x2 simple con fase impar | 2431 / 13272 (18,3 %) |
| x2 simple con fase par | 1328 (y 230 celdas fantasma de la pose 24) |
| FIX01 | 1187 (8,9 %); fantasma 2 celdas a ≤1,4 u del barrido continuo, ruido por debajo de la rejilla |

## Cargas (B)

Sin tirada: todo episodio elegible selecciona reacción.

- **Elegibilidad:** contexto, LOS, correa, canario, radio 900 y cooldown reactivo de 45 tics propios. Se reevalúa en cada tic mientras no haya intento, y una diferencia (`CHARGE_REACTION_DEFERRED`, registrada una vez) no consume el episodio.
- **Consumo del episodio:** solo lo consume un intento real, es decir, un ATTACK forzado o un golpe nativo fresco con esa causa. Si el motor anula el ATTACK forzado en el mismo tic (agua, daño o caída), el episodio sigue abierto (`NATIVE_ATTACK_FORCED_PREEMPTED`).
- **Cooldowns separados:**
  - Reactivo (cargas y pasividad): 45 desde el inicio del golpe reactivo.
  - Aleatorio: 12 % por golpe elegible, con 120–210 tics y un golpe normal entre ráfagas.
  - El cooldown de uno no bloquea al otro.
- **Inicio del golpe:** con la intención activa, se inicia el ATTACK nativo cuando la distancia 3D es menor de 400, el Red encara a Link dentro de 0x1000 y el bloqueo nativo `field_0x6c2` vale 0. El inicio es desde FIND estado 4 (con el suelo comprobado el tic anterior) o desde ATTACK estado 0. Incluye el caso a menos de 200, donde el nativo retrocede. También cuenta un golpe que el nativo inicie por sí mismo. El golpe va a x2.

## Aproximación rápida y segura (C)

Se implementa en FIND estado 4, solo durante una reacción (carga, pasividad o intercepción pendiente).

- **Condiciones:** el motor solo persigue a 20 cuando `lockout==0 && dist≥300` o `lockout≠0 && dist≥400` (márgenes de 302 y 402); se exigen además velocidad nativa entre 0 y 20, sin evento ni orden de reclutamiento, sin pared y correa con margen 100+30.
- **Escritura en PRE:** con `n=chase(v,20,1)` (el valor nativo de este tic) y `r=1.5·n`, se escribe `X=r±1` para que el `cLib_chaseF` nativo produzca exactamente `r`. `posMoveF` y `CrrPos`, con paredes y suelo, mueven `r`.
- **Restauración en POST:** se escribe el valor nativo `n`. Si el nativo pasó a WATCH o FIND_STAY antes del chase, se devuelve el valor previo `v`. Si otra escritura cambió `speedF`, no se toca (`RUSH_RELEASED`).
- **Límite documentado:** en el tic exacto en que `searchOtherOc` activa WATCH, ese tic se mueve `X` en lugar de `v` (≤11 u). No se puede predecir desde PRE sin recorrer otros actores; queda registrado como `RUSH_TICK_PREEMPTED_BY_NATIVE`.
- **Medición:** `RUSH_MEASURED` registra el desplazamiento XZ medido frente a la suma de velocidades nativas equivalentes, con su ratio. El lab verifica exactamente ×1,5 y que la trayectoria de `speedF` coincide con la nativa.

No se escribe `current.pos`, `home` ni ningún temporizador, y la escritura no precede a ninguna prioridad nativa: `damage_check` solo cambia de modo con impacto TG o *cut-down*, ambos vetados en PRE; `checkFall` exige no tener suelo, y PRE lo exige; WATER_DEATH pone `speedF` a 0.

## Intercepción de saltos (D)

| Ataque de Link | PROC | Aterrizaje | Comprobación cruzada | Resultado |
|---|---|---|---|---|
| Jump Attack con espada | 0x26 | 0x27 (ChkGroundHit) | `checkCutJumpMode()` const == (proc==0x26) | Intercepta |
| Jump Strike | 0x30 | 0x31 | ninguna virtual de carga activa | Intercepta |
| Helm Splitter | 0x2D/0x2E | — | — | Negativo |
| Golpe final / remate | 0x2B/0x2C | — | — | Negativo |
| FINISH_JUMP_UP | 0x23/0x24 | — | — | Negativo |
| Cut Reverse, voltereta atrás, paso lateral, rodar, caer, saltos de locomoción | otros PROC | — | — | Negativo |
| Lobo, montura, demo/evento, timer de daño, sin espada | `l.safe=false` o PROC distinto | — | — | Negativo |

Condiciones para interceptar:

- El ataque va dirigido a **este** Red: `mTargetedActor==this` (comparación de punteros) o, sin objetivo, Link mira hacia el Red dentro de 0x1800 y a menos de 700.
- La trayectoria es válida: dos muestras de PRE en pases de gestión consecutivos; la vertical se integra con la velocidad y la gravedad propias de Link (−3,4) hasta su `mLinkAcch.GetGroundH()`.
- Alcance modelado: XZ ≤ 260 y pies de Link ≤ 110 sobre el Red.
- Se inicia cuando hay al menos un tic activo (t+8..t+12) en alcance antes del aterrizaje y esperar un tic más no aumenta su número.
- Si el aterrizaje llega antes de t+8, se registra `INTERCEPT_MISSED_NOT_ENOUGH_TIME` con el motivo de bloqueo. Nunca se finge éxito.

Más reglas:

- Prioridad 1: puede saltarse el bloqueo nativo posterior al ataque (solo la intercepción), pero nunca un golpe propio en curso (`INTERCEPT_SKIPPED`, sin retiming), un retroceso, daño, correa, agua, caída, WATCH ni orden de reclutamiento.
- Una sola decisión por episodio.
- Si el salto continúa una carga de Jump Strike ya reaccionada, se omite (`SAME_JUMP_STRIKE_EPISODE_ALREADY_REACTED_TO_CHARGE`). Si la intención de esa carga seguía pendiente, se convierte en la intercepción, sin doble reacción.

Tiempos del laboratorio (física portada de Link; no observados en juego):

| Ataque | Inicio forzado | Tics activos | Aterrizaje |
|---|---|---|---|
| Jump Attack | tic de detección | t+8..t+12 | t+14 |
| Jump Strike | tic 10, tras despegue en el 5 | 18..22, todo el descenso, pies ≤110 | 23 |

## Telemetría (E)

- `LINK_REACTIVE_SAMPLE_OBSERVED` solo se emite en transición de (id de Link, presencia, contexto, canario) por actor y generación.
- Eventos separados: `NATIVE_ATTACK_STARTED`, `RATE_APPLIED_READBACK`, `NATIVE_CONTACT_TARGET_IDENTIFIED`, `SHIELD_BLOCK_NATIVE_OBSERVED`, `DAMAGE_CONFIRMED` e `INTERRUPTION_OBSERVED` / `NOT_OBSERVED` / `UNKNOWN`.
  - El contacto se identifica por igualdad de puntero con los tres cilindros TG de Link, y solo después de un registro del propio golpe, porque `ClrAtHitInf` solo limpia los objetos registrados.
  - El escudo se observa como retroceso nativo en estado 3 con tasa −1.
  - `DAMAGE_CONFIRMED` llega reenviado de la confirmación propia de CP14 (una línea de solo lectura en `red_watch`).
  - La interrupción es la entrada de Link en un PROC de daño dentro de 30 tics; si no hay prueba, se registra UNKNOWN.
- Presupuesto: 500 líneas rutinarias (30 por generación) más una reserva crítica de 500 (48 por generación), con contadores siempre activos.
- `SESSION_COUNTERS` y `CAUSE_SUMMARY` por causa, hasta 40 resúmenes. Solo `PERF_WINDOW` usa nivel ERROR.

## Superficie cerrada y coste

- **Escrituras sobre el Red que ejecuta:**
  - `setPlaySpeed` en dos sitios: el golpe propio y la restauración al cerrar.
  - `setActionMode(ATTACK,0)` una vez, con `speedF=0`.
  - `speedF` de la aproximación y su restauración.
  - `SetC`/`SetR` de `mSphs_at[0/1]` durante una pasada.
- Nada se escribe en Link, HP, AtHit, ATP, fotogramas, temporizadores, `home` ni posición.
- **Imports:** CP17 menos `startCreatureSound`, más `cM3dGSph::SetC`/`SetR`. Un solo hook `execute` con PRE y POST. La política no usa matemáticas del CRT (raíz de Newton y atan2 polinómica propias, error ≤2,1e-4 rad), para no añadir imports `api-ms-win-crt-math` no revisados.
- **Memoria:** 64 ranuras × 336 B = 21 504 B estáticos, sin heap por fotograma y sin censos globales.
- **CPU:** coste O(1) por ejecución propia.
- **Preservación:** fuera del bloque Red, `mod.cpp` es idéntico a CP17 (la puerta de fuente lo compara). Los 24 ficheros de runtime son idénticos byte a byte. Blue FIX01, Black, CP14 y la clasificación no cambian.

Límites: la cobertura de contacto, los tiempos y el alcance se validan con puertos y geometría sintética, no con animaciones BCK ni con el motor. La lectura física de Link, el audio, el escudo y la interrupción requieren la prueba humana. La puerta de fuente usa tokens literales del motor oficial.
