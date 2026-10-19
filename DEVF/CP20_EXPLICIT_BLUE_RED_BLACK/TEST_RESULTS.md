# CP20 — pruebas automáticas

Resultados completos y contadores autoritativos: TEST_RESULTS.json, evidencia de Actions y download-result.json del segundo job. Todo laboratorio es offline; no constituye ejecución del juego.

Local: corpus compartido C#/C++ generado desde tests/matrix.py (85 archivos); C++ añade pruebas de API/bytes, seis identidades, aislamiento, generaciones, rollback, compatibilidad y daño. Se ejecutan además CP13, CP14 sobre headers integrados, CP15, CP16/FIX01, CP17/FIX01, CP18, CP19/FIX01/FIX02 y los30 casos originales de identidad/regeneración sobre headers CP20. El guard de fuente exige políticas congeladas idénticas. Windows: primero CI FIX02 completa sin alterar, después pareja CP20, ABI C/MSVC, COFF de SDK, .NET autocontenido, controles WinForms reales y ambos parsers sobre el mismo corpus. Export C#→decode/runtime/records C++ incluido.

La descarga independiente verifica ZIP publicado, hashes y bytes individuales, CRC, ambos .dusk, manifests, DLLs AMD64, imports exactos revisados, metadata de servicios, ambos hooks y binding a imports, exports, ausencia TLS/delay imports y ledger de fuentes. No se carga DLL del juego.

Matriz: Provider0.14+clienteV1 soportado con E_OC2; Provider0.14+Expansion0.16.3 soportado; Provider0.13+Expansion0.16.3 rechazado sin fallback; Expansion0.16.2+JSON legacy/Black válido soportado por decoder anterior; Expansion0.16.2+Blue/Red/mixed explícito rechazado atómicamente; editorCP20 Install rechaza pareja antigua o IDs duplicados.

Estados se reportan separados al terminar: SOURCE_IMPLEMENTED / LAB_PASS / WINDOWS_MSVC_PASS / INDEPENDENT_DOWNLOAD_PASS / HUMAN_RUNTIME_PENDING. No marcar los dos estados Windows/download como PASS hasta que ambos jobs terminen con éxito. Zora Swim, audio y gameplay conservan su necesidad de prueba humana real.
