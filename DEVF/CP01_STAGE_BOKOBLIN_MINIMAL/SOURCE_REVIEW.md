# Contrato oficial exacto y preset

Raíz40457c6adb381928e4b5fef6ed459ed291edd5e2/Aurora3227d76c60e1e782ca576610bce61c9e7744d8be. SOURCE_MANIFEST fija fuentes y herramientas reusadas. No modificaciones al host/SDK.

- sdk/include/mods/svc/stage.h: Stage1.0 add_actor(ctx,stage,room,layer,record,size,handle); room específico. src/dusk/mods/svc/stage.cpp: register_edit copia bytes a vector propio; filtra mod activo/stage/room/layer y registra MOD_OK. No hay creación síncrona ni garantía render.
- src/d/d_stage.cpp:2611–2635 copia ACTR a append, conserva parámetros/room y resuelve nombre mediante dStage_actorCreate1635–1663/fopAcM_Create. roomReLoader2816–2840 procesa adiciones entre common actors y layerActorLoader. Alias canónico OBJNAME("E_oc",fpcNm_E_OC_e,-1), perfil510. No números escritos en name.
- m_Do_main.cpp:930–934 inicializa mods antes de main01. Loader activa mod_initialize en startup. Si se habilita tarde, no aparece inmediatamente: necesita carga posterior de habitación; no activar por frame ni hot reload.
- stage_remove_mod borra las ediciones atribuidas al mod en detach. loader.cpp llama modules_mod_detached antes de unload_native. El actor nativo vivo no se elimina por quitar la edición; sus métodos/perfil son del host, no del DLL. No se registra callback personalizado ni perfil. Shutdown solo informa una vez; host es autoridad del retiro. No promesa universal de descarga ni reversión de progreso.
- daE_OC_c::create2704–2746: modo0 ordinario; byte1=0 selecciona búsquedaS; bytes2/3=FF evitan switch derrota/ramas de switch. No DEMO3/4 ni modo2 de puente. E_OC normal si byte alto de angle.x=0/FF; E_ocb común cargado por el actor mismo. setIDFFFF sigue el patrón oficial y registros nativos.
- angle.x=0 sería normal con mMoveRange0, sin chequeo de home range en rama2420. Se elige6: mMoveRange600, pequeño positivo (600>200 de executeMoveOut). Es umbral de decisiones nativas, no una barrera física. Yaw/roll0; no HIO modificado ni IA personalizada.

StageService es la mutación AUTORIZADA. Se importa además Host2.2 para versión2.0.3, Log1.0 y Game2 automático del featuregame. No custom profile, ActorService, Hook/Resource/OverlayService, cambios EXE/ROM/saves o registros originales. Ejemplo custom_actor_demo y docs/modding.md reutilizados exclusivamente para patrón de ACTR/add_actor/empaquetado.
