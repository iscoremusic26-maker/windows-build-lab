# Human runtime check — pending

Use Dusklight 2.0.3 Windows x64 DirectX 12, with only the two CP21 bundles installed/enabled. Preserve your previous setup outside the mods folder. Codex has not executed the game.

1. Cold start without an external encounter file. Confirm exact menu names, original images and no Framework/editor tabs, windows or actions.
2. Visit F_SP121 layer 6 rooms 0, 2, 7, 9, 10 and 12. Compare encounter IDs and placement traces with the frozen input; confirm 32 registrations and no legacy FIELD/NORTH pair. Stage-edit receipts alone do not prove that every actor spawned or stands correctly on terrain.
3. Leave and reenter rooms. Confirm one live instance per encounter, no doubled spawns, correct resource/variant and Black HP 440. Check Blue recruitment, Red aggression, Black fury/hunt, native audio 0x5e, attack/shield behavior, textures and feedback against CP20.
4. Restart with untouched old data files present; the result must remain the same. Disable Expansion, then Framework; check cleanup and restored hooks/state, then perform a fresh installation and cold start. Existing actors may need scene reload or restart after stage-edit removal.
5. Test Expansion with Framework unavailable: the loader must suspend/fail it cleanly because its required V1.1 dependency is missing, without partial encounter registrations.
6. Preserve logs and report any `LINK_LAYOUT_CANARY_FAILED_LIFESTEAL_DISABLED`. Do not mark Red lifesteal fixed without a human observation and confirming trace.

Record installed SHA256, Dusklight version/backend, observations and relevant log excerpts. Status remains HUMAN_RUNTIME_PENDING until these checks have evidence. Seek separate approval before a public release.
