# Black: casos independientes

[S] CP21 mod.cpp :894 dynamic_post llama post_update; :1877 ocurre después de native execute; :1881 black_ai_post. Native action apaga AT antes de damage_check, decide el modo después; executeAttack activa AT en frames PRE 14..22; execute hace cc_set tras action. dynamic_post modifica los mismos objetos de esfera que cc_set registró, no una copia de potencia demostrada aquí. La fuente de cCcS.Move muestra CalcArea/ChkAtTg/ChkCo después de registro; falta cerrar el encadenamiento binario y el consumo posterior por Link.

| Caso | Código real ejercitado | Resultado | Estado |
|---|---|---|---|
| 3→6 con pérdida no letal | capture_pre + post_update | dos escrituras a esferas, fury=300 | STOP_POWER_POSTHOOK para propuesta |
| 6→6 con pérdida | post_update | refresh=300, cero escrituras de potencia | separado; candidato de fase madura |
| fury=1 con pérdida | post_update | renueva sin gap 6→3 | confirmado [L] |
| fury=1 sin pérdida | post_update | vuelve a3 | confirmado [L]; no extrapolar a gameplay |
| chain / fin B/C | black_ai.pre/post | no cubierto íntegramente por ensayo maduro | pendiente específico |
| shield chip flags persistentes | chip_step real, 10 generaciones ×100 repeticiones | un Charge y luego Duplicate por generación/episodio | dedup local probado; atribución física pendiente |

No declarar que el golpe en curso necesariamente llega con6 en todos los órdenes: la fuente muestra la posibilidad de modificar la esfera registrada, no una medición de daño de Link. El ensayo 6→6 elimina la transición 3→6 como causa, pero no certifica las poses de sweep, orden de contactos, rate pendiente ni identidad. Experimento siguiente: registrar PRE damage_check/POST damage_check/POST execute y resolución dCcS con identidad de episodio por valor; probar guardia y chain en gemelos, sin segundo escritor. Si no se demuestra orden de un hook observacional adicional, mantenerlo como propuesta.
