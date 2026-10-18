# Cobertura y evidencia
El conjunto operativo se calcula de CatalogService.Discover sobre el FST real GZ2E01rev0. La captura comunicada reporta variante00 de R00,R01,R02,R03,R04,R05,R06,R07,R09,R10,R11,R12,R13,R14,R15. Esto NO es una lectura propia de esos15 recursos; R08 no aparece y no se inventa. STG_00 es recurso de escenario.

|Room|Variante|Estado conocido aquí|Selección/preparación CP13|Suelo/QA|
|---|---|---|---|---|
|00|00|Reportada; estado exacto pendiente local|Sí si presente|Comprobar local; humano pendiente|
|01|00|Reportada; QA humana previa R01/L6 comunicada|Sí|Revalidación local; regresión CP13 pendiente|
|02|00|Reportada; estado exacto pendiente local|Sí|Comprobar local; humano pendiente|
|03|00|GEOMETRY_AVAILABLE, ARC auténtico disponible CI|Sí|R03/L6 humano previo; regresión CP13 pendiente|
|04|00|Reportada; estado exacto pendiente local|Sí|Comprobar local; humano pendiente|
|05|00|Reportada; estado exacto pendiente local|Sí|Comprobar local; humano pendiente|
|06|00|KCL_DEGENERATE reportado por captura|Sí, intención desactivada|Activación bloqueada; diagnóstico local necesario|
|07|00|Reportada; estado exacto pendiente local|Sí|Comprobar local; humano pendiente|
|09|00|Reportada; estado exacto pendiente local|Sí|Comprobar local; humano pendiente|
|10|00|Reportada; estado exacto pendiente local|Sí|Comprobar local; humano pendiente|
|11|00|Reportada; estado exacto pendiente local|Sí|Comprobar local; humano pendiente|
|12|00|Reportada; estado exacto pendiente local|Sí|Comprobar local; humano pendiente|
|13|00|Reportada; estado exacto pendiente local|Sí|Comprobar local; humano pendiente|
|14|00|Reportada; estado exacto pendiente local|Sí|Comprobar local; humano pendiente|
|15|00|Reportada; estado exacto pendiente local|Sí|Comprobar local; humano pendiente|
|STG|00|KCL ausente reportado, etiqueta truncada|Recurso separado en catálogo/reporte|No es sala; ninguna asociación compartida demostrada|

No se asigna GEOMETRY_AVAILABLE a una sala por su nombre. Exportar reporte F_SP121 inspecciona cada entrada auténtica local, KCL/PLC/DZR, hashes de recurso y causa exacta. No contiene ruta ni hash del disco ni bytes de geometría. Solo entonces se podrá actualizar esta matriz con evidencia por recurso. CI_SYNTHETIC_PASS para salas adicionales no equivale a REAL_GEOMETRY_VERIFIED.
