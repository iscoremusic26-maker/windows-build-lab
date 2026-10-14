# Diagnóstico diferencial CP04.1

## Evidencia primaria y límite

El usuario comunicó el extracto de líneas1797–1804 de dusklight-20261014-103054.log: un HP_APPLIED220→150 paraActorId147, seguido de CP04_VISUAL_REJECTED/key0/PARTICLE_RESOURCE_OR_POOL_GATE_FAILED y desactivación del módulo. Vio E_OC2 sin aura, lo derrotó y no observó crash. El log personal completo y la captura **no fueron leídos por Codex**; aquí se conserva un resumen derivado del extracto confirmado, sin publicar el log personal.

**SOURCE_VERIFIED**: en v0.4.0 Visual::step traduce cualquier create()==0 a la misma razón. Cada salida listada abajo es posible; el extracto no distingue una. No implica un fallo de DX12, de GPU, del banco o de un guard específico. El detach termina telemetría; el actor materializado puede seguir vivo. No extrapolar HP posterior al aviso.

## Recursos disponibles

Se consultó el árbol privado de entradas y los directorios centrales de sus cuatro ZIP: ZIP original Windows, fuente2.0.3 y dos evidenciasARC. No hay archivosJPC en esas entradas ni JPC sueltos encontrados en /workspace. El checkout escaso del runner usa esas mismas entradas y SDK fuente; no lleva ROM o instalación del usuario. **NOT_VERIFIED**: existencia física/propiedades reales de0x029D en common.jpc/common-r.jpc; no se asigna hash o contenido inventado. No se extraen ni publican recursos originales.

## Carga y contratos exactos reutilizados

Root oficial40457c6, Aurora3227d76 y importlib4dc4a511…; SOURCE_MANIFEST fija revisión/hash. d_com_inf_game.cpp904–905 crea el controlador. d_s_logo.cpp733–763 espera sync de mParticleCommand,938 llama createCommon antes de terminar el logo. d_particle.cpp1197 selecciona banco porbit0x8000;0x029D selecciona0. createCommon1200–1214 crea manager con19grupos/2bancos y registra common en0; createRoomScene carga banco1. JPAResourceManager.cpp27–32 busca el índice;67–73 devuelve UserWork de Dynamics. JPAEmitterManager.cpp51–76 falla si no encuentra recurso o no hay emisores libres. level::entry1064–1085 retorna0 si la tabla128 está llena, invalidando el emisor y quitando immortal. Se conserva stopDraw posterior a ese fallo.

La secuencia fuente fundamenta disponibilidad normal del banco común anterior al juego; **no demuestra los punteros de la sesión fallida** ni justifica un retry tardío. CP04.1 no incorpora reintentos. No se repiten host/ABI/HP/terreno.

## Guards y restricciones del piloto

Se mantiene exactamente la aceptación de v0.4.0 y su orden: work&0xFD90, hijo, bloqueETX, fixed interval, keyrate/lifetime, randomrate[0,1], randomlife[0,1]. Los nombres UNSAFE significan incompatibilidad con este piloto conservador; no corrupción demostrada del recurso nativo.

|Guard|Fuente/implicación|Dato efectivo0x029D|
|---|---|---|
|work&0xFD90|d_particle.cpp grupo/callbacks. Excluye0x10,80,100,400,800,1000,2000,4000,8000: indirect/grupos especiales/callbacks de partícula o luz.0x20/40 se omiten al usar factory directa, sin callbacks.|NOT_VERIFIED|
|getCsp()!=null|JPAParticle.cpp245–247 puede crear hijos con parámetros/vida independientes. La presencia no acredita emisión efectiva, pero no hay prueba que permita mantener el bound de vida del piloto con hijos.|NOT_VERIFIED|
|getEts()!=null|JPAExTexShape.h36 distingue isUseIndirect del bloque presente (puede contener textura secundaria); JPAResource.cpp417 agrega ruta del bloque. Guard conservador más amplio que indirect activo: **no eliminar sin propiedades reales y prueba de compatibilidad**. Nuevo log distingue bloque y activo.|NOT_VERIFIED|
|FixedInterval0x2|JPADynamicsBlock::create sustituye acumulador de rate por volumen/divisiones; setRate0.4 no impone el bound.|NOT_VERIFIED|
|KeyID0/4|JPAResource::calcKey1313–1331 escribe rate/lifetime durante cálculo, después de la configuración privada del mod.|NOT_VERIFIED|
|RateRandom/LifeRandom|Bounds fuente CP04: tasa≤0.8/vida≤18 con private rate0.4/life18. NaN/±inf y valores fuera de[0,1] rechazan.|NOT_VERIFIED|

La comparación offline cubre32768 combinaciones incluyendo32bitsWork,16combinacionesflags y8valores de cada random. No simula contenidosJPC ni render. No hay causa causalmente probada para aflojar guard/cambiar ID; se entrega **DIAGNOSTIC_ONLY**.

## Ramas exactas instrumentadas

|Condición realmente observada por create_checked|Razón|
|---|---|
|control==null|PARTICLE_CONTROL_NULL|
|manager==null|EMITTER_MANAGER_NULL|
|pResMgrAry==null|RESOURCE_ARRAY_NULL|
|ridMax<1|RESOURCE_BANK_COUNT_INVALID|
|banco0==null|RESOURCE_BANK0_NULL|
|getResource029D==null|RESOURCE_029D_MISSING|
|getDyn==null|RESOURCE_DYNAMICS_NULL|
|keyNum>0 y ppKey==null|RESOURCE_KEY_TABLE_NULL|
|entrada de key==null|RESOURCE_KEY_ENTRY_NULL|
|primer guard fallido según orden anterior|RESOURCE_USERWORK_FLAGS_UNSAFE / RESOURCE_CHILD_UNSAFE / RESOURCE_INDIRECT_UNSAFE / RESOURCE_FIXED_INTERVAL_UNSAFE / RESOURCE_RATE_OR_LIFETIME_KEY_UNSAFE / RESOURCE_RATE_RANDOM_UNSAFE / RESOURCE_LIFETIME_RANDOM_UNSAFE|
|factory retorna null tras lookup y guards válidos|EMITTER_POOL_CREATE_FAILED|
|entry retorna0 tras emisor creado|NATIVE_LEVEL_ENTRY_FAILED|
|refresh/readback o ownership falla|OWNERSHIP_OR_PRIVATE_CONFIG_READBACK_FAILED (no atribuido a create)|

Se registra la **primera** razón, no todas las condiciones coexistentes. Un rechazo deja fault enclavado, sin más intentos. La instantánea create almacena únicamente escalares, nunca direcciones. El log único de rechazo incluye probe_mask,work,blocked_work,dyn_flags,key_count,child,indirect_block,indirect_active,rate_step y random. Mask:1control válido,2manager,4array,8banco0,16resource,32dynamics y sus datos,64UserWork. Campos cuyo bit no está presente son defaults **NO leídos**, nunca valoresRAM atribuidos. Float no finito se escribe NONFINITE, no número. Refresh fallido no imprime una instantánea create como gate fallido. No hay dump, offset, dirección o log porframe.

## Diff de producto

Carpeta nueva CP04_1_AURA_DIAGNOSTIC. ParticleAccess::create_checked comparte la secuencia de producción con tiposSDK y pruebas con tiposfake; retorno CreateResult tipado en vez deuint32. No cambia recurso, grupos, callbacks, factory, entry, ownership, rate, color, escala, HP ni cleanup. Visual consume key/gate y enclava rechazo; mod.cpp formatea detalle y propaga la razón precisa al aviso de fallo. Mismo IDdev.devf.cp04_native_aura/v0.4.1: loader.cpp156–165/869–873 impide cargar simultáneamente ambos CP04 con mismo ID. HostService no ofrece censo ni regla de conflictos por ID distinto: exclusión de CP03 se exige mediante instalación de un solo módulo y comprobación del log de arranque, sin inventar enforcement automático.

Policy/observer/registration son copias byteidénticas CP04/CP03. PRE/after_execute identidad, HP150 y GameABI no se replantean. Modelos/materiales/recursos/AI/colisión compartidos intactos. Shutdown conserva la política nativa documentada de CP04, sin acceso al motor desde DLL shutdown; cierre completo obligatorio. No hotreload o eliminación dinámica garantizada.
