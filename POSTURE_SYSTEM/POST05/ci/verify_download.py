"""POST05: verificacion INDEPENDIENTE del artefacto descargado (no del directorio de compilacion).
Uso: verify_download.py --download <dir> --project <POST04> --ci-source <json> --out <dir>"""
import argparse
import hashlib
import json
import sys
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import pe_audit  # noqa: E402


def sha(b):
    return hashlib.sha256(b).hexdigest()


def lf(b):
    return b.replace(b'\r\n', b'\n')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--download', type=Path, required=True)
    ap.add_argument('--project', type=Path, required=True)
    ap.add_argument('--ci-source', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    res = {'status': 'ARTIFACT_VERIFY_FAIL', 'errors': []}
    try:
        files = sorted(p for p in a.download.rglob('*') if p.is_file())
        names = sorted(p.name for p in files)
        duskfiles = [p for p in files if p.suffix == '.dusk']
        if len(duskfiles) != 1 or 'DOWNLOAD_MANIFEST.json' not in names or 'SHA256SUMS.txt' not in names or len(files) != 3:
            raise RuntimeError('Unexpected artifact files: %s' % names)
        dusk = duskfiles[0]
        data = dusk.read_bytes()
        meta = json.loads((dusk.parent / 'DOWNLOAD_MANIFEST.json').read_text())
        cs = json.loads(a.ci_source.read_text())
        sums = (dusk.parent / 'SHA256SUMS.txt').read_text().split()
        if sha(data) != meta['package_sha256'] or sums != [meta['package_sha256'], dusk.name] or meta['package_bytes'] != len(data):
            raise RuntimeError('Hash/size mismatch between .dusk, manifest and SHA256SUMS')
        if meta['source_commit'] != cs['source_commit'] or meta['runtime_tested'] or meta['gameplay_changes'] is not True or 'game_writes' not in meta:
            raise RuntimeError('Manifest provenance/claims mismatch')
        f2=meta.get('cp05_fix02',{})
        for key in ['ordinary_Link_sword_terminal_native_receipt','exact_PRE_POST_and_live_animation_readback','old_POST05_transaction_and_writer_preserved','all_fresh_native_Link_contacts_use_existing_paired_cleanup','native_inactive_AT_rejected_before_collision','guard_observer_all_variants_read_only','fresh_guard_can_initialize_existing_layout_receipt','combo_profiles_RNG_Blue_shield_Link_HUD_unchanged']:
            if f2.get(key) is not True:raise RuntimeError('Missing FIX02 verified policy '+key)
        if f2.get('base_compiled_source')!='785f7044a09588ea9f570f09b4b74580e4f7ee93' or f2.get('new_native_writers')!=0 or f2.get('new_hook_targets')!=0 or f2.get('real_game_or_visual_certification') is not False or f2.get('runtime')!='HUMAN_RUNTIME_PENDING':
            raise RuntimeError('FIX02 scope/provenance mismatch')
        with zipfile.ZipFile(dusk) as z:
            members = [i.filename for i in z.infolist()]
            if sorted(m for m in members if not m.endswith('/')) != ['lib/windows-amd64/mod.dll', 'mod.json']:
                raise RuntimeError('Unexpected members %s' % members)
            if any(m.startswith('/') or '..' in m.split('/') for m in members):
                raise RuntimeError('Unsafe member path')
            mod_json = z.read('mod.json')
            dll = z.read('lib/windows-amd64/mod.dll')
        if lf(mod_json) != lf((a.project / 'mod.json').read_bytes()):
            raise RuntimeError('mod.json differs from source manifest')
        if sha(dll) != meta['dll_sha256']:
            raise RuntimeError('DLL hash differs from manifest')
        tmp = a.out / 'extracted-mod.dll'
        tmp.write_bytes(dll)
        audit = pe_audit.audit(tmp, a.project / 'ci' / 'expected_pe.json')
        tmp.unlink()
        if audit['status'] != 'PE_AUDIT_PASS':
            raise RuntimeError('PE audit on downloaded DLL: %s %s' % (audit['status'], audit['errors']))
        if meta.get('hook_targets') != sorted(audit['hook_thunks']) or meta.get('hook_meta_kinds') != audit['meta_kinds'] or meta.get('game_import_count') != len(audit['imports']['dusklight.exe']):
            raise RuntimeError('Download manifest hook/import claims differ from actual audited DLL')
        if 'paired own AtSet' not in meta['game_writes'] or 'no new game writes' in meta.get('lnk_session01',''):
            raise RuntimeError('Download manifest omits the scoped FIX01 enemy writer')
        if 'procGuardBreakInit' not in meta['game_writes'] or 'lnk_session02' not in meta or meta.get('version') != '0.12.2' \
                or '?procGuardBreakInit@daAlink_c@@QEAAHXZ' not in audit['imports']['dusklight.exe']:
            raise RuntimeError('Download manifest/DLL omit the LNK SESSION02 native Link transition (procGuardBreakInit)')
        fx_imports = ('?setStickData@daAlink_c@@QEAAXXZ', '?setShieldGuard@daAlink_c@@QEAAXXZ', '?setRStatus@daAlink_c@@QEAAXE@Z', '?spActionTrigger@daAlink_c@@QEAAHXZ',
                      '?checkMoveDoAction@daAlink_c@@QEAAHXZ', '?setCollision@daAlink_c@@QEAAXXZ')
        if 'lnk_session02_fix01' not in meta or 'FLG2_UNK_8000000 switched OFF only' not in meta['game_writes'] or 'crouch' not in meta['lnk_session02_fix01'] \
                or any(i not in audit['imports']['dusklight.exe'] for i in fx_imports) or meta.get('hook_meta_kinds', {}).get('HOOK_MEM_EXT') != 14:
            raise RuntimeError('Download manifest/DLL omit the LNK SESSION02 FIX01 manual-shield/penalty hooks and writes')
        if meta.get('hud_fix03', {}).get('version') != '0.11.5' or 'DISABLED' not in meta['game_writes']:
            raise RuntimeError('HUD FIX03 policy missing from package manifest')
        cp=meta.get('cp02_cp03',{})
        if cp.get('regen_rates')!=[10,7,4,2,1] or cp.get('regen_delay_eligible_ticks')!=60 or cp.get('parry_frame_delta')!=[-1,7] or cp.get('parry_age_max_ticks')!=9 or cp.get('link5to8')!='NOT_APPROVED_NOT_APPLIED':
            raise RuntimeError('CP02+CP03 approved policy missing')
        balance = meta.get('balance_session01', {})
        if balance.get('weights_sword_parry_iron') != {'BLUE':[25,40,50],'RED':[17,27,33],'BLACK':[13,20,25],'BLACK_FURY':[13,20,25]} or balance.get('standing_rearm') != 25 or balance.get('getup_rearm') != 0 or balance.get('new_native_writes') is not False:
            raise RuntimeError('Download manifest omits or changes BALANCE SESSION01 approved policy')
        if json.loads(mod_json).get('version') != '0.12.2' or not dusk.name.startswith('LIGHT_FADES_TWICE_LIZALFOS_FIX02_0_12_2_'):
            raise RuntimeError('Wrong CP02+CP03 package version/name')
        cp4=meta.get('cp04',{})
        if cp4.get('rates')!=[1.1,1.1,1.5,1.5] or cp4.get('gaps_sim_executes')!=[8,8,2] or cp4.get('rates_0_11_10')!=[1,1,1.5,1.5] or cp4.get('gaps_0_11_10')!=[12,12,2] or not cp4.get('native_shield_init') or not cp4.get('one_native_shield_action_per_parry') or cp4.get('visual')!='VISUAL_SHIELD_MOTION_HUMAN_PENDING':
            raise RuntimeError('CP04 native combo and shield provenance policy missing')
        if meta.get('rollback_0_11_6')!='0aa8e8d2941d644a33765fe288bc3f9867cc1e188506fed691caf069c63fb019' or not cp4.get('one_parry_per_native_shield_action_global') or not cp4.get('untrusted_init_never_borrows_input'):
            raise RuntimeError('CP04 receipt policy or human-approved rollback missing')
        if any(x not in audit['imports']['dusklight.exe'] for x in ('?procGuardAttackInit@daAlink_c@@QEAAHXZ','?setActionMode@daE_OC_c@@QEAAXHH@Z')):
            raise RuntimeError('CP04 verified native transition imports missing')
        cp5=meta.get('cp05',{})
        if cp5.get('red_probabilities')!=['1/2','1/2'] or cp5.get('black_normal_probabilities')!=['1/3','1/3','1/3'] or cp5.get('black_reduction')!='UNBIASED_UINT64_REJECTION':
            raise RuntimeError('CP05 exact conditional probability policy missing')
        if cp5.get('kinds')!={'RED4':4,'BLACK8':8,'BLACK7':7} or cp5.get('per_swing_rates')!={'RED4': [1.05, 1.35, 1.35, 1.05], 'BLACK8': [1.05, 1.05, 1.3, 1.3, 1.3, 1.3, 1.3, 1.05], 'BLACK7': [1.3, 1.3, 1.05, 1.05, 1.2, 1.2, 1.2]} or cp5.get('maximum_rate')!=1.35 or cp5.get('black_normal_maximum_rate')!=1.3 or cp5.get('at_begin_targets_ticks')!={'RED4': [60, 36, 62], 'BLACK8': [62, 62, 38, 38, 38, 38, 64], 'BLACK7': [38, 62, 64, 47, 47, 47]}:
            raise RuntimeError('CP05 physical patterns/tempos differ from approved candidate source')
        if not cp5.get('native_timer_lease_only_during_execute') or not cp5.get('fury_and_red_specials_unchanged') or not cp5.get('blue_and_shield_source_frozen') or cp5.get('runtime')!='HUMAN_RUNTIME_PENDING' or meta.get('rollback_0_11_7')!='e446ed3ee1111837a9feed628adf939942a006e60913fe66b00d48986b71826e':
            raise RuntimeError('CP05 scope/runtime/rollback policy missing')
        fx=meta.get('cp05_fix01',{})
        if not all(fx.get(k) for k in ['raw_Tg_is_not_committed_priority','committed_native_injury_still_cancels','changed_native_mode_timer_never_restored']) or fx.get('AT_centres_and_radii')!='READ_ONLY' or fx.get('runtime')!='HUMAN_RUNTIME_PENDING':
            raise RuntimeError('CP05 FIX01 diagnosis/ownership/runtime policy missing')
        c6=meta.get('cp06',{})
        if c6.get('base_compiled_source')!='d6e4c2179c2c42a7aa807ce862290de728abb396' or c6.get('version')!='0.11.11' or c6.get('parry_window_frames')!=[-1,7] or c6.get('parry_age_max_ticks')!=9 or c6.get('new_R_B_per_parry') is not True:
            raise RuntimeError('CP06 provenance or unchanged parry rule missing')
        if c6.get('new_native_writers')!=1 or c6.get('new_native_writer_functions')!=['cp06_native_chip_restore'] or c6.get('new_hook_targets')!=0 or c6.get('new_game_imports')!=0 or c6.get('real_game_or_visual_certification') is not False or c6.get('runtime')!='HUMAN_RUNTIME_PENDING':
            raise RuntimeError('CP06 writer/hook/runtime claims mismatch')
        if 'cp06_native_chip_restore' not in meta['game_writes'] or meta.get('rollback_0_11_10')!='9c8f0074962386d7f46984cab43c0cff8fb429911d731e846aa52bef44e4bb4f':
            raise RuntimeError('CP06 writer disclosure or 0.11.10 rollback missing')
        f1=meta.get('cp06f1',{})
        if f1.get('base_compiled_source')!='0ef0ad2824832c9e03d899c298b5f9a8e860d590' or f1.get('version')!='0.11.12' or f1.get('new_native_writers')!=3 or f1.get('new_native_writer_functions')!=['cp06f1_native_sphere_place','cp06f1_native_pose_frame','cp06f1_native_kcolor'] \
                or f1.get('new_hook_targets')!=1 or f1.get('new_game_imports')!=4 or f1.get('real_game_or_visual_certification') is not False or f1.get('runtime')!='HUMAN_RUNTIME_PENDING' or f1.get('rhythms_parry_chip_unchanged') is not True:
            raise RuntimeError('CP06 FIX01 writer/hook/runtime claims mismatch')
        if any(x not in audit['imports']['dusklight.exe'] for x in ('?modelCalc@mDoExt_McaMorfSO@@QEAAXXZ','?SetC@cM3dGSph@@QEAAXAEBUcXyz@@@Z','?SetR@cM3dGSph@@QEAAXM@Z','?modelDraw@daAlink_c@@QEAAXPEAVJ3DModel@@H@Z')) \
                or 'cp06f1_native_kcolor' not in meta['game_writes'] or meta.get('rollback_0_11_11')!='7bcebf6ff442147d6741daf65d9b1e2128b524af8219296272ab64f236d15be3':
            raise RuntimeError('CP06 FIX01 imports, writer disclosure or 0.11.11 rollback missing')
        lz=meta.get('liz',{})
        if lz.get('base_compiled_source')!='5c8ac17e75ed2c8d2b9995277edec59e3ea8360d' or lz.get('version')!='0.12.2' or lz.get('hp')!=370 or lz.get('hp_regen')!=0 or lz.get('gains_sword_linkparry_iron_block')!=[6,12,30,5] or lz.get('gain_to_link_on_enemy_parry')!=25 \
                or lz.get('impacts')!=[4,7,9,6] or lz.get('actions')!=[3,6,6,5] or lz.get('parry_pct')!=[12,35,45] or lz.get('new_game_imports')!=10 or lz.get('new_native_writers')!=14 or lz.get('combo_timing_changed') is not False or meta.get('rollback_0_12_1')!='08b18329128041b9b4bec99d10e2736fbd942fe58d4a1b037e7b88aef4480e23' or lz.get('real_game_or_visual_certification') is not False or lz.get('runtime')!='HUMAN_RUNTIME_PENDING' \
                or '?setCutType@daAlink_c@@QEAAXE@Z' not in audit['imports']['dusklight.exe'] or meta.get('hook_meta_kinds',{}).get('HOOK_NAME')!=12 or 'liz_native_break' not in meta['game_writes'] or meta.get('rollback_0_11_12')!='82f678a8ffacf197d7828721513597fe1b4d3c6752ca07ce2d6a6c9c2797d2ed':
            raise RuntimeError('LIZALFOS 0.12.2 policy, writer disclosure, hook records or rollback missing')
        res.update(status='ARTIFACT_DOWNLOADED_VERIFIED', package_sha256=sha(data), package_bytes=len(data), dll_sha256=sha(dll),
                   mod_id=json.loads(mod_json)['id'], version=json.loads(mod_json)['version'], members=members,
                   pe_audit_status=audit['status'], hook_thunk_bound_import=audit['hook_thunk_bound_import'], filename=dusk.name,
                   runtime_tested=False, errors=[])
    except Exception as ex:  # noqa: BLE001
        res['errors'].append(str(ex))
        (a.out / 'download-verification.json').write_text(json.dumps(res, indent=2) + '\n')
        print('ERROR:', ex)
        return 1
    (a.out / 'download-verification.json').write_text(json.dumps(res, indent=2) + '\n')
    print(json.dumps(res, indent=2))
    return 0


if __name__ == '__main__':
    sys.exit(main())
