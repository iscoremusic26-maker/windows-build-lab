# CP06 — Gate focal SDK40457c6

HostService2.2 sdk/include/mods/svc/host.h:data_dir(ctx,const char**), persistente; mod_dir se borra al arrancar y no sirve para persistir.
src/dusk/mods/svc/host.cpp77–109 construye absolute(ConfigPath/mod_data/mod_id), crea directorio y conserva UTF-8 hasta shutdown. FileService1.0 file.h exige locations opacos: el mod pasa data_dir a join y no concatena paths.
src/dusk/mods/svc/file.cpp524–542 delega join;276–361 open/size/read/close asocian handles al mod. Encounter/borealis@4ac5e7052a8c49a122f8d57f626b5c75c5ca6968 src/io.cpp476–500 acepta carpeta filesystem Windows sin scheme, une relative seguro y requiere archivo existente. Así queda fundamentada data_dir→join en Windows, no por suposición. write_all explícitamente no atómico; no se usa ni se promete guardado atómico del mod.

StageService1.0 stage.h:add_actor y remove_actor_edit. stage.cpp139–153 consume Add por stage/room/layer y crea cada carga;189–218 almacena/copia y elimina por propietario/detach. API270–288 devuelve edit handle, NO ActorId. d_stage.cpp2610–2634 copia record a append y1635–1662 llama fopAcM_Create descartando ID; no hay devolución por placement/callback SDK. No se introduce token en setID: participa en flags/saves nativos (2047/2101), no es campo libre.
Se conserva el gate de identidad CP05 para1record. Multiinstancia general/epoch de escena requiere nueva correlación precisa, no sólo coordenadas. No se registra perfil ni se usa ActorService.create_actor para sortearlo.

Sitio1: DEVF/CP01_STAGE_BOKOBLIN_MINIMAL/LOCATION.md conserva la consulta y procedencia; CP03–05 ya observado por humano. Sitio anterior(-4806...) en CP01_G4_EOC_RESOURCE_CONTINUATION/POSITION_REASSESSMENT.md seguía NO_SAFE_POSITION_VERIFIED; no se recicla como sitio2 aprobado. No nuevo KCL/ARC/J3D/32fuentes investigado.
Gate: GO limitado para catálogo externo/presets/1sitio, CP06_PARTIAL. Salida A(moverXYZ),B(2activos/2sitios/epochgeneral) pendientes; no MVP completo. C enable/remove por próximo inicio está implementado, runtime CP06 aún no ejecutado. MSVC/ABI/IO pendientes de pipeline, no se hereda PASS CP05.

Fuentes fijadas: https://github.com/TwilitRealm/dusklight/tree/40457c6adb381928e4b5fef6ed459ed291edd5e2 y https://github.com/encounter/borealis/tree/4ac5e7052a8c49a122f8d57f626b5c75c5ca6968 .
