# Prueba humana posterior — Windows 2.0.3/DX12

Solo después de BUILD_PASS/PACKAGE_VERIFIED/DOWNLOAD_VERIFIED y de tu aprobación del paquete. Codex no instala, ejecuta juego ni modifica tu PC.

1. Cierra completamente Dusklight. Conserva respaldo de saves/configuración. Verifica el EXE oficial con Get-FileHash: SHA256 ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049. Otra compilación 2.0.3 podría tener distinto inlining/ABI: detener.
2. Retira temporalmente CP01, CP02, G5 y demás DEVF; también otros mods de enemigos/stages/HP/hooks. Guarda los archivos sin borrar. Instala manualmente **solo** el `.dusk` CP03 verificado, no elZIP completo ni una DLL suelta.
3. Inicia el ejecutable intacto con DirectX12 y entra naturalmente a F_SP121/room 3/layer 6, en el punto probado del puente hacia Kakariko. Sin teleport, flags, hot reload o cambios de saves por el mod.
4. Comprueba EDIT_REGISTERED y **una** HP_APPLIED por ActorId/instancia, old_hp=220 / new_hp=150 / phase=PRE_FIRST_EXECUTE / evidence=FIELD_READBACK. Esa lectura demuestra el campo propio en ese instante; no render/combate ni HP máximo. El campo nativo field_0x560 no se modificó. Sin línea HP_APPLIED, no concederPASS numérico.
5. Observa un Bokoblin adicional con animaciones, recursos/efectos ycombate normales. Tras daño no debe aparecer otra HP_APPLIED para ese ID ni recuperarse 150 por frame. Un enemigo original debe conservar su comportamiento; elcódigo/fake garantiza que un registro que no coincide no se escribe, pero la prueba negativa humana queda pendiente. No medir150 únicamente por conteo de golpes/barra.
6. Detén ante HP_REJECTED, init failure, warning de target unresolved/inlined, error deHookSystem, falta del modelo, duplicado, comportamiento alterado, curación/crash o cambios enotros enemigos. Cierra y conserva log/capturas privados; no hot reload para «arreglarlo».
7. Para recarga natural desala, una nueva instancia puede tener nuevo ID yuna nueva HP_APPLIED. Dos instancias coincidentes vivas deben producir rejection, nunca una segunda escritura silenciosa. Es una prueba controlada posterior, no unPASS de laboratorio.
8. Reversión: cierra completamente, retira solo CP03, reinicia y vuelve naturalmente. Debe faltar el adicional. No asumir que quitar/desactivar en un proceso abierto borra el actor materializado. Reinstala tus mods anteriores solo en otra sesión sin CP03.

Puedes comparar CP02/CP03 en procesos completos separados y con arma constante, sin atribuir150 o un multiplicador de daño a la cantidad de golpes. Envíanos el log anonimizado y observaciones; nunca publicar rutas personales/logs automáticamente. CP03 no se declara PASS_RUNTIME ni estable hasta revisar esa evidencia. No CP04.
