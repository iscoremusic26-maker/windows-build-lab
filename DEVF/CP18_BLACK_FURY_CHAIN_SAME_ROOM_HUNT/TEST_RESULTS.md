# CP18 — resultados

**SOURCE_IMPLEMENTED / LAB_PASS (local) / WINDOWS_MSVC_PENDING / INDEPENDENT_DOWNLOAD_PENDING / HUMAN_RUNTIME_PENDING**

## Evidencia local (Linux WSL2, g++ 15.2, C++20, ASan+UBSan; LeakSanitizer no disponible bajo ptrace)

| Gate | Resultado | Clase de evidencia |
|---|---|---|
| `cp18_lab` (política Black del DLL + CP10.6 real contra un puerto de fuente de `execute/action/executeAttack/executeFind/executeWait/executeWatch/executeMoveOut/executeFindStay/executeDamage/setWaitStSound/J3DFrameCtrl`) | 43/43 CASE_PASS | LAB |
| Mutaciones de la política (`tests/cp18_mutations.py`) | 30/30 eliminadas | LAB |
| `mod.cpp` con g++ contra las cabeceras oficiales 40457c6 | 0 errores, 0 avisos en ficheros propios (sin el `static_assert` PMF propio de MSVC) | COMPILADOR NO-MSVC |
| `source_gate.py` contra CP17 FIX01 y 19 blobs oficiales | PASS: 25 ficheros runtime idénticos, `red_ai_policy.hpp` idéntico, fuera del bloque Black solo las líneas marcadas, imports idénticos | INSPECCIÓN |

Casos: constants_x115_x2_fury300_atp3_6_reused cp10_6_fury_300_ticks_refresh_no_stack_unchanged hunt_armed_once_from_cp10_6_event_no_second_detector hp_init_and_regeneration_are_not_damage lethal_and_terminal_never_arm link_other_room_or_unknown_room_never_arms other_black_blue_red_never_inherit normal_blows_x115_at_frame1_both_clips_native_pauses_kept fury_blows_x2_decided_frame1_applied_frame2_even_phase_five_at fury_change_mid_blow_never_changes_rate foreign_or_not_fresh_rate_skipped natural_clip_end_rate0_is_not_a_foreign_write sfx_all_fire_at_x115_and_x2 translation_conserved_at_x2 x2_sweep_reused_only_for_owned_x2_never_pose24 shutdown_restore_only_owned_running_blow fury_chain_no_bck1c_no_wait_st_no_lockout_between_blows control_native_without_fury_plays_wait_and_lockout native_bc_choice_preserved_all_sequences chain_only_after_complete_clip_previous_window_whole chain_out_of_reach_pursues_find3_no_wait_no_lockout_then_attacks chain_requires_facing_height_los_and_no_wall fury_end_restores_x115_and_native_pause damage_during_chain_keeps_native_priority shield_recoil_completes_before_next_chained_blow fury_find0_after_damage_skipped_and_lockout_cleared terminal_event_hidden_transition_never_commanded fury_leash_without_hunt_keeps_native_move_out hunt_keeps_pursuit_without_perception_timer_held hunt_runs_after_link_across_room_far_from_home hunt_attack_end_with_leash_goes_find0_not_move_out hunt_damage_end_move_out_redirected room_change_resets_and_return_stays_reset_until_new_damage a_b_a_without_black_execute_resets_via_poll_epoch room_minus1_and_absent_link_hold_without_reset stage_change_resets death_and_generation_reuse_clear_hunt two_blacks_independent_hunts watch_sound_wait_redirect_and_bounce_limit find_stay_is_native_geometry_guard_logged_not_overridden pursuit_blocked_by_wall_detected_from_measurement telemetry_bounded_critical_reserve_chain_detail_cap leash_next_is_the_native_action_update 

Lo que el laboratorio **no** prueba: animación BCK real, audio real, colisión y escudo reales, rutas de Hyrule Field, rendimiento. El puerto usa finales de clip de laboratorio.

## Windows MSVC y descarga independiente (CI)

Pendiente de ejecutar.
