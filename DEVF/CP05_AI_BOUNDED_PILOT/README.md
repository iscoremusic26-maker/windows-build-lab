# CP05 — Guardia/persecución acotada por instancia, piloto privado

Estado: IMPLEMENTED_OFFLINE_VALIDATION_PENDING. Base f7a6623f6d931bacef6f2c8e54f46f7476780ade; checkpoint inicial c01bb2e9e0da87905f703929b516de05b4c6ae78. No build/game PASS heredado.

Un único E_OC2 DEVF negro y HP150, misma aparición/identidad/colores CP04.4, nuevo ID dev.devf.cp05_ai_guard v0.5.0. Instalar sólo este DEVF cuando haya paquete verificado; nunca simultáneo CP03/CP04. Mecanismo visual copiado sin cambios funcionales desde el baseline; no nueva auditoría/render/ARC.

Cambio AI medible: la instancia probada usa nativamente percepción5000 y leash600; el piloto usa percepción1200 en guardia,1400 durante interés y una gracia de hasta30 updates FIND fuera1400 con percepción nativa5000, antes de permitir desistimiento nativo. Sólo se acota mPlayerRange dentro de execute y se restaura5000 en POST. No modifica acciones, timers, mMoveRange, posiciones, velocidad, HIO ni otros actores. Retorno nativo a zona: WALK apunta home cuando distancia>=500; no garantiza pathfinding ni retorno exacto. Son 30 updates elegibles, NO segundos ni frames GPU.

Ver SOURCE_AND_AI_GATE, AI_BEHAVIOR_SPEC, TEST_RESULTS, HUMAN_TEST y RECOVERY. Estado real final se entrega con SHA/Actions y evidencia; ningún TEST antes de gates. CP06/expansión fuera del alcance.
