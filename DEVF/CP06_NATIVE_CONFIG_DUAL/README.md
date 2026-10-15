# DEVF CP06 — dos registros nativos configurables por JSON

Mod nuevo `dev.devf.cp06_native_config_dual`, versión 0.6.2. Alcance: activar por separado los sitios field y north mediante placements.json al arrancar en frío. Cero, uno o dos registros E_oc nativos; sin hooks, HP/tinte/IA, ActorId ni identidad personalizada. No es CP06 completo.

## Configuración

HostService2.2 data_dir + FileService1.0 join/open/size/read/close, sin concatenar rutas en el mod. Ruta Windows habitual:
`%APPDATA%\TwilitRealm\Dusklight\mod_data\dev.devf.cp06_native_config_dual\placements.json`.
La autoridad es el data_dir del host; no la carpeta --mods. Si Dusklight usa datos portables, una carpeta personalizada o --user-dir, usar esa carpeta de datos activa/mod_data/dev.devf.cp06_native_config_dual/placements.json. No cambiar el host: el mod sigue data_dir automáticamente. Carga única al inicializar. No hot reload ni escritura automática de JSON.

Copiar un ejemplo y renombrarlo placements.json: both-enabled (dos), field-only (sitio 1), north-only (norte), disabled (ninguno). examples/placements.json es una copia de referencia BOTH; no se instala automáticamente.

Subconjunto schema1: raíz exacta schema_version/variants/placements. Catálogo único native_elite/E_OC2/510/220/native/native. 220 es metadato; nunca se escribe HP.
0..2 placements totales, IDs distintos, máximo uno por sitio incluso si está disabled. Se validan todos antes de registrar. Coordenadas comparadas como float32, yaw=0, parameters=4294902016, escenario F_SP121/3/6:
- Field: (-1250,-6304.00830078125,60750).
- North: (-1250,-6322.6005859375,61950), altura inicial de QA B; no sustituir por suelo KCL.
Claves desconocidas/duplicadas, tipos/versiones/catálogo/escena/valores incompatibles se rechazan íntegramente. JSON ASCII válido dentro de UTF-8, sin BOM/escapes, límite 16384 bytes, profundidad 8, nodos 512, strings 64, IDs 32. Archivo ausente o inválido/IO fallido: cero altas, sin fallback.

## Integración y procedencia

Copia/adaptación mínima del parser y IO de CP06 (blobs f952fc3db42a3d97036778f9601118bcc703704a / 7b1ed503d312106000b5753337e669ef136ceb30) y core QA A/B en689f79c1c785bea31715d814f7d03818a7f0dd72. Native records se copian intactos; registro recorre field/north en ese orden según máscara validada. Rollback inverso sólo de handles propios. remove_actor_edit no destruye actores vivos. update no hace IO ni añade registros.

Base literal 89dad44ca07ddd414e946e6d910371b9b7a1e7a3. Fuentes/SDK oficial40457c6… sólo lectura, sin host/Aurora/importlib build. Archivos y paquetes anteriores intactos.
El usuario reportó QA A correcto y dos adicionales vivos simultáneamente en QA B. El resumen de dusklight-20261015-101726.log fue analizado en ChatGPT: records=2/edit0=1/edit1=2 y PID147/148; Codex no recibió sus bytes ni atribuye cada PID a un placement. El adjunto10:17:12 era QA B truncado, no un log A. No se heredan reentrada, combate, FPS ni identidad personalizada de esas observaciones.

Build, tests y descarga de esta versión: **PENDING** hasta evidencia nueva. Gameplay nuevo: HUMAN_RUNTIME_PENDING. Véase HUMAN_TEST.md; no repetir ensayos históricos.
