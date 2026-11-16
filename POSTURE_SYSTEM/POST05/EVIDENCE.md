# POST05 — Evidencia

Fecha: 2026-11-01. Local: WSL2/Linux; el juego **no** se ejecutó y la DLL **no** se cargó. Etiquetas: **[R]** juego real · **[S]** fuente · **[B]** binario · **[C]** MSVC/PE/CI · **[L]** laboratorio · **[P]** pendiente.

## 1. Etiquetas
| Etiqueta | Estado | Respaldo |
|---|---|---|
| GATE 05-R (investigación fuente + binario + estrategias) | **completo** [S][B] | `POST05_NATIVE_REACTION_AUDIT.md` |
| GATE 05-S (viabilidad de la transición selectiva) | **viable en fuente y binario** | §3–4 de la auditoría; `POST05_WRITE_EFFECTS_AUDIT.md` |
| GATE 05-L (núcleo, mutaciones, pasividad, ABI) | **sí** [L] | §3 |
| GATE 05-C (CI MSVC, PE, artefacto descargado) | **sí** [C] | §4 |
| `POST05_SOURCE_IMPLEMENTED_WITH_GATES` | **sí** (fuente + CI + PE + descarga; **sin** runtime) | |
| GATE 05-RUNTIME / `G2` | **PENDIENTE** | nadie ha cargado POST05 |
| `POST05_RUNTIME_PASS`, `G2_PASS`, `UNIVERSAL_COMPATIBILITY_PASS`, `PERFORMANCE_PASS`, `BALANCE_APPROVED` | **no** | |
| POST04 | `POST04_RUNTIME_PASS_SCOPED` | `POST04/RUNTIME_EVIDENCE_A1_B1.md` |
| G3, POST06–POST08 | `NOT_STARTED` | |

## 2. Procedencia
- Base: cierre documental de POST04 `01b0b7f96ebf6f9ea4b1014d8a84112c1857fd54` (rama `feature/posture-post04-numeric-core`). El binario de POST04 (`ccf0f2d`) queda congelado.
- Rama de POST05: `feature/posture-post05-selective-reactions`.
  - Primer commit: `34e8889`, notas y borrador sin compilar, conservado en la historia.
- `POST00`–`POST04`: sin cambios desde `01b0b7f`.
- **Diferencias de código respecto a POST04** (líneas `<`/`>` de `diff`):
  - `hit_core.hpp` 11 (struct `NativeAux`);
  - `engine.hpp` 5 (transporte de `aux`);
  - `lifecycle_core.hpp` 10 (campos `aux`, versión);
  - `posture_core.hpp` 55 (`ConsumeOutcome`, `ever_frozen`);
  - `mod.cpp` 107 (lecturas aux, comprobación de layout, `restore_reaction`, logs).
- **Nuevos:** `reaction_core.hpp` (299 líneas) y `reaction_lab.cpp`.
- **Labs heredados:** 0 líneas cambiadas, salvo `collider_lab.cpp` (2, literal de versión).

## 3. Laboratorios y mutaciones [L] (g++ ASAN+UBSAN; MSVC en §4)
| Laboratorio | Comprobaciones | Fallos | Nota |
|---|---|---|---|
| `hit_lab` | 1660 | 0 | heredado, mismo conteo que POST04 |
| `lifecycle_lab` | 29558 | 0 | heredado, mismo conteo |
| `collider_lab` | 12826 | 0 | heredado, mismo conteo; una aserción de **literal de versión** pasa de `v=0.4.0` a `v=0.5.0` (ninguna cifra cambia) |
| `posture_lab` | 556878 | 0 | heredado, mismo conteo; la muestra del formateador se regeneró por el cambio de versión |
| `reaction_lab` | **36933** | 0 | nuevo |

- **`reaction_lab`:** cubre T01–T26 del encargo.
  - T23 repite los **27 informes reales** de POST04 A1/B1 con los ticks reales entre golpes, lo que reproduce la recuperación de 7 unidades de 505/18. POST04 da exactamente el mismo resultado (12 ponderados en B1) y la única decisión elegible es la que predice un generador Python independiente: **B1 evento 3** (162/9, ATTACK estado 0, 50→75). Supuestos: `cd=0>10`, verificado en los bytes reales de los 24 golpes, y precondiciones auxiliares cumplidas (POST04 no las registraba).
  - T25/T26: estrés con 6 semillas fijas × 15000 cuadros y 36 364 comparaciones contra un oráculo que re-deriva la elegibilidad del modelo. Resultado: 274 supresiones (estados 0/1/2: 85/105/84), 35 988 vanilla (323 golpes de umbral) y 102 inseguras, sin discrepancias. El propio laboratorio exige esa cobertura mínima.
  - T16/T20 son gemelos (POST05 abierto y cerrado): los campos modelados distintos de los tres de modo, y las cifras de POST03/POST04, son idénticos; el original corre exactamente una vez por llamada.
- **Mutaciones: 113 de 113 detectadas.**
  - 25 heredadas de lógica, 28 de postura, **25 de reacción** (BIG_DAMAGE, letal o umbral suprimibles; cualquier familia o modo; ATTACK 3; sin cada precondición auxiliar; plan que no restaura; sin re-comprobación o verificación; sin guarda de duplicados; token no validado; etc.).
  - 35 negativas de fuente, **8 nuevas de escritura**: escritura extra en el escritor, escritura fuera de él, alias de puntero, `setActionMode`, getter derivado, segunda llamada al escritor, cooldown e incremento por puntero.
  - Una mutación heredada (N06) se detecta por tiempo límite.
- **Hallazgos propios, corregidos antes de cerrar:**
  - un test reutilizaba un actor y cruzaba el umbral;
  - la repetición real no avanzaba el reloj;
  - la cobertura del estrés era insuficiente (9 casos elegibles);
  - una regla de auditoría daba un falso positivo con una lambda.

## 4. CI Windows, PE y artefacto [C]
- **Toolchain:** MSVC 19.51.36260 x64 (VCTools 14.51.36231, VS 18.0), Windows SDK 10.0.26100.0. SDK oficial `40457c6a…`, Aurora `3227d76c…`, ZIP e import lib fijados por SHA-256. Compilan **todos** los `static_assert`, incluidos los nuevos:
  - `field_0x6c0/6c2` en `+0x8e0/+0x8e2`;
  - `fopEn_enemy_c::mFlags` en `+0x69a` y `HeadLock=0x80`;
  - `sizeof(dCcD_Sph)=0x1b8`;
  - Tg/Co polimórficos.
- **Run `41337085240`:** primer y único run de POST05, en éxito. Compila `a9627a2373be3c61e744f875f65f4237d42e6121`; espejo `84f0c5e2d65a129d65789e69c18c564c56ce1958`. El libro de control (30 archivos) es idéntico byte a byte al commit fuente y a la rama de CI (verificado de forma independiente).
- **En el runner Windows:** `tool-tests` OK (incluye que `reaction_trace.inc` está al día); `source-audit` PASS. Los 5 laboratorios con MSVC dan 1660 / 29558 / 12826 / 556878 / **36933**, 0 fallos, y la misma cobertura de estrés que con g++. La muestra del formateador es idéntica entre MSVC y g++. `PE_AUDIT_PASS` y **11/11 negativas PE** rechazadas.
- **PE:** AMD64; los mismos 5 exports; 1 `HOOK_MEM` enlazado a `?damage_check@daE_OC_c@@QEAAXXZ`. Imports de `dusklight.exe` **idénticos a POST04** (3) y CRT idéntico a POST04: **0 imports nuevos**.
- **Desensamblado de NUESTRA DLL [C]:**
  - existen **exactamente tres** escrituras a memoria del actor en los offsets relevantes: `mov %edx,0x8c8(%rax)`, `mov %r8d,0x8d0(%rax)` y `mov %r9d,0x8cc(%rax)`, es decir, `restore_reaction`;
  - **ninguna** escritura a HP (`0x656`), cooldown (`0x8ec`), temporizadores (`0x8e0/0x8e2`), flags (`0x69a`) ni bits de esfera;
  - las lecturas de `ChkTgSet/ChkCoSet` se compilaron exactamente sobre `+0xce8/+0xd08/+0xea0/+0xec0`, las mismas direcciones que escribe `setActionMode` en el EXE;
  - MSVC plegó `layout_matches_binary` a **verdadero** en tiempo de compilación (`movl $0x1` sobre el indicador de layout), así que la supresión no queda cerrada por un layout distinto.
- **Mi verificación por segunda ruta:** artefacto `POST05_TEST_41337085240_1`, id **13095946275**, bajado por la API por id. El ZIP (32325 bytes) coincide con el digest publicado `sha256:a4735061055c523526e380a6e9b1d7f7f3532802294e42bc5083e8b2958ea9ad`.
  - `.dusk` `POSTURE_POST05_SELECTIVE_REACTIONS_windows-amd64_a9627a2373be.dusk`: 31313 bytes, SHA-256 `f6c1a3c642761aba92d23ecf2100b124c018befed498e10b463cbad93d0db068`.
  - `mod.json` idéntico al del commit fuente; DLL 65024 bytes, `5a0a297452195280eccc08ba678d910be41cae00d7f68f0e19ebafcad0fd164f`, igual al manifiesto.
  - El manifiesto declara `gameplay_changes: true`, `game_writes` y `runtime_tested: false`.
  - PE local: PASS; 11/11 negativas.
  - Caduca el 2026-12-01 18:33 UTC. Artefactos de texto y de verificación de descarga del mismo run.
- **No se hizo:** instalar ni abrir Dusklight; cargar la DLL; PR, merge, tag ni release.

## 5. Límites
- **Ninguna evidencia de juego de POST05.** La continuidad de la animación, el audio, la IA, el doble golpe a Link y la interacción con Expansion 0.17.0 / EVF 0.15.0 (cuya fuente exacta no estaba disponible) son **[P]**.
- La voz de daño no suena en los golpes suprimidos; el SE del impacto sí.
- Blue no se identifica: toda generación que haya pasado por `UNKNOWN_UNSAFE` queda fuera de la supresión.
- D1–D5 siguen abiertas; G3 y POST06 no se han iniciado.
