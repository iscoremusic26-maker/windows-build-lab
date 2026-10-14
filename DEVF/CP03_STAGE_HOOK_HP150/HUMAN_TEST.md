# Prueba humana posterior — Windows2.0.3/DX12

Solo después de BUILD_PASS/PACKAGE_VERIFIED/DOWNLOAD_VERIFIED y de tu aprobación del paquete. Codex no instala, ejecuta juego ni modifica tu PC.

1. Cierra completamente Dusklight. Conserva respaldo de saves/configuración. Verifica el EXE oficial con Get-FileHash: SHA256ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049. Otra compilación2.0.3 podría tener distinto inlining/ABI: detener.
2. Retira temporalmente CP01, CP02, G5 y demás DEVF; también otros mods de enemigos/stages/HP/hooks. Guarda los archivos sin borrar. Instala manualmente **solo** el `.dusk` CP03 verificado, no elZIP completo ni unaDLL suelta.
3. Inicia el ejecutable intacto con DirectX12 y entra naturalmente a F_SP121/room3/layer6, enelpunto probado delpuente hacia Kakariko. Sin teleport, flags, hot reload o cambios desaves por elmod.
4. Comprueba EDIT_REGISTERED y **una** HP_APPLIED porActorId/instancia, old_hp220/new_hp150/phasePRE_FIRST_EXECUTE/evidenceFIELD_READBACK. Esa lectura demuestra elcampo propio enese instante; no render/combate ni HPmáximo. Nativefield_0x560 no se modificó. Sin líneaHP_APPLIED, no concederPASS numérico.
5. Observa unBokoblin adicional conanimaciones, recursos/efectos ycombate normales. Tras daño no debe aparecer otraHP_APPLIED paraeseID ni recuperarse150 porframe. Un enemigo original debe conservar sucomportamiento; elcódigo/fake garantiza que unregistro que no coincide no se escribe, pero la prueba negativa humana queda pendiente. No medir150 únicamente por conteo degolpes/barra.
6. Detén ante HP_REJECTED, initfailure, warning de target unresolved/inlined, error deHookSystem, falta delmodelo, duplicado, comportamiento alterado, curación/crash o cambios enotros enemigos. Cierra yconserva log/capturas privados; no hotreload para «arreglarlo».
7. Para recarga natural desala, una nueva instancia puede tener nuevoID yuna nuevaHP_APPLIED. Dos instancias coincidentes vivas deben producir rejection, nunca una segunda escritura silenciosa. Es una prueba controlada posterior, no unPASS de laboratorio.
8. Reversión: cierra completamente, retira solo CP03, reinicia yvuelve naturalmente. Debe faltar eladicional. No asumir que quitar/desactivar enunproceso abierto borra elactor materializado. Reinstala tusmods anteriores solo enotra sesión sinCP03.

Puedes comparar CP02/CP03 enprocesos completos separados ycon arma constante, sin atribuir150 o un multiplicador dedaño a lacantidad degolpes. Envíanos ellog anonimizado yobservaciones; nunca publicar rutas personales/logs automáticamente. CP03 no se declara PASS_RUNTIME ni estable hasta revisar esa evidencia. No CP04.
