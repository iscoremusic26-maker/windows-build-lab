# Recuperación/checkpoint

Base fuente documental1a61173bfe0e5da3e5e1144c687a399c78a0e660, CP16 compiladoe364fdb1489f7e61a0e0719a62fb627b5749f730, CIa5c33dcc065d6cf7861d00bbe28b40e88b7b2911. PR72/57 y artifacts históricos no se modifican. Nueva rama fuente/CI y PR draft FIX01; sin merge/tags/release.

Rollback frío: cerrar Dusklight completamente, retirar únicamente Expansion0.13.1, restaurar BokoblinExpansion_0.13.0_CP16_TEST.dusk (99543bytes; SHA256216e19d91f67039e3ac999c38b0b8bdeb1881f445672bed01e23f959c829ef7b). Mantener Provider/editor0.13.0, JSON/saves respaldados. No coexistencia de dos Expansion, no hot-unload. No borrar backups ni recompilar baseline.

Trabajo recuperable: nuevo runtime segregado, lab53, CMake, gate de procedencia, CI/auditoría y manual. SOURCE_CI_CP16_FIX01.json reside en rama CI y fija source exacto/blobs; conservar distinción implementación compilada vs cierre documental. VERIFIED_PACKAGE.json se publica solo con run/artifact/hashes reales. Si hay corte antes, el build sigue PENDING; no usar artifacts de fallo.

Siguiente paso tras build+descarga PASS: QA humana dirigida de HUMAN_TEST, no CP17. Hasta reporte FIX01=HUMAN_RUNTIME_PENDING. Tres abortos genéricos históricos no tienen causa exacta recuperable; los nuevos logs permiten discriminar. Red canary independiente pendiente.
