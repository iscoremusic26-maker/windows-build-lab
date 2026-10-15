# DEVF CP07.1 — Editor nativo dentro de Dusklight

Mismo ID dev.devf.cp06_native_config_dual, versión nueva0.7.0. Instalar **sólo una versión DEVF**. Reutiliza placements.json de0.6.2 en HostService.data_dir, normalmente %APPDATA%/TwilitRealm/Dusklight/mod_data/dev.devf.cp06_native_config_dual/. La carpeta de datos activa personalizada/portable/--user-dir prevalece; no inferirla de --mods.

Menú del juego: **DEVF Editor**, register_menu_tab + window_push de UiService2.3. Dos toggles FIELD/NORTH, posiciones de sólo lectura, estado guardado/provisional, Guardar configuración y Descartar cambios. UiService es import opcional: si falta o falla UI, el core CP06 continúa con su JSON válido. No fallback a Mods panel necesario. No UI/IO por frame; getters/predicados sólo consultan memoria. El cierre/reapertura conserva el borrador; Descartar vuelve al último estado guardado de esta sesión.

Guardar sólo persiste JSON para el **siguiente arranque frío**. Nunca registra/destruye actores ni cambia HP/color/IA desde UI. Datos/records/coreStage/parser/lector y109casos anteriores se copian intactos a esta carpeta. Mismos sitios whitelist, yaw0 y paramsFFFF0100. No coordenadas libres ni personalización.

## Protección del archivo
Se valida JSON generado con el parser schema1 antes de IO. IDs de sitios existentes se conservan; sitios ausentes se incorporan desactivados o según toggle explícito, sin colisiones de ID. Catálogo nativo único,220 sólo metadato.
Se distingue archivo ausente de fallo de acceso mediante FileService.list completo. JSON inválido/acceso fallido -> editor sólo lectura, sin sobrescritura automática. Ausente -> borrador0/0, sin spawns; Guardar es creación explícita por create_child exclusivo.
Antes de sobrescribir uno válido: comprobar que sus bytes no cambiaron, crear respaldo exclusivo placements.cp07-backup-01.json..32, escribir/flush/close y releer/validar bytes del respaldo, revalidar original, escribir nuevo/flush/close, releer/validar. No se sobrescriben backups previos.32slots ocupados -> rechazar guardado antes de tocar original; mover respaldos manualmente fuera con juego cerrado si se necesita espacio.
Error al escribir -> intentar restaurar bytes previos, conservando respaldo. Si falla recuperación, editor bloquea nuevo guardado y avisa recuperación manual. Si falló creación de un archivo antes ausente, intentar dejar JSON válido desactivado; FileService no tiene delete. Puede quedar archivo incompleto ante fallo persistente: no reiniciar antes de reparar.
**No reemplazo atómico, bloqueo entre procesos ni garantía ante corte de energía/crash.** Backup verificado es recuperación, no atomicidad. No editar el mismo archivo externamente mientras se guarda. Nunca borrar backup automáticamente.

Sólo llamadas oficiales Host/File/Stage/UI/Log, sin SDK/host/Aurora/Borealis/EXE/importlib/ROM/saves modificados. remove_actor_edit retira edición, no actor vivo. UiService elimina slots/callbacks propios al detach; shutdown primero desactiva callbacks y retira menú/ventana, luego ediciones. Sin hot unload.

Estado build/package/download **PENDING** hasta ejecución real; nuevo gameplay **HUMAN_RUNTIME_PENDING**. Cierre CP06 en ../CP06_NATIVE_CONFIG_DUAL/HUMAN_RUNTIME_RESULTS_2026-10-15.md usa sólo reporte humano/sumario aportado, sin fingir lectura de logs.
