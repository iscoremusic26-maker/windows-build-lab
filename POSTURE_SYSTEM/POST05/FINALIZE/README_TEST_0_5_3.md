# POST05 0.5.3 (FINALIZE) — prueba humana (10–15 min)

**Estado:** `HUMAN_RUNTIME_PENDING`. Compila y supera las auditorías del CI, pero **nadie la ha cargado en el juego**.

## Artefacto
- **Descarga:** https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/41366769060/artifacts/13107547306
- **`.dusk`:** `POSTURE_POST05_3_FINALIZE_PRIVATE_TEST_windows-amd64_cf2fa7816f14.dusk`, 35136 bytes, SHA-256 `cf120e41d35faac8e21d069dc2140e0abd9e8b6782baf9464ce8e793791bae44`.
- **Caducidad:** 2026-12-02.

- **Instala solo el `.dusk`.** Comprueba el SHA-256 con `Get-FileHash -Algorithm SHA256`; si no coincide, no lo instales.
- **Rollback a 0.5.2:** cierra el juego, borra el 0.5.3 y copia `POSTURE_POST05_2_RB_PRIVATE_TEST_windows-amd64_e554fd1b4201.dusk` (SHA-256 `f39b88278ff864f71824f7ccbea27246f1039832c22ade163f4094df57145021`). No hay que tocar partidas ni configuración.
- **Mismo ID:** **nunca** cargues 0.5.3 junto a 0.5.2, 0.5.1, 0.5.0 ni POST04/03/02/01.

## Qué cambia frente a 0.5.2
- **Sin Expansion (fase A):**
  - el Red ya **no se tambalea** si le das justo en el **primer cuadro** de su espadazo (el evento 2 de tu sesión aislada), justo cuando **arranca** el espadazo ni al **final** del clip;
  - en el log: `phase=FRAME_EARLY|STARTUP_TICK|CLIP_ENDING … env=NATIVE_ONLY compat=NATIVE_ONLY_NO_RATE_OWNER action=SUPPRESSION_APPLIED`.
- **Con Expansion (fase B):**
  - el **primer cuadro** de un espadazo normal x1,15 (Red o Black) y de un Black 3/3 **ya no interrumpe**: `compat=X1_15_ALREADY_OWNED` o `BLACK_BASE_NO_X2_PENDING`;
  - **siguen vanilla a propósito** el tick de arranque y el primer cuadro de un espadazo que puede ir a x2 (Red de reacción o aleatorio, Black en furia), para no perder el x2: `reason=E_OC2_STARTUP_TICK compat=RATE_DECISION_THIS_TICK` o `E_OC2_FRAME_EARLY compat=X2_PENDING_POSSIBLE`. Son 1–2 ticks por espadazo.
- **Sin cambios:**
  - Blue igual que 0.5.2;
  - el golpe que lleva la postura a 100 y los siguientes (latch) siguen vanilla: **POST06 (derribo) no existe**;
  - fuera de ataque (FIND corriendo hacia ti o retrocediendo, WALK…) se reacciona como siempre.

## Fase A — POST05 0.5.3 + Zora Swim (5 min)
1. **Arranque.** Busca en el log:
   - `POST05_INIT version=0.5.3 … hook_skip_original=0 …`;
   - `POST05_E2_ENV env=NATIVE_ONLY`;
   - `POST03_TARGET … inline_sites_flag=0`.
2. **Red.** Espera a que empiece a atacar y córtale lo antes posible (justo cuando levanta el arma), y también en mitad del movimiento. Debería **terminar su espadazo** mientras la postura sea 25, 50 o 75.
3. **Al 4.º espadazo** (postura 100) se tambalea: es lo esperado.
4. **Blue nativo:** como en 0.5.2.

## Fase B — los 8 mods (7 min)
1. Busca `POST05_E2_ENV env=EXPANSION_POSSIBLE`.
2. **Red y Black normal:** cortes en distintos momentos del espadazo. Deben continuar salvo en el instante exacto de arranque (y en el primer cuadro de un x2).
3. **Black en furia:** debe seguir atacando y **encadenar** como siempre. El x2 se nota en que el espadazo va claramente más rápido; tras un corte en mitad del golpe no debe volverse lento.
4. **Escudo** contra un Black en furia: como mucho **1** chip por espadazo suyo.
5. Cierra desde el menú.

## STOP inmediato
Cierra el juego, vuelve a 0.5.2 y mándame el log por el chat (no a GitHub) si ves:
- **Estabilidad:** crash, `inline_sites_flag=1`, `VERIFY_FAILED` o `RECHECK_FAILED`.
- **Log:** `ok=0` en cualquier `…_CONSERVATION`, o `env=NATIVE_ONLY` cuando Expansion **sí** está instalado.
- **Movimiento y animación:** espadazo en bucle, Bokoblin congelado, deslizándose o con animación a saltos.
- **Daño y vida:** doble daño, doble robo de vida o más de un chip.
- **Expansion:** un Black en furia o un Red reactivo cuyo espadazo pierde la velocidad x2 justo después de tu corte.
- **Rendimiento y audio:** caída fuerte de FPS o audio roto.

## Qué mandar
El log de cada fase. Para cada caso raro: qué variante, qué hacía, cuándo le cortaste y qué pasó.
