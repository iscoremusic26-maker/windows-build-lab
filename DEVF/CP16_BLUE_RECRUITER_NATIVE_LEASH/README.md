# CP16 — Blue Recruiter / Expansion 0.13.0 TEST privado

Reemplaza por completo la huida de CP15 por convocatoria nativa. Blue ofrece órdenes a Blue/Red/Black clasificados dentro de 1800 XZ / 250 Y, en stage/room/capa activa iguales y dentro de la correa propia del receptor (margen100). Cada receptor admite su orden en su propio execute PRE, usando `setActionMode(SOUND_WATCH,0)` y `mWatchPos`. No crea actores.

Blue da una oportunidad acotada al aliado antes de nuevos golpes. El grito inicial solicita WATCH3→4 / BCK7 / VOICE_CRY nativos una sola vez por combate. Movimiento, llegada, combate, audio y aspecto reales requieren QA humana. SOUND_WATCH puede adquirir a Link y pasar a FIND antes de llegar: ese evento no se llama llegada.

Se mantienen CP15 niveles1/2/3, tasas1.15/1.30 y bloqueos20/17/15; CP10.6 bonus650/250 solo por proximidad real; Red/Black y visuales anteriores sin cambios. CP14 `LINK_LAYOUT_CANARY_FAILED_LIFESTEAL_DISABLED` es una incidencia humana PREVIA conocida; CP16 no la resuelve ni certifica robo de vida.

Se entrega solo una Expansion nueva. Conservar Provider/editor CP13 0.13.0 y JSON propios; no instalar dos Expansion. Ver HUMAN_TEST.md y RECOVERY.md. No instalar hasta CI Windows y descarga independiente positivas; evidencia humana CP16 pendiente.
