# Pruebas reproducibles

Ejecutar desde RED_BLACK: `python tests/run_research.py --cp21 vendor/cp21 --out evidence --sanitize` (g++). Windows MSVC usa el mismo runner sin --sanitize.

[L] Nuevas: interaction_probe 1474 comprobaciones, shadow_lab 153613, 11/11 mutaciones detectadas. CP21 headers exactos, con manifiesto de siete blobs. Shadow tiene oráculo independiente de conjunciones y rejilla determinista de 153600 entradas. Los estados CP21 de prueba son sintéticos; no son gameplay.

[L] Baseline g++ ASAN+UBSAN: hit 1660; lifecycle 29558; collider 12826; posture 556878; reaction 36933; compat 55953; cero fallos. Mutaciones baseline 142/142. Source audit y PE baseline pasan. ASAN/UBSAN se ejecutaron con detect_leaks=0 porque LeakSanitizer aborta bajo ptrace en este executor; no atribuir comprobación de fugas a LSAN.

Corrección de prueba: faltaba include initializer_list en shadow_lab, corregido. Una guarda redundante de state>2 no aporta una mutación semántica independiente porque la comprobación B/C ya la rechaza; no se cuenta como mutación detectada. El rechazo guard3 sigue cubierto por oracle y baseline. La suite nueva no pretende cubrir las R01–R48 completas: Link HP, lifesteal efectivo, sweep, order callbacks, HM/Bars bajo nueva supresión, gameplay prolongado y carga de nueva DLL quedan pendientes. No mezclar las 142 mutaciones históricas con las 11 nuevas para afirmar auditoría de un mod nuevo.

El extractor histórico aborta en e_oc_atp=1|2. tests/extract_b2.py verifica SHA/bytes/líneas y exporta únicamente decisiones de esquema exacto y valores acotados. Recupera seis aplicaciones Blue y nueve EOC2; no registra nuevos campos de animación. Fixture real separado de contrafactuales sintéticos.

Primer CI Windows 41349926996 falló correctamente en hashes por checkout CRLF. Se corrigió exclusivamente core.autocrlf=false antes del checkout, sin relajar el manifiesto de hashes ni cambiar CP21. Reejecución 41350031195 en éxito, descarga independiente del artefacto 13101101529 verificada. MSVC coincide en todos los conteos nuevos.
