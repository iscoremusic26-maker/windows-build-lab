# CP18 — Black: furia encadenada y persecución en la misma sala · Expansion 0.15.0 TEST privado

Sustituye **solo** la Expansion 0.14.1 (CP17 FIX01). Provider/editor CP13 0.13.0, APIs, JSON y recursos del usuario permanecen intactos. Windows x64, Dusklight 2.0.3@40457c6, DX12. No instalar dos Expansion ni usar hot reload.

- **Black normal**: golpes B/C nativos a x1,15 con sus pausas nativas. Antes del primer daño sigue siendo convocable por Blue.
- **Furia** (la de CP10.6: 300 tics, se renueva con daño real, ATP 6, aura/ojos): golpes nuevos a x2 (fase par y cobertura de contacto de CP17 FIX01) **encadenados**: al terminar el clip, el siguiente ataque nativo empieza sin la rama de espera (BCK 0x1C + voz WAIT_ST) ni el lockout de 20 tics; si Link no está a alcance, aproximación nativa directa.
- **Hunt**: el primer daño real (detector CP10.6 reutilizado) fija la persecución en la sala del Black hasta que Link cambie de sala/escenario o el Black muera. Sigue tras la furia, con x1,15 y pausas nativas. Al volver a la sala hace falta daño nuevo.
- Red 0.14.1, Blue FIX01, CP14 y clasificación: byte a byte iguales. Ningún import nuevo.

Ver IMPLEMENTATION, HUMAN_TEST, RECOVERY y TEST_RESULTS. Estado: HUMAN_RUNTIME_PENDING; no se afirma CHAIN_VISUAL/PURSUIT_REAL/SHIELD/HIT/AUDIO/SAME_ROOM_RESET PASS.
