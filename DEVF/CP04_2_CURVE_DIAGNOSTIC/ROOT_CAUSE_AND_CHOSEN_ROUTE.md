# CP04.2 — Causa confirmada y decisión acotada

## Observación primaria

Resumen explícito suministrado por el usuario de dusklight-20261014-111640.log. El archivo completo no se recibió ni se leyó; no se publica. V0.4.1 única; EDIT_REGISTERED64/ARMED65, HP_APPLIED1803 para ActorId147 (220→150), rechazo1804 y desactivación1805 sin crash observado.

La medición completa probe_mask0x7F acredita: work 0x12, blocked_work 0x10, dyn_flags0x118, dos claves, child0, indirect_block0/active0, rateStep0, fixedInterval0, rate/lifeRandom0 y rate_life_key1. **No acredita IDs concretos, puntos, tangentes o extremos de las curvas.** No se interpreta como fallo del pool o DX12. No hubo creación de emisor.

## Causa y respaldo fuente exacto

CP04/CP04.1 rechazaban `work&0xFD90`, por tanto el bit0x10 es la causa inmediata demostrada. El guard reflejaba una restricción conservadora del piloto, no corrupción del recurso. Con source40457c6, dPa_group_id_change (d_particle.cpp167–221):0x02→grupo1;0x10→grupo9. El bit0x10 de UserWork selecciona la prioridad de dibujo con fog y no implica que el recurso contenga un bloqueETX. El log confirma ETX ausente.

Grupo 9 es menor que gidMax19 creado por createCommon; se calcula en calc3D, grupos0..13 (d_particle.cpp1329–1338). d_particle.h413 identifica drawFogPri2→9; d_com_inf_game.cpp4939–4942 lo despacha; m_Do_graphic.cpp2529–2536 lo dibuja en la rama normal de partículas, después del filtro opaco. dPa_control_c::draw1360–1376 activa niebla para grupos6..11. No se crea un pipeline DX12 ni hook gráfico. Esta fuente acredita el contrato, **no píxeles de la sesión** ni dibujo durante pausa/wipe/configuración que suprima partículas.

Factory51–76 acepta el grupo y callbacks NULL y copia posición;0x12 no contiene100/400/800, que instalarían callbacks de partícula/luz en dPa::set. Usar grupo1 después de retirar el guard10 sería semánticamente incorrecto. La traducción9 es defendible, pero no resuelve las curvas.

JPAResource::calc795–855 aplica calcKey antes de pDyn->create. calcKey1313–1340:ID0 sobrescribe mRate;ID4 sobrescribe mLifeTime(s16). Dynamics183–218 usa mRate y acumulador antes de crear; init_p26 toma la vida de mLifeTime y calc_p213–217 retira partículas por esa vida. Ningún setRate0.4/setLifeTime18 en nuestro POST garantiza los límites después de ese cálculo. Readback o pool finito3000 no certifican coste aceptable; un pool finito tampoco autoriza emisión sin bound.

## Ruta elegida y NO-GO

**Vía A pendiente de certificar las dos curvas reales.** La parte grupo/callbacks está fundamentada; faltan datos insustituibles para el bound. No se aflojan guards ni se fabrica una corrección funcional condicional con límites inventados. No se ejecuta factory. No hay recurso alternativo de Vía B con propiedades medidas: elegir otro ID sería especulativo y no se hace.

Se descartan escrituras restauradoras anteriores a calcKey. Crear partículas manualmente requiere work data del manager perteneciente al emisor/cálculo activo (createParticle usa pWd); no se reconstruye ese contexto fuera de su contrato. No se añaden callbacks DLL para vencer el orden, ni copias privadas de JPAResource con lifetime no acreditado, ni cambios del tick/claves/recursos compartidos.

**Único diagnóstico adicional indispensable:** observar exactamente esas dos claves del recurso029D una sola vez y sin producir efectos. No busca otros IDs o actores, no llama calcKey ni key::calc, no crea emisores ni modifica recursos. El nuevo adaptador solo lee banco0/029D; todos sus punteros son locales y el resultado son escalares. ID/tag/key de emisores no aplican porque no existen emisores propios o ajenos adquiridos.

KFA1: loader91–96; key block constructor datos+12; cabecera ID8/count9/loop11; punto(time,value,incoming tangent,outgoing tangent). JPACalcKeyAnmValue115–136 mantiene endpoint antes/después y usa Hermite con outgoing del punto izquierdo e incoming derecho. Se leen como máximo2×255×4 floats (máximo uint8 del formato: 8.160 bytes de payload), con magic/size/data-pointer/count/finite/order/rango comprobados. Cabecera con size inválido, data nulo, keys cambiadas, flagsUserWork desconocidos o grupo9 fuera del manager produce motivo exacto y abandono; ningún fallback de emisión. Confía en la validez de objetos/bloques cargados por el host como las APIs nativas; no promete recuperación ante corrupción arbitraria de memoria.

Log único por instancia: IDs, count, loop, primer/último tiempo y valor, extremos de valores y envolvente cúbica; además baseRate/baseLife/startFrame/maxFrame, grupos y metadatos. Sin raw floats/tangentes punto a punto, direcciones, offsets, ROM o archivoJPC. `keyN_valid=0` invalida el resumen parcial. Probe_mask marca campos leídos; finiteness flags distinguen defaults de valores reales.

Para intervalo dt, Bézier controla (v0, v0+dt*out0/3, v1-dt*in1/3, v1). Su convex hull captura overshoot que los extremos de puntos no detectan. **EXACT_MATH_BEZIER_HULL_FLOAT32_MARGIN_NOT_CERTIFIED**: es información matemática de diagnóstico, no un certificado de redondeoFloat32 ni una admisión visual. Endpoints y loop también distinguen una RATE no cíclica que termine en0 y apagaría la emisión: un máximo positivo no acredita persistencia. No se cambia el tick del emisor para ocultar esto.

El siguiente análisis mínimo debe aplicar margen conservador a las propiedades medidas, truncadoLife s16, loops/endpoint, emisión inicial y acumulador. Si los límites no sirven, permanecer NO-GO; no repetir búsqueda de IDs a ciegas.

## Diff focalizado y contratos intactos

Carpeta nueva CP04_2_CURVE_DIAGNOSTIC, módulo mismoID/v0.4.2/DIAGNOSTIC ONLY. policy/observer/registration y el cuerpo PRE completo son byteidénticos a CP04.1 y baselineCP03. Solo sustituye la ruta visual por lectura de curvas, logging, metadatos y pruebas propias. POST conserva comprobaciones de actor/modelo y HP; no cambia fields del actor salvo la escrituraHP150 ya autorizada. La propia captura enclava fallo para detener el módulo fuera del hook; `DIAGNOSTIC_CURVES_COMPLETE_NO_EMITTER` significa lectura completa, **no error nativo ni arreglo visual**.

Sin createSimpleEmitterID/levelentry/get/cutTable/onActive, sin callbacks de partículas, sin recursos nuevos, sin acceso a motor desde shutdown. El emisor máximo es0. La instancia nativa materializada y HP pueden persistir tras detach: no continuar combate o afirmar telemetría posterior. Cierre completo para retirada. No CP05/CP06 ni merge.
