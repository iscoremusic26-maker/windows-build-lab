# POST05-COMPAT — C2: admisión dinámica de Blue (Expansion 0.17.0)

Etiquetas: **[R]** log B1 de 0.5.0 · **[S]** fuente (Expansion `192a773`, nativa) · **[L]** `compat_lab` · **[P]** pendiente.

## 1. El problema (0.5.0) [R]
- **Qué hace Expansion:** pone `AtAtp` 2/2 a un Blue cuando tiene un compañero cerca (`BLUE_GROUP_ON`) y lo devuelve a 1/1 al separarse (`BLUE_GROUP_OFF`). Lo hace en el POST de `execute`, sobre las dos esferas a la vez (`SphereIo`). [S]
- **Qué hacía 0.5.0:**
  - exigía `AtAtp == 1`, así que cada `BLUE_GROUP_ON` pasaba el actor a `UNKNOWN_UNSAFE`;
  - eso congelaba la postura (`ever_frozen`) y POST05 rechazaba esa generación para siempre;
  - un Blue que **naciera** en grupo ni siquiera recibía postura.
- **Reflejo en B1:** 10 `POST03_ADMISSION_CHANGED`, 5+5 eventos de grupo y el único golpe en ataque (evento 7) en `SKIP_UNSAFE / ADMISSION_EVER_UNSTABLE`.

## 2. Cambio de 0.5.1 [S]
**Política V2:** un E_OC con el bit `0x10000` y par en *lockstep* `(1,1)` o `(2,2)` es `E_OC_CANDIDATE`.

**Lo que no cambia:**
- par desigual (`-2`) o cualquier otro valor ⇒ `UNKNOWN_UNSAFE`;
- falta de `0x10000` ⇒ `UNKNOWN_UNSAFE`;
- la política de **reacción** de E_OC: las mismas reglas, motivos y precondiciones auxiliares de 0.5.0 (`policy_id` sin cambios).

**Nuevo motivo `POWER_CHANGED_IN_CALL`** (`SKIP_UNSAFE`): si el par cambia entre el PRE y el POST de nuestra llamada, o el recurso leído cambia, no se escribe. Expansion no escribe dentro de `damage_check` [S], así que con 0.17.0 no debería verse; si aparece, delata a un escritor desconocido.

## 3. ¿Es seguro suprimir el *flinch* de un Blue en grupo?
| Pregunta | Respuesta | Etiqueta |
|---|---|---|
| ¿Cambia la potencia del golpe en curso por suprimir? | **No.** Suprimir no toca `AtAtp`. El ataque continúa con la potencia que Expansion ya había puesto (2 en grupo). Si el grupo se deshace o se forma a mitad del golpe, Expansion cambia `AtAtp` igual que en vanilla, donde un Blue en grupo también ataca con 2 | [S] |
| ¿Reacciona Expansion al modo DAMAGE de un Blue? | Solo en dos sitios:<br>• la **gracia del llamador** del reclutamiento (`transient_caller`: DAMAGE 0–3/5 **o** golpe en modo de combate, que incluye ATTACK);<br>• el borrado del **indicador residual** de último golpe (`residual_clear`: cuenta 30 ticks en cualquier modo vivo distinto de DAMAGE).<br>Con la supresión, el primero sigue concediendo gracia (ATTACK + golpe); el segundo empieza a contar 30 ticks antes que en vanilla. **No hay escrituras de modo de Expansion en DAMAGE para Blue** | [S] `blue_recruit_policy.hpp:136-139`; `cp19_policy.hpp:121-128` |
| ¿Fija Expansion la velocidad de ataque de Blue? | Al **inicio** del golpe (`ATTACK_RATE_APPLIED`, estados B/C). La supresión mantiene el golpe en curso; la velocidad ya aplicada no cambia | [S] `mod.cpp:1103`; [R] B1: 3 líneas |
| ¿Qué ve el jugador? | Lo mismo que con un E_OC nativo en 0.5.0: el Blue sigue su ataque sin tambalearse. Si está en grupo, ese ataque tiene potencia 2 (diseño de Expansion, no de POST05) | [P] observación humana |

## 4. Pruebas [L]
- **T01–T12:** identidad. Incluye la rejilla exhaustiva V1/V2 contra el oráculo: 480 celdas × 3 comprobaciones, de las que solo 31 celdas difieren entre V1 y V2 (E_OC 2/2 con `0x10000` y todo E_OC2 vivo).
- **T13:** el patrón de B1 en V2 da 0 cambios de admisión. El golpe con 2/2 y el golpe tras volver a 1/1 son `ELIGIBLE` y se aplican.
- **T14:** el mismo patrón en V1 reproduce el bloqueo real (`ADMISSION_EVER_UNSTABLE`, máscara `ADMISSION`).
- **T15:** un desacuerdo de esferas marca la máscara `ADMISSION` y cierra la supresión.
- **T19:** potencia cambiada dentro de la llamada da `POWER_CHANGED_IN_CALL`.
- **T31/T32:** los 10 cambios de admisión de B1 (221×4, 222×4, 161×2) con V1 dan 10 eventos; con V2, 0. El evento 7 pasa de `ADMISSION_EVER_UNSTABLE` a `ELIGIBLE`.
- **T34:** un Blue que nace en grupo recibe postura en V2 y no en V1.
- **Estrés:**
  - 140 supresiones, 73 con `AtAtp` 2 y 14 `POWER_CHANGED_IN_CALL`, sin discrepancias con el oráculo;
  - las escrituras de Expansion se modelan **entre** llamadas (grupo y furia) y, rara vez, dentro de ella.

## 5. Veredicto C2
**`C2_PASS` en fuente y laboratorio.** Runtime **[P]**: con 0.5.1 cargado, B1 debería mostrar `POST03_ADMISSION_CHANGED = 0` para los Blue y decisiones de Blue en ataque distintas de `ADMISSION_EVER_UNSTABLE`.

**Límite [S]:** en el evento 7, que la decisión sea `ELIGIBLE` depende de precondiciones auxiliares (temporizadores 0x6c0/0x6c2, HeadLock, bits de esfera) que 0.5.0 no registró en esa ruta.
