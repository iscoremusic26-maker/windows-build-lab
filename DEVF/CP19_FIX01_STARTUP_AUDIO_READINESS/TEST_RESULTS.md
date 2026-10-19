# CP19 FIX01 — resultados

**SOURCE_IMPLEMENTED / LAB_PASS / WINDOWS_MSVC_PASS / INDEPENDENT_DOWNLOAD_PASS / HUMAN_BOOT_RUNTIME_PENDING / HUMAN_A-G_PENDING**

El juego no se ha ejecutado: que 0.16.1 arranque sin crash solo lo puede confirmar la prueba humana.

## Local (WSL2, g++ 15.2, C++20, ASan+UBSan)

| Gate | Resultado | Clase |
|---|---|---|
| `cp19f1_lab`: 65 casos CP19 + 8 de arranque (motor falso con el ciclo de vida de la fuente y contadores de llamadas; incluye el orden de 0.16.0, que sí toca el audio sin heap) | 73/73 | LAB |
| Mutaciones (30 CP19 + 11 de la puerta) | 41/41 eliminadas | LAB |
| `mod.cpp` y `cp19f1_abi.cpp` con g++ contra cabeceras oficiales | 0 errores | COMPILADOR NO-MSVC |
| `source_gate.py` contra CP19 y 33 blobs oficiales | PASS | INSPECCIÓN |

## Windows MSVC y descarga independiente (CI)

[Run 39277191308](https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/39277191308): build 118144532016 + independent-download 118146654428 **SUCCESS** (primer intento). Fuente `5c8ce0025a63f7c6a130a407609b2ad353998eb0`, CI `cc6e65cdb387db719517b93750b3952122f6a4fc`. MSVC 14.51.36231 x64; 0 avisos printf, 0 avisos en ficheros propios.

| Gate | Resultado CI |
|---|---|
| Blobs fijados / UTF-8 / puerta de fuente vs CP19 (26 idénticos, política solo añadidos, audio solo en el adaptador con puerta, 33 blobs oficiales) | 654 / PASS / PASS |
| CP19 FIX01 / CP19 / CP18 / CP17 FIX01 / CP17 / CP16 FIX01 / CP16 / CP15 / CP14 | 73 / 65 / 43 / 53 / 64 / 53 / 56 / 38 / 16 |
| CP13 runtime / map / Black / variant / damage / dynamic / visual / eyes / strokes / perf / CP12 / texture | 26 / 66 / 26 / 30 / 22 / 28 / 22 / 9 / 36 / 4 / 7 / 20 |
| Editor CP13 / CP08 / CP09 / multi / nombres | 39 / 42 / 71 / 110 / 25 |
| COFF MSVC (5 988 B, `f2f79a1bfe58c6c2ba8d2ee5c5bfb919ab946bf5db36b990752d0e015d416af1`): `mDoRstData::mShutdown` 16, `mReturnToMenu` 17, tamaño 24; `Z2AudioMgr::mResettingFlag` 1520 (0x5f0); `mDoAud_zelAudio_c` 5768 B | CP19F1_MSVC_COFF_SOURCE_ABI_PASS |
| PE/ZIP: hook único PRE+POST; imports = CP19 + los 4 revisados (33 en total); 21 negativos rechazados (incluye los 3 símbolos nuevos sin revisar y los manifiestos 0.16.0 y 0.15.0) | CP19F1_PE_ABI_PASS |
| Descarga independiente + re-descarga local (bytes y SHA-256 iguales) | DOWNLOAD_VERIFIED |

## Puerta demostrada en la DLL compilada (inspección binaria local)

`evidence/dll_gate_dominance.py` sobre `objdump -d -M intel` de la DLL del artifact: grafo de control por función, dominadores, y para cada llamada a los thunks de `getWaveLoadStatus`/`loadSeWave`/`eraseSeWave` y cada lectura del IAT de `Z2AudioMgr::mAudioMgrPtr` exige que la domine la rama OK de los 8 bytes de `AudioFlags` con su polaridad.

| DLL | Resultado | Detalle |
|---|---|---|
| 0.16.1 `adf6dbb2…19c7` | `ALL_AUDIO_CALLS_DOMINATED_BY_GATE_READY_BRANCH` | `audio_pass` 0x47f0: `call flags` (0x1ca90) en 0x4821; cada fallo pone `dil` = 1..7, `xor dil,dil` solo en 0x487a (tras las 8 comprobaciones); `test dil,dil; je 0x4908` en 0x48fa; `getWaveLoadStatus` 0x49c1, `eraseSeWave` 0x4b6c, `loadSeWave` 0x4c06 y las lecturas del gestor 0x4908/0x4b4d/0x4be7/0x4c96 dominadas por 0x4908. `audio_shutdown` 0x4d90: 0x4e99, 0x4ed2, 0x4e2b, 0x4eb5 dominadas directamente por las 8 ramas OK. No hay otras referencias a esos thunks ni a ese IAT en la DLL |
| 0.16.1 `flags()` 0x1ca90 | solo lecturas exportadas | lee `mAudioMgrPtr`, `mInitFlag`, `g_mDoAud_audioHeap`, `mResetFlag`; `[gestor+0x5f0]` (`mResettingFlag`) solo si gestor, init y heap son no nulos; `mResetData+0x10/+0x11` solo si no es nulo. Los offsets coinciden con la sonda MSVC |
| 0.16.0 `79f50b66…33cc` (control negativo) | `GATING_NOT_PROVEN` | `cp19_poll` 0x14440 llama a `getWaveLoadStatus` en 0x14525 (retorno 0x1452a = trama #01 del crash) sin ninguna comprobación de disponibilidad |

Salidas: `evidence/dll_gate_dominance_0.16.1.txt`, `evidence/dll_gate_dominance_0.16.0_negative_control.txt`.

## Paquete

`BokoblinExpansion_0.16.1_CP19_FIX01_STARTUP_AUDIO_READINESS_TEST.dusk` 147 628 B, SHA-256 `8b93fabd3ba088f10c6dcc8b2cf1100656df10f019726fcb61a99378c3ac28d9`; DLL 392 192 B `adf6dbb2ff57783f706eaae074faa23c041cacc304a8d28326c2641df89e19c7`; ZIP 2 361 255 B `6edb0da1366b9abf932aeb5aab17f88d3ea8e01780c99d0e73cdfaa26d1f427c`. Artifact 12197643964, caduca 18-11-2026 10:25:29 America/Panama.
