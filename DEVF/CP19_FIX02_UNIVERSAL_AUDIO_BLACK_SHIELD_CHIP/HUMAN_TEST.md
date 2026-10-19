# CP19 FIX02 — prueba humana (Windows / DX12)

## Instalación (juego cerrado por completo)
1. Quita la Expansion 0.16.1 (y cualquier copia anterior). Instala solo `BokoblinExpansion_0.16.2_CP19_FIX02_TEST.dusk`. Nunca dos Expansion a la vez. Provider/editor 0.13.0 y los JSON no cambian.
2. Arranque en frío. El log está en la carpeta de logs de Dusklight de siempre (`dusklight-AAAAMMDD-HHMMSS.log`). Al compartirlo, súbelo como evidencia, como las veces anteriores.

## Qué debe aparecer en el log
- Al iniciar: `SHIELD_CHIP_ARMED` (si aparece `SHIELD_CHIP_ARMED_FLASH_HOOK_UNAVAILABLE`, el destello no está activo: anótalo).
- Después de `AUDIO_READY`:
  - `AUDIO_DEPENDENCY_IDENTIFIED`, `AUDIO_RESERVATION_ACQUIRED`, `AUDIO_LOAD_REQUESTED` y `AUDIO_LOAD_ACCEPTED`;
  - después `AUDIO_LOAD_READY` y `AUDIO_NATIVE_REFUSAL_CAUSE_MEASURED`, que lleva el hueco libre del heap de audio y el tamaño del grupo.
- Si sale `AUDIO_RESERVATION_REFUSED`, `AUDIO_LOAD_REFUSED` o `AUDIO_LOAD_GAVE_UP`, para y envía el log.

## A. Sonidos (marca sí/no y la sala)
1. **F_SP121 sala 10, capa 6, Black 003/004/005.** ¿Oyes voces (espera, alerta, grito), pasos, ataques, golpes, daño y muerte? ¿Suenan desde la posición del Bokoblin, más bajo al alejarte?
2. **Sala 7, Black 001** (cuando aparezca): las mismas preguntas.
3. **Una sala con Bokoblins nativos** (p. ej. 2 o 3): ¿siguen sonando ellos y el ambiente igual que antes?
4. **Otras salas sin Bokoblins de fábrica** (0, 5, 9, 11, 12): ¿suena el ambiente de la sala? Si DEVF tiene encuentros ahí, ¿suenan los Bokoblin?
5. **Otro escenario o mazmorra** con encuentros DEVF, solo si el editor los permite. Si no, queda **PENDIENTE**: no se declara universal.
6. **Ciclo completo:**
   - aparición y desaparición, muerte, varios a la vez;
   - salir a otra sala y volver;
   - volver al menú y recargar, cerrar y abrir en frío.

   ¿Algún sonido de sala o música se pierde?

## B. Escudo contra Black en furia
Mira los corazones y el modelo de Link.

1. **Black sin furia:** bloquea un golpe. Esperado: **no** pierdes vida por el escudo y no aparece `SHIELD_CHIP_APPLIED`.
2. **Furia (aura y ojos morados):** bloquea un espadazo. Esperado:
   - pierdes **exactamente 1/4 de corazón**;
   - Link se pone **rojo** brevemente;
   - sigue con la guardia y puede bloquear el siguiente golpe enseguida;
   - no hay animación de daño.

   En el log: `SHIELD_CHIP_APPLIED` y luego `SHIELD_CHIP_METER_READBACK reason=LIFE_DROPPED_EXACTLY_ONE_UNIT`.
3. **Cadena de 2 golpes bloqueados:** −2/4. **Dos Black:** −1/4 por cada golpe.
4. **Multiplicador de daño** de Dusklight en x1, x5 y x8 (y x2/x10 si quieres): **siempre −1/4** por golpe bloqueado.
5. **Armaduras:** Ropas del Héroe, Armadura Zora y Armadura Mágica (con rupias y con sus modos de Dusklight): **siempre −1/4** y destello rojo visible. ¿Se te descuentan rupias por el bloqueo? Esperado: no.
6. **Nunca debe quitar vida extra:** golpe recibido en el cuerpo (daño normal, sin el 1/4 extra), escudo bajado, golpe al aire, Red y Blue.
7. **Con 1/4 de corazón:** bloquea en furia. Esperado: muerte nativa (o hada si tienes), sin vida negativa ni cuelgue.
8. **Transiciones:** cambio de sala, guardar/cargar, cerrar el juego. Sin cuelgues.

Separa lo que viste de lo que dice el log. Estado: HUMAN_RUNTIME_PENDING (A y B).
