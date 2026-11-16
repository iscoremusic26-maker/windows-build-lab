# POST05 — Auditoría de la reacción nativa (GATE 05-R / 05-S)

Etiquetas: **[S]** fuente `TwilitRealm/dusklight@40457c6adb381928e4b5fef6ed459ed291edd5e2` · **[B]** binario `dusklight.exe` SHA-256 `ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049` (desensamblado local con `objdump`; el EXE no sale del equipo) · **[R]** logs del usuario · **[C]** MSVC/CI · **[L]** laboratorio · **[P]** pendiente.

## 1. Inventario ABI [B]
| Símbolo (export) | Thunk | Cuerpo real |
|---|---|---|
| `?damage_check@daE_OC_c@@QEAAXXZ` | `0x14006c4ae` | `0x140e06390` |
| `?setActionMode@daE_OC_c@@QEAAXHH@Z` | `0x1400287ae` | `0x140e0d260` |
| `?action@daE_OC_c@@QEAAXXZ` | `0x1400685c0` | `0x140e046e0` |
| `?executeDamage@daE_OC_c@@QEAAXXZ` | `0x1400527d4` | `0x140e08000` |
Offsets Win64 usados (todos con `static_assert` en `mod.cpp` salvo los de las esferas, que son `protected` y se comprueban en tiempo de ejecución, §6):
- `mActionMode` `+0x8c8`, `mOldActionMode` `+0x8cc`, `mOcState` `+0x8d0`.
- `field_0x6c0/6c2` `+0x8e0/+0x8e2`, `field_0x6cc` (cooldown) `+0x8ec`.
- `fopEn_enemy_c::mFlags` `+0x69a` (bit HeadLock `0x80`).
- Bit «set» de Tg/Co de `mSphs_cc[0]` en `+0xce8/+0xd08`, y de `mSphs_cc[1]` en `+0xea0/+0xec0`; `sizeof(dCcD_Sph)=0x1b8`.
POST05 solo usa el hook que ya existía; no añade imports del juego [C].

## 2. Enum y orden del cuadro [S]
- **Modos:** `WAIT 0, WALK 1, TALK 2, FIND 3, ATTACK 4, DAMAGE 5, BIG_DAMAGE 6, WATCH 7, SOUND_WATCH 8, DEATH 9, …` (`d_a_e_oc.cpp:20-37`).
- **Orden:** `execute()` (`:2586`) decrementa temporizadores y llama a `action()` (`:2415`). `action()` apaga los AT de las esferas de ataque, llama **primero** a `damage_check()` (`:2418`) y **después** al `switch(mActionMode)` (`:2475`), que ejecuta `executeAttack` o `executeDamage` en ese mismo cuadro.

## 3. `damage_check`: ramas y efectos (`:638-785`)
| Paso | Efecto | Antes de `setActionMode` final | Lo toca POST05 |
|---|---|---|---|
| CutDownHit → `health=0`, `setActionMode(DEATH,1)`, `offTgSph`, return | Finish | — | no (excluido) |
| `field_0x6cc != 0` → return | cooldown activo | — | no |
| `mStts.Move()`; TG de `mSphs_cc[0/1]` → `mAtInfo.mpCollider` (cc1 prevalece) | — | sí | no |
| cooldown `field_0x6cc = 10` (`0x14` si AT_TYPE_UNK) | | sí | no |
| cadena de armas: espada `my_val=getCutType()` + alternancia `field_0x6d6/6dc`; escudo 3; flecha 0/1; bomba 5 y `health=0`; hierro 5 y `+80/+140`; bumerán 4; AT40; tirachinas (ramas con return) | | sí | no |
| `cc_at_check(this,&mAtInfo)`: **resta HP, SE de impacto, hit-stop (`pause_time`), marca de golpe** (`d_cc_uty.cpp:372+`) | **daño legítimo** | sí | no |
| `GetAtSpl()==1` o `health<=1` → `my_val>=5` | | sí | no |
| `CUT_TYPE_JUMP && checkCutJumpCancelTurn()` → `setActionMode(DAMAGE,my_val)` + `field_0x6cc=3` + return | salto cancelado | — | no (`cooldown≠10` → vanilla) |
| `my_val>=5` → `setActionMode(BIG_DAMAGE,…)` + `offTgSph()` | reacción fuerte | — | no |
| **`else setActionMode(DAMAGE, my_val)`** | **reacción ordinaria solicitada** | **última sentencia** | **sí: se deshace bajo condición** |

[B] Ruta ordinaria: `cmp %bpl,%sil; jb 0x140e067b4` → `mov %ebp(=5),%edx` → `call setActionMode` (`0x140e067b9`) → epílogo (cookie de pila, restauración de registros, `ret`). Ninguna escritura posterior. El salto cancelado es distinguible: `movw $0x3,0x8ec(%rbx)` tras su `call`.

## 4. `setActionMode` [S `:584-594`][B `0x140e0d260`]
Escrituras exactas, 13 instrucciones:
- `+0x8cc ← +0x8c8` (modo previo);
- `orl $1` en `+0xd08`, `+0xce8`, `+0xec0` y `+0xea0` (Co/Tg «set» de las dos esferas);
- `u32 +0x8e0 = 0` (`field_0x6c0` y `field_0x6c2`);
- `and $0xff7f, +0x69a` (`offHeadLockFlg`);
- `+0x8c8 ← modo`, `+0x8d0 ← estado`.

No hay más efectos: ni animación, ni sonido, ni velocidad.

## 5. Reacciones y estados [S]
- **`executeDamage`** (`:1534-1601`): los estados 0–2 hacen `speedF=0`, `setBck(9|10|8)` y la voz `V_DAMAGE_LR`, y pasan a 5; los estados 3 y 4 corresponden a escudo y bumerán. Esta función es la que produce el *flinch* visible.
- **`executeAttack`** (`:1379-1532`):
  - estado 0: frena (`cLib_chaseF(speedF,0,1)`) y elige el ataque B o C (`setBck(5|6)`, voz);
  - estados 1/2: golpe en curso; AT activos en los fotogramas 14–22; estela; desplazamiento con tabla;
  - estado 3: rebote tras guardia de Link (animación en reversa).
  - No usa `field_0x6c0/6c2`, `mOldActionMode` ni el cooldown.
- **Uso de `mOldActionMode`:** solo se escribe (`:585`), nunca se lee en E_OC. POST03 lo lee como firma P2.

| Modo PRE / estado | Decisión POST05 | Razón |
|---|---|---|
| ATTACK 0 | admitido | sin animación de ataque aún; al restaurar, el actor frena y elige su golpe como haría sin el impacto |
| ATTACK 1, 2 | admitido | golpe en curso; la animación no la tocó `damage_check`; `executeAttack` continúa en el mismo cuadro |
| ATTACK 3 | **vanilla** | rebote por guardia: no hay un ataque que preservar |
| WAIT, WALK, TALK, FIND, WATCH, SOUND_WATCH, DAMAGE, BIG_DAMAGE, DEATH, … | **vanilla** | fuera de la *whitelist* |

## 6. Secuencia temporal de un golpe elegible
1. `mod_update` (tick de postura) → `fpcM_Management` → `daE_OC_c::execute` (Expansion engancha aquí PRE/POST, §8) → `action()`.
2. **PRE** de `damage_check` (POST03.1): admisión, *snapshot* (modo, estado, `old_mode`, cooldown), hit obj crudos y lecturas auxiliares PRE (temporizadores, HeadLock, bits Tg/Co). Siempre `HOOK_CONTINUE`.
3. **Original, una sola vez:** cooldown, cadena de armas, `cc_at_check` (HP, SE, hit-stop, marca; aquí actúa el hook de HM) y `setActionMode(DAMAGE,s)`.
4. **POST:**
   - POST03.1 juzga (P1 cooldown 0→10 + P2 modo 5 con `old_mode`=PRE) y lee el tipo AT (una sola desreferencia).
   - POST04 consume el evento **una vez** y, si toca, emite `BREAK_READY`.
   - POST05 decide con el `HitReport` y el resultado de POST04. Si es `SUPPRESS_ELIGIBLE` y el layout coincide con el EXE, relee los tres campos, los restaura y verifica.
5. Resto de `action()`: `searchOtherOc`, agua, `switch(mActionMode)` → **`executeAttack`** (no `executeDamage`). Animación, voz de daño y `speedF=0` del *flinch* no ocurren.
6. POST de `execute` de Expansion: ve ATTACK.

## 7. Estrategias comparadas
| Criterio | A: hook en `setActionMode` (vetar) | **B: POST de `damage_check`, deshacer la última `setActionMode`** | C: interceptar `executeDamage` o después |
|---|---|---|---|
| HP / SE / hit-stop | conservados | **conservados** (`cc_at_check` ya corrió) | conservados |
| Firma P2 de POST03.1 | **se pierde** (sin modo 5 no hay `HIT_CONFIRMED`) | **intacta** (POST05 decide después) | intacta |
| Consumo POST04 | perdido o desacoplado | **una vez, antes** | una vez |
| Hooks nuevos | uno más (`setActionMode` aparece en 55 llamadas de `d_a_e_oc.cpp`) | **ninguno** | uno más |
| `HOOK_SKIP_ORIGINAL` | necesario | **no** | necesario o reescribir estado |
| Animación/voz de daño | evitadas | **evitadas** (aún no empezaron) | **ya iniciadas** (`setBck`, voz) |
| Escrituras | ninguna en memoria (saltar original) | **3 campos a su valor PRE** | varias (animación, velocidad, estado) |
| Correlación con el golpe | difícil (dentro de `damage_check`) | **por token del mismo frame** | indirecta |
| Rollback | n/a | relectura y verificación; si falla, la sesión se cierra | difícil |
**Elegida: B**, la menos invasiva que sostienen fuente y binario. A y C se descartan por las razones de la tabla.

## 8. Otros mods
- **HM CP11** (`hm_203/core8`) [S]: solo `cc_at_check` (PRE/POST) dentro de `damage_check`, antes del `setActionMode` final. Ningún hook sobre E_OC ni sobre `setActionMode` de E_OC (solo el de `daE_TT_c`). El HP que POST03.1 ve en el POST ya está reconciliado por HM; POST05 no lo usa para decidir.
- **Bars CP06** [S]: solo `dMeter2_c`.
- **Expansion 0.17.0 y EVF 0.15.0: fuente exacta NO disponible [P].**
  - La fuente más cercana, Expansion 0.16.2, engancha `daE_OC_c::execute` PRE+POST. Fuera de `damage_check` escribe `setActionMode`, `field_0x6c2` (bloqueos) y la velocidad de reproducción del ataque Blue. También fuerza `ATTACK 0` en ciertas intercepciones.
  - Con POST05, el POST de `execute` de Expansion ve ATTACK en vez de DAMAGE: coherente con lo que el actor hace realmente, pero **no auditado** para 0.17.0.
  - Si Expansion deja `field_0x6c2≠0` en un ataque, POST05 se abstiene (`PRE_TIMERS_NONZERO`).
- **Blue:** no hay detector de procedencia. Cualquier generación que haya pasado por `UNKNOWN_UNSAFE` (p. ej. el `AtAtp` transitorio de Blue) queda **excluida de la supresión durante toda su vida** (`ADMISSION_EVER_UNSTABLE`); POST04 sigue acumulando. Es una exclusión prudente, no una identificación de Blue (D1 sigue abierta).

## 9. Qué demuestra el laboratorio y qué no
- **Puede demostrar [L]:**
  - que la decisión sigue la política;
  - que solo se escriben tres campos y a su valor PRE;
  - que HP, cooldown, POST04 y POST03 quedan iguales que sin POST05 (gemelos T16/T20);
  - que el original corre una vez.
- **No puede demostrar:** que la animación de ataque continúe con naturalidad, que el Bokoblin no se quede trabado, que el sonido sea coherente, que no haya doble golpe a Link, ni la interacción real con Expansion 0.17.0. Todo eso es **[P] hasta la prueba humana (G2)**.

## 10. Riesgos residuales [P]
- **Audio:** la voz de daño de la criatura deja de sonar en los golpes suprimidos; el SE del impacto se mantiene.
- **Estado 0:** el Bokoblin inicia su golpe inmediatamente tras recibir uno.
- **Estados 1/2:** el golpe en curso puede alcanzar a Link (fotogramas 14–22). Es el comportamiento buscado, pero hay que verlo en el juego.
- **Hooks de terceros sobre `daE_OC_c::setActionMode`:** si los hubiera, ya habrían visto DAMAGE. No los hay en las fuentes auditadas; 0.17.0 [P].
- **Escala de tiempo, pausas y cinemáticas:** POST05 no los mira. Solo actúa dentro de `damage_check` con modo ATTACK.
