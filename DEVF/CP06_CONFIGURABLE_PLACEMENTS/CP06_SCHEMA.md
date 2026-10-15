# CP06 — Schema1, subconjunto conservador

JSON UTF-8 sin BOM, claves/strings ASCII sin escapes, máximo16384 bytes, profundidad8/nodos512/strings64. IDs lowercase a-z/0-9/_/- hasta32. Claves desconocidas/duplicadas, tipos incorrectos y combinaciones no soportadas se rechazan; no se ignoran.

Root exacto schema_version=1, variants(1..2), placements(0..2). Variante: variant_id,enemy=E_OC2,native_profile=510,hp,appearance,ai. Únicas combinaciones soportadas:150/charcoal/bounded_guard y220/native/native. No editor/presets generales.
Placement: placement_id,variant_id,enabled,stage=F_SP121,room3,layer6,x/y/z,yaw_s16[-32768,32767],parameters4294902016 (FFFF0100). Pitch262 y setIDFFFF fijos por contrato nativo; escala/campos extra no soportados.
Coordenadas finitas y límites de formato X/Z±1e6,Y±1e5; esos límites NO acreditan geometría. Whitelist espacial estricta: sólo(-1250,-6304.00830078125,60750), comparada como float32 nativo; sitio2 desconocido se rechaza incluso en entrada desactivada.

Ejemplos listos: examples/black-guard.json, examples/native-elite.json, examples/disabled.json; copiar UNO con nombre placements.json al data_dir. examples/invalid-two-active.json debe rechazarse MULTI_ACTIVE_NOT_SUPPORTED con0 actores. Dos IDs de placement no significan dos instancias simultáneas.
No valores default que activen spawn: archivo ausente/inválido o placements[] =>0 registros. Cambios sólo al cerrar/reabrir juego, no promesa de recarga al vuelo.
