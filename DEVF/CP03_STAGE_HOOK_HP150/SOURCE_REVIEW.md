# Revisión focalizada del piloto

Fuente oficial `40457c6adb381928e4b5fef6ed459ed291edd5e2`, Aurora `3227d76c60e1e782ca576610bce61c9e7744d8be`, importlib SHA256 4dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4. SOURCE_MANIFEST fija los archivos relevantes y herramientas reutilizadas. Ninguno se modifica.

## Target y momento efectivos

En src/d/actor/d_a_e_oc.cpp 2771–2779 native create establece field_0x560/health 40 o 220;2808 borra current/shape.angle.x;2829 llama daE_OC_Execute, cuya función2642–2644 llama al miembro execute;2631 ejecutaaction, que2418 llama damage_check. El pre-hook sobreexecute se sitúa después de fijar 220 y antes de esas acciones. Home.angle no se borra en esa secuencia.

La comprobación estática del **Windows EXE oficial**, SHA256 ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049, confirma la llamada crítica create→wrapper→execute y execute→action, sin flags de inlining para esos targets en elmanifiesto incrustado. [evidence/host-target.json](evidence/host-target.json) conserva nombres, RVAs derivados y cadenas de llamada, sin bytes originales/disassembly. ci/verify_host_target.py permite repetir únicamente esa inspección de lectura; no ejecuta el EXE. No se usó la compilación Linux ni se compiló el host.

DEFINE_HOOK(&daE_OC_c::execute,NativeEocExecute) y mods::hook::add_pre usan el generador oficial de SDK. La firma int(daE_OC_c::*)() y health s16 se verifican por compiler; se exige PMF compacto de 8 bytes enWindows x64. La auditoría valida metadata Hook MEM, nombres de clase/target, PMF→thunk de import exacto y servicios Host2.2/Log1.0/Stage1.0/Hook1.1/Game2.0. El import execute es una referencia detarget; el mod no llama manualmente aexecute/create ni instala bytepatches. HookService controla eloriginal.

## Identidad y ámbito de confianza

Lecturas tipadas, ninguna dirección/offset deGameCube: profname 510, room 3, parameters FFFF0100, setID FFFF, home.pos **igualdad float32 exacta** con el registro, home.angle (262,0,0), stage F_SP121 ylayer efectivo6 por getter nativo. Después se exige mName E_OC2, health 220, current.angle.x 0, IsCreating(ID) e init_state 0/1. Dentro de create esas señales identifican el pre-first-execute; health 220 por sí solo no lo hace.

El inventario ya decodificado contiene 40 registros E_oc originales entre elstage/sala/capas disponibles y cero coincidencias dehome pos; ACT6 contiene 7. [identity-uniqueness.json](evidence/identity-uniqueness.json) registra el hash del inventario y el resultado sin copiar records/ARC ni reanalizar geometría. StageService añade una vez el único registro propio. **Es un fingerprint de instancia, no una API de ownership de StageService.** La prueba requiere únicamente CP03, sin otros mods que añadan/editen enemigos o hooks deE_OC. No se promete aislamiento ante un tercero que copie exactamente el registro o altere parámetros originales.

Solo se consulta porSDK el ID previamente aceptado para rechazar un segundo candidato concurrente aún vivo; no se busca por perfil/proximidad ni se hace censo. No se guarda pointer. Estado3 anterior permite una nueva generación en recarga. IDs nuevos deben ser mayores que el último procesado; repetición durantecreate o wrap/reutilización falla cerrado. ElID conocido fuera decreación ignora ejecuciones posteriores, incluso con HP reducido o devuelto a220 por lógica nativa. Nueva instancia válida tras recarga recibe una sola asignación.

## Errores y descarga

Inicialización idempotente: valida structs/versiones/punteros; instala hook primero; StageService después; armado tras éxito/handle no cero. Si falla, queda desarmado, intenta rollback oficial del hook cuando corresponde y devuelve error; loader detach elimina elhook/edit poseídos aunque initialization parcial no llame shutdown. La decisión deHP nunca retorna SKIP_ORIGINAL ni cambia args/result. Fallos de identidad/fase/HP desarman, registran HP_REJECTED y mod_update consume el error fuera del dispatch; no se cura ni se escribe en otros actores.

HookService.h 53–57 declara callbacks en el stack e hilo de juego. hook.cpp888–927 elimina callbacks/candidatos del mod y restaura backend o transfiere el trampoline a otro cliente;1057 conecta modDetached. Loader296–338 ejecuta detach antes de unload; natives563–583 difiere FreeLibrary hasta el drenaje siguiente. No se retiene callback en petición de actor. Si falla retirar un backend vivo, hook.cpp914 llama fatal; borealis/log.hpp164–166 marca esa llamada noexcept/noreturn. Esa ruta impide continuar como si unhook vivo fuese seguro; no se prueba aquí el fallo real delbackend ni se promete recuperación sin cierre.

Shutdown desarma yno borra actores vivos. StageService retira eledit yHookService elhook; una instancia ya materializada conserva su nativo yhealth hasta muerte/cambio/salida. La prueba de desinstalación requiere cierre completo yreentrada. No se implementa pinning ni hot-unload manual.

## Límites de prueba

El laboratorio compila los headers propios de política/registro con SDK exacto y mocks de servicios; usa snapshots de actor y simula nativeinit→pre→action. No construye unactor real, ejecuta nuestra DLL/EXE del juego ni simula detours gráficos. El bridge tipado real yDEFINE_HOOK se compilan y auditan en la DLL de producción; su invocación en el stack real, HP leídos/log, comportamiento yretirada requieren prueba humana. El tripwireGX heredado pertenece solo al EXE de laboratorio yabortaría cualquier uso; no aparece en producción. No se repiten suites anteriores.
