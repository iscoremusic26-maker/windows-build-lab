# CP20 — auditoría de fuente

Baseline runtime: CP19 FIX02 `a0dd9e5a780c413a27fd165c9d3133b14d1e8613`, descendiente documental `ed56faaa2c718a6bf8b301881de96f02294c3015`. Provider/editor se copian de CP13 del árbol FIX02; se mantienen las carpetas históricas sin editar.

Dusklight exacto `40457c6adb381928e4b5fef6ed459ed291edd5e2`: d_a_e_oc.cpp blob `57828ccb7c987757929835da38623d8a6b04ebbd`; StageService header blob `9dbd94cff17f61716a5cf18e3ede9918cfe898c7`; mods/api.h blob `3015130ae3ceb41468723741902c257fc7c7fc8f`. Los tres se verificaron contra GitHub y la copia local. create() selecciona mName por byte alto de angle.x, rango por byte bajo; ambas variantes usan profile510. Archivo oficial sin modificaciones. CI fija también Aurora `3227d76c60e1e782ca576610bce61c9e7744d8be` y la import library oficial 2.0.3 auditada por FIX02.

`ci/source_gate.py` compara SHA256 de todos los runtime files FIX02 excepto parser/config IO/client y los dos headers de identidad adaptados y mod.cpp. En mod.cpp, la totalidad del contenido fuera de lectura de contexto, inicialización, import/version/globales, lookup tipado, causa de admisión y guard de firma Black debe coincidir byte a byte. Incluye intactos audio 0x5E/ARAM/gate, shield chip/flash, AFTER execute, HP/regen, daño, IA, hooks, render, sondeo y shutdown. El gate conserva los hashes de cada archivo congelado en la evidencia.

Las fuentes compiladas se fijan por Git blob en SOURCE_CI_CP20.json y por SHA256 en el ZIP. Se conserva el ledger histórico de CI FIX02: algunos laboratorios antiguos proceden de los snapshots históricos que ya fijaba esa CI (CP17 64 casos en su snapshot histórico, frente a66 del árbol fuente actual); no se sustituyen ni se rebajan assertions para cuadrar cifras.

Git local no pudo conectar al proxy del entorno. Se recuperaron archivos mediante el conector GitHub y se verificó cada Git blob antes de materializarlo; commits y ramas se publican por Git Data con lease, sin tocar ramas previas. La compilación Windows y descarga independiente se realizan en GitHub Actions.
