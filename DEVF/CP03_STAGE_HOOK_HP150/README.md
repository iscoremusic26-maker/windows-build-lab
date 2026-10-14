# CP03 StageService + HookService HP150 — piloto privado

Implementación autorizada por la nueva excepción: HookService oficial 1.1 y una sola escritura tipada `health=150` a la instancia adicional identificada. No ActorService, callbacks de solicitud, hooks manuales, globals, cambios de perfil, field_0x560 ni host/SDK modificados.

Base funcional CP02 `0d2430262181bb2844f13bb8da0a628f71740998`; rama nueva `feature/devf-cp03-stage-hook-hp150`. Las revisiones CP03 de PR22/23 se conservan en sus propias ramas y no se reescriben. ID del piloto `dev.devf.cp03_stage_hook_hp150`, versión 0.3.0.

Una entrada E_oc ACTR32, parameters FFFF0100, angle (262,0,0), setID FFFF, F_SP121/room 3/layer 6, XYZ(-1250,-6304.0083,60750). Hook registrado antes del stage edit; ninguna escritura en mod_update. Mismo comportamiento/recursos nativos CP02, con health inicial 150 antes de execute/action cuando se valida la primera creación. No se afirma daño2× ni150=X golpes.

[SOURCE_REVIEW.md](SOURCE_REVIEW.md) documenta identidad/fase/ciclo de vida y pruebas del target; [HUMAN_TEST.md](HUMAN_TEST.md) define la prueba posterior. La evidencia de integridad antigua se reutiliza sin reabrir terreno/ARC/G5/A0/A1.

Estado inicial: SOURCE_VERIFIED y build/package/download pendientes. La compilación/laboratorio no prueban instalación del detour, recursos, campoHP, render o combate dentro del juego; HUMAN_RUNTIME_NOT_TESTED. No instalar/ejecutar desde Codex. Solo se entregará el `.dusk` si los gates Windows/PE/ZIP/descarga pasan. No CP04.
