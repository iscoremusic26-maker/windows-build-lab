"""POST05: auditoria de solo lectura de un mod.dll (PE32+ AMD64). Sin ejecutar nada.
Reglas duras: 24 objetivos revisados (15 compactos +9 Link extendidos), thunks/vcall/PMF ligados al EXE oficial; sin exports de servicio,
sin TLS/delay-imports; solo transiciones nativas mutadoras expresamente revisadas; imports del juego y de CRT/sistema EXACTAMENTE iguales a ci/expected_pe.json
(revisado a mano la primera vez que se genero). Uso: pe_audit.py <dll> [--expected ci/expected_pe.json] -> JSON en stdout."""
import json
import re
import struct
import sys
from pathlib import Path

HOOK_NAMES = ['??_7daE_OC_c@@6B@', 'daE_OC_c::damage_check']
LIZ_HOOK_NAMES = ['src/d/actor/d_a_e_dn.cpp#' + x for x in ('daE_DN_Execute', 'daE_DN_Delete', 'damage_check', 'small_damage', 'big_damage', 'e_dn_fight_run', 'e_dn_attack_0', 'e_dn_attack', 'e_dn_tail_attack', 'at_hit_check', 'anm_init', 'e_dn_guard')] + ['src/Z2AudioLib/Z2SoundObject.cpp#Z2SoundObjBase::startCollisionSE']  # FIX03: by name (MSVC cannot constant-initialize that member pointer)
HOOK_VTABLE = {'dCcS':'??_7dCcS@@6B@','dPa_control_c':'??_7dPa_control_c@@6B@','dMeter2Draw_c':'??_7dMeter2Draw_c@@6B@','J2DPane':'??_7J2DPane@@6B@','J2DPicture':'??_7J2DPicture@@6B@','COutFont_c':'??_7COutFont_c@@6B@','JUTFont':'??_7JUTFont@@6B@','J2DScreen': '??_7J2DScreen@@6B@', 'daAlink_c':'??_7daAlink_c@@6B@', 'daE_OC_c': '??_7daE_OC_c@@6B@', 'dMeter2_c': '??_7dMeter2_c@@6B@', 'dMeterButton_c': '??_7dMeterButton_c@@6B@'}
# LNK01: the ONLY virtual hook target. Its PMF points to a LOCAL MSVC vcall thunk (no import); the host follows it to the vtable slot
# (src/dusk/mods/svc/hook.cpp vcall_slot_offset) and resolves the overrider by display name (manifest) or by the vtable symbol. Slot must be 0 (draw).
VCALL_HOOKS = {'dMeterButton_c::draw':0,'COutFont_c::draw':24,'static_cast<void (J2DPicture::*)(f32,f32,f32,f32,bool,bool,bool)>(&J2DPicture::draw)':440}
HOOK_IMPORT = '?damage_check@daE_OC_c@@QEAAXXZ'
FIX03_SET = 'static_cast<JPABaseEmitter* (dPa_control_c::*)(u8, u16, const cXyz*, const dKy_tevstr_c*, const csXyz*, const cXyz*, u8, dPa_levelEcallBack*, s8, const GXColor*, const GXColor*, const cXyz*, f32)>(&dPa_control_c::set)'
# Imports del juego REVISADOS y su razon. Cualquier otro simbolo del EXE es un fallo; las transiciones LNK05/CP04 tienen auditoria de escritor separada:
GAME_IMPORT_REASONS = {
    # FIX03 0.12.3 P1-3 (parry spark/clash) and P1-4 (Sekiro-style lock-on option)
    '?ProcAtTgHitmark@dCcS@@QEAAX_N0PEAVcCcD_Obj@@1PEAVdCcD_GObjInf@@2PEAVcCcD_Stts@@3PEAVdCcD_GStts@@4PEAUcXyz@@0@Z': 'FIX03: PRE/POST hook target only: the shield contact of the collision pass whose hit mark is the parry spark (decision, never a call)',
    '?setHitMark@dPa_control_c@@QEAAXGPEAVfopAc_ac_c@@PEBUcXyz@@PEBVcsXyz@@1I@Z': 'FIX03: PRE/POST hook target only: scope of the native spark emitters of that contact (never called)',
    '?set@dPa_control_c@@QEAAPEAVJPABaseEmitter@@EGPEBUcXyz@@PEBVdKy_tevstr_c@@PEBVcsXyz@@0EPEAVdPa_levelEcallBack@@CPEBUGXColor@@40M@Z': 'FIX03: POST hook target only: the emitter instance the native call returns inside that hit mark (never called)',
    '?movePitch@JAISoundParamsMove@@QEAAXMI@Z': 'FIX03: only in pfx_native_pitch: the pitch of that one playback (x1.12, reverted the same tick on a mismatch)',
    '??1JPAEmitterCallBack@@UEAA@XZ': 'FIX03: base destructor of the chained emitter callback object (static; never destroyed while emitters use it)',
    '?GetAc@dCcD_GAtTgCoCommonBase@@QEAAPEAVfopAc_ac_c@@XZ': 'FIX03: read only: the owners of the At/Tg objects of the shield contact and of Link\'s guard record',
    '?dComIfGp_getAttention@@YAPEAVdAttention_c@@XZ': 'FIX03 P1-4: read the native lock state; writes only in lockon_native_attention',
    '?dComIfGs_getOptAttentionType@@YAEXZ': 'FIX03 P1-4: read only: the user\'s HOLD/SWITCH option (never written)',
    '?m_cpadInfo@mDoCPd_c@@2PAUinterface_of_controller_pad@@A': 'FIX03 P1-4: PAD_1 logical L and right stick of THIS frame, only in lockon_update (read) and lockon_native_pad (write, option ON, enemy locked)',
    '?setDamagePoint@daAlink_c@@QEAAHHHHH@Z': 'LIZ 0.12.2: PRE hook target only: raises its own argument by 50% for an attributed Tg contact during a posture break episode',
    '?GetAc@cCcD_Obj@@QEAAPEAVfopAc_ac_c@@XZ': 'LIZ 0.12.2: read only: the actor that owns the At of the contact Link took, to attribute the x1.5 of a posture break',
    '?checkMagicArmorNoDamage@daAlink_c@@QEAAHXZ': 'LIZ 0.12.2: read only: Magic Armor taking the hit with rupees: no life lost, no multiplier',
    '?checkCutReverseAt@daAlink_c@@QEAAHPEAVdCcD_GObjInf@@@Z': 'LIZ 0.12.2: POST hook target only: FALSE on an ordinary block of an admitted Lizalfos shield; parry keeps the native rebound',
    '?setCutType@daAlink_c@@QEAAXE@Z': 'LIZ 0.12.0: POST hook target only: the swing-start signal of the adaptive defense memory',
    '?ClrHit@cCcD_ObjTg@@QEAAXXZ': 'LIZ 0.12.0: only from liz_native_tg_clear, a Shield Attack Tg mark on an admitted Lizalfos cc_sph below posture 100',
    '?setGuardSe@daAlink_c@@QEAAXPEAVdCcD_GObjInf@@@Z': 'LIZ 0.12.1: POST receipt hook (plays nothing) + the single call in liz_native_link_feedback for a real shield record dropped by native GUARD_SLIP',
    '?StartShock@dVibration_c@@QEAA_NHHUcXyz@@@Z': 'LIZ 0.12.1: only in liz_native_link_feedback, the shock of the native shield contact branch',
    '?dComIfGp_getVibration@@YAAEAVdVibration_c@@XZ': 'LIZ 0.12.1: only in liz_native_link_feedback',
    '?def_se_set@@YAXPEAVZ2Creature@@PEAVcCcD_Obj@@IPEAVfopAc_ac_c@@@Z': 'LIZ 0.12.1: only in liz_native_bash_se, the native collision SE of a real Shield Attack contact whose cc_at_check no longer runs below 100',
    '?getLayerNo@dComIfG_play_c@@SAHH@Z':'CP04 read-only current layer identity in the actor generation/context key; verified official export, int(int)',
    '?execute@daE_OC_c@@QEAAHXZ':'CP04 typed execute PRE/POST at documented priority-100, native original always runs',
    '?setActionMode@daE_OC_c@@QEAAXHH@Z':'CP04 selected Blue native clip exits/gap expiry only; full typed native transition and readback',
    '?checkBck@daE_OC_c@@QEAA_NH@Z':'CP04 read-only native B/C or idle resource identity',
    '?checkBeforeFloorBg@daE_OC_c@@QEAA_NM@Z':'CP04 read-only native water/floor query before selected gap restart',
    '?procGuardAttackInit@daAlink_c@@QEAAHXZ':'CP04 verified new R+B native animation, voice and collider reset; POST observer and readback',
    '?checkUnderMove0BckNoArc@daAlink_c@@QEBAHW4daAlink_ANM@1@@Z':'CP04 read-only actual native shield animation resource identity',
    '?checkShieldDraw@daAlink_c@@QEAA_NXZ':'CP04 read-only real equipped and drawable shield, no flag writes',

    '?drawPikari@dMeter2Draw_c@@QEAAXMMPEAMMUTColor@JUtility@@111MEM@Z':'FIX03 paired Pikari emission ownership by native lower slot animation pointer; original always CONTINUE; no native counter writes',
    '?fpcEx_SearchByID@@YAPEAUbase_process_class@@I@Z':'FIX02 CP06 authentic allied Helmasaur-leaf rule: live parent process lookup by ID, read-only; never cached',
    '?draw@J2DPane@@QEAAXMMPEBVJ2DGrafContext@@_N1@Z':'FIX02 scoped visual-only pane draw argument; original caches execute; paired POST restores by-value bool',
    '?drawString_size_scale@JUTFont@@QEAAMMMMMPEBDI_N@Z':'FIX02 direct text original executes for exact advance; paired local font-color alpha lease',
    '?checkNowWolfEyeUp@daPy_py_c@@SAHXZ':'Read-only senses state identical CP06 snapshot eligibility',
    '?dComIfGs_wolfeye_effect_check@@YAHXZ':'Read-only senses state identical CP06 snapshot eligibility',

    '?draw@J2DScreen@@QEAAXMMPEBVJ2DGrafContext@@@Z': 'HUD FIX01: synchronous screen draw PRE/POST geometric render lease; no action state writes',
    '?procGuardBreakInit@daAlink_c@@QEAAHXZ': 'LNK05: the ONE native Link transition (LINK_NATIVE_TRANSITION). Called once per accepted episode from the existing checkDamageAction POST, inside Link execute, exactly where the game itself returns procGuardBreakInit(); mProcID verified == PROC_GUARD_BREAK afterwards',
    '?checkDamageAction@daAlink_c@@QEAAHXZ':'FIX01 readonly PRE/POST native damage receipt',
    # LNK SESSION02 FIX01 0.11.1 (manual shield from Lazy Shield Rework 1.0.0, Kamilink; extended post-break penalty). Six Link hook thunks + two calls.
    '?setStickData@daAlink_c@@QEAAXXZ': 'FIX01-S2 hook thunk (POST): read R/A/B as sampled by the game (mItemButton/mItemTrigger); clear BTN_B in mItemTrigger only for an offered R+B Shield Attack',
    '?setShieldGuard@daAlink_c@@QEAAXXZ': 'FIX01-S2 hook thunk (POST): clear FLG2_UNK_8000000 only when the lock-on branch set it without R / during penalty / before rearm (never sets it)',
    '?setRStatus@daAlink_c@@QEAAXE@Z': 'FIX01-S2 hook thunk (PRE): skip only BUTTON_STATUS_SHIELD_ATTACK (0x3a) and mark the Shield Attack offer; other statuses continue',
    '?spActionTrigger@daAlink_c@@QEAAHXZ': 'FIX01-S2 hook thunk (POST): only the call right after the offer; returns R+B (FALSE during penalty) so the native procGuardAttackInit runs',
    '?checkMoveDoAction@daAlink_c@@QEAAHXZ': 'FIX01-S2 hook thunk (PRE): during the extended penalty refuses A with DoStatus JUMP/0x79 (sidehop/backflip/side roll/front roll admission) returning FALSE',
    '?setCollision@daAlink_c@@QEAAXXZ': 'FIX01-S2 hook thunk (PRE): during penalty/rearm clear a residual FLG2_UNK_8000000 before Tg shield registration (never in GUARD_BREAK/SLIP/ATTACK)',
    '?dComIfGp_getDoStatus@@YAEXZ': 'FIX01-S2: read-only A-button status to classify the vetoed dodge (same call checkMoveDoAction makes)',
    '?setBStatus@daAlink_c@@QEAAXE@Z': 'FIX01-S2: HUD-only B status "Shield Attack" while R is held and the defence is READY (as Lazy Shield Rework)',
    '?checkPolyDamage@daAlink_c@@QEAAHXZ':'FIX01 scoped readonly native environmental-branch result',
    '?cc_set@daE_OC_c@@QEAAXXZ':'FIX01 paired own At registration cleanup',
    '?executeAttack@daE_OC_c@@QEAAXXZ':'FIX01 readonly POST actual native AtSet evidence',
    # CP06 FIX01 0.11.12: one extended Link hook target and three engine calls used ONLY by the three scoped writers (ci/cp06f1_pe_audit.py).
    '?modelDraw@daAlink_c@@QEAAXPEAVJ3DModel@@H@Z':'CP06F1 hook thunk (PRE +90 capture / PRE -250 counterfactual / POST -250 exact restore): Expansion CP19F2 render-only flash restarted by a CP06-neutralized chip; exact-value only, body/face/hat/hand scope',
    '?modelCalc@mDoExt_McaMorfSO@@QEAAXXZ':'CP06F1 cp06f1_native_pose_frame: the engine pose calc of THIS actor at a vanilla frame, then at the exact frame back (all joint matrices verified equal)',
    '?SetC@cM3dGSph@@QEAAXAEBUcXyz@@@Z':'CP06F1 cp06f1_native_sphere_place: centre of one own AT sphere (the setter cc_set uses), exact vanilla/native pose reaching Link registered Tg',
    '?SetR@cM3dGSph@@QEAAXM@Z':'CP06F1 cp06f1_native_sphere_place: native radius 45/30 of that sphere',

    '?executeDamage@daE_OC_c@@QEAAXXZ':'POST only; suppress premature HeadLock until real Shield Attack',
    '?checkMasterSwordEquip@daPy_py_c@@SAHXZ':'Read-only sword identity for CP21 Helm 100/50 rule',
    '?dComIfGp_getLinkPlayer@@YAPEAVdaPy_py_c@@XZ':'Read-only live Link lookup; no cached player pointer',
    '?dComIfGs_getSelectEquipSword@@YAEXZ':'Read-only selected sword id for Wood exclusion',
    '?damage_check@daE_OC_c@@QEAAXXZ': 'thunk del unico hook (PRE/POST observacionales)',
    '?fopAcM_SearchByID@@YAHIPEAPEAVfopAc_ac_c@@@Z': 'consulta de existencia de solo lectura (fpcM_IsCreating + recorrido de g_fopAcTg_Queue), solo en mod_update',
    '?dComIfGp_getStartStageName@@YAPEBDXZ': 'lectura del nombre del stage de inicio para el epoch de contexto (puntero a miembro)',
    # POST07 HUD (display only): the same native 2D path Enemy Health Bars CP06 imports from the same EXE (checked against its shipped DLL).
    '?_execute@dMeter2_c@@QEAAHXZ': 'POST07 hook thunk: HUD meter execute POST = simulation tick counter',
    '?_draw@dMeter2_c@@QEAAHXZ': 'POST07 hook thunk: HUD meter draw POST = sample + queue our 2D packet',
    '?_delete@dMeter2_c@@QEAAHXZ': 'POST07 hook thunk: HUD meter delete PRE = scrub our packet (always continues)',
    '??0J2DOrthoGraph@@QEAA@MMMMMM@Z': 'POST07 painter: local ortho context (stack object)',
    '?setOrtho@J2DOrthoGraph@@QEAAXAEBU?$TBox2@M@JGeometry@@MM@Z': 'POST07 painter: logical widescreen bounds of the local context',
    '?setPort@J2DOrthoGraph@@UEAAXXZ': 'POST07 painter: viewport/projection of the local context',
    '?setColor@J2DGrafContext@@QEAAXUTColor@JUtility@@000@Z': 'POST07 painter: per-corner colours of the local context',
    '?fillBox@J2DGrafContext@@QEAAXAEBU?$TBox2@M@JGeometry@@@Z': 'POST07 painter: one quad of the posture bar',
    '?dComIfGp_getCurrentGrafPort@@YAPEAVJ2DGrafContext@@XZ': 'POST07 painter: restore the native 2D port after painting',
    '?mDoLib_project@@YAXPEAUVec@@0@Z': 'POST07: world anchor -> ortho point (pure projection)',
    '?dComIfGd_getView@@YAPEAUview_class@@XZ': 'POST07: projection precondition (view present)',
    '?dCam_getBody@@YAPEAVdCamera_c@@XZ': 'POST07: camera for the read-only line-of-sight query',
    '?lineBGCheck@dCamera_c@@QEAA_NPEAUcXyz@@0I@Z': 'POST07: read-only camera line of sight to the anchor (same flags as EHB)',
    '?fopAcM_searchActorDistance@@YAMPEBVfopAc_ac_c@@0@Z': 'POST07: player distance for the 3700-4500 fade (pure)',
    '?dMeter2Info_getMeterClass@@YAPEAVdMeter2_c@@XZ': 'POST07 painter: HUD meter state re-check inside the painter',
    '?dMeter2Info_getWindowStatus@@YAEXZ': 'POST07: menu window open => no HUD',
    '?g_dComIfG_gameInfo@@3VdComIfG_inf_c@@A': 'POST07: game info (pause/event/player life/camera; draw list for our packet)',
    '?g_fopAcTg_Queue@@3Unode_list_class@@A': 'POST07: ONE read-only walk of the live actor queue per simulation tick (hud_sim_pass)',
    '?m_widthF@mDoGph_gInf_c@@2MA': 'POST07: logical viewport width (read)',
    '?m_heightF@mDoGph_gInf_c@@2MA': 'POST07: logical viewport height (read)',
    '?m_minXF@mDoGph_gInf_c@@2MA': 'POST07: logical viewport left (read)',
    '?m_minYF@mDoGph_gInf_c@@2MA': 'POST07: logical viewport top (read)',
    '?set@dDlst_list_c@@QEAAHAEAPEAPEAVdDlst_base_c@@0PEAV2@@Z': 'POST07: native 2D list insertion (inline set2DXlu) of OUR packet only',
    'PSVECSquareMag': 'POST07: cXyz::abs() of the camera-to-anchor vector (pure)',
    # POST07, found on the first MSVC link (run 41392200339, module map): /Zc:inline- + the inline virtual ~J2DOrthoGraph make MSVC emit
    # ??_7J2DGrafContext@@6B@ in mod.cpp.obj; its slots reference these EXE virtuals. Our code never calls them directly (the local ortho object
    # lives on the stack; setPort is called through J2DOrthoGraph::setPort, which the EXE implements by calling J2DGrafContext::setPort itself).
    '?place@J2DGrafContext@@UEAAXAEBU?$TBox2@M@JGeometry@@@Z': 'POST07: vtable slot of the compiler-emitted J2DGrafContext vtable (not called by the mod)',
    '?setPort@J2DGrafContext@@UEAAXXZ': 'POST07: vtable slot of the compiler-emitted J2DGrafContext vtable (not called by the mod)',
    '?setScissor@J2DGrafContext@@UEAAXXZ': 'POST07: vtable slot of the compiler-emitted J2DGrafContext vtable (not called by the mod)',
    '?setup2D@J2DGrafContext@@UEAAXXZ': 'POST07: vtable slot of the compiler-emitted J2DGrafContext vtable (not called by the mod)',
}
KINDS = {0: 'PAD', 1: 'HEADER', 2: 'IMPORT', 3: 'EXPORT', 4: 'HOOK_FN', 5: 'HOOK_MEM', 6: 'HOOK_NAME', 7: 'HOOK_MEM_EXT'}
# Simbolos cuyo IMPORT o presencia indicaria un uso que modifica el juego o parchea memoria (el patron exacto lo fija expected_pe.json).
FORBIDDEN = ['setActionMode', 'offTgSph', 'cc_at_check', 'at_power_check', 'ClrTgHit', 'ResetTgHit', 'onDownFlg', 'offDownFlg',
             'setStabPos', 'executeBigDamage', 'executeDeath', 'OffTgSetBit', 'OnTgSetBit', 'OffCoSetBit',
             'OnCoSetBit', 'ClrCcMove', 'fopAcM_delete', 'dComIfGp_setHitMark', 'WriteProcessMemory', 'VirtualProtect', 'funchook',
             'procGuardSlipInit', 'commonProcInit', 'setDamagePoint', 'procDamageInit', 'procCoLargeDamageInit',
             # FIX01-S2: no crouch shielding / crouch attacks / animation rewrites from LazyTweaks, no forced dodge procs, no raw pad tables, no Shield Attack init
             'procWait', 'procMove', 'setSmallGuard', 'setSingleAnime', 'handleQuickTransform', 'procGuardAttackInit', 'procCutNormalInit', 'setCutDash',
             'procBackJumpInit', 'procSideStepInit', 'procFrontRollInit', 'procSideRollInit', 'm_cpadInfo', 'dComIfGp_setRStatus', 'setDoStatus']


# Only these two fully decorated official native transitions are authorized
# by CP04. Keep the broad mutator ban for every other class/signature.
CP04_NATIVE_TRANSITIONS=frozenset({'?setActionMode@daE_OC_c@@QEAAXHH@Z','?procGuardAttackInit@daAlink_c@@QEAAHXZ'})
# FIX03 0.12.3 P1-4: the ONE reviewed raw pad symbol (the lock-on option writes this frame's logical L / right stick of PAD_1). Every other pad
# table, setter or proc init stays forbidden.
FIX03_PAD = '?m_cpadInfo@mDoCPd_c@@2PAUinterface_of_controller_pad@@A'
FIX03_HOOK_FN = ('dusklight.exe', '?def_se_set@@YAXPEAVZ2Creature@@PEAVcCcD_Obj@@IPEAVfopAc_ac_c@@@Z')
# LIZ 0.12.2: the exact native commit of Link's damage may be IMPORTED only as a hook target (its PMF). hook_only_called() proves no code calls it: this mod never inflicts damage on its own.
LIZ_HOOK_ONLY_TARGETS=frozenset({'?setDamagePoint@daAlink_c@@QEAAHHHHH@Z'})
# FIX03: hook targets the mod observes and NEVER calls (no code reference to their import thunk outside the hook record).
FIX03_HOOK_ONLY = frozenset({'?ProcAtTgHitmark@dCcS@@QEAAX_N0PEAVcCcD_Obj@@1PEAVdCcD_GObjInf@@2PEAVcCcD_Stts@@3PEAVdCcD_GStts@@4PEAUcXyz@@0@Z',
    '?setHitMark@dPa_control_c@@QEAAXGPEAVfopAc_ac_c@@PEBUcXyz@@PEBVcsXyz@@1I@Z',
    '?set@dPa_control_c@@QEAAPEAVJPABaseEmitter@@EGPEBUcXyz@@PEBVdKy_tevstr_c@@PEBVcsXyz@@0EPEAVdPa_levelEcallBack@@CPEBUGXColor@@40M@Z'})
def hook_only_called(raw,p,slots,wanted):
    text=[s for s in p.sections if s[0]=='.text']
    if len(text)!=1:
        return True
    _,vs,va,rs,ro=text[0]
    code=raw[ro:ro+min(vs,rs)]
    thunks=set()
    want_slots={k for k,v in slots.items() if v==wanted}
    for i in range(len(code)-5):
        if code[i]==0xff and code[i+1]==0x15 and va+i+6+struct.unpack_from('<i',code,i+2)[0] in want_slots:
            return True  # a call through the import slot
        if code[i]==0xff and code[i+1]==0x25 and va+i+6+struct.unpack_from('<i',code,i+2)[0] in want_slots:
            thunks.add(va+i)
    for i in range(len(code)-4):
        if code[i] in (0xe8,0xe9) and va+i+5+struct.unpack_from('<i',code,i+1)[0] in thunks:
            return True  # a call or a tail jump to the import thunk
    return False
def forbidden_decorated(raw,bad):
    for allowed in CP04_NATIVE_TRANSITIONS|LIZ_HOOK_ONLY_TARGETS:
        token=allowed.encode()+b'\0'
        raw=raw.replace(token,b'\0'*len(token))
    return ('?'+bad+'@').encode() in raw

class PE:
    def __init__(self, raw):
        self.raw = raw
        if raw[:2] != b'MZ':
            raise ValueError('not MZ')
        self.pe = struct.unpack_from('<I', raw, 0x3c)[0]
        if raw[self.pe:self.pe + 4] != b'PE\0\0':
            raise ValueError('not PE')
        self.machine, self.nsec = struct.unpack_from('<HH', raw, self.pe + 4)
        optsize = struct.unpack_from('<H', raw, self.pe + 20)[0]
        self.opt = self.pe + 24
        if struct.unpack_from('<H', raw, self.opt)[0] != 0x20B:
            raise ValueError('not PE32+')
        self.image_base = struct.unpack_from('<Q', raw, self.opt + 24)[0]
        self.sections = []
        s = self.opt + optsize
        for i in range(self.nsec):
            o = s + 40 * i
            name = raw[o:o + 8].rstrip(b'\0').decode('ascii', 'replace')
            vs, va, rs, ro = struct.unpack_from('<IIII', raw, o + 8)
            self.sections.append((name, vs, va, rs, ro))

    def datadir(self, i):
        return struct.unpack_from('<II', self.raw, self.opt + 112 + 8 * i)

    def off(self, rva):
        for _, vs, va, rs, ro in self.sections:
            if va <= rva < va + max(vs, rs) and rva - va < rs:
                return ro + rva - va
        raise ValueError('unmapped rva %#x' % rva)

    def cstr(self, rva):
        o = self.off(rva)
        return self.raw[o:self.raw.index(b'\0', o)].decode('ascii', 'replace')

    def imports(self):
        out, slots = {}, {}
        rva, size = self.datadir(1)
        if not rva:
            return out, slots
        q = self.off(rva)
        while any(self.raw[q:q + 20]):
            oft, _, _, name, ft = struct.unpack_from('<5I', self.raw, q)
            dll = self.cstr(name)
            t = self.off(oft or ft)
            i = 0
            names = []
            while True:
                v = struct.unpack_from('<Q', self.raw, t + 8 * i)[0]
                if not v:
                    break
                if v >> 63:
                    n = 'ORDINAL#%d' % (v & 0xFFFF)
                else:
                    n = self.cstr((v & 0x7FFFFFFF) + 2)
                names.append(n)
                slots[ft + 8 * i] = (dll, n)
                i += 1
            out[dll] = sorted(names)
            q += 20
        return out, slots

    def exports(self):
        rva, size = self.datadir(0)
        if not rva:
            return []
        o = self.off(rva)
        count = struct.unpack_from('<I', self.raw, o + 24)[0]
        names_rva = struct.unpack_from('<I', self.raw, o + 32)[0]
        no = self.off(names_rva)
        return sorted(self.cstr(struct.unpack_from('<I', self.raw, no + 4 * i)[0]) for i in range(count))

    def meta_records(self):
        sec = [s for s in self.sections if s[0] == 'modmeta']
        if len(sec) != 1:
            raise ValueError('modmeta section count %d' % len(sec))
        _, vs, _, rs, ro = sec[0]
        end = ro + min(vs, rs)
        n = ro
        recs = []
        while n + 8 <= end:
            if not any(self.raw[n:n + 8]):
                n += 8
                continue
            size, kind, flags = struct.unpack_from('<HBB', self.raw, n)
            if size < 8 or size % 8 or n + size > end:
                raise ValueError('bad meta record size %d at %#x' % (size, n))
            rec = {'kind': KINDS.get(kind, 'KIND_%d' % kind), 'size': size, 'flags': flags}
            if kind == 1:
                rec['abi'] = struct.unpack_from('<I', self.raw, n + 4)[0]
            elif kind == 2:
                rec['major'], rec['min_minor'] = struct.unpack_from('<HH', self.raw, n + 4)
                rec['service'] = self.raw[n + 16:n + 80].split(b'\0')[0].decode('ascii', 'replace')
            elif kind == 5:
                rec['pmf'] = self.raw[n + 8:n + 24]
                rec['resolved'] = struct.unpack_from('<Q', self.raw, n + 24)[0]
                rec['names'] = [s.decode('ascii') for s in self.raw[n + 32:n + size].split(b'\0') if s]
            elif kind == 6:
                rec['resolved'] = struct.unpack_from('<Q', self.raw, n + 8)[0]
                rec['names'] = [x.decode('ascii') for x in self.raw[n + 16:n + size].split(b'\0') if x]
            elif kind == 4:  # FIX03: ModMetaHookFn {rec, reserved, target (the &fn relocation), resolved}
                rec['target'] = struct.unpack_from('<Q', self.raw, n + 8)[0]
                rec['resolved'] = struct.unpack_from('<Q', self.raw, n + 16)[0]
            elif kind == 7:
                rec['pmf_size'] = struct.unpack_from('<I', self.raw, n+4)[0]
                rec['materialize'] = struct.unpack_from('<Q', self.raw, n+8)[0]
                rec['resolved'] = struct.unpack_from('<Q', self.raw, n+16)[0]
                rec['names'] = [s.decode('ascii') for s in self.raw[n+24:n+size].split(b'\0') if s]
            recs.append(rec)
            n += size
        return recs


def vcall_slot(raw, p, va):
    """Python replica of the host's x64 vcall_slot_offset: follow E9 stubs, then match mov rax,[rcx]; jmp [rax(+imm)] (MSVC) or
    mov rax,[rcx]; mov rax,[rax(+imm)]; jmp rax (clang-cl). Returns the slot byte offset or None."""
    o = p.off(va - p.image_base)
    for _ in range(8):
        if raw[o] != 0xE9:
            break
        va = va + 5 + struct.unpack_from('<i', raw, o + 1)[0]
        o = p.off(va - p.image_base)
    w = raw[o:o + 32]
    q = None
    for i in range(13):
        if w[i:i + 3] == b'\x48\x8b\x01':
            q = i + 3
            break
    if q is None:
        return None
    if w[q:q + 2] == b'\xff\x20':
        return 0
    if w[q:q + 2] == b'\xff\x60':
        return struct.unpack_from('<b', w, q + 2)[0]
    if w[q:q + 2] == b'\xff\xa0':
        return struct.unpack_from('<i', w, q + 2)[0]
    if w[q:q + 2] == b'\x48\x8b' and w[q + 2] in (0x00, 0x40, 0x80):
        off, r = 0, q + 3
        if w[q + 2] == 0x40:
            off, r = struct.unpack_from('<b', w, q + 3)[0], q + 4
        elif w[q + 2] == 0x80:
            off, r = struct.unpack_from('<i', w, q + 3)[0], q + 7
        if any(w[r + i:r + i + 2] == b'\xff\xe0' for i in range(9)):
            return off
    return None


def audit(path, expected=None):
    raw = Path(path).read_bytes()
    p = PE(raw)
    r = {'file_bytes': len(raw), 'machine': hex(p.machine), 'sections': [s[0] for s in p.sections]}
    errors = []
    if p.machine != 0x8664:
        errors.append('machine is not AMD64')
    for idx, label in [(9, 'TLS'), (13, 'DELAY_IMPORT')]:
        if any(p.datadir(idx)):
            errors.append(label + ' directory present')
    imports, slots = p.imports()
    r['imports'] = imports
    r['exports'] = p.exports()
    recs = p.meta_records()
    r['meta_kinds'] = {}
    for rec in recs:
        r['meta_kinds'][rec['kind']] = r['meta_kinds'].get(rec['kind'], 0) + 1
    r['meta_imports'] = sorted((x['service'], x['major'], x['min_minor']) for x in recs if x['kind'] == 'IMPORT')
    hooks = [x for x in recs if x['kind'] in ('HOOK_MEM','HOOK_MEM_EXT')]
    if r['meta_kinds'].get('HOOK_MEM') != 18 or r['meta_kinds'].get('HOOK_MEM_EXT') != 14 or r['meta_kinds'].get('HOOK_NAME') != len(LIZ_HOOK_NAMES) or r['meta_kinds'].get('HOOK_FN') != 1 or 'EXPORT' in r['meta_kinds']:
        errors.append('expected exactly 18 HOOK_MEM (15 + FIX03 ProcAtTgHitmark, setHitMark, dPa set) + 14 audited Link HOOK_MEM_EXT + %d HOOK_NAME (LIZ E_DN statics + FIX03 startCollisionSE) + 1 HOOK_FN (FIX03 def_se_set) and no export record: %s' % (len(LIZ_HOOK_NAMES), r['meta_kinds']))
    # FIX03: the single function hook binds the def_se_set import thunk (E_DN's own shield SE of a parry contact); never another function.
    for h in [x for x in recs if x['kind'] == 'HOOK_FN']:
        try:
            o = p.off(h['target'] - p.image_base)
            fb = slots.get((h['target'] - p.image_base) + 6 + struct.unpack_from('<i', raw, o + 2)[0]) if raw[o:o + 2] == b'\xff\x25' else None
        except (ValueError, struct.error):
            fb = None
        r['hook_fn'] = list(fb) if fb else None
        if fb != FIX03_HOOK_FN or h['resolved'] != 0:
            errors.append('FIX03 function hook not bound to def_se_set: %s' % (fb,))
    # LIZ 0.12.0: by-name records resolved by the host's symbol manifest. Exactly these file-local E_DN display aliases (manifest flags: code,local,display; never inline, multi or dup).
    if sorted(n for x in recs if x['kind'] == 'HOOK_NAME' for n in x['names']) != sorted(LIZ_HOOK_NAMES) or any(x['flags'] for x in recs if x['kind'] == 'HOOK_NAME'):
        errors.append('LIZ by-name hook set changed')
    if r['meta_kinds'].get('HEADER') != 1:
        errors.append('expected one HEADER record')
    bound = None
    expected_hooks={'static_cast<void (dMeter2Draw_c::*)(f32,f32,f32*,f32,JUtility::TColor,JUtility::TColor,JUtility::TColor,JUtility::TColor,f32,u8,f32)>(&dMeter2Draw_c::drawPikari)':'?drawPikari@dMeter2Draw_c@@QEAAXMMPEAMMUTColor@JUtility@@111MEM@Z','static_cast<void (J2DPicture::*)(f32,f32,f32,f32,bool,bool,bool)>(&J2DPicture::draw)':None,'COutFont_c::draw':None,'J2DPane::draw':'?draw@J2DPane@@QEAAXMMPEBVJ2DGrafContext@@_N1@Z','JUTFont::drawString_size_scale':'?drawString_size_scale@JUTFont@@QEAAMMMMMPEBDI_N@Z','J2DScreen::draw':'?draw@J2DScreen@@QEAAXMMPEBVJ2DGrafContext@@@Z','daE_OC_c::damage_check':HOOK_IMPORT,'daE_OC_c::executeDamage':'?executeDamage@daE_OC_c@@QEAAXXZ',
                    'dMeter2_c::_execute':'?_execute@dMeter2_c@@QEAAHXZ','dMeter2_c::_draw':'?_draw@dMeter2_c@@QEAAHXZ','dMeter2_c::_delete':'?_delete@dMeter2_c@@QEAAHXZ',
                    'dMeterButton_c::draw':None,'daAlink_c::checkDamageAction':'?checkDamageAction@daAlink_c@@QEAAHXZ','daAlink_c::checkPolyDamage':'?checkPolyDamage@daAlink_c@@QEAAHXZ','daE_OC_c::executeAttack':'?executeAttack@daE_OC_c@@QEAAXXZ','daE_OC_c::cc_set':'?cc_set@daE_OC_c@@QEAAXXZ',
                    'daAlink_c::setStickData':'?setStickData@daAlink_c@@QEAAXXZ','daAlink_c::setShieldGuard':'?setShieldGuard@daAlink_c@@QEAAXXZ',
                    'static_cast<void (daAlink_c::*)(u8)>(&daAlink_c::setRStatus)':'?setRStatus@daAlink_c@@QEAAXE@Z','daAlink_c::spActionTrigger':'?spActionTrigger@daAlink_c@@QEAAHXZ',
                    'daAlink_c::checkMoveDoAction':'?checkMoveDoAction@daAlink_c@@QEAAHXZ','daAlink_c::setCollision':'?setCollision@daAlink_c@@QEAAXXZ','daE_OC_c::execute':'?execute@daE_OC_c@@QEAAHXZ','daAlink_c::procGuardAttackInit':'?procGuardAttackInit@daAlink_c@@QEAAHXZ',
                    'dCcS::ProcAtTgHitmark':'?ProcAtTgHitmark@dCcS@@QEAAX_N0PEAVcCcD_Obj@@1PEAVdCcD_GObjInf@@2PEAVcCcD_Stts@@3PEAVdCcD_GStts@@4PEAUcXyz@@0@Z','dPa_control_c::setHitMark':'?setHitMark@dPa_control_c@@QEAAXGPEAVfopAc_ac_c@@PEBUcXyz@@PEBVcsXyz@@1I@Z',
                    FIX03_SET:'?set@dPa_control_c@@QEAAPEAVJPABaseEmitter@@EGPEBUcXyz@@PEBVdKy_tevstr_c@@PEBVcsXyz@@0EPEAVdPa_levelEcallBack@@CPEBUGXColor@@40M@Z',
                    'daAlink_c::modelDraw':'?modelDraw@daAlink_c@@QEAAXPEAVJ3DModel@@H@Z','daAlink_c::setCutType':'?setCutType@daAlink_c@@QEAAXE@Z','daAlink_c::setGuardSe':'?setGuardSe@daAlink_c@@QEAAXPEAVdCcD_GObjInf@@@Z','daAlink_c::checkCutReverseAt':'?checkCutReverseAt@daAlink_c@@QEAAHPEAVdCcD_GObjInf@@@Z','daAlink_c::setDamagePoint':'?setDamagePoint@daAlink_c@@QEAAHHHHH@Z'}
    ext_ok={'daAlink_c::procGuardAttackInit','daAlink_c::checkDamageAction','daAlink_c::checkPolyDamage','daAlink_c::setStickData','daAlink_c::setShieldGuard','static_cast<void (daAlink_c::*)(u8)>(&daAlink_c::setRStatus)',
            'daAlink_c::spActionTrigger','daAlink_c::checkMoveDoAction','daAlink_c::setCollision','daAlink_c::modelDraw','daAlink_c::setCutType','daAlink_c::setGuardSe','daAlink_c::checkCutReverseAt','daAlink_c::setDamagePoint'}
    if {h['names'][-1] for h in hooks}!=set(expected_hooks):errors.append('unexpected hook target set')
    for h in hooks:
        if h['kind']=='HOOK_MEM_EXT':
            try:
                if h['names'][-1] not in ext_ok or h['pmf_size']!=24 or h['flags']!=0:
                    raise ValueError('unapproved extended member target/layout')
                from pmf_audit import decode_materializer
                h['pmf']=decode_materializer(p,h['materialize'],h['pmf_size'],slots)
                r.setdefault('extended_materializers',{})[h['names'][-1]]={'size':24,'static_decode':'READ_ONLY_STORES_TO_OUTPUT_ONLY','target_and_adjustments_hex':h['pmf'][:20].hex(),'padding':'bytes20..23 unspecified native ABI padding; never interpreted'}
            except (ValueError,KeyError,IndexError) as ex:
                errors.append('extended PMF rejected: '+str(ex));continue
        elif 'daAlink_c::' in h['names'][-1]:
            errors.append('Link hook must preserve SDK native extended PMF representation')
        cls = 'dPa_control_c' if h['names'][-1] == FIX03_SET else 'dMeter2Draw_c' if h['names'][-1]=='static_cast<void (dMeter2Draw_c::*)(f32,f32,f32*,f32,JUtility::TColor,JUtility::TColor,JUtility::TColor,JUtility::TColor,f32,u8,f32)>(&dMeter2Draw_c::drawPikari)' else 'J2DPicture' if h['names'][-1]=='static_cast<void (J2DPicture::*)(f32,f32,f32,f32,bool,bool,bool)>(&J2DPicture::draw)' else 'daAlink_c' if h['names'][-1] == 'static_cast<void (daAlink_c::*)(u8)>(&daAlink_c::setRStatus)' else h['names'][-1].split('::')[0]
        if h['names'][-1] not in expected_hooks or h['names'] != [HOOK_VTABLE.get(cls), h['names'][-1]]:
            errors.append('hook names %s' % h['names'])
        if h['resolved'] != 0 or any(h['pmf'][8:]):
            errors.append('PMF slot not a zero-adjustment compact pointer / resolved slot not initially zero')
        va = struct.unpack_from('<Q', h['pmf'], 0)[0]
        if h['names'][-1] in VCALL_HOOKS:  # LNK01: local vcall thunk, exact slot, never an import
            try:
                slot_off = vcall_slot(raw, p, va)
            except (ValueError, IndexError) as ex:
                slot_off = None
                errors.append('vcall PMF target unmapped: %s' % ex)
            r.setdefault('vcall_hooks', {})[h['names'][-1]] = slot_off
            if slot_off != VCALL_HOOKS[h['names'][-1]]:
                errors.append('virtual hook %s: PMF is not a slot-%d vcall thunk (got %s)' % (h['names'][-1], VCALL_HOOKS[h['names'][-1]], slot_off))
            r.setdefault('hook_thunks', {})[h['names'][-1]] = None
            continue
        try:
            o = p.off(va - p.image_base)
            if raw[o:o + 2] != b'\xff\x25':
                errors.append('PMF does not point to an import thunk')
            else:
                slot = (va - p.image_base) + 6 + struct.unpack_from('<i', raw, o + 2)[0]
                bound = slots.get(slot)
        except ValueError as ex:
            errors.append('PMF target unmapped: %s' % ex)
        r.setdefault('hook_thunks', {})[h['names'][-1]] = list(bound) if bound else None
        r['hook_thunk_bound_import'] = r['hook_thunks'].get('daE_OC_c::damage_check')
        if not bound or bound != ('dusklight.exe', expected_hooks.get(h['names'][-1])):
            errors.append('hook thunk bound to %s' % (bound,))
    flat = [n for names in imports.values() for n in names]
    game = sorted(imports.get('dusklight.exe', []))
    r['game_imports'] = {n: GAME_IMPORT_REASONS.get(n, 'UNREVIEWED') for n in game}
    for n in game:
        if n not in GAME_IMPORT_REASONS:
            errors.append('unreviewed game import ' + n)
    for n in GAME_IMPORT_REASONS:
        if n not in game:
            errors.append('reviewed game import missing: ' + n)
    for bad in FORBIDDEN:
        hit = [n for n in flat if bad in n and n not in CP04_NATIVE_TRANSITIONS and n not in LIZ_HOOK_ONLY_TARGETS and n != FIX03_PAD] + [e for e in r['exports'] if bad in e]
        if hit:
            errors.append('forbidden symbol %s in imports/exports: %s' % (bad, hit))
        if forbidden_decorated(raw,bad) and not (bad == 'm_cpadInfo' and raw.count(b'?m_cpadInfo@') == raw.count(FIX03_PAD.encode())):
            # Un nombre MSVC decorado de un mutador (?nombre@...) no debe aparecer en la imagen; el texto libre de un log (p. ej. «cc_at_check=UNDETERMINED») no es un simbolo.
            errors.append('forbidden decorated symbol ?%s@ present in image' % bad)
    for n in sorted(LIZ_HOOK_ONLY_TARGETS | FIX03_HOOK_ONLY):
        if hook_only_called(raw, p, slots, ('dusklight.exe', n)):
            errors.append('hook-only target is called by code: ' + n)
    r['errors'] = errors
    if expected is not None:
        exp = json.loads(Path(expected).read_text())
        for key in ('imports', 'exports', 'meta_imports', 'meta_kinds'):
            got = json.loads(json.dumps(r[key]))
            if got != exp[key]:
                errors.append('PIN_MISMATCH %s: got %s expected %s' % (key, got, exp[key]))
        r['pinned'] = True
    else:
        r['pinned'] = False
    r['status'] = 'PE_AUDIT_PASS' if not errors and r['pinned'] else ('PE_AUDIT_UNPINNED' if not errors else 'PE_AUDIT_FAIL')
    return r


if __name__ == '__main__':
    exp = None
    if '--expected' in sys.argv:
        exp = sys.argv[sys.argv.index('--expected') + 1]
    res = audit(sys.argv[1], exp)
    print(json.dumps(res, indent=2, sort_keys=True))
    sys.exit(0 if res['status'] == 'PE_AUDIT_PASS' else 2)
