# POST05-COMPAT — C1: ruta de golpe y ABI de E_OC2

Etiquetas: **[S]** fuente `d_a_e_oc.cpp` (decompilación de referencia) · **[B]** `dusklight.exe` 2.0.3, SHA-256 `ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049`, desensamblado local con `objdump` (el EXE no sale del equipo ni se versiona) · **[P]** pendiente.

## Pregunta
¿Es la reacción de un golpe sobre un E_OC2 la **misma máquina** que en E_OC? Es decir: misma clase y layout, mismo `damage_check`, la misma última `setActionMode(DAMAGE)` y los mismos efectos extra de `setActionMode`. Si la respuesta es sí, `restore_reaction` (3 campos) es mecánicamente aplicable a E_OC2. **Si es seguro aplicarlo es otra pregunta (C3/C4).**

## Respuestas
| # | Comprobación | Resultado | Evidencia |
|---|---|---|---|
| 1 | E_OC y E_OC2 son la misma clase `daE_OC_c` (mismo perfil 510) y solo cambia `mName`, elegido en `create` por el byte alto de `current.angle.x` | **sí** | [S] `create` ~2734–2739; Expansion 0.17.0 comprueba `profname==510` para ambos (`mod.cpp` líneas 900 y 1779 en `192a773`) |
| 2 | Diferencias de `create`: E_OC tiene `AtAtp(1,1)`, HP 40 y el bit de estado `0x10000`; E_OC2 tiene `AtAtp(2,2)`, HP 220 y no tiene `0x10000`. Recursos de modelo distintos | **sí** | [S] 2772–2801 |
| 3 | `damage_check` lee `mName` **una sola vez**: la rama de tirachinas (E_OC pierde 5 HP; E_OC2 solo emite un SE) | **sí** | [S] línea 730. [B] en el cuerpo `0x140e06390–0x140e067e3`, la única lectura de `+0x8c0` (`mName`) está en `0x140e06670`, seguida de un `strcmp` inline contra `.rdata 0x141eb3174` = `"E_OC\0"` |
| 4 | `damage_check` no lee `AtAtp` ni el bit `0x10000` | **sí** | [S]. [B] ningún acceso a esos campos en el cuerpo |
| 5 | `setActionMode` no depende de `mName` | **sí** | [S] 584–630 |
| 6 | La última llamada ordinaria `setActionMode(DAMAGE)` (`0x140e067b9`), seguida solo del epílogo, es común a ambos recursos | **sí** | [B] (`POST05_NATIVE_REACTION_AUDIT.md`; el código no ramifica por recurso antes de ella) |
| 7 | Los offsets que restaura POST05 (`+0x8c8` modo, `+0x8cc` modo anterior, `+0x8d0` estado) y los que lee (`+0x8e0/+0x8e2`, `+0x69a`, bits de esfera `+0xce8/+0xd08/+0xea0/+0xec0`) son de la clase, no del recurso | **sí** | [S] layout de la clase. [C] `static_assert` de 0.5.0 compilados en MSVC |
| 8 | `AtAtp` (offset de `dCcD_Sph::mAt`) se puede leer sin escribir | **sí** | [S] `GetAtAtp()` es un getter. 0.5.1 lo lee igual que POST03 ya hacía en E_OC |

## Conclusión C1
**`C1_PASS` (fuente + binario).** El hit path y la ABI de E_OC2 son los de E_OC. `restore_reaction` sería mecánicamente idéntico.

Esto **no** autoriza a suprimir en E_OC2: con Expansion, E_OC2 es Red o Black, y su IA y su potencia dependen del modo que deja `damage_check` (C3/C4). Por eso 0.5.1 admite E_OC2 en POST03/POST04 (observación y postura) y le asigna `ALLOW_VANILLA / E_OC2_SUPPRESSION_DEFERRED`.

## Límites
- No se ejecutó el juego; el comportamiento de E_OC2 con el modo restaurado es **[P]**.
- El EXE de referencia es la copia local usada desde POST03.1 (mismo SHA-256 que el host del log real).
