# POST05 0.5.1 (POST05-COMPAT) — Runbook de prueba con todas las variantes de Bokoblin

**Estado:** `HUMAN_RUNTIME_PENDING`. Compila con MSVC y supera las auditorías de CI, pero **nadie lo ha cargado en Dusklight**. Una línea `SUPPRESSION_APPLIED` solo significa que tres campos se restauraron en memoria. No prueba que el Bokoblin siguiera atacando bien en pantalla: eso solo lo puedes ver tú.

- **Identificación:** ID `dev.posture.post05_selective_reactions`, versión **0.5.1**. Es el mismo ID que 0.5.0, que lo **reemplaza**.
- **Instalación exclusiva:** no cargues nunca juntos 0.5.0, 0.5.1, POST04, POST03.1, POST03, POST02 ni POST01. Todos enganchan el mismo método.
- **Base de comparación:** 0.5.0 (`f6c1a3c6…db068`) sigue disponible y no se ha tocado.

## Qué cambia respecto a 0.5.0
| Variante | 0.5.0 | 0.5.1 |
|---|---|---|
| E_OC nativo (sin Expansion) | supresión según la política | **igual** |
| **Blue** solo (Expansion, `AtAtp` 1/1) | supresión, pero al entrar en grupo quedaba bloqueado para siempre | supresión según la política; **ya no se bloquea** al entrar o salir de grupo |
| **Blue en grupo** (`AtAtp` 2/2) | nunca (`UNKNOWN_UNSAFE`, sin postura si nacía en grupo) | supresión según la política. El ataque continúa con la potencia 2 que pone Expansion |
| **Red** (E_OC2 nativo y Red de Expansion) | invisible (`E_OC2_EXCLUDED`) | **observado + postura pasiva, reacción NATIVA** (`E_OC2_SUPPRESSION_DEFERRED`) |
| **Black** normal (3/3) y en furia (6/6) | invisible | **observado + postura pasiva, reacción NATIVA** |

**Por qué Red y Black no se suprimen:** un Black golpeado entra en furia *después* de nuestro hook, así que su golpe en curso pasaría a potencia 6. La IA de Black y la del Red reactivo (con robo de vida) dependen del modo DAMAGE. Detalles en `05_RED_COMPAT_AUDIT.md` y `05_BLACK_COMPAT_AUDIT.md`.

## Artefacto
- **Enlace:** https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/41344244739/artifacts/13098463747 (ZIP `POST05C_TEST_41344244739_1`).
- **Instala solo** `POSTURE_POST05_1_COMPAT_ALL_BOKOBLINS_windows-amd64_ac99b4dfb141.dusk`: 32350 bytes, SHA-256 `f5be0ed2e00c196ce261db72756256fd5513229c4370ae4deee62ae6d90b83fb`.
- **Comprueba el SHA-256** con `Get-FileHash -Algorithm SHA256`. Si difiere, detente.
- **Caducidad:** 2026-12-01 19:57 UTC.
- **Más detalles:** `05_COMPAT_DELIVERY.md`.

## Instalar / quitar (juego cerrado)
1. Cierra Dusklight y respalda partidas y configuración como siempre.
2. Retira el `.dusk` de POST05 0.5.0 y de cualquier POST04/03.1/03/02/01.
3. Copia el `.dusk` de 0.5.1.
4. Para quitarlo: cierra el juego y borra el `.dusk`. Para volver a 0.5.0, cópialo de nuevo.

## Fase A — POST05 0.5.1 + Zora Swim (sin Expansion)
1. Arranca y busca en el log:
   - `POST05_INIT version=0.5.1 … hook_skip_original=0 … admission_policy=V2_COMPAT … e_oc2=OBSERVE_POSTURE_ONLY`;
   - `POST03_TARGET … inline_sites_flag=0`.
2. **T1 (E_OC nativo):** golpea Bokoblins nativos mientras atacan. Debe comportarse **igual que 0.5.0**: algunos `SUPPRESS_ELIGIBLE … SUPPRESSION_APPLIED … resource=E_OC atp=1>1`.
3. **T5 sin Expansion (E_OC2 vanilla), si encuentras uno:** las decisiones deben ser `ALLOW_VANILLA reason=E_OC2_SUPPRESSION_DEFERRED resource=E_OC2 atp=2>2` y el *flinch* debe ser normal.
4. Cierra por el menú.

## Fase B — 8/8 mods (solo si A no tuvo ningún STOP)
POST05 0.5.1 + HM CP11 ×2 + Bars CP06 + EVF 0.15.0 + Expansion 0.17.0 + Zora Swim + Luau + Randomizer.

| Caso | Qué hacer | Qué esperar en el log | Qué observar |
|---|---|---|---|
| **T2** Blue solo | espadazo normal mientras un Blue arranca o lanza su golpe, con postura < 75 | `SUPPRESS_ELIGIBLE … SUPPRESSION_APPLIED … resource=E_OC atp=1>1 admission=E_OC_CANDIDATE` | sigue atacando sin tambalearse; pierde HP; suena el impacto |
| **T3** Blue en grupo | lo mismo, con dos o más Blue juntos (Expansion registra `BLUE_GROUP_ON`) | `… resource=E_OC atp=2>2 …`, elegible o vanilla según la política. **No** debe aparecer `ADMISSION_EVER_UNSTABLE` por el grupo | el golpe en curso continúa con el daño de grupo de Expansion |
| **T4** transición de grupo | separa y junta a los Blue varias veces y golpea después | `POST03_ADMISSION_CHANGED` **no** debe aparecer para esos Blue | — |
| **T6** Red | golpea un Red mientras ataca y mientras está quieto | siempre `ALLOW_VANILLA reason=E_OC2_SUPPRESSION_DEFERRED resource=E_OC2 atp=2>2`; `POST04_POSTURE_HIT` con +25 | **tambaleo NORMAL** siempre. El robo de vida y la regeneración ×2 de Expansion, como antes |
| **T7** Black normal | primer golpe a un Black | `… resource=E_OC2 atp=3>3 … E_OC2_SUPPRESSION_DEFERRED` | tambaleo normal; Expansion registra `BLACK_FURY_START` |
| **T8** Black en furia | golpes siguientes durante la furia | `… atp=6>6 … E_OC2_SUPPRESSION_DEFERRED`; la postura sube y puede llegar a `POST04_BREAK_READY` | tambaleo normal; furia como antes |
| Ruptura | el 4.º espadazo ponderado (75→100) a cualquier variante | `CROSSES_THRESHOLD` o `E_OC2_SUPPRESSION_DEFERRED`; `POST04_BREAK_READY` | sin derribo (POST06 no existe) |
| Otras armas, letales, Finish | — | `FAMILY_NOT_SWORD`, `LETHAL` o sin decisión (Finish) | nativo |

**T9 (casos no verificados):** si ves cualquier línea con `reason=POWER_CHANGED_IN_CALL`, o una admisión `UNKNOWN_UNSAFE` en un Bokoblin de Expansion, anota el momento. No es un fallo de seguridad (no se escribe nada), pero indica un escritor que no hemos visto.

## Cómo describir lo que veas
Para cada caso elegible o dudoso, anota aproximadamente:
- el `event=` de la línea `POST05_DECISION`;
- la variante;
- qué hacía antes y qué hizo después;
- si te alcanzó y con cuánto daño.

Si puedes, graba un vídeo corto de los casos dudosos.

## STOP inmediato (cierra, retira el `.dusk` y envía el log completo por el chat; no lo subas a GitHub)
- **Estabilidad:** crash, excepción, hook parcial o `inline_sites_flag=1`.
- **Daño, HP y barras:**
  - cambio inexplicable de vida o daño, o doble daño a Link;
  - **cualquier supresión sobre un Red o un Black** (`SUPPRESSION_APPLIED` con `resource=E_OC2`): no debe ocurrir nunca;
  - barras de salud incoherentes.
- **Comportamiento:**
  - bucle de ataque o golpe repetido sin pausa;
  - Bokoblin congelado, vibrando, deslizándose o teletransportándose;
  - derribo o Finish sin causa nativa;
  - el escudo de Link deja de bloquear bien;
  - un Red o un Black sin *flinch* tras un golpe normal;
  - postura duplicada o heredada.
- **Rendimiento y log:**
  - pérdida de animación, sonido o efectos fuera del *flinch* esperado;
  - caída fuerte de FPS;
  - `ok=0` en cualquier conservación (incluida `POST05_CONSERVATION … r6_resources`);
  - rutas o direcciones en el log.

## Después de la prueba
- **Extracción de evidencia:** `COMPAT/tools/extract_post05_runtime.py` (lista blanca y escáner de privacidad; acepta los campos `resource/atp/admission` de 0.5.1).
- **`G2_PASS`:** solo se declarará si ves al menos un caso elegible de Blue (o E_OC nativo) que continúa su ataque con el daño correcto y sin anomalías, y si los casos no elegibles y **todos** los de Red y Black se comportan como en vanilla.
- **`ALL_BOKOBLINS_RUNTIME_PASS`:** exige además observar T1–T8 con logs.
