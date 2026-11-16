# POST05-COMPAT — C4: Bokoblin Black (normal 3, furia 6)

Etiquetas: **[R]** log B1 de 0.5.0 · **[S]** fuente (Expansion 0.17.0 en `192a773`) · **[L]** laboratorio · **[P]** pendiente.

## 1. Identidad y potencia
- **Registro DEVF exacto:** `source=DEVF_BLACK` y encuentro concreto, sobre recurso E_OC2 nativo (220→440 HP). [S] CP10.2/CP10.4; [R] B1 actor 218, encuentro `bokoblin-010`.
- **`AtAtp`:** 3/3 en estado normal; **6/6 en furia** durante 300 ticks tras **cualquier** pérdida de HP no letal, renovada con cada golpe. Vuelve a 3 al terminar la furia o al morir. [S] CP10.6.
- **B1 [R]:** `BLACK_FURY_START` 3→6 en el primer golpe (440→420), 15 `BLACK_FURY_REFRESH` y un `BLACK_FURY_END` 6→3 al morir (`TERMINAL_STATE_DEATH…`).

## 2. Por qué la supresión de Black queda DIFERIDA
| # | Hecho | Etiqueta | Riesgo si se suprimiera |
|---|---|---|---|
| B1 | **La furia se activa en el mismo cuadro del golpe, después de `damage_check`:** `dynamic_post` (POST de `execute` de Expansion) ve la pérdida de HP y escribe `AtAtp` 6/6 (`REAL_NON_LETHAL_HP_LOSS_IN_NATIVE_EXECUTE`) | [S] `mod.cpp:894-901`; [R] B1 | en vanilla el Black entra en DAMAGE y el golpe en curso se pierde. Con supresión, **el golpe en curso continuaría con potencia 6** (el doble). Nuestro hook no puede verlo: ocurre después de que retorne. `POWER_CHANGED_IN_CALL` no lo cubre |
| B2 | **IA propia de Black** (CP18/CP19): su PRE escribe cadenas de golpes, `setActionMode`, temporizadores y bloqueos; su POST cambia la velocidad de reproducción, desplaza las esferas de ataque (barridos) y aplica el *chip* de escudo | [S] `black_ai_policy.hpp`, `mod.cpp` 1405–1480 y CP19 FIX02 | su guarda `safe()` **excluye DAMAGE/BIG_DAMAGE y golpes** (`black_ai_policy.hpp:41`). Al restaurar ATTACK, la IA de Black **seguiría actuando** en el cuadro siguiente sobre un actor recién golpeado, un estado que su autor excluyó a propósito |
| B3 | **Sin línea base en ataque:** con 0.5.0, POST03 excluía E_OC2. Las 16 `BLACK_DAMAGE_RECOGNIZED` de B1 lo ven ya en DAMAGE (5/5), con 11–25 ticks entre golpes. Es la reacción vanilla, pero sin el modo PRE | [R] | no se sabe en qué fracción de los golpes estaba atacando |

**Decisión 0.5.1:**
- todo golpe confirmado sobre Black (E_OC2) da `ALLOW_VANILLA / E_OC2_SUPPRESSION_DEFERRED`;
- el Black se observa y acumula postura pasiva;
- su par 3/3 o 6/6 se admite como `E_OC2_CANDIDATE` estable, y la furia **no** cambia la admisión.

## 3. Pruebas [L]
- **T18:** Black 3/3 → furia 6/6 entre llamadas → 3/3.
  - 0 cambios de admisión;
  - 5 golpes diferidos y 0 escrituras;
  - la postura llega a 100 (latch) sin congelarse.
- **T19:** 3→6 dentro de la llamada: la política de E_OC2 manda antes (diferido, sin escritura).
- **T22:** golpe letal y otras familias, diferidos.
- **Estrés:** la furia 3↔6 se modela entre llamadas y, rara vez, dentro; ninguna escritura sobre E_OC2.
- **Mutaciones:**
  - C04: `AtAtp` 4/5 aceptado;
  - C06/C15/C16: E_OC2 sin detalle o sin postura;
  - C07/C08/C19: diferimiento quitado, debilitado o tardío;
  - C22: E_OC2 contado como E_OC.

## 4. Veredicto C4
`C4_SAFE_EXCLUSION`: Black normal y en furia son **compatibles** (observados, con postura y reacción vanilla). **No** hay supresión de Black.

Abrirla exigiría:
- una forma de neutralizar B1, es decir, que la furia no multiplique un golpe ya en curso. Requiere coordinación con Expansion, que este encargo prohíbe (no se crea ningún servicio de Expansion);
- una auditoría de la IA de Black con el modo restaurado (B2);
- logs humanos de línea base con 0.5.1.
