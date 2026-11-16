# POST05-COMPAT — C0: identidad de variante y transiciones

Etiquetas: **[R]** log real B1 · **[S]** fuente (nativa y Expansion 0.17.0 en `192a773`) · **[L]** laboratorio · **[P]** pendiente.
Regla del encargo: **ni `AtAtp`, ni el HP, ni el color pueden ser el único identificador.** POST05 no inventa un detector de Blue ni crea ningún servicio de Expansion.

## 1. Lo que ve nuestro hook (sin Expansion API)
Dentro de `damage_check` solo leemos:
- recurso (`mName`: E_OC / E_OC2);
- el par `AtAtp` de las dos esferas de ataque;
- el bit de estado `0x10000`;
- HP, modo/estado y la identidad `(id, generation)` de POST03.

Con eso **no** se puede decir «Blue», «Red» ni «Black». Solo se puede saber si el actor está en una **combinación coherente** con alguno de los perfiles conocidos.

| Variante (Expansion) | Recurso | `AtAtp` posibles | `0x10000` | HP inicial | Fuente |
|---|---|---|---|---|---|
| E_OC nativo, sin Expansion | E_OC | 1/1 | sí | 40 | [S] `create` |
| **Blue** (todo E_OC nativo con Expansion) | E_OC | 1/1; **2/2 en grupo** (por proximidad, ON/OFF muchas veces en una vida) | sí | 220 | [S] CP10.4/CP10.6; [R] B1: 5 ON + 5 OFF |
| Blue DEVF sobre recurso E_OC2 (registro exacto) | E_OC2 | 1/1 o 2/2 | no | 220 | [S] CP10.4 (`EXACT_DEVF_BLUE_E_OC_REGISTRATION`); no visto en logs |
| E_OC2 nativo, sin Expansion | E_OC2 | 2/2 | no | 220 | [S] |
| **Red** (todo E_OC2 nativo con Expansion) | E_OC2 | 2/2 | no | 330 | [S]; [R] B1: 158, 159, 160, 163, 172 |
| **Black** (registro DEVF exacto) | E_OC2 | 3/3; **6/6 en furia** (300 ticks tras pérdida no letal de HP) | no | 440 | [S]; [R] B1: 218, 3→6→…→3 |

**Consecuencias:**
- Desde `damage_check`, el Red de Expansion y el E_OC2 nativo son **indistinguibles**: mismo recurso y mismos `AtAtp`. Solo cambia el HP, que no se puede usar como identificador.
- Black (3/6) se distingue del Red **por `AtAtp`**, pero no se puede usar como identificador único. Tampoco hace falta: la decisión de 0.5.1 para E_OC2 no depende de ello.

## 2. Política de admisión V2 (0.5.1)
| Recurso | Condición (las dos esferas en *lockstep*) | Admisión | Supresión |
|---|---|---|---|
| E_OC | `(1,1)` o `(2,2)` **y** bit `0x10000` | `E_OC_CANDIDATE` | según la política de 0.5.0 (C2) |
| E_OC2 | `(a,a)` con `a ∈ {1,2,3,6}` **y** sin `0x10000` | **`E_OC2_CANDIDATE`** (nuevo) | **no**: `ALLOW_VANILLA / E_OC2_SUPPRESSION_DEFERRED` (C3/C4) |
| cualquier otro (par desigual, valor fuera de lista, bit incoherente, nombre desconocido, lectura imposible) | — | `UNKNOWN_UNSAFE` con motivo | no (`SKIP_UNSAFE`) |

- **Por qué el par en *lockstep*:** Expansion siempre escribe las dos esferas a la vez (`SphereIo`, [S]). Un par desigual indica un escritor desconocido o una lectura a medias. En V1 y en V2 eso es inseguro.
- **Por qué E_OC2 entra en POST04 (postura) pero no en la supresión:** POST04 es pasivo (solo contabilidad interna). Observar Red y Black da la línea base que hace falta para decidir más adelante (POST09/POST10) y no escribe nada en el juego.
- **Política V1 (0.5.0):** sigue en el código, seleccionable, y la usan los laboratorios heredados para que sus conteos no cambien.

## 3. Transiciones dentro de una generación
| Transición | Causa | V1 (0.5.0) | V2 (0.5.1) |
|---|---|---|---|
| E_OC 1/1 ↔ 2/2 | grupo Blue ON/OFF | `CANDIDATE → UNKNOWN_UNSAFE`: `ever_frozen`, **nunca** se suprime ([R] B1 evento 7) | `CANDIDATE → CANDIDATE`: **sin cambio de admisión**, la postura sigue |
| E_OC2 2/2, 3/3 ↔ 6/6 | furia de Black | siempre `E_OC2_EXCLUDED` (sin postura) | `E_OC2_CANDIDATE` estable; sin supresión |
| par desigual o valor fuera de lista | escritor desconocido | `UNKNOWN_UNSAFE` | `UNKNOWN_UNSAFE`: se congela la postura y queda `ever_unsafe` |
| cambio de recurso | imposible en una generación ([S]: `mName` solo se fija en `create`) | — | si ocurriera, `UNKNOWN_UNSAFE` |

- **Cambio de `AtAtp` dentro de la llamada:** si el par cambia entre el PRE y el POST de **nuestra** llamada a `damage_check`, la decisión es `SKIP_UNSAFE / POWER_CHANGED_IN_CALL`. Con Expansion 0.17.0 no debería ocurrir: Expansion solo escribe en su PRE/POST de `execute`, fuera de `damage_check` ([S]). Si apareciera, sería un escritor desconocido.
- **Historial inseguro (0.5.1):** la generación guarda una máscara con el motivo:
  - `ADMISSION_UNSAFE`: pasó por `UNKNOWN_UNSAFE`;
  - `LOOKUP_UNVERIFIED`: lookup sin verificar.

  `ADMISSION_EVER_UNSTABLE` sigue cerrando la supresión, igual que en 0.5.0.

## 4. Pruebas
- **[L]:** `compat_lab` T01–T12 (identidad), T13–T20 (transiciones) y mutaciones de admisión. Resultados en `05_COMPAT_TEST_RESULTS.md`.
- **[R] disponible:** B1 contiene las transiciones 1↔2 de Blue y 3→6→3 de Black con 0.5.0.
- **[P]:** las mismas transiciones con 0.5.1 cargado.
