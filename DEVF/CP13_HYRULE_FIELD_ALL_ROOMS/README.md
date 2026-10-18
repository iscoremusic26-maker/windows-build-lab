# DEVF CP13 — F_SP121 completo en catálogo
TEST privado; HUMAN_RUNTIME_PENDING. Provider0.13.0, editor0.13.0, Expansion0.10.0. Baseline CP12 ee27d038b8572eb3e41ac6184654dc5c476681d2 y CI04f7827e8654b2d3059582af65af4ec155f4d345.

Importar CISO local GZ2E01 rev0 → todas las entradas reales F_SP121 → seleccionar recurso → suelo estricto/capa explícita → encuentro disabled → autorización QA → guardar prueba → instalar en frío. No requiere recorrer con Link. Schema1 y2 anteriores siguen legibles; no hay migración implícita. La interfaz CP08 permanece en Compatibilidad CP08.

Se admiten hasta32 encuentros y64 recursos (128KiB,8192 nodos), solo F_SP121. El dominio nativo de sala es0..63; el selector proviene del catálogo, no de una lista. Variantes de ARC son claves separadas; nunca capas. STG no es sala. Un número admitido por el ABI no prueba que exista en el disco.

KCL degenerado/ausente sigue visible y bloqueado para spawn. Puede guardarse una intención `.preparation.json` desactivada, separada del JSON runtime. El reporte F_SP121 inspecciona recursos locales y exporta solo metadatos, sin imagen/posición/bytes comerciales ni rutas personales. No asociamos STG a una sala sin evidencia. CP13 es parcial respecto a la habilitación de salas sin suelo auténtico utilizable; no certifica todas las salas en runtime.

Instalar solo el nuevo proveedor y consumidor juntos; nunca dos versiones del mismo ID. Mantener backups de CP12 y de los JSON. No hot reload ni instalación automática. El nuevo consumidor preserva CP12 con cambios de versión/capacidad; ocho auras simultáneas y64 actores clasificados permanecen como antes.32 encuentros configurados no garantizan32 enemigos simultáneos, ni FPS.
