# CP19 FIX02 — resultados

**A y B: SOURCE_IMPLEMENTED / LAB_PASS (local) / WINDOWS_MSVC_PENDING / INDEPENDENT_DOWNLOAD_PENDING / HUMAN_RUNTIME_PENDING**

## Local (WSL2, g++ 15.2, C++20, ASan+UBSan)

| Gate | Resultado | Clase |
|---|---|---|
| `cp19f2_lab`: 40 casos contra puertos del motor (loader/heap ARAM/wave table, escena, colisión con escudo, medidor, `setDamagePoint` como control negativo, calendario de color de `draw`) | 40/40 | LAB |
| Mutaciones de `cp19f2_policy.hpp` y de la puerta FIX01 | 35/35 eliminadas | LAB |
| `mod.cpp` y `cp19f2_abi.cpp` con g++ contra cabeceras oficiales | 0 errores, 0 avisos propios | COMPILADOR NO-MSVC |
| `source_gate.py` contra FIX01 y 52 blobs oficiales | PASS | INSPECCIÓN |

## Windows MSVC y descarga independiente (CI)

Pendiente.
