# POST05 — Supresión selectiva de la reacción ordinaria (TEST privado): runbook

**Estado:** `HUMAN_RUNTIME_PENDING` / `G2_PENDING`. Compila con MSVC y supera las auditorías de CI, pero **nadie lo ha cargado en Dusklight**. Una línea `SUPPRESSION_APPLIED` en el log **no** es una prueba visual: significa que tres campos se restauraron en memoria, no que el Bokoblin siguiera atacando bien.
ID `dev.posture.post05_selective_reactions` v0.5.0. **Reemplaza** a POST04, POST03.1, POST03, POST02 y POST01: nunca los cargues a la vez (todos enganchan el mismo método).

## Qué cambia respecto a POST04 (y qué no)
- **Cambia (solo esto):** un espadazo normal **no letal** que alcanza a un Bokoblin E_OC mientras **está atacando** (modo ATTACK, estados 0/1/2), y que no lleva su postura a 100, ya no inicia el *flinch* (la animación y la voz de daño). El Bokoblin debería seguir con su ataque.
- **No cambia:**
  - el HP perdido, el sonido del impacto, el *hit-stop* y el cooldown;
  - todos los golpes en otros momentos (quieto, caminando, buscándote, vigilando);
  - BIG_DAMAGE (giros, saltos, especiales) y las muertes;
  - el Finish;
  - el golpe que lleva la postura a 100 (sigue nativo y deja `POST04_BREAK_READY`) y los posteriores;
  - escudo, bumerán, bola de hierro, flechas, bombas y tirachinas;
  - los E_OC2 (rojos/negros) y cualquier Bokoblin cuya admisión haya sido inestable (p. ej. Blue).
- **Ruptura:** no hay derribo ni HUD. `BREAK_READY` sigue siendo solo una línea de log.

## Artefacto
- **Enlace directo (ZIP de GitHub con el .dusk):** https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/41337085240/artifacts/13095946275 (repo privado; la API confirma que el id `13095946275` pertenece al run `41337085240`, se llama `POST05_TEST_41337085240_1` y fue construido desde el espejo `84f0c5e`).
- Run: https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/41337085240 (`build` e `independent-download` en éxito).
- Comando opcional: `gh run download 41337085240 --repo iscoremusic26-maker/codex-cp00-inputs --name POST05_TEST_41337085240_1`
- El ZIP (32325 bytes, SHA-256 `a4735061055c523526e380a6e9b1d7f7f3532802294e42bc5083e8b2958ea9ad`) contiene `POSTURE_POST05_SELECTIVE_REACTIONS_windows-amd64_a9627a2373be.dusk` (**31313 bytes**), `DOWNLOAD_MANIFEST.json` y `SHA256SUMS.txt`. **Instala solo el .dusk.**
- SHA-256 del .dusk: `f6c1a3c642761aba92d23ecf2100b124c018befed498e10b463cbad93d0db068`. DLL interna: 65024 bytes, SHA-256 `5a0a297452195280eccc08ba678d910be41cae00d7f68f0e19ebafcad0fd164f`. Caduca el **2026-12-01 18:33 UTC**.
- Fuente compilada: `a9627a2373be3c61e744f875f65f4237d42e6121` (rama `feature/posture-post05-selective-reactions`). Host: Dusklight 2.0.3 (`dusklight.exe` `ffa99203…0049`).

## Instalar / quitar (juego cerrado)
1. Cierra Dusklight; respalda partidas/config como siempre.
2. **Retira el .dusk de POST04** (y cualquier POST03.1/03/02/01) de tu carpeta de mods.
3. Copia el .dusk de POST05 y comprueba su SHA-256 (`Get-FileHash -Algorithm SHA256`); si difiere, detente.
4. Quitar: cierra el juego y borra el .dusk.

## Fase A — POST05 + Zora Swim opcional (seguridad básica)
1. Arranca y busca en el log:
   - `POST05_INIT … writes_to_game=1 … hook_skip_original=0`;
   - `POST03_TARGET … resolve=MOD_OK … inline_sites_flag=0`;
   - `POST03_HOOK_INSTALLED`.
2. Golpea Bokoblins nativos en distintos momentos. Con 40 HP pueden morir de uno o dos golpes, y entonces no hay caso elegible. **Que no veas ninguna supresión en A no es un fallo**: el log dirá `reason=LETHAL` o `PRE_MODE_NOT_ATTACK`.
3. Si consigues golpear (sin matar) a un Bokoblin **justo cuando empieza o está dando su golpe**, observa qué hace.
4. Cierra por el menú.

## Fase B — 8/8 mods (solo si A no tuvo ningún STOP)
POST05 + HM CP11 ×2 + Bars CP06 + EVF 0.15.0 + Expansion 0.17.0 + Zora Swim + Luau + Randomizer. Con HM los Bokoblins aguantan varios golpes, así que es donde más fácil aparecerá un caso elegible.

Casos a buscar, en este orden, y qué esperar:
| Caso | Qué hacer | Esperado |
|---|---|---|
| **Elegible** | espadazo normal mientras el Bokoblin arranca o lanza su golpe, con postura <75 | `POST05_DECISION … decision=SUPPRESS_ELIGIBLE … action=SUPPRESSION_APPLIED`; en pantalla, **sigue atacando** sin el tambaleo, pierde HP, suena el impacto |
| No en ataque | golpe con el Bokoblin quieto, caminando o buscándote | tambaleo normal; `ALLOW_VANILLA reason=PRE_MODE_NOT_ATTACK` |
| Ruptura | el cuarto espadazo (75→100) | tambaleo normal + `POST04_BREAK_READY`; `reason=CROSSES_THRESHOLD`. Después, todos nativos (`NOT_WEIGHTED`) |
| Fuertes | giro, salto, golpe especial | BIG_DAMAGE nativo; `reason=BIG_DAMAGE` o `COOLDOWN_NOT_ORDINARY` |
| Otras armas | escudo, bumerán, bola de hierro | nativo; `FAMILY_NOT_SWORD` |
| Muerte / Finish | matar y rematar | nativo (`LETHAL` / sin decisión) |
| Blue / grupo | si aparece | con cuidado; puede salir `ADMISSION_EVER_UNSTABLE` (nunca se suprime) |

## Fase C — comparación controlada (opcional, muy útil)
Misma configuración de mods y una situación lo más parecida posible, primero con **POST04** y luego con **POST05** (nunca los dos a la vez). La única diferencia esperada es el caso elegible: sin tambaleo y con el ataque continuando. HP, sonidos, frecuencia de ataque y física deben ser iguales.

## Cómo describir lo que veas
Para cada caso elegible anota, aunque sea aproximado:
- el **momento** (tick o el `event=` de la línea `POST05_DECISION`);
- **qué hacía** el Bokoblin antes;
- **qué hizo** después: ¿completó el golpe?, ¿te alcanzó?, ¿se quedó quieto?, ¿repitió el golpe?, ¿se deslizó?, ¿cambió de animación de golpe?
- si el HP de la barra bajó.

Con eso se puede cruzar `(id, generation, event, pre_state, posture)` con el log. Si puedes, graba un vídeo corto solo de los casos dudosos.

## Líneas de log
`POST05_DECISION` (una por golpe confirmado), `POST05_SUMMARY`, `POST05_LAYOUT … MATCHES_EXE`, `POST05_CONSERVATION ok=1`. Además siguen `POST04_*` y `POST03_*` con sus conservaciones `ok=1`, y al salir «all mods unloaded».
- Si aparece `POST05_LAYOUT … MISMATCH_SUPPRESSION_CLOSED`, POST05 decidió no escribir nunca en esa sesión: es seguro, pero repórtalo.
- Si aparece `VERIFY_FAILED`, repórtalo también; la sesión se cierra sola.

## STOP inmediato (cierra, retira el .dusk y envía el log completo por el chat; no lo subas a GitHub)
- Crash.
- Cambio inexplicable de vida o de daño; doble daño a Link.
- Bucle de ataque o golpe repetido sin pausa.
- El escudo de Link deja de bloquear bien.
- Pérdida de animación, sonido o efectos fuera del tambaleo esperado.
- Bokoblin congelado, vibrando, deslizándose o teletransportándose.
- Derribo o Finish sin causa nativa.
- Barras de salud incoherentes.
- Caída fuerte de FPS.
- Cualquier cambio en E_OC2.
- Postura duplicada o heredada.
- `ok=0` en cualquier conservación.
- Hook parcial, `inline_sites_flag=1`, excepción.
- Rutas o direcciones en el log.

## Después de la prueba
La evidencia saneada se extraerá con `POST04/evidence_tools/extract_posture_runtime.py --module dev.posture.post05_selective_reactions --extra-tags POST05_` (lista blanca, reconstrucción POST03↔POST04 y escáner de privacidad) más el recuento de las líneas `POST05_DECISION`. `G2_PASS` solo se declarará si ves al menos un caso elegible que continúa su ataque con el daño correcto y sin anomalías, y los casos no elegibles se comportan como en POST04.
