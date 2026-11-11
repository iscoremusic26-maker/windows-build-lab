# CP06 — Revisión incremental de imports CRT

Run38689862134 pasó30casos y ya no tiene TLS/delay import tras construir stage_record por valor. Auditoría estricta bloqueó las nuevas dependencias estándar del parser/std::string/vector/charconv.
Se acepta exactamente MSVCP140 std::_Xlength_error/_Xout_of_range; VCRUNTIME exception/throw/copy/destroy y memcmp/memmove; UCRT malloc/free/_callnewh, floor y registro de destructores globales/terminate. Son asignación, validación numérica y destrucción de contenedores propios, sin hooks manuales ni IO Win32 directo. La lectura externa sólo usa FileService.
La lista dusklight.exe sigue idéntica a CP05; API nueva FileService1.0 se verifica por metadata9records/6servicios y añade negativo de nombre de servicio. No se relaja allowlist, TLS, exports ni PMF→IAT. El MSVCP140 requerido es runtime MSVC; disponibilidad y carga efectiva en el equipo del usuario quedan pendientes de TEST, no se adjuntan librerías del SDK/juego.
Run38689650849: TLS por static local dinámico; se corrigió código local, no gate. No se distribuyó paquete listo desde runs bloqueados.
