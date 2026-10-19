# CP17 — resultados en curso

| Gate | Estado antes de CI | Evidencia |
|---|---|---|
| Fuente/política | SOURCE_IMPLEMENTED | Red separado,24 runtimefiles iguales, resto de mod.cpp preservado |
| Local LAB | PASS |64 casos ASan/UBSan; LeakSanitizer no ejecutado en entorno ptrace |
| Contacto/SFX port nativo | SOURCE_EXCERPT_LAB_ONLY | Defecto B12.5 reproducido; reparación condicionada; ventanas/traslación exactas del port |
| Windows MSVC/ABI/PE | PENDING | No sustituir por inspección |
| Descarga independiente | PENDING | No TEST_READY todavía |
| Cargas Link físicas | HUMAN_RUNTIME_PENDING | Layout/virtuales fuente no equivalen a lectura de juego observada |
| Combate, escudo, audio, rush e interrupción CP17 | HUMAN_RUNTIME_PENDING | Codex no ejecutó Dusklight |

64 casos: clasificación Red/Blue/Black; tasas B/C frescas/foráneas/clip/stop/recoil/restore; generación/contexto/64slots/semillas; PROC29/2A/2F vs28/30/virtuales; canario negativo;70%/12% reproducibles; cooldowns/anti-rachas;105 pasivo/guardia frente a ataque/dodge/daño/exploración; prioridad de carga, cancelación90, contexto/radio/Y/leash/LOS/pared; recepción SOUND_WATCH; contactos de fuente/ventana14–22; SFX B/C y compensación; presupuesto de logs.

El port usa duración40 como fixture legal, NO como duración medida de BCK. No incluye esqueleto/contacto físico/audio real ni prueba de1.5 de marcha (no implementado). No emitir ATTACK_X3_PASS humano por readback3.

Baseline humana FIX01 comunicada en el encargo: «Mejor. Ahora sí se nota», log185830 identificado por el usuario;47 calls,3 ofertas/consumos/inicios,2 arrived,10 recalls sin LOS actual,61 tasas y54 lockouts Blue. Se atribuye a confirmación humana+extractos comunicados, NO lectura directa del rawlog ausente. No prueba alcance exitoso>1800 ni Red CP17. Canary CP14 lifesteal sigue pendiente.

Primer run39182819992 FAILURE antes de configure/MSVC: source_gate usaba lectura Windows predeterminada y una cabecera UTF-8 produjo charmap. Se fija encoding UTF-8 explícito; no hubo paquete y descarga fue SKIPPED. La telemetría CP17 distingue además lectura PRE actual de NOT_SAMPLED_POST para no interpretar datos no capturados como ausencia de Link. No cambia gameplay.

Segundo run39183207051: configure y compilación/enlace de Expansion DLL correctos, pero msvc global FAILURE al enlazar el helper ABI EXE por GXDestroyTexObj arrastrado por cabeceras. Se reemplaza SOLO ese helper por un target OBJECT y auditoría del COFF real MSVC: sin juego/DLL cargados, sin stubs falsos ni SDK modificado. No paquete, independent-download SKIPPED.

Tercer run39183741446: MSVC SUCCESS y CP17 64/FIX01 53/CP16 56/CP15 38 PASS. Falló el lector COFF por off-by-one: el stub virtual auténtico de10 bytes (JMP indirecto en sus6 bytes finales) no se examinaba en el último índice permitido. Se corrige el límite manteniendo todas las verificaciones de slots/u16/layout y se agregan3 checks de lector (stub real/truncado/no-op). No se generó paquete, independent-download SKIPPED.
