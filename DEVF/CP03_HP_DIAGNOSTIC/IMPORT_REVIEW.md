# Imports reales del diagnóstico

Run 38540468476 compiló producto/laboratorio y aprobó los 17 casos nuevos. Se detuvo antes del TEST porque el PE añadió `__std_terminate` a VCRUNTIME140.dll, fuera de la lista heredada. No se publicó un artefacto diagnóstico incompleto.

El mapa de enlace propio identifica `__std_terminate` y `__imp___std_terminate` con proveedor `vcruntime:VCRUNTIME140.dll`. Es terminación del runtime C++ de MSVC, compatible con las funciones noexcept del observador; no un target de juego, detour, parche, API de memoria o driver. No se afirma que haya ocurrido una terminación runtime. Logger tiene captura de excepciones y rechazo sin escrituras adicionales.

Se autoriza **solo ese símbolo en esa DLL**. La identidad esperada nueva tiene 29 imports: 6 nativos idénticos al baseline y 23 CRT/sistema. No cambia target, PMF, metadata de siete records, cinco exports, servicios ni superficie nativa. La auditoría sigue exigiendo igualdad completa, sin wildcard, y ausencia de TLS/delay/manual hooks/ActorService.

Tras esta corrección exclusiva de auditoría/CI, el código C++, headers, CMake y laboratorio no cambian. lab-reuse.json sella sus seis hashes, SDK/Aurora/lib/toolchain y el run real de los 17 casos. No se vuelven a ejecutar por rutina; si hay drift se bloquea reutilización. Nuevas auditorías PE/negativos/package sí se ejecutan en el siguiente run.
