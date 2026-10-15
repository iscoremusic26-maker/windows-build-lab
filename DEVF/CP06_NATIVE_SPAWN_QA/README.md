# CP06 — dos QA nativos .dusk-only
Checkpoint de implementación: A=norte solamente; B=sitio1+norte, condicionado al PASS humano A. IDs distintos, sin hooks/HP/tinte/AI/JSON y sin correlación ActorId. Nada de host nuevo.
Registro BE32bytes E_oc/FFFF0100/pitch0106/setIDFFFF, F_SP121/3/6. Norte spawnY=floor+5 redondeado a float32; sitio1 intacto. StageService copia el registro; cleanup sólo retira ediciones propias, no promete destruir actores vivos. No reinyección/frame.
Lab compila el mismo registration.hpp de DLL, con fake sólo de la API StageService; no simula actores/juego. Build/PE/package/download aún pendientes, no TEST_READY inicial. B no se prueba antes de A.
