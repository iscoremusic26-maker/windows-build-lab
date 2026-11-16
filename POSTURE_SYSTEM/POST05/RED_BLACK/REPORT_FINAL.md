# POST05-RB — informe de cierre de esta ejecución

**RESEARCH_ONLY. La extensión de gameplay solicitada sigue incompleta.** Se verificó A5 y se implementó una política shadow offline por fase. No se generó .dusk nuevo ni se habilitaron escrituras E_OC2. Esto no es un dictamen de imposibilidad: la fase madura sigue siendo una ruta prometedora que requiere cerrar pruebas de interacción.

## Resultado técnico

1. Recuperación: baseline fuente compilada ac99b4dfb1413355e3e935e9b22d311c40d84bee, HEAD documental 2621e53bfb92f923d7b19d5328d6ca39c276f79f, CP21 192a7731623d69baacdd37011db60ce894d9de18. Snapshot local 5ae33d3 distinto del upstream; ningún cambio previo de runtime se recuperó. Proxy Git CLI falló; se usó conector GitHub.
2. A5 [S][L]: pending hit hace pre_valid=false. En inicio de B/C omite x1.15; en frame1→2 borra pending sin aplicar x2. En episodio ya decidido/propietario, las pruebas mantienen rate y secuencia con hit persistente. No extrapolar esta premisa privada a un getter público de frame/rate.
3. Black [S][L]: CP10.6 inicia 3→6 después de nuestro hook; refresh 6→6 no reescribe potencia; furia1 con daño renueva300 y sin daño expira3. El chip real deduplica flags persistentes por episodio en la prueba. No se certificó atribución completa a Link ni orden binario de toda la colisión.
4. Red [S][P]: native y Expansion comparten firma visible. Se propone intersección de fases maduras; AtAtp/HP no identifican variante. Curación legítima tras ataque conservado no es motivo genérico de bloqueo. Lifesteal/rush/sweep y contactos gemelos siguen pendientes.
5. Pruebas: seis suites baseline pasan con conteos originales; 142/142 mutaciones baseline; nuevas 1474 interacciones, 153613 shadow y 11/11 mutaciones. ASAN+UBSAN local; LSAN deshabilitado por ptrace. Source audit y PE corresponden al baseline 0.5.1, no a una nueva DLL. Matriz R01–R48 completa aún pendiente.
6. Cambios exclusivamente bajo RED_BLACK: documentos, pruebas, siete headers CP21 exactos como fixtures, manifiesto de procedencia y evidencia saneada. Ningún cambio a src/mod.json/CMake/SDK/host/otros mods. Escritor e imports actuales: los de 0.5.1, sin adiciones. Shadow no entra en la compilación de la DLL.
7. GitHub: rama fuente feature/posture-post05-red-black-research; rama CI ci/posture-post05-red-black-research. Sin PR, merge, tag ni release. Metadatos finales de CI en DELIVERY.md y RESULTS.json.

La tabla de siete variantes y los gates H0–H11 están en POLICY_AND_WRITESET.md. El detalle de cada fase está en ATTACK_PHASE_AND_RATE_AUDIT.md. Runbook humano en README_TEST_RED_BLACK.md; ninguna nueva versión está autorizada por estos ensayos para instalarse.

## Paso que falta para abrir escritura

Ejercitar el adaptador completo en episodios maduros Red y Black6 con combinaciones de owned/pending/pose, callback PRE/POST y resolución de contactos reales del motor, incluyendo escudo/lifesteal/chip y generaciones. Verificar que un snapshot vivo, con ABI fijada, distingue o cubre de forma segura esos casos. Una consulta por valor a Expansion es propuesta opcional si no se puede demostrar la intersección sin ella; no está implementada ni se declara indispensable sin probar alternativas.

G2_PENDING, POST05_ALL_REACTIONS_RUNTIME_PENDING, HUMAN_RUNTIME_PENDING. POST06, ruptura física y reset del latch100 no se iniciaron.
