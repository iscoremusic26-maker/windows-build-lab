# CP19 — resultados

**SOURCE_IMPLEMENTED / LAB_PASS / WINDOWS_MSVC_PASS / INDEPENDENT_DOWNLOAD_PASS / HUMAN_RUNTIME_PENDING**

## Evidencia local (WSL2, g++ 15.2, C++20, ASan+UBSan)

| Gate | Resultado | Clase |
|---|---|---|
| `cp19_lab`: 43 casos CP18 (escenarios de cadena dentro de 240) + 22 CP19 contra puertos de E_OC, Z2SceneMgr/JASWaveArc y damage_check/cc_at_check, con CP10.6 real | 65/65 | LAB |
| Mutaciones (`tests/cp19_mutations.py`, 4 políticas) | 30/30 eliminadas | LAB |
| `mod.cpp` y `cp19_abi.cpp` con g++ contra cabeceras oficiales 40457c6 | 0 errores, 0 avisos propios | COMPILADOR NO-MSVC |
| `source_gate.py` contra CP18 y 28 blobs oficiales | PASS | INSPECCIÓN |

El laboratorio no prueba audio, colisión ni animación reales.

## Windows MSVC y descarga independiente (CI)

[Run 39271517569](https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/39271517569): build 118124756747 + independent-download 118126710761 **SUCCESS** (primer intento). Fuente `c0166f0384124245352b55e279340a6530a65b47`, CI `3b0de9ea509989057de4325d7c2b34b4c55f2cee`. MSVC 14.51.36231 x64; 0 avisos printf, 0 avisos en ficheros propios.

| Gate | Resultado CI |
|---|---|
| Blobs fijados / UTF-8 / puerta de fuente vs CP18 (23 idénticos, 28 blobs oficiales) | 605 / PASS / PASS |
| CP19 / CP18 / CP17 FIX01 / CP17 / CP16 FIX01 / CP16 / CP15 / CP14 | 65 / 43 / 53 / 64 / 53 / 56 / 38 / 16 |
| CP13 runtime / map / Black / variant / damage / dynamic / visual / eyes / strokes / perf / CP12 / texture | 26 / 66 / 26 / 30 / 22 / 28 / 22 / 9 / 36 / 4 / 7 / 20 |
| Editor CP13 / CP08 / CP09 / multi / nombres | 39 / 42 / 71 / 110 / 25 |
| COFF MSVC (5 869 B, `65a9a7fe6d480030b8dc5aa5d4cc7a11ba716a7140979de7b05a089b0d43fffe`): `mCutType` 1640, `attention_info` 1592, `field_0x6cc` 2284, `mSound` 1768, `field_0xa3` +387, `Z2SceneMgr` ondas +13..+22, `mBattleDistState` +214, `BATTLE`=4 | CP19_MSVC_COFF_SOURCE_ABI_PASS |
| PE/ZIP: hook único PRE+POST; imports = CP18 + los 6 revisados; 18 negativos rechazados | CP19_PE_ABI_PASS |
| Descarga independiente + re-descarga local (bytes y SHA-256 iguales) | DOWNLOAD_VERIFIED |

Paquete `BokoblinExpansion_0.16.0_CP19_BUGFIX_TEST.dusk` 146428 B, SHA-256 `87abfb78beb824f40b6015907c45783619932b45292112809749000a9f774692`; DLL 390656 B `79f50b66636807c3d365186f99064529f4a8174735150af2789abe7c8af133cc`; ZIP 2143400 B `9745eaf43bf21955e51ff66cac9f6a4eddd83470aefb969b7524b6b659ddc8ab`. Artifact 12196235038, caduca 18-11-2026 09:46:54 America/Panama.
