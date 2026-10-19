# CP17 FIX01 — Red x2 · intercepción de saltos · Expansion 0.14.1 TEST privado

Sustituye **solo** la Expansion CP17 0.14.0. Provider/editor CP13 0.13.0, APIs, JSON y recursos del usuario permanecen intactos. Windows x64, Dusklight 2.0.3@40457c6, DX12. No instalar dos Expansion ni usar hot reload.

- Red clasificado (CP10.4): B/C nativos x1.15 normales; golpe rápido **exactamente x2** (decidido en el fotograma 1, aplicado en el 2: cinco registros AT nativos). x3 solo es historia de 0.14.0.
- Contacto x2: por tic, las dos esferas AT nativas cubren también la pose nativa saltada (una pasada, restauración automática por `cc_set`); escudo, daño y atribución CP14 siguen siendo nativos. Nunca se registra la pose 24.
- Prioridad ofensiva: 1 intercepción de **Jump Attack (PROC26)** y **Jump Strike (PROC30)** antes de aterrizar; 2 cargas Spin/Great Spin/Jump Strike (todo episodio elegible, sin tirada); 3 pasividad (~105); 4 aleatorio 12 %; 5 normal.
- Aproximación de reacción: ×1,5 exacto del desplazamiento nativo de FIND estado 4, con velocidad nativa restaurada tras `posMoveF`; medido en `RUSH_MEASURED`.
- Telemetría: muestras de Link solo en transición, reserva crítica, `SESSION_COUNTERS` + `CAUSE_SUMMARY`.
- Blue FIX01 (2800XZ/250Y, correa 100, llamadas, grito, gracia 10, bonus 650/250, tasas 1.15/1.30, lockouts 17/15), Black y Red CP14 (330 HP, 3 HP/s, aura/ojos, regen x2, lifesteal) sin cambios.

Estado: **SOURCE_IMPLEMENTED / LAB_PASS (local g++ ASan/UBSan: 53 casos FIX01 + 22/22 mutantes) / WINDOWS_MSVC_PENDING / INDEPENDENT_DOWNLOAD_PENDING / HUMAN_RUNTIME_PENDING**. El enlace del TEST se añadirá solo tras build + descarga independiente verificados.

Ver IMPLEMENTATION, HUMAN_TEST, RECOVERY y TEST_RESULTS. El canario CP14 sigue independiente y no se relaja.
