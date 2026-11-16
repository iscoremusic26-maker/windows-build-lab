# POST05 — Auditoría de efectos de escritura

Etiquetas: **[S]** fuente · **[B]** binario · **[C]** compilación · **[L]** laboratorio · **[P]** pendiente. Base de evidencia: `POST05_NATIVE_REACTION_AUDIT.md`.

## 1. Conjunto completo de escrituras de POST05
`restore_reaction()` en `src/mod.cpp` es la **única** escritura del módulo en memoria del juego. Son tres asignaciones `int` a través del puntero `this` del hook, válido durante la llamada. El auditor de fuente rechaza cualquier otra escritura a través de `->`, cualquier otro campo y cualquier segunda llamada.

| Campo | Offset Win64 | Tipo | Valor escrito | Quién más lo escribe | Precondición |
|---|---|---|---|---|---|
| `daE_OC_c::mActionMode` | `+0x8c8` (`static_assert`) | `int` | el **modo PRE** de esta misma llamada (ATTACK=4) | `setActionMode` (55 llamadas en `d_a_e_oc.cpp`); Expansion vía `setActionMode` en `execute` | relectura inmediata = DAMAGE (5) |
| `daE_OC_c::mOcState` | `+0x8d0` | `int` | el **estado PRE** (0, 1 o 2) | `setActionMode` y las propias `execute*` | relectura = `my_val` del golpe (0..2) |
| `daE_OC_c::mOldActionMode` | `+0x8cc` | `int` | el **`old_mode` PRE** | solo `setActionMode` | relectura = modo PRE |

No se escribe nada más: ni HP, cooldown, colisión, animación, velocidad, audio, Link ni otro actor. No hay llamadas a métodos mutadores del juego, ni `HOOK_SKIP_ORIGINAL`, ni punteros guardados entre llamadas.

## 2. Por qué esto equivale a «no haber llamado» a la última `setActionMode`
[B] `setActionMode` hace, además de los tres campos:
- `u32 +0x8e0 = 0`;
- `orl $1` en los bits «set» Tg/Co de las dos esferas;
- `and $0xff7f, +0x69a`.

POST05 solo actúa si esos tres efectos fueron **no-ops** en esta llamada, comprobado con lecturas `const` PRE **y** POST:
- `field_0x6c0 == 0` y `field_0x6c2 == 0`;
- `checkHeadLockFlg() == 0`;
- `cCcD_ObjHitInf::ChkTgSet()` y `ChkCoSet()` a 1 en ambas esferas.

Con eso, restaurar los tres campos a su valor PRE deja la memoria del actor idéntica a la de un `damage_check` que hubiera omitido solo esa sentencia final. [L] Gemelos T16/T20 (mismo guion con POST05 abierto y cerrado): todos los campos modelados fuera de esos tres son idénticos (HP, cooldown, temporizadores, bits Tg/Co, HeadLock, collider y un campo testigo), igual que las cifras de POST03/POST04. Es una comparación del modelo, no del actor real completo.

**Comprobación sobre la DLL real [C] (run 41337085240):**
- Las lecturas `ChkTgSet/ChkCoSet` se compilaron exactamente sobre `+0xce8/+0xd08/+0xea0/+0xec0`.
- MSVC resolvió `layout_matches_binary` como verdadera en tiempo de compilación.
- Las únicas escrituras de la DLL a esos campos del actor son `mov %edx,0x8c8(%rax)`, `mov %r8d,0x8d0(%rax)` y `mov %r9d,0x8cc(%rax)`.

El layout de las esferas no se puede fijar con `static_assert`, porque los miembros son `protected`. Se comprueba en tiempo de ejecución con aritmética de direcciones de `GetObjTg()/GetObjCo()` (sin leer ni escribir) frente a `+0xce8/+0xd08/+0xea0/+0xec0`, más el vptr. **Si no coincide, la supresión se cierra para toda la sesión** (`POST05_LAYOUT … MISMATCH_SUPPRESSION_CLOSED`) y el mod sigue siendo pasivo.

## 3. Orden dentro del cuadro y lo que NO se deshace
| Efecto del golpe | Dónde ocurre | ¿Se conserva? |
|---|---|---|
| HP perdido | `cc_at_check` | sí |
| SE de impacto, hit-stop, marca de golpe | `cc_at_check` | sí |
| HM (reconciliación de HP) | hook `cc_at_check` | sí |
| cooldown `field_0x6cc = 10` | `damage_check` | sí (10 cuadros sin nuevo procesamiento, como en vanilla) |
| alternancia de corte `field_0x6d6/6dc`, `mStts.Move()` | `damage_check` | sí |
| clasificación POST03.1, consumo POST04, `BREAK_READY` | POST, antes de POST05 | sí, una vez |
| `setActionMode(DAMAGE)` | última sentencia | **deshecha** |
| `speedF=0`, animación y voz `V_DAMAGE_LR` del *flinch* | `executeDamage` (mismo `action()`, después) | **no ocurren**: corre `executeAttack` |

## 4. Rutas de fallo
| Caso | Resultado |
|---|---|
| Cualquier condición de política o de seguridad no cumplida | **no se escribe nada**; motivo contado (`ALLOW_VANILLA` / `SKIP_UNSAFE`) |
| Los tres campos cambiaron entre la lectura POST y la escritura | **no se escribe** (`RECHECK_FAILED_NO_WRITE`) |
| La relectura tras escribir no coincide | `VERIFY_FAILED`, aviso en log y **sesión cerrada** (no más intentos). No existe *rollback* adicional probado: por eso esta ruta cierra la supresión en vez de reintentar [P] |
| Layout distinto del EXE | sesión cerrada antes del primer intento |
| Gancho no instalado / excepción / observador deshabilitado | ninguna decisión ni escritura (el original sigue) |
| Actor retirado, generación distinta, informe duplicado | `TOKEN_MISMATCH` o duplicado: sin escritura |

## 5. Vida útil y concurrencia
La escritura ocurre en el hilo del juego, dentro del callback POST del mismo `damage_check` cuyo `this` es el actor (`hook.h`). No se guarda el puntero ni se escribe desde `mod_update`. Tampoco se lee ni escribe un actor retirado: la decisión exige que `valid(slot, generation)` y el id coincidan con el token del evento.

## 6. Lo que este documento no garantiza [P]
- Que `executeAttack` continúe de forma visualmente natural.
- Que no haya un doble golpe a Link o un bloqueo de IA.
- Que Expansion 0.17.0 / EVF 0.15.0 reaccionen bien a ver ATTACK donde esperaban DAMAGE.

Requiere la prueba humana (`README_TEST.md`).
