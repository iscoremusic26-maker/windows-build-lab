# CP09 — Windows build y descarga verificados
Dictamen técnico: **WINDOWS_BUILD_VERIFIED + PRIVATE_TEST_READY**.
Dictamen humano: **CISO_REAL_IMPORT_HUMAN_PENDING**; Codex no ejecutó Dusklight.

- Fuente realmente compilada: `b9ea7518684126b733532cb0a99a0e3b98d4d74e`.
- CI exacta: `04e87f713f1d96301d7ede29cda9628ba8fa84f9`; .NET SDK 8.0.426, net8.0-windows, self-contained win-x64.
- [Run final](https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/38918182460): build SUCCESS, independent-download SUCCESS.
- [Paquete privado](https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/38918182460/artifacts/12053167316), artifact 12053167316, DEVF_CP09_IMPORTER_TEST_38918182460.
- Caduca 2026-11-15 09:46:03 America/Panama (2026-11-15T14:46:03Z).

| Gate | Evidencia real | Resultado |
|---|---|---|
| Aplicación WinForms / lector CISO sparse e ISO / FST / catálogo / caché / cancelación | 54 casos importador ejecutados Windows, fixtures sintéticos legales | PASS de laboratorio |
| Regresión modo CP08 | 42 casos, ARC auténtico por hash, controles reales add/enable/yaw/drag/paint, guardado/backups, mapa↔mundo | PASS |
| Geometría real | R03_00.arc F_SP121/room3; 5326 triángulos completos, 100 en subconjunto histórico | PASS de lectura |
| Contraste independiente | Lector Python CP01 vs C#: alturas .002 unidades; tres referencias humanas redondeadas tolerancia .02 | PASS |
| Seguridad de formato | FST/rangos/traversal/duplicados, Yaz0 excesivo/truncado, RARC ciclos/alias, KCL/PLC corruptos, NaN, pisos superpuestos, caché manipulada/interrumpida | PASS de laboratorio |
| PE | AMD64/PE32+, 2 negativos de cabecera/truncado | PASS |
| Paquete / descarga independiente | Segundo runner descargó artifact; SHA256, bytes, CRC, PE y todos los blobs de SourceZip contra ledger | PASS |
| CISO comercial GZ2E01 completo | No disponible en CI ni leído por Codex | HUMAN_PENDING |
| Dos salas auténticas adicionales | No disponibles en inputs utilizados | NOT_TESTED |
| Exportar nuevas salas / runtime CP10 | Bloqueado; no implementado | OUT_OF_SCOPE |

Resumen de catálogo **sintético**: 1 stage T_TEST, 3 recursos de sala distintos; 2 mallas legales sintéticas disponibles, 1 recurso corrupto aislado. No representa cifras del juego.
Recursos auténticos procesados: **1 sala** (R03_00.arc), SHA256 dc53106667811c4cde18b35edb5279357f00ebecaef2f9de546aea9a5a2b06af. No se distribuyen ARC, KCL, PLC, mallas ni ROM.

Historial CI: run 38917466456 compiló y pasó 92 casos, pero falló empaquetado por KeyError source_ledger; no fue TEST_READY. Se corrigió el ledger y fijó SDK .NET8. Run38918110915 pasó build/descarga; el run final añade rechazo de alias RARC y su negativo. No se reutilizan hashes de builds previas.

CP08 humano histórico sigue limitado a tres Bokoblins reportados por el usuario. Esta CI no vuelve a ejecutar el juego ni amplía ese dictamen. Los .dusk proveedor0.8.0/Expansion0.2.0 y sus fuentes permanecen sin cambios.
