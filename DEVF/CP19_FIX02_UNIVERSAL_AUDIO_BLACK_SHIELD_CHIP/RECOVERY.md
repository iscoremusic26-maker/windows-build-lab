# Recuperación CP19 FIX02

Base: CP19 FIX01, PR #78 draft `feature/devf-cp19-fix01-startup-audio-readiness` @ `716677f` (fuente compilada `5c8ce00`), CI PR #63 `ci/devf-cp19-fix01-startup-audio-readiness` @ `cc6e65c`. Ramas nuevas: `feature/devf-cp19-fix02-universal-audio-black-shield-chip` y `ci/devf-cp19-fix02-universal-audio-black-shield-chip`, con PR draft nuevos apilados. Sin modificar PR anteriores ni la rama de evidencias; sin merge, release ni tags.

Rollback si FIX02 falla (con el juego cerrado por completo): retira la Expansion 0.16.2 y restaura solo la 0.16.1:
- `BokoblinExpansion_0.16.1_CP19_FIX01_STARTUP_AUDIO_READINESS_TEST.dusk`, 147 628 B, SHA-256 `8b93fabd3ba088f10c6dcc8b2cf1100656df10f019726fcb61a99378c3ac28d9` (run 39277191308, artifact 12197643964, caduca 18-11-2026 10:25:29 America/Panama).

Si 0.16.1 tampoco sirve: CP18 0.15.0, `BokoblinExpansion_0.15.0_CP18_BLACK_FURY_CHAIN_SAME_ROOM_HUNT_TEST.dusk`, 138 755 B, SHA-256 `c2f25da000731779ef0acde2913468cf7401543ec60d60be0a94cc5b622cc8d4`.

Provider/editor 0.13.0, JSON y partidas no cambian. FIX02 no guarda nada en disco: la reserva ARAM y el grupo de sonido viven solo en memoria durante la sesión. Nunca dos Expansion a la vez.
