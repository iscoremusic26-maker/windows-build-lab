# CP05 — Especificación mínima y diferencia vanilla

| Aspecto | Actor nativo con el record probado | Piloto sólo DEVF |
| --- | --- | --- |
| Percepción radial WAIT/WALK |5000, más FOV/LOS/sonido/pares|1200, mismos filtros nativos |
| FIND reconocido tras ejecución nativa dentro1200 |FSM nativa|Interest externo, rango1400 en scopes FIND |
| Link>=1400 durante interés |percepción5000|Hasta29 updates elegibles conserva5000; al30 pasa ReturnNative, rango1200 |
| Regreso a zona|WALK dirige home cuando>=500; leash600 conserva MOVE_OUT|Los mismos métodos, sin forzar acción/posición/velocidad; rearme externo sólo dentro500 y Link>=1200 |
| Ataque/daño/stun/muerte/demo/agua/caída/event/hit pendiente |Nativo|Sin override de rango en PRE; cancelación externa, sin alterar su acción |
| Fuera del scope execute |mPlayerRange5000|Restaurado5000 |
| Retail/otra identidad/generación/contexto |Nativo|Ningún write |

1200 y1400 provienen de HIO nativo;30 del timer nativo FIND/MOVE_OUT. No equivalen a garantía geométrica ni duración en segundos. 500 es el umbral de rumbo home nativo; mMoveRange600 no se cambia. El piloto es guardia más selectiva, no persecución más amplia que vanilla. Diferencia controlable: con Link visible y sin sonido/pares, a1300 el original admite detección radial mientras el guardia no la inicia por ese canal; tras interés cercano,1400/30 controla el desistimiento. Los disparadores nativos alternativos pueden entrar FIND y se conservan.

Estados externos Guard/Interest/ReturnNative, sin escribir mActionMode/mOcState. Interest sólo se anuncia después de observar FIND nativo dentro1200; AI_EVENT_OBSERVED no es animación verificada. ReturnNative indica retirada de la ampliación de interés, no que ya caminó ni llegó. Eventos acotados48/generación; log incluye identidad/epoch, acción, radios, contador y distancia cuadrada. No logs por frame.

Un snapshot fuera de contexto/recurso/creación no recibe writes. Contrato5000/600 divergente, NaN, reentrada seleccionada, missingPOST u otro writer => rechazo/error controlado fuera del callback. No rollback que sobrescriba un cambio ajeno. No dynamic allocation AI; null model/create impide override.
