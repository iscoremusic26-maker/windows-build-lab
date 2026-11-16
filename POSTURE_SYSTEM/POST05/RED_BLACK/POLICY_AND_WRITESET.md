# Política y conjunto de escritura

Todo src/ y el binario 0.5.1 siguen intactos. El único escritor histórico sigue siendo restore_reaction: mActionMode, mOcState, mOldActionMode. Un hook damage_check; PRE HOOK_CONTINUE. Sin nuevos imports, hooks ni escrituras a HP, potencia, animación, timers o colisión.

La implementación nueva tests/shadow_policy.hpp es OFFLINE: no se incluye en el mod, no recibe punteros ni ofrece callback de escritura. Calcula un candidato de fase con common_verified, modo4, B/C coherente, frame>2, playmode0, rate en {1,1.15,1.30,2}, clip vivo con margen antes del fin, potencia estable y perfil verificado. Rechaza el perfil3 por transición posthook. release_enabled es false. profile_verified/common_verified son premisas sintéticas del laboratorio, NO datos observados ni un mecanismo autenticado implementado.

H0 confirmado en alcance snapshot; H1 parcial (EXE hash y funciones nativas revisadas; no cadena completa CP21 DLL/colisiones); H2 parcial por fase; H3/H4/H5/H6 pendientes de escritura; H7/H8 baseline preservado; H9 LAB_PASS acotado; H10 nueva DLL no construida; H11 HUMAN_RUNTIME_PENDING. Mantener G2_PENDING y POST05_ALL_REACTIONS_RUNTIME_PENDING.

| Caso | Admisión/postura 0.5.1 | Supresión actual | Después de este trabajo |
|---|---|---|---|
| E_OC nativo | sí | política Blue 0/1/2 | igual; baseline probado |
| Blue solo | sí | política Blue | igual |
| Blue grupo | sí, par1↔2 | política Blue | igual |
| Red nativo | EOC2 candidato | diferida | candidato maduro offline; sin activar |
| Red Expansion | EOC2 candidato | diferida | inicio/frame12 bloqueados por contraejemplo; madura pendiente de contactos |
| Black normal | EOC2 candidato | diferida | 3→6 bloqueado en propuesta |
| Black furia | EOC2 candidato | diferida | madura 6→6 candidata independiente; contactos/propiedad pendientes |

No se afirma que ninguna ruta pueda hacerse segura. Se entrega una implementación de investigación con gates cerrados; la extensión de gameplay sigue incompleta.
