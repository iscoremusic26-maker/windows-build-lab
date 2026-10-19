# CP19 FIX02 — sonidos nativos universales + chip de escudo del Black en furia (Expansion 0.16.2)

Base: CP19 FIX01 0.16.1, fuente compilada `5c8ce0025a63f7c6a130a407609b2ad353998eb0`, cierre documental `716677f3f73013040611ba19e6d10d7fca4f9fbe` (PR #78 draft). Dusklight `40457c6`. Auditoría previa y evidencia: `SOURCE_AUDIT.md`.

## A. Audio nativo universal

**Causa (0.16.1).** `Z2SceneMgr::loadSeWave(0x5e)` devolvió false 10 veces con el grupo inactivo (estado 0). En la fuente, `JASWaveArc::load(NULL)` solo puede fallar así si `JASHeap::alloc` no encuentra hueco en el heap de audio ARAM del juego: 10 MiB fijos, que llenan los grupos de la propia escena (estáticos, SE de campo `0x4a`, sala `0x60`, BGM). Esta conclusión sale por **eliminación en la fuente**. FIX02 la mide en juego: `AUDIO_NATIVE_REFUSAL_CAUSE_MEASURED` compara el mayor hueco libre de ese heap (`JASKernel::getAramFreeSize`) con el tamaño real del grupo, una vez cargado en nuestro bloque.

**Solución.**
1. **Dependencia por familia, no por sala.** Todo `daE_OC_c` (E_OC y E_OC2, variantes Blue/Red/Black) usa el grupo SE `0x5e`, el que el motor carga donde viven E_OC nativos (`setSceneName`, F_SP121 salas 2/3/4). Se aplica en cualquier escenario, sala y capa en cuanto DEVF tiene encuentros configurados (o se ejecuta un Bokoblin clasificado). No hay lista de salas ni de escenarios.
2. **Memoria propia, fuera del presupuesto de audio del juego.** Se toma una sola vez un bloque de 4 MiB de la cola del heap ARAM gráfico (`JKRAramHeap`, ~14 MiB en PC frente a ~6 MiB usados en GameCube). Solo se reserva si después siguen libres al menos 4 MiB en un bloque. Sobre él se crea un `JASHeap` raíz. Ambos objetos viven en memoria del juego y nunca se liberan.
3. **Carga por el cargador nativo.** `JASWaveArcLoader::sAramHeap` se fija a ese heap solo alrededor de una llamada síncrona a `Z2SceneMgr::loadSeWave(0x5e)` y vuelve a `nullptr` justo después. Si otro mod lo tenía fijado, no se toca. El motor hace todo lo demás: asignación, DVD→ARAM, `onLoadDone`→`incWaveTable`. La reproducción sigue siendo la nativa y posicional, del propio actor; no se dispara ningún sonido.
4. **Residente, nunca liberado.** `decWaveTable` borra los handles que apuntan al grupo borrado, también los de ids que comparten grupos de escena cargados después. Borrarlo silenciaría sonidos de sala, así que el mod no borra nunca, ni al cambiar de sala, ni al morir, ni al apagar o desactivar.
   - Si la escena ya tiene el grupo cargado: `AUDIO_RESIDENT_NOT_OWNED`, sin reclamar nada.
   - Si la escena lo pide estando el nuestro cargado: su `loadSeWave` devuelve false y los sonidos se sirven desde el nuestro.
5. **Puerta FIX01 intacta.** `cp19_policy.hpp` es byte a byte el de FIX01. No hay ninguna llamada de audio antes de `audio_gate()`, y el apagado no hace ninguna llamada de audio.
6. **Telemetría acotada.** Eventos:
   - `AUDIO_DEPENDENCY_IDENTIFIED`
   - `AUDIO_RESERVATION_ACQUIRED` / `AUDIO_RESERVATION_REFUSED`
   - `AUDIO_LOAD_REQUESTED`, `AUDIO_LOAD_ACCEPTED` (solo petición) y `AUDIO_LOAD_READY` (solo con estado 2 del motor y el grupo en nuestro bloque)
   - `AUDIO_NATIVE_REFUSAL_CAUSE_MEASURED`
   - `AUDIO_LOAD_REFUSED` (motivo leído del estado)
   - `AUDIO_LOAD_NOT_READY_YET`, `AUDIO_RESIDENT_LOST_ENGINE_ERASED`, `AUDIO_LOAD_GAVE_UP` (3 intentos espaciados 300 pasadas)
   - `AUDIO_RESIDENT_KEPT_NO_RELEASE`
   - `CP19F2_SUMMARY`

   No existe `AUDIO_RELEASED` porque no hay liberación.

## B. Black en furia: 1/4 de corazón al bloquear con escudo

1. **Atribución nativa.** En el POST del `execute` de un Black clasificado, tras la política CP18, se exige a la vez:
   - una de sus 2 esferas AT tiene `ChkAtHit`, `ChkAtShieldHit` y `GetAtHitObj` igual a un cilindro TG de Link;
   - ese cilindro tiene `ChkTgHit`, `ChkTgShieldHit` y `GetTgHitObj` igual a esa esfera.

   Es el registro del propio `dCcS::SetAtTgGObjInf` de la pasada de colisión anterior. Los punteros solo se comparan. No basta con distancia, animación, retroceso ni `ChkAtShieldHit` solo.
2. **Furia.** Cuenta el valor de furia CP10.6 en el POST donde **empieza** el golpe, el mismo con el que CP18 decide x2:
   - un golpe x2 empezado en furia cuenta aunque la furia acabe a mitad;
   - un golpe x1.15 no cuenta aunque la furia empiece a mitad.
3. **Una vez por golpe.** La clave es (id del Black, generación, secuencia de golpe CP18): las 2 esferas y los bits que persisten varios frames cuentan una sola vez.
   - Una cadena nueva o un Black distinto es otro golpe.
   - Los bits leídos antes de que las esferas del golpe se registren en una pasada se ignoran por obsoletos.
   - Los bits leídos en el POST donde empieza un golpe se juzgan contra el golpe anterior.
   - No hay enfriamiento global.
4. **Exactamente 1 unidad.** `dComIfGp_setItemLifeCount(-1, tipo pendiente sin cambiar)` es la cola de vida pendiente del medidor, fuera de `setDamagePoint`. Por eso:
   - sin multiplicador de daño (x1/x2/x5/x8/x10), sin armadura, sin rupias de la Armadura Mágica;
   - sin `mDamageTimer` ni proc de daño, y sin tocar guardia, colisiones ni invulnerabilidad.

   El medidor suma con los demás cambios del mismo frame y recorta en 0. La muerte es la nativa (`checkDeadHP`, hada incluida). Con vida 0, en evento o sin Link, no se aplica (`SHIELD_CHIP_SKIPPED_*`). Se lee de vuelta el contador pendiente (`SHIELD_CHIP_APPLIED`) y, una pasada después, la vida real (`SHIELD_CHIP_METER_READBACK`).
5. **Destello rojo, solo de render.**
   - Un hook PRE/POST en `daAlink_c::modelDraw` (exportado; `draw()` lo llama 25 veces por el stub ILT, verificado en el EXE). En PRE escribe `tevStr.TevKColor`, sin bajar nunca un valor nativo, solo para los modelos que el destello nativo colorea: cuerpo, cara, sombrero, manos y modelos demo. En POST restaura el valor previo.
   - El calendario es el nativo: 30 tics, `32-(30%16)`, R 15/10/5, `|sin(t*0x800)|`.
   - Funciona también con Armadura Mágica: no depende de `checkMagicArmorNoDamage`.
   - Si el host rechaza el hook, el chip sigue funcionando y lo dice (`SHIELD_CHIP_ARMED_FLASH_HOOK_UNAVAILABLE`).
6. **Sin sonido añadido.** El sonido de bloqueo es el nativo.

## Superficie y conservación

- Ficheros:
  - 27 ficheros runtime idénticos byte a byte a FIX01 (incluido `cp19_policy.hpp`: A–G y la puerta).
  - `cp19f2_policy.hpp` es nuevo.
  - `mod.cpp`: adaptador de audio (+reserva y `load_own`), bloque FIX02 B, una línea en `black_ai_post`, el hook y el armado.
- Escrituras nuevas:
  - el contador pendiente de vida (1 llamada);
  - `TevKColor` de Link, restaurado tras cada modelo;
  - `JASWaveArcLoader::sAramHeap`, puesto y repuesto alrededor de una llamada.
- Escrituras sobre actores: idénticas a FIX01. Escrituras previas sobre Link (`mCutType`): sin cambios.
- Imports: FIX01 más 13 revisados en la librería oficial.
- Hooks: `daE_OC_c::execute` PRE/POST y `daAlink_c::modelDraw` PRE/POST.

## Límites reales

- La identidad del grupo `0x5e` como portador de las muestras E_OC es una inferencia de la fuente (no hay acceso a datos del juego). Si no se oyen sonidos con `AUDIO_LOAD_READY` en el log, el grupo no es el correcto: **STOP_BLOCKED_AUDIO_GROUP** con ese log.
- Los ids compartidos entre `0x5e` y un grupo de escena cargado **antes** que el nuestro se quedan en manos de la escena. Si la escena lo borra y el nuevo grupo no los tiene, esos ids concretos faltarían hasta cambiar a una sala que los traiga. No es una regresión: el juego nativo tampoco los tendría ahí. Las muestras propias de E_OC no se ven afectadas en salas sin E_OC nativo. Prueba humana: cambios de sala.
- El destello y la guardia continua solo se pueden confirmar mirando.
- Si dos Black golpean el mismo cilindro en la misma pasada, el motor guarda un solo `GetTgHitObj`: uno de los dos golpes no se cobra (conservador).
