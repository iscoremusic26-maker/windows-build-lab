# Revisión incremental — observación de health

Fuente oficial exacta `40457c6adb381928e4b5fef6ed459ed291edd5e2`, Aurora `3227d76c60e1e782ca576610bce61c9e7744d8be`. SOURCE_MANIFEST fija archivos/hashes. La prueba estática previa del target execute, PMF8 y create→execute→action se reutiliza desde [checkpoint v0.3.0](../CP03_STAGE_HOOK_HP150/evidence/host-target.json): mismo EXE SHA256 `ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049`, sin nuevo target ni nueva inspección binaria. add_post comparte target, metadata y g_orig con add_pre.

## Vida del actor dentro de PRE/original/POST

SDK hook.hpp 61–99 llama PRE, original y POST en el mismo stack. HookService hook.h 53–57 fija hilo del juego; hook.cpp 630–652 despacha POST sin descargar el actor.

E_OC execute 2631 procesa action; action 2418 llama damage_check. Las tres solicitudes de eliminación en la lógica E_OC (1873, 1951, 2206) usan fopAcM_delete. f_op_actor_mng.cpp 240–243 llama fpcM_Delete; manager.cpp 44–45 llama fpcDt_Delete. Deletor.cpp 63–133 solicita cola, retira de ejecución y marca init_state=3; no libera memoria en esa llamada. Handler 36–43 procesa la cola y deleteMethod 19–33 llama fpcBs_Delete. Base.cpp 102–110 destruye y libera. Manager.cpp 85–110 llama handler antes de creator y executor; E_OC no llama handler ni libera this directamente.

Así, un actor que solicita eliminación en execute sigue asignado hasta el POST inmediato; estado3 significa eliminación en cola. POST puede leer su HP final sin guardar el puntero. No se lee después de que el handler destruya la instancia. Esta garantía es de la fuente exacta y del protocolo sin mods conflictivos, no una prueba runtime universal.

## Identidad, pares y fallos

policy.hpp se copia byte a byte de v0.3.0: identidad exacta, creación, HP220, ID/generación y ONE-SHOT intactos. src/mod.cpp conserva una sola escritura health=150. La observación comienza tras esa escritura. PRE revalida atributos del registro y capa/stage; solo captura ID aceptado y recurso E_OC2. POST usa el ID del par y revalida registro/recurso/fase antes de leer health.

Observer solo guarda escalares (ID, HP, contadores), ocho frames fijos, sin actor/collider/callback persistente, heap o censo. Llamadas ignoradas tienen sentinelas. Reentrada del propio actor, profundidad excesiva, POST con identidad inválida o par ausente rechazan/desarman, sin escribir HP. Recarga normal reinicia contadores con ID nuevo y emite un resumen del anterior. Shutdown limpia observación y deja retirada de callbacks/edit al host oficial.

Registro: PRE y POST deben instalarse antes del Stage edit. Falta de add_post, error o excepción después de PRE retira el target por uninstall oficial y retorna error; no crea silenciosamente un E_OC2 sin diagnóstico. El detach del loader sigue siendo autoridad tras fallo parcial. No se altera retorno original, argumentos ni skip/replace. Ciclo de vida ya revisado de HookService se reutiliza, sin nueva auditoría ActorService.

## Qué mide y qué no

execute procesa damage_check (638–785), que incluye CutDown, head jump/bomba a cero, sumas de HP para Iron Ball/AT_TYPE_40 y cc_at_check. d_cc_uty.cpp 416–449 multiplica potencia según espada/potenciadores y trunca a cero. PRE/POST mide el **cambio neto de health en la ventana**, no una lectura de mAttackPower ni certeza de un impacto individual. Varias operaciones pueden compensarse dentro de una llamada. 20→0 acredita pérdida efectiva20; no daño bruto20. UNKNOWN_OR_SPECIAL no afirma que ocurrió un movimiento especial.

Cambios desde el último POST hasta el PRE siguiente se etiquetan OBSERVED_BETWEEN_EXECUTES; no prometen captura exacta del golpe mortal fuera de execute. Si el actor deja de ejecutarse sin pasar por nuestra ventana no existe lectura posterior: no se busca un actor eliminado para fabricar resumen final. Resumen conserva la última lectura conocida.

Incrementos son HP_INCREASE_OBSERVED sin atribución ni corrección. Sin cambios no hay log. Límite32 incluye aumentos/pérdidas y ventanas entre executes; después se actualizan solo escalares, con un aviso de supresión y resumen. No se instrumenta control original: evita ampliar alcance. Comparar CP02 E_OC2 en sesión separada; un E_OC40 no es equivalente a E_OC2.

Laboratorio nuevo prueba Observer/Registration reales con snapshots y mock de servicios, no la DLL, backend o actor real. Instalación del POST, daño, coste real y logs del juego siguen pendientes de Windows/DX12.
