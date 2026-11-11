# Resultados CP07.2A
Estado de trabajo BUILD_PENDING/HUMAN_PENDING; no gameplay de Codex.
Regresiones reales previstas:109coreCP06+55editorCP07.1+50broker/probeAPI. Misma ABI C11/C++20. Nuevos casos usan parser/serializer/Store/Session/Ui reales de DLL y dobles publicos File/Host/Ui; lector nativo compilado por separado, su contenido fisico Link queda HUMAN_PENDING.
Casos nuevos:multi-stage/16encuentros/nombres-ID unicos/UTF8-escapes/roundtrip/cantidad1..4/NaNInf/rangos/duplicadosclaves/overlimit/BOM/esquema invalido,guardar/reinicio/exportar/backup exclusivo,fallosIO/flush/close/lectura/rollback/conflictoexterno,pending capture/cancelacion/UIausente/noIOupdate,coleccion yeditor independientes.
MSVC/PE/GameABI/package/download se anotaran solo con runs ejecutados. No transferir PASS humano0.7.1 ni CP06 a captura0.7.2. No juego/loader real ejecutado en CI. Regresion de probe incluye cabecera/core sin cambiar y descarga/auditoria de probe binario anterior.
