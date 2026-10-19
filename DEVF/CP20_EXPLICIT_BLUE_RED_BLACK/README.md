# CP20 — selección explícita Blue / Red / Black

Pareja obligatoria: Provider **0.14.0**, Expansion **0.16.3**, editor **0.14.0**. Windows x64 / Dusklight 2.0.3 / D3D12. Un solo paquete por mod ID. Reinicio en frío; sin hot reload.

Derivado de FIX02 compilado `a0dd9e5a780c413a27fd165c9d3133b14d1e8613`; padre documental `ed56faaa2c718a6bf8b301881de96f02294c3015` ([fuente #79](https://github.com/iscoremusic26-maker/Mods-Duskligh/pull/79)). CI apilada sobre [#64](https://github.com/iscoremusic26-maker/codex-cp00-inputs/pull/64), commit `db30418c6e97eecd1a6dcf45a80d8839483ddc48`. No se fusionan ni modifican esas ramas.

El ZIP de CI incluye ambos .dusk, editor autocontenido, fuentes, hashes, resultados y guía humana. `VERIFICATION_MANIFEST.json` fija los commits y bytes compilados. El job independiente descarga el artifact ya publicado y repite hashes, CRC y auditorías PE. `TEST_RESULTS.json` contiene el resultado completo del baseline y CP20.

Los resultados automáticos habilitan prueba humana. **HUMAN_RUNTIME_PENDING**. No se ejecutó el juego ni se midieron modelos, audio, vida de Link o compatibilidad universal con Zora Swim.
