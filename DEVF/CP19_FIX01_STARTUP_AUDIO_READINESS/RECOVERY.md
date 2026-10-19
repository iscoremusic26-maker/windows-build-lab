# Recuperación CP19 FIX01

Base: CP19, PR #77 draft `feature/devf-cp19-bugfix` @ `ec6f76d` (fuente compilada `c0166f0`, **0.16.0 crashea al arrancar**), CI PR #62 `ci/devf-cp19-bugfix` @ `3b0de9e`. Ramas nuevas: `feature/devf-cp19-fix01-startup-audio-readiness`, `ci/devf-cp19-fix01-startup-audio-readiness`, PR draft nuevos. Sin modificar PR anteriores ni las ramas de evidencias; sin merge, release ni tags.

Rollback si FIX01 falla (juego cerrado por completo): retirar la Expansion 0.16.1 (y cualquier 0.16.0) y restaurar solo la CP18 0.15.0, probada por el usuario:
- `BokoblinExpansion_0.15.0_CP18_BLACK_FURY_CHAIN_SAME_ROOM_HUNT_TEST.dusk`, 138 755 B, SHA-256 `c2f25da000731779ef0acde2913468cf7401543ec60d60be0a94cc5b622cc8d4` (run 39233324745, artifact 12177953880).

Paquete FIX01 (el que se retira en caso de rollback): `BokoblinExpansion_0.16.1_CP19_FIX01_STARTUP_AUDIO_READINESS_TEST.dusk`, 147 628 B, SHA-256 `8b93fabd3ba088f10c6dcc8b2cf1100656df10f019726fcb61a99378c3ac28d9` (run 39277191308, artifact 12197643964). Fuente compilada `5c8ce00`, CI `cc6e65c`, PR draft fuente #78 y CI #63.

Provider/editor 0.13.0, JSON y partidas no cambian; nada se guarda en disco. Nunca dos Expansion a la vez.
