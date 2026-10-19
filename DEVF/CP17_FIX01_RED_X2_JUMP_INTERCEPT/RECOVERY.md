# Recuperación CP17 FIX01

Base: CP17 PR #74 draft `feature/devf-cp17-red-aggressive-ai` @ `7b8a7c5` (binario 0.14.0 compilado desde `8fbaea7`), CI PR #59 `ci/devf-cp17-red-aggressive-ai` @ `a14ca4e`. Nuevas ramas apiladas `feature/devf-cp17-fix01-red-x2-jump-intercept` y `ci/devf-cp17-fix01-red-x2-jump-intercept`, PR draft nuevos. Sin modificar PR #74/#59 ni históricos, sin merge, release ni tags. La rama de evidencias no se toca.

Rollback (juego cerrado por completo): retirar SOLO la Expansion 0.14.1 y restaurar una de estas, nunca dos a la vez:
- CP17 0.14.0: `BokoblinExpansion_0.14.0_CP17_RED_AGGRESSIVE_AI_TEST.dusk`, 113 055 B, SHA-256 `d3b888d13c6ca23c91761337e7a2cbbf43794e6ea78f5d9f6f497f9c48fcc461`.
- CP16 FIX01 0.13.1: `BokoblinExpansion_0.13.1_CP16_FIX01_TEST.dusk`, 103 638 B, SHA-256 `f5d601f5e9d7a7f8e141a59aa1b0ee811bcbf2dfac16c1add6a40033e2e729b6`.
Mantener Provider/editor 0.13.0, JSON y partidas. Sin hot reload, sin borrar configuración ni assets.

Código, política, laboratorio, mutaciones, CMake y CI están segregados en `DEVF/CP17_FIX01_RED_X2_JUMP_INTERCEPT`. `SOURCE_CI_CP17F1.json` (repo CI) fija la fuente real compilada y los blobs del espejo; los ficheros CP17 del espejo conservan su origen compilado `8fbaea7` para la regresión de 64 casos. Si el trabajo se corta antes de CI + descarga PASS, no declarar paquete listo.

Siguiente acción mínima tras una entrega válida: HUMAN_TEST (pasos 2–4 primero). No iniciar CP18.
