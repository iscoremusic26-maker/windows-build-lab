# A5: fases y velocidad [S][L]

Fuente CP21 exacta indicada en vendor/CP21_MANIFEST.json. Este ensayo compila las funciones reales, no una reescritura de sus ramas. El adaptador de juego y el motor de colisiones no se ejecutan en el laboratorio.

Secuencia: Expansion PRE execute → red_ai_pre/black_ai_pre → native execute/action → PRE damage_check → original → POST POST05 → executeAttack/executeDamage → play → cc_set → Expansion dynamic_post → red_ai_post/black_ai_post. La fuente del gestor sitúa ejecución antes de dibujo; dScnPly_Draw llama Ccsp.Move. No se ha cerrado una prueba binaria completa de toda esa cadena y del consumo posterior por Link.

Red red_ai_policy.hpp: safe excluye hit (:107 aproximadamente; localizar por símbolo), pre :214 guarda pre_valid; post :368–372 borra pending ANTES de comprobar pre_valid para escribir x2; :374 conserva el episodio decidido y su rate si own_live; :377 rechaza inicio nuevo cuando pre_valid es falso. Black black_ai_policy.hpp: safe :41, pre :137, pending :229–233, continuidad :235–238, comienzo :242. mod.cpp red_ai_pre/post y black_ai_pre/post son los callsites efectivos.

| Variante/fase | Rate esperado/propietario | Impacto y resultado del ensayo | Gate |
|---|---|---|---|
| Red nativo sin Expansion ATTACK0 | nativo 1 | no pasa por CP21; cambio de modo mecánicamente posible | debe compartir intersección con Expansion |
| Red normal, inicio 0→1 | CP21 x1.15 | pending hit → pre_valid=false → no rate | ATTACK_RATE_INIT_PENDING |
| Black normal inicio 0→1 | CP21 x1.15 | mismo contraejemplo, además potencia futura | ATTACK_RATE_INIT_PENDING |
| Red especial / Black furia frame1→2 | CP21 x2 en POST frame2 | pending se borra y x2 no se aplica; sin retry de ese pending | FRAME12_RATE_UNVERIFIED |
| Red B/C maduro | rate ya poseído 1.15 o 2 | mantiene owned, rate y secuencia con hit verdadero o falso | candidato de fase; no permiso de escritura |
| Black B/C maduro, ya furia | rate ya poseído | mantiene episodio y rate; 6→6 no escribe potencia | candidato independiente de 3→6 |
| Blue nativo/Expansion | 1, 1.15 o 1.30 según grupo | política histórica no alterada; no inferir su propiedad de rate de la prueba Red | baseline LAB_PASS |
| ATTACK3 / guardia | reversa nativa | excluido de propuesta y política histórica | nativo |
| fin de clip / chain | tasa puede ser 0; PRE puede cambiar modo | fuera del ensayo maduro y del shadow candidato | ANIMATION_UNKNOWN |

A5-1/2: gemelos hit=false/true antes de setBck y POST frame1. A5-3: pending auténtico y POST frame2. A5-4/7: B/C, frames 3..29, rate 1.15/2, hit persistente o ausente. A5-5/6: número de episodio permanece 7 en esos casos; no prueba contactos físicos ni sesiones largas reales. No se probó todo posible estado privado de CP21. En particular, ni frame>2 ni leer rate=2 demuestra por sí solo owned=true, identidad de variante o validez de la pose previa del sweep.
