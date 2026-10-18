# Contrato CP13
## Cambios
El overlay runtime/ contiene la clausura de35 fuentes CP12/CP10: las políticas de combate/render se copian sin cambios. Solo se modifican versión del consumidor, capacidad de registros/tickets/tablas Black-Normal32, parser128KiB/8192 nodos, identidad room0..63/ARC variante y broker. El juego/SDK y las carpetas históricas no se modifican. PERF_WINDOW, reglas Blue220/regen2/650XZ250Y, Red330/regen3 y Black/regen4/furia300, restituciones y presupuesto8 auras mantienen sus fuentes CP12. Los registros native_placement legacy mantienen0x0106; encounter conserva0x0132.

## Sala y suelo
SDK StageService1.0 add_actor usa uint8_t room,int8_t layer; stage.cpp:stage_create_new_actors compara room con roomNo y layer_matches. Cero no se excluye; wildcard255 no se admite aquí. Captura previa encounter_core ya valida0..63. No cambia ABI request/receipt/ServiceHeader ni los servicios@1.0. Se requieren capas explícitas0..14, sin wildcard ni deducción del sufijo.

PlacementGeometry comprueba hash ARC, nombre/ruta Stage/F_SP121/Rnn_vv.arc, PLC room y DZR único con cabecera/tabla válidas, malla, bounds, pendiente/huella/pared y ambigüedad. R03 auténtico conserva Config.GeometryCheck histórico solo para CP08_OBSERVED/F_SP121/3/6/00/hash exacto. Cualquier movimiento/capa/yaw invalida el estado legacy. Los recursos se identifican por stage/room/variante; hashes de contenido iguales no sustituyen contexto. Runtime comprueba metadatos/referencias, no tiene CISO ni certificado criptográfico de suelo. No acepta STG como room.

KCL_DEGENERATE proviene de GeometryReader.Read: r1=n.Cross(a),r2=b.Cross(n), denominadores Dot(c) <=1e-8. Se rechaza el recurso completo; omitir un prisma podría ocultar una pared o superficie. KCL_ABSENT_OR_MULTIPLE exige una pareja KCL/PLC única. Sin recursos locales de R06/STG no se demuestra si hay asociación compartida o defecto de origen. El reporte local cuenta KCL/PLC/DZR y repite la reconstrucción, conservando la causa exacta. No se ha implementado composición de geometría no demostrada. Las intenciones bloqueadas nunca son encuentros runtime.

## Capacidad y costes
32 slots globales, incluidos los dos posibles placements legacy del proveedor; rollback inverso del lote y ownership sin cambios. IDs globales únicos; separación horizontal300 solo en misma room/layer; distintas variantes ARC no cambian ese contexto físico. 64 recursos máximos;128KiB;8192 nodos; profundidad8. Schema1 conserva límite8. El ABI no contiene arrays de estos tamaños. Clasificador/dinámica64 y máximo8 auras permanecen intactos. Clausura acotada, sin nuevo escaneo del mundo por tick. El laboratorio imprime sizeof y tiempo de100 parseos; no mide FPS ni GPU.

## Archivos
app/ añade catálogo dinámico, preparación bloqueada y reporte de metadatos local. Catálogo e importación conservan CP09; nueva lectura raw solo read-only por hash/FST. El modo legacy conserva el formato y coordenadas previas. Guardado de encuentros mantiene backup verificado/conflicto externo/temporal/relectura/rollback. Los nuevos puntos siguen disabled y requieren autorización explícita; exportar no crea actor. remove_actor_edit no elimina actor vivo.

El preset inicial de nuevos Black es440 en CP13. Los HP concretos ya presentes en schema2 se conservan sin alteración, incluido cualquier valor permitido anterior.
