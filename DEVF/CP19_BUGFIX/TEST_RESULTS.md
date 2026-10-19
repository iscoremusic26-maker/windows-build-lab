# CP19 — resultados

**SOURCE_IMPLEMENTED / LAB_PASS (local) / WINDOWS_MSVC_PENDING / INDEPENDENT_DOWNLOAD_PENDING / HUMAN_RUNTIME_PENDING**

## Evidencia local (WSL2, g++ 15.2, C++20, ASan+UBSan)

| Gate | Resultado | Clase |
|---|---|---|
| `cp19_lab`: 43 casos CP18 (escenarios de cadena dentro de 240) + 22 CP19 contra puertos de E_OC, Z2SceneMgr/JASWaveArc y damage_check/cc_at_check, con CP10.6 real | 65/65 | LAB |
| Mutaciones (`tests/cp19_mutations.py`, 4 políticas) | 30/30 eliminadas | LAB |
| `mod.cpp` y `cp19_abi.cpp` con g++ contra cabeceras oficiales 40457c6 | 0 errores, 0 avisos propios | COMPILADOR NO-MSVC |
| `source_gate.py` contra CP18 y 28 blobs oficiales | PASS | INSPECCIÓN |

El laboratorio no prueba audio, colisión ni animación reales.

## Windows MSVC y descarga independiente (CI)

Pendiente de ejecutar.
