# Implementación y límites

## Fuente y alcance

Copia segregada de runtime CP16. Solo cambian blue_recruit_policy.hpp y el adaptador Blue en mod.cpp, versión/init telemetry y manifest0.13.1.23 archivos runtime restantes son byte-idénticos (incluido blue_ai_policy.hpp). source_gate compara el resto de mod.cpp contra CP16, lista cerrada de escrituras y cinco llamadas nativas setActionMode; reviewed-imports.json idéntico, un execute hook PRE/POST. Grupo CP10.6, CP14/Red, Black, gráficos, regen y daño no se retocan. Los demás2082 blobs anteriores y los42 archivos CP16 se preservan por path/blob/mode/type; verificar número real en cierre remoto.

## Hecho, hipótesis y causa

Rawlog aportado por el usuario, leído en esta sesión: dusklight-20261018-175612.log,7895 líneas,966974 bytes,SHA256790108c6386e2cab37a1f476ececec1c22d6e6e4a98ed85212d73c8747ed775a. No se publica el archivo.59 BLUE_CALL_ISSUED,5 ofertas/consumos/inicios,1 llegada,2 proximidades sin aproximación medida,3 esperas,1 grupo listo,5 abortos.59 llamadas no equivalen a59 intentos fallidos.

161g7→163g9: oferta~1327.1XZ, aproximación y proximidad, cancelación por plazo;1066→1067 también plazo.629g10→630g11 abortó con causa compuesta; no hay datos para elegir un predicado.1007g32→1009g34 abortó dos veces y cada aborto está seguido por snapshot del caller DAMAGE5; esto respalda una hipótesis transitoria, NO demuestra retrospectivamente el predicado exacto. Los replays de IDs son modelos condicionales con el mismo policy header, no reconstrucción de campos ausentes ni gameplay.

Cerca en pantalla puede seguir siendo >1800XZ, >250Y, otro contexto, no idle, combate, hit, snapshot stale/culling, orden/backoff o destino fuera de home/range propio. FIX01 amplía solo el primer límite y aporta causas para distinguir los restantes; no conoce camino navegable.

## Activación y llamadas

kRadius única fuente2800; deltaY250, margen100. Admisión conserva requisitos previos. incoming suprime propagación. Solo detected nativo propio abre combat.active; luego safe FIND/ATTACK/FIND_STAY + Link presente permite recalls150 sin búsqueda positiva actual. ATTACK guard/recoil state>=3, daño/eventos/transiciones/ground/vida/contexto inválidos no llaman.150 ejecuciones genuinas de calma/Link ausente cierran el combate; no se reinicia sin nueva percepción. El grito nativo WATCH3→4/BCK7/CRY y SOUND_WATCH receptor siguen iguales. No se altera mMoveRange/home/field_0x6e3/speed/physics ni se escribe otro actor desde el emisor.

## Diagnóstico

Prioridad primaria: dominio de snapshot → clasificación/vida/terminal → freshness → stage/room/layer → XZ/Y → combate/idle/hit/ready → correa → orden/combate previo → backoff. Vacíos y emisor se contabilizan aparte. Offered+reasons=examined_tracked. ALREADY_FIGHTING precede BUSY si ambos aplican. Hay16 buckets; not-classified solo puede medirse si un snapshot así existe, no significa que se observe todo actor retail. Identidad autoritativa se valida al consumir/refrescar; falla se registra como CALLER_VERIFIED_IDENTITY_FAILED, no se inventa verificación desde el snapshot del emisor.

RECRUIT_CALL_SUMMARY conserva todos los contadores reales aunque se limite detalle. Máximo2 candidatos seleccionados por llamada, cursor rotativo; evento legacy NATIVE_LEASH conservado. Presupuesto global adicional por30 management passes:16 call,16 summaries,8 details, límites sesión600/600/240. Lifecycle1200 separado. Los detalles son muestreo, no promesa de línea para cada enemigo; suppressed_total y scope=ledger aclaran cobertura. Distancias solo de snapshots finitos/contexto fresco para resumen; detalle señala known/UNKNOWN y edad, nunca direcciones. Logs tienen versión, id/generación, salud, modos, unsafe flags, identidad/edad, contexto, leash, timers y razón. Cap de logs y estructuras fijas, sin mapas crecientes.

## Abortos y gracia

Chequeos duros en orden: caller ausente/generación; receptor dominio/clase/vida/frescura/ready/hit; caller dominio/claseBlue/vida/frescura/contexto/ready/leash; correa receptor; modo receptor; verificación autoritativa. Razones diferenciadas también al publicar cambios de contexto y desprender ledger. Caller DAMAGE ordinario0..3/5 o hit normal solo permite conservar una orden ya consumida y verificada, por10 ejecuciones receptor; no inicia otras ni escribe target/timer/estado durante hold. Recuperarse no reinicia start/progress/deadlines. A10 aborta; muerte/contexto/stale/identidad/leash/unsafe cancelan inmediatamente incluso dentro de gracia. Gales DAMAGE4/6,BIG_DAMAGE,caída/agua/demo se rechazan.

Fundamento exacto: d_a_e_oc.cpp@40457c6, damage_check~638–700 usa debounce10 ordinario (20 ataque especial); executeDamage~1534–1587 modos0/1/2/3→5, animación terminada→FIND o MOVE_OUT.10 es una gracia conservadora alineada al debounce, NO duración verificada del clip ni garantía de cubrir toda reacción. No se otorga la ventana larga de daño especial.180/240 aproximación,60 sin progreso,240 esperaBlue,90backoff,150recall y15refresh se mantienen. after() continúa corriendo en gracia y limita progreso/plazo. Un ENGAGED no demuestra llegada y un Blue que se mueve no acredita progreso del receptor.

## CI y procedencia

Workflow nuevo apilado sobre CI CP16; source exacto pinneado, mismo SDK/importlib oficiales, MSVC x64, package oficial. Se ejecuta lab CP16 histórico56 sin alterar golden y FIX01 shared-policy53; CP15 y regresiones siguen su contexto histórico. Provider/editor CP13 se reconstruyen solo para regresión, no se entregan. SourceZip/ledger registra origen exacto; dos documentos CP16 del espejo conservan su blob de fuente compilada e364, no los cierres posteriores1a611 (excepciones explícitas de origen documental). Runtime previo coincide con1a611/e364. Descarga independiente verifica bytes/CRC/SHA/PE/ABI/imports/manifest/source real. CI no observa juego, audio ni rendimiento humano.
