# Recuperación verificable

Estado: RESEARCH_ONLY; no existe un mod nuevo. Se recuperaron 131 archivos, normalizados al byte exacto y comprobados contra sus SHA de blob GitHub: 70 de POST05/contratos y 61 de CP21. El worktree de tarea parte del snapshot local 5ae33d3, que NO es un commit upstream. Origen real: HEAD documental 2621e53bfb92f923d7b19d5328d6ca39c276f79f; fuente compilada 0.5.1 ac99b4dfb1413355e3e935e9b22d311c40d84bee. CP21 192a7731623d69baacdd37011db60ce894d9de18.

| Hallazgo | Evidencia recuperada | Dictamen |
|---|---|---|
| A1: log y Blue 6/6, EOC2 9 | evidence/runtime/B2_SUMMARY_SANITIZED.json | [R] confirmado sobre 0.5.1 |
| A2: copia original sin commits | rama work sin commit; snapshot aislado y worktree posterior | confirmado; no confundir con upstream |
| A3: proxy Git falla | conexión a proxy:8080 falló; lectura/descarga mediante conector | confirmado |
| A4: EXE y POST Expansion | EXE SHA fijado; dynamic_post exacto CP21 | [S][B] confirmado en alcance documentado |
| A5: velocidad por fase | tests/interaction_probe.cpp contra siete headers CP21 exactos | [L] confirmado para casos modelados |
| Archivo experimento Astra | no existía después de interrupción | reconstruido; no atribuido como cambio sobreviviente |

El baseline descargado por ID 13098463747 fue verificado: .dusk 32350 bytes, SHA256 f5be0ed2e00c196ce261db72756256fd5513229c4370ae4deee62ae6d90b83fb. Se preserva sin reemplazar. Las pruebas históricas se repitieron antes de las nuevas. No se modificó src, CMake, manifiesto ni auditorías históricas.
