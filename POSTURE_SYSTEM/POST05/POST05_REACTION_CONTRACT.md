# POST05 — Contrato de decisión, contabilidad y registro

Etiquetas: **[S]** fuente · **[L]** laboratorio · **[P]** pendiente de juego. Motor nativo: `POST05_NATIVE_REACTION_AUDIT.md`. Escrituras: `POST05_WRITE_EFFECTS_AUDIT.md`.
La política de postura de POST04 **no cambia**: SWORD +25, IRON_BALL +50, resto +0, retardo 120 ticks, 1 unidad cada 6, latch a 100. Es provisional; el balance lo decide POST09. POST05 no depende del HP ni de HM.

## 1. Entrada y salida (núcleo puro `src/reaction_core.hpp`)
- **Entrada por valor (`DecisionInput`):**
  - el `HitReport` de POST03.1 de **esta** llamada: token `event, slot, generation, id`; `kind`, `eligible`, familia y lectura AT; letalidad; *snapshots* PRE/POST de modo, estado, `old_mode` y cooldown; lecturas auxiliares PRE/POST;
  - el `ConsumeOutcome` de POST04 para ese mismo evento: aplicado, antes/después, cruce, latch, congelado, muerto, `ever_frozen`;
  - la vista del ledger para el slot: válido, admisión y vida.
  
  No hay ningún puntero a actor.
- **Salida:** una única `Decision` por evento nuevo (los duplicados se cuentan aparte y no deciden) y un `RestorePlan` con los valores PRE a restaurar y los valores que se esperan antes de escribir.
- `apply()` relee, escribe **solo** a través del escritor que le pasa `mod.cpp` (`restore_reaction`) y verifica.
- **Significado de las salidas:** `SUPPRESS_ELIGIBLE` = permiso para *intentar*. `SUPPRESSION_APPLIED` = los tres campos quedaron restaurados en memoria. **Ninguna de las dos equivale a «el Bokoblin siguió atacando en pantalla»**; eso solo lo establece la observación humana.

## 2. Motivos (orden de evaluación; cada uno pertenece a una sola decisión)
| Decisión | Motivos |
|---|---|
| `NOT_APPLICABLE` | `NOT_CONFIRMED`: Finish, firma no coincidente, no procesado |
| `ALLOW_VANILLA` (fuera de política) | `SUPPRESSION_DISABLED`, `FAMILY_NOT_SWORD`, `LETHAL`, `BIG_DAMAGE`, `POST_MODE_NOT_DAMAGE`, `COOLDOWN_NOT_ORDINARY` (salto cancelado = 3, AT_TYPE_UNK = 20), `NOT_WEIGHTED` (POST04 no aplicó: latched, peso 0, congelado…), `CROSSES_THRESHOLD` (el golpe que llega a 100 queda nativo), `LATCHED_AFTER`, `PRE_MODE_NOT_ATTACK`, `PRE_STATE_NOT_WHITELISTED` (ATTACK 3 incluido), `POST_STATE_NOT_ORDINARY` |
| `SKIP_UNSAFE` (cualquier duda) | `SESSION_DISABLED` (fallo previo o layout ≠ EXE), `TOKEN_MISMATCH`, `NOT_CANDIDATE_ACTIVE`, `ADMISSION_EVER_UNSTABLE`, `FROZEN_OR_DEAD`, `POST_SIGNATURE_MISMATCH`, `PRE_NOT_FRESH`, `COLLIDER_NOT_FRESH`, `AUX_MISSING`, `PRE_TIMERS_NONZERO`, `PRE_HEAD_LOCK`, `PRE_SET_BITS_OFF`, `POST_AUX_UNEXPECTED` |
| `SUPPRESS_ELIGIBLE` | `ELIGIBLE` |

Matriz resultante:
- **Se suprime solo:** SWORD + `HIT_CONFIRMED` + no letal + DAMAGE ordinario (estado 0–2, cooldown 0→10) + POST04 aplicó +25 y el resultado queda por debajo de 100 + PRE en ATTACK 0/1/2 + admisión estable + layout = EXE + los efectos extra de `setActionMode` fueron no-ops.
- **Todo lo demás es vanilla:** BIG_DAMAGE, muerte, Finish, el golpe de ruptura, los golpes tras el latch, escudo, flecha, bomba, hierro, bumerán, tirachinas, AT40, OTHER_VERIFIED, AMBIGUOUS, UNAVAILABLE, E_OC2, UNKNOWN_UNSAFE y cualquier modo PRE distinto de ATTACK 0/1/2.

## 3. Contabilidad (`POST05_SUMMARY`, `POST05_CONSERVATION`)
- **Contadores:** `decisions duplicates not_applicable allow_vanilla skip_unsafe eligible confirmed attempted applied recheck_failed verify_failed not_attempted applied_by_pre_state(0/1/2) session_disabled reasons=…` (27 valores, en el orden del enum).
- **R1** `decisions = not_applicable + allow_vanilla + skip_unsafe + eligible`.
- **R2** la suma de los motivos de cada categoría es igual a la categoría.
- **R3** `eligible = attempted + not_attempted`.
- **R4** `attempted = applied + recheck_failed + verify_failed`.
- **R5** `attempted ≤ eligible ≤ confirmed ≤ decisions` y `Σ applied_by_pre_state = applied`.

Siguen vigentes, sin cambios, `POST03_CONSERVATION` (incluido `pending=0`) y `POST04_CONSERVATION` (E1–E5 e invariantes en 0). El laboratorio comprueba con gemelos (POST05 abierto y cerrado) que POST03 y POST04 dan las mismas cifras.

## 4. Líneas de log
| Línea | Cuándo |
|---|---|
| `POST05_INIT version source_commit policy_id writes_to_game=1 write_set=… trigger=… eligible=… pre=ATTACK_0_1_2 cd=0>10 hook_skip_original=0 hp_untouched=1 layout_check=runtime_vs_exe` | arranque |
| `POST05_DECISION id generation event family decision reason pre_mode pre_state post_mode post_state cd posture action` | cada `HIT_CONFIRMED` (tope 300, dentro del presupuesto de golpes del observador) |
| `POST05_SUMMARY`, `POST05_LAYOUT`, `POST05_CONSERVATION` | con cada resumen (`PERIODIC`/`SHUTDOWN`) |

`action` vale `SUPPRESSION_APPLIED`, `RECHECK_FAILED_NO_WRITE`, `VERIFY_FAILED` o `NONE`. El techo global de 2000 líneas por sesión no cambia. Ninguna línea lleva punteros, rutas ni nombres.

## 5. Identificación
- **Módulo:** `dev.posture.post05_selective_reactions` v0.5.0. Reemplaza a POST04, 03.1, 03, 02 y 01, que nunca deben cargarse juntos.
- **Lo que no cambia:** las líneas `POST03_*`, `POST03_1_*` y `POST04_*` conservan su formato; solo cambia el literal de versión `v=0.5.0`.
- **`POST04_INIT`:** declara `posture_core_passive=1` en lugar de `writes_to_game=0`, porque el módulo POST05 sí escribe.
