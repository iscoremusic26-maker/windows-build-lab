# POST05-GENERAL 0.5.4 — anti-tambaleo general hasta romper la postura

Etiquetas: **[R]** log real del usuario · **[S]** fuente exacta · **[B]** binario del juego · **[L]** laboratorio · **[C]** CI · **[P]** pendiente de prueba humana.

## Regla
**Postura < 100 tras contar el golpe ⇒ sin tambaleo, en cualquier modo normal.**
- Un golpe que lleva la postura a 100 sí reacciona: es la ruptura, con la reacción nativa como sustituto, porque **POST06 no existe**.
- Con la postura ya enganchada en 100, todo sigue nativo hasta POST06.
- Los golpes letales y los Finish siempre siguen nativos.

## Base y rama
- **Base:** 0.5.3, fuente compilada `cf2fa7816f1401a626c37ea5d239dceec387d8b6` y documentos `c835756`.
- **Rama nueva:** `feature/posture-post05-general-anti-stagger`. Las ramas 0.5.3 y 0.5.2 no se tocan.

## Causa demostrada [R][S]
- **Las 23 reacciones [R]:** en los logs 0.5.3 (`dusklight-20261101-193759`, POST05 + Zora, `02086d76…`; `dusklight-20261101-193154`, 8 mods, `0757194f…`) hubo **23** reacciones no letales con postura por debajo de 100. Detalle en `evidence/REAL_0_5_3_SUB100_STAGGERS.tsv`.
  - 20 fueron `PRE_MODE_NOT_ATTACK` (FIND 3/4, WATCH 7 y DAMAGE 5 encadenado);
  - 1 fue `FAMILY_NOT_SWORD` (golpe de escudo);
  - 2 fueron rechazos de fase E_OC2 con Expansion.
  - Ahí están Red 172 (eventos 1–7) y Blue 161 y 162.
- **Origen [S]:** el gate `PRE_MODE_NOT_ATTACK` de `classify()` y la restricción a espada y a la ruta DAMAGE.
- **Con la regla 0.5.4, las 23 se protegerían.** Es una reproducción offline de las decisiones con la regla nueva, no una observación en el juego.

## Rutas de reacción de `daE_OC_c::damage_check` [S] (`d_a_e_oc.cpp:638-786`)
| Ruta | Qué escribe la reacción | 0.5.4 |
|---|---|---|
| `checkCutDownHitFlg` (Finish) | DEATH 1 + `offTgSph` | nativo |
| Cooldown ≠ 0 | nada (golpe ignorado) | — |
| Tirachinas | E_OC: daño 5 y BIG si muere; E_OC2: SE + `setActionMode(FIND, 1)` desde WAIT/WALK/TALK | no pasa por la decisión |
| **Ordinaria** (espada 0/1/2, flecha 0/1, escudo 3, bumerán 4) | `setActionMode(DAMAGE, s)`: modo, estado, modo previo, `field_0x6c0`/`0x6c2` = 0, Tg/Co set on (`mSphs_cc[0..1]`), HeadLock off | **se deshace** por debajo de 100 |
| **BIG_DAMAGE** (4.º corte, giros, salto, Mortal Draw, Twirl, `AtSpl`, bola de hierro) | lo anterior + `offTgSph` (Tg set off + `ClrTgHit`) | **se deshace** por debajo de 100 (Tg set vuelve a su PRE) |
| Letal (`health <= 1`) y bomba | BIG con muerte | nativo |
| Salto con giro cancelado | `setActionMode(DAMAGE)`, cooldown 3 | se deshace por debajo de 100 |

Todo lo demás del golpe se conserva:
- `field_0x6cc`, la alternancia de cortes (`0x6d6`/`0x6dc`), `mStts.Move` y `cc_at_check` (vida, SE, hit-stop, marca);
- `ClrTgHit` en BIG: el golpe ya está procesado.

## Implementación
- **`reaction_core.hpp`:**
  - `StaggerPolicy::General` con `classify_general()`; `mod.cpp` lo fija una sola vez. Por defecto sigue `AttackOnly`, la política 0.5.3 intacta para los labs heredados.
  - **Orden de `classify_general()`:** confirmado → familia verificada → no letal → forma POST de la reacción (DAMAGE 0..4 o BIG_DAMAGE 0..2) → cooldown 3/10/20 → **postura** (cruce → ruptura nativa; latch o ≥ 100 → nativo; omisión de POST04 distinta de peso 0 → nativo) → modo PRE no de guion → seguridad: identidad y generación, una única llamada a `setActionMode`, forma POST exacta del aux (bits `0x0F` o, en BIG, `0x0A`), potencia → `ELIGIBLE`.
  - **El modo PRE no es una condición de juego.** Solo se excluyen TALK, DEATH, WATER_DEATH, DEMO y FALL/FALL_DEAD (guion, conversación o caída física).
  - **Plan:**
    - **`RESTORE_PRE`:** modo, estado, modo previo, timers, HeadLock y bits Tg/Co vuelven a su valor PRE exacto. Sirve para FIND, ATTACK (incluido el rebote 3), DAMAGE o BIG en curso (la reacción en curso **no se reinicia**), FIND_STAY y MOVE_OUT.
    - **`NOTICE_FIND1`:** WAIT, WALK, WATCH y SOUND_WATCH quedan como `setActionMode(FIND, 1)`, que es la reacción nativa del propio `damage_check` al tirachinas. El Bokoblin descubre a Link (voz `V_FIND`) sin animación de daño.
  - **`apply()`:** relee todos esos campos, escribe y verifica todos. Un escritor de 3 campos con un plan que necesita más no se intenta. Si la verificación falla, la sesión se cierra.
- **`mod.cpp` `restore_reaction(r, target)`:** el **único** escritor.
  - Asigna `mActionMode`, `mOcState`, `mOldActionMode`, `field_0x6c0` y `field_0x6c2`.
  - Llama solo a los setters inline que el juego usa en esas mismas funciones: `On/OffTgSetBit` y `On/OffCoSetBit` de `mSphs_cc[0..1]` y `on/offHeadLockFlg`.
  - Offsets verificados en el EXE desde 0.5.0 [B]: `+0x8c8/+0x8cc/+0x8d0`, u32 `+0x8e0`, `+0x69a` bit `0x80`, `orl $1`/`ClrSet` en `+0xce8/+0xd08/+0xea0/+0xec0`.
  - Nunca toca vida, cooldown, AT, animación, velocidad ni el bit de golpe.
  - Mismo hook único, PRE siempre continúa, sin imports nuevos.
- **E_OC2 con Expansion:** la fase y el entorno solo se **registran**. Si el golpe cae en el tick de arranque o en el `frame` 1 de un x2, ese golpe se suprime y su velocidad queda nativa. En el log sale `compat=EXPANSION_RATE_NATIVE_THIS_BLOW`.
  - Código real de CP21: 32 casos, todos bien formados (una sola escritura de `rate`, sin x2 tardío, sin barrido huérfano, chip ≤ 1, fin de clip nativo).

## CI y binario [C][B]
- **Run:** https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/41371589990. `build` e `independent-download` en éxito al primer intento.
- **Origen del build:** espejo `ci/posture-post05-general-anti-stagger` @ `2ec9298fcc22bde351db1866eeb5ea340f4f4124`; fuente compilada `3053a75113024335cf583ed14474ddc2a4a62839`; libro de control de 41 archivos.
- **Labs con MSVC:** los 8 labs (incluido `general_lab`, 5560) dan los mismos conteos que con g++.
- **Auditorías:** `SOURCE_AUDIT_PASS` (con las reglas nuevas del escritor y de la política), `PE_AUDIT_PASS` y 11/11 negativas PE.
- **Imports y exports:** los mismos 3 imports del juego y los mismos 5 exports.
- **Descarga:** https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/41371589990/artifacts/13109308222 (ZIP `POST05G_TEST_41371589990_1`, 38212 bytes, `b731db78…3394`).
- **`.dusk`:** `POSTURE_POST05_4_GENERAL_PRIVATE_TEST_windows-amd64_3053a7511302.dusk`, **36965 bytes**, SHA-256 **`d3b7ddb63dc300bf55d42fca184bea9c12c72b3797c105b5e43337b2126d2a91`**. Caduca el 2026-12-02 01:17 UTC.
- **DLL:** 75264 bytes, `3d4c9de9a15846de4ea497ccb3d3a23c57340b02015c55d7847996f2ea858c9f`. `mod.json` es idéntico al de la fuente.
- **Descarga independiente por ID** (API): ZIP y `.dusk` coinciden con el digest y con `SHA256SUMS`. El job lo marca `ARTIFACT_DOWNLOADED_VERIFIED`.
- **Escrituras en la DLL descargada:** exactamente 10 stores al actor, en `restore_reaction` (0x180001290–354), tras la relectura `cmp`:
  - `0x8c8`, `0x8d0` y `0x8cc` (modo);
  - `0x8e0` y `0x8e2` (timers, 16 bits);
  - `0x69a` (16 bits, solo el bit `0x80` cambia);
  - `or $1` / `and ~1` del bit 0 en `0xce8`, `0xd08`, `0xea0` y `0xec0`. Son las mismas operaciones que `setActionMode`/`offTgSph` del EXE [B].
  - 0 stores a HP (`0x656`), cooldown (`0x8ec`), `mName`, `mAtInfo`, `mpMorf` o `speedF`.

## Cobertura
| Variante y estado | IMPLEMENTED | LAB_PASS | CI_PASS | Runtime |
|---|---|---|---|---|
| Blue solo: WAIT/WALK/WATCH/SOUND → descubre (FIND 1) sin daño | sí | sí | sí | HUMAN_RUNTIME_PENDING |
| Blue solo y en grupo: FIND, ATTACK, DAMAGE en curso | sí | sí | sí | HUMAN_RUNTIME_PENDING |
| Red nativo (caso real 172 en FIND, regeneración 23→98) | sí | sí (fixture) | sí | HUMAN_RUNTIME_PENDING |
| Red y Black con Expansion en cualquier modo normal | sí | sí (CP21 en ATTACK) | sí | HUMAN_RUNTIME_PENDING |
| Golpe en el tick de arranque o en el `frame` 1 x2 con Expansion | sí (sin tambaleo; velocidad nativa en ese golpe) | sí (CP21: bien formado) | sí | PENDIENTE: par privado de Expansion para conservar x2 |
| BIG_DAMAGE por debajo de 100 (giros, salto, 4.º corte, bola) | sí | sí | sí | HUMAN_RUNTIME_PENDING |
| Flecha, escudo y bumerán por debajo de 100 | sí (decisión de diseño a confirmar) | sí | sí | HUMAN_RUNTIME_PENDING |
| Cruce a 100 / latch / letal / Finish / guion | nativo | sí | sí | [R] observado en 0.5.x |

## Lo que queda sin cubrir (no se presenta como PASS)
1. **Velocidad de Expansion en 1–2 ticks:**
   - **Qué pasa:** con Expansion, un golpe en el tick de arranque o en el `frame` 1 de un golpe x2 no tambalea, pero ese golpe pierde su x1,15/x2. Expansion lee el golpe en su PRE (`red_ai_policy.hpp` :381/:374 y su equivalente en Black), antes del hook.
   - **Arreglo propuesto (par privado de Expansion, NO implementado en esta iteración):** en `post()`, aceptar la decisión de tasa cuando tras el `execute` nativo el golpe B/C sigue vivo (`blow(a)`) aunque el PRE viera `hit`, tanto en el arranque como en el x2 pendiente.
   - **Cómo probarlo:** con las mismas funciones CP21 en `rb_lab`.
   - **Por qué no está hecho:** exige compilar y empaquetar Expansion en un pipeline aparte. Queda **PENDIENTE**.
2. **Bumerán y golpe de escudo:** ya no aturden por debajo de 100. Pesan 0, así que nunca rompen la postura. Es la regla pedida, pero es un cambio de diseño de esas herramientas: pendiente de la decisión de balance (POST09).
3. **WAIT/WALK/WATCH/SOUND_WATCH → FIND 1:** el Bokoblin hace la animación nativa de descubrir a Link, no la de daño. Si quieres que ni siquiera la haga, es otra decisión.
4. **Modos de guion, muerte, agua, demo y caída:** siguen nativos a propósito.
5. **Postura 100:** no hay derribo real (POST06).

## Pruebas [L] (g++ con ASAN y UBSAN; MSVC en CI)
- **Labs heredados:** 1660 / 29558 / 12826 / 556878 / 36933 / 55991 / 21313, conteos de 0.5.3 salvo `compat`/`rb` (+4 y +41 por formato y casos nuevos).
- **`general_lab`, 5547 comprobaciones:**
  - Red 172 en FIND: 0→25→50→75 sin DAMAGE y restauración exacta de FIND 4 con timers 7/15; 75→100 es ruptura nativa; después, latch.
  - Regeneración 23→48→73→98.
  - Blue solo en WAIT/WALK/WATCH/SOUND → FIND 1, y luego FIND sin DAMAGE.
  - DAMAGE en curso no reiniciado; rebote ATTACK 3.
  - BIG_DAMAGE (cortes 5/6/7, `AtSpl`, bola de hierro +50) con los bits Tg restaurados.
  - Letal, bomba, Finish, familias, modos de guion, salto cancelado, flecha con latch.
  - Timers y HeadLock PRE exactos; escritor de 3 campos no intentado; relectura; verificación con escritor roto y con escritor parcial.
  - Gemelos con y sin POST05 + oráculo independiente: 1200 golpes en 12 modos y 10 tipos. Vida, cooldown, alternancia y postura idénticos; protegidos ⇔ regla.
  - Varios actores a la vez y reciclado de generación.
