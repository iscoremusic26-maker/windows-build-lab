# POST05-COMPAT — C5: conjunto de escritura, ABI e implementación compartida

Etiquetas: **[S]** fuente · **[L]** laboratorio · **[C]** CI/PE/desensamblado de NUESTRA DLL · **[P]** pendiente.

## 1. Un solo escritor, tres campos
| Comprobación | Resultado | Etiqueta |
|---|---|---|
| Funciones que escriben en el juego | **solo `restore_reaction`** (`mod.cpp`), sin cambios desde 0.5.0. Asigna `mActionMode`, `mOcState` y `mOldActionMode` a través de `r` y nada más. Se define y se llama una sola vez | [S] `source_audit` (reglas heredadas) |
| Escrituras a `AtAtp`, HP, cooldown, temporizadores, flags o bits de esfera | **ninguna**. `SetAtAtp` pasa a la lista de llamadas prohibidas (negativa S41) | [S] |
| Desensamblado de la DLL descargada (`614207b7…a951`, 65536 bytes) | **exactamente tres** escrituras al actor en esos offsets: `mov %edx,0x8c8(%rax)`, `mov %r8d,0x8d0(%rax)` y `mov %r9d,0x8cc(%rax)` (`0x18000254c–0x180002559`). **Ninguna** escritura en `0x656` (HP), `0x6cc/0x8ec` (cooldown), `0x8e0/0x8e2`, `0x69a`, `0xce8/0xd08/0xea0/0xec0` ni `0x8c0` | [C] |
| Lecturas de los bits Tg/Co | `movzbl` sobre `+0xce8/+0xea0/+0xd08/+0xec0`, como en 0.5.0 | [C] |
| Imports de `dusklight.exe` | los mismos 3 que 0.5.0 (`damage_check`, `fopAcM_SearchByID`, `dComIfGp_getStartStageName`); **0 nuevos**. Exports: los mismos 5. Metadatos: 1 `HOOK_MEM`, 4 `IMPORT` | [C] `pe-audit.json` |
| Hooks | 1 (`daE_OC_c::damage_check`), PRE siempre `HOOK_CONTINUE`. **Ningún hook nuevo**. Sin `HOOK_SKIP_ORIGINAL` | [S]+[C] |

## 2. Lo que cambió en la fuente respecto a 0.5.0
Base `c06b44e` → `ac99b4d`. Las cifras cuentan líneas `+`/`-`.

| Archivo | Líneas | Cambio |
|---|---|---|
| `hit_core.hpp` | 35 | `E_OC2_CANDIDATE`, `is_candidate`, `AdmissionPolicy` (por defecto **V1Strict**), regla V2 y `NativeAux.atp/resource` |
| `engine.hpp` | 5 | `policy` del motor; detalle para cualquier candidato |
| `lifecycle_core.hpp` | 14 | `calls_adm[5]`, versión 0.5.1, `adm(…/e_oc2_cand)` en `POST03_SUMMARY` |
| `posture_core.hpp` | 15 | postura para cualquier candidato; `unsafe_mask` (`ADMISSION` / `LOOKUP`) |
| `reaction_core.hpp` | 56 | motivos `E_OC2_SUPPRESSION_DEFERRED` (primera regla de política) y `POWER_CHANGED_IN_CALL`; contadores por recurso y R6; campos `resource/atp/admission` en `POST05_DECISION` |
| `mod.cpp` | 11 | `read_aux` lee `AtAtp` y el recurso (getters const); `g.eng.policy = V2Compat` (una vez); `do_pre` usa la política; `POST05_INIT` amplía campos |

**Lo que no cambia:**
- la política de **reacción** de E_OC (`policy_id` igual);
- `restore_reaction`;
- las precondiciones auxiliares;
- el hook;
- `POST00`–`POST04` y los documentos de 0.5.0 (diff vacío).

**Implementación compartida:** E_OC, Blue, Red y Black pasan por el **mismo** tubo (`on_pre` → POST03.1 → POST04 → `post05::process`), con el **mismo** escritor. No hay ramas por variante fuera de la admisión y de la primera regla de política.

## 3. Límite honesto: la comprobación de layout
`layout_matches_binary` compara direcciones de subobjetos, que el compilador conoce en tiempo de compilación. En 0.5.0 se verificó en el desensamblado que MSVC la plegaba a `true`. **No es una comprobación en tiempo de ejecución contra el EXE**: equivale a un `static_assert`, porque la DLL se compila contra las mismas cabeceras del SDK fijado (`40457c6a…`).

Si el EXE del usuario no fuera el 2.0.3 fijado, la protección real sería la comprobación de versión del host (`HOST_NOT_2_0_3`), no esta función. El nombre `layout_check=runtime_vs_exe` de `POST05_INIT` es, por tanto, **impreciso**: se conserva por compatibilidad del formato y queda documentado aquí. En 0.5.1 no se volvió a localizar el plegado en el desensamblado.

## 4. Pruebas
- **[L] `compat_lab`:**
  - T28/T29, gemelos con POST05 abierto o cerrado. En E_OC, todo lo que no son los 3 campos es idéntico (HP, cooldown, temporizadores, flags, bits, `AtAtp`). En E_OC2 todo es idéntico, incluidos los 3 campos;
  - estrés: ningún E_OC2 escrito nunca.
- **Mutaciones y negativas:** C01–C22 y S36–S42, todas detectadas o rechazadas.
- **[C] PE:** 11/11 negativas rechazadas.
