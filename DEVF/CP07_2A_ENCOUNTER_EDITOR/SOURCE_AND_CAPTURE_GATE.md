# Captura focal — procedencia y ABI
Referencia unica Dusklight40457c6adb381928e4b5fef6ed459ed291edd5e2, SDK GameService2.0. No nuevo getter ficticio de Stage/Actor/GameService.
Reusa contrato G2/G4B (DEVF/CP01_G2_READONLY_PACKAGE/src/native_reader.cpp y G4B READ_API_CONTRACTS) sin reauditar KCL/render/ARC. Nueva lectura exclusivamente bajo peticion y dentro de mod_update, no callback de render/UI ni worker. Boton arma solicitud; cerrar/cambiar pestana cancela. No cache previo a pausa que pueda quedar obsoleto; el siguiente update lee estado ACTUAL y asigna su tick. Al menu pausado no se avanza/modifica juego: solo lectura de estructuras registradas. No IO por frame.

| Dato/guard | Fuente exacta |
| --- | --- |
| Stage actual | include/d/d_com_inf_game.h:1567;src/d/d_com_inf_game.cpp:4194,dComIfGp_getStartStageName(), copia acotada7chars |
| Transicion | header1579;cpp4242,dComIfGp_isEnableNextStage() |
| Evento/cutscene | header1625;cpp4426,dComIfGp_event_runCheck(); rechazo si activo, no promesa de detectar todos scripts |
| Link | header1847;cpp5269,dComIfGp_getPlayer(int),no retencion |
| Membresia | fopAcIt_Judge(callback,stackdata),sin registro; limite512, compara puntero antes de leer clase |
| Creacion completa | base_process_class::state.init_state2,!fpcM_IsCreating,actor_conditionINIT,perfilALINK |
| XYZ/yaw | include/f_op/f_op_actor.h,current.pos/current.angle.y,SDK GameABI2 define layout x64; no offsets literales |
| Sala/capa | dStage_roomControl_c::getStayNo(),dComIfG_play_c::getLayerNo(0),valores efectivos0..63/0..15 |
| Room scene estable | getStatusProcID/fopScnM_SearchByID,perfilROOM_SCENE,creacion completa,layer registrado,create/delete_count0,!fpcLy_IsDeletingMesg |
| Layers registrados | fpcLy_RootLayer(),lista acotada256/ciclos,reutiliza contrato G2; no traversal STL privado |
| Recheck final | mismo stage,room,layer,room ID,player pointer; sin transicion/evento,escalares finitos |

Headers SDK oficiales GameABIConfig.cmake aplican TARGET_PC/PARTIAL_DEBUG/DUSK_MOD_FEATURE_GAME y global.h; game_feature.cpp importa servicio game major2. link library SDK original del ZIP privado CP00:9400986bytes SHA2564dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4. Solo se extrae/lee ese miembro; no se genera ni modifica biblioteca, no EXE recompilado. Aurora3227d76c… aporta headers, no build del renderer.
ci/native-read-imports.json fija solo12simbolos decorados ya revisados G4B. La auditoria PE verifica que las imports reales sean subconjunto exacto; linker MSVC debe resolverlas contra biblioteca original. SOURCE_VERIFIED no es GPU/gameplay; resultado de enlace/ABI se registra en evidencia build.

CaptureCheck usa el mismo core de DLL/lab para rechazar contexto cambiado/transicion/evento/no listo/tick0/orden invertido/edad>2. Reader nativo ejecuta guards de memoria/contexto antes de copiar; dobles no demuestran seguridad frente a host corrupto ni vida de punteros en juego. Todos borrows limitados al mismo stack/update, ninguna escritura a actores.
No epoch/ActorId por placement ni tokens derivados de coords. Tick local reinicia con modulo y no es tiempo civil; historial de captura con stage/room/layer se conserva como valores. Cantidad/separacion/respawn son solo diseño; no terreno, triangulos, navegacion o spawning.

Compatibilidad: broker.hpp/sharedABI/probe.hpp y10core/editor/tests copiados sin cambios desde c72ba390…. El servicio nativo sigue1.0/80+24+24/deferred/required. Probe0.1.0 anterior se validara como binario descargado, NO se recompila ni se distribuye reemplazo. Nuevo servicio game2.0 adicional requerido solo por captura. Ui2.3 opcional: si falta, core/schema1/API siguen funcionando. Sin editor no hay archivos de diseño creados ni escritura por frame.
