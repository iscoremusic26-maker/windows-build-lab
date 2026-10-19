# CP17 FIX01 — resultados (18-10-2026, America/Panama)

**SOURCE_IMPLEMENTED / LAB_PASS / WINDOWS_MSVC_PASS / INDEPENDENT_DOWNLOAD_PASS / HUMAN_RUNTIME_PENDING**

## Evidencia local (Linux WSL2, g++ 15.2, C++20, ASan+UBSan; LeakSanitizer no disponible bajo ptrace)

| Gate | Resultado | Clase de evidencia |
|---|---|---|
| `cp17f1_lab` (política del DLL contra puertos de `executeFind`/`executeAttack`/`cLib_chaseF`/`setCutJumpSpeed`/física de Link y geometría sintética) | 53/53 CASE_PASS | LAB |
| Mutaciones de la política (`tests/cp17f1_mutations.py`) | 22/22 eliminadas | LAB |
| `mod.cpp` con g++ contra las cabeceras oficiales 40457c6 (defines ABI oficiales) | 0 errores, 0 avisos de formato (solo el `static_assert` PMF propio de MSVC) | INSPECCIÓN / COMPILADOR NO-MSVC |
| `source_gate.py` contra CP17 y 17 blobs oficiales | PASS: 24 ficheros runtime idénticos, fuera del bloque Red idéntico, 1 línea de reenvío CP14 | INSPECCIÓN |
| Regresiones: CP17 0.14.0 (snapshot 8fbaea7) 64, CP16 FIX01 53, CP16 56, CP15 38, CP14 16 | PASS | LAB |

Métricas del laboratorio (no del motor):

```
TIMING jump_attack start=180 forced_tick=0 first_active=8 land_tick=14 contact_xz=77 contact_feet_y=42
TIMING jump_attack start=300 forced_tick=0 first_active=8 land_tick=14 contact_xz=85 contact_feet_y=42
TIMING jump_attack start=520 forced_tick=0 first_active=8 land_tick=14 contact_xz=99 contact_feet_y=42
TIMING jump_strike start=180 forced_tick=10 first_active=18 land_tick=23 contact_xz=66 contact_feet_y=13
TIMING jump_strike start=300 forced_tick=10 first_active=18 land_tick=23 contact_xz=62 contact_feet_y=13
TIMING jump_strike start=520 forced_tick=10 first_active=18 land_tick=23 contact_xz=55 contact_feet_y=13
COVERAGE native=13272 miss_plain_odd_x2=2431 miss_plain_even_x2=1328 miss_sweep=1187 phantom_cells=2 max_phantom_depth=1.4 phantom_plain_even_pose24=230
STATE_BYTES 336 TABLE_BYTES 21504
```

Casos: raíz y arcotangente sin CRT (error ≤2,1e-4 rad) y convención de yaw s16; tasas x1.15/x2 y fase par; ventana nativa t+8..t+12; SFX completos a x2 en cualquier fase (y defecto x3 histórico reproducido); traslación conservada; propiedad/tasa ajena/retroceso/restauración/apagado; x2 pendiente invalidado; cargas en todo episodio sin tirada, reevaluación, radio, cooldown reactivo vs aleatorio, ATTACK forzado a <400/<200, bloqueos (lockout, orientación, FIND3, FIND_STAY, daño pendiente, cut-down, suelo/evento, reclutamiento, correa); aproximación ×1,5 exacta con trayectoria nativa de `speedF`, sin aproximación al retroceder/lockout/correa/velocidad negativa, preempción y escritura ajena, medición; familias de salto y negativos; Jump Attack 180–520 y Jump Strike 180–520 antes de aterrizar con pies ≤110 (referencia independiente); bypass de lockout solo para intercepción; MISSED_NOT_ENOUGH_TIME con motivo; dirigido/otro objetivo/sin objetivo; trayectoria que se aleja; golpe propio en curso; carga→Jump Strike sin doble reacción; una por episodio; prioridades; pasividad 105; contacto rancio vs registrado; escudo; daño reenviado/rancio; interrupción observada/no/desconocida; cobertura de contacto sin fantasmas; guardas del barrido; pose 23/22 en el último tic; empuñadura; telemetría en transición (predicado 0.14.0 reproducido: 1000 líneas/1000 tics); reserva crítica; ranuras/generación; no-Red/contexto; canario fallido.

## Windows MSVC y descarga independiente (CI)

[Run 39194139875](https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/39194139875): build 117873097120 + independent-download 117873928875 **SUCCESS**. Fuente compilada `77e2da4c…`, CI `76fb2a9b…`. MSVC 14.51.36231 x64, SDK 10.0.26100.0; 0 avisos printf, 0 avisos en ficheros propios (los avisos restantes vienen de cabeceras oficiales).

| Gate | Resultado CI |
|---|---|
| Blobs fijados / UTF-8 / puerta de fuente (24 idénticos, 17 blobs oficiales, 1 reenvío CP14) | 508 / PASS / PASS |
| CP17F1 lab / CP17 histórico 0.14.0 / CP16 FIX01 / CP16 / CP15 / CP14 | 53 / 64 / 53 / 56 / 38 / 16 |
| CP13 runtime / map / legacy / Black / variant / damage / dynamic / visual / eyes / strokes / perf / CP12 / texture | 26 / 66 / exit0 / 26 / 30 / 22 / 28 / 22 / 9 / 36 / 4 / 7 / 20 |
| Editor CP13 / CP08 / CP09 / multi / nombres; esquema C#→C++ | 39 / 42 / 71 / 110 / 25; PASS (estático) |
| COFF MSVC real (5 555 B, SHA-256 `157efaa6…31e7`): layout CP17 + `mTargetedActor` 14128, `mLinkAcch` 9064, `speedF` 1568/`gravity` 1572, `field_0x6c2` 2274, `mSphs_at` 4136; virtual const `checkCutJumpMode` slot 1352 | CP17F1_MSVC_COFF_SOURCE_ABI_PASS |
| PE/ABI: AMD64, un hook execute PRE+POST, imports = CP17 − `startCreatureSound` + `cM3dGSph::SetC/SetR` | CP17F1_PE_ABI_PASS |
| Negativos Expansion (x86, hook, servicio, export, símbolos no revisados ×3, ARC, path escape, versión 0.14.0) / Provider | 11 / 19 |
| Descarga independiente: ZIP, .dusk, DLL, CRC, SHA, SourceZip/ledger, PE | DOWNLOAD_VERIFIED |

| Archivo | Bytes | SHA-256 |
|---|---:|---|
| DEVF_CP17_FIX01_RED_X2_JUMP_INTERCEPT_TEST.zip (interior) | 1 709 110 | `aa37425ebd85838200567636046288ff94310d5dce1a5a7d43959781f4f27ef3` |
| BokoblinExpansion_0.14.1_CP17_FIX01_RED_X2_JUMP_INTERCEPT_TEST.dusk | 124 913 | `a11c921452e0ab5dea35567dbf22bab8e17a48ee097ce1c828225781bf5e0740` |
| DLL interna (no suelta) | 299 520 | `58691b1464da8b3054930f20a423540694ec8633f9b3b1d24d91183d222a57d3` |

Artifact 12160943797, caduca 2026-11-18T03:22:37Z = 17-11-2026 22:22:37 America/Panama. Verificación descargable 12160829537. El juego no se ejecutó: **HUMAN_RUNTIME_PENDING**.
