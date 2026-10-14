# CP04.3 — Decisión técnica y contratos
## Evidencia primaria de CP04.2
Resumen explícito recibido en el prompt del usuario, NO lectura del archivo privado dusklight-20261014-121929.log:
actor147, HP220→150, backendD3D12, mod0.4.2 único. work12/grupo9/gidMax19, dynFlags118, dos keys, sin child/ETX/fixedInterval, rateRandom/lifeRandom0, baseRate0/baseLife1/start0/max0.
Key0 ID0 RATE, tres puntos, no loop, primer(0,18), último(5,0), values/hull0..18.
Key1 **ID1 VOLUME_SIZE**, tres puntos, no loop, primer(0,50), último(4,100), values/hull50..200. **NO LIFETIME (ID4)**.
Sin emisores ni píxeles, rechazo intencional, retirada/reinicio sin extra confirmados por usuario. Esta evidencia invalida029D como primera aura sostenida; no declara recurso corrupto. No quitar bit10 ni convertir grupo9→1; setRate antes de calcKey no vence RATE animada.

## Dos alternativas, máximo
| Ruta | Evidencia exacta | Dictamen |
|---|---|---|
| A:84ED/ID_ZI_S_NIOITAG_A | kytag03 crea una vez, actualiza mismo emisor; tintes privados incluso ramaB2 púrpura; API directa permite emisor propio sin callbacks DLL | Seleccionada para implementación funcional condicionada |
| B: materiales/overlay3D privados | Reutilizar CP04: J3DModelData/nodos/materiales compartidos; paquetes por instancia no prueban clonación/draw lifetime; SDK no demuestra vía privada suficiente | BLOCKED; sin implementación ni nueva auditoría |

Fuente exacta y enlaces:
- [kytag03 create423–451](https://github.com/TwilitRealm/dusklight/blob/40457c6adb381928e4b5fef6ed459ed291edd5e2/src/d/actor/d_a_kytag03.cpp#L423): create único dComIfGp_particle_set(84ED). [execute311–362](https://github.com/TwilitRealm/dusklight/blob/40457c6adb381928e4b5fef6ed459ed291edd5e2/src/d/actor/d_a_kytag03.cpp#L311) mantiene emisor, rate0.5, positionY+30, escala(3,4,3), color privado. RamaB2 PrmFF/C8/FF Env44/1A/6B. Alpha=senses_strength*223+32, sin stopDraw por senses en ese bloque. Delete405–411 invalida/quita immortal/clear callback.
- [enum4ED](https://github.com/TwilitRealm/dusklight/blob/40457c6adb381928e4b5fef6ed459ed291edd5e2/include/d/d_particle_name.h#L1281): ID_ZI_S_NIOITAG_A. Bit8000 selecciona banco1 en d_particle.cpp:1196.
- [grupo167–221](https://github.com/TwilitRealm/dusklight/blob/40457c6adb381928e4b5fef6ed459ed291edd5e2/src/d/d_particle.cpp#L167):1/2/4/8→0/1/2/3; bit10→8/9/10/11. No elegir grupo antes de leer work real.
- [JPAResource calc781–858](https://github.com/TwilitRealm/dusklight/blob/40457c6adb381928e4b5fef6ed459ed291edd5e2/libs/JSystem/src/JParticle/JPAResource.cpp#L781): calcKey antes de create y luego calc_p.
- [JPADynamicsBlock create177–215](https://github.com/TwilitRealm/dusklight/blob/40457c6adb381928e4b5fef6ed459ed291edd5e2/libs/JSystem/src/JParticle/JPADynamicsBlock.cpp#L177), [JPAParticle init18–26/calc213–245](https://github.com/TwilitRealm/dusklight/blob/40457c6adb381928e4b5fef6ed459ed291edd5e2/libs/JSystem/src/JParticle/JPAParticle.cpp#L18), [termination103–128](https://github.com/TwilitRealm/dusklight/blob/40457c6adb381928e4b5fef6ed459ed291edd5e2/libs/JSystem/src/JParticle/JPAEmitter.cpp#L103).

Estas lecturas se completaron en checkout exacto antes del fallo Cloud. No se recibió JPC físico; NO se escaneó banco ni probaron IDs. Referencia de uso persistente no prueba que84ED esté cargado en F_SP121 ni sus metadatos concretos. El preflight propuesto puede RECHAZAR candidato; no fallbacks/loops/diagnóstico presentado como arreglo.

## Preflight y configuración propuesta
Una sola búsqueda de84ED en banco1 por nueva generación autorizada antes del factory. Banco/count/resourceID/dyn/data/baseShape/data deben existir. Validar grupo traducido en gidMax. Work conocido00EF007F: bits de grupo y color20/40/EF0000 permitidos con tinte privado explícito; callbacks100/400/800 y especiales80/1000/2000/4000/8000/unknown rechazados. La factory directa no aplica helpers ambientales: ningún cambio al recurso, no suplantación de callback que el diseño necesite.

Child y TODO ETX rechazados, FixedInterval rechazado, startFrame0, funciónvolumen válida y tipo<=6. RateRandom/lifeRandom finitos[0,1]. **keyNum debe ser0**: rechazo por count antes de ppKey; ninguna key RATE/LIFE/VOLUME o corrupta/extrema/looped puede alcanzar emisión, no mutar keys ni resetear ticks. Si84ED tiene keys, fail-closed y obtener solo causa/metadatos seleccionados; no declarar APPLIED.

Máximo4 field entries válidos. JPAFieldBlock no produce nacimientos ni modifica vidas; no se evalúan partículas globales. Shape billboard2/9/10, parent visible, tamaño positivo<=128. Extra escala permitida solo si random0..1, tiempos0..1 ordenados, endpoints/cached piecewise slopes finitos/acotados (abs<=64) y endpoints0..2; tipo0..2 y ciclos positivos cuando usados. Margen FP32 de0.001 para factor<=2.001. Hasta8 dependencias de textura de ESTE recurso; TDB1, índice<texRegNum, texture/data válidos. Sin censo del banco ni volcados.

Config propia: rate0.4f, rateStep0, life18, maxFrame0 mediante becomeContinuousParticle, drawTimes1; globalScale0.6, localTranslation0/localScale1, volume20/minRadius0, Prm155/38/230 alpha144, Env45/8/90. Nada en sharedresource/model/actor aparte HP heredado. No senses/Link/smellflags/atención/audio.

## Cota independiente del tamaño del pool
En calc nativo: newCount=0.4f*(1+rateRandom*r_zp), r_zp[-1,1]⇒0..0.800001. Residual<1⇒suma<1.800001⇒truncado≤1 nacimiento por calc; FirstEmit también≤1. Sin keys/child/callback/FixedInterval que invaliden esa cota.
Vida=(1-lifeRandom*r_f)*18 con r_f[0,1]⇒0..18; edad inicia−1 y aumenta en el mismo calc, retires antes de dividir. Create ANTES de calc_p⇒≤18 tras calc, pico transitorio≤19. Guardruntime20; una sola emisión propia, fieldmax4 y drawTimes1. Pause no crea backlog. Extra escala inicialmScaleOut1 (sin key10) y randomfactor≤2.000001; tamaño final máximo aproximado308 con margen. Esto NO certifica FPS/overdraw/visibilidad ni GPU.
Fórmulas revisadas estáticamente; modelo adverso FP32 fue escrito para laboratorio, **NO EJECUTADO** por fallo externo. El pool nativo finito3000 NO constituye demostración de coste.

## Vida y propiedad
Solo IDs/key/stamp numérico prefix44564643+ActorId/grupo/escalars persistentes. No actor/emitter/resource pointers fuera del frame. Native level.entry vuelve immortal; refresh onActive/playCalc/playCreate según contrato nativo set(key), sin ticks reset.
Source removeScene primero level.cleanup, luego removeRoomScene clearResourceManager(1) y sceneheapfree. forceDeleteEmitter marca0x300 pero deja pRes antiguo en pool residente. **Comprobar key/name/nullcallbacks/status0x300/stamp/bank/group ANTES de pRes**; un handle ajeno/reciclado jamás se modifica ni corta.
Muerte propia stopDraw/cutTable invalida/quita immortal. Restos residentes hasta18 cálculos nativos, no dibujan. Ausencia de execute puede causar expiry tras level; eventos pueden diferir esa limpieza. Sin retry del handle ni revivir generación mismaID. Identidad monotónica CP03 intacta.
Shutdown no dereferencia engine: solo deja de refrescar, host retira hook/edit; fullquit requerido. No garantiza hot unload manual ni borrado inmediato del actor vivo. Pixel/cleanup/tiempos runtime siguen NOT_TESTED.

## Admisión y bloqueo
Ruta A viable como TEST **condicionado**, todavía no admisión offline. El entorno murió durante preparación del laboratorio, antes de build/push. SDK/MSVC/PE/paquete/descarga no se han ejecutado para CP04.3. Estado CP04_3_NO_GO_BLOCKED por infrastructure, no refutación del recurso. No .dusk ni TEST activo emitido.
