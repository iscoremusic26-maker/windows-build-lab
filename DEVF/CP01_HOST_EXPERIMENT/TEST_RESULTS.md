# Validación incremental del experimento CP01

Estado inicial: CI Windows pendiente. No usar este documento como PASS del host completo.

| Comprobación | Estado | Alcance |
|---|---|---|
| Identidad de los 12 originales y correspondencia patch/manifest | PASS estático local | Byte a byte con source 2.0.3 privado conservado |
| Compilación y laboratorio Windows x64/MSVC | NOT_TESTED pendiente CI | Funciones extraídas; adaptadores sintéticos; DLL propia |
| Compilación/enlace host completo | BLOCKED | Submódulos exactos Borealis/Aurora y transitivas no disponibles |
| Cola real, mount pendiente/error/cancelación, transición de escena | NOT_TESTED | Requiere host integrado |
| Teardown terminal completo, compatibilidad con otros mods | NOT_VERIFIED | Fuente experimental; requiere integración |
| Registro/actores DEVF, render, colisión, DX12 | NOT_TESTED | DISARMED_NO_REGISTRATION |
| CP02 | NO-GO | Host y ubicación aún pendientes |

Las pruebas anteriores conservan sus resultados históricos. No se vuelven a ejecutar las suites completas. No se emiten binarios del juego ni TEST activo.
