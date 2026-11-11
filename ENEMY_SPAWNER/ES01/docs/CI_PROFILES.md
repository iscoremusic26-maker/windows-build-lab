# Perfiles de CI

Lo elige el archivo `ENEMY_SPAWNER/ES01_CI_PROFILE.txt` del repositorio de CI, rama `ci/enemy-spawner-standalone-v0-1-0`.

| Perfil | Ejecuta | `.dusk` |
|---|---|---|
| `fast` | Procedencia y pins, reglas de fuente con negativos, compilación MSVC, auditoría PE, laboratorio del núcleo | No |
| `integration` | Además: catálogo frente a Dawnlight, comprobación del código vendorizado de Expansion, laboratorio del adaptador | No |
| `full` | Además: mutantes ejecutados, empaquetado y descarga independiente | Sí |

No se cachea ningún resultado. Solo `full` nombra y publica el paquete, y un cambio posterior exige otro `full`.
