# POST05-COMPAT — C3: Bokoblin Red (E_OC2 nativo y Red de Expansion)

Etiquetas: **[R]** log B1 de 0.5.0 · **[S]** fuente (nativa; Expansion 0.17.0 en `192a773`) · **[B]** binario · **[L]** laboratorio · **[P]** pendiente.

## 1. Identidad
- Con Expansion 0.17.0, **todo** E_OC2 nativo es un Red: 220→330 HP, `source=NATIVE_E_OC2`, y conserva `AtAtp` 2/2 nativo. [S] CP10.4; [R] B1: 158, 159, 160, 163 y 172.
- Sin Expansion, el mismo actor es el E_OC2 vanilla.
- Desde `damage_check` son **indistinguibles** (C0). Cualquier decisión sobre «Red» es una decisión sobre todo E_OC2 cuando Expansion está instalado.

## 2. Mecánica (C1)
La ruta de golpe y la ABI son idénticas a las de E_OC ([S]+[B], `05_EOC2_HIT_PATH_ABI_AUDIT.md`). Restaurar los 3 campos sería mecánicamente igual.

## 3. Por qué la supresión de Red queda DIFERIDA
| # | Hecho | Etiqueta | Riesgo si se suprimiera |
|---|---|---|---|
| R1 | **IA reactiva:** `REACTION_SELECTED`, `RUSH_STARTED` y `SPECIAL_CANCELLED` eligen embestidas, barridos y saltos según el estado de Link y del Red, y fuerzan ataques | [S] `mod.cpp:1181-1192`, `red_ai`; [R] B1: 9 `REACTION_SELECTED`, 7 `SPECIAL_CANCELLED`, 1 `RUSH_STARTED` | hoy el *flinch* es lo único que interrumpe un ataque forzado. Sin él, un Red golpeado mientras embiste seguiría embistiendo. Su combinación con la IA reactiva no está auditada ni observada |
| R2 | **Robo de vida y ventana de grupo:** cada golpe efectivo a Link cura al Red un 25 % (`RED_LIFESTEAL_APPLIED`) y abre una ventana de 120 ticks de regeneración ×2 para él y sus aliados | [S] `red_policy.hpp` 12–48; `red_step` (`mod.cpp:870`) escribe `health` en el POST | suprimir el *flinch* aumenta los golpes que llegan a Link, y por tanto la curación. Es un cambio de balance **acoplado al HP**, que POST05 promete no tocar |
| R3 | **Indistinguible del E_OC2 vanilla:** cualquier regla para «Red» afectaría también a partidas sin Expansion | [S] | doble alcance no pedido |
| R4 | **Sin línea base en ataque:** POST03 0.5.0 excluía E_OC2, así que no existe **ni un** `POST03_HIT_EVENT` de Red con PRE/POST | [R] B1 | no se puede comparar con vanilla. La única huella es de Expansion (`RED_HIT_REJECTED` y `HELM_SPLITTER` sobre 163) |

**Decisión 0.5.1:**
- todo golpe confirmado sobre E_OC2 da `ALLOW_VANILLA / E_OC2_SUPPRESSION_DEFERRED`;
- el Red **sí** se observa (`POST03_HIT_EVENT`) y **sí** acumula postura pasiva (POST04);
- la decisión registra `resource=E_OC2 atp=2>2 admission=E_OC2_CANDIDATE`.

Con eso, la próxima prueba humana da por fin la línea base de Red: en qué modo y estado recibe los golpes y cuántos *habrían* sido elegibles. La regla de diferimiento va **antes** que cualquier otra regla de política: lo fija `source_audit` y lo prueban las mutaciones C07, C08 y C19.

## 4. Pruebas [L]
- **T17:** Red en ATTACK 0/1/2: postura +25 por golpe, `E_OC2_SUPPRESSION_DEFERRED`, 0 escrituras y DAMAGE nativo.
- **T20:** el mismo Red en V1 queda excluido, como en 0.5.0.
- **T21:** cualquier señal de E_OC2 (admisión del ledger, recurso PRE o recurso POST) basta para diferir.
- **T22:** golpe letal y otras familias, también diferidos.
- **T23:** población mezclada, con contadores por recurso exactos y R6.
- **T24:** Blue DEVF sobre recurso E_OC2, diferido.
- **T29:** gemelo E_OC2, todos los campos idénticos con POST05 abierto o cerrado.
- **Estrés:** 25 174 decisiones de E_OC2, todas diferidas y sin escrituras. 133 de ellas *habrían* sido elegibles con la regla de E_OC: es la cobertura que demuestra que el diferimiento no es vacío.

## 5. Veredicto C3
`C3_SAFE_EXCLUSION`: el Red es **compatible** (observado, con postura y reacción vanilla). **No** hay supresión de Red.

Para abrirla haría falta:
- una auditoría de la IA reactiva de Red con el modo restaurado;
- una decisión explícita de balance sobre el robo de vida (POST09);
- logs humanos de línea base con 0.5.1.
