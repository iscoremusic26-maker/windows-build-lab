# Recuperación
Baseline source CP08 9b44c40e12f108d1b585f15e1c17bc2c621ccffe; CI92e877a44b392e75bc9dc819f4932c43deb5e0a0.
CP09 se construye únicamente desde app/DEVF.MapEditor.csproj en rama nueva; no se compilan los .dusk.

Rollback usuario: cerrar CP09 y abrir DEVF Map Editor0.8.0 histórico. Conservar proveedor0.8.0 y Expansion0.2.0. Configuración schema1 y backups históricos no se convierten. Para reconstruir catálogo, con CP09 cerrado retirar solo archivos catalog-v1-*.json bajo LocalAppData/DEVF/MapEditor/CP09; la imagen original no se modifica. last-image.txt es referencia privada local y puede eliminarse.

Build reproducible: Windows .NET8 SDK, dotnet publish app/DEVF.MapEditor.csproj -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true. La CI aislada registra SHA fuente e inputs en SOURCE_CI.json, verifica ledger y ejecuta --self-test contra el R03 privado existente. Nunca agregue ese ARC a Git ni al ZIP.

No merges, force, amend, retag ni cambios a PR48/34. Una falla requiere corregir solo CP09 y repetir su CI, manteniendo evidencias anteriores.
