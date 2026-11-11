# CP06 — Catálogo externo, primer paso seguro

**CP06_PARTIAL — validación Windows pendiente. No CP06 MVP completo.**
Archivo externo placements.json, leído una vez al inicio frío por HostService2.2 data_dir + FileService1.0. Dos presets reales seleccionables sin recompilar: black_guard (HP150/negro/AI CP05) y native_elite (HP220, sin HP write/tinte/guardia). Un único placement activo máximo; dos entradas pueden existir pero no estar activas a la vez.

Sólo se admite el sitio humano F_SP121/room3/layer6, XYZ(-1250,-6304.00830078125,60750). Cambiar yaw, preset, enable/remove sí; **mover XYZ a otro sitio NO**, devuelve SITE_NOT_VERIFIED. Dos activos devuelve MULTI_ACTIVE_NOT_SUPPORTED antes de hooks/StageService. No se presenta esta limitación como configurabilidad espacial completa.

Se reutilizan visual/AI CP05 sin retocar colores/radios; headers originales permanecen en sus carpetas. Identidad exacta de un registro admitido, no ActorId fijo. placement_id/variant_id persistentes en config, ActorId efímero y generación/admission_epoch observados. Scene epoch general y dos actores simultáneos pendientes; no se inventa semántica que StageService no ofrece.
No hot reload. No cambios host/SDK/ROM/saves, CP01–05 inmutables. CP07/08/expansión no iniciados.
