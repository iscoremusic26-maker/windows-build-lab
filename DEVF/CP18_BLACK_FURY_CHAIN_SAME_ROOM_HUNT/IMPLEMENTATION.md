# CP18 — Black: furia encadenada y persecución en la misma sala (Expansion 0.15.0)

Base exacta: Expansion 0.14.1 CP17 FIX01 (fuente compilada `77e2da4`, documental `43de14d`, run 39194139875). Fuente oficial: TwilitRealm/dusklight `40457c6`. Solo cambia Black; Red 0.14.1, Blue FIX01, CP14, CP10.x, Provider y editor 0.13.0, JSON y ROM quedan igual.

## Rutas oficiales leídas (no se repitió ingeniería inversa)

| Ruta | Hecho usado |
|---|---|
| `d_a_e_oc.cpp` `execute()` | decrementa `field_0x6c0/6c2/…` y llama a `action()`, `mtx_set()`, `cc_set()` |
| `action()` | `OffAtSetBit` → `damage_check` → actualización de correa `field_0x6e3` (`mMoveRange`, +300, −100) → `searchOtherOc` → agua → `checkFall` → switch de modo → `setFootNoteSound` → `posMoveF` → `CrrPos` → `mpMorf->play` |
| `executeAttack()` estados 1/2 | sonidos `checkFrame` 10/12,5/19 (B) y 7/11,5 (C), bit AT con el fotograma previo en 14..22, traslación por tabla, y **rama final**: `if (isStop) { setBck(0x1c,2,0,1); V_WAIT_ST; MOVE_OUT / estado 0 / FIND 0 }` |
| `executeAttack()` estado 3 | retroceso por escudo (`setPlaySpeed(-1)`), al terminar la misma pareja `setBck(0x1c)`+`V_WAIT_ST` |
| `executeFind()` 0 | `field_0x6c2 = 20`, `setBck(0x1c)`+`V_WAIT_ST`, cae a 3 |
| `executeFind()` 3/4 | 3: arranque de paso/carrera; 4: suelo (`checkBeforeFloorBg(200)` → MOVE_OUT/FIND_STAY), giro, persecución `cLib_chaseF` 20/−3, ataque si 200<d<400 y de frente; con `field_0x6c0==0`: `!searchPlayer2()` → WAIT, si no, correa → MOVE_OUT |
| `setWaitStSound()` (desde `setFootNoteSound` cada tic) | mientras el clip es BCK 0x1C, `checkFrame(0)` vuelve a emitir `V_WAIT_ST` |
| `setActionMode()` | guarda modo anterior, fija modo/estado, pone `field_0x6c0 = field_0x6c2 = 0` |
| `m_Do_ext.h` `isStop()` y `J3DFrameCtrl::update` | modo una vez: al llegar al final `frame = end − 0.001`, `rate = 0`, estado 1 |
| `d_a_alink.cpp` `setRoomInfo()` | la sala de Link es la del polígono de suelo (`GetRoomId`), `fopAcM_SetRoomNo` |
| `f_op_actor_mng.h/.cpp` | `fopAcM_GetRoomNo/SetRoomNo` = `current.roomNo`; distancia al jugador = 3D |
| `d_a_e_oc.cpp` | E_OC nunca cambia su `roomNo`: la sala nativa de cada Black es constante |

`V_WAIT_ST` aparece en 9 sitios del fuente: final del golpe (2), FIND (4), WATCH (2) y `setWaitStSound` (1, solo con BCK 0x1C). La puerta de fuente lo comprueba.

## Solución de la pausa BCK 0x1C / WAIT_ST (no es un cambio de `field_0x6c2`)

El fin de un golpe B/C se detecta cuando `play()` deja el clip parado (`isStop`). La rama final de `executeAttack` corre en el **siguiente** `execute`. El PRE de ese `execute` (el mismo hook de siempre, antes del código nativo) ve `ATTACK`, estado 1/2/3 e `isStop`, y durante la furia CP10.6 (>0) elige:

- **En alcance** (d<400, |ángulo|<0x1000, |Δy|≤200, sin pared y con línea de visión `fopAcM_otherBgCheck`): `setActionMode(ATTACK,0)`. El nativo ejecuta el estado 0: elige B o C al azar (`cM_rndF`), `setBck(5|6,…,1.0)` y su voz `V_ATTACK_B/C`. La rama final (BCK 0x1C + `V_WAIT_ST` + FIND 0 + lockout 20) **no se ejecuta**; nunca se reproduce para luego silenciarse.
- **Fuera de alcance / sin orientación**: `setActionMode(FIND,3)`: arranque nativo de aproximación (voz de paso o carrera), estado 4 sin lockout; el ataque nativo salta en cuanto 200<d<400 y de frente.
- Con furia, un `FIND 0` nativo (tras daño, caída o borde) se convierte en `FIND 3`, y un lockout ya corriendo en FIND 4 se pone a 0 solo para ese Black.

El golpe anterior está completo cuando esto ocurre: todos sus SFX (≤19), su ventana AT 14..22 y su traslación (la tabla satura en el fotograma 18) ya pasaron. El retroceso de escudo (estado 3) se reproduce entero; solo al terminar se encadena. La espera vuelve a aparecer: tras expirar la furia, sin furia (x1,15 con pausas nativas), en FIND_STAY o en MOVE_OUT sin Hunt.

El POST confirma cada cadena: modo ATTACK, estado 1/2, clip 5/6 en el fotograma 1 y clip distinto de 0x1C → `BLACK_CHAIN_NEXT_ATTACK_STARTED` + `BLACK_NATIVE_WAIT_TRANSITION_AVOIDED`. Esto es ruta de fuente + lectura del clip, **no audio observado**. Si daño, agua, caída o evento se quedan el `execute`, se registra `BLACK_CHAIN_DEFERRED_NATIVE_PRIORITY` (el agua y la caída se deciden después del PRE y ganan siempre).

## Velocidad x1,15 / x2 y contacto

Un golpe B/C fresco (PRE en ATTACK 0, POST en el fotograma 1, velocidad 1,0, modo una vez) se decide una sola vez con la furia CP10.6 de ese POST: sin furia x1,15 inmediato; con furia x2 decidido en el fotograma 1 y aplicado en el 2 (fase par: 5 registros AT, pre-play 14/16/18/20/22). Una furia que empieza, termina o se renueva a mitad de golpe no lo cambia. Propiedad condicional: tasa ajena, clip distinto, retroceso o fin natural (rate 0) liberan sin sobrescribir. Para cada golpe x2 propio se reutiliza **sin modificar** el barrido de CP17 FIX01 (`red_ai_policy.hpp` byte a byte idéntico): las dos esferas AT nativas cubren también la pose saltada, nunca la pose 24. Sin compensación de SFX. ATP 3/6, HP del JSON, 4 HP/s, aura y ojos siguen siendo CP10.6/CP12.

## Hunt en la misma sala

- **Activación**: una línea en `dynamic_post` reenvía solo `FuryStart/FuryRefresh` de CP10.6 (PRE con HP tras las escrituras iniciales, POST antes de regeneración, pérdida real no letal). Es el detector existente: prueba pérdida de HP, **no** quién o qué la causó. Arma una vez por generación si Link (sala válida 0..63) está en la sala nativa del Black y el mismo escenario; si no, `BLACK_HUNT_NOT_ARMED`.
- **Mientras dura** (Link en la misma sala): FIND 4 mantiene `field_0x6c0` en 2 (no corre la comprobación nativa `searchPlayer2`/correa); al final de un golpe con la correa activa se usa FIND 0 (pausa nativa) en vez de MOVE_OUT; WAIT/WALK/MOVE_OUT/WATCH/SOUND_WATCH vuelven a la entrada nativa de FIND (2 sin furia, 0 desde MOVE_OUT, 3 con furia). Si el nativo abandona la persecución 3 veces en 45 tics (borde, correa), se deja el nativo 90 tics y se registra `BLACK_HUNT_PURSUIT_BLOCKED_BY_GEOMETRY`. FIND_STAY (comprobación de suelo nativa) nunca se anula. No se escriben `home`, `mMoveRange`, `field_0x6e3`, posición ni colisiones; no hay teletransporte ni pathfinding. La velocidad de persecución es la nativa (20); `BLACK_HUNT_PURSUIT_MEASURED` mide desplazamiento real frente a la suma de `speedF`.
- **Reinicio**: un observador de sala de Link (una lectura por pasada de `mod_update`, además del PRE) incrementa una época con cada cambio de (sala válida, escenario). Hunt guarda la época al armarse: A→B o A→B→A, aunque el Black no se ejecute, lo reinicia (`poll` en la pasada de gestión o el siguiente PRE). Sala −1 o Link ausente es transitorio (`BLACK_HUNT_HELD_LINK_ROOM_TRANSIENT`): sin reinicio y sin escrituras. Muerte, ahogamiento, caída mortal, liberación del ledger o nueva generación lo borran. Al volver a A hace falta daño real nuevo. La furia no se toca: si quedan tics, siguen su curso CP10.6.
- **Blue Recruiter**: un Black con Hunt aborta cualquier orden de reclutamiento activa (`BLACK_HUNT_LOCKED_SAME_ROOM_PRIORITY_OVER_RECRUIT_ORDER`) antes de que `blue_ai_pre` la consuma; el arrendamiento SOUND_WATCH se libera con la ruta CP16 existente. Antes del primer daño Black sigue siendo convocable.

## Escrituras del adaptador (puerta de fuente cerrada)

Todas en el Black que se ejecuta: `setActionMode` (una llamada), `speedF=0` con la entrada a ATTACK 0, `field_0x6c0=2` (FIND 4 con Hunt), `field_0x6c2=0` (FIND 4 con furia), `setPlaySpeed` (aplicar/restaurar) y `mSphs_at[0/1].SetC/SetR` en una pasada de colisión de un golpe x2 propio. Nada en Link, HP, ATP, AtHit, fotogramas, `home`, `mMoveRange`, `field_0x6e3`, posición ni otros actores. Ningún import nuevo: el conjunto revisado es idéntico al de CP17 FIX01.

## Integración

Mismo hook único `daE_OC_c::execute` PRE/POST. Fuera del bloque Black solo cambian: el `#include`, una declaración adelantada, el reenvío CP10.6, el aborto de reclutamiento, las llamadas `black_ai_pre`/`black_ai_post`, `black_ai_poll` en `mod_update`, `black_ai_shutdown` y versión/cabecera. La puerta de fuente elimina exactamente esas líneas y exige igualdad byte a byte con CP17 FIX01; 25 ficheros runtime idénticos.

## Telemetría

`DEVF_CP18_BLACK_AI` con actor/generación, sala, escenario, modo/estado, clip/fotograma/tasa, lockout, temporizador, correa, furia restante, Hunt y época, distancia a Link, causa. Eventos: `BLACK_DAMAGE_RECOGNIZED_EXISTING_CP10_6`, `BLACK_HUNT_LOCKED_FIRST_DAMAGE`, `BLACK_HUNT_NOT_ARMED`, `BLACK_HUNT_RESET_PLAYER_ROOM_CHANGED|ACTOR_TERMINAL|GENERATION|ACTOR_CONTEXT`, `BLACK_HUNT_HELD_LINK_ROOM_TRANSIENT`, `BLACK_HUNT_NATIVE_ABANDON_PREVENTED`, `BLACK_HUNT_PURSUIT_STARTED|MEASURED|BLOCKED_BY_GEOMETRY`, `BLACK_NATIVE_ATTACK_STARTED|ENDED`, `BLACK_RATE_APPLIED_READBACK|SKIPPED|READBACK_FAILED|OWNERSHIP_RELEASED`, `BLACK_CHAIN_NEXT_ATTACK_STARTED|DEFERRED_NATIVE_PRIORITY`, `BLACK_NATIVE_WAIT_TRANSITION_AVOIDED`, `BLACK_FURY_LOCKOUT_CLEARED`, `BLACK_NATIVE_CONTACT_LINK|OTHER`, `BLACK_NATIVE_SHIELD_BLOCK`, `BLACK_SUMMARY`. Los eventos de furia siguen siendo los CP10.6 (`BLACK_FURY_START/REFRESH/END`). Presupuesto: 600 normales + 800 críticos por sesión, 40 + 120 por generación, detalle de cadena 6 golpes por episodio de furia, 40 resúmenes; nada por tic. Solo `PERF_WINDOW` usa nivel ERROR.

## Límites conocidos

- Laboratorio sobre un puerto de fuente del E_OC: no anima BCK reales, no reproduce audio y no colisiona. Contacto, escudo, cadena visual/sonora y persecución real requieren prueba humana.
- La persecución usa la velocidad nativa; si Link corre más rápido, Black lo alcanzará cuando se detenga. No hay pathfinding: con un obstáculo intransitable Black se queda contra él (registrado) o en FIND_STAY.
- Con la correa activa, el MOVE_OUT que el nativo fija al terminar un daño queda en el modo un tic, pero el siguiente PRE lo redirige a FIND antes de que `executeMoveOut` llegue a ejecutarse.
- El detector de daño CP10.6 no distingue la causa de la pérdida de HP (espada, flecha, bomba, …).
