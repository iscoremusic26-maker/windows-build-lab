# QA nuevo CP07.1 — no repetir cuatro pruebas CP06

1. Respaldar saves y copiar placements.json fuera como respaldo. Cerrar completamente Dusklight. Retirar .dusk0.6.2/A/B/otros DEVF; instalar sólo el nuevo0.7.0 con el mismo ID.
2. Conservar el placements.json actual; para este flujo partir de BOTH. Ruta normal: %APPDATA%/TwilitRealm/Dusklight/mod_data/dev.devf.cp06_native_config_dual/placements.json. Si datos activos personalizados/portables, usar esa carpeta/mod_data/ID. No instalar SourceZip ni DLL suelta.
3. Abrir el menú del juego y **DEVF Editor**. Confirmar FIELD/NORTH activados, posiciones de sólo lectura y estado guardado. Desactivar NORTH: debe quedar pendiente sin cambiar enemigos de esta sesión.
4. Pulsar **Guardar configuración** y comprobar mensaje de guardado verificado/respaldo. Si error, no reiniciar ni continuar casos; conservar log/captura y recuperar el archivo desde backup con juego cerrado si hace falta.
5. Cerrar totalmente y reiniciar: sólo FIELD adicional. Reabrir editor, activar NORTH, guardar y comprobar éxito.
6. Cerrar/reiniciar: ambos adicionales. Probar un cambio provisional y **Descartar cambios**; verificar que vuelve a lo guardado. Cerrar/reabrir editor conserva borrador hasta Guardar/Descartar; no se hace hot reload.
7. Aportar observación/log privado y captura del editor. CI no demuestra píxeles ni jugabilidad. Parar ante crash/duplicados/anomalías.
8. Rollback: cerrar, retirar0.7.0. Para volver al0.6.2 original, instalar sólo ese paquete histórico; el mismo JSON schema1 es compatible. Si se desea recuperar el JSON anterior, copiar bytes del backup verificado con juego cerrado. No borrarlos desde el editor.

Ausente: cero enemigos iniciales; Guardar crea explícitamente el archivo. Inválido: editor sólo lectura; reparar/restaurar manualmente con juego cerrado. No se modifica automáticamente. No activar variantes/color/HP/IA ni CP07.2/CP08.
