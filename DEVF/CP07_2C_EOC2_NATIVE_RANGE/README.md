# CP07.2C — rango nativo finito E_OC2
Proveedor TEST privado 0.7.4, mismo ID y data_dir. Sólo los registros de encounter_placement@1.0 cambian angle.x de 0x0106 a 0x0132. Cada ACTR de 32 bytes difiere del anterior únicamente en byte 25: 06 → 32. Legacy FIELD/NORTH y native_placement@1.0 conservan 0x0106. Consumidor Bokoblin Expansion 0.1.0 intacto; no instalar otro consumidor.

## Evidencia
USER_REPORTED: el usuario probó A_ONLY, BOTH, combate y retirada en frío del consumidor; apariciones y ausencia posterior funcionaron. También observó regreso caminando a spawn. No se recibieron rawlogs aquí y no se atribuye su lectura a Codex.
SOURCE_VERIFIED: el código oficial interpreta el byte bajo como rango finito multiplicado por 100; el alto conserva E_OC2. No se cambia IA ni HIO.
HUMAN_RUNTIME_PENDING: determinar si aumenta el movimiento útil y evita la retirada prematura. executeWalk conserva regreso/patrullaje hacia home.pos. 5000 es un parámetro nativo, no persecución visual certificada.

[Fuente y alcance](SOURCE_AND_DIFF.md) · [QA](HUMAN_TEST.md) · [Recuperación](RECOVERY.md) · [Resultados](TEST_RESULTS.md).
