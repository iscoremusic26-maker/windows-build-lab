# Windows y paquete CP04.1

Fuente nueva en feature/devf-cp04-1-aura-diagnostic, contra CP04histórico080f8e4. CI privada ci/devf-cp04-1-aura-diagnostic, contra CIhistórica4bea35d. Workflow nuevo devf-cp04-1-aura-diagnostic.yml, solamente PR del mismo repo privado/baseSHA exacto; workflows históricos intactos. actionscheckout/upload y MSVCsetup fijados a los SHAs del pipelineCP04. SDK40457c6, Aurora3227d76 e importlibSHA4dc4a511d761e6eb52541733fa31919a2234ed9cd5b2f16f8fea91c468f30ca4.

Comando de runner: python DEVF/CP04_1_AURA_DIAGNOSTIC/ci/run_windows.py --source .official --inputs . --work <RUNNER_TEMP>/cp04-1-aura-diagnostic. MSVCx64/WindowsSDK, CMakeNinjaRelease; build propio devf_cp04_1_aura_diagnostic + cp04_1_visual_lab, package oficialSDK. Nunca host/juego/DLLload. CI comprueba dependencia/archivooriginalCP00 intactos, ledger propio, 14casos, PEAMD64/ABI1/cincoexports/serviciosHostLogGameStageHook/únicoHookexecute/imports exactos, negativosPE y DLLinternaequaloose. TEST privado separado de evidenciaTEXT sanitizada, retención30días. Ningún recurso original o binariohost se publica.

Pendiente completar enlaces/run/SHAs después del build y descarga real. Se entrega un único DIAGNOSTIC_ONLYv0.4.1, nunca una variante especulativa. [HUMAN_TEST](HUMAN_TEST.md) exige instalación única y cierre total.
