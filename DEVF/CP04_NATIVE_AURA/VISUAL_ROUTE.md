# Ruta visual concreta y propiedad

Fuente oficial exacta40457c6 y Aurora3227d76; SOURCE_MANIFEST fija hashes. Se reutilizan sin repetir el target execute Windows y su timing/lifetime documentados en CP03. No se instala otro hook.

## Vía de material descartada

E_OC CreateHeap (2670–2695) obtiene modelData de recursos0x3/0x4. Draw158–185 obtiene material nodes del modelData; d_kankyo.cpp4527–4565 vuelve a obtener materiales del mismo objeto. J3DModel.cpp calcMaterial/diff lee esos nodos y encola packets. El modelo tiene packets propios, pero el material/texture/modelData no demuestra propiedad privada. No se ha demostrado que todos los consumos diferidos terminen antes de un POST de draw. No se escriben materiales ni se hace write/restore, clonación pesada o textura global.

## Marcador elegido

El E_OC nativo usa setMidnaBindEffect (include/f_op/f_op_actor_enemy.h), que mantiene recursos comunes0x29D/29E/29F. CP04 usa **solo0x29D**, sin activar WolfLock, modo Midna, sonido, ataques o callbacks del binding. No toca IDs nativos del actor.

Logo carga common.jpc/common-r.jpc (d_s_logo.cpp119–125 y938); d_particle.cpp1200–1214 registra banco0. JPAEmitterManager::createSimpleEmitterID51–76 toma un emisor libre, lo inicializa por separado con el recurso compartido y copia posición; callbacks ambos NULL. JPAEmitter.cpp init reinicia UserWork a0. Color global es un **campo de ese emisor**, no global del motor. JPABaseShape.cpp mezcla mGlobalPrmClr/mGlobalEnvClr por emisor y, en TARGET_PC, copia colores al contexto de batch por partícula. No se modifica J3DModelData/J3DMaterial ni textura/asset.

Backend comprueba pool/control/banco y recurso en la primera ejecución elegible, después de que E_OC haya construido modelos. Rechaza hijo, bloque de textura indirecta, callbacks de luz/model/trace por user-work, grupos especiales, emisión fixed-interval, curvas que alteran rate/lifetime o random fuera de[0,1]. Estos datos son **NOT_VERIFIED offline** porque no se recibió common.jpc; no se inventa un hash o contenido. El guard de aceptación es real, fail-closed y produce CP04_VISUAL_REJECTED, no un supuesto PASS visual. Puede bloquear en una instalación con propiedades no compatibles.

Para recursos aceptados, se configuran únicamente campos privados de emisor: RGB(155,38,230), env(45,8,90), alpha144, escala0.6, rate0.4, lifetime18 y maxFrame0. Sin claves rate/lifetime, fixed-interval ni hijos, el cálculo de JPADynamicsBlock::create emite como máximo una partícula por tick (rate-random<=1), y JPAParticle::init_p/calc_p mantiene vida<=18 ticks. Pool/tabla nativos son finitos. No se crea un emisor por frame: el mismo key se activa/actualiza. Pixel color, textura, intensidad, persistencia visible y coste real requieren DX12 humano; APPLIED lee configuración privada, no certifica render.

## Identidad y retirada

Solo ActorId aceptado por la política exacta CP03 y cuyo POST conserva identidad/recurso puede llegar al visual. Se comprueba mpMorf/getModel/mpModel y user-area del modelo igual al actor (lectura, sin modificarlo). eyePos ya fue calculado por cc_set; se copia, sin retener matrices o punteros. HP150 y field_0x560 permanecen como CP03.

Visual guarda ID/key, nunca punteros persistentes. El emisor propio se marca con UserWork numérico DVF4+ActorId, sin dirección de actor/DLL. No tiene callbacks que interpreten ese campo. Lookup verifica key/nombre/recurso/callbacks y tag antes de refrescar o liberar; un handle reciclado no autoriza tocar emisores originales. Fuente init/UserWork y callbacks NULL fundamentan ese control, además del mock.

Muerte/state3 o nueva generación: stopDraw del emisor propio y cutTable nativo (d_particle.cpp1106–1124); cleanup937–961 limpia key, invalida y quita immortal. JPA pool libera después de sus partículas finitas. No se llama forceDeleteEmitter junto a un registro todavía enlazado. Si el key ya desapareció, solo se limpia estado propio. Un key ajeno se rechaza sin escritura.

Shutdown no accede al motor: deja de refrescar y borra escalares. La tabla nativa level::execute971–1003 expira emisores no activos; un evento puede diferir esa expiración hasta terminar. El evento RELEASED de shutdown informa REFRESH_RELINQUISHED, no destrucción inmediata. Las callbacks del mod se retiran por HookService/loader como CP03; el emisor contiene solo código nativo/NULL. La reversión exigida es cierre completo, retirada y reinicio. No se promete hot-delete de actor o marcador durante una escena pausada.

La instancia comparte solo recursos inmutables. El aislamiento real debe confirmarse con **un E_OC2 original que use el mismo recurso/modelo**; E_OC40 no es control equivalente. Mocks no son render. El baseline humano CP03 permanece intacto y no sustituye prueba CP04. No CP05/CP06.
