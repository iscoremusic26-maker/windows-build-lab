# DEVF Map Editor CP09 0.9.0 TEST
Importador local GameCube USA GZ2E01, disco 0, revisión 0. Windows x64 autónomo, sin Dolphin, Python ni compilador en el PC del usuario.

1. Descomprima el ZIP privado en una carpeta propia. Mantenga el editor CP08 como respaldo y los dos mods CP08 **sin cambios**.
2. Abra **DEVF Map Editor CP09.exe** → **Importar juego / Catálogo** → **Importar juego CISO / ISO**. Seleccione su copia legítima.
3. Espere el hash y el catálogo. Puede cancelar; la imagen se abre exclusivamente para lectura. La ruta se recuerda solo en su PC.
4. Seleccione una sala del árbol. Rueda para zoom, arrastre para desplazar, Encuadrar para regresar. No necesita recorrer el juego con Link.
5. Las salas nuevas son **solo visualización**. No exportan encuentros. Solo el R03_00.arc exacto de CP08 ofrece **Editar en modo CP08**, con las restricciones históricas intactas.
6. También puede usar **Importar mapa ARC** y abrir su encounters.json existente como antes. Respáldelo antes de guardar.

La primera importación calcula SHA256 de toda la imagen y procesa los ARC de escenarios secuencialmente. Guarda únicamente metadatos en LocalAppData/DEVF/MapEditor/CP09; la geometría se carga de nuevo desde la imagen cuando selecciona una sala. No incluye ni redistribuye datos del juego.

Soporta CISO sparse GameCube (cabecera 0x8000, tabla de bloques), ISO/GCM. No admite CSO PSP, RVZ, WIA, Wii, otras regiones ni revisiones no verificadas. Rechazo claro antes de importar. El catálogo conserva por separado cada variante ARC. Room procede del nombre observado, con atribución de inferencia; habitaciones PLC y chunks de capa se registran aparte. El sufijo del ARC **no demuestra una capa**.

Estado de ejecución real: **CISO_REAL_IMPORT_HUMAN_PENDING**. El CI prueba formatos sintéticos legales y el R03_00.arc auténtico privado, no un disco comercial completo. Superficies y colores del visor no certifican terreno seguro, navegación o spawn. CP10 no implementado.
