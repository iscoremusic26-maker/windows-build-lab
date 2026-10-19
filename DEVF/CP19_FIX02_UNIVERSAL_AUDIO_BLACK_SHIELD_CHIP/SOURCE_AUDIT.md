# CP19 FIX02 — auditoría previa (fuente oficial, exports y binario oficial)

Alcance limitado a lo que FIX02 necesita. Fuente: Dusklight `40457c6adb381928e4b5fef6ed459ed291edd5e2` (52 blobs fijados en `ci/official-source-blobs.json`, comprobados por `ci/source_gate.py`). Exports: `sdk/windows-amd64.lib` oficial (SHA-256 `4dc4a511…0ca4`). Binario: `dusklight.exe` del ZIP oficial `Dusklight-v2.0.3-win32-x86_64.zip` (56 037 777 B, SHA-256 `ac63514e…2caf`; exe 52 309 504 B, SHA-256 `ffa99203…0049`), desensamblado local solo en los puntos que la fuente no fija (inlining y offsets reales). No se ha leído ningún dato del juego (ROM/ISO/ARC/AW/BAA), partida ni AppData.

## Evidencia humana

Rama `evidence/devf-cp19-fix02-universal-audio-black-shield-chip-20261019`, commit `875a6fc833afd56a3358b68bdb42487e901f7347`, `DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/evidence/HUMAN_RUNTIME_2026-10-19/dusklight-20261019-105916.log`: 881 527 B, 4 937 líneas, SHA-256 `9ee23af1b7d7f7eb23eb5cd82e41784ba7a9cf66bd34bd5c444ac3613fbe6ffe`, blob `44bebed9ede8d0271807402fcd047c5d8a540823` (verificados al leerlo por el commit fijado; no copiado ni modificado).

Hechos usados (sin rutas ni datos personales):
- 0.16.1 arranca: `CP19_AUDIO_NOT_READY_DEFERRED` (init flag) → `AUDIO_READY`; más tarde `AUDIO_RESET_IN_PROGRESS` → `AUDIO_READY_AGAIN`.
- `FIELD_SE_WAVE_LOAD_REFUSED` ×10. En cada línea `native_mode`/`native_state` son `requestSeWave_2`/`loadedSeWave_2` = 96 = `0x60` (sala 10) y `value_a` = estado de `0x5e` = **0** (inactivo). El resumen final: `wave_loads=0 wave_load_failures=10 audio_gate=READY`. La razón `Z2SCENEMGR_LOADSEWAVE_FALSE_HEAP_OR_STATE_RETRY_AFTER_300` era una etiqueta del mod, no del motor.
- `aurora::ar Initialized 0x1800000 bytes of ARAM` (24 MiB emulados).
- 10 `BLACK_NATIVE_SHIELD_BLOCK` (inferidos por retroceso, no confirmación nativa): 7 en golpes x2 de furia, 3 en golpes x1.15. La función de chip no existía en 0.16.1.
- El usuario oyó silencio total de los Black (testimonio, no log).

## A. Audio

| Punto | Verificado | Fuente / binario |
|---|---|---|
| Familia | `daE_OC_c` sirve a los perfiles E_OC y E_OC2 (`mName`), `mSound.setEnemyName("E_oc")`, ids `Z2SE_EN_OC_*` (voz, pasos `FOOTNOTE`, ataque, golpe `CLUB/NATA_HIT`, daño, muerte, salto…) | `d_a_e_oc.cpp` |
| Bancos y grupos | Un banco de ondas tiene grupos (= ficheros `.aw` de escena); un mismo id de onda puede estar en varios grupos; los datos se duplican entre escenas | `docs/jaudio.md`, `JASBasicWaveBank.h` |
| Qué grupo lleva E_OC | `setSceneName`: F_SP121 salas 2/3/4 (con E_OC nativos) → `se_wave2 0x5e`; 7 → `0x5f`; 10 → `0x60` | `Z2SceneMgr.cpp`. **Contenido de muestras inferido** (no hay acceso a los datos del juego); lo confirma el oído humano |
| Por qué falla `loadSeWave(0x5e)` | `loadSeWave` → `getWaveArc` → `JASWaveArc::load(NULL)`: false si `mEntryNum<0`, `mStatus!=0`, `mHeap.alloc(getRootHeap(), mFileLength)` falla o `sendCmdMsg` falla. El log da estado 0 y el grupo lo carga la propia escena en 2/3/4 (archivo y entrada existen) → la causa que queda es **el `alloc` en el heap de audio ARAM**, fijo en 10 MiB (`setAramAudioBufSize(0xA00000)`) y ocupado por los grupos de la escena | `JASWaveArcLoader.cpp`, `JASHeapCtrl.cpp`, `m_Do_machine.cpp`. Conclusión por **eliminación en la fuente**; FIX02 la **mide en juego** (`AUDIO_NATIVE_REFUSAL_CAUSE_MEASURED`: hueco libre mayor del heap de audio frente al tamaño real del grupo) |
| ARAM libre fuera del audio | `mem2Size = 24 MiB`; `JKRAram`: audio 10 MiB, el resto es el heap gráfico `JKRAramHeap` (GameCube: 16 MiB en total, ~6 MiB gráficos) | `m_Do_main.cpp`, `JKRAram.cpp`, `AR.cpp` (aurora) |
| Raíz configurable sin parche | `JASWaveArcLoader::getRootHeap()` devuelve `sAramHeap` si no es nulo (dato exportado `?sAramHeap@JASWaveArcLoader@@2PEAVJASHeap@@EA`) | Fuente + EXE: `getRootHeap` = `mov rax,[0x142722790]; test; je` y `JASWaveArc::load` llama a `getRootHeap` con `heap==NULL` |
| Carga DVD→ARAM | Con dirección no nula escribe en ella sin pedir bloques (`JKRDvdAramRipper`); la reproducción lee la ARAM emulada por dirección (`DuskDsp`, `ARGetStorageAddress`) | `JKRDvdAramRipper.cpp`, `DuskDsp.cpp`, `AR.cpp` |
| Liberar es inseguro | `incWaveTable`: el primer grupo cargado se queda el handle de un id; `decWaveTable` borra los handles que apuntan al grupo borrado y no los reasigna → borrar nuestro grupo silenciaría ids compartidos con grupos de escena cargados después | `JASBasicWaveBank.cpp` |
| Escena pide el grupo ya cargado | `loadSeWave` devuelve false (`mStatus!=0`) → `loadedSeWave_2 = 0`; nunca lo borrará la escena; los sonidos siguen sirviéndose | `Z2SceneMgr::load2ndDynamicWave` |
| Servicio `audio_res` | Solo reemplaza/añade ondas desde ficheros que el mod lleve (exigiría redistribuir muestras del juego): **descartado** | `sdk/include/mods/svc/audio_res.h` |
| Exports necesarios | `JKRAram::sAramObject`, `JKRAramHeap::alloc/getFreeSize`, `JKRHeap::alloc/free` (estáticos), `JASHeap` ctor/`initRootHeap`/`getTotalFreeSize`, `JASKernel::getAramFreeSize`, `JASWaveArcLoader::sAramHeap` | `windows-amd64.lib` |
| Offsets reales | `JASHeap` mBase +0x70, mSize +0x78 (`initRootHeap`); `JKRAramBlock` mAddress +0x28, mSize +0x2c, `JKRAramHeap::mHeap` +0x60 (`allocTail`); `JKRAram::mAramHeap` +0xf8 (ctor) | EXE; la sonda MSVC debe coincidir (`ci/abi_probe.py`) |

Falta (no verificable sin datos del juego o sin el juego): el contenido exacto de las muestras de cada grupo, y que se oiga. Por eso el audio queda en **HUMAN_RUNTIME_PENDING**.

## B. Escudo, vida y destello

| Punto | Verificado | Fuente / binario |
|---|---|---|
| Bloqueo nativo | `dCcS::SetAtTgGObjInf`: si `ChkShield` acepta, `tg->OnTgShieldHit()` y **no** suma daño al stts; `at->OnAtShieldHit()` también se pone con `TgHitMark==8` (no es prueba sola). Ambos lados guardan el objeto del otro (`SetHitObj`) | `d_cc_s.cpp`, `c_cc_d.h` |
| Rama de Link | `checkDamageAction`: `if (var_r29->ChkTgShieldHit()) { setGuardSe(...) ... }` sin daño corporal | `d_a_alink_damage.inc` |
| Bits persistentes | `cCcS::ChkAtTg` limpia solo los objetos registrados en esa pasada | `c_cc_s.cpp` |
| Ruta nativa de daño | `setDamagePoint`: `damageMagnification` (multiplicador de Dusklight, x1.5 por flag, Zora x10, lobo x2), Armadura Mágica → rupias, `DOUBLE_DEFENSE` /2, `mDamageTimer` + color → **no se usa** | `d_a_alink_damage.inc` |
| Ruta de vida usada | `dComIfGp_setItemLifeCount(f, type)` suma a `mItemLifeCount`; `dMeter2` lo aplica: `life + pending`, recorta a [0, máx], limpia | `d_com_inf_game.h`, `d_meter2.cpp` |
| Muerte | `checkDeadHP`: vida 0 sin hada | `d_a_alink_demo.inc` |
| Destello nativo | `draw()`: con `mDamageTimer`, `TevKColor = R*|sin(t*0x800)|` (R 15/10/5 según t>32/>16, `t = mDamageTimer + mDamageColorTime`), salvo `checkMagicArmorNoDamage()`; se pone a 0 antes de espada/vaina/escudo/objetos | `d_a_alink.cpp`, `d_a_alink_HIO_data.inc` (mInvincibleTime 30) |
| `modelDraw` | `setLightTevColorType_MAJI(model, &tevStr)` → `setTevKColor(0, TevKColor)` por material | `d_a_alink.cpp`, `d_kankyo.cpp` |
| Hook real | `draw()` llama a `modelDraw` por su stub ILT (25 llamadas, no inline); `modelDraw` usa `lea r8,[rcx+0x1f0]` (tevStr); `draw()` pone `TevKColor` a 0 en `this+0x550/0x552` tras `modelDraw` de `[this+0x770,0x788,0x810,0x820,0x780,0x7d8,0x7e0|0x778]` = cuerpo, mano, demo HL/HR, sombrero, cara FC blend/tongue o cara | EXE |
| Exports | `dComIfGp_setItemLifeCount`, `dComIfGp_getItemLifeCountType`, `daAlink_c::modelDraw` | `windows-amd64.lib` |

Falta (solo verificable en juego): que el destello se vea con las tres armaduras y que la guardia continúe. **HUMAN_RUNTIME_PENDING**.
