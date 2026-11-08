"""POST05: auditoria estatica de PASIVIDAD sobre src/*.cpp|hpp (texto). No es una prueba formal: es una red contra regresiones.
Uso: source_audit.py <proyecto>  -> JSON; salida distinta de 0 si falla alguna regla."""
import json
import re
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
import cp04_audit
import cp05_audit
import cp05_fix01_audit


def strip_comments(t):
    t = re.sub(r'/\*.*?\*/', ' ', t, flags=re.S)
    t = re.sub(r'//[^\n]*', ' ', t)
    return re.sub(r'"(?:\\.|[^"\\])*"', '""', t)  # cadenas fuera (los textos de log no cuentan como codigo)


FORBIDDEN_CALLS = ['setActionMode', 'offTgSph', 'cc_at_check', 'at_power_check', 'ClrTgHit', 'ResetTgHit', 'OffTgSetBit',
                   'OnTgSetBit', 'OffCoSetBit', 'OnCoSetBit', 'onDownFlg', 'offDownFlg', 'offCutDownHitFlg', 'onCutDownHitFlg',
                   'setStabPos', 'setBck', 'executeBigDamage', 'executeDamage', 'damage_check(', 'Move()', 'fopAcM_delete',
                   'dComIfGp_setHitMark', 'mDoAud', 'startCollisionSE', 'startCreatureVoice', 'SetAtAtp',
                   # POST05-RB: la animacion y la velocidad solo se LEEN
                   'setPlaySpeed', 'setFrame', 'setAnm', 'setRate(', 'setAttribute', 'setEnd(', 'setStart(', 'setLoop(', 'changeAnm', 'checkBck']
FORBIDDEN_TOKENS = ['HOOK_SKIP_ORIGINAL', 'replace<', 'DEFINE_HOOK_SYMBOL', 'VirtualProtect', 'WriteProcessMemory', 'funchook',
                    'memcpy', 'memset', 'const_cast', 'reinterpret_cast<std::uintptr_t*>',
                    'fpcM_Search', 'fpcEx_Search', 'fopAcIt_', 'dComIfG_Ccsp', 'fopAcM_SearchByName', 'fopAcM_Search(',
                    'GetTgHitAc', 'GetAc(', 'IsBadReadPtr', 'IsBadWritePtr', 'VirtualQuery', '__try', '__except', 'SetUnhandledExceptionFilter', 'mHitType', 'mHitBit', 'mAttackPower', 'mpActor', 'mpSound', 'mHitDirection', 'mPowerType',
                    'mHitStatus', 'getCutType', 'daPy_getPlayerActorClass', 'mStts']
# POST04: nada de memoria dinamica, E/S, hilos ni dependencias de otros mods en el camino de la mod (memoria O(64) fija)
DYNAMIC = [r'\bnew\b', r'\bdelete\b', r'\bmalloc\b', r'\bcalloc\b', r'\brealloc\b', r'\bfree\s*\(', r'std::vector', r'std::map', r'std::unordered', r'std::string\b',
           r'std::unique_ptr', r'std::shared_ptr', r'make_unique', r'make_shared', r'std::function', r'std::mutex', r'std::thread', r'std::atomic', r'std::ofstream',
           r'std::ifstream', r'std::fstream', r'\bfopen\b', r'\bfwrite\b', r'std::cout', r'std::cerr', r'std::list', r'std::deque', r'std::set\b', r'(?<![a-z_])printf\s*\(']
OTHER_MODS = [r'EnemyHealth', r'enemy_health', r'HmService', r'HealthBars', r'bars_203', r'\bDEVF', r'devf', r'Bokoblin', r'bokoblin', r'Expansion']
# LNK SESSION02 FIX01 0.11.1: excepciones EXACTAS (texto + numero + funcion anfitriona). Todo lo demas sigue prohibido.
FX_SKIP = '    return HOOK_SKIP_ORIGINAL;\n'
FX_FLAG_OFF = 'link->offNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);'
FX_B_CLEAR = 'link->mItemTrigger = static_cast<u8>(link->mItemTrigger & ~static_cast<unsigned>(daAlink_c::BTN_B));'
FX_FUNCS = ('void on_stick_post_fx(', 'void on_shield_post_fx(', 'HookAction on_rstatus_pre_fx(', 'void on_spaction_post_fx(', 'HookAction on_movedo_pre_fx(',
            'HookAction on_collision_pre_fx(')
FX_FORBIDDEN = ['checkShieldCrouch', 'setShieldCrouch', 'ANM_CROUCH_DEF', 'CROUCHDEF', 'setSmallGuard', 'procWait', 'procMove(', 'handleQuickTransform', 'setSingleAnime',
                'mButtonZAlpha', 'getHoldLockR', 'getTrigB', 'getTrigLockR', 'm_cpadInfo', 'mDoCPd_c', 'onNoResetFlg', 'mGuardBreakAnm', 'mpHIO', 'procBackJumpInit',
                'procSideStepInit', 'procFrontRollInit', 'procSideRollInit', 'procGuardAttackInit', 'procCutNormalInit', 'setCutDash', 'dComIfGp_setRStatus',
                'setDoStatus', 'mItemButton =', 'mItemButton &=', 'mItemButton |=', 'mItemTrigger |=', 'mItemTrigger &=', 'mStickValue =', 'mNormalSpeed']


def fx_body(text, head):
    m = re.search(re.escape(head) + r'.*?\n\}\n', text, flags=re.S)
    return m


def fx_strip(text):
    """mod.cpp sin las construcciones exactas auditadas de FIX01-S2 (solo si estan en su funcion anfitriona; si no, se dejan y el escaneo general falla)."""
    out = text
    for head, token in (('HookAction on_rstatus_pre_fx(', FX_SKIP), ('HookAction on_movedo_pre_fx(', FX_SKIP), ('void on_shield_post_fx(', FX_FLAG_OFF),
                        ('HookAction on_collision_pre_fx(', FX_FLAG_OFF), ('void on_stick_post_fx(', FX_B_CLEAR)):
        m = fx_body(out, head)
        if m and m.group(0).count(token) == 1:
            out = out[:m.start()] + m.group(0).replace(token, '/*FX_AUDITED*/') + out[m.end():]
    return fix02_strip(out)



# FIX02 exceptions are complete function hashes; no wildcard writer exemption.
FIX02_PINS = {'on_picture_pre_fix02': '2a9b66fac5a983cd13f533ea778055a8fc5fbb470c5fb8f7872cf4ecf03f2c51', 'on_outfont_pre_fix02': 'd8f9bc70481fed6eaad15f3d6ccfa2b44359fa7235de5460f935a7d419e563f8', 'on_string_pre_fix02': 'e8d28cc0c34a9374af5bdf475853d62c8ecb801692726fd0360dfb1d83313d8e', 'on_string_post_fix02': '541c923ca386704b316ee988b5c4545121771228f41481feb682f92ff0259088', 'on_pane_pre_fix02': 'ae8defb0c1ca6325f2ce3c3308cb44eb62fbd7c41bd1bb4dba64444924ce3508', 'on_pane_post_fix02': 'e7fe26563167f2ed34c20d6cf915d5d0ba54fe8f1d837af549ef4ea4eb26b155'}
def fix02_spans(text):
    spans=[]
    for name,want in FIX02_PINS.items():
        m=re.search(r'^(?:HookAction|void) '+name+r'\([^;{]*\) noexcept \{',text,re.M)
        if not m:continue
        pos=m.end();depth=1
        while pos<len(text) and depth:
            if text[pos]=='{':depth+=1
            elif text[pos]=='}':depth-=1
            pos+=1
        body=re.sub(r'\s+','',text[m.start():pos])
        import hashlib
        if hashlib.sha256(body.encode()).hexdigest()==want:spans.append((m.start(),pos))
    return spans

def fix02_strip(text):
    for a,b in sorted(fix02_spans(text),reverse=True):
        text=text[:a]+text[a:b].replace('HOOK_SKIP_ORIGINAL','HOOK_CONTINUE')+text[b:]
    # Authentic read-only parent lookup, restricted to its exact original body.
    exact='bool allied(const fopAc_ac_c* actor) {\n    if (actor->name == fpcNm_E_HB_e && fpcM_GetParam(actor) == 0xffff0040) return true;\n    if (actor->name == fpcNm_E_HB_LEAF_e) {\n        auto* parent = static_cast<fopAc_ac_c*>(fpcM_SearchByID(actor->parentActorID));\n        return parent && parent->name == fpcNm_E_HB_e && fpcM_GetParam(parent) == 0xffff0040;\n    }\n    return false;\n}'
    body=re.search(r'bool allied\(const fopAc_ac_c\* actor\) \{.*?\n\}',text,re.S)
    if body and re.sub(r'\s+','',body.group(0))==re.sub(r'\s+','',exact):
        text=text[:body.start()]+body.group(0).replace('fpcM_SearchByID(actor->parentActorID)','FIX02_READ_PARENT(actor->parentActorID)')+text[body.end():]
    return text

LOCAL_INCLUDES = {'fix01_core.hpp','cp05_fix02_core.hpp','cp04_combo_core.hpp','shield_chain_core.hpp','cp06/tracker.h', 'hudfix_core.hpp', 'balance_core.hpp', 'balance_adapter.hpp', 'lnks2fix_core.hpp', 'lnks2_core.hpp', 'engine.hpp', 'hit_core.hpp', 'lifecycle_core.hpp', 'posture_core.hpp', 'reaction_core.hpp', 'hud_core.hpp', 'parry_core.hpp', 'lnk_core.hpp'}
# LNK01: la supresion de prompts es SOLO visual y focal: ni ocultar todo el HUD, ni alfas/padres globales, ni busquedas de panes, ni volver a mostrar nada.
LNK01_FORBIDDEN = ['->show(', '.show(', 'hideAll', '2dShowOff', 'mParentAlpha', 'setAlphaRate', 'g_drawHIO', 'search(', 'emphasisButtonDelete',
                   'mpParent', 'field_0x2fd5', 'mpButtonParent', 'mpMeterDraw->mpScreen->hide', 'getMainScreenPtr', 'mStatus =', 'setString(', 'mpTextScreen',
                   'mpScreen->hide', 'dMeter2Info_set', 'dComIfGp_set', 'mDoAud', 'setLife', 'setMaxLife', 'onNoResetFlg', 'offNoResetFlg', 'setItemLifeCount',
                   'setDamagePoint', 'clearItemLifeCount', 'setDamageColorTime']
STD_INCLUDES = {'cstddef', 'cstdint', 'cstdio', 'cstring', 'type_traits'}
ALLOWED_SERVICES = {'HostService', 'LogService', 'HookService'}
ALLOWED_OPTIONAL_SERVICES = {'InterpService', 'GfxService', 'HmVitalsService'}  # POST07: presentation interpolation of the HUD anchor only
# Asignacion / incremento sobre un miembro alcanzado desde un puntero (a->x = ..., a->x[i].y += ..., ++a->x)
ASSIGN = re.compile(r'(?<![\w.])(?:a|out)\s*->\s*[\w\[\]\.>\-\(\)]*?\s*(=(?!=)|\+=|-=|\|=|&=|\^=|<<=|>>=|\+\+|--)')
PREINC = re.compile(r'(\+\+|--)\s*(?:a|out)\s*->')



CP06_PRODUCTION = {'tektite_view.h': '19494f125ff117262d08a32e85b0b53eb524ea3b9ef57d0176cd32f4a477b67d', 'tracker.cpp': '31c9cf0e024df6ff21f68bbe2540a11a2640c30fd191f4551daf93589ace26d4', 'armos_view.h': '95162b7e3cc95fc38d76cf17f2a02945f09339ce7bcb53e11b9fde06e8d80f02', 'hm_bridge.cpp': 'cc4876a2475866aaba1e4501fc1f47ea3aea98ac687dcb2827216a4b91844f64', 'hm_vitals_api.h': '5f592e4a21190eb2353a4a78a5ee70cfb2ce3b2bed7a54076ce8d80dbaa692c7', 'tracker.h': 'c34732d645a53de701989f40ddd4e8c3082c68864b484b47f5feac8313d0d981', 'hm_bridge.h': '507cf0f54a5c80b87f8dd5bb22a95d5408aaf3d0f202f2bcef0a0edae4b3467b', 'reused/health_model.h': 'fb81b53bb5df603ee42340730527d2abd7af04f71dab337bed65c5c6c6b60ca5', 'reused/normal_model.h': 'bfa08846d7c397261bff2298b9e812230d79e56c291d9525034dd5047f795461', 'reused/boss_catalog.h': '1bad1eb79e610d4de13e10f96203b9ec60e43b4d49f634f2f93ec58f37977630', 'reused/normal_runtime.h': 'b3cc945aa59572ed28f7d9d87c2731cff67fcd2b0e8a965e887b52c9ac946eb7'}
CP06_ORACLE = {'tektite_view.h': '19494f125ff117262d08a32e85b0b53eb524ea3b9ef57d0176cd32f4a477b67d', 'tracker.cpp': '2472d6c45901c699d3234c0c55179e044b62822c53439f6998bb24a7bd2a3340', 'armos_view.h': '95162b7e3cc95fc38d76cf17f2a02945f09339ce7bcb53e11b9fde06e8d80f02', 'hm_bridge.cpp': 'cc4876a2475866aaba1e4501fc1f47ea3aea98ac687dcb2827216a4b91844f64', 'hm_vitals_api.h': '5f592e4a21190eb2353a4a78a5ee70cfb2ce3b2bed7a54076ce8d80dbaa692c7', 'tracker.h': '1eb0035d627661962ba46b9760f6d4c58d2a679aa71a94ea0504587d518cd84a', 'hm_bridge.h': '507cf0f54a5c80b87f8dd5bb22a95d5408aaf3d0f202f2bcef0a0edae4b3467b', 'reused/health_model.h': 'fb81b53bb5df603ee42340730527d2abd7af04f71dab337bed65c5c6c6b60ca5', 'reused/normal_model.h': 'bfa08846d7c397261bff2298b9e812230d79e56c291d9525034dd5047f795461', 'reused/boss_catalog.h': '1bad1eb79e610d4de13e10f96203b9ec60e43b4d49f634f2f93ec58f37977630', 'reused/normal_runtime.h': 'b3cc945aa59572ed28f7d9d87c2731cff67fcd2b0e8a965e887b52c9ac946eb7', 'reused/recovered/normal_renderer.h': 'adf72b8c1fcfeb17b4b4c0fc554c7f6ff82634cd429a472e2d2d0b879b324179', 'reused/recovered/renderer_geometry.h': '6bb9bc36f3c684d6c5ac7ac5c9d41f27a7950e6f5efe3197a683c948abdc672f', 'reused/recovered/renderer.h': '5e928e67a4df79f1a5732659002706e9bce87aaf9eec1fff088553db29c91c09'}
def fix02_vendor(project):
    import hashlib
    errors=[]
    for folder,expected in [('src/cp06',CP06_PRODUCTION),('tests/cp06_reference',CP06_ORACLE)]:
        actual={str(p.relative_to(project/folder)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in (project/folder).rglob('*') if p.is_file()}
        if actual!=expected:errors.append('CP06 exact authenticated inventory changed: '+folder)
    mod=(project/'src/mod.cpp').read_text(encoding='utf-8')
    for token in ('add_pre<ScreenDrawHudFix01>(on_screen_pre_fix02)', 'hg.mirror.sample(', 'hg.mirror.tracker.presented(', 'hud_interp_lookup(hg.frame_interp[k], world)'):
        if token not in mod:errors.append('FIX02 production binding missing: '+token)
    if 'add_pre<ScreenDrawHudFix01>(on_screen_pre_hudfix)' in mod:errors.append('FIX02 must not register retired reflow')
    return errors

def main(project):
    project = Path(project)
    res = {'files': [], 'errors': []}
    import liz_audit
    res['errors'].extend(liz_audit.audit(project))
    import cp06f1_audit
    res['errors'].extend(cp06f1_audit.audit(project))
    import cp06_audit
    res['errors'].extend(cp06_audit.audit(project))
    import cp05_fix02_audit
    res['errors'].extend(cp05_fix02_audit.audit(project))
    res['errors'].extend(cp05_audit.audit(project))
    res['errors'].extend(cp05_fix01_audit.audit(project))
    res['errors'].extend(cp04_audit.audit(project))
    res['errors'].extend(fix02_vendor(project))
    import fix03_audit
    res['errors'].extend(fix03_audit.audit(project))
    import store_audit
    res['errors'].extend(store_audit.source(project,json.loads((project/'ci/baseline_0_11_2_barrier.json').read_text())))
    hook_defs = 0
    all_text = []
    for f in sorted((project / 'src').glob('*')):
        if f.suffix not in ('.cpp', '.hpp'):
            continue
        text = strip_comments(cp04_audit.legacy_source(f.name,f.read_text(encoding='utf-8')))
        text = re.sub(r'static_assert\((?:[^;])*?\);', ' ', text, flags=re.S)  # los static_assert son de compilacion, no de ejecucion
        res['files'].append(f.name)
        raw_text = re.sub(r'/\*.*?\*/', ' ', cp04_audit.legacy_source(f.name,f.read_text(encoding='utf-8')), flags=re.S)
        raw_text = re.sub(r'//[^\n]*', ' ', raw_text)
        for pat in (r'%p\b', r'%[0-9]*(?:ll|l|z|I64|j|t)[xX]', r'%0?16[xX]', r'PRIx', r'PRIX'):
            for m in re.finditer(pat, raw_text):
                res['errors'].append('%s: pointer-width hex formatting is forbidden in log strings (%s)' % (f.name, m.group(0)))
        for pat in DYNAMIC:
            if re.search(pat, text):
                res['errors'].append('%s: dynamic allocation / IO / threading is forbidden (%s)' % (f.name, pat))
        for pat in OTHER_MODS:
            if re.search(pat, text):
                res['errors'].append('%s: dependency on another mod is forbidden (%s)' % (f.name, pat))
        if f.suffix == '.hpp':  # nucleos puros: solo biblioteca estandar y cabeceras locales; ninguna cabecera del juego o del SDK
            for inc in re.findall(r'#\s*include\s*[<"]([^>"]+)[>"]', raw_text):
                if inc not in LOCAL_INCLUDES and inc not in STD_INCLUDES:
                    res['errors'].append('%s: pure core includes a non-core header: %s' % (f.name, inc))
        if f.name == 'balance_adapter.hpp':
            for bad in ('health','hp_max','GetAtAtp','reinterpret_cast','restore_reaction','write_vitality'):
                if bad in text:res['errors'].append('BAL01 adapter cannot use '+bad)
            for required in ('variants.classify(', 'in.epoch_last', 'in.life!=post03::Life::Active', 'a.resource!=static_cast<unsigned>(post03::NameKind::E_OC)'):
                if required not in text:res['errors'].append('BAL01 identity provenance missing '+required)
        if f.name == 'posture_core.hpp':
            for line in text.splitlines():
                if 'uintptr_t' in line and 'actor' not in line:
                    res['errors'].append('posture_core.hpp: the posture sidecar must not store addresses (uintptr_t outside a pass-through parameter)')
        # POST05-GENERAL: los setters inline de bits Tg/Co y HeadLock solo se permiten DENTRO de restore_reaction (se audita aparte)
        scan = re.sub(r'void restore_reaction\(daE_OC_c\* r, const post05::LiveMode& t\) noexcept \{.*?\n\}\n', ' ', fx_strip(text), flags=re.S) if f.name == 'mod.cpp' else text
        for t in FORBIDDEN_CALLS + FORBIDDEN_TOKENS + (['onHeadLockFlg', 'offHeadLockFlg', 'OffCoSetBit'] if f.name == 'mod.cpp' else []):
            if t in scan.replace('DEFINE_HOOK(&daE_OC_c::executeDamage, EocShieldStunPost06);', ''):
                res['errors'].append('%s: forbidden token %s' % (f.name, t))
        for m in ASSIGN.finditer(text):
            res['errors'].append('%s: member assignment through game pointer: %s' % (f.name, m.group(0)[:60]))
        for m in PREINC.finditer(text):
            res['errors'].append('%s: increment through game pointer: %s' % (f.name, m.group(0)[:60]))
        hook_defs += len(re.findall(r'\bDEFINE_HOOK\s*\(', text))
        all_text.append(text)
        if f.name == 'mod.cpp':
            for svc in re.findall(r'IMPORT_SERVICE\s*\(\s*(\w+)', text):
                if svc not in ALLOWED_SERVICES:
                    res['errors'].append('mod.cpp: imports service %s (only Host/Log/Hook are allowed)' % svc)
        if f.name == 'mod.cpp':
            for m in re.finditer(r'return\s+(HOOK_\w+)\s*;', fx_strip(text)):
                if m.group(1) != 'HOOK_CONTINUE':
                    res['errors'].append('mod.cpp: pre-hook returns %s' % m.group(1))
            pres = sorted(re.findall(r'add_pre<(\w+)>', text)); posts = sorted(re.findall(r'add_post<(\w+)>', text))
            if pres != ['AttackObserveFix01', 'CollisionCleanupFix01', 'EocDamageCheckPost05', 'LinkCollisionFx', 'LinkDamageFix01', 'LinkMoveDoFx', 'LinkRStatusFx', 'MeterDeletePost07', 'OutFontDrawFix02', 'PaneDrawFix02', 'PictureDrawFix02', 'PikariDrawFix03', 'PromptDrawLnk01', 'PromptDrawLnk01', 'ScreenDrawHudFix01', 'StringDrawFix02'] or posts != ['AttackObserveFix01', 'CollisionCleanupFix01', 'EocDamageCheckPost05', 'EocShieldStunPost06', 'LinkDamageFix01', 'LinkDamageFix01', 'LinkPolyFix01', 'LinkShieldFx', 'LinkSpActionFx', 'LinkStickFx', 'MeterDrawPost07', 'MeterExecutePost07', 'PaneDrawFix02', 'PikariDrawFix03', 'PromptDrawLnk01', 'PromptDrawLnk01', 'ScreenDrawHudFix01', 'StringDrawFix02']:
                res['errors'].append('mod.cpp: unexpected hook registrations pre=%s post=%s' % (pres, posts))
            for svc in re.findall(r'IMPORT_OPTIONAL_SERVICE\w*\s*\(\s*(\w+)', text):
                if svc not in ALLOWED_OPTIONAL_SERVICES:
                    res['errors'].append('mod.cpp: optional service %s is not allowed' % svc)
            hooks_declared = re.findall(r'DEFINE_HOOK\(&(\w+)::(\w+),\s*(\w+)\)', text)
            if sorted(hooks_declared) != sorted([('J2DPane','draw','PaneDrawFix02'),('COutFont_c','draw','OutFontDrawFix02'),('JUTFont','drawString_size_scale','StringDrawFix02'),('J2DScreen', 'draw', 'ScreenDrawHudFix01'), ('daE_OC_c', 'damage_check', 'EocDamageCheckPost05'), ('daE_OC_c', 'executeDamage', 'EocShieldStunPost06'),
                                                 ('dMeter2_c', '_execute', 'MeterExecutePost07'), ('dMeter2_c', '_draw', 'MeterDrawPost07'), ('dMeter2_c', '_delete', 'MeterDeletePost07'),
                                                 ('dMeterButton_c', 'draw', 'PromptDrawLnk01'), ('daAlink_c','checkDamageAction','LinkDamageFix01'), ('daAlink_c','checkPolyDamage','LinkPolyFix01'), ('daE_OC_c','executeAttack','AttackObserveFix01'), ('daE_OC_c','cc_set','CollisionCleanupFix01'),
                                                 ('daAlink_c','setStickData','LinkStickFx'), ('daAlink_c','setShieldGuard','LinkShieldFx'), ('daAlink_c','spActionTrigger','LinkSpActionFx'),
                                                 ('daAlink_c','checkMoveDoAction','LinkMoveDoFx'), ('daAlink_c','setCollision','LinkCollisionFx')]) \
                    or text.count('DEFINE_HOOK(static_cast<void (daAlink_c::*)(u8)>(&daAlink_c::setRStatus), LinkRStatusFx);') != 1 or text.count('DEFINE_HOOK(static_cast<void (dMeter2Draw_c::*)(f32,f32,f32*,f32,JUtility::TColor,JUtility::TColor,JUtility::TColor,JUtility::TColor,f32,u8,f32)>(&dMeter2Draw_c::drawPikari), PikariDrawFix03);') != 1 or text.count('DEFINE_HOOK(static_cast<void (J2DPicture::*)(f32,f32,f32,f32,bool,bool,bool)>(&J2DPicture::draw), PictureDrawFix02);') != 1:
                res['errors'].append('mod.cpp: unexpected hook targets %s' % hooks_declared)
            # POST08 PARRY: lecturas crudas en post08_judge; UNA escritura (post08_deflect = restore_reaction del PRE con estado 3) llamada una vez desde
            # post08_step; postura solo por preview_parry/apply_parry (una llamada cada una, en post08_step); nada escribe vida, Link ni animacion.
            pj = re.search(r'void post08_judge\(.*?\n\}\n', text, flags=re.S)
            ps = re.search(r'bool post08_step\(.*?\n\}\n', text, flags=re.S)
            if not pj or not ps:
                res['errors'].append('mod.cpp: post08_judge/post08_step not found')
            else:
                for m in re.finditer(r'(ChkAtHit|GetAtHitObj)\s*\(', text):
                    if not text[:m.start()].endswith('cCcD_ObjHitInf::'):
                        res['errors'].append('mod.cpp: %s must be the raw base-class cCcD_ObjHitInf getter' % m.group(1))
                    elif not (pj.start() <= m.start() < pj.end()):
                        res['errors'].append('mod.cpp: At-hit reads only inside post08_judge')
                if len(re.findall(r'ChkAtShieldHit\s*\(', text)) != 1 or 'ChkAtShieldHit' not in pj.group(0):
                    res['errors'].append('mod.cpp: ChkAtShieldHit read exactly once, inside post08_judge')
                for tok in ('GetAtHitAc', 'GetAtHitGObj', 'restore_reaction', 'write_vitality', '.rearm(', '.consume(', 'apply_parry', 'post08_deflect'):
                    if tok in pj.group(0):
                        res['errors'].append('mod.cpp: post08_judge is read-only (found %s)' % tok)
                cd = [m.start() for m in re.finditer(r'\bpost08_deflect\s*\(', text)]
                if len(cd) != 2 or not (ps.start() <= cd[1] < ps.end()):
                    res['errors'].append('mod.cpp: post08_deflect must be defined once and called once, from post08_step')
                for fn in ('apply_parry', 'preview_parry'):
                    cs = [m.start() for m in re.finditer(r'\.' + fn + r'\s*\(', text)]
                    if len(cs) != 1 or not (ps.start() <= cs[0] < ps.end()):
                        res['errors'].append('mod.cpp: %s must be called exactly once, inside post08_step' % fn)
                if 't.state = post08::kDeflectState;' not in ps.group(0) or 'post05::LiveMode t = g.cur.pre_live;' not in ps.group(0):
                    res['errors'].append('mod.cpp: the deflection target must be the PRE live mode with state = post08::kDeflectState')
                if 'if (plan == post08::DeflectPlan::Refuse) stage = "";' not in ps.group(0) or 'a->health != g.cur.pre_hp' not in ps.group(0) \
                        or 'post08::deflect_plan(pre_s, now_s, post08::link_protected_now(in.now))' not in ps.group(0):
                    res['errors'].append('mod.cpp: post08_step must plan the deflection from the reaction shape and Link protection, and check HP before writing')
                # FIX02: el desvio solo escribe RestorePre (PRE exacto) o ForceRecoil (PRE con estado 3); NativeContinues no escribe nada
                if 'if (plan == post08::DeflectPlan::ForceRecoil) t.state = post08::kDeflectState;' not in ps.group(0) or len(re.findall(r't\.state\s*=', ps.group(0))) != 1:
                    res['errors'].append('mod.cpp: the only deflection state change is ForceRecoil -> kDeflectState')
                # FIX02: frescura del registro At: un solo note_post (en do_post, tras todas las escrituras) y el juicio exige g.arm.fresh
                dp_ = re.search(r'void do_post\(daE_OC_c\* a\) noexcept \{.*?\n\}\n', text, flags=re.S)
                np_ = [m.start() for m in re.finditer(r'arm\.note_post\s*\(', text)]
                if not dp_ or len(np_) != 1 or not (dp_.start() <= np_[0] < dp_.end()) or dp_.group(0).find('arm.note_post') < dp_.group(0).find('post06_step(a, r, has_event, rep, parry_crossed)'):
                    res['errors'].append('mod.cpp: g.arm.note_post exactly once, in do_post, after post06_step')
                if 'in.fresh = in.tracked && g.arm.fresh(g.cur.slot, g.cur.id, g.cur.generation, g.p8.tick);' not in pj.group(0):
                    res['errors'].append('mod.cpp: the verdict must require a fresh (armed previous pass) At record')
                # FIX02: el observador de IA es de solo lectura
                for fn in ('post08_watch_out', 'post08_watch_feed'):
                    fb = re.search(r'void ' + fn + r'\([^;{]*\) noexcept \{.*?\n\}\n', text, flags=re.S)
                    if not fb or any(t in fb.group(0) for t in ('restore_reaction', 'write_vitality', 'post08_deflect', 'apply_parry', '.rearm(', 'helm_set_cut')):
                        res['errors'].append('mod.cpp: %s must exist and be read-only' % fn)
                # FIX01: el estado de Link se captura UNA vez por tick, en post08_observe_link (solo lectura), llamado una vez desde do_update; el juicio
                # usa esa instantanea (pass_for) y nunca decide con el estado de Link de la propia llamada.
                ol = re.search(r'void post08_observe_link\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
                if not ol or len(re.findall(r'\bpost08_observe_link\s*\(', text)) != 2 or not re.search(r'void do_update\(\) noexcept \{\n  probe_variant_ai_env\(\);\n  post08_update\(\);\n  post08_observe_link\(\);', text):
                    res['errors'].append('mod.cpp: post08_observe_link must be defined once and called once, from do_update (mod_update)')
                else:
                    if len(re.findall(r'link_track\.observe\s*\(', text)) != 1 or 'link_track.observe(' not in ol.group(0):
                        res['errors'].append('mod.cpp: link_track.observe only inside post08_observe_link')
                    for tok in ('restore_reaction', 'write_vitality', 'helm_set_cut', 'post08_deflect', 'apply_parry', '.rearm('):
                        if tok in ol.group(0):
                            res['errors'].append('mod.cpp: post08_observe_link is read-only (found %s)' % tok)
                if 'in.pass = g.link_track.pass_for(g.p8.tick, in.link_ok, link_id);' not in pj.group(0) or re.search(r'in\.pass\s*=\s*in\.now', text):
                    res['errors'].append('mod.cpp: the verdict must use the per-tick pass snapshot (pass_for), never the current Link state')
                if len(re.findall(r'\.parry\.enabled\s*=(?!=)', text)) != 1 or 'g.parry.enabled = true;' not in text:
                    res['errors'].append('mod.cpp: g.parry.enabled must be set exactly once, to true')
            # POST07 HUD: DISPLAY ONLY. The HUD block (struct HudGlue .. emit_hud_summary) never writes game/posture/POST05/POST06 state.
            hb = re.search(r'struct HudGlue \{.*?void emit_hud_summary\(', text, flags=re.S)
            if not hb:
                res['errors'].append('mod.cpp: POST07 HUD block not found')
            else:
                h = hb.group(0)
                for tok in ('restore_reaction', 'write_vitality', 'helm_set_cut', 'helm_guard_duplicate', '.rearm(', 'set_regen_rate', '.consume(', '.step(', '.commit(',
                            'g.rx', 'on_pre(', 'on_post(', 'on_update(', 'setActionMode', 'fopAcM_SearchByID', 'get_service', 'mAtInfo', 'mCutType', 'field_0x6cc'):
                    if tok in h:
                        res['errors'].append('mod.cpp: the HUD block may not use %s (display only)' % tok)
                for m in re.finditer(r'\bg\.(pe|br|eng)\b[\w\.]*', h):
                    if m.group(0) not in ('g.pe.px.entry', 'g.br.slot', 'g.eng.obs.ledger.slot', 'g.eng.obs.disabled'):
                        res['errors'].append('mod.cpp: the HUD block may only read g.pe.px.entry/g.br.slot/g.eng.obs.ledger.slot/g.eng.obs.disabled, not %s' % m.group(0))
                if re.search(r'->\s*[\w\.\[\]]+\s*(=(?!=)|\+=|-=|\|=|&=|\^=|<<=|>>=|\+\+|--)', h) or re.search(r'(\+\+|--)\s*\w+\s*->', h):
                    res['errors'].append('mod.cpp: the HUD block writes through a pointer')
                for m in re.finditer(r'(?<![\w.])(?:const\s+)?(?:fopAc_ac_c|daE_OC_c)\s*\*\s*(\w+)', h):
                    if not m.group(0).startswith('const') and m.group(1) not in ('act', 'player'):
                        res['errors'].append('mod.cpp: unexpected actor pointer %s in the HUD block' % m.group(1))
            # one read-only walk of the live actor queue, inside hud_sim_pass; one queue of OUR packet; scrub only in hud_drop_all
            sp = re.search(r'void hud_sim_pass\(dMeter2_c\* meter\) noexcept \{.*?\n\}\n', text, flags=re.S)
            dp = re.search(r'void hud_drop_all\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            pp = re.search(r'void hud_packet_paint\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            qs = [m.start() for m in re.finditer(r'g_fopAcTg_Queue', text)]
            if not sp or len(qs) != 1 or not (sp.start() <= qs[0] < sp.end()):
                res['errors'].append('mod.cpp: g_fopAcTg_Queue may be walked exactly once, inside hud_sim_pass')
            qa = [m.start() for m in re.finditer(r'queue_after_hud\s*\(', text)]
            if not sp or len(qa) != 1 or not (sp.start() <= qa[0] < sp.end()) or 'set2DXlu' in text or re.search(r'\.set\s*\(', text):
                res['errors'].append('mod.cpp: the packet is queued exactly once per pass (queue_after_hud in hud_sim_pass); no direct set()/set2DXlu')
            sc = [m.start() for m in re.finditer(r'scrub_all\s*\(', text)]
            if not dp or len(sc) != 1 or not (dp.start() <= sc[0] < dp.end()):
                res['errors'].append('mod.cpp: scrub_all only inside hud_drop_all')
            if len(re.findall(r'\bhud_drop_all\s*\(', text)) != 4:
                res['errors'].append('mod.cpp: hud_drop_all must be defined once and called from the draw-pass failure path, the meter delete pre-hook and mod_shutdown')
            if not pp:
                res['errors'].append('mod.cpp: hud_packet_paint not found')
            else:
                for tok in ('fopAc_ac_c', 'daE_OC_c', 'g_fopAcTg_Queue', 'g.eng', 'g.pe', 'g.br', 'samples', 'act', 'anchors.update', 'publish('):
                    if re.search(r'(?<![\w.])' + re.escape(tok) + r'(?![\w])', pp.group(0)):
                        res['errors'].append('mod.cpp: the painter callback may only read hg.frame value copies (found %s)' % tok)
            # La unica consulta de existencia: una llamada a fopAcM_SearchByID(, dentro de lookup_actor, invocada solo desde do_update (mod_update).
            calls = [m.start() for m in re.finditer(r'fopAcM_SearchByID\s*\(', text)]
            body = re.search(r'Lookup lookup_actor\(.*?\n}\n', text, flags=re.S)
            if len(calls) != 1 or not body or not (body.start() <= calls[0] < body.end()):
                res['errors'].append('mod.cpp: fopAcM_SearchByID must be called exactly once, inside lookup_actor')
            if len(re.findall(r'\blookup_actor\s*\(', text)) != 2:
                res['errors'].append('mod.cpp: lookup_actor must be defined once and called once (from do_update)')
            if len(re.findall(r'\bdo_update\s*\(', text)) != 2:
                res['errors'].append('mod.cpp: do_update must be defined once and called once (from mod_update)')
            # POST03: lecturas de collider
            if len(re.findall(r'mAtInfo\s*\.\s*mpCollider', text)) != 1 or len(re.findall(r'mAtInfo', text)) != 1:
                res['errors'].append('mod.cpp: mAtInfo may only be touched once, as mAtInfo.mpCollider (pointer value, in do_post)')
            for m in re.finditer(r'ChkTgHit', text):
                if not text[:m.start()].endswith('cCcD_ObjHitInf::'):
                    res['errors'].append('mod.cpp: ChkTgHit must be the raw base-class cCcD_ObjHitInf::ChkTgHit')
            # POST05: la UNICA escritura al juego. Toda asignacion a traves de '->' en mod.cpp debe estar en restore_reaction y ser una de las tres.
            # POST05-GENERAL: el escritor asigna SOLO los 5 campos de setActionMode (modo, estado, modo previo, timers) a partir de t, y llama SOLO a los
            # setters inline que setActionMode/offTgSph usan (On/OffTgSetBit, On/OffCoSetBit de mSphs_cc[0|1], on/offHeadLockFlg) sobre r.
            rr = re.search(r'void restore_reaction\(daE_OC_c\* r, const post05::LiveMode& t\) noexcept \{(.*?)\n\}\n', text, flags=re.S)
            writes = list(re.finditer(r'(\w+)\s*->\s*([\w\.\[\]]+)\s*(=(?!=)|\+=|-=|\|=|&=|\^=|<<=|>>=|\+\+|--)', text))
            allowed = {'mActionMode': 't.mode', 'mOcState': 't.state', 'mOldActionMode': 't.old_mode', 'field_0x6c0': 't.t6c0', 'field_0x6c2': 't.t6c2'}
            inside = [w for w in writes if rr and rr.start(1) <= w.start() < rr.end(1)]
            if not rr or len(inside) != 5 or {w.group(2) for w in inside} != set(allowed) or any(w.group(1) != 'r' or w.group(3) != '=' for w in inside):
                res['errors'].append('mod.cpp: restore_reaction must assign exactly mActionMode, mOcState, mOldActionMode, field_0x6c0, field_0x6c2 through r')
            elif any(not re.search(r'r->' + k + r'\s*=\s*' + re.escape(v) + r'\s*;', rr.group(1)) for k, v in allowed.items()):
                res['errors'].append('mod.cpp: restore_reaction fields must come from the plan target t')
            if rr:
                calls_rr = re.findall(r'r->([\w\[\]\.]+)\(\)', rr.group(1))
                ok_calls = {'onHeadLockFlg', 'offHeadLockFlg'} | {'mSphs_cc[%d].%s' % (i, s) for i in (0, 1) for s in ('OnTgSetBit', 'OffTgSetBit', 'OnCoSetBit', 'OffCoSetBit')}
                if sorted(calls_rr) != sorted(['onHeadLockFlg', 'offHeadLockFlg'] + ['mSphs_cc[%d].%s' % (i, s) for i in (0, 1) for s in ('OnTgSetBit', 'OffTgSetBit', 'OnCoSetBit', 'OffCoSetBit')]) or not set(calls_rr) <= ok_calls:
                    res['errors'].append('mod.cpp: restore_reaction may only call the inline Tg/Co set-bit and HeadLock setters, once each branch')
                if re.search(r'\b(health|field_0x6cc|mSphs_at|mpMorf|speedF|mAtInfo|ClrTgHit|setActionMode)\b', rr.group(1)):
                    res['errors'].append('mod.cpp: restore_reaction touches a field outside the reaction write set')
            if len(re.findall(r'\.stagger_policy\s*=(?!=)', text)) != 1 or len(re.findall(r'\bg\.rx\.stagger_policy\s*=\s*post05::StaggerPolicy::General\s*;', text)) != 1:
                res['errors'].append('mod.cpp: g.rx.stagger_policy must be set exactly once, to post05::StaggerPolicy::General')
            # POST06: segundo escritor ACOTADO: write_vitality asigna SOLO r->health = hp y (rama letal) r->attention_info.flags = 0.
            wv = re.search(r'void write_vitality\(daE_OC_c\* r, int hp, bool lethal\) noexcept \{(.*?)\n\}\n', text, flags=re.S)
            inside_wv = [w for w in writes if wv and wv.start(1) <= w.start() < wv.end(1)]
            if not wv or sorted(w.group(2) for w in inside_wv) != ['attention_info.flags', 'health'] or any(w.group(1) != 'r' or w.group(3) != '=' for w in inside_wv) \
                    or 'r->attention_info.flags = 0;' not in wv.group(1) or 'r->health = static_cast<s16>(hp);' not in wv.group(1):
                res['errors'].append('mod.cpp: write_vitality must assign exactly r->health = hp and r->attention_info.flags = 0')
            extra=[]
            # HUD FIX01: exact render-lease bodies only; all original writer checks remain.
            import hashlib
            def hfcanonical(v):
                v=re.sub(r'/\*.*?\*/',' ',v,flags=re.S);v=re.sub(r'//[^\n]*',' ',v)
                return re.sub(r'\s+','',v)
            hfpins = {'hudfix_restore': '4a9de52245362426408903c0091417d081f63b54ddc5993e309a975255a12516', 'hudfix_root': '013ad811ab1b630644df09bdf939b9ecd85c55a847e1e77a55a7f3fb45aef89a', 'on_screen_pre_hudfix': '922682e5fc30e4e230d012f7fec616d7071bec7a039060dcf5117b855e75659f'}
            for fn,want in hfpins.items():
                body=re.search(r'^(?:void|hudfix01::Matrix|HookAction) '+re.escape(fn)+r'\([^;]*?\) noexcept \{.*?^\}',text,flags=re.M|re.S)
                if not body or hashlib.sha256(hfcanonical(body.group(0)).encode()).hexdigest()!=want:
                    res['errors'].append('HUD FIX01 exact render lease changed: '+fn)
                elif body:
                    extra += [w for w in writes if body.start() <= w.start() < body.end()]
            if 'lg.ui.enabled = false;' not in text or any(x.strip()!='false' for x in re.findall(r'lg\.ui\.enabled\s*=([^;]+);',text)):
                res['errors'].append('HUD FIX01 legacy root hiding must remain disabled')
            # LNK01: escritores VISUALES (HUD_VISUAL_ONLY) exactos: hide() de la raiz (una llamada) y los dos slots de brillo del prompt (dos asignaciones).
            ah = re.search(r'void action_ui_hide\(J2DScreen\* r\) noexcept \{ r->hide\(\); \}', text)
            asl = re.search(r'void action_ui_slots\(dMeterButton_c\* r, std::uint8_t s0, std::uint8_t s1\) noexcept \{\n  r->field_0x4be\[0\] = s0;\n  r->field_0x4be\[1\] = s1;\n\}\n', text)
            if not ah or not asl:
                res['errors'].append('mod.cpp: LNK01 visual writers must be exactly action_ui_hide (r->hide()) and action_ui_slots (field_0x4be[0..1])')
            else:
                extra += [w for w in writes if asl.start() <= w.start() < asl.end()]
            if len(re.findall(r'hide\s*\(\s*\)', text)) != 1 or len(re.findall(r'\baction_ui_hide\s*\(', text)) != 2 or len(re.findall(r'\baction_ui_slots\s*\(', text)) != 3:
                res['errors'].append('mod.cpp: hide() exactly once (action_ui_hide, called once from action_ui_step); action_ui_slots defined once and called from the prompt draw PRE and POST')
            for t_ in LNK01_FORBIDDEN:
                if t_ in fx_strip(text):
                    res['errors'].append('mod.cpp: LNK01 forbidden token %s (focal visual suppression only; native guard counter untouched)' % t_)
            stp = re.search(r'void action_ui_step\(dMeter2_c\* meter, unsigned site\) noexcept \{.*?\n\}\n', text, flags=re.S)
            if not stp or 'action_ui_hide(s)' not in stp.group(0) or 'if (d.hide && s)' not in stp.group(0):
                res['errors'].append('mod.cpp: the root is hidden only from action_ui_step, only on a validated decision')
            rd = re.search(r'lnk01::UiInput action_ui_read\(.*?\n\}\n', text, flags=re.S)
            if not rd or 'in.vptr_ok = vs != nullptr && vs == *reinterpret_cast<void* const*>(ref) && vs == *reinterpret_cast<void* const*>(waku);' not in rd.group(0) \
                    or 'if (!in.vptr_ok) return in;' not in rd.group(0) or re.search(r'->\s*[\w\.\[\]]+\s*(=(?!=))', rd.group(0)):
                res['errors'].append('mod.cpp: action_ui_read must fail closed on the J2DScreen vptr check and be read-only')
            pre_ = re.search(r'HookAction on_prompt_draw_pre\(.*?\n\}\n', text, flags=re.S)
            if not pre_ or 'meter->mpEmpButton == self' not in pre_.group(0) or 'lg.ui.layout_ok_for(key)' not in pre_.group(0):
                res['errors'].append('mod.cpp: the sparkle mask applies only to the LIVE validated prompt (mpEmpButton == self)')
            ins = re.search(r'hq = mods::hook::add_post<PromptDrawLnk01>\(on_prompt_draw_post\);\n    hp = hq == MOD_OK \? mods::hook::add_pre<PromptDrawLnk01>\(on_prompt_draw_pre\) : MOD_ERROR;', text)
            if not ins:
                res['errors'].append('mod.cpp: the prompt draw POST must be installed before (and as a precondition of) its PRE')
            # LNK01: postura de Link = contador PRIVADO alimentado solo por el veredicto POST08 (una vez, en do_post tras post08_step) y por un tick en mod_update
            lc = re.search(r'void lnk01_contact\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            lu = re.search(r'void lnk01_update\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            dp2 = re.search(r'void do_post\(daE_OC_c\* a\) noexcept \{.*?\n\}\n', text, flags=re.S)
            du2 = re.search(r'void do_update\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            if not lc or not lu or not dp2 or not du2:
                res['errors'].append('mod.cpp: lnk01_contact/lnk01_update/do_post/do_update not found')
            else:
                for fb_, nm in ((lc, 'lnk01_contact'), (lu, 'lnk01_update')):
                    if any(t_ in fb_.group(0) for t_ in ('restore_reaction', 'write_vitality', 'helm_set_cut', 'helm_guard_duplicate', 'post08_deflect', 'apply_parry',
                                                         '.rearm(', '.commit(', 'action_ui_', 'judge(')) or re.search(r'->\s*[\w\.\[\]]+\s*(=(?!=)|\+=|-=|\+\+|--)', fb_.group(0)):
                        res['errors'].append('mod.cpp: %s may only update the private Link posture counter' % nm)
                if len(re.findall(r'\.on_contact\s*\(', text)) != 2 or '.on_contact(' not in lc.group(0) or len(re.findall(r'\blnk01_contact\s*\(', text)) != 2 \
                        or 'lnk01_contact();' not in dp2.group(0) or dp2.group(0).find('lnk01_contact();') < dp2.group(0).find('post08_step(a, r)'):
                    res['errors'].append('mod.cpp: Link posture contacts only from the POST08 verdict, once, in do_post after post08_step')
                if 'k.result = v.result;' not in lc.group(0) or 'k.blocked = v.blocked;' not in lc.group(0) or 'k.token = v.token;' not in lc.group(0) or 'e.parried' not in lc.group(0):
                    res['errors'].append('mod.cpp: the Link contact must carry the POST08 verdict, native block flag, attack token and the parry episode')
                if len(re.findall(r'\.posture\.tick\s*\(', text)) != 1 or 'lg.posture.tick(lg.sim_active, pm, ob.guard_safe)' not in lu.group(0) or len(re.findall(r'\blnk01_update\s*\(', text)) != 2 \
                        or 'lnk01_update();' not in du2.group(0):
                    res['errors'].append('mod.cpp: Link posture regeneration ticks exactly once per mod_update (lnk01_update from do_update)')
                if not re.search(r'lg\.sim_active = l\.valid && life > 0 && !g_dComIfG_gameInfo\.play\.isPauseFlag\(\) && !g_dComIfG_gameInfo\.play\.getEvent\(\)->runCheck\(\) &&\s*dMeter2Info_getWindowStatus\(\) == 0;', lu.group(0)):
                    res['errors'].append('mod.cpp: regeneration must freeze without Link, at 0 life, in pause, in events and with a menu window open')
            aw=re.search(r'void fix01_at_mask\(daE_OC_c\* r,unsigned mask,bool on\) noexcept \{.*?\n\}\n',text,re.S)
            expected_at='void fix01_at_mask(daE_OC_c* r,unsigned mask,bool on) noexcept {\n if(mask&1){if(on)r->mSphs_at[0].OnAtSetBit();else r->mSphs_at[0].OffAtSetBit();}\n if(mask&2){if(on)r->mSphs_at[1].OnAtSetBit();else r->mSphs_at[1].OffAtSetBit();}\n}\n'
            if not aw or aw.group(0)!=expected_at:res['errors'].append('FIX01 own At writer differs from exact bounded bit lease')
            outside=text[:aw.start()]+text[aw.end():] if aw else text
            if re.search(r'(?:OnAtSetBit|OffAtSetBit|ClrAtHit)\s*\(',outside):res['errors'].append('FIX01 At mutator outside audited helper')
            for fn,call in [('on_cc_pre_fix01','fix01_at_mask(a,fx.cc_added,true);'),('on_cc_post_fix01','fix01_at_mask(a,fx.cc_added,false);')]:
                fb=re.search(r'(?:HookAction|void) '+fn+r'\(.*?\n\}\n',text,re.S)
                if not fb or call not in fb.group(0):res['errors'].append('FIX01 missing paired At registration hook '+fn)
            if len(re.findall(r'\bfix01_at_mask\(',text))!=3 or 'mask,p.fresh&&p.tracked&&p.hp>1&&!p.event_running,p.mode==4&&(p.state==1||p.state==2)' not in text:res['errors'].append('FIX01 cleanup must require fresh own Link contact and actual intervention')
            for required in ('postbal01::capture(g.eng.obs.ledger,lg.posture.variants,g.cur.slot,g.cur.id,g.cur.generation,r.aux)', '},ev,rep,g.cur.balance_profile);', 'const postbal01::Snapshot balance_profile=g.cur.balance_profile;', 'g.eng.obs.ledger, balance_profile);', 'g.eng.obs.ledger, balance_profile, &pv);'):
                if required not in text:res['errors'].append('BAL01 production snapshot binding missing '+required)
            # FIX01: native Link capture and real native attack observation are read-only.
            for fn in ('on_link_damage_pre_fix01','on_link_damage_post_fix01','on_link_poly_post_fix01','on_attack_pre_fix01','on_attack_post_fix01','fix01_ai_feed','fix01_flush_direct'):
                fb=re.search(r'(?:HookAction|void) '+fn+r'\(.*?\n\}\n',text,re.S)
                if not fb or any(t in fb.group(0) for t in ('restore_reaction(', 'write_vitality(', 'helm_set_cut(', 'post08_deflect(', '.rearm(', 'ClrTgHit(', 'ResetTgHit(', 'setPlaySpeed(', 'setDamagePoint(')):
                    res['errors'].append('FIX01 read-only path missing/unsafe: '+fn)
            fl=re.search(r'void fix01_flush_direct\(.*?\n\}\n',text,re.S)
            np=re.search(r'HookAction on_link_damage_pre_fix01\(.*?\n\}\n',text,re.S)
            if not fl or 'const auto o=lg.posture.on_contact(z.k);' not in fl.group(0) or 'fx.direct.take(i,g.p8.tick,identity,ob.id,ob.active&&ob.human&&ob.life>0)' not in fl.group(0):
                res['errors'].append('FIX01 deferred contact must use bounded native join and identity gate')
            if not np or 'break;' not in np.group(0) or 'n.blocked=link->mTgCyls[k].ChkTgShieldHit()!=0;' not in np.group(0) or 'if(link->field_0x2fb8==0)' not in np.group(0):
                res['errors'].append('FIX01 native capture must preserve first-cylinder priority and native eligibility')
            if 'ap, g.cur.continuation==lnks1fix::Continuation::Safe);' not in text or 'g.cur.continuation=lnks1fix::continuation(' not in text:
                res['errors'].append('FIX01 continuation proof must gate the actual POST05 process')
            if 'n.collider=reinterpret_cast<std::uintptr_t>(link->mTgCyls[k].cCcD_ObjHitInf::GetTgHitObj());' not in text or 'n.blocked=link->mTgCyls[k].ChkTgShieldHit()!=0;break;' not in text:
                res['errors'].append('FIX01 cannot fabricate native source or change its priority')
            if 'const bool actual=a->mSphs_at[0].ChkAtSet()||a->mSphs_at[1].ChkAtSet();' not in text or 'fx.native.poly_seen=true;fx.native.poly_result=*static_cast<const int*>(ret);' not in text:
                res['errors'].append('FIX01 must observe real native arm bits and environmental return value')
            if 'fx.ai.native_attack(g.cur.slot,g.cur.id,g.cur.generation,actual,completed);' not in text or 'const bool completed=terminal&&' not in text:
                res['errors'].append('FIX01 recovery requires observed native terminal exit')
            if 'fx.direct.offer(k,' not in text or 'fix01_flush_direct(ob);' not in text or 'k.tg_match = g.cur.lnk_tg_match;' not in text:
                res['errors'].append('FIX01 contact join missing or legacy Tg attribution fabricated')
            # LNK SESSION01: confirmacion de golpes directos UNA vez por mod_update, despues de observe_link; lectura PRE de atribucion de solo lectura
            if lc and lu:
                if len(re.findall(r'\.confirm_direct\s*\(', text)) != 1 or 'lg.posture.confirm_direct(ob, g.p8.tick, de);' not in lu.group(0) \
                        or lu.group(0).find('lg.posture.confirm_direct(') < lu.group(0).find('lg.posture.observe_link(') \
                        or lu.group(0).find('lg.posture.confirm_direct(') > lu.group(0).find('lg.posture.tick('):
                    res['errors'].append('mod.cpp: confirm_direct exactly once per mod_update, after observe_link and before the regeneration tick')
                if 'k.tg_match = g.cur.lnk_tg_match;' not in lc.group(0):
                    res['errors'].append('mod.cpp: a direct hit must carry the Link Tg attribution read in the PRE')
                ob_ = re.search(r'ob\.guard_safe = lg\.sim_active && ob\.human && l\.upper_guard && l\.damage_timer == 0 && l\.proc != daAlink_c::PROC_GUARD_SLIP &&\s*'
                                r'l\.proc != daAlink_c::PROC_GUARD_ATTACK && l\.proc != daAlink_c::PROC_GUARD_BREAK && l\.proc != daAlink_c::PROC_CUT_REVERSE;', lu.group(0))
                if not ob_:
                    res['errors'].append('mod.cpp: safe guard = active human upper-guard without GUARD_SLIP/ATTACK/BREAK/CUT_REVERSE and without invulnerability')
            lp_ = re.search(r'void lnk_pre\(daE_OC_c\* a, const PreReads& r, daPy_py_c\* lp\) noexcept \{.*?\n\}\n', text, flags=re.S)
            if not lp_ or len(re.findall(r'\blnk_pre\s*\(', text)) != 2 or 'lnk_pre(a,r,lp);' not in text \
                    or re.search(r'->\s*[\w\.\[\]]+\s*(=(?!=)|\+=|-=|\+\+|--)', lp_.group(0)) \
                    or any(t_ in lp_.group(0) for t_ in ('restore_reaction', 'write_vitality', 'helm_set_cut', 'post08_deflect', 'apply_parry', '.on_contact(', '.rearm(')):
                res['errors'].append('mod.cpp: lnk_pre must exist, be read-only and be called once (from do_pre)')
            elif 'break;' not in lp_.group(0) or 'cCcD_ObjHitInf::ChkTgHit()' not in lp_.group(0) or 'cCcD_ObjHitInf::GetTgHitObj()' not in lp_.group(0):
                res['errors'].append('mod.cpp: the attribution must use the FIRST Link Tg cylinder with a raw hit (checkDamageAction order)')
            pp2 = re.search(r'void hud_packet_paint\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            if pp2 and any(t_ in pp2.group(0) for t_ in ('lg.posture', 'lg.hud.', 'lg.ui', 'action_ui_', 'lnk01_link_now')):
                res['errors'].append('mod.cpp: the painter draws the Link bar only from the value copy hg.player')
            if len(re.findall(r'\blg\.hud\.tick\s*\(', text)) != 1:
                res['errors'].append('mod.cpp: the Link bar animates exactly once per simulation pass (hud_sim_pass)')
            for fn,field in [('helm_set_cut','mCutType'),('helm_guard_duplicate','field_0x6cc')]:
                fbody=re.search(r'void '+fn+r'\(.*?\n\}\n',text,flags=re.S)
                found=[w for w in writes if fbody and fbody.start()<=w.start()<fbody.end()]
                if len(found)!=1 or found[0].group(1)!='r' or found[0].group(2)!=field or found[0].group(3)!='=':
                    res['errors'].append('invalid audited Helm writer '+fn)
                extra+=found
            if 'if(cut==5||cut==8) r->mCutType=cut;' not in text or 'r->field_0x6cc=1;' not in text:
                res['errors'].append('Helm writers must keep cut5/8 and cooldown1 bounds')
            stk = fx_body(text, 'void on_stick_post_fx(')
            if stk and stk.group(0).count(FX_B_CLEAR) == 1 and 'if (lnkfx::consume_b(lf.b_trig, lf.r_held, lf.def.phase(), lf.bash_offered_prev)) {\n      ' + FX_B_CLEAR in stk.group(0):
                extra += [w for w in writes if stk.start() <= w.start() < stk.end() and 'mItemTrigger' in w.group(0)]
            spans02=fix02_spans(text)
            if len(spans02)!=len(FIX02_PINS):res['errors'].append('HUD FIX02 exact visual callback changed')
            for w in writes:
                if any(a<=w.start()<b for a,b in spans02):extra.append(w)
            for w in writes:
                if w not in inside and w not in inside_wv and w not in extra:
                    res['errors'].append('mod.cpp: write through a pointer outside restore_reaction/write_vitality: %s' % w.group(0)[:60])
            calls_r = [m.start() for m in re.finditer(r'\brestore_reaction\s*\(', text)]
            hosts = [re.search(r'void restore_reaction\(daE_OC_c\* r, const post05::LiveMode& t\) noexcept \{', text)] + \
                    [re.search(p_, text, flags=re.S) for p_ in (r'void do_post\(daE_OC_c\* a\) noexcept \{.*?\n\}\n', r'bool post06_bonus\(.*?\n\}\n', r'bool post06_stun\(.*?\n\}\n',r'bool post06_recover\(.*?\n\}\n',r'void on_stun_post\(.*?\n\}\n',r'void post08_deflect\(daE_OC_c\* a, const post05::LiveMode& t\) noexcept \{ restore_reaction\(a, t\); \}')]
            if len(calls_r) != 8 or any(h is None for h in hosts) or not all(any(h.start() <= c < h.end() for h in hosts) for c in calls_r):
                res['errors'].append('mod.cpp: restore_reaction must be defined once and called only from do_post (POST05), post06_bonus and post06_drop')
            if len(re.findall(r'\bwrite_vitality\s*\(', text)) != 3:
                res['errors'].append('mod.cpp: write_vitality must be defined once and called once (post06_bonus)')
            if len(re.findall(r'\.br\.enabled\s*=(?!=)', text)) != 1 or len(re.findall(r'\bg\.br\.enabled\s*=\s*true\s*;', text)) != 1:
                res['errors'].append('mod.cpp: g.br.enabled must be set exactly once, to true')
            # POST05-COMPAT: el mod usa la admision V2 (y solo la fija una vez); toda llamada a admit() del mod usa la politica del motor
            if len(re.findall(r'\.policy\s*=(?!=)', text)) != 1 or len(re.findall(r'\bg\.eng\.policy\s*=\s*AdmissionPolicy::V2Compat\s*;', text)) != 1:
                res['errors'].append('mod.cpp: g.eng.policy must be set exactly once, to AdmissionPolicy::V2Compat')
            adm_calls = re.findall(r'\badmit\s*\(([^)]*)\)', text)
            if not adm_calls or any(c.replace(' ', '') != 'r.facts,g.eng.policy' for c in adm_calls):
                res['errors'].append('mod.cpp: every admit() call must pass the engine policy (admit(r.facts, g.eng.policy))')
            # POST05-RB: la politica E_OC2 se fija UNA vez, a MatureBC; mpMorf/speedF solo se leen en read_aux (y la direccion de mFrameCtrl en layout)
            if len(re.findall(r'\.e2_policy\s*=(?!=)', text)) != 1 or len(re.findall(r'\bg\.rx\.e2_policy\s*=\s*post05::E2Policy::MatureBC\s*;', text)) != 1:
                res['errors'].append('mod.cpp: g.rx.e2_policy must be set exactly once, to post05::E2Policy::MatureBC')
            ra_ = re.search(r'NativeAux read_aux\(const daE_OC_c\* a\) noexcept \{.*?\n\}\n', text, flags=re.S)
            lm_ = re.search(r'bool layout_matches_binary\(daE_OC_c\* a\) noexcept \{.*?\n\}\n', text, flags=re.S)
            for tok in ('mpMorf', 'speedF', 'mFrameCtrl'):
                for m in re.finditer(r'\b' + tok + r'\b', text):
                    if not ((ra_ and ra_.start() <= m.start() < ra_.end()) or (tok != 'speedF' and lm_ and lm_.start() <= m.start() < lm_.end())):
                        res['errors'].append('mod.cpp: %s may only be read inside read_aux (or addressed in layout_matches_binary)' % tok)
            if re.search(r'\bfc\s*\.\s*\w+\s*(=(?!=)|\+=|-=)', text) or re.search(r'const\s+J3DFrameCtrl\s*&\s*fc', text) is None:
                res['errors'].append('mod.cpp: the frame controller must be bound as const J3DFrameCtrl& fc and never written')
            # POST05-FINALIZE: UNA consulta de servicio ajeno, solo de existencia, solo en probe_variant_ai_env (llamada una vez, desde do_update);
            # el resultado solo se compara con nulo; e2_env solo se asigna ahi. Contrato nuevo y acotado (ver FINALIZE/).
            pe_ = re.search(r'void probe_variant_ai_env\(\) noexcept \{.*?\n\}\n', text, flags=re.S)
            calls_ = [m.start() for m in re.finditer(r'get_service\b', text)]
            if not pe_ or len(calls_) != 2 or any(not (pe_.start() <= c < pe_.end()) for c in calls_):
                res['errors'].append('mod.cpp: get_service may only appear inside probe_variant_ai_env (one guarded call)')
            if pe_ and (len(re.findall(r'\bfound\b', pe_.group(0))) != 3 or '&found' not in pe_.group(0).replace(' ', '') or 'found==nullptr' not in pe_.group(0).replace(' ', '')):
                res['errors'].append('mod.cpp: the service pointer returned by get_service may only be compared with nullptr')
            if raw_text.count('"dev.devf.encounter_placement.v11"') != 1 or not re.search(r'get_service\(mod_ctx, kVariantAiRequiredService, 1, 0, &found\)', text):
                res['errors'].append('mod.cpp: get_service must query exactly kVariantAiRequiredService ("dev.devf.encounter_placement.v11", major 1)')
            for m in re.finditer(r'\.e2_env\s*=(?!=)', text):
                if not (pe_ and pe_.start() <= m.start() < pe_.end()):
                    res['errors'].append('mod.cpp: e2_env may only be assigned inside probe_variant_ai_env')
            if len(re.findall(r'\bprobe_variant_ai_env\s*\(', text)) != 2 or not re.search(r'void do_update\(\) noexcept \{\n  probe_variant_ai_env\(\);', text):
                res['errors'].append('mod.cpp: probe_variant_ai_env must be defined once and called once, first thing in do_update (mod_update only)')
            for m in re.finditer(r'Chk(?:Tg|Co)Set', text):
                if not text[:m.start()].endswith('cCcD_ObjHitInf::'):
                    res['errors'].append('mod.cpp: ChkTgSet/ChkCoSet must be the raw base-class const getters')
            lm = re.search(r'bool layout_matches_binary\(daE_OC_c\* a\) noexcept \{.*?\n\}\n', text, flags=re.S)
            gets = [m.start() for m in re.finditer(r'GetObj(?:Tg|Co)\s*\(', text)]
            if len(gets) != 4 or not lm or any(not (lm.start() <= g < lm.end()) for g in gets):
                res['errors'].append('mod.cpp: GetObjTg/GetObjCo may only be used (address only) inside layout_matches_binary')
            for m in re.finditer(r'GetTgHitObj', text):
                if not text[:m.start()].endswith('cCcD_ObjHitInf::'):
                    res['errors'].append('mod.cpp: GetTgHitObj must be the raw base-class cCcD_ObjHitInf::GetTgHitObj')
            if len(re.findall(r'GetAtType\s*\(', text)) != 1:
                res['errors'].append('mod.cpp: GetAtType() must be called exactly once')
            rt = re.search(r'bool read_at_type\(.*?\n}\n', text, flags=re.S)
            if not rt or 'GetAtType' not in rt.group(0) or len(re.findall(r'reinterpret_cast\s*<\s*const\s+cCcD_Obj\s*\*\s*>', text)) != 1:
                res['errors'].append('mod.cpp: the only collider dereference must be inside read_at_type')
            for cb in ('do_pre', 'do_post'):
                m = re.search(r'void ' + cb + r'\(.*?\n}\n', text, flags=re.S)
                if not m or 'lookup_actor' in m.group(0) or 'SearchByID' in m.group(0):
                    res['errors'].append('mod.cpp: lookups are forbidden inside ' + cb)
    blob = '\n'.join(all_text)
    # POST05-RB: E_OC2 solo llega a ELIGIBLE con la politica MatureBC, senales coherentes y el gate de fase, que va tras las reglas comunes y antes de seguridad
    rc = [t for t in all_text if 'Reason classify(' in t]
    if len(rc) != 1:
        res['errors'].append('reaction_core.hpp: classify() not found exactly once')
    else:
        body = re.search(r'Reason classify\(.*?\n  \}\n', rc[0], flags=re.S)
        b = body.group(0) if body else ''
        defer = b.find('if (e2 && !(e2_policy == E2Policy::MatureBC && e2_consistent(in))) return Reason::E_OC2SuppressionDeferred;')
        elig = b.find('return Reason::Eligible;')
        first_policy = b.find('return Reason::FamilyNotSword;')
        last_common = b.find('return Reason::PostStateNotOrdinary;')
        phase = b.find('if (!e2_allowed(phase, e2_env, r.aux_pre.atp, r.aux_pre.anim_rate, compat)) {')
        safety = b.find('if (const Reason s = common_safety(in, e2); s != Reason::Count) return s;')
        cand = safety + 1 if safety >= 0 and 'in.adm != (e2 ? Admission::E_OC2_CANDIDATE : Admission::E_OC_CANDIDATE)' in blob else -1
        anim = b.find('if (e2 && !same_anim(r.aux_pre, r.aux_post)) return Reason::PostAuxUnexpected;')
        power = b.find('return Reason::PowerChangedInCall;')
        if not body or min(defer, elig, first_policy, last_common, phase, safety, cand, anim, power) < 0 or \
                not (defer < first_policy < last_common < phase < safety < cand < anim < power < elig) or b.count('return Reason::Eligible;') != 1 or \
                'const bool e2 = any_e_oc2(in);' not in b:
            res['errors'].append('reaction_core.hpp: classify() order must be: E_OC2 deferral (unless MatureBC+consistent) < common policy < E_OC2 phase gate < '
                                 'safety (candidate per resource, same animation, power) < Eligible once, last')
    cs = re.search(r'Reason common_safety\(const DecisionInput& in, bool e2\) const noexcept \{.*?\n  \}\n', blob, flags=re.S)
    if not cs or [cs.group(0).find(x) >= 0 for x in ('return Reason::SessionDisabled;', 'return Reason::TokenMismatch;', 'return Reason::NotCandidateActive;',
            'return Reason::AdmissionEverUnstable;', 'return Reason::FrozenOrDead;', 'return Reason::PostSignatureMismatch;', 'return Reason::PreNotFresh;',
            'return Reason::ColliderNotFresh;', 'return Reason::AuxMissing;', 'return Reason::Count;')] != [True] * 10 or blob.count('common_safety(in, e2)') != 2:
        res['errors'].append('reaction_core.hpp: common_safety() must hold the shared identity/generation/signature/freshness/collider/aux checks and be used by both policies')
    if 'StaggerPolicy stagger_policy = StaggerPolicy::AttackOnly;' not in blob:
        res['errors'].append('reaction_core.hpp: Reaction::stagger_policy must default to AttackOnly (inherited laboratories keep the 0.5.3 policy)')
    gc = re.search(r'Reason classify_general\(.*?\n  \}\n', blob, flags=re.S)
    if not gc:
        res['errors'].append('reaction_core.hpp: classify_general() not found')
    else:
        g_ = gc.group(0)
        pos = [g_.find(s) for s in ('return Reason::NotConfirmed;', 'return Reason::Lethal;', 'return Reason::PostShapeUnknown;', 'return Reason::CrossesThreshold;',
                                    'return Reason::LatchedAfter;', 'return Reason::PostureSkipUnsafe;', 'return Reason::PreModeScripted;', 'if (const Reason s = common_safety(in, e2); s != Reason::Count) return s;',
                                    'return Reason::PostAuxUnexpected;', 'return Reason::PowerChangedInCall;', 'return Reason::Eligible;')]
        if min(pos) < 0 or pos != sorted(pos) or g_.count('return Reason::Eligible;') != 1 or 'PreModeNotAttack' in g_ or 'kModeAttack)' in g_.split('return Reason::PreModeScripted;')[0]:
            res['errors'].append('reaction_core.hpp: classify_general order must be confirmed < lethal < post shape < break/latch < posture skip < scripted mode < '
                                 'safety (signature, post aux, power) < Eligible once, last; no ATTACK condition')
    if 'E2Env e2_env = E2Env::Possible;' not in blob:
        res['errors'].append('reaction_core.hpp: Reaction::e2_env must default to Possible (conservative until the service probe proves absence)')
    if 'E2Policy e2_policy = E2Policy::Deferred;' not in blob:
        res['errors'].append('reaction_core.hpp: Reaction::e2_policy must default to Deferred (inherited laboratories keep the 0.5.1 E_OC2 behaviour)')
    if 'AdmissionPolicy pol = AdmissionPolicy::V1Strict' not in blob:
        res['errors'].append('hit_core.hpp: admit() must default to V1Strict (inherited laboratories keep the 0.5.0 admission)')
    # el reloj de postura: UN avance por mod_update y UN consumo por evento, ambos solo desde la envoltura
    if len(re.findall(r'\.advance\s*\(\s*\)', blob)) != 1 or len(re.findall(r'\bvoid\s+advance\s*\(', blob)) != 1:
        res['errors'].append('px.advance() must be defined once and called once (PostureEngine::on_update)')
    if len(re.findall(r'\.consume\s*\(', blob)) != 1 or len(re.findall(r'\b(?:void|ConsumeOutcome)\s+consume\s*\(', blob)) != 1:
        res['errors'].append('px.consume() must be defined once and called once (PostureEngine::on_post)')
    mc = [t for t in all_text if 'do_update' in t]
    if mc and len(re.findall(r'\bg\.pe\.on_update\s*\(', mc[0])) != 1:
        res['errors'].append('mod.cpp: g.pe.on_update() must be called exactly once (inside do_update)')
    # DEFINE_HOOK aparece en mod.cpp una sola vez (el nombre de objetivo se comprueba en el PE)
    # LNK SESSION02 (LNK05/06): UNA transicion nativa de Link, por la ruta del propio juego, y nada mas.
    mt = [t for f_, t in zip(res['files'], all_text) if f_ == 'mod.cpp']
    if mt:
        m_ = mt[0]
        pg = re.findall(r'\w*procGuard\w*\s*\(', m_)
        nb = re.search(r'bool lnk05_native_break\(daAlink_c\* link\) noexcept \{\n  \+\+lb\.native_calls;\n  \(void\)link->procGuardBreakInit\(\);\n'
                       r'  return link->mProcID == daAlink_c::PROC_GUARD_BREAK;\n\}\n', m_)
        if pg != ['procGuardBreakInit('] or not nb:
            res['errors'].append('mod.cpp: the only native Link procedure call is link->procGuardBreakInit(), inside lnk05_native_break, verified by mProcID')
        po = re.search(r'void on_link_damage_post_lnk05\(.*?\n\}\n', m_, flags=re.S)
        if not po or len(re.findall(r'\blnk05_native_break\s*\(', m_)) != 2 or 'lnk05_native_break(link)' not in po.group(0) \
                or po.group(0).find('if (gate != lnk05::Gate::Ok) {') < 0 or po.group(0).find('if (gate != lnk05::Gate::Ok) {') > po.group(0).find('lnk05_native_break(link)') \
                or 'lb.m.state() != lnk05::State::TriggerPending' not in po.group(0) or 'lb.m.triggered(entered,' not in po.group(0):
            res['errors'].append('mod.cpp: the native break is called once, from the checkDamageAction POST, only with a pending episode and an OK gate')
        if len(re.findall(r'add_post<LinkDamageFix01>\(on_link_damage_post_lnk05\)', m_)) != 1:
            res['errors'].append('mod.cpp: LNK05 rides the existing LinkDamageFix01 target with one extra POST callback')
        if 'gi.damage_this_call = fx.native.valid && fx.native.link_key == reinterpret_cast<std::uintptr_t>(link) && fx.native.tick == g.p8.tick &&\n' \
                '                          static_cast<int>(link->mDamageTimer) > fx.native.timer_before;' not in m_:
            res['errors'].append('mod.cpp: a native damage of the same execute must veto the break')
        if len(re.findall(r'\.break_reset\s*\(', m_)) != 1 or 'if (bo.reset_posture) lg.posture.break_reset();' not in m_:
            res['errors'].append('mod.cpp: posture reset only once, after the observed native exit')
        if len(re.findall(r'\blnk05_request\s*\(', m_)) != 3 or 'if (o.v == lnk01::Verdict::Confirmed) lnk05_request(lnk05::Kind::Block,' not in m_ \
                or 'if (d.r == lnk01::DirectResult::Confirmed) lnk05_request(lnk05::Kind::Direct,' not in m_:
            res['errors'].append('mod.cpp: break requests only from a confirmed block or a confirmed direct hit')
        m_s = fx_strip(m_)
        if 'lb.m.parry(g.p8.tick)' not in m_ or re.search(r'mProcID\s*[-+|&^]?=(?!=)|field_0x2fd5|mModeFlg\s*[-+|&^]?=(?!=)|mDamageTimer\s*[-+|&^]?=(?!=)'
                                                         r'|\b(?:on|off)ModeFlg\s*\(|\b(?:on|off)NoResetFlg\d\s*\(|\bcommonProcInit\s*\(|\bsetDamagePoint\s*\(', m_s):
            res['errors'].append('mod.cpp: parry protection required; no mProcID/field_0x2fd5/mModeFlg/mDamageTimer write')
        if not re.search(r'if \(o\.v == lnk01::Verdict::RejectedParry && v\.result == post08::Result::Confirmed\)[^\n]*\n\s*lnk05_log\(lb\.m\.parry\(g\.p8\.tick\)', m_) \
                or len(re.findall(r'\blb\.m\.parry\s*\(', m_)) != 1 or len(re.findall(r'\blb\.m\.observe\s*\(', m_)) != 1 \
                or len(re.findall(r'\blb\.m\.triggered\s*\(', m_)) != 1 or len(re.findall(r'\blb\.m\.request\s*\(', m_)) != 1:
            res['errors'].append('mod.cpp: one parry cancel (confirmed parry only), one observation per update, one trigger site, one request site')
        if len(re.findall(r'\bposture\.hold\s*\(', m_)) != 2 or len(re.findall(r'\bposture\.hold\s*\(lb\.m\.hold\(\)\)', m_)) != 2:
            res['errors'].append('mod.cpp: regeneration hold mirrors the break machine only')
    # LNK SESSION02 FIX01 0.11.1: defensa manual + penalizacion. Escrituras EXACTAS, hooks todo-o-nada, sin defensa agachada, constante unica.
    if mt:
        m_ = mt[0]
        bodies = {h: fx_body(m_, h) for h in FX_FUNCS}
        if any(b is None for b in bodies.values()):
            res['errors'].append('mod.cpp: the six FIX01-S2 callbacks must exist')
        else:
            B = {h: b.group(0) for h, b in bodies.items()}
            if m_.count(FX_SKIP) != 2 or B['HookAction on_rstatus_pre_fx('].count(FX_SKIP) != 1 or B['HookAction on_movedo_pre_fx('].count(FX_SKIP) != 1 \
                    or 'if (mods::arg<u8>(args, 1) != BUTTON_STATUS_SHIELD_ATTACK || !lnkfx_link(link, id)) return HOOK_CONTINUE;' not in B['HookAction on_rstatus_pre_fx('] \
                    or '!lf.def.dodge_blocked()) return HOOK_CONTINUE;' not in B['HookAction on_movedo_pre_fx('] \
                    or 'if (d == lnkfx::Dodge::None) return HOOK_CONTINUE;' not in B['HookAction on_movedo_pre_fx(']:
                res['errors'].append('mod.cpp: HOOK_SKIP_ORIGINAL only for the Shield Attack R status (0x3a) and for a classified voluntary dodge during the extended penalty')
            if m_.count(FX_FLAG_OFF) != 2 or 'if (lnkfx::clears_flag(gd)) {\n      ' + FX_FLAG_OFF not in B['void on_shield_post_fx('] \
                    or 'if (link->checkNoResetFlg2(daPy_py_c::FLG2_UNK_8000000)) {\n      ' + FX_FLAG_OFF not in B['HookAction on_collision_pre_fx('] \
                    or 'if (proc == daAlink_c::PROC_GUARD_BREAK) return HOOK_CONTINUE;' not in B['HookAction on_collision_pre_fx('] \
                    or '!lf.def.guard_blocked()) return HOOK_CONTINUE;' not in B['HookAction on_collision_pre_fx(']:
                res['errors'].append('mod.cpp: FLG2_UNK_8000000 is only switched OFF: by setShieldGuard POST on a denied lock-on guard, and by setCollision PRE during penalty/rearm')
            if m_.count('mItemTrigger =') != 1 or B['void on_stick_post_fx('].count(FX_B_CLEAR) != 1:
                res['errors'].append('mod.cpp: the only input write is clearing BTN_B for an offered R+B Shield Attack (setStickData POST)')
            if m_.count('*static_cast<BOOL*>(ret) =') != 2 or '*static_cast<BOOL*>(ret) = b == lnkfx::Bash::Accepted ? TRUE : FALSE;' not in B['void on_spaction_post_fx('] \
                    or '*static_cast<BOOL*>(ret) = FALSE;' not in B['HookAction on_movedo_pre_fx('] or 'if (!lf.enabled || !args || !ret || !lf.bash_pending) return;' not in B['void on_spaction_post_fx(']:
                res['errors'].append('mod.cpp: return values forced only at the Shield Attack site (R+B) and for a vetoed dodge')
            if len(re.findall(r'\bsetBStatus\s*\(', m_)) != 1 or 'if (lf.r_held && lf.def.phase() == lnkfx::Phase::Ready) {\n      link->setBStatus(BUTTON_STATUS_SHIELD_ATTACK);' not in B['void on_spaction_post_fx(']:
                res['errors'].append('mod.cpp: setBStatus only as the HUD Shield Attack label while R is held and the defence is READY')
            for t_ in FX_FORBIDDEN:
                if t_ in m_:
                    res['errors'].append('mod.cpp: FIX01-S2 forbidden token %s (no crouch shielding, no raw pad, no forced procs, no animation/speed writes)' % t_)
            ins = re.search(r'const ModResult fr\[6\] = \{.*?lf\.enabled = fx_ok;', m_, flags=re.S)
            if not ins or ins.group(0).count('mods::hook::uninstall<') != 6 or 'if (!fx_ok) {' not in ins.group(0) or m_.count('lf.enabled = true') != 0 \
                    or len(re.findall(r'\blf\.enabled\s*=(?!=)', m_)) != 3:
                res['errors'].append('mod.cpp: FIX01-S2 hooks are all-or-nothing (six installs, six uninstalls on failure, enabled only after all succeed)')
            for h, b in B.items():
                if 'if (!lf.enabled' not in b or 'lnkfx_link(link, id)' not in b or 'catch (...)' not in b:
                    res['errors'].append('mod.cpp: %s must check lf.enabled, the live human Link and fail closed' % h)
            if len(re.findall(r'\blf\.def\.begin\s*\(', m_)) != 2 or len(re.findall(r'\blf\.def\.observe\s*\(', m_)) != 1 or len(re.findall(r'\blf\.def\.sample\s*\(', m_)) != 1:
                res['errors'].append('mod.cpp: penalty begins only at a confirmed native entry (mod break / adopted / observed), one observation per tick, one input sample per execute')
            if 'if (lf.enabled && lf.def.request_blocked()) {' not in m_ \
                    or m_.count('ob.guard_safe = lnkfx::regen_bonus(ob.guard_safe, lf.enabled, lf.guard_accepted, lf.def.phase());') != 1 \
                    or len(re.findall(r'\bob\.guard_safe\s*=(?!=)', m_)) != 2:
                res['errors'].append('mod.cpp: no cascading break request during the penalty; LNK04 x1.5 only with an effective manual guard')
    alltxt = '\n'.join(all_text)
    if len(re.findall(r'kTotalPenaltyTicks\s*=', alltxt)) != 1:
        res['errors'].append('the penalty length is ONE versioned constant (lnkfx::policy::kTotalPenaltyTicks)')
    lfx = [t for f_, t in zip(res['files'], all_text) if f_ == 'lnks2fix_core.hpp']
    if len(lfx) != 1 or any(tok in lfx[0] for tok in ('uintptr_t', 'reinterpret_cast', 'daAlink', '->', 'daPy_', '#include <d/', '#include <m_Do')):
        res['errors'].append('lnks2fix_core.hpp: the defence machine is pure (no address, no game type)')
    lk2 = [t for f_, t in zip(res['files'], all_text) if f_ == 'lnks2_core.hpp']
    if len(lk2) != 1 or any(tok in lk2[0] for tok in ('uintptr_t', 'reinterpret_cast', 'daAlink', '->')):
        res['errors'].append('lnks2_core.hpp: the break machine is pure (no address, no game type)')
    if hook_defs != 22:
        res['errors'].append('DEFINE_HOOK count %d != 22 (21 inherited + 1 scoped visual Pikari observer)' % hook_defs)
    # POST07: the HUD core is pure display logic: it never names a posture/POST05/POST06 type and owns no address
    hc = [t for f_, t in zip(res['files'], all_text) if f_ == 'hud_core.hpp']
    if len(hc) != 1:
        res['errors'].append('hud_core.hpp missing')
    else:
        for tok in ('post04', 'post05', 'post06', 'post03', 'uintptr_t', 'reinterpret_cast', 'static_cast<fopAc', '->'):
            if tok in hc[0]:
                res['errors'].append('hud_core.hpp: display core may not use %s' % tok)
    # LNK01: el nucleo de Link es logica pura: ni tipos de POST03-06 ni direcciones
    lk = [t for f_, t in zip(res['files'], all_text) if f_ == 'lnk_core.hpp']
    if len(lk) != 1:
        res['errors'].append('lnk_core.hpp missing')
    else:
        for tok in ('post04', 'post05', 'post06', 'post03', 'uintptr_t', 'reinterpret_cast', 'static_cast<fopAc', 'daAlink', 'dMeter'):
            if tok in lk[0]:
                res['errors'].append('lnk_core.hpp: pure core may not use %s' % tok)
    res['define_hook_count'] = hook_defs
    res['status'] = 'SOURCE_AUDIT_PASS' if not res['errors'] else 'SOURCE_AUDIT_FAIL'
    print(json.dumps(res, indent=2))
    return 0 if not res['errors'] else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1]))
