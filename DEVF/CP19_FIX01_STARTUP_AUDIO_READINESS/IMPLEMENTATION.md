# CP19 FIX01 — arranque sin crash: puerta de disponibilidad del audio (Expansion 0.16.1)

Base: CP19 0.16.0, fuente compilada `c0166f0384124245352b55e279340a6530a65b47` (punta documental `ec6f76d`), run 39271517569. Dusklight `40457c6`. **0.16.0 crashea al arrancar: no usar.**

## Evidencia del crash

Rama `evidence/devf-cp19-fix01-startup-crash-20261019-095456`, commit `991017852155ede4db16ce2742a408c4c86dad51`, `DEVF/CP19_FIX01_STARTUP_CRASH/evidence/HUMAN_RUNTIME_CRASH_2026-10-19/dusklight-20261019-095456.log`: 11 585 B, 101 líneas, SHA-256 `a67a661fde28bc16a15bfc38e86bae29fed2c5d78b171ccffa909faffb46d6f3` (verificado al leerlo por el commit fijado; no copiado).

Hechos del log: Expansion 0.16.0 y Provider 0.13.0 cargados (también un mod de terceros, Zora Swim 1.3.0, sin relación con la traza); la Expansion acepta los encuentros F_SP121 7/10 y el log se corta; `EXCEPTION_ACCESS_VIOLATION`, dirección `0x1a0`; #00 `dusklight.exe+0xA9773B`, #01 `mod.dll+0x1452A`, #02 `mod.dll+0x300CA`.

Verificación independiente en la DLL exacta del artifact 12196235038 (`mod.dll` 390 656 B, SHA-256 `79f50b66…33cc`, base 0x180000000), por desensamblado:
- `0x1452A` es el retorno de `call 0x301AB`; el thunk `0x301AB` es `jmp [0x33250]` y el IAT `0x33250` es `?getWaveLoadStatus@Z2SceneMgr@@QEAAHII@Z`. Argumentos: `rcx = mAudioMgrPtr + 0x550` (subobjeto `Z2SceneMgr`), `edx = 0x5e`, `r8d = 0`. Es `sm->getWaveLoadStatus(devf_cp19::kOcWave, 0)` de `cp19_poll()`.
- `0x300CA` es el retorno de `call 0x14440` en la secuencia `black_poll, regen_poll, recruit_poll, red_ai_poll, black_ai_poll, cp19_poll, red_watch` de `mod_update`.

## Causa (fuente oficial 40457c6)

- `static mDoAud_zelAudio_c g_mDoAud_zelAudio;` → el constructor de `Z2AudioMgr` hace `mAudioMgrPtr = this` al iniciar el proceso. El único chequeo de 0.16.0 (`if(!m)return;`) nunca protegía nada.
- El `JAUSectionHeap` (y con él toda la tabla de bancos de ondas) lo crea `Z2AudioMgr::init` (`JAUNewSectionHeap`), llamado por `mDoAud_Create()` desde `mDoAud_Execute()` solo cuando los archivos de audio terminaron de cargarse; justo después `onInitFlag()`.
- `Z2SceneMgr::getWaveLoadStatus` hace `JASGlobalInstance<JAUSectionHeap>::getInstance()` (`sInstance`, nulo antes de init; `JUT_ASSERT` no protege en release) y luego `sectionHeap->getWaveBankTable()`. Una dirección `0x1a0` es coherente con un campo de un puntero nulo. **Hipótesis fuerte, no certificada:** sin volcado de memoria no se puede afirmar el campo exacto.

## Corrección

`cp19_policy.hpp` (solo añadidos):
- `audio_gate(AudioFlags)`: `Ready` solo si existe el gestor, `mDoAud_zelAudio_c::mInitFlag` está activo (después de `Z2AudioMgr::init`), `g_mDoAud_audioHeap` no es nulo (`mDoAud_Create` activa el flag aunque falle la reserva del heap), no hay reset de audio (`mResetFlag`, `isResetting()`), y `mDoRst::mResetData` existe sin `isShutdown()` ni `isReturnToMenu()` (las mismas condiciones con las que `mDoAud_Execute` decide crear el audio).
- `audio_pass<Io>`: la pasada completa (ondas de escena, `getWaveLoadStatus`, `loadSeWave`, `eraseSeWave`, lectura BGM y lista de enemigos) va **después** del `return` de la puerta. Una línea por transición: `CP19_AUDIO_NOT_READY_DEFERRED reason=…`, `AUDIO_READY`, `AUDIO_READY_AGAIN` (máx. 24 por sesión).
- `audio_shutdown<Io>`: idempotente; borra la onda propia solo con audio listo, estado 2 y sin que la escena la pida; si no, `FIELD_SE_WAVE_SHUTDOWN_RELEASE_SKIPPED` y solo se olvida.

`mod.cpp`: `cp19_poll()` y `cp19_shutdown()` pasan a ser un adaptador `Cp19AudioIo`. `flags()` lee solo datos exportados (no desreferencia nada detrás de `Z2AudioMgr`; `isResetting()` solo tras `init` y heap); el resto de miembros son los únicos lugares del mod que tocan objetos de audio. Sin espera fija de N fotogramas, sin try/catch ni SEH.

Imports nuevos (presentes en `windows-amd64.lib` oficial): `?mInitFlag@mDoAud_zelAudio_c@@2EA`, `?mResetFlag@mDoAud_zelAudio_c@@2EA`, `?g_mDoAud_audioHeap@@3PEAVJKRSolidHeap@@EA`, `?mResetData@mDoRst@@2PEAUmDoRstData@@EA`.

## Comprobación en la DLL compilada

La puerta se verificó también sobre la DLL del artifact 12197643964 (SHA-256 `adf6dbb2…19c7`) con `evidence/dll_gate_dominance.py` (dominadores en el grafo de control): las 5 llamadas a funciones de onda y las 6 lecturas del gestor de audio quedan dominadas por la rama "las 8 condiciones OK". El mismo verificador falla sobre la DLL 0.16.0 en la llamada 0x14525, la del crash. Detalle en TEST_RESULTS.

## Preservación

26 ficheros runtime idénticos byte a byte a CP19 (incluidas las políticas Red/Black/reclutamiento). Las funciones A–G no cambian: A conserva exactamente su lógica de onda (solo se ejecuta cuando el audio está listo); C conserva su diagnóstico de solo lectura. Escrituras sobre actores y Link idénticas a CP19. `source_gate.py` lo comprueba y además exige que todas las llamadas de audio estén dentro del adaptador y que, en la política, ninguna llamada `io.*` preceda a la puerta.

## Límites

- La puerta usa los mismos datos con los que el motor decide crear su audio; no hay una función exportada "heap listo". No se ha ejecutado el juego: el arranque sin crash queda **HUMAN_BOOT_RUNTIME_PENDING**.
- El mod de terceros Zora Swim estaba instalado; no aparece en la traza.
