# CP19 FIX01 — arranque sin crash · Expansion 0.16.1 TEST privado

Sustituye a la 0.16.0 (que crashea al arrancar). Windows x64, Dusklight 2.0.3@40457c6, DX12. Una sola Expansion; Provider/editor 0.13.0 y JSON intactos.

- Causa: `cp19_poll()` consultaba el banco de ondas de audio (`Z2SceneMgr::getWaveLoadStatus`) en los primeros fotogramas, antes de que el motor crease su sistema de audio.
- Corrección: todo acceso de audio de la Expansion espera a la señal de audio inicializado del propio motor (y se suspende durante reset, cierre y vuelta al menú).
- Las correcciones A–G de CP19 no cambian.

Descarga (privada): [artifact 12197643964](https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/39277191308/artifacts/12197643964) del run 39277191308, caduca 18-11-2026 10:25:29 America/Panama. Dentro del ZIP: `BokoblinExpansion_0.16.1_CP19_FIX01_STARTUP_AUDIO_READINESS_TEST.dusk`, 147 628 B, SHA-256 `8b93fabd3ba088f10c6dcc8b2cf1100656df10f019726fcb61a99378c3ac28d9`.

Ver IMPLEMENTATION, HUMAN_TEST, RECOVERY y TEST_RESULTS. Estado: SOURCE_IMPLEMENTED / LAB_PASS / WINDOWS_MSVC_PASS / INDEPENDENT_DOWNLOAD_PASS / HUMAN_BOOT_RUNTIME_PENDING / HUMAN_A-G_PENDING.
