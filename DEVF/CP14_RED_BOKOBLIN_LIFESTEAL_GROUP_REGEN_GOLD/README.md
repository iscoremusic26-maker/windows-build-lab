# DEVF CP14 — Bokoblin Red (robo de vida, regeneración grupal, dorado)
TEST privado. **HUMAN_RUNTIME_PENDING.** Expansion **0.11.0**, a usar con el Provider 0.13.0 y el editor de CP13 (sin cambios, no se vuelven a entregar).
Base: CP13, fuente `57b969d792e93147d759b0374d47c9b27b0c234e` (compilada CP13 `c3257bdd…`), CI `d126316a01d3243a805f899a1ffe9ec77ce90723`.

- **Red** = E_OC2 nativo sin registro DEVF (330 HP, 3 HP/s, ataque base 2, sin cambios). Las excepciones DEVF Black/Normal se respetan.
- **Robo de vida:** un golpe que realmente quita vida a Link cura al Red un 25 % de 330. La fracción se resuelve con un acumulador exacto: 82, 83, 82…
- **Ventana de 4 s:** el mismo golpe abre o renueva la ventana. Durante ella, el Red regenera x2 y cada Bokoblin vivo dentro del rango del grupo Blue (650 XZ / 250 Y) regenera x2 su propia tasa mientras siga cerca. Nunca más de x2.
- **Dorado:** aura dorada (misma ruta compartida, presupuesto 8) y ojos dorados del Red mientras su ventana está abierta.
- **Atribución:** usa el propio registro de colisión del motor, se reclama tras la pasada de colisión y se confirma una pasada después por la caída real de vida de Link y el reinicio de su invulnerabilidad. Escudo, invulnerabilidad y armadura mágica no cuentan. Sin hook nuevo; Link solo se lee.

Detalle: [IMPLEMENTATION.md](IMPLEMENTATION.md) · Prueba: [HUMAN_TEST.md](HUMAN_TEST.md) · Resultados: TEST_RESULTS.md (tras la CI).
