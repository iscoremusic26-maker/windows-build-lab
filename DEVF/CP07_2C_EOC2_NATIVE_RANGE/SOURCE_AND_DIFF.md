# Fuente y diferencial
Fuente oficial intacta: TwilitRealm/dusklight@40457c6adb381928e4b5fef6ed459ed291edd5e2, src/d/actor/d_a_e_oc.cpp.
- create L2711–2715: parameters FFFF0100 selecciona búsqueda M; HIO L90–109 define M=5000 y S=1200.
- create L2728–2738: low angle.x FF se normaliza a 0; mMoveRange=100*u_var. Alto 01 elige E_OC2. 06=600; 32 hexadecimal=50 => 5000.
- action L2420–2430: compara desplazamiento XZ desde home, rango+300 y rango−100; no es una garantía de distancia visible de persecución.
- executeMoveOut L2304–2395 y transiciones de executeFind: retirada nativa.
- executeWalk L983–1058: retorno/patrullaje independiente hacia home; puede permanecer.
- s_other_oc L205–224 usa getMoveRange en cooperación; se evita FF/rango cero.

Base exacta: 7c588d2a06d307b0288256fc2383186129430401. Cambios funcionales localizados: encounter_broker.hpp BE24=0132; mod.cpp versión informativa; provider/mod.json 0.7.4. Test expansión actualiza expectativa sólo del record nuevo. No cambia native_records.hpp, registration.hpp, consumidor, JSON/editor, APIs, bootstrap, capacidad 2, ownership ni dependencias.
No hooks, escrituras a actores, cambio global de IA/HIO, host/SDK/EXE/ROM/saves. SDK y biblioteca oficial sólo se leen intactos.
El serializer de encuentros conserva su etiqueta de formato/productor histórica 0.7.3 para no ampliar este cambio; no modifica el manifest real 0.7.4.
