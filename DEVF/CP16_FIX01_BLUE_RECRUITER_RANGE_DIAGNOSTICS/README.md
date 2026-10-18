# CP16 FIX01 — Blue recruiter 0.13.1 TEST privado

Solo sustituye Bokoblin Expansion. Mantener Provider/editor CP13 0.13.0, JSON y datos del usuario. No instalar dos Expansion.

Deltas: convocatoria hasta2800 unidades XZ euclídeas inclusive, altura250, correa nativa propia con margen100; primera llamada y grito requieren detección nativa propia. Un combate ya iniciado puede reevaluar cada150 ejecuciones propias en FIND/ATTACK/FIND_STAY seguro sin LOS actual. No ofrece bonus: grupo650/250, niveles, daño, velocidades, lockout, aura y ojos permanecen iguales.

Diagnóstico limitado al ledger existente, no censo del nivel. Las ofertas no prueban marcha, llegada, sonido ni combate. Abortos ahora llevan causas diferenciadas. Una orden consumida puede conservarse hasta10 ejecuciones del receptor durante hit/DAMAGE ordinario del convocante; en la gracia no se escriben órdenes/targets/timers, no se prorrogan plazos ni progreso. Las salvaguardas duras siguen activas.

Fuente base documental1a61173bfe0e5da3e5e1144c687a399c78a0e660; CP16 realmente compilado e364fdb1489f7e61a0e0719a62fb627b5749f730. PR72 y CI57 quedan históricos. SDK40457c6adb381928e4b5fef6ed459ed291edd5e2.

Estado al implementar:53 casos FIX01 locales ASan/UBSan PASS (LeakSanitizer no ejecutable en este entorno ptrace). Build Windows/descarga pendientes hasta evidencia de Actions. FIX01 HUMAN_RUNTIME_PENDING. El PASS humano favorable corresponde a0.13.0, no a este TEST. Incidente Red LINK_LAYOUT_CANARY_FAILED_LIFESTEAL_DISABLED sigue pendiente.

Ver HUMAN_TEST.md y RECOVERY.md antes de instalar. No modifica host/SDK/ROM/saves, Provider, editor, API ni archivos JSON.
