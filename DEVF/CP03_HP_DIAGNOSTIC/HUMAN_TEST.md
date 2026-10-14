# Prueba humana v0.3.1 — Windows x64/DX12

Solo después de BUILD_PASS, PACKAGE_VERIFIED y DOWNLOAD_VERIFIED, con tu aprobación. Codex no ejecuta el juego ni instala en tu PC.

1. Cierra por completo y respalda saves/configuración. Verifica EXE oficial: SHA256 `ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049`; si difiere, detener.
2. Retira temporalmente CP03 v0.3.0, CP01/CP02 y mods de enemigos, stages, HP/hooks. Conserva v0.3.0 para reversión. Instala solo el nuevo .dusk v0.3.1 verificado; comparte module ID con v0.3.0.
3. Inicia Dusklight 2.0.3 con DirectX12, entra normalmente a F_SP121/sala3/capa6 en el punto probado. Log debe mostrar version=0.3.1, EDIT_REGISTERED y HP_APPLIED220→150 una vez por instancia.
4. Da varios golpes con arma/tipo de ataque lo más constantes posible; permite derrota normal. Conserva HP_DELTA: ActorId, hp_before/after, pérdida efectiva y cambio con signo. Cero UNKNOWN_OR_SPECIAL no atribuye ataque. No inferir potencia bruta, HP máximo o daño exacto por golpes/barra. Incrementos se observan, nunca se corrigen. OBSERVED_BETWEEN_EXECUTES no es un impacto individual demostrado.
5. Sal y vuelve naturalmente una vez: ID nuevo, otra HP_APPLIED y contadores desde1. Máximo32 deltas por instancia, luego un aviso de supresión; resumen contiene última lectura conocida. No esperar mensajes en frames sin cambio.
6. Detén y cierra ante HP_REJECTED/HP_DIAG_REJECTED, warnings unresolved/inlined/backend, duplicados, crash, comportamiento alterado o lectura incoherente. No hot reload. Envía log anonimizado y observaciones privadas al terminar.
7. Reversión: cierre completo, retirar v0.3.1, reiniciar y comprobar ausencia. La retirada previa de v0.3.0 es antecedente; esta versión requiere nueva evidencia. Para volver al baseline, otro proceso con solo v0.3.0.

Comparación opcional con CP02 E_OC2 de220: otra sesión completamente separada, misma arma/corte, nunca instalado junto a CP03. Un E_OC original de40 no es comparador equivalente. La instrumentación no registra ni modifica actores originales. No declarar CP03_DIAGNOSTIC_PASS_RUNTIME hasta revisar log real, combate y retirada; no CP04/CP06.
