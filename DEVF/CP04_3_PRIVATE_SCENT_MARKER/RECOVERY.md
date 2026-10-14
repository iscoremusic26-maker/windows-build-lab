# CP04.3 — Recuperación exacta, sin pérdida ni falsas garantías
Cloud devolvió estado actual failed/offline, failure.code=environment_startup_failed. Una espera de readiness devolvió ready pero la siguiente operación real falló "exec-server transport closed"; exec pwd quedó pendiente y recheck actual confirma failed. Esta es evidencia externa concreta. GitHub app siguió accesible con repositorio privado/push disponibles y permite publicar este checkpoint documental.

## Resultado publicado
Nuevo commit de informes derivados y borrador propio sin compilar, rama feature/devf-cp04-3-private-scent-marker, desde dac8d172081e8933f3e9fcdeea40e09b4374f372. PR nuevo draft contra feature/devf-cp04-2-curve-diagnostic. No cambios de archivos previos, host/SDK/log humano/assets originales. El árbol raíz anterior se conserva por base_tree/parent; comparar Git trees para evidencia de preservación. No merge/force/actualización de refs anteriores.

**No tag nuevo creado si Cloud continúa offline:** la integración disponible crea branches/commits pero no expone creación de tags; no simular un tag con un branch. Final informar esa limitación. Un commit alcanzable desde branch privado es recuperable; fsck local y lectura física del nuevo checkout quedan pendientes hasta restauración.

## Trabajo local antes de fallo — no presentado como fuente compilada
Git status inicial clean: feature/devf-cp04-2-curve-diagnostic/dac8d172...; CI clean ci/devf-cp04-2-curve-diagnostic/4cb4c238... .
Creada rama local feature/devf-cp04-3-private-scent-marker y nueva carpeta DEVF/CP04_3_PRIVATE_SCENT_MARKER. Código/adaptador/pruebas/pipeline se prepararon localmente y el último comando de creación del laboratorio terminó con exit0. Ningún commit/push local CP04.3 antes de fallo. No se puede asegurar persistencia/bytes de esos archivos tras reinicio. Este checkpoint incluye el borrador de código reconstruido de operaciones de escritura confirmadas en la sesión. No afirma cotejo posterior con disco inaccesible ni fuente compilada. recovery-provenance.json distingue copias exactas de GitHub, transformaciones confirmadas y código recuperado de la sesión.

Clones/checkouts de trabajo conocidos:
- /workspace/Mods-Duskligh
- /workspace/work/devf-g2-input-ci
- /workspace/work/dusklight-v203-baseline
- /workspace/work/devf-cp04-3 (scripts prepare.py/mod_pipeline.py y snapshots refs previas)
- Original EXE/lib y ZIP privados se conservan donde ya estaban; no reconstruir/cambiar assets.

## Primer paso al restablecer Cloud
1. Leer este checkpoint y git status/log/diff ANTES de editar. Revisar si siguen carpeta nueva/scripts, no repetir estudio kytag03/JPA ni CP01–03.
2. Recuperar branch privado mediante fetch; si el repo tiene trabajo local no confirmado, crear checkout/worktree independiente o copia/shelf de SOLO archivos propios. NO reset --hard, checkout -f, clean o sobrescribir drafts.
3. Verificar checkpoint dac8... y carpetas/tag anteriores; ledger17CP03 en evidence/windows-run-38541018826/result.json. policy/registration/observer deben ser byteexactos CP04.2. Reusar source/importlib/SDK manifiestos; no nuevas investigaciones generales.
4. Recuperar borrador local o reconstruir SOLO implementación pendiente según preflight-spec.json/decisión. Última prueba escrita estaba pendiente de añadir <algorithm>/<JPAParticle.h> y caso actor_id_recycled. Comprobar que std::remove_if/std::max y JPANode<JPABaseParticle> estén completos. El fallo ocurrió ANTES de aplicar esa corrección/documentos locales.
5. Revisar además finitud de eyePos, lectura de extra shape/tables contra contrato del host oficial, ownership ANTES de pRes y no retry de misma generación. No relajar guards si el candidato aún no tiene datos reales.
6. CMake nuevo mod0.4.3, Stage/Hook oficial, CP03 contrato intacto. MSVCx64 Actions privado, SDK40457/Aurora3227/importlibSHA4dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4. Reusar auditores CP04/CP04.2: exact38imports esperados (15game+23CRT), confirmar realbinario, no asumir; PMFexecute/5exports/7metadata/ABI1;4PEnegativos.
7. CI nueva rama ci/devf-cp04-3-private-scent-marker desde4cb4c238a1010f5638c5200c6ff5716cc2dba49d, workflow private same-repo PR/base exacta. Fuente compilada debe referenciar commit propio publicado. Ejecutar solo nuevas ramas afectadas y certificar registro antiguo sin rutina.
8. Solo tras build/lab/auditoría/package PASS, descargar artefactos reales por separado y verificar SHA externo/CRC/tamaños/manifests/internalDLL/PE/sourceledger. Estado OFFLINE_TEST_READY únicamente si todo pasa; sino preservar NO_GO.
9. Publicar checkpoint final/tag propio y ambos PRs draft; verificar por fetch independiente/fsck y preservación de todos los refs históricos. Nunca mover tags antiguos ni hacer merge.
10. HUMAN_TEST.md sólo para paquete real validado: fullquit/backup saves/retirar anteriores/incompatibles, hashEXE ffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049, DX12, único0.4.3, apariencia extra+control ORIGINAL E_OC2 MISMO modelo primero. Combate/muerte/reentry solo si estable. STOP si rejected/invisible/error. Reversión fullquit/remove/restart. Runtime nunca PASS por readback.

No pedir ROM/JPC completo ni repetir diagnóstico029D. Próximo dato runtime mínimo, DESPUÉS de la admisión offline, es el preflight84ED propio (o rechazo específico), log privado y captura del extra + control mismoE_OC2.
