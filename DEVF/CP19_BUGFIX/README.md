# CP19 — correcciones A–G · Expansion 0.16.0 TEST privado

Sustituye **solo** la Expansion 0.15.0 (CP18). Provider/editor 0.13.0, JSON y recursos del usuario intactos. Windows x64, Dusklight 2.0.3@40457c6, DX12. Sin dos Expansion ni hot reload.

- **A audio**: en salas de Hyrule Field cuyo banco de efectos no trae las muestras de Bokoblin, carga el banco de las salas 2/3/4 con la API del propio motor mientras haya Bokoblin.
- **B Z-target**: devuelve el fijado nativo a los Bokoblin que sobrevivieron con 1 HP y se recuperaron.
- **C música**: completa la muerte nativa de Bokoblin que quedaban vivos con 0 HP (rama Jump Attack) y limpia la bandera residual de golpe mortal que congelaba la mezcla de batalla.
- **D Blue**: los aliados convocados pueden ignorar su correa mientras el Blue viva o, muerto el Blue, mientras sigan peleando.
- **E/G golpes x2**: reacciones Red y furia Black golpean desde ≤240 tras un acercamiento nativo, en vez de lanzar el golpe a 300–400.
- **F Helm Splitter**: 100 HP con la Espada Maestra, 50 con la de Ordon, en vez de muerte instantánea; con menos vida, muerte nativa.

Estado: **SOURCE_IMPLEMENTED / LAB_PASS / WINDOWS_MSVC_PASS / INDEPENDENT_DOWNLOAD_PASS / HUMAN_RUNTIME_PENDING**.

[Descargar TEST privado](https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/39271517569/artifacts/12196235038) (run 39271517569), caduca **18-11-2026 09:46:54 America/Panama**. Fuente compilada `c0166f0384124245352b55e279340a6530a65b47`, CI `3b0de9ea509989057de4325d7c2b34b4c55f2cee`. `BokoblinExpansion_0.16.0_CP19_BUGFIX_TEST.dusk` 146428 B, SHA-256 `87abfb78beb824f40b6015907c45783619932b45292112809749000a9f774692`. Este cierre documental no cambia el binario.

Ver IMPLEMENTATION, HUMAN_TEST, RECOVERY y TEST_RESULTS.
