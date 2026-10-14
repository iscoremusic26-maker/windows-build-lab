# Una prueba humana diagnóstica — CP04.1

**DIAGNOSTIC_ONLY_HUMAN_TEST_PENDING. NO-GO de apariencia.** No se solicita combatir, reentrar o repetir la batería visual. No hay corrección causal construida.

1. Cerrar Dusklight por completo. Respaldar saves sin alterarlos. Guardar el .dusk CP04v0.4.0 íntegro fuera de la carpeta de mods para comparación; retirar CP04v0.4.0, CP03 y mods de enemigos/render/hooks. Instalar **solo** el nuevo .dusk oficial indicado en BUILD_AND_PACKAGE; no el ZIP externo deActions ni DLL suelta. Verificar tamaño/SHA256 con Get-FileHash.
2. Usar Windowsx64/DX12 y EXE oficial2.0.3/rev40457c6/SHAffa992038d04290654b8907a7663f4dcf62083beaad2b35014fdfd901d390049. No cambiar common.jpc ni assets. Inicio debe mostrar una sola carga dev.devf.cp04_native_aura version0.4.1, nombreDIAGNOSTIC ONLY y EDIT_REGISTERED/ARMED. Si aparecen ambosCP03/CP04 o versión0.4.0, detener y corregir instalación tras cierre total.
3. Una sola entrada al área habitual F_SP121/sala3/capa6 para el actor adicional (-1250,-6304.00830078125,60750). Evitar combate. Buscar HP_APPLIED220→150 y primer CP04_VISUAL_REJECTED con reason específico, modeDIAGNOSTIC_ONLY y probe_mask. La desactivación y aviso pueden seguir siendo esperados: cerrar el proceso y enviar **privadamente** esas líneas, las líneas de arranque/backend y si hubo crash. No compartir rutas personales públicamente.
4. Si aparece APPLIED, detener también tras esa primera observación y comunicarlo. APPLIED acredita configuración/readback, **no apariencia**; esta revisión no debe asumirse arreglo. No iniciar la batería visual sin revisión posterior.
5. Reversión: cerrar totalmente, retirar nuevo .dusk, reiniciar sinDEVF y comprobar desaparición de la instancia adicional. No guardar cambios de progreso durante el diagnóstico. No hotreload ni borrado manual de recursos. CP03 puede volver a instalarse solo, tras cierre completo, como baseline previamente validado.

No solicitar ROM, recursos enteros o dumps: el siguiente dato mínimo es el único gate concreto y sus escalares. CP05/CP06 no autorizados.
