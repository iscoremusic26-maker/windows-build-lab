# DEVF — primer servicio nativo proveedor/consumidor

Dos .dusk independientes, NO Bokoblin Expansion terminado:
- Proveedor: dev.devf.cp06_native_config_dual v0.7.1. Conserva editor CP07.1 y placements.json actual.
- Probe: dev.devf.native_api_probe v0.1.0. Import REQUIRED dev.devf.native_placement@1.0, pide una colocación FIELD y no importa StageService.
El proveedor funciona sin el consumidor. Instalar sólo una versión del proveedor; el probe es otro ID.

## Capacidad real y límites
C ABI compartida shared/devf_native_api.h: ServiceHeader1.0, request80bytes, receipt24, service24 en Windows x64; compilada como C11 y C++20. EXPORT_DEFERRED_SERVICE en proveedor, publish_service durante mod_initialize; import requerido en probe garantiza dependencia/orden por SDK. No servicio por lookup dinámico para inicializar al cliente.
El proveedor valida perfiles/escena/XYZ/yaw/params exactos, ID, configuración y conflictos antes de llamar add_actor bajo SU ModContext. Sólo FIELD y NORTH ya probados, E_OC2/510 nativo. Receipt contiene ticket del proveedor, NO ActorId/StageActorHandle ni actor vivo.
Máximo2ediciones totales entre JSON y API. Un placement por consumidor en este corte; IDs y sitios activos únicos, sin personalización HP/color/IA. Se copian records/core/IO/editor/testsCP07 intactos. No archivos de configuración externos se escriben por la API.

## Ventana de admisión
Probe llama sólo en mod_initialize. Proveedor abre admisión tras preparar su watch y publica, y la cierra en su primer mod_update. Loader40457c6 inicializa todos los mods en orden de dependencia antes de tick; proveedores preceden a importers también en la iteración update. Después devuelve MOD_UNAVAILABLE; no registro por frame, callbacks de actores ni hot reload.
Este contrato es una ventana de bootstrap, no introspección del call stack ni certificado global de arranque frío. No detectar ni permitir recarga dinámica; operación/pruebas exclusivamente con juego cerrado y nuevo arranque.

## Ownership y limpieza
Caller pasa su propio ModContext oficial (misma atribución que SDK). Host.mod_id confirma contexto conocido; esto NO autentica contra código nativo hostil que viole el contrato y falsifique contextos. No layouts privados ni lectura LoadedMod/STL.
watch_mod_lifecycle usa ctx proveedor y subject de consumidor sólo para comparación; al detach se retira su edición y se borran owner/token incluso si remove falla. Handle pendiente queda en cuarentena sin owner; nuevas altas rechazadas y proveedor falla controladamente en update para detach. Shutdown inverso sólo de handles propios; core JSON permanece separado.
Probe shutdown consulta Host.get_service antes de llamar remove para no desreferenciar un servicio retirado. Dependencia requerida organiza inicialización; ausente/major incorrecta impide carga del probe según loader. remove_actor_edit NO borra actor vivo; todos los cambios en mods/config se hacen en frío.
No StageActorHandle→ActorId, no cambios al host/SDK/Aurora/Borealis/ROM/saves, no nueva investigación de identidad.

## Uso inicial
Conservar copia de su JSON y usar examples/disabled.json como placements.json para API QA (NONE). Proveedor+probe debe registrar1FIELD; quitar probe y reiniciar con NONE deja0. JSON FIELD/BOTH entra en conflicto y el probe falla limpiamente, sin duplicar. JSON NORTH + probe FIELD permite2ediciones, pendiente de QA si se desea.
El editor sólo muestra/guarda placements del JSON; nunca serializa entradas externas. Si se activa FIELD por editor para próximo inicio mientras existe probeFIELD, al siguiente inicio gana JSON y se rechaza el probe, sin doble alta.
data_dir del mismo ID permanece normal %APPDATA%/TwilitRealm/Dusklight/mod_data/dev.devf.cp06_native_config_dual/; seguir carpeta de datos activa si personalizada/portable/--user-dir.

BUILD/PACKAGE/DOWNLOAD PENDING hasta CI ejecutada. Nuevo par HUMAN_PENDING. Cierre humano0.7.0 está en checkpoint documental separado, sin recompilar paquete histórico. No Expansion, CP07.2, CP08 ni personalización.
