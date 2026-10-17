# CP10: implementación y límites
Base source 6e986652789a40e10488df2092582a3053f02d3b; CP08 runtime 9b44c40e12f108d1b585f15e1c17bc2c621ccffe. Solo archivos nuevos bajo CP10. Disc/Archive/Catalog/Geometry y pruebas CP09 enlazados sin modificación; Config schema1 CP08 enlazado intacto.

## Sala y capa
Host oficial 40457c6adb381928e4b5fef6ed459ed291edd5e2:
- sdk/include/mods/svc/stage.h: add_actor(ctx,stage,uint8_t room,int8_t layer,record,size,out_handle).
- src/dusk/mods/svc/stage.cpp: stage_create_new_actors obtiene getLayerNo(0), filtra stage actual, room exacta y layer exacta (host también admite wildcard; CP10 lo prohíbe).
- src/d/d_stage.cpp:2728–2731 verifica capa global 0<=l<15;2773 layerActorLoader usa esa capa;2816–2835 roomReLoader llama creación de Add después del decode. Los chunks ACTn no prueban capa activa.
No se añade hook ni lectura de actor. Los logs distinguen STAGE_EDIT_ONLY/ACTOR_NOT_OBSERVED; no pueden diagnosticar por sí solos wrong-layer frente a actor ausente.

Candidatas limitadas a rooms1..5/variante00 de F_SP121, como conjunto QA, no whitelist de spawns observados. El editor enlaza nombre de recurso con PLC room y presencia DZR, reconstruye malla y verifica hash real al cargar. Esto no demuestra disponibilidad de E_OC2, scripts, navegación o layer runtime de salas nuevas. Se requiere prueba una sala/capa a la vez.

## Geometría y autorización
PlacementGeometry usa Mesh.Floor del lector CP09: normal Y>=.7, flags PLC conservadores y room exacta. Busca suelo, rechaza alturas superpuestas >30, muestrea huella +/-100 cada50, desnivel <=50, pantalla de pared200 a altura5..250; Y=float32(suelo+5). Bounds del recurso, sin transformaciones de atlas. Control estático, no garantía para colisión dinámica.

Schema2 contiene GZ2E01/rev0, tabla de recursos (ruta interna normalizada, room/variante, ARC/KCL/PLC SHA256, PLC room, bounds y autorización STATIC_QA_NOT_RUNTIME_VERIFIED) y encuentros con referencia, layer explícita, profile510/paramsFFFF0100, estado QA. No contiene hash del disco ni ruta personal.
El editor exige recargar cada recurso auténtico y revalidar cada posición antes de exportar, incluidas disabled; metadatos de un archivo externo no habilitan exportación sin esa comprobación. Consumidor valida toda la tabla, referencias, hashes sintácticos, límites, escena, autorización y posiciones antes de pedir nada. No lee el CISO ni verifica criptográficamente suelo: el JSON es autorización local de QA, no un certificado firmado. Una persona que falsifique todo el JSON puede falsear su procedencia; no se ofrece seguridad contra modificación maliciosa de archivos propios. No confundir hashes con autenticación.
Schema1 conserva su parser/bytes numéricos. Copia explícita a schema2 exige R03 hash original, suelo y posiciones exactas. No sobrescribe original.

## Runtime
Se conservan ambos servicios @1.0 y estructuras C ABI80 bytes request/24 service. Broker legacy CP08 reutilizado. EncounterBroker CP10 mantiene capacidad8 y range0x0132, y pasa stage/room/layer VALIDADOS de la solicitud a StageService. Native legacy usa0x0106 sin cambios.
Room distinta3 o layer distinta6 requiere owner dev.bokoblin.expansion; no prueba suelo. Todos los propietarios comparten slots, IDs globales y separación300 por room/layer; posiciones iguales en distintos contextos no son duplicado. El cliente registra el lote solo tras validar JSON y retira tickets propios en orden inverso si falla. Handles no son actores. Bootstrap termina en primer update; sin reinyección. Remove edit no elimina actores vivos.

## Persistencia y UI
Colección global y mapa por recurso, selección/movimiento/borrado/yaw, activación explícita. Mover cambia a disabled y retira confirmación. Capa modificada también. Descarta/reabre, copia legacy y exporta primero archivo independiente. Instalación explícita en data_dir seleccionado; referencia habitual APPDATA/TwilitRealm/Dusklight/mod_data/dev.bokoblin.expansion. No toca placements.json.
Save verifica hash externo antes y después de temporal, crea respaldo único verificado, flush y relectura; revierte tras fallo de reemplazo. No garantía contra corte de energía o carrera entre procesos. No se modifica disco/juego/SDK.

## Corrección focal de compatibilidad CP08
La ruta CP08_OBSERVED exige recurso auténtico R03_00.arc (SHA Geometry.Hash), F_SP121/3/6 y variante00. Conserva una Geometry.LoadBytes(raw) histórica y delega en Config.Validate + Config.GeometryCheck sin reescribir sus cálculos, región ni tolerancia float32 de altura. CP08_OBSERVED es procedencia legacy, no certificado para puntos nuevos. La ruta QA_CANDIDATE conserva Place/Mesh.Ground sin cambios, inclusive room3. Al cambiar posición, capa o yaw se desactiva el marcador y pasa a QA_CANDIDATE. Seleccionar otra sala no modifica la colección. Los vectores humanos solo existen en tests.
