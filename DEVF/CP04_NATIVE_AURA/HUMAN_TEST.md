# Prueba controlada CP04 — Windows x64/DX12

Solo tras BUILD_PASS/PACKAGE_VERIFIED/DOWNLOAD_VERIFIED y aprobación del usuario. Codex no ejecuta ni instala. Este piloto cambia el marcador, no el cuerpo completo; resource safety gate real aún debe pasar en el juego.

1. Cierra por completo; respalda saves/config. EXE oficial SHA256 ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049. Si difiere, detener.
2. Conserva CP03 original fuera de mods. Instala **solo CP04**, sin CP01/02/03 o mods de enemigos/render/hooks. No recompilar ni cambiar recursos originales para hacerlo pasar.
3. Arranca DX12, entra naturalmente a F_SP121/sala3/capa6 en el punto CP03. Debe aparecer un adicional, HP_APPLIED220→150 y CP04_VISUAL_APPLIED con ActorId/key. ARMED solo anuncia registro; APPLIED es configuración leída, no prueba de pixels. Confirma visualmente un aura púrpura discreta, distinguible en reposo, marcha y combate, sin destellos molestos o equipo/sombras alterados.
4. Si CP04_VISUAL_REJECTED, HP_REJECTED, resource gate fallido, callback/unresolved/inlined/backend warning, crash, FPS anómalo, duplicación o marker invisible: STOP, cerrar y conservar log privado. No concede PASS ni se asume un CP04 funcional solo porque exista el actor. No hot reload o edición de common.jpc para desbloquear.
5. Prueba un **E_OC2 original real**, mismo modelo/recurso, sin marker/cambio de apariencia. Si no es accesible junto al piloto, verificar otra zona/sesión adecuada con solo CP04. Si no puedes hacerlo, marcar PENDING; E_OC normal40 no sirve como control visual equivalente. No reabrir las pruebas CP03.
6. Combate/muerte normales, HP_DELTA sin reset a150. Aura debe desaparecer al morir/retirarse el actor. Sal/reentra naturalmente: ID nuevo, un marker nuevo, sin acumulación; eventos APPLIED/RELEASED correlacionados. Pausas/eventos/coste amplio no se dan por probados.
7. Reversión: cerrar completamente, retirar CP04, reiniciar y volver. No adicional ni cambios visuales. Para recuperar CP03, otra sesión solo con su .dusk original11975 bytes/SHA0d9882b072fe5cd6248c16a181171a47364e1acf5870503c1eb2f39c668b4344.

Envía log anonimizado y observaciones privadas. CP04_APPEARANCE_RUNTIME_PASS exige apariencia distinguible, control E_OC2 original inalterado, combate, reentrada y reversión reales. No se concede por build/mocks o readback. No CP05/CP06.
