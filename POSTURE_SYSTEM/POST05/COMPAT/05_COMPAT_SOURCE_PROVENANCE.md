# POST05-COMPAT — Procedencia de fuentes

Fecha: 2026-11-01. Etiquetas: **[R]** log real · **[S]** fuente · **[B]** binario · **[C]** CI · **[P]** pendiente.

## 1. Este checkpoint
| Elemento | Valor |
|---|---|
| Rama fuente | `feature/posture-post05-compat-all-bokoblins`, creada desde `c06b44e7bea1a279119131f74767b4ecdd8f7422` (docs de POST05 0.5.0) |
| Base binaria congelada | POST05 0.5.0, fuente `a9627a2373be3c61e744f875f65f4237d42e6121`, run `41337085240`, artefacto `13095946275`, `.dusk` `f6c1a3c6…db068` |
| Rama CI | `ci/posture-post05-compat-all-bokoblins` (repositorio privado `codex-cp00-inputs`) |
| ID / versión | `dev.posture.post05_selective_reactions` **0.5.1** (mismo ID que 0.5.0; uno u otro, nunca los dos a la vez) |
| Carpetas intocadas | `POST00`…`POST04`, `POST03_1` y los archivos de 0.5.0 que no son fuente (`POST05/*.md`, `POST05/VERIFICATION.json`). La auditoría por diff lo comprueba (`05_WRITE_SET_AND_ABI_AUDIT.md`) |

## 2. Bokoblin Expansion 0.17.0 y Enemy Variant Framework 0.15.0 — fuente exacta [S]
**Corrección de una afirmación previa.** En POST05 0.5.0 (`EVIDENCE.md` §5 y `POST05_NATIVE_REACTION_AUDIT.md`) escribí que la fuente exacta de Expansion 0.17.0 y EVF 0.15.0 «no estaba disponible». **Era falso.** Solo había buscado en las ramas remotas ya traídas. La fuente está en este mismo repositorio. Los documentos de 0.5.0 **no se editan** (son el registro histórico de aquel checkpoint); esta corrección vale desde POST05-COMPAT.

| Elemento | Valor |
|---|---|
| Commit compilado | `192a7731623d69baacdd37011db60ce894d9de18` (2026-10-19, «CP21: pass the pinned source SHA…») |
| Rama / documentación | `feature/devf-cp21-public-package`, cabeza `fe47c8b828939829914d466a425cb1e2c0223cd9`; PR #81 en DRAFT |
| CI | run `39359746319`, artefacto `12237111893` (`DEVF/CP21_PUBLIC_RELEASE/DELIVERY.md`, `VERIFIED_CP21.json`) |
| `BokoblinExpansion_0.17.0.dusk` | 1912586 bytes, SHA-256 `083de98ca6a9e5918470ebbc31f720e700bdb25dedc4b52d19f863cfc7611e2a` |
| `EnemyVariantFramework_0.15.0.dusk` | 103765 bytes, SHA-256 `51f24f178969dd22d438ecfcc7336c043e7d72f05c29a91248a8f41f405ef1d5` |
| Manifiestos | `DEVF/CP21_PUBLIC_RELEASE/expansion/mod.json`: `dev.bokoblin.expansion` 0.17.0. `…/provider/mod.json`: `dev.devf.cp06_native_config_dual` 0.15.0 |

**Composición de la compilación** (`DEVF/CP21_PUBLIC_RELEASE/{expansion,provider}/CMakeLists.txt`):
- **Expansion:**
  - TU 1: `runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion/mod.cpp` (2100 líneas). Incluye las políticas de CP10.2 (Black), CP10.4 (identidad/HP), CP10.5 (daño), CP10.6 (dinámica Blue/Black), CP10.6A (visual), CP12, CP14 (Red) y el resto que lista ese archivo en sus líneas 1–81;
  - TU 2: `runtime/DEVF/CP12_GROUP_COMBAT_FPS_AND_VARIANT_EYES/expansion/perf_clock.cpp`.
- **EVF:** `runtime/DEVF/CP10_MULTI_ROOM_BOKOBLIN_EXPANSION/provider/mod.cpp` (66 líneas). **Ningún hook**, ninguna escritura de actor.

**Hooks de Expansion** (`mod.cpp` líneas 133, 135, 2020 y 2022):
- `DEFINE_HOOK(&daE_OC_c::execute, …)` con PRE (`on_execute`) y POST (`after_execute`);
- `DEFINE_HOOK(&daAlink_c::modelDraw, …)`, solo de render.

**Ninguno** engancha `daE_OC_c::damage_check`. Nuestro hook en `damage_check` corre **dentro** del `execute` nativo, entre el PRE y el POST de Expansion. El orden de un cuadro es:
1. Expansion PRE `execute`;
2. el `execute` nativo, que llama a `damage_check` y por tanto a nuestro PRE, el original y nuestro POST;
3. Expansion POST `execute`: dinámica Blue/Black (`dynamic_post`, línea 894), robo de vida Red (`red_step`, línea 870) y regeneración.

**Escrituras de Expansion relevantes para POST05** [S]:
- **`AtAtp` de las dos esferas de ataque:**
  - en la admisión (`damage_apply`, línea 849, vía `SphereIo`, línea 152);
  - en `dynamic_post` (grupo Blue 1↔2; furia de Black 3↔6, con 300 ticks tras cualquier pérdida de HP no letal);
  - en el restablecimiento de la línea 924.
- **`setActionMode`** en la IA de reclutamiento Blue (líneas 1003, 1025, 1049, 1054 y 1057), solo en modos de reposo/búsqueda (Sound, Watch, Find, FindStay), y en la IA de Black (PRE).
- **`health`** del Red (robo de vida, línea 873), en el POST.
- **Comentarios explícitos del propio Expansion:** «Native damage_check() retains priority» (líneas 990, 1379 y 1431). Es decir, Expansion no compite con `damage_check`, pero **lee** el modo que este deja.

**Equivalencia con lo instalado:**
- **[R]:** el log B1 muestra `found 'Bokoblin Expansion' v0.17.0` y EVF `v0.15.0`, y las líneas de Expansion llevan `version=0.17.0`.
- **[P]:** que el `.dusk` instalado sea byte a byte el de la tabla. **No** se leyó la carpeta de mods del usuario (no autorizado).

## 3. Motor nativo [S]/[B]
- **Fuente:** decompilación de Dusklight, `d_a_e_oc.cpp`.
  - `create` (~2736–2801) fija `AtAtp(1,1)` y HP 40 para `mName == "E_OC"`, y `AtAtp(2,2)` y HP 220 para E_OC2.
  - El bit de estado `0x10000` solo se pone en E_OC.
  - `damage_check` solo ramifica por `mName` en la rama de tirachinas.
- **Binario:** `dusklight.exe` 2.0.3 (`ffa99203…0049`).
  - `damage_check` y la cola `setActionMode(DAMAGE)` en `0x140e067b9` ya están auditados en `POST05_NATIVE_REACTION_AUDIT.md`.
  - La re-verificación para E_OC2 está en `05_EOC2_HIT_PATH_ABI_AUDIT.md`.

## 4. Lo que NO se hizo
- No se modificó, recompiló ni instaló Expansion ni EVF.
- No se creó ningún servicio ni API de Expansion.
- No se leyeron partidas, configuración ni la carpeta de mods.
