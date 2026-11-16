# POST05 0.5.2 (POST05-RB) — prueba humana corta (10–15 min)

**Estado:** `HUMAN_RUNTIME_PENDING`. La versión compila con MSVC y supera las auditorías del CI, pero **nadie la ha cargado en el juego**. Los laboratorios no prueban animaciones, contactos, sonido ni FPS. Para eso sirve esta prueba.

## Artefacto (TEST privado)
- **Descarga:** https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/41362833995/artifacts/13105797739 (ZIP `POST05RB_TEST_41362833995_1`, 35222 bytes)
- **Instala solo** `POSTURE_POST05_2_RB_PRIVATE_TEST_windows-amd64_e554fd1b4201.dusk`: **34128 bytes**, SHA-256 **`f39b88278ff864f71824f7ccbea27246f1039832c22ade163f4094df57145021`**.
- **Caducidad:** 2026-12-01 23:36 UTC.
- **Vuelta atrás:** el `.dusk` de 0.5.1 (`f5be0ed2e00c196ce261db72756256fd5513229c4370ae4deee62ae6d90b83fb`), que no se ha reemplazado.

## Instalación (con el juego cerrado)
1. **Guarda el `.dusk` de 0.5.1** (`f5be0ed2…83fb`). Es tu vuelta atrás.
2. **Retira de la carpeta de mods** el `.dusk` de POST05 0.5.1, el de 0.5.0 y cualquier POST04/03.1/03/02/01. Mismo ID: **nunca dos a la vez**.
3. **Copia solo** el `.dusk` 0.5.2 del artefacto y comprueba su SHA-256 con `Get-FileHash -Algorithm SHA256`. Si no coincide, no lo instales.
4. **Vuelta atrás:** cierra el juego, borra el 0.5.2 y vuelve a copiar el 0.5.1. No hay que tocar EXE, partidas ni configuración.

## Qué debería cambiar
| Variante | Se suprime el tambaleo (sigue su ataque) | Sigue vanilla (se tambalea) |
|---|---|---|
| Blue solo y en grupo, E_OC nativo | igual que en 0.5.1: ATTACK 0/1/2 | lo mismo que en 0.5.1 |
| **Red** (con o sin Expansion) | **en mitad del espadazo B o C** (desde el 2.º cuadro del clip hasta antes de que termine) y **mientras frena** para empezar a atacar | el **instante exacto en que arranca** el espadazo, el **primer cuadro** del clip, el **final** del clip, el rebote contra tu escudo |
| **Black normal** (3/3) | lo mismo que Red. El golpe que continúa lleva **ya la furia** (potencia 6), porque Expansion la activa con tu golpe | lo mismo que Red |
| **Black en furia** (6/6) | lo mismo que Red | lo mismo que Red, más el final del clip, donde Expansion encadena el siguiente ataque |

Siempre vanilla, como antes:
- golpe letal;
- cualquier arma que no sea espada;
- Finish y golpe que hace caer;
- el 4.º espadazo (la postura llega a 100), y **todo golpe posterior** mientras siga a 100.

**POST04 = 100 sigue enganchado y POST06 (derribo) no existe todavía.** Que deje de suprimir tras 4 espadazos **no es una regresión**.

## Pasos (unos 12 min)
1. **Arranque (1 min).** Busca en el log:
   - `POST05_INIT version=0.5.2 … hook_skip_original=0 … e2_policy=MATURE_BC_OR_BRAKE`;
   - `POST03_TARGET … inline_sites_flag=0`.
2. **Blue (2 min):** golpe normal durante su ataque, solo y en grupo. Debe comportarse **igual que en 0.5.1**.
3. **Red (4 min):** provoca varios espadazos suyos y córtale **en mitad del movimiento** (no justo cuando lo inicia). Hazlo 3 veces y luego deja que se recupere. Si encuentras un Red sin Expansion (Fase A, solo con Zora Swim), repite allí.
4. **Black normal (2 min):** primer golpe en mitad de su espadazo. Debería **seguir el ataque**, entrar en furia (aura de Expansion) y golpear con su fuerza de furia.
5. **Black en furia (2 min):** golpes en mitad de sus espadazos x2. Debería seguir el ataque; el final del clip y la cadena de furia son como siempre.
6. **Escudo (1 min):** bloquea un espadazo de un Black en furia tras haberle golpeado. El chip de escudo debe quitarte **como mucho 1** unidad por golpe suyo.
7. Cierra **desde el menú**.

## Qué se espera ver
- **En el log, cuando se suprime:** `POST05_DECISION … resource=E_OC2 … decision=SUPPRESS_ELIGIBLE reason=ELIGIBLE action=SUPPRESSION_APPLIED … phase=BC_MATURE` (o `phase=ATTACK0_BRAKE`). Los campos `frame=` y `rate=` dicen en qué cuadro y a qué velocidad iba.
- **En el log, cuando sigue vanilla:** `ALLOW_VANILLA reason=E_OC2_STARTUP_TICK | E_OC2_FRAME_EARLY | E_OC2_CLIP_ENDING | E_OC2_ANIM_UNVERIFIED | …`.
- **En pantalla:** el Red o el Black **termina su espadazo**, pierde vida, suena el impacto y, si te alcanza, recibes **un** golpe. El robo de vida del Red puede activarse **si su golpe te alcanza**; es correcto.

## STOP inmediato
Cierra el juego, vuelve a 0.5.1 y mándame el log por el chat, **no lo subas a GitHub**, si ves cualquiera de estas cosas:
- **Estabilidad:** crash, `inline_sites_flag=1`, `VERIFY_FAILED` o `RECHECK_FAILED`.
- **Animación y movimiento:**
  - Bokoblin congelado, vibrando, deslizándose o teletransportándose;
  - espadazo que se repite en bucle;
  - animación cortada o a saltos;
  - un espadazo que acelera o se frena de golpe a mitad.
- **Daño y vida:**
  - **doble daño** de un solo espadazo;
  - doble robo de vida del Red por un solo contacto;
  - **más de 1 chip** de escudo por golpe del Black;
  - vida o barras incoherentes.
- **Rendimiento y audio:** caída fuerte de FPS o audio roto.
- **Log:** `ok=0` en cualquier `…_CONSERVATION` o rutas/direcciones en el log.
- **Fuera de las fases previstas:** cualquier `SUPPRESSION_APPLIED` con `phase=` distinto de `BC_MATURE` o `ATTACK0_BRAKE` en un E_OC2.

## Qué mandarme
- El log completo de la sesión por el chat.
- Para cada caso raro: qué variante, qué hacía antes y después del golpe y, si puedes, la hora aproximada. Un vídeo corto ayuda.

## Qué decide cada cosa del log
| Hipótesis | Señal |
|---|---|
| La supresión E_OC2 ocurre en las fases previstas | `SUPPRESSION_APPLIED` + `phase=BC_MATURE` o `ATTACK0_BRAKE` + `resource=E_OC2` |
| No suprimió por la fase (correcto) | `E2_*` en `reason` + `frame`/`rate`/`speed` |
| No suprimió por la postura | `CROSSES_THRESHOLD`, `NOT_WEIGHTED` (latch) |
| El Black 3→6 continuó con furia | `atp=3>3` en la decisión y, después, `BLACK_FURY_START` de Expansion |
| Expansion mantuvo su tasa | líneas `DEVF_CP17_RED_AI` y `DEVF_CP18_BLACK_AI` con `rate`/`owned_rate` sin `RATE_OWNERSHIP_RELEASED` justo tras la supresión |

No se declara `G2_PASS` ni `POST05_RB_RUNTIME_PASS` hasta ver tu log y tu descripción.
