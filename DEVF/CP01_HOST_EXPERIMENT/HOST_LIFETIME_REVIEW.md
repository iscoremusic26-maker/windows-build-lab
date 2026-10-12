# CP01 — experimento del contrato de cierre del host

Estado: **IMPLEMENTACIÓN EXPERIMENTAL PARCIAL; NO-GO**. Base `5f3a7ef` / `DEVF_CP01_EXTRA_COLLISIONS_REVIEW`. No habilita DEVF. El código del mod conserva `DISARMED_NO_REGISTRATION`.

La auditoría existente demuestra que SDK 2.2/ActorService 1.0 no proporcionan una barrera de solicitudes y unload. Un `.dusk` no puede impedir que el loader retire sus datos tras un shutdown fallido. Se conserva esa conclusión sin repetir la auditoría. La nueva autorización permite un parche del host en `experiment/devf-cp01-host-lifetime`, separado del mod. No se modifica ni distribuye un ejecutable del juego.

## Código experimental

`host-lifetime.patch` corresponde únicamente a los 12 archivos enumerados en `SOURCE_MANIFEST.json`, con hashes originales y resultantes. Los originales se conservan intactos en la extracción privada. No aplicar a una instalación ni al binario 2.0.3. Aplicar solamente a una copia del source exacto mediante `ci/prepare_experiment.py`; verifica los originales antes de editar y los resultados después.

Contrato propuesto, restringido al hilo del motor y a mods que cumplen sus contratos de recursos:

1. Al cerrar, `deactivationPending` impide reactivación y nuevas creaciones del dueño, incluso de perfiles de otro mod. Perfiles individuales quedan en cierre persistente. Las factorías rechazan antes de reservar el append. StageService rechaza nuevas ediciones durante el cierre, pero permite retirarlas.
2. La solicitud estándar conserva el dueño que la envió desde antes del primer callback. El escáner verifica tanto ese dueño como el dueño del perfil. Solicitudes opacas bloquean conservadoramente: no se interpretan mediante offsets supuestos.
3. Se permite **drenado natural** de solicitudes ya aceptadas, con un permiso interno limitado a la asignación del proceso. No se inventa una cancelación del mount ni se utiliza el flag pegajoso `is_cancel` como comprobación de limpieza. Una solicitud cancelada bloquea hasta su retirada real de la cola. Si no progresa, el cierre permanece bloqueado sin timeout.
4. ProfileName se asigna de forma monótona durante el proceso, independiente del índice reciclable de SlotMap. No vuelve a usarse tras borrar un perfil. Agotamiento del rango s16 rechaza registros; requiere reiniciar el proceso. No wraparound.
5. Los wrappers de callbacks cuentan entradas del dueño y del perfil. Execute/Draw dejan de invocar código del perfil en cierre; los callbacks de creación ya aceptada y destrucción siguen disponibles para drenar. El desregistro reentrante de un perfil con su callback en ejecución se rechaza.
6. El predrenado solicita destrucción y comprueba las colas de actores y de borrado. Un valor de retorno positivo sin retirada de la cola no basta. `forceDelete` deja de enmascarar el fallo del destructor; el heap/proceso/perfil se conservan. El cambio afecta perfiles personalizados, no el callback delete de actores originales.
7. **Predrenar actores no retira perfiles.** Shutdown dispone de sus handles para limpiar. Solo después de shutdown exitoso se retiran los registros restantes de ActorService/StageService y se permite retirar la DLL. Shutdown incompleto retiene perfiles que el propio mod no haya retirado, contexto, bundle y DLL. Recursos pendientes conservados por el destructor impiden predrenar; no se llama `resDelete(phase.id==1)` como cancelación improvisada.
8. Reload/disable/forget/uninstall detienen la operación si no se drena; no sustituyen bundle/contexto ni borran el paquete. Los dependientes retenidos participan en el cierre del proveedor. Retiro nativo sigue diferido hasta el siguiente drenado del loader.
9. Un error de inicialización o excepción con estado incierto entra en retención conservadora sin repetir callbacks de rollback cuyo contrato no se conoce. Un fallo explícito de shutdown puede reintentarse. La idempotencia de los destructores y shutdown sigue siendo obligación del mod; el host no la puede fabricar.
10. Si shutdown terminal no puede drenar, la cuarentena retiene **ownership completo**, no solamente un PIN de DLL. No ejecuta el cierre global de registros. Es terminal e idempotente y bloquea nuevos tick/init. Este fallback es una propuesta experimental: el orden real de teardown del motor y de estáticos necesita validación antes de adoptarlo.

## Límite de implementación y bloqueo concreto

El ZIP exacto no incluye el contenido de `extern/borealis` ni `extern/aurora`. El CMake raíz requiere `extern/borealis/cmake/DetectVersion.cmake` antes de `project()`, y los archivos modificados incluyen `borealis/io.hpp`, `borealis/update.hpp`, `fmt/format.h` y Aurora. `.gitmodules` contiene URLs, pero no acredita los gitlinks exactos del release para todas las dependencias y sus transitivas. El pipeline previo solo usaba headers Aurora fijados para **compilar el mod**, no para compilar el host.

Por la instrucción de detenerse ante una dependencia imprescindible ausente, **no se improvisa Borealis ni se compila un host contra HEAD ajeno**. La implementación queda como parche revisable y laboratorio de funciones extraídas. Se necesita el checkout 2.0.3 con los gitlinks y submódulos/transitivas exactos para integrar/compilar el host Windows completo. No hacen falta ARC ni nuevas capturas para este trabajo.

## Pruebas y alcance

El laboratorio compila cuerpos efectivos del parche, sin transcribir una segunda versión del algoritmo: ownership, solicitudes pendientes, predrenado/desregistro, cierre del loader, dependencias retenidas y retiro nativo. Sus adaptadores de colas, layouts, heap y servicios son **sintéticos**, no declaraciones de ABI. La fixture DLL reutilizada es propia del proyecto CP01; LoadLibrary/FreeLibrary sí se ejecutan en Windows real. Ningún test ejecuta Dusklight.

Se prueban: pre-primer-callback, dos dueños, solicitudes opacas, actor activo, destrucción fallida, cola no retirada, reintento, reentrancia, callback pendiente, perfiles originales, permiso de asignación aceptada, rango sin reciclaje, dependiente retenido, init fallida, excepciones, shutdown incompleto con DLL real, idempotencia y shutdown terminal. El caso de cancelación comprueba el allocator monotónico y retirada de cola, **no** ejecuta `fpcCtRq_Cancel` ni un mount real. El wrapper RAII se adapta en el laboratorio; su integración con los layouts originales sigue pendiente.

No se repiten los 16 casos originales ni las suites de colisión/ubicación. No se construye `.dusk` activo ni el ejecutable del juego. Ver `TEST_RESULTS.md` para resultados y enlaces una vez terminada CI.

## Riesgos y garantías pendientes

- Cambios internos en la solicitud estándar alteran layout TARGET_PC: todos sus consumidores deben recompilarse juntos. No es un parche binario ni ABI compatible con un EXE sin reconstruir.
- El modelo presume APIs Actor/Stage en el hilo del motor. No garantiza mods que guardan pointers crudos, tienen threads sin join, callbacks externos no contabilizados o incumplen shutdown. No hay GO de compatibilidad general.
- Solicitudes opacas pueden bloquear otros mods temporal o indefinidamente. El agotamiento de nombres limita hot reloads por proceso. Es una política deliberadamente conservadora.
- Faltan integración real de colas/fases, recurso pendiente/error/cancelación, transición de escena, Stage edits ya aplicados, dependencias/delegación, ABI y cierre global real. La retención terminal no autoriza finalizar registros usados aún por el motor.
- Si un callback devuelve éxito mintiendo sobre sus recursos, el host no puede detectarlo con estas APIs. El contrato exige un destructor honesto. Si delete/shutdown fallan persistentemente, se retiene; no se garantiza unload ni recuperación en esa sesión.
- Reintentar una operación bloqueada es una petición al loader; el drenado ocurre en frames del motor. No se exige al usuario evitar Reload para conservar memoria. Una solicitud fallida no se transforma silenciosamente en un reload diferido exitoso.

Siguiente paso mínimo: conseguir los gitlinks/dependencias del source release exacto y compilar/aplicar el parche en un host experimental Windows separado. Después revisar integración real antes de habilitar DEVF. La ubicación continúa NOT_VERIFIED por los motivos ya documentados; no se reanaliza. **No está listo para una prueba activa Windows/DX12; CP02 permanece NO-GO.**
