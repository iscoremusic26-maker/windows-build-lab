# CP03 v0.3.1 — diagnóstico privado de HP

Continuación incremental del piloto v0.3.0, checkpoint `76c53b6bcc61228df3b5a19f35b28fc777d4bde9`, sin cambiar su directorio, rama, tag o artefacto. El usuario reporta HP_APPLIED real para ActorId 147/581/784, combate y muerte normales. No existe todavía medición por impacto. La evidencia recibida se conserva como **USER_REPORTED**, sin pedir de nuevo los logs.

La política HP150 y el registro E_oc de F_SP121/sala3/capa6 son los mismos. Una única asignación tipada health=150; el nuevo observador solo lee. Añade POST al mismo execute mediante HookService oficial, con pares de ID/HP en pila fija y sin punteros persistentes. Empieza desde 150: no registra 220→150 como golpe.

HP_DELTA/HP_INCREASE_OBSERVED registran cambios netos efectivos, con signo, pérdida no negativa, ordinal y origen EXECUTE_WINDOW u OBSERVED_BETWEEN_EXECUTES. No mide ataque bruto ni atribuye espada/bomba; cero se marca UNKNOWN_OR_SPECIAL. Máximo 32 deltas por instancia, un aviso de supresión y resumen al cambiar de generación/salir. Frames sin cambio: cero eventos.

Estado de preparación: SOURCE_VERIFIED; compilación, pruebas nuevas, paquete y descarga pendientes. HUMAN_RUNTIME_NOT_TESTED para v0.3.1. No ActorService, hooks manuales, modificaciones del host/SDK/originales, AI, colisiones, saves o field_0x560. No CP04/CP06, instalación por Codex ni hot reload.

[Fuente y seguridad](SOURCE_REVIEW.md), [prueba humana y reversión](HUMAN_TEST.md). El module ID se conserva; instalar **solo un archivo** CP03, nunca v0.3.0 y v0.3.1 simultáneamente.
