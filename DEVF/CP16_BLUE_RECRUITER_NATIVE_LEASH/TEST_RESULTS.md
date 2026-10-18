# Resultados CP16 (checkpoint fuente)

| Gate | Estado y evidencia |
|---|---|
| Base CP15 | SOURCE_VERIFIED: runtime compilado9fddfd9 y cierre933f249 difieren solo documentación |
| Nueva política | LOCAL_LAB_PASS:54 casos, misma cabecera que DLL + port explícito de SOUND_WATCH/WATCH/FIND |
| Sanitizers | ASan+UBSan local PASS; LeakSanitizer no ejecutable bajo ptrace de este entorno, detect_leaks=0 explícito |
| Correa/contexto/órdenes | LAB_PASS:1800/250,64 slots, límites, stale/gen, dos Blue,150ticks,180/240 y timeout progreso60 |
| Espera/ofensiva/bonus | LAB_PASS:gate PRE sin vetoPOST; fallback; bonus650/250 separado; tasas/bloqueos CP15 |
| Causalidad locomoción | LAB_PASS en mundo sintético con/sin orden; pared/agua/caída negativos; NO navegación física |
| Archivos ajenos al adaptador | LOCAL_SOURCE_GATE_PASS:22 idénticos, resto mod.cpp byte-preserving salvo integraciones explícitas |
| Windows MSVC/importlib/PE/paquete | PENDING hasta run remoto positivo |
| Descarga independiente | PENDING hasta runner separado positivo |
| Gameplay/audio/animación/FPS | HUMAN_RUNTIME_PENDING |

Reporte CP15 comunicado por usuario: todo parecía funcionar excepto huida solitaria que se buguea; se elimina, no se depura. Log identificado `dusklight-20261018-161906.log`, no leído como rawlog por Codex. Se conserva la incidencia PREVIA CP14 `LINK_LAYOUT_CANARY_FAILED_LIFESTEAL_DISABLED`; las pruebas de política Red no la convierten en lifesteal runtime PASS.

El CI conserva suites CP08–CP15 relevantes. Los38 casos de CP15 histórico comprueban su checkpoint histórico; los54 de CP16 ejecutan la nueva política sin evasión. Resultados definitivos, fuente/CI y hashes se añadirán tras ejecución real, sin inventarlos.

Run inicial39172622838: FAIL MSVC (residuo de mensaje de inicialización CP15 referenciaba AiLine.d1/kEvadeRadius eliminados). Configure,376 pines, fuente nativa e importlib/símbolos exactos sí verificados. No produjo TEST. Se corrige la referencia residual y se conservan evidencia e historial; no es PASS de compilación.
