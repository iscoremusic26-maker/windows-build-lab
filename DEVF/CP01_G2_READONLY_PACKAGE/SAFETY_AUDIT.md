# Auditoría previa al empaquetado G2

**Decisión de fuente: PASS_STATIC_FOR_PACKAGING_REVIEW**, exclusivamente para el observador de lectura en el host original, con registros sanos y ejecución serial del hilo del juego. No quedan riesgos críticos de código propio sin una condición respaldada por la fuente examinada. La publicación sigue condicionada a compilación/laboratorio/auditoría binaria y verificación del paquete. Ejecución, comportamiento real y ausencia de fallos en Windows/DX12: **NOT_VERIFIED**; no se promete riesgo cero.

Base `2124adddae1e4433f7523e777cf0a773b2119bb2`; oficial `40457c6adb381928e4b5fef6ed459ed291edd5e2`. Se reutilizan MSVC/link, lib exacta,36 comprobaciones y tres guardas de G1. Esta revisión nueva se limita a los préstamos, lifecycle y empaquetado del observador. No reabre el bloqueo de perfiles personalizados, el parche, Dawnlight, recursos o geometría de ubicación.

## Lifecycle y servicios

`loader.cpp:210–294` resuelve imports requeridos y establece `mod_ctx` antes de initialize. Los slots oficiales Host2.2/Log1.0/Game2.0 son exigidos por metadata y comprobados nuevamente por el módulo; solo acepta strings exactos2.0.3/2.0.3.0. Host.version es un literal del host; build_id pertenece al vector persistente de la imagen (`svc/host.cpp:177–206`). Se copia a hex≤32bytes; ausente/excesivo queda NOT_VERIFIED. El string no autentica un EXE: el procedimiento exige el hash original ya acreditado en CP00.

Initialize fallido no necesita cleanup del juego: no adquirió recursos ni registró nada. Initialize repetido conserva el estado activo y devuelve conflicto. Update inactivo o con presupuesto agotado no llama al ABI. State contiene únicamente bool y contadores; Snapshot/ActorIds/pointers solo existen dentro de una llamada y salen como texto copiado. No hay threads, timers, tareas, watches ni callbacks persistentes.

`loader.cpp:296–338` continúa la descarga aunque shutdown falle; **no se utiliza un veto por resultado**. Shutdown solo escribe como máximo una línea y borra State. Si una llamada a LogService falla excepcionalmente, tampoco queda un préstamo o recurso propio pendiente que requiera retener DLL. `natives.cpp:563–584` retira la imagen y la libera después; `loader.cpp:1100/1268` trata ticks siguientes y salida. `native_module.cpp:11–21` utiliza LoadLibraryExW/GetProcAddress/FreeLibrary normales, sin parche DEVF. No se ensaya hot reload ni unload dentro del juego.

La llamada del loader a resolver hooks recorre exclusivamente los registros declarados (`svc/hook.cpp:993`): este DLL no declara ninguno. El bookkeeping normal del loader no registra hooks de este mod. La auditoría PE exige solo headerABI1 y tres imports, sin hooks/exports de servicios. Ningún callback de actores proviene de esta DLL. El contexto pertenece al loader hasta detach; los slots SDK no se reutilizan después de shutdown.

## Validez de las lecturas

| Préstamo / riesgo examinado | Evidencia y precondición efectiva | Decisión |
|---|---|---|
| Hilo y transición concurrente | `m_Do_main.cpp:302/326` llama fapGm_Execute desde el bucle serial. `f_ap_game.cpp:807/824/828` ejecuta ModLoader.tick **antes** de fpcM_Management. Este procesa destroy/priority/create/execute después (`f_pc_manager.cpp:90–113`). No callback, yield o API mutadora del observador puede intercalar destrucción durante la captura. | PASS_STATIC para el flujo original; comportamiento real queda NOT_VERIFIED |
| Nombre de stage/room | Getter público devuelve el array residente del contexto; copia hasta8chars y NUL propio. Room público0..63; omite detalle fuera de F_SP121/R3. enableNextStage=true omite toda captura. | Sin lectura fuera del nombre ni acceso a tablas con índice inválido |
| Registro layer_class | Root público y next del listado registrado; source `f_pc_layer.cpp:95–118/135–145` inserta/remueve antes de limpiar un layer. Tipos públicos y `standard_layout/offsetof(node)==0` justifican conversión del nodo inicial; no offset privado o GameCube. Array local128, detección de ciclos y exceso; current/owner opacos hasta match. | PASS_STATIC; no pretende reparar un host cuyo listado contiene memoria corrupta |
| Link temprano/obsoleto | getPlayer es solo candidato: comparación de identidad en cola sin dereferenciarlo. Match de cola antes de perfil ALINK/init_state2/!IsCreating/INIT y owner registrado; máximo256visitados. Datos copiados después, nunca cacheados. | Omisión conservadora si falta cualquiera |
| Objetos de cola parcialmente construidos | `fpcBs_Create:165–175` establece encabezado/id/perfil antes de common Create. `f_op_actor.cpp:453–454` añade el tag antes de terminar Create, por lo que cola ≠ readiness. Se consulta primero el encabezado base público; campos E_OC solo tras complete e INIT. | Corrección mínima de orden y omisión explícita ante E_OC incompleto |
| Destrucción temprana | `fpc_deletor.cpp:90–100` marca init_state3 al sacar de ejecución. `f_op_actor.cpp:421` quita actor de cola antes de DeleteHeap; `fpc_base.cpp:102–110` libera memoria después del Delete común. Datos de actor parcialmente borrado no se leen como completos. | PASS_STATIC bajo el orden original |
| ROOM_SCENE | getStatusProcID acotado, SearchByID resuelve en lista pública de escenas (`f_op_scene_mng.cpp:13`; scene_iter.cpp:13). Perfil/ID/base completos antes de child layer/counters; padre layer_tag y child layer deben encontrarse en registry. Scene Delete retira tag antes de liberar (`f_op_scene.cpp:32–40`). | RoomId distinto de child layer y owner padre; no se exige igualdad con Link |
| Censo E_OC | Visitor synchronous: actor_iter.cpp:28 → c_list_iter → c_node_iter:28 → c_tag_iter:14. No guarda función/data, no mutación durante recorrido. ≤256visitas y≤8registros; nulos, incompletos, owner desconocido, duplicados, room distinto o truncación no producen muestra aceptada. | Omisión/NOT_VERIFIED; no atribuye ownership DEVF a un match |
| Recursos y layer lógico | getObjectResInfo/getResInfo (`d_resorce.cpp:874`) solo recorre nombre/count; pointer devuelto no se dereferencia. getLayerNo F_SP121 (`d_com_inf_game.cpp:642/880/890`) consulta getters/bits y calcula variables locales; no cambia flags/layers. | Registered no equivale a archivo íntegro/carga completa. Sin setRes/load/sync/model |
| Texto y descarga | LogService copia antes de retornar (`sdk log.h:28`, `svc/log.cpp:9`, `log_buffer.cpp:33/57` asigna string propia). Buffers≤2048,64líneas totales,120updates entre capturas; no lecturas tras límite/shutdown. | La DLL no deja puntero a stack, string o función en el log |

La pertenencia al registro no es una validación general de memoria arbitraria: la validez de los nodos recorridos está respaldada por ownership/listas del host intacto y ejecución serial. Un puntero candidato no encontrado se trata como opaco. Si el host/otros mods corrompen listas o ejecutan mutaciones concurrentes, esa premisa no se mantiene: detener, no probar lectura especulativa, SEH, offsets o hooks. La prueba futura excluye mods que alteren actores/escenas/ABI/hooks. No se ha demostrado esta premisa mediante una ejecución real todavía.

## Correcciones acotadas respecto al observador0.0.1

Copias propias en este directorio, sin modificar CP01_A1_PREFLIGHT:

1. Encabezado público primero; null tags y E_OC incompleto se omiten antes de campos del actor.
2. Censo truncado/owner desconocido devuelve NOT_VERIFIED; validación final rechaza IDs repetidos, room incompatible, contadores pendientes o identidades desconocidas.
3. `room_owner` padre y `room_child_layer` se diferencian y validan; Link/current/room owner **no necesitan ser iguales**. Corrige la ambigüedad del log anterior, sin setter o corrección del estado del motor.
4. Version propia0.0.2/manifest para esta entrega; identidad del mod conservada. Sin nuevos imports o dependencias de juego.

DEVF_READ_ONLY1, constants false, ningún spawn/registro/edit/hook o escritor de archivos. El filesystem de CI y el logger/loader oficiales se distinguen de un escritor en el módulo. El loader normal puede escribir caché/configuración/logs; ejecutar el juego puede guardar/autoguardar por su cuenta. El respaldo previo sigue necesario.

## Puertas previas a publicar

`evidence/pre-package-review.json` fija por hash los archivos propios revisados y fuentes oficiales relevantes. CI exige coincidencia **antes** de configurar. Recompila código propio afectado,47 comprobaciones sintéticas,guardas negativas,imports/exports/metadata exactos. Solo después invoca el target package oficial y verifica ambos DLL y todos los miembros. Solo un resultado final satisfactorio permite subir el artefacto privado. Ver PACKAGE_VERIFICATION para el resultado efectivamente ejecutado.

No se declara PASS de renderer, censo real, préstamos en ejecución, aparición, colisión o cleanup del juego. G2 sigue NOT_VERIFIED; G0PARTIAL, G4BLOCKED, G5/CP02NO-GO. El bloqueo independiente de DLL con perfiles personalizados no cambia.
