# DEVF CP15 — IA Blue: evasión en solitario y agresividad escalonada en grupo
TEST privado. **HUMAN_RUNTIME_PENDING.** Expansion **0.12.0**, a usar con el Provider 0.13.0 y el editor de CP13 (sin cambios, no se reentregan).
Base: CP14 — fuente `3a647f0e4ebbc868f0bb75aeabe9a22a3edbdddd` (compilada CP14 `e30218bb…`), CI `57df06866810d9999195629f75e976f75de39be7`.

- **Grupo:** nivel = 1 + min(compañeros, 2). Un compañero es cualquier Bokoblin vivo clasificado (Blue, Red o Black) dentro del filtro CP10.6 sin cambios (650 XZ / 250 Y, misma sala).
- **Blue solo (nivel 1):** no persigue; se gira y corre alejándose de Link por un rumbo comprobado (pared, agua, suelo, correa); no empieza ataques nuevos; sin ruta segura se queda quieto.
- **Pareja / grupo (nivel 2 / 3):** animaciones de ataque B y C a 1,15x / 1,30x (fijadas una vez al arrancar el golpe) y bloqueo posterior 20 → 17 / 15 ticks. Daño, esferas, ventana de contacto en fotogramas, HP y Red/Black: sin cambios.
- Un hook (`daE_OC_c::execute`), lista cerrada de escrituras al actor, 6 imports de juego de solo lectura nuevos.

Detalle y límites: [IMPLEMENTATION.md](IMPLEMENTATION.md) · Prueba: [HUMAN_TEST.md](HUMAN_TEST.md) · Resultados: TEST_RESULTS.md (tras la CI).
