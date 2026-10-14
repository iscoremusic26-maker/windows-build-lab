# Revisión focalizada del piloto

Fuente oficial `40457c6adb381928e4b5fef6ed459ed291edd5e2`, Aurora `3227d76c60e1e782ca576610bce61c9e7744d8be`, importlib SHA2564dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4. SOURCE_MANIFEST fija los archivos relevantes y herramientas reutilizadas. Ninguno se modifica.

## Target y momento efectivos

En src/d/actor/d_a_e_oc.cpp2771–2779 nativecreate establece field_0x560/health40 o220;2808 borra current/shape.angle.x;2829 llama daE_OC_Execute, cuya función2642–2644 llama al miembro execute;2631 ejecutaaction, que2418 llama damage_check. El pre-hook sobreexecute se sitúa después de fijar220 y antes de esas acciones. Home.angle no se borra enesa secuencia.

La comprobación estática del **Windows EXE oficial**, SHA256ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049, confirma la llamada crítica create→wrapper→execute y execute→action, sin flags de inlining para esos targets enelmanifiesto incrustado. [evidence/host-target.json](evidence/host-target.json) conserva nombres, RVAs derivados y cadenas de llamada, sin bytes originales/disassembly. ci/verify_host_target.py permite repetir únicamente esa inspección de lectura; no ejecuta elEXE. No se usó la compilación Linux ni se compiló elhost.

DEFINE_HOOK(&daE_OC_c::execute,NativeEocExecute) y mods::hook::add_pre usan el generador oficial de SDK. La firma int(daE_OC_c::*)() y healths16 se verifican porcompiler; se exige PMFcompacto8 enWindowsx64. La auditoría valida metadata HookMEM, nombres de clase/target, PMF→thunk de import exacto y servicios Host2.2/Log1.0/Stage1.0/Hook1.1/Game2.0. El importexecute es una referencia detarget; elmod no llama manualmente aexecute/create ni instala bytepatches. HookService controla eloriginal.

## Identidad y ámbito de confianza

Lecturas tipadas, ninguna dirección/offset deGameCube: profname510, room3, parametersFFFF0100, setIDFFFF, home.pos **igualdad float32 exacta** conelregistro, home.angle(262,0,0), stageF_SP121 ylayer efectivo6 porgetter nativo. Después se exige mNameE_OC2, health220, current.angle.x0, IsCreating(ID) e init_state0/1. Dentro decreate esas señales identifican el pre-first-execute; health220 por sísolo no lo hace.

El inventario ya decodificado contiene40 registrosE_oc originales entre elstage/sala/capas disponibles y cero coincidencias dehomepos; ACT6 contiene7. [identity-uniqueness.json](evidence/identity-uniqueness.json) registra elhash delinventario yelresultado sin copiar records/ARC ni reanalizar geometría. StageService añade una vez elúnico registro propio. **Es un fingerprint de instancia, no una API de ownership deStageService.** La prueba requiere únicamente CP03, sin otros mods que añadan/editen enemigos o hooks deE_OC. No se promete aislamiento ante untercero que copie exactamente elregistro o altere parámetros originales.

Solo se consulta porSDK elID previamente aceptado para rechazar unsegundo candidato concurrente aún vivo; no se busca porperfil/proximidad ni se hace censo. No se guarda pointer. Estado3 anterior permite una nueva generación enrecarga. IDs nuevos deben ser mayores queelúltimo procesado; repetición durantecreate o wrap/reutilización falla cerrado. ElID conocido fuera decreación ignora ejecuciones posteriores, incluso conHP reducido o devuelto a220 por lógica nativa. Nueva instancia válida trasrecarga recibe una sola asignación.

## Errores y descarga

Inicialización idempotente: valida structs/versiones/punteros; instala hookprimero; StageService después; armadotraséxito/handle no cero. Si falla, queda desarmado, intenta rollback oficial delhook cuando corresponde y devuelve error; loaderdetach elimina elhook/edit poseídos aunque initialization parcial no llame shutdown. La decisión deHP nunca retorna SKIP_ORIGINAL ni cambia args/result. Fallos de identidad/fase/HP desarman, registranHP_REJECTED ymod_update consume elerror fuera deldispatch; no se cura ni se escribe enotrosactores.

HookService.h53–57 declara callbacks enelstack e hilo dejuego. hook.cpp888–927 elimina callbacks/candidatos delmod yrestaura backend otransfiere eltrampoline aotrocliente;1057 conecta modDetached. Loader296–338 ejecuta detach antes deunload; natives563–583 difiere FreeLibrary hasta el drenaje siguiente. No se retiene callback enpetición deactor. Si falla retirar unbackend vivo, hook.cpp914 llama fatal; borealis/log.hpp164–166 marca esa llamada noexcept/noreturn. Esa ruta impide continuar como si unhook vivo fuese seguro; no se prueba aquí elfallo real delbackend ni se promete recuperación sin cierre.

Shutdown desarma yno borra actores vivos. StageService retira eledit yHookService elhook; una instancia ya materializada conserva sunativo yhealth hasta muerte/cambio/salida. La prueba de desinstalación requiere cierre completo yreentrada. No se implementa pinning ni hot-unload manual.

## Límites de prueba

El laboratorio compila los headers propios de política/registro con SDK exacto y mocks de servicios; usa snapshots deactor ysimula nativeinit→pre→action. No construye unactor real, ejecuta nuestraDLL/EXE deljuego ni simula detours gráficos. El bridge tipado real yDEFINE_HOOK se compilan y auditan enlaDLL deproducción; su invocación enelstack real, HP leídos/log, comportamiento yretirada requieren prueba humana. El tripwireGX heredado pertenece solo alEXE de laboratorio yabortaría cualquier uso; no aparece enproducción. No se repiten suites anteriores.
