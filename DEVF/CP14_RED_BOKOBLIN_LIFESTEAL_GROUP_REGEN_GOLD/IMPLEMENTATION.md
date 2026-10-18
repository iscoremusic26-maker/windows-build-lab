# CP14 — Bokoblin Red: robo de vida, ventana de regeneración grupal y dorado

Expansion **0.11.0** = Expansion 0.10.0 (CP13) + Red. Provider 0.13.0 y editor 0.13.0 de CP13 **no cambian** y no se entregan de nuevo; la CI los recompila solo para regresión.
Motor: Dusklight 2.0.3, `TwilitRealm/dusklight@40457c6`. Etiquetas de evidencia: SOURCE (código), LAB, WINDOWS_MSVC, INDEPENDENT_DOWNLOAD, HUMAN.

## 1. Dónde vive cada cosa (base CP13, SOURCE)
| Pregunta | Lugar |
|---|---|
| Reconocimiento de Red | `CP10_4…/variant_policy.hpp` `make_plan`: E_OC2 nativo sin registro DEVF → RED. Registro DEVF Black → BLACK; registro DEVF Normal → BLUE (precedencia intacta). |
| HP / regen / daño por variante | `variant_policy.hpp` (Red 330 HP, 3 HP/s; Blue 220/2; Black HP de JSON/4), `damage_policy.hpp` + `dynamic_policy.hpp` (atp Red 2 fijo, nunca buff). |
| Bono de grupo Blue | `dynamic_policy.hpp` `find_companion`/`eligible`: otro Bokoblin clasificado vivo a ≤650 XZ y ≤250 Y, misma sala y escenario, ejecutado recientemente. |
| Furia Black | `dynamic_policy.hpp` `post_update`: pérdida real de HP no letal → 300 ticks. |
| Auras y ojos | `strokes_policy.hpp` (ruta única 0x0B9D, color por variante), `visual_policy.hpp` (fundidos, presupuesto 8), `variant_eyes.hpp` + `mod.cpp` (`GameFx`, variantes CMPR privadas del paquete del ojo). |
| Daño real a Link y atacante | Motor: `daAlink_c::checkDamageAction` (`d_a_alink_damage.inc`) — ver §3. |

## 2. Diseño de Red (`expansion/red_policy.hpp`, puro; el mod y el lab ejecutan el mismo código)
- **Robo de vida:** cada golpe efectivo cura el 25 % del **máximo** (330 → 82,5), con tope en el máximo.
  - **Fracción:** acumulador por generación en mili-HP. Cada golpe suma `max_hp × 250`; se cura la parte entera y el resto (<1 HP) pasa al siguiente golpe del mismo Red: 82, 83, 82, 83… (media exacta 82,5; 1000 golpes = 82 500 HP, LAB).
  - El tope solo recorta el HP escrito; el resto se conserva igualmente (determinista). Una generación nueva empieza en 0. Un HP por encima del máximo puesto por otro mod nunca se baja.
- **Ventana de regeneración grupal:** el mismo golpe abre o renueva 120 ticks propios del Red (4,000 s, base CP10.3).
  - Regla de ticks igual que la furia: el POST que aplica el golpe es el tick 0; cada POST posterior decrementa; el que llega a 0 la cierra. Bonus en los ticks 0..119.
  - Red x2 (3→6 HP/s). Todo otro Bokoblin clasificado vivo en el rango CP10.6 de ese Red (650 XZ / 250 Y, misma sala/escenario, snapshot reciente; `devf_dyn::eligible` sin cambios) x2 de su **propia** tasa mientras esté dentro: Blue 2→4, Red 3→6, Black 4→8.
  - Al salir del rango vuelve a x1 en el tick siguiente; si vuelve a entrar con la ventana abierta, x2 otra vez.
  - x2 es booleano: varios Red, un Red dentro del rango de otro Red o una renovación nunca dan más de x2 (LAB).
  - Una renovación reinicia a 120 sin apilar.
  - Red terminal (muerte, caída, agua, demo): cierra la ventana al instante y descarta curas pendientes.
  - Sin aliados cerca, la ventana se abre igual y solo beneficia al Red (caso de prueba exigido).
  - **Interpretación anotada:** el texto dice "cuando Red logre un golpe efectivo y haya Bokoblins cerca"; la lista de pruebas exige "Red solo: ventana activa y regen x2 solo para él". Se implementa la segunda: la primera lectura la contradiría.
- **Radio:** se reutiliza el del grupo Blue ya validado (650 XZ / 250 Y); no se cambia.
- **Visual:**
  - Aura del Red por la misma ruta compartida (0x0B9D, misma geometría, fundidos, reintentos y presupuesto de 8), color dorado (prm 255,190,30 / env 150,90,0).
  - Ojos dorados en el paquete del ojo del modelo del Red, por el mismo canal `step_eyes` que los ojos Blue. Siete variantes CMPR privadas generadas al clasificar al Red (nunca en combate), regla verde del atlas E_OC2 (la misma que los ojos rojos Black validados por humano); si no hay ojos verdes, se rechaza (sin "dorado falso").
  - Aura y ojos siguen exactamente a la ventana; al cerrarse, fundido de 15 ticks y textura nativa restaurada.
  - Los aliados beneficiados **no** reciben dorado (decisión acordada). Opción futura documentada: un indicador ligero en aliados.

## 3. Atribución del golpe efectivo a Link (SOURCE, motor 40457c6)
**Orden de una pasada de gestión:**
1. `fapGm_Execute` → `duskExecute` → `ModLoader::tick` (**mod_update**).
2. `fpcM_Management` ejecuta actores por línea: Link línea 5, E_OC línea 7, METER2 línea 12.
3. Draws: `dScnPly_Draw` → `dCcS::Move`. Ahí se calculan los golpes at/tg de esa pasada; los flags quedan hasta el siguiente `Move`, que los borra al empezar (`ClrAtHitInf`/`ClrTgHitInf`).

**Cómo aplica Link un golpe del `Move` N** (en su execute de la pasada N+1, `checkDamageAction`):
- `damageTimerCount` decrementa primero un temporizador en marcha.
- Si `mDamageTimer != 0`, ignora el golpe.
- Toma el **primer** `mTgCyls[i]` con golpe (`field_0x2fb8 == 0`). Su atacante es el registrado en **ese** cilindro.
- Bit tg-escudo → rama de guardia, sin daño.
- Si no: `setDamagePoint(mCcStts.GetDmg())` → `dComIfGp_setItemLifeCount(-dmg)` y `mDamageTimer = 30` (HIO por defecto). Con armadura mágica se cobran rupias, no vida.
- METER2 mueve después el contador pendiente a la vida guardada.

**Lo que hace CP14, sin hook nuevo y sin escribir nada en Link:**
- **Reclamo (mod_update N+1, después de `Move` N y antes de que Link procese).** Todas estas condiciones a la vez:
  - el cilindro que Link usará tiene un golpe no-escudo cuyo **colisionador atacante es una de las dos esferas de ataque** (`mSphs_at`) de un Red clasificado vivo (comparación de direcciones, nunca se sigue el puntero);
  - esa esfera tiene el golpe inverso sobre un cilindro de Link, también sin bit escudo;
  - `GetDmg() > 0`;
  - `mDamageTimer ≤ 1` (1 llega a 0 justo antes de `checkDamageAction`);
  - Link vivo;
  - sin contador de vida pendiente positivo.
- **Confirmación (mod_update N+2):**
  - mismo objeto Link;
  - `mDamageTimer` reiniciado (mayor que el valor reclamado);
  - vida efectiva (guardada + pendiente) bajó ≥ 1.
- **Aplicación:** solo entonces el Red recibe una cura pendiente, que se aplica en su POST (después del execute nativo, antes de la regeneración; ahí mismo se abre la ventana).
- **Canarios de layout (fallo cerrado):**
  - `mCcStts.GetAc() == Link` y los 3 cilindros apuntan a `mCcStts`;
  - en el Red: sus dos esferas apuntan a su `mStts` y `mStts.GetAc() == Red`;
  - temporizador en 0..150.
  - Si fallan, nunca se reclama (`LINK_LAYOUT_CANARY_FAILED_LIFESTEAL_DISABLED`, una línea).

**Nunca acreditado (LAB):** escudo (lado Link o lado Red), invulnerabilidad, armadura mágica (sin pérdida de vida), colisionador que no es de ese Red, otro enemigo en el primer cilindro, Red muerto/sustituido, reclamo sin caída real de vida o sin reinicio del temporizador.

**Rutas descartadas:**
- **Hook en `daAlink_c::checkDamageAction`/`setDamagePoint`:** añadiría un objetivo de hook en Link cuya resolución/inlining en el binario exacto no se puede verificar en CI. Si fallara, podría impedir cargar la Expansion o no dispararse nunca. Además no resuelve el único caso residual (daño de suelo en la misma llamada).
- **Solo el lado Red (`ChkAtHit`):** no demuestra que Link perdiera vida (escudo/invulnerabilidad/armadura) ni que Link eligiera ese golpe.
- **Solo la vida de Link:** no identifica al atacante.
- **`checkPlayerGuard` del propio E_OC:** es estado de guardia, no daño.

**Imports nuevos revisados** (de solo lectura; verificados en la `windows-amd64.lib` oficial fijada):
- `?dComIfGp_getLinkPlayer@@YAPEAVdaPy_py_c@@XZ`
- `?dComIfGs_getLife@@YAGXZ`
- `?dComIfGp_getItemLifeCount@@YAMXZ`

El resto de lecturas son accesores inline de las cabeceras del motor (`PARTIAL_DEBUG`: el ABI de structs del SDK es el del juego). Sigue habiendo **un** `DEFINE_HOOK` (`daE_OC_c::execute`, PRE+POST).

## 4. Cambios exactos (CI los comprueba)
- `runtime/DEVF/CP14_…/expansion/mod.cpp`: copia del `mod.cpp` CP13 con 18 líneas sustituidas (lista exacta en `ci/mod_cp13_replaced_lines.json`). Toda otra línea CP13 sigue presente y en orden, más el código Red (gate CI).
- `runtime/DEVF/CP14_…/expansion/red_policy.hpp`: nuevo.
- Parches literales (gate CI de reemplazo exacto sobre la copia CP13):
  - `strokes_policy.hpp`: `resource_for(Red)` = 0x0B9D y `kRedGoldColour`/`colour_for(Red)`. Blue/Black idénticos.
  - `visual_policy.hpp`: `Ledger::record` acepta Red.
  - `variant_policy.hpp`: `step(…, unsigned mult=1)`, `acc += rate*mult`. Con 1, igual a la regla CP10.3 (LAB: 4000 secuencias aleatorias).
- Las 18 copias runtime restantes: byte a byte iguales a CP13 (gate).
- `mod.json` 0.11.0. Provider, editor, JSON de encuentros, schema, ABI, servicios @1.0, capacidad 32 y presupuesto de 8 auras: sin cambios.

## 5. Límites y riesgos conocidos
- **Residual de atribución:** daño de suelo/eléctrico de Link en la **misma** pasada de 1/30 s que un golpe Red que Link no procesó podría acreditarse. Necesita esa coincidencia exacta; no detectable sin hook.
- **Ajuste de Dusklight "corazones infinitos":** restablece la vida antes de `mod_update`, así que no hay pérdida observable y no hay robo de vida. Correcto según "no reduce la vida".
- **Orden POST entre actores:** un aliado que ejecuta antes que el Red en la misma pasada ve la ventana con un tick de desfase (inicio/fin). Es 1/30 s.
- **Presupuesto de 8 auras compartido:** con 8 auras vivas, el Red no obtiene aura en esa generación (semántica CP10.6A existente: rechazo permanente por generación). La mecánica sigue activa.
- **Layout de `daAlink_c`/`daPy_py_c`:** compilado con las cabeceras del SDK oficial; verificado en runtime solo por los canarios. HUMAN_RUNTIME_PENDING.
- **La CI no ejecuta el juego:** golpe efectivo, cura, x2 y dorado son SOURCE + LAB hasta la prueba humana.
- **Pin:** el pin CP13 incluía `DEVF/CP10_2_BLACK_BOKOBLIN/SOURCE_CI_CP102.json`, que existe solo en el espejo CI. El pin CP14 lo excluye, junto con 3 documentos CP13 que cambiaron en 57b969d y no se compilan.
