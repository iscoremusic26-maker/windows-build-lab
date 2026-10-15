# Una QA humana nueva — API nativa

1. Respaldar saves y placements.json, cerrar completamente Dusklight2.0.3/DX12. Retirar0.7.0/0.6.x/A/B de mods, conservándolos fuera. Instalar sólo proveedor0.7.1 + probe0.1.0 (dos IDs distintos); nunca dos proveedores.
2. Poner examples/disabled.json como placements.json en el data_dir EXISTENTE del proveedor: normalmente %APPDATA%/TwilitRealm/Dusklight/mod_data/dev.devf.cp06_native_config_dual/. Si datos personalizados, usar carpeta activa/mod_data/ID. Guardar copia del JSON anterior; no instalar SourceZip ni DLL suelta.
3. Arranque frío con proveedor+probe y JSON NONE. Visitar F_SP121/sala3/capa6: observar UN adicional nativo en FIELD(-1250,-6304.00830078125,60750). Logs API_PUBLISHED/REQUEST_ACCEPTED/PROBE_REQUEST_ACCEPTED evidencian servicio/edición, no píxeles ni ActorId. Compartir privadamente log y observación. Si error/crash/duplicados, cerrar y detener.
4. Cerrar completamente, retirar sólo probe y reiniciar con proveedor y NONE: observar0adicionales. Editor queda disponible sin consumidor; no repetir su QA histórica. No pedir ActorId ni atribuir el ticket a PID.
5. Rollback: cerrar, retirar nuevo par y restaurar sólo proveedor0.7.0 histórico con su placements.json respaldado. No hot reload ni desactivar consumer/proveedor para simular eliminación de actor vivo. Backups del editor se conservan; remove sólo retira edición.
6. Opcional después de caso básico estable: JSON NORTH + probeFIELD registra2; JSON FIELD + probeFIELD rechaza consumidor sin duplicado. Esos escenarios están en laboratorio; no hace falta repetir cuatro modos humanosCP06 ni CP07.1 para la entrega.

El probe NO es Bokoblin Expansion. No HP/tintes/IA ni API de actores. Gameplay del nuevo par HUMAN_PENDING hasta reporte del usuario; CI no ejecuta el juego.
