# CP19 FIX01 — resultados

**SOURCE_IMPLEMENTED / LAB_PASS (local) / WINDOWS_MSVC_PENDING / INDEPENDENT_DOWNLOAD_PENDING / HUMAN_BOOT_RUNTIME_PENDING / HUMAN_A-G_PENDING**

## Local (WSL2, g++ 15.2, C++20, ASan+UBSan)

| Gate | Resultado | Clase |
|---|---|---|
| `cp19f1_lab`: 65 casos CP19 + 8 de arranque (motor falso con el ciclo de vida de la fuente y contadores de llamadas; incluye el orden de 0.16.0, que sí toca el audio sin heap) | 73/73 | LAB |
| Mutaciones (30 CP19 + 11 de la puerta) | 41/41 eliminadas | LAB |
| `mod.cpp` y `cp19f1_abi.cpp` con g++ contra cabeceras oficiales | 0 errores | COMPILADOR NO-MSVC |
| `source_gate.py` contra CP19 y 33 blobs oficiales | PASS | INSPECCIÓN |

## Windows MSVC y descarga independiente (CI)

Pendiente.
