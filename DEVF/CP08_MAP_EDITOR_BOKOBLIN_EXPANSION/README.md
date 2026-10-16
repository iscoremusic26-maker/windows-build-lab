# DEVF Map Editor · CP08 · TEST privado
TEST privado: utilice únicamente el artifact cuya ejecución de build y descarga independiente figure SUCCESS. Gameplay y experiencia interactiva en su PC: HUMAN_RUNTIME_PENDING.

Tres componentes independientes: DEVF Map Editor (Windows x64 autónomo), proveedor DEVF 0.8.0 y Bokoblin Expansion 0.2.0. Conservan los IDs anteriores. Solo una versión de cada mod a la vez.

El editor importa localmente **R03_00.arc de F_SP121** que el usuario posea legítimamente. Comprueba SHA256 `dc53106667811c4cde18b35edb5279357f00ebecaef2f9de546aea9a5a2b06af`. No se distribuye ese archivo, texturas ni geometría del juego. No necesita conexión ni estar abierto durante el juego.

Mapa técnico obtenido de KCL/PLC auténticos, con X/Z del mundo, zoom y desplazamiento. Alcance: F_SP121 / sala3 / capa6, X[-2250,-250], Z[59000,63000], margen100. Calcula Y del suelo +5 unidades de separación inicial. No usa Link ni las dos capturas antiguas para calibrar. Rechaza PLC incompatibles, altura ambigua, pendiente, discontinuidad de huella y pared cercana. La pantalla estática no certifica navegación, eventos, ocupación dinámica, recursos en todos los estados ni gameplay.

Hasta ocho registros globales del proveedor (incluye legacy y otros consumidores). El consumidor admite 0–8 entradas, separa activos al menos300 XZ y lee su propio `encounters.json` al arrancar. Archivo ausente/inválido: cero solicitudes; no fallback a qa.json. Se mantienen ambos servicios ABI1.0, editor CP07 y rango0132 de encuentros; legacy0106 intacto.

Formato nuevo `devf_map_encounters` schema1. El único enemigo admitido es E_OC2 nativo; cada marcador es un enemigo. Los únicos atributos editables son posición obtenida del mapa, yaw y enabled. Sin HP/color/IA personalizados.

[Prueba e instalación](HUMAN_TEST.md) · [Recuperación](RECOVERY.md) · [Implementación](IMPLEMENTATION.md).
