# Recuperación CP18

Base: CP17 FIX01, PR #75 draft `feature/devf-cp17-fix01-red-x2-jump-intercept` @ `43de14d` (fuente compilada `77e2da4`), CI PR #60 `ci/devf-cp17-fix01-red-x2-jump-intercept` @ `76fb2a9`. Ramas nuevas apiladas: `feature/devf-cp18-black-fury-chain-same-room-hunt` y `ci/devf-cp18-black-fury-chain-same-room-hunt`, con PR draft nuevos. Sin modificar PR #75/#60/#74/#59 ni históricos, sin merge, release ni tags. La rama de evidencias no se toca.

Rollback (juego cerrado por completo): retirar SOLO la Expansion 0.15.0 y restaurar la 0.14.1:
- `BokoblinExpansion_0.14.1_CP17_FIX01_RED_X2_JUMP_INTERCEPT_TEST.dusk`, 124 913 B, SHA-256 `a11c921452e0ab5dea35567dbf22bab8e17a48ee097ce1c828225781bf5e0740` (run 39194139875, artifact 12160943797).

Provider/editor 0.13.0, JSON y partidas no cambian. CP18 no guarda nada en partidas ni JSON: el Hunt y la cadena viven solo en memoria y desaparecen al cerrar el juego. Sin hot reload.

Código, política, laboratorio, mutaciones, CMake y CI están en `DEVF/CP18_BLACK_FURY_CHAIN_SAME_ROOM_HUNT`. `SOURCE_CI_CP18.json` (repo CI) fija la fuente compilada y los blobs del espejo. Si el trabajo se corta antes de CI + descarga independiente PASS, no declarar paquete listo.
