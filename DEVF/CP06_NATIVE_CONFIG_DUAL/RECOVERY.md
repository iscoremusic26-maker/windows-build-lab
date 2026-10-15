# Recuperación de CP06 Native Config Dual

Base inmutable:89dad44ca07ddd414e946e6d910371b9b7a1e7a3.
Rama nueva:feature/devf-cp06-json-dual-native-qa. Carpeta nueva:DEVF/CP06_NATIVE_CONFIG_DUAL/. Sólo adiciones; sin merge, amend, force, rebase ni retag. PR/commits CI y fuente realmente compilada se anotarán después de verificarse.

Copias mínimas propias: native_records de QA B intacto; registration adaptado de bool dual a máscara validada. Parser/IO originales preservados en sus rutas históricas. Nuevo Runtime coordina comprobación de servicios, lectura completa, validación, registro y cleanup; lo usan la DLL y laboratorio.
Se conserva CP06 v0.6.0:41419bytes/SHA256 b1e8f63797e93c0064e83fc39a984c1c806d046761f675b20ac55063b6a6dfa2; QA A/B y tags históricos intactos.

En clon autorizado, git fetch origin feature/devf-cp06-json-dual-native-qa; comprobar SHA exacto publicado y git merge-base --is-ancestor89dad44… antes de checkout separado. Nunca sustituir un paquete histórico por una recompilación.
No Cloud shell disponible; cambios/publicación reales por conector GitHub y build por Actions. No gameplay en Codex.

Rollback humano en HUMAN_TEST.md. Config ausente/inválida deja el mod desarmado; no fallback de aparición. Retirar edición no mata actores vivos: cierre completo obligatorio.
