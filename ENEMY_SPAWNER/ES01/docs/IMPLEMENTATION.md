# Implementación

- `src/catalog.hpp`, `src/spawner_core.hpp`: lógica pura (catálogo, pulsaciones, momento seguro, suelo, seguimiento, observación de Expansion).
- `src/spawner_adapter.inc`: lo que toca el motor. Lo incluyen `mod.cpp` y `tests/adapter_lab.cpp`.
- `src/mod.cpp`: servicios, interfaz, sonda de suelo y comprobaciones de tipos del SDK.

## Flujo
1. El botón solo anota la pulsación. Rechaza Black, Expansion ausente o desmentida, una pulsación ya pendiente y el límite.
2. Al principio del siguiente frame (`mod_update`), si la escena es segura (Link, capa válida, sin cambio de escena, evento ni pausa), se prueban puntos a 300, 220 y 160 delante de Link. Si se pasan 10 s sin un momento seguro, la pulsación caduca.
3. Con el primer punto válido, `create_actor` se llama con la capa de Link activa (regla de Dawnlight) y la capa anterior se restaura siempre.
4. El ID devuelto se sigue con `fopAcM_SearchByID` y `fpcM_IsCreating`, en lectura: solicitud enviada, aparecido o terminado. Nunca se borra un actor.
5. Bokoblin con Expansion: se leen `health` y `field_0x560` del actor creado. 40→220 es Blue y 220→330 es Red, la firma de Expansion 0.17.0. Si no aparece, se informa como nativo y las entradas de Expansion se desactivan en esa sesión.

## Escrituras nativas
- `fpcLy_SetCurrentLayer`: alrededor de una sola llamada a `create_actor`.
- Toadpoli: `mExecuteState` / `mActionTimer[3]` del actor creado por el mod, cada frame antes de su Execute.

No se escribe HP, IA, estado ni posición de ningún actor. No se instala ningún gancho.

## Compatibilidad
- Light Fades Twice: el Lizalfos creado es un E_DN nativo y LFT lo admite con su propio gancho, sin código de este mod.
- Bokoblin Expansion: Blue y Red los clasifica Expansion. Este mod solo lo observa.
- Detección: la presencia del servicio del Framework es necesaria pero no suficiente. La confirmación es lo que Expansion deja en un Bokoblin recién creado.
