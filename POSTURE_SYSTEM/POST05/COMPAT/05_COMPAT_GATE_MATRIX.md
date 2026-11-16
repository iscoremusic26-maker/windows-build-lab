# POST05-COMPAT — Matriz de gates y de cobertura

Etiquetas: **[R]** log real · **[S]** fuente · **[B]** binario del juego · **[C]** CI/PE · **[L]** laboratorio · **[P]** pendiente.

## Gates
| Gate | Estado | Respaldo |
|---|---|---|
| **C0** identidad y transiciones | **PASS** [S][L]; transiciones vistas en [R] con 0.5.0 | `05_VARIANT_IDENTITY_AND_TRANSITIONS.md`; T01–T16 |
| **C1** ruta de golpe y ABI de E_OC2 | **PASS** [S][B] | `05_EOC2_HIT_PATH_ABI_AUDIT.md` |
| **C2** admisión dinámica de Blue | **PASS** [S][L]; runtime [P] | `05_BLUE_DYNAMIC_ADMISSION_AUDIT.md` |
| **C3** Red | **SAFE_EXCLUSION**: observado, con postura y reacción vanilla [S][L] | `05_RED_COMPAT_AUDIT.md` |
| **C4** Black normal y en furia | **SAFE_EXCLUSION** (ídem) [S][L][R base vanilla] | `05_BLACK_COMPAT_AUDIT.md` |
| **C5** implementación compartida, escritor único de 3 campos | **PASS** [S][L][C] | `05_WRITE_SET_AND_ABI_AUDIT.md` |
| **C6** laboratorios T01–T40 con mutaciones | **PASS** [L][C]: 6 labs, 142/142 mutaciones | `05_COMPAT_TEST_RESULTS.md` |
| **C7** CI, PE y descarga independiente | **PASS** [C]: run `41344244739`, artefacto `13098463747` | `05_COMPAT_DELIVERY.md` |
| **C8** runbook | **entregado** | `README_TEST_ALL_VARIANTS.md` |
| `G2` / runtime humano | **PENDIENTE** | nadie ha cargado 0.5.1 |

**Veredicto: `POST05_COMPAT_PARTIAL_WITH_SAFE_EXCLUSIONS`.**
- Blue (solo y en grupo) y E_OC nativo: soporte completo.
- Red y Black: compatibles, con supresión diferida y motivo documentado.

**No se declara** `ALL_BOKOBLINS_RUNTIME_PASS`, `G2_PASS` ni `POST05_RUNTIME_PASS`.

## Cobertura T1–T9 (casos de juego del encargo)
| Caso | Variante | Comportamiento 0.5.1 | Fuente / lab | Runtime |
|---|---|---|---|---|
| T1 | Blue original (E_OC nativo, sin Expansion) | supresión según la política de 0.5.0 | [S][L] T33 | 0.5.0: 4 aplicadas en A1 [R]; 0.5.1: [P] |
| T2 | Blue Expansion solo (1/1) | supresión según la política; sin bloqueo por grupo | [S][L] T13, T31 | [P] |
| T3 | Blue Expansion en grupo (2/2) | supresión según la política, con la potencia de grupo de Expansion | [S][L] T13, estrés (73) | [P] |
| T4 | transición de grupo 1↔2 | admisión estable, sin `ADMISSION_CHANGED` | [S][L] T13, T31–T32 | 0.5.0 bloqueaba [R]; 0.5.1: [P] |
| T5 | Red nativo (E_OC2 sin Expansion) | observado + postura; reacción nativa (`E_OC2_SUPPRESSION_DEFERRED`) | [S][L] T17, T29 | [P] |
| T6 | Red de Expansion | ídem (indistinguible de T5) | [S][L] T17 | sin hits POST03 en 0.5.0 [R]; [P] |
| T7 | Black normal (3/3) | observado + postura; reacción nativa | [S][L] T18 | base vanilla de Expansion [R]; [P] |
| T8 | Black en furia (6/6) | ídem; la furia no altera la admisión | [S][L] T18, T19 | 3→6→3 en B1 [R]; [P] |
| T9 | no verificados (escritor desconocido, desacuerdo, otro recurso o perfil) | `UNKNOWN_UNSAFE` o `POWER_CHANGED_IN_CALL` ⇒ nunca se escribe | [S][L] T03–T08, T15, T19, estrés | [P] |
