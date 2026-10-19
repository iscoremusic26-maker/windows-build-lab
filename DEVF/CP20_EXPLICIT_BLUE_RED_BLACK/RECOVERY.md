# CP20 — recuperación

Juego completamente cerrado. Conservar los backups .backup-* del guardado y .conversion-*.backup.json de conversiones. Abrir/verificar el backup deseado y restaurarlo mediante acción explícita; conservar copia del archivo actual antes de reemplazarlo.

Para volver al baseline humano: retirar Provider0.14.0 y Expansion0.16.3 de la carpeta activa, restaurar exactamente los paquetes Provider0.13.0 y Expansion0.16.2 probados y el encounters.json legacy previo a conversiones. Usar editor0.13.0. No arrancar Expansion0.16.2 con JSON Blue/Red nuevo: lo rechazará. Nunca mantener dos Expansion ni dos Provider activos.

No se modifica save/ROM/EXE/SDK. Cambios solo en nuevos .dusk/editor y JSON cuando el usuario confirma guardar/instalar. Sin hot reload; comprobar desde arranque frío. Si un registro se aísla, conservar log y JSON, corregir causa en el editor y reiniciar; no hay reasignación tardía de identidad durante combate.
