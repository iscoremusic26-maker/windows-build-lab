# CP19 FIX01 — arranque sin crash · Expansion 0.16.1 TEST privado

Sustituye a la 0.16.0 (que crashea al arrancar). Windows x64, Dusklight 2.0.3@40457c6, DX12. Una sola Expansion; Provider/editor 0.13.0 y JSON intactos.

- Causa: `cp19_poll()` consultaba el banco de ondas de audio (`Z2SceneMgr::getWaveLoadStatus`) en los primeros fotogramas, antes de que el motor crease su sistema de audio.
- Corrección: todo acceso de audio de la Expansion espera a la señal de audio inicializado del propio motor (y se suspende durante reset, cierre y vuelta al menú).
- Las correcciones A–G de CP19 no cambian.

Ver IMPLEMENTATION, HUMAN_TEST, RECOVERY y TEST_RESULTS. Estado: HUMAN_BOOT_RUNTIME_PENDING / HUMAN_A-G_PENDING.
