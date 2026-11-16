# POST05-RB 0.5.2 — supresión E_OC2 por fase nativa (implementación)

Etiquetas: **[S]** fuente exacta · **[B]** binario `dusklight.exe` 2.0.3 (SHA-256 `ffa99203…0049`, desensamblado local, no versionado) · **[L]** laboratorio · **[C]** CI/MSVC/PE · **[R]** log real · **[P]** pendiente de prueba humana.

## Qué cambia en el juego
Mismo ID `dev.posture.post05_selective_reactions`, versión **0.5.2**. Sustituye a 0.5.1 y no se carga nunca junto a ella.

- **E_OC (Blue solo y en grupo, E_OC nativo):** sin cambio semántico respecto a 0.5.1.
  - Los cinco laboratorios heredados conservan sus conteos exactos.
  - `rb_lab` compara 400 golpes E_OC con la política E_OC2 abierta y cerrada, y deciden igual.
- **E_OC2 (Red nativo, Red de Expansion, Black 3/3 y Black en furia 6/6):** las mismas reglas que E_OC (espada ordinaria no letal, ponderada, por debajo de 100, ATTACK 0/1/2, cooldown 0→10, efectos extra de `setActionMode` nulos, sin cambio de potencia ni de recurso en la llamada). Además, un **gate de fase nativa** leído en el PRE de esa misma llamada. Solo hay supresión en estas fases:

| Fase PRE | Condición exacta | Resultado |
|---|---|---|
| **B/C maduro** | ATTACK 1 con fin de clip 42 o ATTACK 2 con fin 47; `attr=0` (once); sin bit de parada; `rate` ∈ {1, 1,15, 2}; `frame` ≥ 2 y < fin | **suprimible** |
| **Frenada de ATTACK 0** | \|`speedF`\| > 1,5, así que `cLib_chaseF(&speedF, 0, 1)` no llega a 0 y en este tick no arranca el clip | **suprimible** |
| Arranque (ATTACK 0 con \|`speedF`\| ≤ 1,5) | en este tick se ejecuta `setBck` (B/C) | vanilla `E_OC2_STARTUP_TICK` |
| `frame` < 2 en B/C | tick de `frame` 1: Expansion decide el x2 en este POST | vanilla `E_OC2_FRAME_EARLY` |
| Fin de clip (`isStop`) | rama de fin nativa y cadena de furia del Black | vanilla `E_OC2_CLIP_ENDING` |
| Clip, modo, `rate` o `frame` no autenticados | escritor desconocido | vanilla `E_OC2_ANIM_UNVERIFIED` |
| ATTACK 3 (rebote por guardia) y otros modos | igual que en 0.5.1 | vanilla |
| Potencia distinta de 2, 3 y 6 (p. ej. 1 sobre E_OC2) | perfil no verificado | vanilla `E_OC2_PROFILE_DEFERRED` |
| Señales mixtas: ledger y recurso PRE/POST no son E_OC2 | — | vanilla `E_OC2_SUPPRESSION_DEFERRED` |
| Animación PRE ≠ POST dentro de la llamada | — | `SKIP_UNSAFE` `POST_AUX_UNEXPECTED` |

**Red nativo y Red de Expansion** comparten firma (E_OC2 2/2). No se intenta distinguirlos: ambos usan **la misma intersección**, segura para los dos.

## Por qué estas fases [S][L]
- **Expansion solo consulta el golpe (`pre_valid = safe(a)`, que exige `!hit`) en dos sitios del POST:**
  - **la decisión de tasa del arranque** (`red_ai_policy.hpp` :381; `black_ai_policy.hpp` equivalente): A5-1;
  - **la aplicación del x2 pendiente en `frame` 2** (:374): A5-2.
- **En un golpe ya decidido**, `post()` no vuelve a leer `pre_valid` (:378 y equivalente en Black).
- **`pre()` no emite órdenes con el golpe en curso:**
  - Red: :302, con `inblow`;
  - Black: solo actúa sobre un clip parado o en FIND;
  - con golpe, `pre()` retorna antes y solo cancela la intención (que no existe con el golpe en curso) y el contador de pasividad; vanilla hace lo mismo.
- **Por tanto, un tick B/C maduro con golpe suprimido y un tick sin golpe producen la misma salida de Expansion.**
- **La frenada de ATTACK 0 no arranca el clip en ese tick**, así que la decisión de tasa se toma en un tick posterior y limpio.

**Prueba con el código REAL de CP21** (`tests/rb_lab.cpp`, parte B; cabeceras de `192a773` copiadas byte a byte en `tests/cp21/`, con SHA-256 en `CP21_MANIFEST.json`):
- **Montaje:** golpes completos B y C de Red normal (x1,15), Red de reacción (x2), Black 3/3, Black en furia (x2) y Red nativo. Se usa el modelo nativo de `executeAttack` y de `J3DFrameCtrl::update`.
- **Variantes de estado privado** (con el mismo snapshot): no propietario, pose inválida, no registrado y `last_frame` obsoleto.
- **Comparación:** para cada tick se compara el gemelo con golpe suprimido contra el gemelo con **la misma pérdida de vida sin bandera de golpe**. Se cubren modo, estado, clip, `frame`, `rate`, escrituras de `rate` de Expansion, barrido x2 de esferas y chip de escudo.
- **Resultado:** en **todos** los ticks que el gate habilita, las trayectorias son idénticas. Los ticks donde difieren son exactamente los de arranque y de `frame` 1, que el gate excluye. Cifras en `RESULTS.json`.

## Black 3→6 [S][B][L]
- **Cuándo se escribe la furia:** Expansion escribe 6/6 en `dynamic_post`, en el POST de `execute`. Eso ocurre **después** de este `damage_check` y de `cc_set`, y **antes** de `dCcS::Move`.
- **Cuándo se consume la potencia:** en `dCcS::SetAtTgGObjInf` (`d_cc_s.cpp` :576, `GetAtAtp` → `PlusDmg`), durante `Move`. Link la aplica en su `execute` siguiente y la deduplica con `mDamageTimer` (CP14 `red_policy.hpp` :25-33).
- **Consecuencia:** el golpe que continúa contacta con 6 de forma coherente y es un único contacto. POST05 lee 3 en PRE y en POST, así que no hay `POWER_CHANGED_IN_CALL`.
- **Efecto sobre el golpe en curso:** no cambia su `rate`. No hay barrido x2 (no era un golpe x2) y **no hay chip de escudo** (`chip_step` fija la furia al empezar el golpe y da `NotFury`). Expansion ya cambia la potencia a mitad de golpe cuando la furia expira (6→3).
- **3/3 golpeado en la frenada de ATTACK 0:** la furia empieza antes del clip, así que ese golpe nace como un golpe de furia normal de Expansion. Se decide en el tick limpio de arranque, con x2 una vez en `frame` 2 y chip como máximo una vez. Lo comprueba `rb_lab` (b02). Es efecto de la vida perdida, igual que en vanilla tras el *flinch*.
- **Opción de retirada:** `kE2BlackFuryStartEnabled` (`reaction_core.hpp`) desactiva solo 3→6 sin tocar 6→6.

## ABI de las lecturas nuevas [B][C]
Solo lectura, en `read_aux` (PRE y POST de la misma llamada):

| Lectura | Offset | Evidencia en el EXE 2.0.3 |
|---|---|---|
| `mpMorf` | `+0x6d8` | `executeAttack` 0x140e06f90: `mov 0x6d8(%rcx),%rax` |
| `mFrameCtrl` en `mDoExt_morf_c` | `+0x18` | `frame` `0x2c(%rax)`, `rate` `0x28`, `state` `0x21` |
| `J3DFrameCtrl` | `attr` +8, `state` +9, `end` +0xc, `rate` +0x10, `frame` +0x14 | `J3DFrameCtrl::update` 0x1416b3410 |
| `speedF` | `+0x620` | `lea 0x620(%rsi),%rcx` → `cLib_chaseF(.., 0, 1.0)` en ATTACK 0 |

- **Comprobación en compilación:** `static_assert` en `mod.cpp`.
- **Comprobación en ejecución:** `layout_matches_binary` comprueba además, solo con aritmética de direcciones, que la base `mDoExt_morf_c` de `mpMorf` coincide.
- **Límite:** es una comprobación de cabeceras y de layout, no una lectura del EXE en ejecución.
- **Fin de clip B=42 y C=47 [R]:** `clip_end` real de las líneas `DEVF_CP17/18` de Expansion en el log del usuario (`dusklight-20261101-150643`, SHA-256 `7d103dc7…1f1d`).
- **Sin imports ni hooks nuevos:** los getters son inline y const.

## Escritor
- **Sin cambios:** `restore_reaction()` sigue escribiendo solo `mActionMode`, `mOcState` y `mOldActionMode`, con relectura previa y verificación posterior. Sin `HOOK_SKIP_ORIGINAL`.
- **Política:** `g.rx.e2_policy = MatureBC` se fija una sola vez (lo audita `source_audit`). Por defecto vale `Deferred`, que reproduce 0.5.1 en los laboratorios heredados.

## CI y binario [C]
- **Run:** https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/41362833995. `build` e `independent-download` en éxito al primer intento.
- **Origen del build:**
  - espejo `ci/posture-post05-rb-implementation` @ `19d23d8dad8839463be2eee2167d0e119a48940c`;
  - fuente `e554fd1b4201ac1b4b67790d4f1f3f99974cff20`;
  - libro de control de 40 archivos verificado por el build.
- **Compilador y labs:** MSVC 19.51.36260 x64. Los 7 laboratorios, con MSVC, dan los mismos conteos que con g++ y ASAN+UBSAN: 1660 / 29558 / 12826 / 556878 / 36933 / 55972 / 21007, todos con 0 fallos.
- **Auditorías:** `SOURCE_AUDIT_PASS`, `PE_AUDIT_PASS` y 11/11 negativas PE rechazadas.
- **Imports y exports:** los mismos 3 imports del juego que 0.5.1 (`damage_check` thunk, `dComIfGp_getStartStageName`, `fopAcM_SearchByID`) y los mismos 5 exports.
- **Artefactos:**

| | id | bytes | SHA-256 |
|---|---|---|---|
| ZIP `POST05RB_TEST_41362833995_1` | 13105797739 | 35222 | `f550a3c365d2b9416e09c8c68bdfc7b88028616cc2fa61fe6853240900964456` |
| `.dusk` `POSTURE_POST05_2_RB_PRIVATE_TEST_windows-amd64_e554fd1b4201.dusk` | — | 34128 | `f39b88278ff864f71824f7ccbea27246f1039832c22ade163f4094df57145021` |
| `mod.dll` | — | 69632 | `efc70b9dafb7ba3c2153fbcf351bab894d3df106888244e95843fc9f23602f59` |

- **Descarga independiente por ID** (API, fuera del job), recalculada:
  - ZIP y `.dusk` coinciden con el digest de GitHub, el manifiesto y `SHA256SUMS`;
  - `mod.json` es idéntico byte a byte al de la fuente.
- **Desensamblado de la DLL descargada:**
  - **Escrituras al actor:** exactamente 3 stores a sus campos de modo (`0x8c8`/`0x8d0`/`0x8cc` en 0x180006337–34a, tras la relectura `cmp`). 0 stores a HP (`0x656`), cooldown (`0x8ec`), timers (`0x8e0`/`0x8e2`), flags (`0x69a`), bits de esfera (`0xce8`/`0xd08`/`0xea0`/`0xec0`), `mName` (`0x8c0`) ni `mAtInfo`.
  - **Lecturas nuevas (`read_aux`, 0x18000a63b):** `mov 0x6d8(%rdi)` → `mpMorf`; `movzbl 0x20`, `movzbl 0x21`, `movzwl 0x24`, `mov 0x28` y `mov 0x2c` sobre `mpMorf`; `mov 0x620(%rdi)` → `speedF`.
  - **Sin escrituras por `mpMorf`:** se carga una sola vez y todos sus stores van al struct de salida.
- **Binario distinto del baseline 0.5.1 (DLL de 65536 bytes):** se esperaba por las lecturas y la política nuevas. **Nadie ha cargado la DLL.**

## Pruebas [L]
| Suite | Comprobaciones | Fallos | Nota |
|---|---|---|---|
| `hit_lab` / `lifecycle_lab` / `collider_lab` / `posture_lab` / `reaction_lab` | 1660 / 29558 / 12826 / 556878 / 36933 | 0 | conteos idénticos a 0.5.0 y 0.5.1 |
| `compat_lab` | 55972 | 0 | semántica 0.5.1 (`E2Policy::Deferred`); solo versión, motivos y campos de formato actualizados |
| **`rb_lab`** | **21007** | 0 | parte A (motor completo + gemelos) y parte B (código real de CP21) |

Cifras de `rb_lab`:
- **Parte B, gemelos con CP21:**
  - 7280 gemelos;
  - **6680 ticks elegibles idénticos** al gemelo "misma vida perdida sin golpe", 0 distintos;
  - 240 ticks no elegibles distintos: 160 de arranque y 80 de `frame` 1, que el gate excluye;
  - 360 ticks no elegibles idénticos (exclusión conservadora);
  - 1540 casos Black 3→6 con la potencia 6 escrita en el POST de Expansion (POST05 lee 3 en PRE y POST);
  - 300 ticks distintos del gemelo sin golpe; los 60 elegibles entre ellos son solo el efecto de la furia del Black, comprobado como golpe de furia bien formado.
- **Predicado independiente:** 6000 decisiones aleatorias. Toda decisión `ELIGIBLE` cumple todas las condiciones; hubo 268 elegibles, 50 de ellas E_OC2.
- **Mutaciones (`tests/run_mutations.py`):** **173/173**, sin ninguna inválida:
  - 122 lógicas detectadas, 22 de ellas nuevas (RB01–RB22);
  - 51 negativas de fuente rechazadas, 9 de ellas nuevas (S43–S51);
  - salida en `evidence/MUTATIONS_0_5_2.txt`.

## Riesgos residuales conocidos
1. **Lo no visto en pantalla:** si la animación continúa, contacta y suena bien solo lo puede confirmar la prueba humana [P].
2. **Golpes de Red que ahora conectan:** pueden activar el robo de vida de Expansion. Es un contacto legítimo, no una duplicación, pero cambia el balance respecto a vanilla.
3. **Black 3→6 suprimido:** el primer golpe que sigue lo hace con potencia 6. Es coherente (un contacto, potencia aplicada en `Move`), pero en vanilla+Expansion no existía.
4. **Helm Splitter (CP19):** Expansion enmascara el tipo de corte y aplica el daño exacto tras `damage_check`. Si eso fuese letal con la reacción suprimida, CP19 completa la muerte por su ruta de "vida 0 en modo vivo". Riesgo heredado: ya existía para Blue en 0.5.1.
5. **Atención y pasividad de Red:** el contador de pasividad y `last_threat` se reinician en el tick del golpe, igual que en vanilla. Se pierde la intención de reacción del Red golpeado en la frenada: el golpe sale x1,15 en vez de x2, igual que en vanilla.
6. **Extractor de logs:** la reconstrucción POST03↔POST04 marca golpes sin `POST04_POSTURE_HIT` cuando el presupuesto de log de POST04 se agotó (sesiones largas con latch). Los contadores de los resúmenes son la fuente fiable.
