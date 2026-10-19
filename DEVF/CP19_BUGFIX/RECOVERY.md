# Recuperación CP19

Base: CP18, PR #76 draft `feature/devf-cp18-black-fury-chain-same-room-hunt` @ `e84f2e3` (fuente compilada `71bfcb3`), CI PR #61 `ci/devf-cp18-black-fury-chain-same-room-hunt` @ `c12eda0`. Ramas nuevas apiladas: `feature/devf-cp19-bugfix` y `ci/devf-cp19-bugfix`, PR draft nuevos. Sin modificar PR anteriores ni la rama de evidencias `evidence/devf-cp19-bugs-human-runtime-20261019`; sin merge, release ni tags.

Rollback (juego cerrado por completo): retirar SOLO la Expansion 0.16.0 y restaurar la 0.15.0:
- `BokoblinExpansion_0.15.0_CP18_BLACK_FURY_CHAIN_SAME_ROOM_HUNT_TEST.dusk`, 138 755 B, SHA-256 `c2f25da000731779ef0acde2913468cf7401543ec60d60be0a94cc5b622cc8d4` (run 39233324745, artifact 12177953880).

Provider/editor 0.13.0, JSON y partidas no cambian. CP19 no guarda nada en disco: el banco de audio extra, los permisos y las máscaras viven solo en memoria y se liberan al cerrar. Sin hot reload.

Si el trabajo se corta antes de CI + descarga independiente PASS, no declarar paquete listo.

Entrega verificada: run 39271517569, artifact 12196235038, fuente `c0166f0`, CI `3b0de9e`. Instalar solo `BokoblinExpansion_0.16.0_CP19_BUGFIX_TEST.dusk` (146428 B, SHA-256 `87abfb78beb824f40b6015907c45783619932b45292112809749000a9f774692`).
