# Recuperación CP08
Base inmutable: `5c7d2be69cb4291bb37a11fd240c406b1f4d2dd8`, PR47.
Rama: feature/devf-cp08-map-editor; únicamente archivos nuevos bajo esta carpeta. No modifica fuentes históricas; las reutiliza por include/link de archivos de la base exacta.

Rollback:
- Provider0.7.4 SHA256 `934a0a1a5127fa994b9325a5f94a493d0df53f3b5f9af9acfce3709006379fbb`.
- Expansion0.1.0 SHA256 `104a6c0908f498b0a833b7ed31d12e75f224bc2f232633bd921446c217d2333f`.
- Restaurar con juego cerrado y JSON propios respaldados; jamás dos versiones con el mismo ID.

La compilación CI usa SDK40457c6 intacto, importlib oficial verificada y .NET8 WinForms self-contained. SOURCE_CI.json identifica el commit fuente (solo mirror CI). SOURCE_COMPILED.zip contiene fuentes propias, no SDK ni ARC. La verificación independiente descarga el artifact real y contrasta ZIP, DLL, metadata/API, app PE y ledger.

El archivo de mapa original queda en el PC del usuario; no se incluye en la entrega. La prueba privada CI usa el input previamente preservado sin publicarlo. No hay juego ejecutado por CI.
