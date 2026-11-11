# Recuperación exacta CP07.2B

Base inmutable: **b0b506968cbe974f312d14dcfedaf80fde48f827** (PR45). Cierre humano de captura/colección/exportación en **ab3c382ef1c98ae89dd34a1eddce4393ebc78420**; procedencia: usuario y extractos comunicados. La descripción de PR45 se actualizó con autorización; su HEAD y paquete permanecen intactos.

- Rama principal: feature/devf-cp07-2b-bokoblin-expansion, [PR46 draft](https://github.com/iscoremusic26-maker/Mods-Duskligh/pull/46).
- Fuente REALMENTE compilada: **5446365ed343905ff59bf7e6bc4929c5e395e6b1**.
- Build CI: **21666ef596e7d421228a67226167f73ebe7b61e2**, [run38801130769](https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/38801130769), SUCCESS.
- Verificador CI: **cec8b53891fa842c462eafbee606a21571c6a552**, [run38801845274](https://github.com/iscoremusic26-maker/codex-cp00-inputs/actions/runs/38801845274), SUCCESS.
- Rama CI: ci/devf-cp07-2b-bokoblin-expansion, [PR32 draft](https://github.com/iscoremusic26-maker/codex-cp00-inputs/pull/32).
- El commit documental final se lee en PR46 y la entrega; es posterior a la fuente compilada y no la sustituye.

Para recuperar sin pisar trabajo: git fetch origin feature/devf-cp07-2b-bokoblin-expansion; comprobar el SHA completo y su ascendencia desde la base; usar git switch --detach SHA_VERIFICADO. No reset/rebase/amend/force/retag/merge.

## Paquetes y rollback

Artifacts12003399553 (proveedor) y12004087737 (Expansion), con caducidad2026-11-14 15:23:16/17 Panamá. Hashes de ZIP/.dusk/DLL en BUILD_AND_PACKAGE y evidence. No se reemplazó ningún paquete previo.

Rollback en frío: retirar ambos nuevos .dusk y restaurar SOLO proveedor0.7.2,127380bytes, SHA2569e63d0e66f6e0cc6f4695ec665eca7bbddfca3d428ee44294c2fa246cc4c60ce, más los archivos propios respaldados. El consumidor nuevo nunca escribe en el data_dir del proveedor. No instalar el probe histórico durante esta QA.

## Procedencia y continuidad

Input reconstruido del prompt:1512bytes, SHA256303e45636ceee8be0bafbf443a266dbda59e8bc3b31500067312c7ad4e38d71d. SHA del adjunto original fd8ee3aeddf4294843d35594e8216d1500a182e96841a842fea2b3679a9413e4: USER_REPORTED, no igualdad byte a byte alegada. A/B y sus records se verificaron materialmente; terreno no acreditado.

1539 entradas históricas principales y204 de la base CI iguales por path/blob/mode/type;56 fuentes espejo coincidentes. Siete tags anotados verificados sin cambios. Solo se añadieron fuentes/herramientas propias y acta humana, más un workflow nuevo aislado. PR43/44/45 y PR29/30/31 conservan sus HEAD; únicamente descripción PR45 actualizada. No tags nuevos ni merge.

346 casos C++,20 comprobaciones de generación,28 negativos, paquete y descarga PASS; HUMAN_PENDING para UNO→DOS→CERO. Cloud offline: trabajo por GitHub/Actions, sin shell o gameplay fingido. No host/SDK/renderer/ROM/saves modificados ni recursos originales distribuidos.

Si se interrumpe: leer esta recuperación, los informes y HEAD remoto antes de repetir trabajo. Preservar paquetes y esperar QA humana antes de modificar posiciones o ampliar funcionalidades.
