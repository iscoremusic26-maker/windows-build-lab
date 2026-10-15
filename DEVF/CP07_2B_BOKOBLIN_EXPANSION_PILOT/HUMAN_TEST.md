# QA humana nueva: A_ONLY → BOTH → retirar consumidor

1. Respaldar saves, placements.json, encounters.work.json y encounters.export.json. Cerrar Dusklight2.0.3/DX12. Retirar el proveedor0.7.2 y cualquier probe u otro DEVF. Instalar solo proveedor0.7.3 y UN Bokoblin Expansion0.1.0, con IDs distintos. No instalar DLL, fuentes o CLI como mods.
2. Poner placements.json del proveedor en NONE usando su ejemplo disabled.json y conservando el original. No cambiar los archivos encounters. Crear la carpeta de datos del consumidor si falta y copiar su ejemplo A_ONLY.json como qa.json. Ruta normal:
   `%APPDATA%\TwilitRealm\Dusklight\mod_data\dev.bokoblin.expansion\qa.json`.
   Si los datos están personalizados, usar la carpeta activa correspondiente a ese ID.
3. **Primer arranque, A_ONLY:** visitar F_SP121/room3/layer6. Buscar un E_OC2 nativo adicional en A=(5394.84716796875,-5603.69873046875,51442.3046875), yaw inicial-2421, y ningún añadido de Expansion en B. El enemigo puede moverse por su IA nativa. Estas capturas no acreditan suelo o ausencia de solapes. Conservar log y observación privada; detener ante crash, bloqueo o anomalía grave.
4. Cerrar completamente, sustituir qa.json por el ejemplo BOTH.json y reiniciar. Comprobar uno en A y otro en B=(3528.7822265625,-5850.8798828125,51201.09375), yaw inicial-18402; originales sin cambios y sin duplicados. Recorrer ambos puntos si no caben en pantalla. No afirmar simultaneidad visual si no se observó. Conservar log.
5. Cerrar, retirar SOLO Bokoblin Expansion y dejar proveedor0.7.3 con placements.json NONE. **Tercer arranque:** confirmar cero adicionales de Expansion y editor disponible. NONE.json permite además cero solicitudes sin retirar el consumidor, como prueba opcional.
6. Enviar logs, confirmación visual y observaciones de posición/conducta. Los eventos de registro no prueban píxeles ni ActorId. Si falla un caso, no continuar: cerrar, retirar ambos nuevos .dusk y restaurar SOLO proveedor0.7.2 y los archivos respaldados. No intentar borrar actores vivos desactivando mods en caliente.

No repetir las QA históricas. La apariencia, navegación, terreno y simultaneidad de este nuevo piloto siguen pendientes hasta tu reporte.
