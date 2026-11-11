# Recuperación CP10
Source base 6e986652789a40e10488df2092582a3053f02d3b; CI base c6e20919e0f0d9ae3139879974c4e50220b1e429.
Ramas feature/devf-cp10-multi-room y ci/devf-cp10-multi-room; PRs nuevas draft. Fuente compilada se fija en SOURCE_CI.json y MANIFEST.json del ZIP, no en HEAD mutable.

Cerrar Dusklight antes de sustituir mods o JSON. Restaurar pareja CP08 (provider0.8.0 / expansion0.2.0), editor CP09 hotfix y archivos respaldados. Guardado CP10 conserva backup exclusivo; seleccionar backup válido en Abrir, verificar y restaurar explícitamente. Catálogo CP09 y CISO permanecen intactos. No mezclar dos versiones del mismo mod ID.
No reset/force/merge/retag. Los cambios nuevos se revierten retirando CP10, sin editar históricos.
Si falla CI, no distribuir TEST_READY. Reproducir workflow nuevo con source SHA literal y inputs privados existentes; no requiere CISO comercial ni recompilar host. CP11 no iniciado.

Reanudación: el run38960997596 compiló MSVC/.NET y pasó43 casos runtime CP10, pero falló en FLOOR_AMBIGUOUS al migrar legacy; no produjo paquete válido. La corrección está en PlacementGeometry.Check y delega al validador histórico solo para identidad legacy exacta. La CI repite todos los gates sobre source pin nuevo, sin sustituir artefactos anteriores.
