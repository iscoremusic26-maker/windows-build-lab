# Prueba nueva JSON 0/1/2 — Windows/DX12

1. Respaldar saves y cerrar totalmente Dusklight. Retirar A/B, CP06v0.6.0 y otros DEVF a respaldo fuera de mods. Instalar sólo el nuevo .dusk dev.devf.cp06_native_config_dual0.6.2 verificado.
2. Crear la carpeta de configuración si hace falta y copiar un ejemplo como placements.json en `%APPDATA%\TwilitRealm\Dusklight\mod_data\dev.devf.cp06_native_config_dual\`. No instalar SourceZip ni DLL suelta. Sin JSON no aparecen adicionales.
3. Empezar con both-enabled.json, renombrado placements.json. Arrancar en frío y entrar F_SP121/sala3/capa6. Confirmar dos adicionales nativos en sitio1(-1250,-6304.00830078125,60750) y norte(-1250,-6322.6005859375,61950); no negro/HP150/guardia especial.
4. Para completar los cuatro casos: cerrar totalmente, reemplazar por field-only, reiniciar; repetir con north-only y disabled. No cambiar JSON con el juego abierto ni instalar varios DEVF.
5. Conservar log y observación por sesión. CONFIG_ACCEPTED/STAGE_EDIT_REGISTERED prueban configuración y altas, no píxeles ni ActorId individual. Si falla un caso o hay crash/duplicados/anomalías, detener y aportar privadamente log/descripción; no continuar ensayos de riesgo.
6. Rollback: cerrar el juego, retirar este .dusk y arrancar en frío. El JSON puede conservarse sin efecto. Si se desea, restaurar después sólo el CP06 histórico con su JSON. No hot unload ni promesa de destruir actores vivos al quitar una edición.

Ésta es prueba de configuración de la versión nueva, no repetición del estudio A/B ni de los cinco ensayos históricos. Codex no ejecutó el juego.
