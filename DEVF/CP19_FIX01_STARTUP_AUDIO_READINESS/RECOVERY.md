# Recuperación CP19 FIX01

Base: CP19, PR #77 draft `feature/devf-cp19-bugfix` @ `ec6f76d` (fuente compilada `c0166f0`, **0.16.0 crashea al arrancar**), CI PR #62 `ci/devf-cp19-bugfix` @ `3b0de9e`. Ramas nuevas: `feature/devf-cp19-fix01-startup-audio-readiness`, `ci/devf-cp19-fix01-startup-audio-readiness`, PR draft nuevos. Sin modificar PR anteriores ni las ramas de evidencias; sin merge, release ni tags.

Rollback si FIX01 falla (juego cerrado por completo): retirar la Expansion 0.16.1 (y cualquier 0.16.0) y restaurar solo la CP18 0.15.0, probada por el usuario:
- `BokoblinExpansion_0.15.0_CP18_BLACK_FURY_CHAIN_SAME_ROOM_HUNT_TEST.dusk`, 138 755 B, SHA-256 `c2f25da000731779ef0acde2913468cf7401543ec60d60be0a94cc5b622cc8d4` (run 39233324745, artifact 12177953880).

Provider/editor 0.13.0, JSON y partidas no cambian; nada se guarda en disco. Nunca dos Expansion a la vez.
