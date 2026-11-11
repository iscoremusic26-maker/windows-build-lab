# DEVF CP10 — multi-room QA privado
Editor Windows 0.10.0, proveedor 0.10.0 y Expansion 0.3.0. F_SP121 únicamente. Máximo global: ocho ediciones contando placements legacy del proveedor.

El catálogo/importador CP09 se reutiliza sin cambios, incluido CISO sparse con cola física no referenciada <=4096 bytes. El usuario confirmó importación real GZ2E01 rev0 (79 stages, 305 salas, 384 recursos, 282 con geometría), visualización de tres recursos, compatibilidad CP08 y reapertura. Procedencia: reporte del usuario; no lectura propia de rawlogs.

CP10 permite QA explícito en salas candidatas 1–5, variante00, si el recurso local tiene DZR, PLC room coincidente y suelo compatible. Capas 0–14 seleccionadas explícitamente: ninguna capa nueva se declara observada. F_SP121/3/6 conserva la evidencia CP08 limitada a tres Bokoblins. F_SP113/F_SP117 permanecen solo visualización.

Abra el editor -> CP10 edición multi-room -> CISO/Catálogo -> seleccione recurso -> Usar recurso en CP10 QA. Seleccione capa candidata, añada en suelo aceptado. El marcador nace desactivado. Habilitar exige confirmación de QA. Guarde primero una prueba; instalar JSON requiere confirmar juego cerrado.

Schema1 sigue intacto en editor de compatibilidad y en los mods nuevos. Para combinar sus tres ubicaciones con nuevas salas: cargar R03 desde catálogo, usar **Copiar legacy schema1**, confirmar copia y guardar schema2 en otro archivo. No hay migración automática.

Solo se entregan herramienta, dos mods y fuentes propias. No incluye mapas/ARC/CISO/KCL/PLC. El CISO se lee localmente, sin Dolphin ni Link. Ver HUMAN_TEST.md. No hot reload, ActorId, personalización, CP11 o certificación universal.
