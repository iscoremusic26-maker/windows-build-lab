# POST05-COMPAT — Comparativa runtime A1/B1 de POST05 0.5.0

Fecha: 2026-11-01. Etiquetas: **[R]** log real del juego (sesión humana) · **[S]** fuente · **[B]** binario · **[C]** CI/PE · **[L]** laboratorio · **[P]** pendiente.
Esta comparativa **no** declara `G2_PASS` ni `ALL_BOKOBLINS_RUNTIME_PASS`. Un log solo prueba lo que el módulo escribió en memoria y lo que registró; la continuidad visual del ataque es observación humana.

## 1. Fuentes (logs crudos NO versionados)
| Sesión | Archivo | Bytes | Líneas | SHA-256 | Módulo |
|---|---|---|---|---|---|
| A1 (POST05 + Zora Swim, 2/2) | `dusklight-20261101-134809.log` | 243083 | 2878 | `e58221c8d0783cdf6898c6dd8315cec3172d173a31ae3a2d12d739370478d291` | `dev.posture.post05_selective_reactions` 0.5.0 |
| B1 (8/8 mods) | `dusklight-20261101-134505.log` | 525010 | 2979 | `7be6645afe66638f288db84f24c46db7d821192126c0560250897c246e8a595f` | ídem |

- **B1, mods activos:** POST05 0.5.0, Bokoblin Expansion 0.17.0, EVF 0.15.0 (`dev.devf.cp06_native_config_dual`), HM CP11, Bars CP06, Zora Swim 1.3.0, Luau 1.0.0 y Randomizer 1.0.6.
- **Lectura:** autorizada por el usuario solo para estos dos archivos. Se leyeron copias temporales fuera de los repositorios, que se borran al terminar.
- **Derivados saneados** en `evidence/`:
  - `POST05_A1_B1_EVENTS_SANITIZED.tsv`: `7ae6ea8158bb3c02f4580bf9a805a44bbd80dd75e73f3bf5687e9b99cacfb7f8`;
  - `POST05_A1_B1_SUMMARY_SANITIZED.json`: `5b882585f2c790c19bbef043294afc13f2213c021140c07e3e8ddb76655ed4c6`;
  - `POST05_A1_B1_SOURCE_MANIFEST.json`: `b37f8e7e171f3810d56b068e318547980d0d3827822fd5dfbd47d74662c4ac6f`.
- **Cómo se generaron:**
  - con `tools/extract_post05_runtime.py`, que reutiliza la reconstrucción POST03↔POST04 de `tools/posture_base_extract.py`, copia literal de `POST04/evidence_tools/extract_posture_runtime.py`;
  - dos ejecuciones dieron salida idéntica byte a byte, con `violations=[]`.
- **Pruebas del extractor:**
  - `tools/test_extract_post05_runtime.py`: 11 pruebas adversariales sobre logs sintéticos, más la revalidación opcional con los logs reales;
  - `tools/run_extractor_mutations.py`: **19/19** mutaciones detectadas.
  - **Hallazgo propio corregido:** el extractor aceptaba cualquier etiqueta `POST05_*` por prefijo; ahora usa una lista exacta.
  - Tres mutaciones sobrevivían porque estaban enmascaradas por otra capa de defensa. Se añadió una prueba que ejercita cada capa por separado.

## 2. Resultados POST05 [R]
| | A1 | B1 |
|---|---|---|
| Decisiones (`POST05_DECISION` + Finish) | 12 (12 confirmados) | 9 (7 confirmados + 2 Finish) |
| `SUPPRESS_ELIGIBLE` / `SUPPRESSION_APPLIED` | **4 / 4** (eventos 3, 5, 9, 11) | **0 / 0** |
| `ALLOW_VANILLA` | 8: 6 `LETHAL`, 1 `BIG_DAMAGE`, 1 `FAMILY_NOT_SWORD` | 6: 2 `FAMILY_NOT_SWORD`, 2 `PRE_MODE_NOT_ATTACK`, 2 `BIG_DAMAGE` |
| `SKIP_UNSAFE` | 0 | **1** (evento 7, actor 222/4, `ADMISSION_EVER_UNSTABLE`) |
| `recheck_failed` / `verify_failed` | 0 / 0 | 0 / 0 |
| `POST05_LAYOUT` / `POST05_CONSERVATION` | `MATCHES_EXE` / `ok=1` | `MATCHES_EXE` / `ok=1` |
| POST04: ponderados, cruces, unidades añadidas/cerradas | 5, 0, 125/116 (+9 regeneradas) | 5, 0, 125/125 |
| POST03: `inline_sites_flag`, cierre | 0, `all mods unloaded` | 0, `all mods unloaded` |
| HM / Bars | — | HM: 53 eventos, `hp_diff=0`; Bars: 6784 dibujos |

**A1 — lo que el log sí muestra [R]:**
- **Las 4 supresiones** ocurrieron en ATTACK con estado PRE 0 o 2, todas con postura 0→25 y cooldown 0→10.
- **El siguiente golpe registrado sobre el mismo actor** lo encontró más avanzado en su comportamiento, no en DAMAGE:
  - 220/2: ATTACK 0 → ATTACK 2;
  - 356/12: ATTACK 0 → ATTACK 2;
  - 221/3: el siguiente golpe llegó con modo PRE 3 (FIND);
  - 328/11: el siguiente golpe llegó con modo PRE 3 (FIND).

  Es coherente con que la IA siguió su curso, pero **no** prueba que la animación continuara bien en pantalla: eso es observación humana [P].
- **Estado PRE 1:** ninguna supresión en A1 tuvo estado PRE 1.

**B1 — el bloqueo de Blue [R]:**
- **Cambios de admisión:** `POST03_ADMISSION_CHANGED` aparece 10 veces, alternando entre `E_OC_CANDIDATE` y `UNKNOWN_UNSAFE`, en los actores 221/3 (×4), 222/4 (×4) y 161/8 (×2).
- **Coinciden uno a uno** con los `BLUE_GROUP_ON/OFF` de Expansion: 5 ON (`atp 1>2`) y 5 OFF (`atp 2>1`), siempre con `source=NATIVE_E_OC`.
- **Causa:** la admisión de 0.5.0 exige `AtAtp == 1` en E_OC. Con el grupo Blue (`AtAtp = 2`) el actor pasa a `UNKNOWN_UNSAFE`, POST04 lo marca `ever_frozen` y POST05 lo rechaza para siempre en esa generación.
- **El único golpe en ataque de B1** (evento 7: 222/4, ATTACK estado 1, espada, 220→200, postura 0→25) quedó en `SKIP_UNSAFE / ADMISSION_EVER_UNSTABLE`.
- **Sin ese bloqueo** habría sido el primer candidato de B1. Si la política nueva lo haría elegible depende de precondiciones auxiliares que 0.5.0 no registraba en esa ruta: **[P]**.

**B1 — Red y Black: sin decisiones POST05 porque POST03 los excluye [R]:**
- **Excluidos por POST03:** los actores Red de Expansion (158, 159, 160, 163, 172; `source=NATIVE_E_OC2`, 220→330 HP) y el Black 218 (`source=DEVF_BLACK`, encuentro `bokoblin-010`, 220→440 HP) aparecen en POST03 como `adm=E_OC2_EXCLUDED`. Por eso no tienen `POST03_HIT_EVENT` ni `POST05_DECISION`.
- **Black 218:** Expansion registró 16 `BLACK_DAMAGE_RECOGNIZED`, de 440 HP a 0.
  - `BLACK_FURY_START` llevó `atp` de 3 a 6 en el primer golpe no letal, con `fury_ticks_remaining=300`.
  - Hubo 15 `BLACK_FURY_REFRESH` (6→6) y un `BLACK_FURY_END` (6→3) por muerte.
  - Expansion lo reconoció en cada golpe **ya en DAMAGE** (`native_mode=5`, estado 5), salvo el último (BIG_DAMAGE 6/3). El intervalo fue de unos 11–25 ticks de Expansion entre golpes.
  - Es la reacción vanilla, como se esperaba con POST05 0.5.0.
- **Red 163:** un `RED_HIT_REJECTED` (`BLOCKED_BY_SHIELD_LINK_SIDE`) y un `HELM_SPLITTER_EXACT_DAMAGE_APPLIED` (también en Blue 221).
- **Qué significa:** son la **línea base vanilla** de Red y Black con 8/8 mods; sirven para comparar en la próxima prueba. **No** dicen nada sobre cómo se comportarían con supresión.

## 3. Conclusiones que alimentan POST05-COMPAT
| Hallazgo | Etiqueta | Consecuencia en 0.5.1 |
|---|---|---|
| Blue de grupo alterna `AtAtp` 1↔2 sin cambiar de recurso; 0.5.0 lo trata como admisión inestable | [R]+[S] | V2: E_OC con par `AtAtp` 1/1 o 2/2 es candidato estable (C2) |
| Red y Black de Expansion usan el recurso E_OC2, que POST03 excluía | [R]+[S] | V2: E_OC2 admitido para **observación y postura**; la supresión queda diferida (C3/C4) |
| Black cambia `AtAtp` 3→6 por la pérdida de HP del golpe. Lo hace en el POST de `execute` de Expansion (`REAL_NON_LETHAL_HP_LOSS_IN_NATIVE_EXECUTE`), es decir, en el mismo cuadro pero **después** de que `damage_check` retorne | [R]+[S] | si suprimiéramos el flinch, el golpe en curso del Black pasaría a potencia 6 sin que nuestro hook pudiera verlo. Es el motivo principal para diferir la supresión de Black (`05_BLACK_COMPAT_AUDIT.md`). `POWER_CHANGED_IN_CALL` cubre solo cambios **dentro** de nuestra llamada |
| A1 nativo: 4 supresiones con escritura verificada y sin fallos | [R] | se mantiene la política 0.5.0 para E_OC |
| Ninguna supresión con estado PRE 1 observada | [R] | sigue siendo [P] en juego |
