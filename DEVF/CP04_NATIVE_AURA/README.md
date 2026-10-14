# CP04 — marcador nativo púrpura por instancia

Piloto nuevo, módulo `dev.devf.cp04_native_aura` versión 0.4.0, desde CP03 humano `139d9329f7a2d5d191b6e0ef7149ac9bbf82d29f`. Conserva policy/observer/registro StageService de CP03: E_oc, F_SP121, sala3/capa6, parameters FFFF0100, XYZ(-1250,-6304.00830078125,60750), home.angle(262,0,0), setID FFFF y health150 una sola vez. CP03 y su paquete11975 bytes no se modifican.

La apariencia es **un marcador de aura nativa**, no una recoloración del cuerpo. Se crea un solo emisor del recurso común0x29D con color privado púrpura, alpha144, escala0.6, tasa0.4 y vida de partícula18 ticks. Se ancla a eyePos nativo después de execute; la geometría, animación, equipo y materiales del E_OC2 siguen originales. No hook de draw, clones o recursos nuevos. El efecto compartido solo se lee; color/posición/ownership se aplican al emisor recién asignado por el pool nativo.

SOURCE_VERIFIED para propiedad/lifecycle y guards del código. BUILD/TEST_FAKE/PACKAGE/DOWNLOAD pendientes. HUMAN_RUNTIME_NOT_TESTED: aspecto real, aceptación del recurso, control de un E_OC2 original, recursos y rendimiento pendientes. El contenido binario common.jpc no fue inspeccionado offline; el piloto exige comprobar sus propiedades en el juego antes de crear el efecto y rechaza formatos/flags incompatibles. No se promete que ese gate ya pasó.

[Ruta visual y límites](VISUAL_ROUTE.md), [protocolo humano](HUMAN_TEST.md). Solo CP04 instalado: retirar CP01/CP02/CP03 y mods de enemigos/render/hooks. No CP05/CP06, juego o instalación por Codex, merge o modificación de host/SDK/Aurora/lib/assets/saves. El paquete se entrega solo tras Windows/PE/ZIP y descarga real.
