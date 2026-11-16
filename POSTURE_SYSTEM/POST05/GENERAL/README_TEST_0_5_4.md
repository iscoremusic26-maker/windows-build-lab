# POST05 0.5.4 (GENERAL) — prueba humana (10–15 min)

**Estado:** `HUMAN_RUNTIME_PENDING`. Compila y supera las auditorías del CI, pero nadie la ha cargado en el juego.

## Instalación (con el juego cerrado)
- **Descarga:** https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/41371589990/artifacts/13109308222
- **Instala solo** `POSTURE_POST05_4_GENERAL_PRIVATE_TEST_windows-amd64_3053a7511302.dusk`: 36965 bytes, SHA-256 `d3b7ddb63dc300bf55d42fca184bea9c12c72b3797c105b5e43337b2126d2a91`. Caduca el 2026-12-02.
- **Mismo ID:** retira 0.5.3 y cualquier POST05/POST04/03 anterior. **Nunca dos a la vez.**
- **Rollback a 0.5.3:** cierra el juego, borra el 0.5.4 y copia `POSTURE_POST05_3_FINALIZE_PRIVATE_TEST_windows-amd64_cf2fa7816f14.dusk` (SHA-256 `cf120e41d35faac8e21d069dc2140e0abd9e8b6782baf9464ce8e793791bae44`). Sin tocar partidas.

## Qué debería pasar
- **Postura por debajo de 100 tras tu golpe:** ningún Bokoblin se tambalea ni cae, esté quieto, caminando, persiguiendo, atacando o recuperándose. Pierde vida, suena el golpe y sube la postura.
  - Si estaba **quieto, caminando o vigilando**, hace la animación de **descubrirte** (la misma que cuando le das con el tirachinas) y empieza a perseguirte.
  - Si **perseguía o atacaba**, sigue haciéndolo.
  - Los **giros, saltos, el 4.º corte del combo y la bola** tampoco lo tumban por debajo de 100.
- **El golpe que llega a 100:** reacción nativa (tambaleo o caída). Es la ruptura provisional; **el derribo de POST06 no existe**.
- **Después, a 100 (latch):** reacción nativa en cada golpe.
- **Golpe letal y Finish:** muere normalmente.

## Fase A — POST05 0.5.4 + Zora Swim (5 min)
1. **Log de arranque:**
   - `POST05_INIT version=0.5.4 … stagger_policy=GENERAL_POSTURE_BELOW_100 … hook_skip_original=0`;
   - `POST03_TARGET … inline_sites_flag=0`.
2. **Red persiguiéndote (FIND):** 1, 2 y 3 espadazos. No debe tambalearse.
   - Log: `decision=SUPPRESS_ELIGIBLE … pre_mode=3 … action=SUPPRESSION_APPLIED plan=RESTORE_PRE`.
3. **Blue solo y quieto:** acércate por detrás y córtale.
   - Debe hacer la animación de **descubrirte**, no la de daño: `plan=NOTICE_FIND1`.
   - Dale 2 golpes más: no se tambalea.
4. **Golpes mientras ataca:** sigue atacando.
5. **4.º espadazo (postura 100):** reacción nativa (`CROSSES_THRESHOLD`). Un golpe más: `LATCHED_AFTER`.
6. **Remata a uno:** muere normalmente.
7. **Giratorio o salto con postura baja:** no lo tumba (`post_mode=6 … SUPPRESSION_APPLIED`).

## Fase B — los 8 mods (7 min)
- **Blue:** solo, en grupo y en un grupo que se disuelve.
- **Red:** nativo y de Expansion.
- **Black:** normal, justo al entrar en furia y ya en furia.
- **Escudo:** la furia del Black debe seguir encadenando y quitarte como mucho 1 chip por espadazo.
- **Velocidad x2 tras tu corte:** si cortas justo cuando el Red o el Black **empieza** el espadazo, ese espadazo puede salir a velocidad normal en vez de x2 (`compat=EXPANSION_RATE_NATIVE_THIS_BLOW`). Es el límite conocido; no debe tambalearse.

## STOP inmediato
Cierra el juego, vuelve a 0.5.3 y mándame el log por el chat si ves:
- **Estabilidad:** crash, `VERIFY_FAILED`, `RECHECK_FAILED`, `ok=0` o `inline_sites_flag=1`.
- **Vida y daño:** vida incoherente, doble daño, doble robo de vida o más de un chip.
- **Comportamiento:**
  - un Bokoblin **que no puede ser golpeado** después de un giro o la bola (colisión perdida);
  - ataque en bucle;
  - Bokoblin congelado, deslizándose o que no vuelve a actuar;
  - Bokoblin que se queda "pensando" sin perseguirte tras golpearlo quieto.
- **Rendimiento y audio:** voz o sonido en bucle, caída fuerte de FPS.

## Qué mandar
El log de cada fase y, para cada caso raro, qué variante, qué hacía y qué pasó.
