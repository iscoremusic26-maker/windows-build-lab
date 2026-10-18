# Recuperación CP16

Cerrar completamente Dusklight; retirar solo Expansion0.13.0 CP16 y restaurar el .dusk CP15 0.12.0 respaldado, 95511 bytes, SHA256 `7444fe51a1eeec8b050775e2991473db08fb3f770b3b7f3028dddbe334d7670d`. Restaurar JSON originales si se usaron encuentros temporales. No instalar dos Expansion ni dos Provider. Provider/editor CP13 no se sustituyen; no se requieren SDK, EXE, ROM o saves nuevos.

Baseline fuente CP15: `9fddfd9d445ade5d1ae5b4cc6f7d8380fc99f336`; cierre `933f249bbf422606d35dd8b55bf1afcbe30722f3`; CI `ea52ba92248a150dda0a74fe3d4b43f014bd27e4`. PR71/56 y su run39166668585 se preservan. No reset, amend, force, merge ni retag.

Reconstrucción CP16: usar SOURCE_CI_CP16.json de la rama CI, verificar source_commit y todos los blobs antes de CMake. Workflow Windows nuevo, exactos SDK40457c6/Aurora3227d76, importlib oficial por SHA; labs/auditoría y descarga independiente obligatorios. SourceZip/manifest identifican la fuente compilada, diferente de un cierre documental posterior. Nunca tomar una ejecución fallida como TEST_READY.

Sin estado persistido de reclutamiento, sin nuevas apariciones ni migración de encuentros. remove_actor_edit no elimina actores vivos. Reiniciar frío para el rollback. No iniciar CP17.
