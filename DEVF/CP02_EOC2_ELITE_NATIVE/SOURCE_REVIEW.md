# Preset nativo E_OC2 — verificación dirigida

Raíz fija [40457c6](https://github.com/TwilitRealm/dusklight/tree/40457c6adb381928e4b5fef6ed459ed291edd5e2), Aurora3227d76c60e1e782ca576610bce61c9e7744d8be, SDK/importlib intactos. Actor source [d_a_e_oc.cpp](https://github.com/TwilitRealm/dusklight/blob/40457c6adb381928e4b5fef6ed459ed291edd5e2/src/d/actor/d_a_e_oc.cpp).

| Campo / efecto por source | CP01 normal | CP02 E_OC2 | Líneas comprobadas |
|---|---|---|---|
| Alias de stage | E_oc | E_oc | d_stage.cpp975: perfil E_OC/510, argumento−1 |
| Parameters BE | FFFF0000 | FFFF0100 | create2706–2727 |
| Byte0 modo | 00 ordinario | 00 ordinario |2706–2709 y2810–2830: no DEMO3/4 o modo2 de puente |
| Byte1 búsqueda | S | M |2711–2715: usa l_HIO.plyr_srch_max_radius_S/M; sin distancias runtime supuestas |
| Bytes2/3 switches | FF/FF | FF/FF |2718–2727; searchPlayer2:350–368 guarda byte alto y modo2; se conservan sin escribir flags |
| angle.x creación | 0006 | 0106 (262) |2728–2738: bajo6→600, alto1→E_OC2 |
| Recurso solicitado | E_OC + E_ocb | E_OC2 + E_ocb |2734–2744; no cargador ARC propio |
| Vida inicial |40|220 (5,5× valor inicial)|2771–2779, branch AVOID_UB activo del source; no edición arbitraria/read runtime |
| Parámetro de ataque |SetAtAtp1|SetAtAtp2|2791–2801; no afirmar daño2× o cantidad exacta de golpes |
| Pitch tras interpretación |0|0|2808 limpia current.angle.x/shape_angle.x; 0106 no es inclinación visible |

El branch E_OC también aplica fopAcM_OnStatus(0x10000) en2798; el branch E_OC2 no lo hace allí. Son diferencias nativas, no un perfil/IA exclusiva o un mod de todos los E_oc. No se escribe HIO, HP/colliders o memoria de actores existentes.

El nuevo ACTR32 mantiene position(-1250,-6304.0083,60750), room3/layer6, yaw/roll0 y setIDFFFF del CP01 observado. No se recalcula suelo/KCL/WRock/rutas. La confirmación humana de CP01 se registra como USER_REPORTED, sin log original adjunto aquí ni combate detallado inventado.

Disponibilidad respaldada estáticamente: inventory privado ya verificado contiene ACT6 E_oc en índices7/8/9/12/21 con alto de angle.x=1 (E_OC2); 10/11 con altoFF (E_OC). Selector1 ya se utiliza en esta sala/capa. Esto y resLoad del source respaldan usar recurso nativo, sin cambios de banco/overlay/preload; no prueba el mount/override de una futura sesión CP02. Si faltan recursos en la prueba humana, parar con el error concreto, no instalar cargadores o hooks.

Contratos oficiales reutilizados: stage.h:25–28/36–37 room específico y propiedad del edit; stage.cpp:146–152 filtra stage/room/layer/mod activo;182–195 copia registro;214–218 retira edits al detach. d_stage.h:210–215 define ACTR32; d_stage.cpp:2611–2635 lo pasa al cargador normal,2816–2840 integra adiciones en roomReLoader. MOD_OK solo registro. Shutdown no borra actores vivos, ni se promete hot unload universal.

Source propio derivado de CP01, diff en evidence/production-diff.log y hashes en SOURCE_MANIFEST. Se conservan /Zc:inline- de producción, metadata oficial y tripwire GX de aborto exclusivo del laboratorio ya establecido. No host build, ActorService, profile, hooks ni CPU/GPU patches.
