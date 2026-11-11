# Prueba humana (Windows / DX12)

Estado: **HUMAN_RUNTIME_PENDING**.

## Instalación (juego cerrado)
Copia el `.dusk` en tu carpeta de mods. No sustituye a ningún otro mod. Para desinstalar, borra solo ese `.dusk`.

## A. Solo Enemy Spawner
1. En una zona abierta y llana, abre la ventana de Mods, pestaña Enemy Spawner. Elige Lizalfos y pulsa **Invocar enemigo**.
   Esperado: aviso "solicitud enviada" y después "ha aparecido". Un Lizalfos delante de Link, mirándole, que combate.
2. Repite con varios enemigos (Keese, Stalfos, Chu, Bokoblin E_OC nativo).
3. Mata uno, cambia de sala y vuelve, recarga y cierra el juego: sin cierres ni enemigos duplicados.
4. Prueba los fallos: al borde de un precipicio, en el agua, contra una pared o durante una escena. Esperado: aviso y nada creado.
5. Entradas de Expansion sin Expansion: aviso "no está cargado". Black: "integración no disponible".

## B. Con Bokoblin Expansion 0.17.0 + Enemy Variant Framework 0.15.0
- **Bokoblin Blue (Expansion)**: aviso "lo clasificó como BLUE"; en el log de Expansion, `VARIANT_ADMITTED` y `HP_INITIALIZED` con 220.
- **Bokoblin Red (Expansion)**: "lo clasificó como RED", 330.
- **Black**: no disponible.

## C. Con Light Fades Twice 0.12.5
Invoca un Lizalfos. En el log aparece `LIZ_POSTURE event=ADMITTED` para su actor. Bloqueos, parries, contraataques y postura deben funcionar igual que con un Lizalfos del mapa.

## D. Con todos tus mods
Repite A con todo cargado.

En el log, busca `ENEMY_SPAWNER` y, al cerrar, `ENEMY_SPAWNER_SUMMARY`.
