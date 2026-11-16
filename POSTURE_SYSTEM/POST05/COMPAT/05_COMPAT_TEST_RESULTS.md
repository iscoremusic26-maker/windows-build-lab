# POST05-COMPAT — Resultados de pruebas (0.5.1)

Etiquetas: **[L]** laboratorio · **[C]** CI/PE · **[R]** log real · **[P]** pendiente. Datos máquina: `05_COMPAT_TEST_RESULTS.json`.

## Laboratorios (MSVC en CI y g++ ASAN+UBSAN local; las cifras coinciden)
| Laboratorio | Comprobaciones | Fallos | Nota |
|---|---|---|---|
| `hit_lab` | 1660 | 0 | heredado; **mismo conteo** que 0.5.0 |
| `lifecycle_lab` | 29558 | 0 | heredado; mismo conteo |
| `collider_lab` | 12826 | 0 | heredado; mismo conteo (solo cambia el literal `v=0.5.1`) |
| `posture_lab` | 556878 | 0 | heredado; mismo conteo (muestra del formateador: solo cambia la versión; MSVC == g++) |
| `reaction_lab` | 36933 | 0 | heredado; mismo conteo y misma cobertura de estrés |
| **`compat_lab`** | **55953** | 0 | nuevo |

Los laboratorios heredados usan la admisión **V1** (por defecto). Que conserven sus conteos exactos demuestra que 0.5.1 no altera la lógica de 0.5.0 cuando se usa su política.

## `compat_lab`: casos T01–T40
| Bloque | Casos | Qué demuestra |
|---|---|---|
| Identidad (C0) | T01–T12 | regla V2 celda a celda; rejilla exhaustiva de 480 celdas contra un oráculo: V1 = 0.5.0, y V2 difiere solo en E_OC 2/2 y E_OC2 vivo (31 celdas) |
| Blue (C2) | T13–T16, T31–T32, T34 | patrón real de B1: 10 cambios de admisión con V1 y 0 con V2. El evento 7 pasa de `ADMISSION_EVER_UNSTABLE` a `ELIGIBLE`. Golpe en grupo 2/2 aplicado. Desacuerdo de esferas y lookup sin verificar quedan marcados en la máscara |
| E_OC2 (C3/C4) | T17–T24 | Red y Black (3↔6) observados con postura; 0 escrituras; cualquier señal de E_OC2 basta; letal y otras familias diferidos; Blue DEVF sobre E_OC2 diferido; contadores por recurso y R6 |
| Formato | T25–T27 | `adm(…/e_oc2_cand)`; campos exactos de `POST05_DECISION` (= lista blanca del extractor); el peor caso de longitud cabe en el búfer; sin `0x…` |
| Escritura (C5) | T28–T29 | gemelos con POST05 abierto o cerrado: solo difieren los 3 campos, y solo en E_OC |
| Estrés | T30 | 6 semillas × 15 000 cuadros, 52 872 comparaciones con un oráculo independiente y 0 discrepancias. 140 supresiones (73 con `AtAtp` 2), 25 174 E_OC2 diferidos (133 que *habrían* sido elegibles), 14 `POWER_CHANGED_IN_CALL` y 56 inseguros. El laboratorio exige esa cobertura mínima |
| Regresión | T33, T35–T40 | E_OC nativo: V1 y V2 deciden igual en 400 golpes aleatorios. Duplicados y sesión cerrada. Tablas de motivos (29) y versiones |

**Hallazgos propios durante el desarrollo** (corregidos antes del CI; ninguno en el código del mod):
- el oráculo del estrés suponía DAMAGE para E_OC2 golpeado con bola de hierro (BIG_DAMAGE) y no modelaba la regeneración de POST04. Ahora se compara el incremento de **cada** golpe desde la postura real previa;
- T14 esperaba evento en un golpe que V1 ni siquiera observa.

## Mutaciones (`tests/run_mutations.py`, salida en `evidence/MUTATIONS_0_5_1.txt`)
- **Mutaciones lógicas:**
  - 25 heredadas;
  - 75 de postura, reacción y compatibilidad: 22 nuevas (C01–C22) y 53 heredadas, de las que P20 se adaptó a la nueva línea de *bind* manteniendo la misma mutación semántica;
  - 6 laboratorios como detectores.
- **Negativas de fuente:** 42, con 7 nuevas (S36–S42): vuelta a V1, `admit` sin política, segunda asignación de política, diferimiento eliminado, valor por defecto V2, escritura de `AtAtp` y E_OC2 elegible por atajo.
- **Resultado:** **142/142** detectadas o rechazadas. N06 se detecta por tiempo límite, como en 0.5.0.

## Extractor de logs (`tools/`)
- 11 pruebas adversariales y **19/19** mutaciones.
- Sobre los logs reales A1/B1: 0 violaciones; salida idéntica en dos ejecuciones.
- **Hallazgo corregido:** aceptaba etiquetas `POST05_*` por prefijo.

## CI, PE y descarga [C]
- **Run y estado:** run `41344244739` (fuente `ac99b4d`, espejo `a67b0d6`); `build` y `independent-download` en éxito al primer intento.
- **Auditorías:** `SOURCE_AUDIT_PASS`, `PE_AUDIT_PASS` y 11/11 negativas PE.
- **Mi descarga por segunda vía:** artefacto **13098463747** (`POST05C_TEST_41344244739_1`) bajado por la API.
  - ZIP de 33414 bytes que coincide con el digest publicado `86135a52…67e1`.
  - `.dusk` de 32350 bytes, SHA-256 `f5be0ed2e00c196ce261db72756256fd5513229c4370ae4deee62ae6d90b83fb`.
  - `mod.json` idéntico al de la fuente; DLL de 65536 bytes, `614207b7…a951`.
- **Desensamblado de la DLL:** 3 escrituras al actor (solo los campos de modo); 0 imports nuevos (detalle en `05_WRITE_SET_AND_ABI_AUDIT.md`).
- **Libro de control del espejo:** 31 archivos, verificados byte a byte contra `ac99b4d`.

## Runtime
**[P]** Nadie ha cargado 0.5.1. No se declara `G2_PASS` ni `ALL_BOKOBLINS_RUNTIME_PASS`.
