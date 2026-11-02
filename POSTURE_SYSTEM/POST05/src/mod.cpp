// Enemy Posture System, POST05: supresion SELECTIVA de UNA reaccion ordinaria (DAMAGE) sobre el observador de POST03.1 y la postura de POST04.
// Unica escritura al juego: restore_reaction() devuelve mActionMode/mOcState/mOldActionMode a sus valores PRE de ESTA llamada, solo cuando
// reaction_core.hpp lo autoriza (espada no letal ponderada, ATTACK 0/1/2, cooldown 0->10, efectos extra de setActionMode no-ops, layout = binario).
// Todo lo demas sigue pasivo. POST04_BREAK_READY sigue siendo solo una linea de log.
// Un unico hook: daE_OC_c::damage_check (PRE siempre HOOK_CONTINUE + POST). La unica consulta extra es de SOLO LECTURA, corre en mod_update
// (fuera de fpcM_Management) y es fopAcM_SearchByID(id, &out) sobre un ID ya observado. Nunca se desreferencia un puntero guardado.
// La unica desreferencia de collider ocurre en el POST, sobre mAtInfo.mpCollider, solo tras un impacto PROCESADO y confirmado cuyo
// puntero coincide con el hit obj crudo de una esfera de la MISMA llamada (ver HIT_PATH_AUDIT.md).
// No llama a setActionMode, offTgSph, cc_at_check, mStts.Move, ClrTgHit ni a ningun mutador.
#include <mods/service.hpp>
#include <mods/svc/hook.hpp>
#include <mods/svc/host.h>
#include <mods/svc/log.h>
#include <m_Do/m_Do_ext.h>
#include <d/actor/d_a_e_oc.h>
#include <d/d_com_inf_game.h>
#include <f_pc/f_pc_manager.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <utility>
#include "break_core.hpp"
#include "helm_core.hpp"
#include "hud_core.hpp"
#include "parry_core.hpp"
#include "lnk_core.hpp"
#include "fix01_core.hpp"
#include "lnks2_core.hpp"
#include <d/actor/d_a_alink.h>
// POST07 HUD (display only): the same native 2D path Enemy Health Bars CP06 uses (dMeter2_c hooks + a packet in the 2D XLU list), re-implemented here.
#include <mods/svc/interp.h>
#include <d/d_meter2.h>
#include <d/d_meter2_info.h>
#include <d/d_meter2_draw.h>
#include <d/d_meter_button.h>
#include <d/d_camera.h>
#include <f_op/f_op_actor_tag.h>
#include <m_Do/m_Do_lib.h>
#include <m_Do/m_Do_graphic.h>
#include <JSystem/J2DGraph/J2DOrthoGraph.h>

#ifndef POST05_SOURCE_COMMIT
#define POST05_SOURCE_COMMIT "unset"
#endif

static_assert(offsetof(daPy_py_c,mCutType)==0x668,"Link cut type offset measured in host setCutType");
static_assert(offsetof(daAlink_c,mProcID)==0x4144,"Link procedure offset measured in host procCutHeadInit");
static_assert(offsetof(daAlink_c,mGuardAtCps)==0x1e78,"Shield collider address measured in host setAtCollision");
static_assert(daAlink_c::PROC_CUT_HEAD==0x2d,"real Helm procedure enum");
DEFINE_MOD();
IMPORT_SERVICE(HostService, svc_host);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(HookService, svc_hook);
// El UNICO objetivo de hook de este modulo.
DEFINE_HOOK(&daE_OC_c::damage_check, EocDamageCheckPost05);
DEFINE_HOOK(&daE_OC_c::executeDamage, EocShieldStunPost06);
DEFINE_HOOK(&daAlink_c::checkDamageAction, LinkDamageFix01);
DEFINE_HOOK(&daAlink_c::checkPolyDamage, LinkPolyFix01);
DEFINE_HOOK(&daE_OC_c::executeAttack, AttackObserveFix01);
DEFINE_HOOK(&daE_OC_c::cc_set, CollisionCleanupFix01);
static_assert(std::is_same_v<decltype(&daAlink_c::checkDamageAction), int (daAlink_c::*)()>);
static_assert(std::is_same_v<decltype(&daAlink_c::checkPolyDamage), int (daAlink_c::*)()>);
static_assert(offsetof(daAlink_c,mTgCyls)==0xb28 && sizeof(dCcD_Cyl)==0x1c0);
static_assert(offsetof(daAlink_c,field_0x2fb8)==0x4114);
// LNK SESSION02: native guard break. procGuardBreakInit is exported (?procGuardBreakInit@daAlink_c@@QEAAHXZ, int (daAlink_c::*)(), no arguments);
// mModeFlg measured in damageTimerCount 0x14085e580: test byte [rcx+0x42fc],8 (checkModeFlg(8)); PROC_GUARD_BREAK = 0x1f.
static_assert(std::is_same_v<decltype(&daAlink_c::procGuardBreakInit), int (daAlink_c::*)()>, "procGuardBreakInit signature");
static_assert(offsetof(daAlink_c, mModeFlg) == 0x42fc && std::is_same_v<decltype(daAlink_c::mModeFlg), u32>, "mModeFlg != exe (damageTimerCount)");
static_assert(lnk05::policy::kProcGuardBreak == daAlink_c::PROC_GUARD_BREAK && lnk05::policy::kProcGuardSlip == daAlink_c::PROC_GUARD_SLIP &&
              lnk05::policy::kProcGuardAttack == daAlink_c::PROC_GUARD_ATTACK, "guard procedures");
static_assert(daAlink_c::PROC_WAIT == 3 && daAlink_c::PROC_MOVE == 4 && daAlink_c::PROC_ATN_ACTOR_MOVE == 7 && daAlink_c::PROC_MOVE_TURN == 9 &&
              daAlink_c::PROC_STEP_MOVE == 0x1b && daAlink_c::PROC_CROUCH == 0x1c && daAlink_c::PROC_SERVICE_WAIT == 1, "allowed origin procedures");
static_assert(offsetof(daPy_py_c,mResetFlg1)==0x684);
// POST07: HUD hooks on the HUD meter (NOT on actors): _execute POST (simulation tick), _draw POST (sample + queue), _delete PRE (scrub). Display only.
DEFINE_HOOK(&dMeter2_c::_execute, MeterExecutePost07);
DEFINE_HOOK(&dMeter2_c::_draw, MeterDrawPost07);
DEFINE_HOOK(&dMeter2_c::_delete, MeterDeletePost07);
// LNK01: VISUAL-ONLY hook on the bottom-centre action-prompt painter (virtual dMeterButton_c::draw, vtable slot 0): PRE masks the two sparkle slots of
// the LIVE prompt (mpEmpButton) for the duration of its own draw, POST restores them. It never skips the original and never touches game logic.
DEFINE_HOOK(&dMeterButton_c::draw, PromptDrawLnk01);
// Optional presentation interpolation of the anchor (same service Enemy Health Bars uses); without it the bar follows the simulation anchor.
IMPORT_OPTIONAL_SERVICE(InterpService, svc_interp);

// ---- ABI: tipos y offsets Win64 comprobados en compilacion (los mismos de POST01, medidos en el EXE oficial) -------------
static_assert(std::is_same_v<decltype(&daE_OC_c::damage_check), void (daE_OC_c::*)()>, "damage_check signature");
static_assert(sizeof(decltype(&daE_OC_c::damage_check)) == 8, "compact Windows x64 PMF required");
static_assert(std::is_same_v<decltype(fopAc_ac_c::health), s16>, "health type");
static_assert(std::is_same_v<decltype(daE_OC_c::field_0x6cc), s16>, "field_0x6cc type");
static_assert(std::is_same_v<decltype(daE_OC_c::mActionMode), int>, "mActionMode type");
static_assert(std::is_same_v<decltype(daE_OC_c::mOcState), int>, "mOcState type");
static_assert(offsetof(fopAc_ac_c, health) == 0x656, "fopAc_ac_c::health offset != exe");
static_assert(offsetof(fopAc_ac_c, actor_status) == 0x580, "fopAc_ac_c::actor_status offset != exe");
static_assert(offsetof(daE_OC_c, mActionMode) == 0x8c8, "daE_OC_c::mActionMode offset != exe");
static_assert(offsetof(daE_OC_c, mOcState) == 0x8d0, "daE_OC_c::mOcState offset != exe");
static_assert(offsetof(daE_OC_c, field_0x6cc) == 0x8ec, "daE_OC_c::field_0x6cc offset != exe");
// POST03: offsets nuevos medidos en el desensamblado de damage_check del EXE oficial (HIT_PATH_AUDIT.md): mOldActionMode 0x8cc (setActionMode),
// mAtInfo 0x1398 (el mov del GetTgHitObj nativo a mAtInfo.mpCollider, que es el primer campo de dCcU_AtInfo).
static_assert(std::is_same_v<decltype(daE_OC_c::mOldActionMode), int>, "mOldActionMode type");
static_assert(offsetof(daE_OC_c, mOldActionMode) == 0x8cc, "daE_OC_c::mOldActionMode offset != exe");
static_assert(offsetof(daE_OC_c, mAtInfo) == 0x1398, "daE_OC_c::mAtInfo offset != exe");
static_assert(offsetof(dCcU_AtInfo, mpCollider) == 0, "dCcU_AtInfo::mpCollider must be the first field");
static_assert(std::is_same_v<decltype(std::declval<const cCcD_Obj&>().GetAtType()), u32>, "GetAtType() is an inline const u32 getter");
static_assert(sizeof(decltype(std::declval<const cCcD_Obj&>().GetAtType())) == 4, "the only collider read is a 4-byte int");
static_assert(alignof(cCcD_Obj) == 8, "hypothesis recorded in the audit: cCcD_Obj is declared 8-aligned (the host heap only guarantees 4)");
static_assert(MOD_OK == 0, "install_sequence assumes MOD_OK == 0");
// POST05: campos que setActionMode escribe ademas del modo (binario 0x140e0d260: u32 en +0x8e0, and $0xff7f en +0x69a) y que deben haber sido no-ops.
static_assert(std::is_same_v<decltype(daE_OC_c::field_0x6c0), s16> && std::is_same_v<decltype(daE_OC_c::field_0x6c2), s16>, "timer types");
static_assert(offsetof(daE_OC_c, field_0x6c0) == 0x8e0 && offsetof(daE_OC_c, field_0x6c2) == 0x8e2, "field_0x6c0/6c2 offsets != exe (u32 write at +0x8e0)");
static_assert(std::is_same_v<decltype(fopEn_enemy_c::mFlags), u16> && offsetof(fopEn_enemy_c, mFlags) == 0x69a, "fopEn_enemy_c::mFlags != exe (+0x69a)");
static_assert(fopEn_enemy_c::fopEn_flag_HeadLock == 0x80, "HeadLock bit != exe (and $0xff7f)");
static_assert(sizeof(dCcD_Sph) == 0x1b8, "dCcD_Sph stride != exe (0xea0 - 0xce8)");
static_assert(std::is_polymorphic_v<cCcD_ObjTg> && std::is_polymorphic_v<cCcD_ObjCo>, "Tg/Co objects carry a vptr before mSPrm under MSVC x64");
// Consulta de existencia: la sobrecarga NO inline (f_op_actor_mng.cpp:152): 0 = ausente; 1 con out==NULL = en creacion; 1 con out = presente.
static_assert(std::is_same_v<decltype(fopAcM_SearchByID(std::declval<fpc_ProcID>(), std::declval<fopAc_ac_c**>())), s32>,
              "fopAcM_SearchByID(id, out) signature");
static_assert(std::is_same_v<fpc_ProcID, unsigned int>, "fpc_ProcID is u32");
// POST05-RB (0.5.2): fase nativa (SOLO LECTURA). Offsets medidos en el EXE 2.0.3 (SHA-256 ffa99203...0049):
//   executeAttack 0x140e06f90: mov 0x6d8(%rcx),%rax ; movss 0x2c(%rax) (getFrame) ; test %cl,0x21(%rax) + movss 0x28(%rax) (isStop) ;
//   lea 0x620(%rsi),%rcx -> cLib_chaseF(&speedF, 0, 1.0) (ATTACK 0) ; J3DFrameCtrl::update 0x1416b3410: attr +8, state +9, end +0xc, rate +0x10, frame +0x14.
static_assert(std::is_same_v<decltype(daE_OC_c::mpMorf), mDoExt_McaMorfSO*>, "mpMorf type");
static_assert(offsetof(daE_OC_c, mpMorf) == 0x6d8, "daE_OC_c::mpMorf offset != exe (+0x6d8)");
static_assert(std::is_base_of_v<mDoExt_morf_c, mDoExt_McaMorfSO>, "mDoExt_McaMorfSO derives from mDoExt_morf_c");
static_assert(offsetof(mDoExt_morf_c, mFrameCtrl) == 0x18, "mDoExt_morf_c::mFrameCtrl offset != exe (+0x18)");
static_assert(offsetof(J3DFrameCtrl, mAttribute) == 0x8 && offsetof(J3DFrameCtrl, mState) == 0x9 && offsetof(J3DFrameCtrl, mEnd) == 0xc &&
                  offsetof(J3DFrameCtrl, mRate) == 0x10 && offsetof(J3DFrameCtrl, mFrame) == 0x14,
              "J3DFrameCtrl layout != exe (update: +8 attr, +9 state, +0xc end, +0x10 rate, +0x14 frame)");
static_assert(std::is_same_v<decltype(J3DFrameCtrl::mEnd), s16> && std::is_same_v<decltype(J3DFrameCtrl::mRate), f32> &&
                  std::is_same_v<decltype(J3DFrameCtrl::mFrame), f32> && std::is_same_v<decltype(J3DFrameCtrl::mAttribute), u8> &&
                  std::is_same_v<decltype(J3DFrameCtrl::mState), u8>,
              "J3DFrameCtrl field types");
static_assert(std::is_same_v<decltype(fopAc_ac_c::speedF), f32> && offsetof(fopAc_ac_c, speedF) == 0x620, "fopAc_ac_c::speedF != exe (+0x620)");
// POST06: rama letal nativa de damage_check (binario 0x140e065bd: cmpw $0x1,0x656(%rbx); jg ...; mov %esi,0x650(%rbx)) = health <= 1 => attention_info.flags = 0.
static_assert(std::is_same_v<decltype(actor_attention_types::flags), u32>, "attention flags type");
static_assert(offsetof(fopAc_ac_c, attention_info) + offsetof(actor_attention_types, flags) == 0x650, "attention_info.flags != exe (+0x650)");
// POST07: the HUD slot table mirrors the ledger 1:1 and reads the POST06 phase numerically.
static_assert(post07::kSlots == post03::kCapacity, "one HUD slot per ledger slot");
// POST08: Link (offsets medidos en el EXE 2.0.3): setCollision 0x1408cb349 lea 0xb28 = mTgCyls (3 x 0x1c0; Tg SPrm en 0xc10/0xdd0/0xf90);
// procGuardAttack 0x14089657f movss 0x2d4c = mUnderFrameCtrl[0].mFrame; comiss 0x45d8/0x45dc = inicio/fin del ataque del escudo;
// checkPlayerGuardAndAttack 0x14084f703 cmpw $0x1e,0x4144 = PROC_GUARD_ATTACK.
static_assert(post08::kSlots == post03::kCapacity, "one parry episode per ledger slot");
static_assert(offsetof(daAlink_c, mTgCyls) == 0xb28 && sizeof(dCcD_Cyl) == 0x1c0, "Link Tg cylinders != exe (setCollision)");
static_assert(offsetof(daAlink_c, mUnderFrameCtrl) + offsetof(J3DFrameCtrl, mFrame) == 0x2d4c, "Link animation frame != exe (procGuardAttack)");
static_assert(offsetof(daAlink_c, field_0x3478) == 0x45d8 && offsetof(daAlink_c, field_0x347c) == 0x45dc, "Shield Attack start/end frame != exe");
static_assert(daAlink_c::PROC_GUARD_ATTACK == 0x1e, "PROC_GUARD_ATTACK != exe (checkPlayerGuardAndAttack)");
static_assert(std::is_same_v<decltype(daAlink_c::field_0x3478), f32> && std::is_same_v<decltype(daAlink_c::field_0x347c), f32>, "Shield Attack frame types");
static_assert(offsetof(daE_OC_c, mSphs_at) == 0x1028, "E_OC At spheres != exe (executeAttack lea 0x1028 / 0x11e0)");
static_assert(static_cast<int>(post06::Phase::Tracking) == static_cast<int>(post07::Ph::Tracking) && static_cast<int>(post06::Phase::Punish) == static_cast<int>(post07::Ph::Punish) &&
                  static_cast<int>(post06::Phase::Falling) == static_cast<int>(post07::Ph::Falling) && static_cast<int>(post06::Phase::Down) == static_cast<int>(post07::Ph::Down) &&
                  static_cast<int>(post06::Phase::GettingUp) == static_cast<int>(post07::Ph::GettingUp) && static_cast<int>(post06::Phase::Dead) == static_cast<int>(post07::Ph::Dead),
              "POST06 phase numbering consumed by the HUD");
static_assert(sizeof(dDlst_list_c::mp2DXluDrawLists) / sizeof(dDlst_base_c*) == 32, "2D XLU list size");
// LNK01: HUD prompt layout MEASURED in the official EXE 2.0.3 (sha256 ffa99203...0049), see POSTURE_SYSTEM/LNK01/IMPLEMENTATION.md:
//   dMeter2_c::_draw 0x140a25270: mov rcx,[rbx+0x1e0] -> set2DOpa/set2DOpaTop (mpMeterDraw); mov rcx,[rbx+0x1f8]; test; set2DOpaTop (mpEmpButton).
//   dMeterButton_c::draw 0x140a01e60: mov rcx,[rbx+0x10] / [rbx+0x18] / [rbx+0x20] / [rbx+0x28] -> J2DScreen::draw (button screen, unused, message
//   frame, message text); cmp byte [rbx+rdi+0x5d2],0|1 (field_0x4be[i] == BUTTON_A|B); _delete 0x1409ff3c0: delete [rbx+0x10] (mpButtonScreen).
//   dMeter2Draw_c::draw 0x140a12de0: mov rcx,[rsi+0x80] -> J2DScreen::draw (mpScreen). J2DPane::draw 0x141663180: cmp byte [rcx+0xb0],0 (mVisible).
static_assert(offsetof(dMeter2_c, mpMeterDraw) == 0x1e0 && offsetof(dMeter2_c, mpEmpButton) == 0x1f8, "dMeter2_c HUD members != exe (_draw)");
static_assert(offsetof(dMeterButton_c, mpButtonScreen) == 0x10 && offsetof(dMeterButton_c, mpScreen) == 0x20, "dMeterButton_c screens != exe (draw/_delete)");
static_assert(offsetof(dMeterButton_c, field_0x4be) == 0x5d2 && std::is_same_v<decltype(dMeterButton_c::field_0x4be), u8[2]>, "sparkle slots != exe (draw)");
static_assert(offsetof(dMeter2Draw_c, mpScreen) == 0x80, "dMeter2Draw_c::mpScreen != exe (draw)");
static_assert(offsetof(J2DPane, mVisible) == 0xb0 && std::is_same_v<decltype(J2DPane::mVisible), bool>, "J2DPane::mVisible != exe (J2DPane::draw)");
static_assert(std::is_same_v<decltype(dMeterButton_c::mpButtonScreen), J2DScreen*> && std::is_base_of_v<J2DPane, J2DScreen> && std::is_polymorphic_v<J2DScreen>,
              "the prompt's button screen is a polymorphic J2DScreen (vptr comparable)");
static_assert(static_cast<int>(dMeterButton_c::BUTTON_NONE_e) == static_cast<int>(lnk01::kButtonNone), "BUTTON_NONE_e");
static_assert(sizeof(decltype(&dMeterButton_c::draw)) == 8, "compact Windows x64 PMF (vcall thunk, slot 0)");
static_assert(lnk01::kSlots == post03::kCapacity, "one attacker ledger entry per ledger slot");
static_assert(daAlink_c::PROC_GUARD_SLIP == 0x1d && daAlink_c::PROC_GUARD_BREAK == 0x1f, "native guard procedures (measured only)");
// LNK SESSION01: Link damage / guard evidence MEASURED in the official EXE 2.0.3 (read only):
//   daAlink_c::damageTimerCount 0x14085e580: dec word [rcx+0x66c] (mDamageTimer), and [rcx+0x674] (mNoResetFlg1).
//   daAlink_c::checkUpperGuardAnime 0x140853b30: test dword [rcx+0x678],0x8000000 (mNoResetFlg2 FLG2_UNK_8000000); test byte [rcx+0x670],2
//   (mNoResetFlg0 FLG0_UNK_2); proc-0x1e <= 1 (GUARD_ATTACK/GUARD_BREAK); cmp 0x25 (CUT_REVERSE). setDamagePoint arms mDamageTimer (d_a_alink_damage.inc:224).
static_assert(offsetof(daPy_py_c, mDamageTimer) == 0x66c && std::is_same_v<decltype(daPy_py_c::mDamageTimer), s16>, "mDamageTimer != exe (damageTimerCount)");
static_assert(offsetof(daPy_py_c, mNoResetFlg0) == 0x670 && offsetof(daPy_py_c, mNoResetFlg1) == 0x674 && offsetof(daPy_py_c, mNoResetFlg2) == 0x678,
              "Link no-reset flags != exe (checkUpperGuardAnime / damageTimerCount)");
static_assert(daPy_py_c::FLG2_UNK_8000000 == 0x8000000 && daPy_py_c::FLG0_UNK_2 == 2, "upper-guard flags");
static_assert(daAlink_c::PROC_CUT_REVERSE == 0x25 && daAlink_c::PROC_GUARD_ATTACK == 0x1e, "guard-excluding procedures (checkUpperGuardAnime)");

namespace {

using namespace post03;

struct Runtime {
  post04::PostureEngine pe;
  Engine& eng = pe.eng;
  post05::Reaction rx;
  int layout = -1;  // -1 sin comprobar, 0 no coincide con el binario (supresion cerrada para la sesion), 1 coincide
  unsigned depth = 0;
  bool reentered = false;
  bool installed = false;
  unsigned consecutive_log_errors = 0;
  // POST05-FINALIZE: entorno de la IA de variantes (ver probe_variant_ai_env)
  bool env_possible = false;
  post06::HelmEpisode helm_episodes[post03::kCapacity]{};
  post06::Breaker br;  // POST06: ruptura -> castigo -> caida nativa -> suelo -> levantamiento -> rearme
  // contexto de la llamada en curso (PRE -> POST de la MISMA llamada; damage_check no se anida)
  struct CallCtx {
    std::uintptr_t actor = 0;
    int slot = -1;
    std::uint64_t generation = 0;
    std::uint32_t id = 0;
    bool tracked = false;
    int pre_hp = 0;
    int pre_mode = 0;
    bool helm = false, mask_owned = false, from_link = false, link_helm_proc = false, link_known = false;
    std::uint32_t link_id = 0;
    int helm_floor = 0;
    std::uintptr_t selected_raw = 0;
    post08::Input pin;      // POST08: lecturas PRE del parry de ESTA llamada
    bool lnk_tg_match = false;  // LNK02: el primer cilindro Tg de Link con impacto registro una esfera At de ESTE actor
    int lnk_tg_cyl = -1;
    post08::Verdict pv;     // POST08: veredicto PRE
    post05::LiveMode pre_live;
    NativeAux pre_aux;
    lnks1fix::Continuation continuation=lnks1fix::Continuation::Safe;
  } cur;
  post08::Parry parry;  // POST08
  post08::LinkTrack link_track;  // POST08 FIX01: Shield Attack de Link sellado por tick (mod_update)
  post08::TimingHist hist;       // POST08 FIX01: delta de los bloqueos durante un Shield Attack
  post08::ArmTrack arm;          // POST08 FIX02: esfera At armada en el tick anterior (frescura del registro de contacto)
  post08::AiWatch watch;         // POST08 FIX02: observador de IA de SOLO LECTURA
  struct P8 {
    int layout = -1;
    bool watch_life = false;
    int life_at = 0;
    unsigned life_lines = 0;
    std::uint64_t attempts = 0, committed = 0, units = 0, breaks = 0, stun_suppressed = 0, unexpected_hp = 0, failed = 0, life_drop_next_tick = 0;
    std::uint64_t tick = 0;  // FIX01: un tick por mod_update (= un tick de simulacion)
    std::uint64_t transitioned = 0, blocked_in_sa = 0, contact_lines = 0, sa_lines = 0, contact_lines_dropped = 0, sa_lines_dropped = 0;
    std::uint64_t plan[4] = {}, watch_lines = 0, watch_lines_dropped = 0;  // FIX02
    // FIX02: ruptura por parry APLAZADA una llamada (el mareo escrito en el tick del contacto dejaba vivo el registro At, log: 221 l.3040 -> 3064)
    struct PendingBreak {
      bool on = false;
      int slot = -1;
      std::uint32_t id = 0;
      std::uint64_t generation = 0, tick = 0;
    } pending_break[post08::kSlots];  // una por slot del ledger (dos rupturas simultaneas en dos actores no se pisan)
    std::uint64_t breaks_deferred = 0, breaks_delivered = 0, breaks_expired = 0;
  } p8;
  unsigned env_ticks = 0, env_queries = 0, env_lines = 0;
} g;

// LNK01: estado PRIVADO de la postura de Link, de su barra y de la supresion visual de los prompts. Ningun puntero del juego se guarda aqui.
struct LnkGlue {
  lnk01::Posture posture;
  lnk01::PlayerHud hud;
  lnk01::ActionUi ui;
  lnk01::SparkleMask mask;
  bool link_valid = false, link_human = false, sim_active = false, sparkle_hook = false;
  std::uint32_t link_id = 0;
  std::uint64_t hud_epoch = 0;
  int last_proc = -1;
  std::uint64_t guard_slip_entries = 0, guard_break_entries = 0, guard_break_at_100 = 0, hide_verify_failed = 0, prompt_draw_pre = 0, prompt_draw_live = 0;
  std::uint64_t player_frames = 0, player_quads = 0, layout_changes = 0;
  unsigned block_lines = 0, regen_lines = 0, guard_lines = 0, ui_lines = 0, layout_lines = 0;
  std::uint64_t block_dropped = 0, regen_dropped = 0, guard_dropped = 0, ui_dropped = 0;
  float last_w = 0, last_h = 0, last_minx = 0;
  // LNK SESSION01
  lnk01::LinkObs obs;
  unsigned direct_lines = 0, parry_lines = 0, s1_regen_lines = 0, balance_lines = 0;
  std::uint64_t direct_dropped = 0, parry_dropped = 0, s1_regen_dropped = 0, balance_dropped = 0, direct_events_dropped = 0, tg_reads = 0, tg_matches = 0;
  std::uint64_t parry_post08_enemy_credit = 0, guard_safe_ticks = 0, life_invalid_ticks = 0;
} lg;
struct Fix01Glue {
 lnks1fix::DirectJoin direct;lnks1fix::AiMonitor ai;lnks1fix::CollisionCleanup cleanup;
 unsigned cc_depth=0,cc_added=0;int cc_slot=-1;std::uintptr_t cc_key=0;
 lnks1fix::NativeDamage native;unsigned native_depth=0;bool native_reentered=false;
 bool attack_open=false;std::uintptr_t attack_key=0;NativeAux attack_entry;int attack_state=-1;
 unsigned direct_lines=0,ai_lines=0,ai_context_lines=0;std::uint64_t direct_dropped=0,ai_dropped=0,ai_context_dropped=0,reentered=0;
} fx;
// LNK SESSION02: maquina PROPIA de ruptura (observa, solicita por la ruta nativa y confirma). Ningun puntero del juego se guarda aqui.
struct LnkBreakGlue {
  lnk05::Machine m;
  bool enabled = false;
  std::uint64_t native_calls = 0, posts_seen = 0;
  unsigned event_lines = 0, native_lines = 0, recovery_lines = 0, gate_lines = 0, vuln_lines = 0;
  std::uint64_t event_dropped = 0, native_dropped = 0, recovery_dropped = 0, gate_dropped = 0, vuln_dropped = 0;
} lb;


template <class T>
bool compatible(const T* p, unsigned major, unsigned minor) noexcept {
  return p && p->header.struct_size >= sizeof(T) && p->header.major_version == major && p->header.minor_version >= minor;
}

// ---- log (cap duro, fail-open) --------------------------------------------------------------------------------------
void emit(LogCat cat, LogLevel level, const char* line) noexcept {
  if (!svc_log || !svc_log->write) return;
  if (!g.eng.obs.budget.take(cat, g.eng.obs.ledger.c)) return;
  try {
    svc_log->write(mod_ctx, level, line);
    g.consecutive_log_errors = 0;
  } catch (...) {
    ++g.eng.obs.ledger.c.log_errors;
    if (++g.consecutive_log_errors >= kDisableLogErrors) g.eng.obs.disable("LOG_SERVICE_EXCEPTIONS");
  }
}

// Vacia la cola de eventos de postura (tamano fijo) respetando los topes por tipo y el presupuesto global de 500 lineas.
void drain_posture() noexcept {
  post04::PItem it;
  char line[kLogLineMax];
  while (g.pe.px.pop(it)) {
    if (!g.pe.px.log_take(it.kind)) continue;
    if (post04::fmt_item(line, sizeof(line), it) <= 0) continue;
    try {
      if (svc_log && svc_log->write) svc_log->write(mod_ctx, LOG_LEVEL_INFO, line);
    } catch (...) {
      ++g.eng.obs.ledger.c.log_errors;
      if (++g.consecutive_log_errors >= kDisableLogErrors) g.eng.obs.disable("LOG_SERVICE_EXCEPTIONS");
    }
  }
}

void emit_events(const Events& ev) noexcept {
  char line[kLogLineMax];
  for (unsigned i = 0; i < ev.n; ++i) {
    if (!g.eng.obs.ledger.may_log(ev.e[i].kind)) continue;
    if (fmt_event(line, sizeof(line), ev.e[i], g.eng.obs.ledger.c.calls) <= 0) continue;
    const bool warn = ev.e[i].kind == EvKind::CapacityExhausted || ev.e[i].kind == EvKind::InvalidKey || ev.e[i].kind == EvKind::Unverified;
    emit(LogCat::Life, warn ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
  }
}

void emit_disabled_once() noexcept {
  if (!g.eng.obs.disabled || g.eng.obs.disabled_logged) return;
  g.eng.obs.disabled_logged = true;
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line), "POST03_DISABLED reason=%s gameplay_untouched=1", g.eng.obs.disabled_reason);
  emit(LogCat::Other, LOG_LEVEL_WARN, line);
}

// ---- lecturas (solo accesores const/inline; ninguna muta flags ni cachea) ---------------------------------------------
bool tg_hit(const dCcD_Sph& s) noexcept {
  // SOLO el bit TG crudo. NO se usa GetTgHitAc()/GetAc(): escribe una cache (d_cc_d.cpp:26-37).
  return s.cCcD_ObjHitInf::ChkTgHit() != 0;
}

ActorFacts read_facts(daE_OC_c* a) noexcept {
  ActorFacts f;
  f.profile_ok = static_cast<int>(a->profname) == static_cast<int>(fpcNm_E_OC_e);
  f.live = a->state.init_state >= 0 && a->state.init_state != 3;  // 3 = en borrado
  if (f.profile_ok && f.live) {
    f.name = classify_name(a->mName);  // lectura acotada a 6 bytes
    f.status_10000 = (a->actor_status & static_cast<u32>(fopAcStts_UNK_0x10000_e)) != 0;
    const int a0 = a->mSphs_at[0].GetAtAtp(), a1 = a->mSphs_at[1].GetAtAtp();
    f.atp = a0 == a1 ? a0 : -2;
  }
  return f;
}

void read_core(daE_OC_c* a, Snapshot& s) noexcept {
  s.health = a->health;
  s.mode = a->mActionMode;
  s.state = a->mOcState;
  s.old_mode = a->mOldActionMode;
  s.cooldown = a->field_0x6cc;
  s.cutdown = a->checkCutDownHitFlg() ? 1 : 0;
  s.tg_bits = static_cast<std::uint8_t>((tg_hit(a->mSphs_cc[0]) ? 1 : 0) | (tg_hit(a->mSphs_cc[1]) ? 2 : 0));
}

// ---- callbacks del hook ------------------------------------------------------------------------------------------------
constexpr AtMasks kMasks = {
    static_cast<std::uint32_t>(AT_TYPE_MASTER_SWORD) | static_cast<std::uint32_t>(AT_TYPE_NORMAL_SWORD),
    static_cast<std::uint32_t>(AT_TYPE_SHIELD_ATTACK),
    static_cast<std::uint32_t>(AT_TYPE_ARROW),
    static_cast<std::uint32_t>(AT_TYPE_BOMB),
    static_cast<std::uint32_t>(AT_TYPE_IRON_BALL),
    static_cast<std::uint32_t>(AT_TYPE_BOOMERANG),
    static_cast<std::uint32_t>(AT_TYPE_40),
    static_cast<std::uint32_t>(AT_TYPE_SLINGSHOT)};

// Hit obj CRUDO de una esfera (cCcD_ObjHitInf::GetTgHitObj, inline const): valor opaco, solo para comparar; 0 si el bit TG no esta.
std::uintptr_t raw_hit_obj(const dCcD_Sph& s) noexcept {
  return s.cCcD_ObjHitInf::ChkTgHit() ? reinterpret_cast<std::uintptr_t>(s.cCcD_ObjHitInf::GetTgHitObj()) : 0;
}

// UNICA desreferencia de collider del modulo. Solo la invoca el motor tras un HIT_CONFIRMED cuyo mAtInfo.mpCollider coincide con el hit obj
// crudo de una esfera de esta misma llamada: es el mismo objeto que damage_check acaba de desreferenciar (ChkAtType, d_a_e_oc.cpp:673-754).
bool read_at_type(std::uintptr_t sel, std::uint32_t& type) noexcept {
  if (!deref_alignment_ok(sel)) return false;  // el unico campo leido es un int de 4 bytes: basta alineacion de 4 (ver COLLIDER_ROOT_CAUSE_AUDIT.md)
  type = reinterpret_cast<const cCcD_Obj*>(sel)->GetAtType();  // inline const
  return true;
}

void emit_hit(const HitReport& rep) noexcept {
  Observer& o = g.eng.obs;
  if (!o.may_log_hit(rep.j.kind)) return;
  char line[kLogLineMax * 2];
  if (fmt_hit_event(line, sizeof(line), rep) <= 0) return;
  emit(LogCat::Hit, rep.j.kind == HitKind::FinishUnexpected ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}

// POST05: lecturas auxiliares (solo getters const; mismos campos que setActionMode toca ademas del modo).
NativeAux read_aux(const daE_OC_c* a) noexcept {
  NativeAux x;
  x.valid = 1;
  x.t6c0 = a->field_0x6c0;
  x.t6c2 = a->field_0x6c2;
  x.head_lock = a->checkHeadLockFlg() ? 1 : 0;
  x.set_bits = static_cast<std::uint8_t>((a->mSphs_cc[0].cCcD_ObjHitInf::ChkTgSet() ? 1 : 0) | (a->mSphs_cc[0].cCcD_ObjHitInf::ChkCoSet() ? 2 : 0) |
                                         (a->mSphs_cc[1].cCcD_ObjHitInf::ChkTgSet() ? 4 : 0) | (a->mSphs_cc[1].cCcD_ObjHitInf::ChkCoSet() ? 8 : 0));
  // POST05-COMPAT: potencia y recurso en ESTE instante (getters const; lectura de mName acotada a 6 bytes). Nunca se escriben.
  const int a0 = a->mSphs_at[0].GetAtAtp(), a1 = a->mSphs_at[1].GetAtAtp();
  x.atp = static_cast<std::int8_t>(a0 == a1 ? (a0 > 100 ? 100 : a0) : -2);
  x.resource = static_cast<std::uint8_t>(classify_name(a->mName));
  // POST05-RB: fase nativa (getters const inline de J3DFrameCtrl; mpMorf vive mientras vive el actor y action() lo usa en esta misma llamada).
  x.speed = a->speedF;
  if (const mDoExt_McaMorfSO* m = a->mpMorf) {
    const J3DFrameCtrl& fc = m->mFrameCtrl;
    x.anim_ok = 1;
    x.anim_attr = fc.getAttribute();
    x.anim_state = fc.getState();
    x.anim_end = fc.getEnd();
    x.anim_rate = fc.getRate();
    x.anim_frame = fc.getFrame();
  }
  return x;
}

// POST05: el layout que leen ChkTgSet/ChkCoSet debe ser EXACTAMENTE el que escribe setActionMode en el EXE (orl $1 en +0xce8/+0xd08/+0xea0/+0xec0;
// mSPrm va tras el vptr). Solo aritmetica de direcciones de subobjetos: no lee ni escribe nada. Si no coincide, la supresion queda cerrada.
bool layout_matches_binary(daE_OC_c* a) noexcept {
  const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(a);
  const auto at = [&](const void* p) { return reinterpret_cast<std::uintptr_t>(p) - base + sizeof(void*); };
  // POST05-RB: la base mDoExt_morf_c de mpMorf esta en desplazamiento 0 (el EXE lee frame en [mpMorf+0x2c] directamente).
  const mDoExt_McaMorfSO* m = a->mpMorf;
  const bool morf_ok = !m || reinterpret_cast<std::uintptr_t>(&m->mFrameCtrl) - reinterpret_cast<std::uintptr_t>(m) == 0x18;
  return at(&a->mSphs_cc[0].GetObjTg()) == 0xce8 && at(&a->mSphs_cc[0].GetObjCo()) == 0xd08 && at(&a->mSphs_cc[1].GetObjTg()) == 0xea0 &&
         at(&a->mSphs_cc[1].GetObjCo()) == 0xec0 && at(&a->mSphs_at[0].GetObjAt()) == 0x1038 && at(&a->mSphs_at[1].GetObjAt()) == 0x11f0 && morf_ok;
}

// FIX01: only bit0 of the two OWN At mSPrm words (+1038/+11f0).
// Lease spans native cc_set only; restore before the global collision pass.
void fix01_at_mask(daE_OC_c* r,unsigned mask,bool on) noexcept {
 if(mask&1){if(on)r->mSphs_at[0].OnAtSetBit();else r->mSphs_at[0].OffAtSetBit();}
 if(mask&2){if(on)r->mSphs_at[1].OnAtSetBit();else r->mSphs_at[1].OffAtSetBit();}
}
void fix01_cleanup_request() noexcept {
 const auto& p=g.cur.pin;
 unsigned mask=(p.at_hit[0]&&p.at_on_link[0]?1u:0u)|(p.at_hit[1]&&p.at_on_link[1]?2u:0u);
 fx.cleanup.request(g.cur.slot,g.cur.id,g.cur.generation,g.p8.tick,mask,p.fresh&&p.tracked&&p.hp>1&&!p.event_running,p.mode==4&&(p.state==1||p.state==2));
}
HookAction on_cc_pre_fix01(ModContext*,void* args,void*,void*) noexcept {
 if(fx.cc_depth++){return HOOK_CONTINUE;}fx.cc_added=0;fx.cc_key=0;fx.cc_slot=-1;
 if(!args||g.eng.obs.disabled||g.reentered||g.rx.session_disabled()||g.layout!=1||!g.cur.tracked||g.cur.slot<0)return HOOK_CONTINUE;
 auto* a=mods::arg<daE_OC_c*>(args,0);
 if(!a||reinterpret_cast<std::uintptr_t>(a)!=g.cur.actor||a->profname!=fpcNm_E_OC_e||a->state.init_state==3||a->health<=1||fopAcM_GetID(a)!=g.cur.id||!g.eng.obs.ledger.valid(g.cur.slot,g.cur.generation))return HOOK_CONTINUE;
 const unsigned bits=(a->mSphs_at[0].ChkAtSet()?1u:0u)|(a->mSphs_at[1].ChkAtSet()?2u:0u);
 fx.cc_added=fx.cleanup.begin(g.cur.slot,g.cur.id,g.cur.generation,g.p8.tick,bits);
 if(fx.cc_added){fx.cc_key=reinterpret_cast<std::uintptr_t>(a);fx.cc_slot=g.cur.slot;fix01_at_mask(a,fx.cc_added,true);}
 return HOOK_CONTINUE;
}
void on_cc_post_fix01(ModContext*,void* args,void*,void*) noexcept {
 if(!fx.cc_depth||--fx.cc_depth||!fx.cc_added)return;
 auto* a=args?mods::arg<daE_OC_c*>(args,0):nullptr;
 if(!a||reinterpret_cast<std::uintptr_t>(a)!=fx.cc_key)return;
 // Always restore the paired live argument, including if another hook disabled us.
 fix01_at_mask(a,fx.cc_added,false);
 const unsigned bits=(a->mSphs_at[0].ChkAtSet()?1u:0u)|(a->mSphs_at[1].ChkAtSet()?2u:0u);
 fx.cleanup.end(fx.cc_slot,fx.cc_added,(bits&fx.cc_added)==0);fx.cc_added=0;fx.cc_key=0;
}

// POST06: segundo escritor, ACOTADO a la vida de ESTE actor y, solo en la rama letal nativa, attention_info.flags = 0 (lo mismo que damage_check
// hace cuando health <= 1). Nunca otro campo. Lo usan bonus y normalizacion Helm tras validar el contacto; se verifica despues.
void helm_set_cut(daPy_py_c* r, u8 cut) noexcept {
  if(cut==5||cut==8) r->mCutType=cut;
}
void helm_guard_duplicate(daE_OC_c* r) noexcept {
  r->field_0x6cc=1;
}
void write_vitality(daE_OC_c* r, int hp, bool lethal) noexcept {
  r->health = static_cast<s16>(hp);
  if (lethal) r->attention_info.flags = 0;
}

// LNK01: los DOS UNICOS escritores nuevos, ownership=HUD_VISUAL_ONLY. Nunca un actor, Link, una entrada ni un estado logico del HUD.
//  * action_ui_hide: J2DPane::hide() inline (mVisible = false, +0xb0, bool) sobre la RAIZ de la pantalla de botones del prompt de accion inferior
//    (dMeterButton_c::mpButtonScreen, +0x10). El juego nunca vuelve a mostrar esa raiz (solo muestra/oculta subpaneles), asi que basta con mantenerla.
//  * action_ui_slots: los dos slots de brillo (field_0x4be, +0x5d2, u8[2]) del MISMO prompt vivo, SOLO durante su draw: el PRE escribe BUTTON_NONE y
//    el POST restaura el valor original exacto (la logica de _execute nunca ve el valor enmascarado).
void action_ui_hide(J2DScreen* r) noexcept { r->hide(); }
void action_ui_slots(dMeterButton_c* r, std::uint8_t s0, std::uint8_t s1) noexcept {
  r->field_0x4be[0] = s0;
  r->field_0x4be[1] = s1;
}

// POST05-GENERAL: lectura de TODO lo que setActionMode (+ offTgSph en BIG_DAMAGE) escribe (mismos getters const que read_aux).
post05::LiveMode read_live(const daE_OC_c* r) noexcept {
  post05::LiveMode m;
  m.mode = r->mActionMode;
  m.state = r->mOcState;
  m.old_mode = r->mOldActionMode;
  m.t6c0 = r->field_0x6c0;
  m.t6c2 = r->field_0x6c2;
  m.head_lock = r->checkHeadLockFlg() ? 1 : 0;
  m.set_bits = static_cast<std::uint8_t>((r->mSphs_cc[0].cCcD_ObjHitInf::ChkTgSet() ? 1 : 0) | (r->mSphs_cc[0].cCcD_ObjHitInf::ChkCoSet() ? 2 : 0) |
                                         (r->mSphs_cc[1].cCcD_ObjHitInf::ChkTgSet() ? 4 : 0) | (r->mSphs_cc[1].cCcD_ObjHitInf::ChkCoSet() ? 8 : 0));
  return m;
}

// Escritor acotado de reaccion: deshace exactamente la reaccion final de damage_check (setActionMode(DAMAGE|BIG_DAMAGE) y, en
// BIG_DAMAGE, offTgSph). Escribe SOLO los campos que esas dos funciones tocan (binario 0x140e0d260: +0x8cc/+0x8c8/+0x8d0, u32 +0x8e0, and $0xff7f
// en +0x69a, orl $1 en +0xce8/+0xd08/+0xea0/+0xec0; offTgSph: Tg ClrSet), con los MISMOS setters inline que usa el juego, hacia el valor PRE exacto
// (RESTORE_PRE) o hacia el resultado de setActionMode(FIND, 1) (NOTICE_FIND1, precedente nativo del tirachinas). Nunca HP, cooldown, colisiones de ataque,
// animacion, velocidad, ni el bit de golpe (ClrTgHit no se deshace: el golpe ya se proceso).
void restore_reaction(daE_OC_c* r, const post05::LiveMode& t) noexcept {
  r->mActionMode = t.mode;
  r->mOcState = t.state;
  r->mOldActionMode = t.old_mode;
  r->field_0x6c0 = t.t6c0;
  r->field_0x6c2 = t.t6c2;
  if (t.head_lock) r->onHeadLockFlg();
  else r->offHeadLockFlg();
  if (t.set_bits & 1) r->mSphs_cc[0].OnTgSetBit();
  else r->mSphs_cc[0].OffTgSetBit();
  if (t.set_bits & 2) r->mSphs_cc[0].OnCoSetBit();
  else r->mSphs_cc[0].OffCoSetBit();
  if (t.set_bits & 4) r->mSphs_cc[1].OnTgSetBit();
  else r->mSphs_cc[1].OffTgSetBit();
  if (t.set_bits & 8) r->mSphs_cc[1].OnCoSetBit();
  else r->mSphs_cc[1].OffCoSetBit();
}

void emit_decision(const HitReport& rep, const post05::DecisionResult& d, post05::Apply ap) noexcept {
  if (!d.counted || rep.j.kind != HitKind::Confirmed) return;  // Finish/no confirmados: solo contadores
  if (!g.rx.log_take()) return;
  char line[kLogLineMax];
  if (post05::fmt_decision(line, sizeof(line), rep, g.pe.last, d, ap) <= 0) return;
  emit(LogCat::Hit, ap == post05::Apply::VerifyFailed ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}

// ---- POST06 ----
// Bonus x1.50: relee vida y modo; escribe la vida (y la rama letal nativa si toca); verifica. Ninguna otra escritura.
bool post06_bonus(daE_OC_c* a, const post06::View& v, const post06::Action& act) noexcept {
  const post05::LiveMode now = read_live(a);
  if (a->health != v.native_hp || now.mode != v.mode || now.state != v.state) return false;
  write_vitality(a, act.new_hp, act.lethal);
  if (act.convert_to_big) {  // = setActionMode(BIG_DAMAGE, 0) tras la setActionMode(DAMAGE) de ESTA llamada: el resto de campos ya son los suyos
    post05::LiveMode t = now;
    t.mode = post06::policy::kModeBig;
    t.state = 0;
    restore_reaction(a, t);
    if (!post05::same_live(read_live(a), t)) return false;
  }
  return a->health == act.new_hp && (!act.lethal || a->attention_info.flags == 0);
}
// Only an explicitly selected decisive contact can request BIG_DAMAGE.
bool post06_stun(daE_OC_c* a, bool) noexcept {
  const post05::LiveMode now=read_live(a);
  if (!(post06::policy::drop_mode_safe(now.mode)||now.mode==6)||a->health<=1) return false;
  const post05::LiveMode t{5,3,now.mode,0,0,0,post03::kAllSetBits};
  restore_reaction(a,t);return post05::same_live(read_live(a),t);
}
bool post06_recover(daE_OC_c* a) noexcept {
  const auto now=read_live(a);if (!post06::policy::drop_mode_safe(now.mode)||a->health<=1) return false;
  // A hit at the deadline retains its own native reaction. No timeout can write BIG_DAMAGE.
  if (now.mode!=5) return true;
  const post05::LiveMode t{3,0,now.mode,0,0,0,post03::kAllSetBits};
  restore_reaction(a,t);return post05::same_live(read_live(a),t);
}
void on_stun_post(ModContext*,void* args,void*,void*) noexcept {
  if(!g.br.enabled||!args||g.eng.obs.disabled||g.rx.session_disabled())return;
  auto* a=mods::arg<daE_OC_c*>(args,0);
  if(!a||g.cur.actor!=reinterpret_cast<std::uintptr_t>(a)||!g.cur.tracked||g.cur.slot<0)return;
  if(!g.eng.obs.ledger.valid(g.cur.slot,g.cur.generation))return;
  const auto& state=g.br.slot(static_cast<unsigned>(g.cur.slot));
  if(state.id!=g.cur.id||state.generation!=g.cur.generation||state.phase!=post06::Phase::Punish||state.shield_prepared)return;
  auto t=read_live(a);if(t.mode==5&&t.head_lock){t.head_lock=0;restore_reaction(a,t);if(!post05::same_live(read_live(a),t))g.rx.close_session();}
}
void post08_watch_out(const post08::AiWatch::Out& o) noexcept;  // FIX02 (definido con POST08)
void post06_step(daE_OC_c* a, const PostReads& r, bool has_event, const HitReport& rep, bool parry_crossed, bool parry_break_pending) noexcept {
  if (!g.br.enabled || g.cur.actor != reinterpret_cast<std::uintptr_t>(a) || !g.cur.tracked || g.cur.slot < 0) return;
  if (!g.eng.obs.ledger.valid(g.cur.slot, g.cur.generation) || g.eng.obs.ledger.slot(static_cast<unsigned>(g.cur.slot)).id != g.cur.id) return;
  const post04::Entry& e = g.pe.px.entry(static_cast<unsigned>(g.cur.slot));
  if (!e.bound || e.generation != g.cur.generation || e.id != g.cur.id || e.frozen) return;  // muerte incluida: POST06 la ve y cierra el ciclo sin escribir
  const post05::LiveMode now = read_live(a);
  post06::View v;
  v.slot = g.cur.slot;
  v.generation = g.cur.generation;
  v.id = g.cur.id;
  v.tracked = true;
  v.pre_hp = g.cur.pre_hp;
  v.native_hp = r.snap.health;
  v.pre_mode = g.cur.pre_mode;
  v.mode = now.mode;
  v.state = now.state;
  v.hit = has_event && rep.j.kind == HitKind::Confirmed && rep.slot == g.cur.slot && rep.generation == g.cur.generation;
  v.sword = v.hit && rep.at_valid && rep.fam.family == Family::Sword;
  v.shield = v.hit && rep.at_valid && rep.fam.family == Family::ShieldAttack;
  v.from_link = g.cur.from_link;
  v.helm = g.cur.helm;
  v.stun_ended = !v.hit && (now.mode == 3 || now.mode == 16);
  v.parry_crossed = parry_crossed;
  v.parry_break_pending = parry_break_pending;
  v.crossed = v.hit && g.pe.last.event == rep.event && g.pe.last.crossed;
  v.latched = e.latched;
  v.writer_ok = g.layout == 1 && !g.rx.session_disabled();
  const post06::Phase before = g.br.slot(static_cast<unsigned>(v.slot)).phase;
  const post06::Action act = g.br.step(v);
  bool ok = true;
  if (act.act == post06::Act::Bonus) ok = post06_bonus(a, v, act);
  else if (act.act == post06::Act::Stun) ok = post06_stun(a, act.why == post06::Why::ShieldPrepared);
  else if (act.act == post06::Act::Recover) ok = post06_recover(a);
  if (ok && (act.act == post06::Act::Rearm || act.act == post06::Act::Recover)) ok = g.pe.px.rearm(v.slot, v.generation, v.id, act.rearm_to);
  if (act.act != post06::Act::None) g.br.commit(v, act, ok);
  if (ok && act.act == post06::Act::Stun)  // FIX02: vigilar la IA tras cada ruptura (solo lectura)
    post08_watch_out(g.watch.start(v.slot, v.id, v.generation, g.p8.tick, post08::AiWatch::Cause::Break, r.aux.resource == static_cast<std::uint8_t>(NameKind::E_OC2)));
  if(ok&&act.act==post06::Act::Stun){fx.ai.intervention(v.slot,v.id,v.generation,g.p8.tick,false,false);fix01_cleanup_request();}
  if (!ok) g.rx.close_session();
  g.pe.px.set_regen_rate(v.slot, v.generation, v.id, g.br.regen_rate(v.slot, a->health));
  const post06::Breaker::Slot& s = g.br.slot(static_cast<unsigned>(v.slot));
  if ((act.act != post06::Act::None || s.phase != before) && g.br.log_take()) {
    char line[kLogLineMax];
    if (post06::fmt_event(line, sizeof(line), v, s, act, before, ok) > 0) emit(LogCat::Hit, ok ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
  }
}

// POST08: unico escritor del desvio (tercer uso acotado de restore_reaction): ATTACK estado 3 a partir del PRE de esta llamada.
void post08_deflect(daE_OC_c* a, const post05::LiveMode& t) noexcept { restore_reaction(a, t); }

// ======================================== POST08: parry de precision con Shield Attack ===================================================
// LECTURA (solo PRE, solo getters const/inline y comparaciones de direccion; nada se desreferencia mas alla del propio actor y de la Link viva):
//   enemigo: mSphs_at[0|1] -> cCcD_ObjHitInf::ChkAtHit / GetAtHitObj (crudos) y dCcD_GObjInf::ChkAtShieldHit (marca nativa de bloqueo con escudo);
//   Link viva (dComIfGp_getLinkPlayer de ESTA llamada): mProcID, mUnderFrameCtrl[0].getFrame(), field_0x3478/0x347c (frames del Shield Attack) y las
//   DIRECCIONES de mTgCyls[0..2] (solo para comparar con el objeto golpeado).
// ESCRITURA (solo POST, solo con veredicto CONFIRMED y desvio no rompedor): restore_reaction() del estado PRE con mOcState = 3 (golpe rechazado nativo de
//   executeAttack). Nunca vida, cooldown, animacion, velocidad, colisiones, Link ni Expansion. La ruptura a 100 la escribe POST06 (BREAK_BY_PARRY).
bool parry_layout_ok(daE_OC_c* a) noexcept {
  const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(a);
  const auto at = [&](const void* p) { return reinterpret_cast<std::uintptr_t>(p) - base + sizeof(void*); };
  // EXE executeAttack 0x140e073f8/0x140e073ff: orl $1,0x1038 / 0x11f0 (mObjAt.mSPrm de las esferas At 0 y 1).
  return at(&a->mSphs_at[0].GetObjAt()) == 0x1038 && at(&a->mSphs_at[1].GetObjAt()) == 0x11f0;
}

void post08_judge(daE_OC_c* a, const PreReads& r, daPy_py_c* lp) noexcept {
  g.cur.pv = post08::Verdict{};
  g.cur.pin = post08::Input{};
  if (!g.parry.enabled) return;
  if (g.p8.layout < 0) g.p8.layout = parry_layout_ok(a) ? 1 : 0;  // una vez: si no coincide con el EXE, parry OFF para la sesion
  if (g.p8.layout != 1) return;
  post08::Input& in = g.cur.pin;
  for (int i = 0; i < 2; ++i) {
    in.at_hit[i] = a->mSphs_at[i].cCcD_ObjHitInf::ChkAtHit() != 0;
    if (!in.at_hit[i]) continue;
    in.at_shield[i] = a->mSphs_at[i].ChkAtShieldHit();
    const std::uintptr_t hit = reinterpret_cast<std::uintptr_t>(a->mSphs_at[i].cCcD_ObjHitInf::GetAtHitObj());
    if (lp && lp->profname == fpcNm_ALINK_e && lp->state.init_state != 3) {
      auto* link = static_cast<daAlink_c*>(lp);
      for (unsigned k = 0; k < 3; ++k)
        if (hit && hit == reinterpret_cast<std::uintptr_t>(static_cast<cCcD_Obj*>(&link->mTgCyls[k]))) in.at_on_link[i] = true;
    }
  }
  if (!post08::Parry::any_contact(in) && !g.cur.tracked) return;  // ni contacto ni identidad: nada que juzgar
  in.tracked = g.cur.tracked && g.cur.slot >= 0 && g.eng.obs.ledger.valid(g.cur.slot, g.cur.generation) &&
               g.eng.obs.ledger.slot(static_cast<unsigned>(g.cur.slot)).id == g.cur.id;
  in.slot = g.cur.slot;
  in.generation = g.cur.generation;
  in.id = g.cur.id;
  if (in.tracked) {
    const post04::Entry& e = g.pe.px.entry(static_cast<unsigned>(g.cur.slot));
    if (!e.bound || e.generation != g.cur.generation || e.id != g.cur.id) in.tracked = false;
    in.latched = e.latched;
    in.frozen = e.frozen;
    in.dead = e.dead;
  }
  in.hp = r.snap.health;
  in.mode = r.snap.mode;
  in.state = r.snap.state;
  in.anim_ok = r.aux.anim_ok != 0;
  in.anim_frame = r.aux.anim_frame;
  in.anim_rate = r.aux.anim_rate;
  in.e_oc2 = r.aux.resource == static_cast<std::uint8_t>(NameKind::E_OC2);
  in.atp = r.aux.atp;
  std::uint32_t link_id = 0;
  if (lp && lp->profname == fpcNm_ALINK_e && lp->state.init_state != 3) {
    auto* link = static_cast<daAlink_c*>(lp);
    in.link_ok = true;
    link_id = static_cast<std::uint32_t>(fopAcM_GetID(lp));
    in.now.valid = true;  // AHORA (solo telemetria: puede haber cambiado ya por el bloqueo, p. ej. GUARD_SLIP)
    in.now.proc = static_cast<int>(link->mProcID);
    in.now.guard_attack = link->mProcID == daAlink_c::PROC_GUARD_ATTACK;
    in.now.frame = link->mUnderFrameCtrl[0].getFrame();
    in.now.start = link->field_0x3478;
    in.now.end = link->field_0x347c;
  }
  in.pass = g.link_track.pass_for(g.p8.tick, in.link_ok, link_id);  // FIX01: Link DURANTE la pasada que produjo estas marcas
  in.fresh = in.tracked && g.arm.fresh(g.cur.slot, g.cur.id, g.cur.generation, g.p8.tick);  // FIX02: registro At de la pasada anterior y armado
  in.event_running = g_dComIfG_gameInfo.play.getEvent()->runCheck() != 0;
  g.cur.pv = g.parry.judge(in);
}

// FIX01: escritor PROPIO de POST08 (cada llamador tiene su propio tope), independiente del presupuesto compartido del observador para que una sesion
// larga con muchos golpes no deje sin diagnostico a POST08 (0.8.0 escribia por la categoria Hit compartida).
void p8_write(LogLevel level, const char* line) noexcept {
  if (!svc_log || !svc_log->write) return;
  try {
    svc_log->write(mod_ctx, level, line);
  } catch (...) {
    ++g.eng.obs.ledger.c.log_errors;
  }
}
// FIX01: telemetria de cada contacto (contadores siempre; lineas acotadas en post08_contact_log).
void post08_contact_telemetry(const post08::Verdict& v, const post08::Input& in) noexcept {
  if (v.transitioned) ++g.p8.transitioned;
  if (v.blocked && in.pass.valid && in.pass.guard_attack) {
    ++g.p8.blocked_in_sa;
    g.hist.add(v.delta);
  }
  g.link_track.note(in.pass.serial, true, v.blocked, false);
}
void post08_log(const post08::Verdict& v, int pre_hp, int post_hp, int pb, int pa, bool crossed, bool suppressed, bool deflected, bool ok, const char* stage) noexcept {
  if (!g.parry.log_take()) return;
  char line[kLogLineMax * 2];
  const int life = static_cast<int>(g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife());
  if (post08::fmt_event(line, sizeof(line), g.cur.pin, v, life, pre_hp, post_hp, pb, pa, crossed, suppressed, deflected, ok, stage) > 0)
    p8_write(ok ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
}

void post08_contact_log(const post08::Verdict& v, daE_OC_c* a, const PostReads& r, const char* stage) noexcept {
  if (g.p8.contact_lines >= 250) {
    ++g.p8.contact_lines_dropped;
    return;
  }
  ++g.p8.contact_lines;
  char line[kLogLineMax * 2];
  const int life = static_cast<int>(g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife());
  const int hp = r.alive ? a->health : g.cur.pre_hp;
  if (post08::fmt_event(line, sizeof(line), g.cur.pin, v, life, g.cur.pre_hp, hp, -1, -1, false, false, false, true, stage) > 0) p8_write(LOG_LEVEL_INFO, line);
}

// FIX02: lineas del observador de IA (solo lectura; tope propio).
void post08_watch_out(const post08::AiWatch::Out& o) noexcept {
  if (o.ev == post08::AiWatch::Event::None || !o.w) return;
  if (g.p8.watch_lines >= 220) {
    ++g.p8.watch_lines_dropped;
    return;
  }
  ++g.p8.watch_lines;
  const post08::AiWatch::W& w = *o.w;
  const char* ev = o.ev == post08::AiWatch::Event::Started ? "START" : (o.ev == post08::AiWatch::Event::Suspected ? "SUSPECTED" : "END");
  char ring[200];
  std::size_t k = 0;
  ring[0] = 0;
  const unsigned n = w.ring_n < post08::AiWatch::kRing ? w.ring_n : post08::AiWatch::kRing;
  for (unsigned i = 0; i < n && k + 32 < sizeof(ring); ++i) {
    const post08::AiWatch::Sample& x = w.ring[(w.ring_n - n + i) % post08::AiWatch::kRing];
    const int m = std::snprintf(ring + k, sizeof(ring) - k, "%s%d/%d@%.1f", i ? "," : "", x.mode, x.state, x.frame);
    if (m > 0) k += static_cast<std::size_t>(m);
  }
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line),
                "POST08_AI_TRACE event=%s verdict=%s cause=%s actor_id=%u generation=%llu e_oc2=%d ticks=%llu samples=%u last=%d/%d frame=%.2f rate=%.2f "
                "t6c0=%d t6c2=%d speed=%.2f swings_started=%u swings_armed=%u swings_aborted=%u frozen_run=%u max_frozen=%u mode_changes=%u recent={%s}",
                ev, o.verdict && *o.verdict ? o.verdict : "-", post08::AiWatch::name(w.cause), w.id, static_cast<unsigned long long>(w.generation),
                w.e_oc2 ? 1 : 0, static_cast<unsigned long long>(w.last_tick - w.start), w.samples, w.last.mode, w.last.state, w.last.frame, w.last.rate,
                w.last.t6c0, w.last.t6c2, w.last.speed, w.swings_started, w.swings_armed, w.swings_aborted, w.frozen_run, w.max_frozen, w.mode_changes, ring);
  p8_write(o.ev == post08::AiWatch::Event::Suspected ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}
void post08_watch_feed(const PreReads& r) noexcept {
  if (!g.cur.tracked || g.cur.slot < 0 || !g.watch.active()) return;
  post08::AiWatch::Sample x;
  x.mode = static_cast<std::int16_t>(r.snap.mode), x.state = static_cast<std::int16_t>(r.snap.state);
  x.t6c0 = r.aux.t6c0, x.t6c2 = r.aux.t6c2, x.frame = r.aux.anim_frame, x.rate = r.aux.anim_rate, x.speed = r.aux.speed;
  const bool dead = r.snap.health <= 0 || post06::policy::death_mode(r.snap.mode);
  const bool ko = r.snap.mode == post06::policy::kModeDamage || r.snap.mode == post06::policy::kModeBig;
  post08_watch_out(g.watch.feed(g.cur.slot, g.cur.id, g.cur.generation, g.p8.tick, x, dead, ko));
}

// Devuelve true si un parry confirmado cruzo 100 en esta llamada (handoff a POST06: BREAK_BY_PARRY).
bool post08_step(daE_OC_c* a, const PostReads& r) noexcept {
  const post08::Verdict v = g.cur.pv;
  if (v.result == post08::Result::None) return false;
  const post08::Input& in = g.cur.pin;
  post08_contact_telemetry(v, in);
  if (v.result != post08::Result::Confirmed) {
    if (v.first_contact || (v.blocked && in.pass.guard_attack)) post08_contact_log(v, a, r, "REJECTED");
    return false;
  }
  ++g.p8.attempts;
  bool ok = false, crossed = false, deflected = false, suppressed = false;
  int pb = -1, pa = -1;
  const char* stage = "COMMITTED";
  const post03::Instance& L = g.eng.obs.ledger.slot(static_cast<unsigned>(in.slot < 0 ? 0 : in.slot));
  const post05::LiveMode now = read_live(a);
  const post08::LiveShape pre_s{g.cur.pre_live.mode, g.cur.pre_live.state, g.cur.pre_live.old_mode}, now_s{now.mode, now.state, now.old_mode};
  if (!r.alive || g.reentered || g.layout != 1 || g.rx.session_disabled()) stage = "WRITER_CLOSED";
  else if (!g.eng.obs.ledger.valid(in.slot, in.generation) || L.id != in.id) stage = "IDENTITY_LOST";
  else if (a->health != g.cur.pre_hp || a->health <= 1) stage = "HP_CHANGED_IN_CALL";
  else if(g.cur.continuation!=lnks1fix::Continuation::Safe)stage="UNSAFE_NATIVE_CONTINUATION";
  else {
    const post08::DeflectPlan plan = post08::deflect_plan(pre_s, now_s, post08::link_protected_now(in.now));
    const post04::Posture::ParryOutcome pv = g.pe.px.preview_parry(in.slot, in.generation, in.id, v.token, g.eng.obs.ledger);
    if (plan == post08::DeflectPlan::Refuse) stage = "REACTION_UNEXPECTED";
    else if (pv.reject != post04::Posture::ParryReject::None) stage = "POSTURE_REJECTED";
    else {
      suppressed = now.mode == 5;  // la reaccion nativa del Shield Attack (DAMAGE 3) estaba escrita: se deshace
      if (plan == post08::DeflectPlan::NativeContinues) {
        deflected = true;  // FIX02: sin escritura; executeAttack arma la esfera y rebota nativamente (Link en GUARD_SLIP) o el golpe sigue bloqueado
        stage = "COMMITTED_NATIVE";
      } else {
        post05::LiveMode t = g.cur.pre_live;  // RestorePre: el PRE exacto; ForceRecoil: el PRE con el estado de golpe rechazado
        if (plan == post08::DeflectPlan::ForceRecoil) t.state = post08::kDeflectState;
        post08_deflect(a, t);
        deflected = post05::same_live(read_live(a), t);
        stage = plan == post08::DeflectPlan::ForceRecoil ? "COMMITTED_FORCE_RECOIL" : "COMMITTED_RESTORE_PRE";
        if (!deflected) {
          stage = "DEFLECT_VERIFY_FAILED";
          g.rx.close_session();
        }
      }
      if (deflected) {
        const post04::Posture::ParryOutcome out = g.pe.px.apply_parry(in.slot, in.generation, in.id, v.token, g.eng.obs.ledger);
        ok = out.applied && out.after == pv.after && out.crossed == pv.crossed;
        crossed = out.crossed;
        pb = out.before;
        pa = out.after;
        if (!ok) stage = "POSTURE_APPLY_FAILED";
        if (ok) ++g.p8.plan[static_cast<unsigned>(plan) % 3];  // [0]=native [1]=restore [2]=force
        if (ok && crossed) {  // FIX02: la ruptura (mareo de pie de POST06) se entrega en la SIGUIENTE llamada de este actor
          ++g.p8.plan[3];
          ++g.p8.breaks_deferred;
          if (in.slot >= 0 && static_cast<unsigned>(in.slot) < post08::kSlots) g.p8.pending_break[in.slot] = Runtime::P8::PendingBreak{true, in.slot, in.id, in.generation, g.p8.tick};
        }
      }
    }
  }
  if(ok)fix01_cleanup_request();
  g.parry.commit(in.slot, in.generation, in.id, v.token, ok);
  g.link_track.note(in.pass.serial, false, false, ok);
  if (ok) {
    ++g.p8.committed;
    g.p8.units += static_cast<std::uint64_t>(pa - pb);
    if (crossed) ++g.p8.breaks;
    if (suppressed && !crossed) ++g.p8.stun_suppressed;
    if (a->health != g.cur.pre_hp) ++g.p8.unexpected_hp;
    g.p8.watch_life = true;  // lectura diagnostica de la vida de Link en el siguiente mod_update
    post08_watch_out(g.watch.start(in.slot, in.id, in.generation, g.p8.tick, post08::AiWatch::Cause::Parry, in.e_oc2));
    fx.ai.intervention(in.slot,in.id,in.generation,g.p8.tick,false,false);
    g.p8.life_at = static_cast<int>(g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife());
  } else {
    ++g.p8.failed;
  }
  post08_log(v, g.cur.pre_hp, r.alive ? a->health : g.cur.pre_hp, pb, pa, crossed, suppressed && !crossed, deflected, ok, stage);
  return false;  // FIX02: nunca se entrega la ruptura en la llamada del contacto (ver post08_break_handoff)
}

// FIX02: entrega de la ruptura aplazada. 0 = nada; 1 = esta llamada es la del contacto (POST06 espera, sin rearme); 2 = entregar BREAK_BY_PARRY ahora.
int post08_break_handoff() noexcept {
  if (g.cur.slot < 0 || static_cast<unsigned>(g.cur.slot) >= post08::kSlots) return 0;
  Runtime::P8::PendingBreak& p = g.p8.pending_break[g.cur.slot];
  if (!p.on || g.cur.slot != p.slot || g.cur.id != p.id || g.cur.generation != p.generation) return 0;
  if (g.p8.tick == p.tick) return 1;
  p.on = false;
  if (g.p8.tick > p.tick + 3) {  // el actor no volvio a ejecutarse a tiempo: POST06 hara su recuperacion segura a 50
    ++g.p8.breaks_expired;
    return 0;
  }
  ++g.p8.breaks_delivered;
  return 2;
}

// FIX01: UNA observacion de Link por mod_update. ModLoader::tick corre al inicio de fapGm_Execute, antes de fpcM_Management: el estado leido aqui es el de
// Link tras su execute del tick anterior = el estado durante la pasada de colision de ese tick (dScnPly_Draw -> Ccsp()->Move()). Solo lectura.
void post08_observe_link() noexcept {
  ++g.p8.tick;
  auto* lp = dComIfGp_getLinkPlayer();
  const bool valid = lp && lp->profname == fpcNm_ALINK_e && lp->state.init_state != 3;
  std::uint32_t id = 0;
  int proc = -1;
  bool ga = false;
  float frame = 0, start = 0, end = 0;
  if (valid) {
    auto* link = static_cast<daAlink_c*>(lp);
    id = static_cast<std::uint32_t>(fopAcM_GetID(lp));
    proc = static_cast<int>(link->mProcID);
    ga = link->mProcID == daAlink_c::PROC_GUARD_ATTACK;
    frame = link->mUnderFrameCtrl[0].getFrame();
    start = link->field_0x3478;
    end = link->field_0x347c;
  }
  const post08::LinkTrack::Transition t = g.link_track.observe(g.p8.tick, valid, id, proc, ga, frame, start, end);
  if (!t.started && !t.ended) return;
  if (g.p8.sa_lines >= 160) {
    ++g.p8.sa_lines_dropped;
    return;
  }
  ++g.p8.sa_lines;
  char line[kLogLineMax];
  if (t.started)
    std::snprintf(line, sizeof(line), "POST08_SHIELD_ATTACK_START sa_serial=%llu tick=%llu frame=%.2f atk_start=%.2f atk_end=%.2f window=[-%.0f,+%.0f]f/%ut",
                  static_cast<unsigned long long>(t.serial), static_cast<unsigned long long>(g.p8.tick), frame, start, end, post08::policy::kPreStartSlack,
                  post08::policy::kWindowFrames, post08::policy::kWindowTicksCap);
  else
    std::snprintf(line, sizeof(line), "POST08_SHIELD_ATTACK_END sa_serial=%llu tick=%llu duration_ticks=%u last_frame=%.2f exit_proc=%d contacts=%u blocks=%u parries=%u",
                  static_cast<unsigned long long>(t.serial), static_cast<unsigned long long>(g.p8.tick), t.duration, t.last_frame, t.exit_proc, t.contacts, t.blocks,
                  t.parries);
  p8_write(LOG_LEVEL_INFO, line);
}

void post08_update() noexcept {
  if (!g.p8.watch_life) return;
  g.p8.watch_life = false;
  const int life = static_cast<int>(g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife());
  if (life < g.p8.life_at) ++g.p8.life_drop_next_tick;  // DIAGNOSTICO: cualquier fuente (otro golpe, chip, caida...)
  if (g.p8.life_lines < 24) {
    ++g.p8.life_lines;
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "POST08_LINK_LIFE_CHECK life_at_parry=%d life_next_tick=%d delta=%d evidence=DIAGNOSTIC_ANY_SOURCE", g.p8.life_at, life,
                  life - g.p8.life_at);
    p8_write(life < g.p8.life_at ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
  }
}

void emit_parry_summary(const char* tag) noexcept {
  static unsigned lines = 0;  // tope propio (PERIODIC + SHUTDOWN); el SHUTDOWN siempre se escribe
  const char* k = "SHUTDOWN";  // comparacion manual: sin import nuevo del CRT (strcmp)
  unsigned i = 0;
  while (tag && tag[i] && k[i] && tag[i] == k[i]) ++i;
  const bool shutdown = tag && tag[i] == 0 && k[i] == 0;
  if (lines >= 60 && !shutdown) return;
  ++lines;
  const post08::PCounters& c = g.parry.c;
  const post08::LinkTrack::Counters& lt = g.link_track.c;
  const auto u = [](std::uint64_t x) { return static_cast<unsigned long long>(x); };
  const auto R = [&](post08::Result r) { return u(c.result[static_cast<unsigned>(r)]); };
  const post04::AuditResult au = g.pe.px.audit();
  const bool inv = au.ok && g.p8.committed == g.pe.px.c.parries && g.p8.units == g.pe.px.c.parry_units && g.p8.breaks == g.pe.px.c.parry_crossings;
  char line[kLogLineMax * 4];
  // 1/3: acciones de Link y contactos
  std::snprintf(line, sizeof(line),
                "POST08_SUMMARY v=%s tag=%s part=1/4 parry=%s layout=%s window=[-%.0f,+%.0f]f/%ut ticks=%llu shield_attacks_started=%llu shield_attacks_ended=%llu "
                "sa_with_enemy_contact=%llu sa_with_native_block=%llu sa_with_parry=%llu link_changes=%llu judged=%llu episodes=%llu contacts_verified=%llu "
                "blocked_during_shield_attack=%llu transitioned_after_block=%llu",
                post08::kParryVersion, tag, g.parry.enabled ? "ON" : "OFF", g.p8.layout < 0 ? "UNCHECKED" : (g.p8.layout ? "MATCHES_EXE" : "MISMATCH_PARRY_OFF"),
                post08::policy::kPreStartSlack, post08::policy::kWindowFrames, post08::policy::kWindowTicksCap, u(g.p8.tick), u(lt.starts), u(lt.ends),
                u(lt.with_contact), u(lt.with_block), u(lt.with_parry), u(lt.link_changes), u(c.judged), u(c.episodes), u(c.contacts), u(g.p8.blocked_in_sa),
                u(g.p8.transitioned));
  p8_write(LOG_LEVEL_INFO, line);
  // 2/3: resultado principal por contacto (cada contacto tiene exactamente uno)
  std::snprintf(line, sizeof(line),
                "POST08_SUMMARY v=%s tag=%s part=2/4 candidates=%llu normal_guard=%llu not_blocked=%llu missed_early=%llu missed_late=%llu duplicate=%llu "
                "latched=%llu fury_chip_refused=%llu not_attacking=%llu untracked=%llu link_invalid=%llu no_pass_snapshot=%llu event_running=%llu",
                post08::kParryVersion, tag, u(c.confirmed), R(post08::Result::NormalGuard), R(post08::Result::NotBlocked), R(post08::Result::MissedEarly),
                R(post08::Result::MissedLate), R(post08::Result::Duplicate), R(post08::Result::Latched), R(post08::Result::FuryChipPossible),
                R(post08::Result::NotAttacking), R(post08::Result::Untracked), R(post08::Result::LinkInvalid), R(post08::Result::NoPassSnapshot),
                R(post08::Result::EventRunning));
  p8_write(LOG_LEVEL_INFO, line);
  // 3/3: commits, postura, seguridad, histograma de timing
  char hist[256];
  g.hist.fmt(hist, sizeof(hist));
  std::snprintf(line, sizeof(line),
                "POST08_SUMMARY v=%s tag=%s part=3/4 commit_attempts=%llu parries_committed=%llu posture_units=%llu breaks_by_parry=%llu stun_suppressed=%llu "
                "unexpected_hp_delta=%llu write_failed=%llu link_life_drop_next_tick=%llu px_parries=%llu px_parry_units=%llu px_parry_crossings=%llu "
                "posture_audit_ok=%d e6_parry=%d invariant_ok=%d lines(contact/sa)=%llu+%llu_dropped/%llu+%llu_dropped timing_delta_hist_frames={%s}",
                post08::kParryVersion, tag, u(g.p8.attempts), u(g.p8.committed), u(g.p8.units), u(g.p8.breaks), u(g.p8.stun_suppressed), u(g.p8.unexpected_hp),
                u(g.p8.failed), u(g.p8.life_drop_next_tick), u(g.pe.px.c.parries), u(g.pe.px.c.parry_units), u(g.pe.px.c.parry_crossings), au.ok ? 1 : 0,
                au.e6 ? 1 : 0, inv ? 1 : 0, u(g.p8.contact_lines), u(g.p8.contact_lines_dropped), u(g.p8.sa_lines), u(g.p8.sa_lines_dropped), hist);
  p8_write(au.ok && inv ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
  // 4/4 (FIX02): frescura del registro At, plan de desvio y observador de IA
  const post08::AiWatch::Counters& wc = g.watch.c;
  std::snprintf(line, sizeof(line),
                "POST08_SUMMARY v=%s tag=%s part=4/4 stale_contacts_ignored=%llu arm_notes=%llu arm_armed=%llu deflect(native/restore_pre/force_recoil/break)=%llu/%llu/%llu/%llu "
                "ai_watch_started=%llu restarted=%llu ended=%llu by_cause(parry/suppression/break)=%llu/%llu/%llu ai_stall_suspected=%llu "
                "suspected_by_cause=%llu/%llu/%llu suspected_e_oc2=%llu resolved_natively=%llu still_suspected=%llu observed_next_attack=%llu dropped_full=%llu "
                "trace_lines=%llu+%llu_dropped breaks_by_parry(deferred/delivered/expired)=%llu/%llu/%llu",
                post08::kParryVersion, tag, u(c.stale), u(g.arm.c.noted), u(g.arm.c.armed), u(g.p8.plan[0]), u(g.p8.plan[1]), u(g.p8.plan[2]), u(g.p8.plan[3]),
                u(wc.started), u(wc.restarted), u(wc.ended), u(wc.by_cause[0]), u(wc.by_cause[1]), u(wc.by_cause[2]), u(wc.suspected), u(wc.suspected_by_cause[0]),
                u(wc.suspected_by_cause[1]), u(wc.suspected_by_cause[2]), u(wc.suspected_e_oc2), u(wc.resolved_natively), u(wc.still_suspected),
                u(wc.observed_next_attack), u(wc.dropped_full), u(g.p8.watch_lines), u(g.p8.watch_lines_dropped), u(g.p8.breaks_deferred), u(g.p8.breaks_delivered),
                u(g.p8.breaks_expired));
  p8_write(wc.suspected ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}

// ======================================== LNK01: postura de Link + barra + prompts inferiores ocultos ===================================
// Escritor de log PROPIO (topes por tipo), independiente del presupuesto compartido del observador y del de POST08.
void lnk_write(LogLevel level, const char* line) noexcept {
  if (!svc_log || !svc_log->write) return;
  try {
    svc_log->write(mod_ctx, level, line);
  } catch (...) {
    ++g.eng.obs.ledger.c.log_errors;
  }
}
bool lnk_take(unsigned& lines, std::uint64_t& dropped, unsigned cap) noexcept {
  if (lines >= cap) {
    ++dropped;
    return false;
  }
  ++lines;
  return true;
}

// LECTURA (por valor) del prompt de accion inferior del HUD nativo VIVO. Solo punteros del propio HUD, releidos en cada llamada (nunca cacheados).
// FAIL_CLOSED: si la pantalla de botones, la pantalla de referencia del HUD o el marco del mensaje flotante faltan, no estan alineados a 8 o sus vptr no
// son el mismo vptr de J2DScreen, no se devuelve ningun puntero escribible.
bool aligned8(const void* p) noexcept { return p && (reinterpret_cast<std::uintptr_t>(p) & 7u) == 0; }
lnk01::UiInput action_ui_read(dMeter2_c* meter, dMeterButton_c*& prompt, J2DScreen*& screen) noexcept {
  lnk01::UiInput in;
  prompt = nullptr;
  screen = nullptr;
  if (!aligned8(meter)) return in;
  in.meter = true;
  in.meter_key = reinterpret_cast<std::uintptr_t>(meter);
  dMeterButton_c* b = meter->mpEmpButton;
  if (!b) return in;
  in.prompt = true;
  in.prompt_key = reinterpret_cast<std::uintptr_t>(b);
  if (!aligned8(b)) return in;
  J2DScreen* s = b->mpButtonScreen;
  const J2DScreen* waku = b->mpScreen;
  const dMeter2Draw_c* md = meter->mpMeterDraw;
  const J2DScreen* ref = aligned8(md) ? md->mpScreen : nullptr;
  in.screen = s != nullptr && waku != nullptr && ref != nullptr;
  in.aligned = in.screen && aligned8(s) && aligned8(waku) && aligned8(ref);
  if (in.aligned) {
    const void* vs = *reinterpret_cast<void* const*>(s);
    in.vptr_ok = vs != nullptr && vs == *reinterpret_cast<void* const*>(ref) && vs == *reinterpret_cast<void* const*>(waku);
  }
  if (!in.vptr_ok) return in;
  in.visible = s->isVisible();
  prompt = b;
  screen = s;
  return in;
}

void action_ui_log(lnk01::UiEvent ev, unsigned site, const lnk01::UiInput& in, bool hidden_now) noexcept {
  if (!lnk_take(lg.ui_lines, lg.ui_dropped, 40)) return;
  static constexpr const char* sites[] = {"METER_EXECUTE_POST", "METER_DRAW_POST", "PROMPT_DRAW_PRE"};
  char line[kLogLineMax * 2];
  std::snprintf(line, sizeof(line),
                "LNK01_ACTION_UI event=%s site=%s family=dMeterButton_c(mpEmpButton) blo=zelda_game_image_button_info tags=ROOT(info_n+abtn_n,bbtn_n,"
                "rbtn,zbtn,3d,cbtn,sbtn,xbtn,ybtn,bin,info_ar*/fnfo_ar*,yaji_*,midona,item) sparkles=SLOT_MASK_DURING_OWN_DRAW kept=FLOATING_MESSAGE(waku+text) "
                "screen=%d aligned=%d vptr_j2dscreen=%d root_hidden_now=%d gameplay=UNAFFECTED writes=HUD_VISUAL_ONLY",
                lnk01::name(ev), sites[site < 3 ? site : 0], in.screen ? 1 : 0, in.aligned ? 1 : 0, in.vptr_ok ? 1 : 0, hidden_now ? 1 : 0);
  lnk_write(ev == lnk01::UiEvent::LayoutRefused ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}

// Un punto de control de la supresion. Idempotente; ocultar ya oculto no escribe. Se verifica la escritura inmediatamente.
void action_ui_step(dMeter2_c* meter, unsigned site) noexcept {
  if (!lg.ui.enabled) return;
  dMeterButton_c* b = nullptr;
  J2DScreen* s = nullptr;
  const lnk01::UiInput in = action_ui_read(meter, b, s);
  const lnk01::UiDecision d = lg.ui.step(in);
  if (d.hide && s) {
    action_ui_hide(s);
    if (s->isVisible()) ++lg.hide_verify_failed;
  }
  if (d.ev != lnk01::UiEvent::None) action_ui_log(d.ev, site, in, s ? !s->isVisible() : false);
}

// Hook VISUAL del draw del prompt (virtual, slot 0). PRE: solo para el prompt VIVO del HUD (mpEmpButton), re-oculta la raiz y enmascara los dos slots de
// brillo; cualquier otro dMeterButton_c (p. ej. el aviso de salto de cinematica) se deja intacto. POST: restaura los slots exactos.
HookAction on_prompt_draw_pre(ModContext*, void* args, void*, void*) noexcept {
  try {
    if (!lg.ui.enabled || !args) return HOOK_CONTINUE;
    ++lg.prompt_draw_pre;
    dMeterButton_c* self = mods::arg<dMeterButton_c*>(args, 0);
    dMeter2_c* meter = dMeter2Info_getMeterClass();
    const bool live = self && aligned8(meter) && meter->mpEmpButton == self;
    if (live) {
      ++lg.prompt_draw_live;
      action_ui_step(meter, 2);
    }
    const std::uint64_t key = reinterpret_cast<std::uintptr_t>(self);
    const lnk01::SparkleMask::Begin bgn =
        lg.mask.begin(live, live && lg.ui.layout_ok_for(key), key, live ? self->field_0x4be[0] : 0, live ? self->field_0x4be[1] : 0, lg.ui.c);
    if (bgn.apply) action_ui_slots(self, bgn.s0, bgn.s1);
  } catch (...) {
    lg.ui.enabled = false;
  }
  return HOOK_CONTINUE;
}
void on_prompt_draw_post(ModContext*, void* args, void*, void*) noexcept {
  try {
    dMeterButton_c* self = args ? mods::arg<dMeterButton_c*>(args, 0) : nullptr;
    const lnk01::SparkleMask::End e = lg.mask.end(reinterpret_cast<std::uintptr_t>(self), lg.ui.c);
    if (e.restore && self) action_ui_slots(self, e.s0, e.s1);
  } catch (...) {
    lg.ui.enabled = false;
  }
}

// Link viva de ESTA llamada (solo lectura): identidad y forma humana (flag inline FLG1_IS_WOLF).
struct LinkNow {
  bool valid = false, human = false, upper_guard = false;
  std::uint32_t id = 0;
  int proc = -1, damage_timer = 0;
  float frame = 0;              // LNK06: frame de la animacion inferior (mUnderFrameCtrl[0], +0x2d4c)
  std::uint32_t mode_flags = 0; // LNK05: daAlink_c::mModeFlg (+0x42fc)
};
LinkNow lnk01_link_now() noexcept {
  LinkNow l;
  daPy_py_c* lp = dComIfGp_getLinkPlayer();
  if (!lp || lp->profname != fpcNm_ALINK_e || lp->state.init_state == 3) return l;
  l.valid = true;
  l.id = static_cast<std::uint32_t>(fopAcM_GetID(lp));
  l.human = lp->checkWolf() == 0;
  l.proc = static_cast<int>(static_cast<const daAlink_c*>(lp)->mProcID);
  l.damage_timer = static_cast<int>(lp->mDamageTimer);  // LNK02/LNK04: invulnerabilidad nativa armada por setDamagePoint
  // LNK04: replica de LECTURA de daAlink_c::checkUpperGuardAnime (flags inline); el proc y el evento se comprueban en lnk01_update.
  l.upper_guard = lp->checkNoResetFlg2(daPy_py_c::FLG2_UNK_8000000) != 0 && lp->checkNoResetFlg0(daPy_py_c::FLG0_UNK_2) == 0;
  l.frame = static_cast<const daAlink_c*>(lp)->mUnderFrameCtrl[0].getFrame();
  l.mode_flags = static_cast<const daAlink_c*>(lp)->mModeFlg;
  return l;
}

// ======================================== LNK05 + LNK06: ruptura nativa de la guardia de Link ===========================================
void lnk05_log(const lnk05::Out& o, const char* family, unsigned& lines, std::uint64_t& dropped, unsigned cap) noexcept {
  if (o.ev == lnk05::Ev::None || !lnk_take(lines, dropped, cap)) return;
  char line[kLogLineMax * 2];
  if (lnk05::fmt_event(line, sizeof(line), family, o, lg.posture.milli()) > 0)
    lnk_write(o.ev == lnk05::Ev::TriggerFailed || o.ev == lnk05::Ev::Stuck || o.ev == lnk05::Ev::Aborted ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}
// Un impacto VALIDO que deja la postura en 100 (o la encuentra en 100) solicita UNA ruptura ligada a su episodio. Solo desde bloqueo o directo confirmados.
void lnk05_request(lnk05::Kind kind, std::uint32_t attacker, std::uint64_t generation, std::uint64_t token, int before, int after) noexcept {
  if (!lb.enabled || after < lnk01::policy::kMaxMilli || !lg.posture.bound()) return;
  lnk05::Episode e;
  e.link_id = lg.posture.link_id(), e.epoch = lg.posture.epoch(), e.tick = g.p8.tick, e.kind = kind, e.attacker = attacker, e.generation = generation, e.token = token;
  e.before = before, e.after = after;
  lnk05_log(lb.m.request(e), "LNK05_EVENT", lb.event_lines, lb.event_dropped, 150);
  lg.posture.hold(lb.m.hold());
}
// UNICA escritura de estado de Link de LNK05 (ownership=LINK_NATIVE_TRANSITION): la MISMA llamada que el juego hace desde checkDamageAction.
// procGuardBreakInit hace commonProcInit(PROC_GUARD_BREAK) + ANM_GUARD_BREAK + vibracion + voz. Nada de mProcID, field_0x2fd5, vida ni flags.
bool lnk05_native_break(daAlink_c* link) noexcept {
  ++lb.native_calls;
  (void)link->procGuardBreakInit();
  return link->mProcID == daAlink_c::PROC_GUARD_BREAK;
}
// Segundo POST del hook YA EXISTENTE de daAlink_c::checkDamageAction (FIX01 captura primero; este corre despues): dentro del execute de Link, tras su
// propia decision de dano/guardia de este tick. Solo actua con una solicitud pendiente y la puerta en OK.
void on_link_damage_post_lnk05(ModContext*, void* args, void*, void*) noexcept {
  try {
    if (!lb.enabled || !args || lb.m.state() != lnk05::State::TriggerPending) return;
    ++lb.posts_seen;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || static_cast<daPy_py_c*>(link) != dComIfGp_getLinkPlayer()) return;
    lnk05::GateIn gi;
    gi.valid = link->profname == fpcNm_ALINK_e && link->state.init_state != 3;
    gi.link_id = static_cast<std::uint32_t>(fopAcM_GetID(link));
    gi.epoch = lg.posture.epoch();
    gi.tick = g.p8.tick;
    gi.human = link->checkWolf() == 0;
    gi.alive = g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife() > 0;
    gi.sim_active = !g_dComIfG_gameInfo.play.isPauseFlag() && dMeter2Info_getWindowStatus() == 0;
    gi.event = g_dComIfG_gameInfo.play.getEvent()->runCheck() != 0;
    gi.proc = static_cast<int>(link->mProcID);
    gi.mode_flags = link->mModeFlg;
    gi.damage_this_call = fx.native.valid && fx.native.link_key == reinterpret_cast<std::uintptr_t>(link) && fx.native.tick == g.p8.tick &&
                          static_cast<int>(link->mDamageTimer) > fx.native.timer_before;
    const lnk05::Gate gate = lb.m.check(gi);
    if (gate != lnk05::Gate::Ok) {
      lnk05::Out r;
      r.ev = lnk05::Ev::RequestRejected, r.gate = gate, r.ep = lb.m.episode(), r.before = r.after = lb.m.state(), r.proc = gi.proc;
      if (gate != lnk05::Gate::ProcNotAllowed || gi.proc != daAlink_c::PROC_GUARD_SLIP) lnk05_log(r, "LNK05_GATE", lb.gate_lines, lb.gate_dropped, 60);
      return;
    }
    const bool entered = lnk05_native_break(link);
    const lnk05::Out o = lb.m.triggered(entered, static_cast<int>(link->mProcID), g.p8.tick);
    if (entered) lg.hud.break_flash();
    lnk05_log(o, "LNK05_NATIVE_BREAK", lb.native_lines, lb.native_dropped, 150);
    if (entered) lnk05_log(o, "LNK06_EVENT", lb.vuln_lines, lb.vuln_dropped, 150);  // LNK06: ventana de vulnerabilidad nativa abierta, regeneracion retenida
  } catch (...) {
    lb.enabled = false;
  }
}

// LNK02/LNK07 (SOLO LECTURA, en el PRE de damage_check tras post08_judge): historia de potencias At de ESTA identidad y atribucion del registro Tg
// de Link de la ultima pasada. checkDamageAction toma el dano del PRIMER cilindro Tg con impacto (d_a_alink_damage.inc:526-531): solo si ese cilindro
// registro una esfera At de ESTE actor puede ser este contacto el que hiere a Link. Punteros comparados, jamas seguidos.
// FIX01: read the native consumer BEFORE it clears Tg; this frame-local
// snapshot is never dereferenced after the call and never writes a game field.
HookAction on_link_damage_pre_fix01(ModContext*,void* args,void*,void*) noexcept {
 if(!args||g.eng.obs.disabled)return HOOK_CONTINUE;
 if(fx.native_depth++){fx.native_reentered=true;++fx.reentered;return HOOK_CONTINUE;}
 fx.native_reentered=false;fx.native=lnks1fix::NativeDamage{};
 auto* link=mods::arg<daAlink_c*>(args,0);const LinkNow live=lnk01_link_now();
 auto& n=fx.native;n.tick=g.p8.tick;
 if(!link||link!=dComIfGp_getLinkPlayer()||!live.valid||!live.human)return HOOK_CONTINUE;
 n.valid=true;n.human=true;n.link_key=reinterpret_cast<std::uintptr_t>(link);n.link_id=live.id;
 n.timer_before=link->mDamageTimer;n.life_before=g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife();
 n.pending_before=g_dComIfG_gameInfo.play.getItemLifeCount();
 n.event=g_dComIfG_gameInfo.play.getEvent()->runCheck()||g_dComIfG_gameInfo.play.isPauseFlag()||dMeter2Info_getWindowStatus()!=0;
 if(link->field_0x2fb8==0)for(int k=0;k<3;++k){
  if(!link->mTgCyls[k].cCcD_ObjHitInf::ChkTgHit())continue;
  n.cylinder=k;n.collider=reinterpret_cast<std::uintptr_t>(link->mTgCyls[k].cCcD_ObjHitInf::GetTgHitObj());
  n.blocked=link->mTgCyls[k].ChkTgShieldHit()!=0;break;
 }
 return HOOK_CONTINUE;
}
void on_link_poly_post_fix01(ModContext*,void* args,void* ret,void*) noexcept {
 if(fx.native_depth!=1||!args||!ret||fx.native_reentered)return;
 auto* link=mods::arg<daAlink_c*>(args,0);
 if(reinterpret_cast<std::uintptr_t>(link)!=fx.native.link_key)return;
 fx.native.poly_seen=true;fx.native.poly_result=*static_cast<const int*>(ret);
}
void on_link_damage_post_fix01(ModContext*,void* args,void*,void*) noexcept {
 if(!fx.native_depth)return;
 if(fx.native_depth>1){--fx.native_depth;return;}
 fx.native_depth=0;
 if(!args||fx.native_reentered||!fx.native.valid)return;
 auto* link=mods::arg<daAlink_c*>(args,0);auto n=fx.native;
 if(!link||link!=dComIfGp_getLinkPlayer()||reinterpret_cast<std::uintptr_t>(link)!=n.link_key||fopAcM_GetID(link)!=n.link_id)return;
 n.timer_after=link->mDamageTimer;n.life_after=g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife();
 n.event=n.event||g_dComIfG_gameInfo.play.getEvent()->runCheck()!=0;
 n.pending_after=g_dComIfG_gameInfo.play.getItemLifeCount();n.electric=link->checkResetFlg1(daPy_py_c::RFLG1_UNK_2)!=0;
 fx.direct.capture(n);
}
void fix01_ai_feed(daE_OC_c* a,const PreReads& r,daPy_py_c* lp) noexcept {
 if(!g.cur.tracked||g.cur.slot<0||!fx.ai.slot(static_cast<unsigned>(g.cur.slot)).on)return;
 lnks1fix::AiSample x;x.tick=g.p8.tick;x.slot=g.cur.slot;x.id=g.cur.id;x.generation=g.cur.generation;
 x.mode=r.snap.mode;x.state=r.snap.state;x.hp=r.snap.health;x.atp=r.aux.atp;x.aux=r.aux;
 x.dead=x.hp<=0||post06::policy::death_mode(x.mode);x.ko=x.mode==5||x.mode==6;x.hit=r.snap.tg_bits!=0;
 x.event=g_dComIfG_gameInfo.play.getEvent()->runCheck()!=0;
 x.target=lp&&lp->profname==fpcNm_ALINK_e&&lp->state.init_state!=3&&g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife()>0&&!lp->checkNoResetFlg1(daPy_py_c::FLG1_IS_WOLF);
 if(x.target)x.distance=fopAcM_searchActorDistance(a,lp);
 x.guard=g.cur.pin.now.guard_attack||lg.obs.guard_safe;
 x.variant=lg.posture.variants.classify(x.slot,x.id,x.generation,r.aux.resource==static_cast<std::uint8_t>(NameKind::E_OC2),r.aux.atp).v;
 const auto verdict=fx.ai.feed(x);
 if(verdict!=lnks1fix::AiVerdict::None){
  if(lnk_take(fx.ai_lines,fx.ai_dropped,160)){char line[kLogLineMax*2];const auto&w=fx.ai.slot(static_cast<unsigned>(x.slot));
   std::snprintf(line,sizeof(line),"LNK_S1_FIX01_AI verdict=%s actor_id=%u generation=%llu variant=%s mode=%d state=%d frame=%.2f rate=%.2f attr=%u atp=%d distance=%.1f target=%d guard=%d hit=%d starts=%u actual_arms=%u complete=%u aborts=%u legitimate_cancels=%u restarts=%u unresolved=%d expansion=CONTEXT_RESOURCE_ATP_RATE_ONLY",
    lnks1fix::name(verdict),x.id,static_cast<unsigned long long>(x.generation),lnk01::name(x.variant),x.mode,x.state,x.aux.anim_frame,x.aux.anim_rate,unsigned(x.aux.anim_attr),x.atp,x.distance,int(x.target),int(x.guard),int(x.hit),w.starts,w.arms,w.completions,w.aborts,w.cancels,w.restarts,int(w.suspected));lnk_write(LOG_LEVEL_INFO,line);
  }
 }
}
HookAction on_attack_pre_fix01(ModContext*,void* args,void*,void*) noexcept {
 fx.attack_open=false;
 if(!args||g.eng.obs.disabled||!g.cur.tracked||g.cur.slot<0||!fx.ai.slot(static_cast<unsigned>(g.cur.slot)).on)return HOOK_CONTINUE;
 auto* a=mods::arg<daE_OC_c*>(args,0);
 if(!a||reinterpret_cast<std::uintptr_t>(a)!=g.cur.actor||!g.eng.obs.ledger.valid(g.cur.slot,g.cur.generation)||fopAcM_GetID(a)!=g.cur.id)return HOOK_CONTINUE;
 fx.attack_open=true;fx.attack_key=reinterpret_cast<std::uintptr_t>(a);fx.attack_entry=read_aux(a);fx.attack_state=a->mOcState;
 return HOOK_CONTINUE;
}
void on_attack_post_fix01(ModContext*,void* args,void*,void*) noexcept {
 if(!args||g.eng.obs.disabled||!g.cur.tracked||g.cur.slot<0)return;
 auto* a=mods::arg<daE_OC_c*>(args,0);
 if(!a||reinterpret_cast<std::uintptr_t>(a)!=g.cur.actor||!g.eng.obs.ledger.valid(g.cur.slot,g.cur.generation)||fopAcM_GetID(a)!=g.cur.id)return;
 if(!fx.ai.slot(static_cast<unsigned>(g.cur.slot)).on)return;
 if(!fx.attack_open||fx.attack_key!=reinterpret_cast<std::uintptr_t>(a))return;
 fx.attack_open=false;
 const bool actual=a->mSphs_at[0].ChkAtSet()||a->mSphs_at[1].ChkAtSet();
 const auto&e=fx.attack_entry;
 const bool terminal=e.anim_ok&&e.anim_attr==0&&((e.anim_state&1)!=0||(fx.attack_state==3&&e.anim_rate<0&&e.anim_frame<=0)||(fx.attack_state!=3&&e.anim_rate>0&&e.anim_frame>=e.anim_end));
 const bool completed=terminal&&(a->mActionMode==3||a->mActionMode==16||(a->mActionMode==4&&a->mOcState==0));
 fx.ai.native_attack(g.cur.slot,g.cur.id,g.cur.generation,actual,completed);
}
void fix01_flush_direct(const lnk01::LinkObs& ob) noexcept {
 for(unsigned i=0;i<lnk01::kSlots;++i){
  const auto& offer=fx.direct.offered(i);if(!offer.on)continue;
  const bool identity=g.eng.obs.ledger.valid(static_cast<int>(i),offer.k.generation)&&g.eng.obs.ledger.slot(i).id==offer.k.id&&g.eng.obs.ledger.slot(i).life==Life::Active;
  const auto z=fx.direct.take(i,g.p8.tick,identity,ob.id,ob.active&&ob.human&&ob.life>0);
  if(!z.present)continue;
  const auto o=lg.posture.on_contact(z.k);
  if(lnk_take(fx.direct_lines,fx.direct_dropped,180)){char line[kLogLineMax*2];
   std::snprintf(line,sizeof(line),"LNK_S1_FIX01_DIRECT event=%s attacker_id=%u generation=%llu variant=%s episode=%llu collider_sphere=%d native_cylinder=%d contact_tick=%llu native_tick=%llu resolve_tick=%llu attribution=%s scoped_damage=%d life=%d>%d pending=%.2f>%.2f damage_timer=%d>%d posture=%d>%d weight=%d",
    lnk01::name(o.v),z.k.id,static_cast<unsigned long long>(z.k.generation),lnk01::name(o.vid.v),static_cast<unsigned long long>(z.k.token),z.matched?(z.native.collider==offer.sphere[0]?0:1):-1,z.native.cylinder,static_cast<unsigned long long>(z.k.tick),static_cast<unsigned long long>(z.native.tick),static_cast<unsigned long long>(g.p8.tick),z.why,int(z.matched),z.native.life_before,z.native.life_after,double(z.native.pending_before),double(z.native.pending_after),z.native.timer_before,z.native.timer_after,o.before,o.after,o.weight);lnk_write(LOG_LEVEL_INFO,line);
  }
 }
}

void lnk_pre(daE_OC_c* a, const PreReads& r, daPy_py_c* lp) noexcept {
  if (g.cur.tracked && g.cur.slot >= 0)
    lg.posture.variants.observe(g.cur.slot, g.cur.id, g.cur.generation, r.aux.resource == static_cast<std::uint8_t>(NameKind::E_OC2), r.aux.atp);
  if (!post08::Parry::any_contact(g.cur.pin) || !lp || lp->profname != fpcNm_ALINK_e || lp->state.init_state == 3) return;
  ++lg.tg_reads;
  const auto* link = static_cast<const daAlink_c*>(lp);
  const std::uintptr_t s0 = reinterpret_cast<std::uintptr_t>(static_cast<const cCcD_Obj*>(&a->mSphs_at[0]));
  const std::uintptr_t s1 = reinterpret_cast<std::uintptr_t>(static_cast<const cCcD_Obj*>(&a->mSphs_at[1]));
  for (int k = 0; k < 3; ++k) {
    if (!link->mTgCyls[k].cCcD_ObjHitInf::ChkTgHit()) continue;
    const std::uintptr_t hit = reinterpret_cast<std::uintptr_t>(link->mTgCyls[k].cCcD_ObjHitInf::GetTgHitObj());
    g.cur.lnk_tg_cyl = k;
    g.cur.lnk_tg_match = hit != 0 && (hit == s0 || hit == s1);
    break;
  }
  if (g.cur.lnk_tg_match) ++lg.tg_matches;
}

// UN juicio de postura de Link por llamada de damage_check con contacto: el MISMO veredicto POST08 (tras post08_step), nunca un hook ni una lectura nueva
// del colisionador. Unica escritura: el contador PRIVADO lg.posture.
void lnk01_contact() noexcept {
  const post08::Verdict& v = g.cur.pv;
  if (v.result == post08::Result::None) return;
  const post08::Input& in = g.cur.pin;
  const LinkNow l = lnk01_link_now();
  lnk01::Contact k;
  k.result = v.result;
  k.blocked = v.blocked;
  k.token = v.token;
  k.slot = in.slot;
  k.id = in.id;
  k.generation = in.generation;
  if (in.slot >= 0 && static_cast<unsigned>(in.slot) < post08::kSlots) {
    const post08::Parry::Episode& e = g.parry.episode(static_cast<unsigned>(in.slot));
    k.episode_parried = e.bound && e.id == in.id && e.generation == in.generation && e.token == v.token && e.parried;
  }
  k.e_oc2 = in.e_oc2;
  k.atp = in.atp;
  k.link_ok = in.link_ok && l.valid;
  k.link_human = l.human;
  k.link_id = l.id;
  k.event_running = in.event_running;
  k.tg_match = g.cur.lnk_tg_match;
  k.tick = g.p8.tick;
  if(k.result==post08::Result::NotBlocked){
    auto* a=reinterpret_cast<daE_OC_c*>(g.cur.actor); // live only in this damage_check; store opaque keys, never this pointer
    fx.direct.offer(k,reinterpret_cast<std::uintptr_t>(static_cast<const cCcD_Obj*>(&a->mSphs_at[0])),reinterpret_cast<std::uintptr_t>(static_cast<const cCcD_Obj*>(&a->mSphs_at[1])),in.fresh);
    return;
  }
  const lnk01::Outcome o = lg.posture.on_contact(k);
  if (o.v == lnk01::Verdict::None) return;
  if (o.v == lnk01::Verdict::Confirmed) lnk05_request(lnk05::Kind::Block, k.id, k.generation, k.token, o.before, o.after);  // LNK05 caso A/B
  if (o.v == lnk01::Verdict::RejectedParry && v.result == post08::Result::Confirmed)  // LNK05 caso C: un parry nunca rompe y cancela lo pendiente
    lnk05_log(lb.m.parry(g.p8.tick), "LNK05_EVENT", lb.event_lines, lb.event_dropped, 150);
  char line[kLogLineMax * 2];
  if (o.v == lnk01::Verdict::RejectedParry && v.result == post08::Result::Confirmed) {  // LNK03: +40 al enemigo (POST08), +0 a Link
    ++lg.parry_post08_enemy_credit;
    if (lnk_take(lg.parry_lines, lg.parry_dropped, 80)) {
      std::snprintf(line, sizeof(line),
                    "LNK_S1_PARRY event=PARRY_CONFIRMED_POST08 enemy_credit=POST08_APPLY_PARRY(+%d) link_credit=+%d excluded=BLOCK+DIRECT_FOR_THIS_ATTACK "
                    "attacker_id=%u generation=%llu attack_token=%llu variant=%s tick=%llu link_posture=%d.%03d episode_parried=%d invariants=LINK_UNCHANGED",
                    post04::policy::kParryPostureGain, lnk01::policy::kParryCostPoints, k.id, static_cast<unsigned long long>(k.generation),
                    static_cast<unsigned long long>(k.token), lnk01::name(o.vid.v), static_cast<unsigned long long>(k.tick), o.after / 1000, o.after % 1000,
                    k.episode_parried ? 1 : 0);
      lnk_write(LOG_LEVEL_INFO, line);
    }
  }
  if (o.v == lnk01::Verdict::Confirmed && lnk_take(lg.balance_lines, lg.balance_dropped, 120) &&
      lnk01::fmt_balance(line, sizeof(line), o.vid, false, o.weight, k.id, k.generation) > 0)
    lnk_write(LOG_LEVEL_INFO, line);
  if (!lnk_take(lg.block_lines, lg.block_dropped, 200)) return;
  if (lnk01::fmt_block(line, sizeof(line), k, o, in.pass.proc, in.fresh) > 0) lnk_write(LOG_LEVEL_INFO, line);
}

// UNA vez por mod_update (tick de simulacion): identidad de Link, regeneracion y medicion (solo lectura) de las rupturas de guardia NATIVAS.
void lnk01_update() noexcept {
  const LinkNow l = lnk01_link_now();
  const int life = static_cast<int>(g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife());
  // LNK04: vida maxima en las MISMAS unidades que la vida (dMeter2_c::moveLife: (getMaxLife()/5)*4), nunca una constante de corazones.
  const int life_max = (static_cast<int>(g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getMaxLife()) / 5) * 4;
  lg.link_valid = l.valid;
  lg.link_id = l.id;
  lg.link_human = l.valid && l.human;
  lg.sim_active = l.valid && life > 0 && !g_dComIfG_gameInfo.play.isPauseFlag() && !g_dComIfG_gameInfo.play.getEvent()->runCheck() &&
                  dMeter2Info_getWindowStatus() == 0;
  // LNK02/LNK04: evidencia nativa de ESTE tick, por valor.
  lnk01::LinkObs& ob = lg.obs;
  ob = lnk01::LinkObs{};
  ob.valid = l.valid, ob.human = l.valid && l.human, ob.id = l.id, ob.life = life, ob.life_max = life_max, ob.damage_timer = l.damage_timer;
  ob.pending_life = g_dComIfG_gameInfo.play.getItemLifeCount();
  ob.active = lg.sim_active;
  ob.guard_safe = lg.sim_active && ob.human && l.upper_guard && l.damage_timer == 0 && l.proc != daAlink_c::PROC_GUARD_SLIP &&
                  l.proc != daAlink_c::PROC_GUARD_ATTACK && l.proc != daAlink_c::PROC_GUARD_BREAK && l.proc != daAlink_c::PROC_CUT_REVERSE;
  lg.posture.observe_link(l.valid, l.id);
  if (lg.posture.epoch() != lg.hud_epoch || !lg.posture.bound()) {  // Link nueva o ausente: la barra se reinicia (sin fantasma)
    if (lg.hud_epoch != lg.posture.epoch() || lg.hud.drawable()) lg.hud.reset();
    lg.hud_epoch = lg.posture.epoch();
  }
  char line[kLogLineMax * 2];
  lnk01::DirectEvents de;
  fix01_flush_direct(ob);
  lg.posture.confirm_direct(ob, g.p8.tick, de);  // LNK02: confirmacion o descarte de los golpes directos pendientes del tick anterior
  lg.direct_events_dropped += de.dropped;
  for (unsigned i = 0; i < de.n; ++i) {
    const lnk01::DirectEvent& d = de.e[i];
    if (d.r == lnk01::DirectResult::Confirmed && lnk_take(lg.balance_lines, lg.balance_dropped, 120) &&
        lnk01::fmt_balance(line, sizeof(line), d.vid, true, d.weight, d.id, d.generation) > 0)
      lnk_write(LOG_LEVEL_INFO, line);
    if (lnk_take(lg.direct_lines, lg.direct_dropped, 150) && lnk01::fmt_direct(line, sizeof(line), d) > 0) lnk_write(LOG_LEVEL_INFO, line);
    if (d.r == lnk01::DirectResult::Confirmed) lnk05_request(lnk05::Kind::Direct, d.id, d.generation, d.token, d.before, d.after);  // LNK05 caso D
  }
  // LNK06: UNA observacion por tick de la maquina nativa (entrada adoptada, frame, salida nativa, caducidad); reset unico tras la salida.
  if (lb.enabled) {
    lnk05::ObsIn bi;
    bi.valid = l.valid && l.human, bi.alive = life > 0, bi.sim_active = lg.sim_active, bi.event = g_dComIfG_gameInfo.play.getEvent()->runCheck() != 0;
    bi.link_id = l.id, bi.epoch = lg.posture.epoch(), bi.tick = g.p8.tick, bi.proc = l.proc, bi.frame = l.frame;
    const lnk05::Out bo = lb.m.observe(bi, lg.posture.milli() >= lnk01::policy::kMaxMilli);
    if (bo.ev == lnk05::Ev::NativeAdopted) lg.hud.break_flash();
    if (bo.reset_posture) lg.posture.break_reset();
    const bool recovery = bo.ev == lnk05::Ev::RecoveredCancel || bo.ev == lnk05::Ev::RecoveredEnd || bo.ev == lnk05::Ev::Interrupted || bo.ev == lnk05::Ev::Aborted;
    if (recovery) lnk05_log(bo, "LNK06_RECOVERY", lb.recovery_lines, lb.recovery_dropped, 150);
    else if (bo.ev == lnk05::Ev::Stuck) lnk05_log(bo, "LNK06_EVENT", lb.vuln_lines, lb.vuln_dropped, 150);
    else lnk05_log(bo, bo.ev == lnk05::Ev::Expired ? "LNK05_EVENT" : "LNK05_NATIVE_BREAK", lb.native_lines, lb.native_dropped, 150);
    if (bo.ev == lnk05::Ev::NativeAdopted) lnk05_log(bo, "LNK06_EVENT", lb.vuln_lines, lb.vuln_dropped, 150);
    lg.posture.hold(lb.m.hold());
  }
  const int pm = lnk01::life_permille(life, life_max);
  if (pm < 0 && l.valid) ++lg.life_invalid_ticks;
  if (ob.guard_safe) ++lg.guard_safe_ticks;
  const lnk01::RegenEvent ev = lg.posture.tick(lg.sim_active, pm, ob.guard_safe);  // LNK04: banda de vida x guardia segura, reloj de simulacion
  if ((ev.started || ev.zero || ev.paused_edge) && lnk_take(lg.regen_lines, lg.regen_dropped, 60)) {
    std::snprintf(line, sizeof(line), "LNK01_REGEN event=%s before=%d.%03d after=%d.%03d delay_ticks=%u rate_milli_pts_per_s=%d tick=%llu timebase=MOD_UPDATE_SIM_TICK",
                  ev.started ? "START" : (ev.zero ? "ZERO" : "PAUSED"), ev.before / 1000, ev.before % 1000, ev.after / 1000, ev.after % 1000,
                  lnk01::policy::kRegenDelayTicks, ev.rate_milli_per_s, static_cast<unsigned long long>(g.p8.tick));
    lnk_write(LOG_LEVEL_INFO, line);
  }
  if ((ev.started || ev.zero || ev.paused_edge || ev.band_change || ev.guard_change) && lnk_take(lg.s1_regen_lines, lg.s1_regen_dropped, 120)) {
    std::snprintf(line, sizeof(line),
                  "LNK_S1_REGEN event=%s life=%d life_max=%d life_permille=%d band=%u(%s) guard=%s rate_milli_pts_per_s=%d delay_ticks=%u before=%d.%03d after=%d.%03d "
                  "paused=%d tick=%llu",
                  ev.started ? "START" : (ev.zero ? "ZERO" : (ev.paused_edge ? "PAUSED" : (ev.band_change ? "BAND_CHANGE" : "GUARD_CHANGE"))), life, life_max, pm, ev.band,
                  ev.band == 0 ? "75-100" : (ev.band == 1 ? "50-75" : (ev.band == 2 ? "25-50" : "0-25")), ev.guard ? "SAFE_GUARD_X1.5" : "NEUTRAL",
                  ev.rate_milli_per_s, lnk01::policy::kRegenDelayTicks, ev.before / 1000, ev.before % 1000, ev.after / 1000, ev.after % 1000, ev.paused_edge ? 1 : 0,
                  static_cast<unsigned long long>(g.p8.tick));
    lnk_write(LOG_LEVEL_INFO, line);
  }
  // Medicion de las rupturas NATIVAS (contador field_0x2fd5 del juego, NO leido ni escrito): entradas en GUARD_SLIP / GUARD_BREAK de Link.
  const int proc = l.valid ? l.proc : -1;
  if (proc != lg.last_proc && (proc == daAlink_c::PROC_GUARD_SLIP || proc == daAlink_c::PROC_GUARD_BREAK)) {
    const bool brk = proc == daAlink_c::PROC_GUARD_BREAK;
    ++(brk ? lg.guard_break_entries : lg.guard_slip_entries);
    if (brk && lg.posture.milli() >= lnk01::policy::kMaxMilli) ++lg.guard_break_at_100;
    if (lnk_take(lg.guard_lines, lg.guard_dropped, 40)) {
      std::snprintf(line, sizeof(line), "LNK01_GUARD event=NATIVE_%s link_posture=%d.%03d tick=%llu note=NATIVE_COUNTER_UNTOUCHED(LNK05_INTEGRATION_PENDING)",
                    brk ? "GUARD_BREAK" : "GUARD_SLIP", lg.posture.milli() / 1000, lg.posture.milli() % 1000, static_cast<unsigned long long>(g.p8.tick));
      lnk_write(LOG_LEVEL_INFO, line);
    }
  }
  lg.last_proc = proc;
}

void emit_lnk01_summary(const char* tag) noexcept {
  static unsigned lines = 0;
  const char* k = "SHUTDOWN";
  unsigned i = 0;
  while (tag && tag[i] && k[i] && tag[i] == k[i]) ++i;
  const bool shutdown = tag && tag[i] == 0 && k[i] == 0;
  if (lines >= 60 && !shutdown) return;
  ++lines;
  const auto u = [](std::uint64_t x) { return static_cast<unsigned long long>(x); };
  const lnk01::PCounters& c = lg.posture.c;
  const auto V = [&](lnk01::Verdict x) { return u(c.verdict[static_cast<unsigned>(x)]); };
  const lnk01::UiCounters& uc = lg.ui.c;
  const lnk01::HudCounters& hc = lg.hud.c;
  const bool inv = lg.posture.conserved() && lg.posture.milli() >= 0 && lg.posture.milli() <= lnk01::policy::kMaxMilli && lg.hide_verify_failed == 0 &&
                   uc.mask_unpaired == 0 && uc.masks == uc.mask_restores + (lg.mask.active() ? 1u : 0u);
  char line[kLogLineMax * 4];
  std::snprintf(line, sizeof(line),
                "LNK01_SUMMARY v=%s tag=%s part=1/2 contacts=%llu blocks=%llu by(blue/red/black/black_fury/unknown)=%llu/%llu/%llu/%llu/%llu "
                "rejected(stale/parry/duplicate/nonblock/untracked/link/event)=%llu/%llu/%llu/%llu/%llu/%llu/%llu current=%d.%03d max=%d gained=%llu regen=%llu "
                "lost_on_reset=%llu regen_ticks=%llu regen_starts=%llu zeros=%llu saturations=%llu paused_ticks=%llu link_binds=%llu link_changes=%llu resets=%llu "
                "native_guard_slip=%llu native_guard_break=%llu native_break_with_link_100=%llu actor_writes=0 link_writes=0 invariants=%d",
                lnk01::kVersion, tag, u(c.contacts), V(lnk01::Verdict::Confirmed), u(c.block_by_variant[0]), u(c.block_by_variant[1]), u(c.block_by_variant[2]),
                u(c.block_by_variant[3]), u(c.block_by_variant[4]),
                V(lnk01::Verdict::RejectedStale), V(lnk01::Verdict::RejectedParry), V(lnk01::Verdict::Duplicate), V(lnk01::Verdict::RejectedNonBlock),
                V(lnk01::Verdict::RejectedUntracked), V(lnk01::Verdict::RejectedLink), V(lnk01::Verdict::RejectedEvent), lg.posture.milli() / 1000,
                lg.posture.milli() % 1000, lnk01::policy::kMaxPoints, u(c.gained_milli), u(c.regen_milli), u(lg.posture.lost_milli()), u(c.regen_ticks),
                u(c.regen_starts), u(c.zeros), u(c.saturations), u(c.paused_ticks), u(c.binds), u(c.link_changes), u(c.resets), u(lg.guard_slip_entries),
                u(lg.guard_break_entries), u(lg.guard_break_at_100), inv ? 1 : 0);
  lnk_write(inv ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
  std::snprintf(line, sizeof(line),
                "LNK01_SUMMARY v=%s tag=%s part=2/2 button_hud=%s sparkle_hook=%s ui_checks=%llu hidden_frames=%llu hide_writes=%llu already_hidden=%llu "
                "no_prompt=%llu no_meter=%llu missing_tags_or_layout_refused=%llu prompts_seen=%llu meters_seen=%llu hud_recreated=%llu hide_verify_failed=%llu "
                "sparkle_masks=%llu restores=%llu foreign_prompt_draws=%llu mask_refused=%llu unpaired=%llu prompt_draws=%llu live=%llu hud_ticks=%llu "
                "hud_paused=%llu hud_frames=%llu hud_quads=%llu bar_hits=%llu bar_pulses=%llu bar_fade_outs=%llu bar_resets=%llu hidden_not_human=%llu "
                "layout_changes=%llu lines(block/regen/guard/ui)=%u+%llu/%u+%llu/%u+%llu/%u+%llu",
                lnk01::kVersion, tag, lg.ui.enabled ? "HIDDEN_ACTIVE" : "OFF", lg.sparkle_hook ? "ON" : "OFF", u(uc.checks), u(uc.hidden_checks),
                u(uc.hide_writes), u(uc.already_hidden), u(uc.no_prompt), u(uc.no_meter), u(uc.refused), u(uc.prompts_seen), u(uc.meters_seen),
                u(uc.hud_recreated), u(lg.hide_verify_failed), u(uc.masks), u(uc.mask_restores), u(uc.mask_foreign), u(uc.mask_refused), u(uc.mask_unpaired),
                u(lg.prompt_draw_pre), u(lg.prompt_draw_live), u(hc.ticks), u(hc.paused), u(lg.player_frames), u(lg.player_quads), u(hc.hits), u(hc.pulses),
                u(hc.fade_outs), u(hc.resets), u(hc.hidden_not_human), u(lg.layout_changes), lg.block_lines, u(lg.block_dropped), lg.regen_lines,
                u(lg.regen_dropped), lg.guard_lines, u(lg.guard_dropped), lg.ui_lines, u(lg.ui_dropped));
  lnk_write(LOG_LEVEL_INFO, line);
  // LNK SESSION01 (LNK02/03/04/07)
  const auto D = [&](lnk01::DirectResult x) { return u(c.direct_result[static_cast<unsigned>(x)]); };
  std::snprintf(line, sizeof(line),
                "LNK_S1_SUMMARY v=%s tag=%s part=1/2 blocks=%llu direct_confirmed=%llu direct_pending_now=%u direct_rejected(not_attributed/no_native_damage/no_life_loss/"
                "ambiguous/link_changed/superseded)=%llu/%llu/%llu/%llu/%llu/%llu parries_post08=%llu parries_excluded_link=%llu link_parry_credit=%d "
                "duplicates=%llu stale=%llu block_by(blue/red/black/fury/unknown)=%llu/%llu/%llu/%llu/%llu direct_by(blue/red/black/fury/unknown)=%llu/%llu/%llu/%llu/%llu "
                "fury_credits=%llu group_power_adjusted=%llu unknown_variant=%llu gained(block/direct)=%llu/%llu damage_edges=%llu unattributed_damage_edges=%llu "
                "tg_reads=%llu tg_matches=%llu",
                lnk01::kVersion, tag, V(lnk01::Verdict::Confirmed), D(lnk01::DirectResult::Confirmed), lg.posture.pending(),
                V(lnk01::Verdict::RejectedDirectNotAttributed), D(lnk01::DirectResult::NoNativeDamage), D(lnk01::DirectResult::NoLifeLoss),
                D(lnk01::DirectResult::Ambiguous), D(lnk01::DirectResult::LinkChanged), D(lnk01::DirectResult::Superseded), u(lg.parry_post08_enemy_credit),
                u(c.parries_excluded), lnk01::policy::kParryCostPoints, V(lnk01::Verdict::Duplicate), V(lnk01::Verdict::RejectedStale), u(c.block_by_variant[0]),
                u(c.block_by_variant[1]), u(c.block_by_variant[2]), u(c.block_by_variant[3]), u(c.block_by_variant[4]), u(c.direct_by_variant[0]),
                u(c.direct_by_variant[1]), u(c.direct_by_variant[2]), u(c.direct_by_variant[3]), u(c.direct_by_variant[4]), u(c.fury_credits), u(c.group_adjusted),
                u(c.unknown_variant), u(c.gained_block_milli), u(c.gained_direct_milli), u(c.damage_edges), u(c.damage_edges_unattributed), u(lg.tg_reads),
                u(lg.tg_matches));
  lnk_write(inv ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
  std::snprintf(line, sizeof(line),
                "LNK_S1_SUMMARY v=%s tag=%s part=2/2 regen_ticks_by_band(75-100/50-75/25-50/0-25)=%llu/%llu/%llu/%llu regen_milli_by_band=%llu/%llu/%llu/%llu "
                "regen_guard_ticks=%llu regen_guard_milli=%llu guard_safe_ticks=%llu life_invalid_ticks=%llu saturations=%llu zeros=%llu resets=%llu current=%d.%03d "
                "invariants=%d anomalies(direct_events_dropped)=%llu link_life_writes=0 link_state_writes=0 native_guard_counter_writes=0 tests=CI_LABS_SEE_RESULTS "
                "lines(direct/parry/regen/balance)=%u+%llu/%u+%llu/%u+%llu/%u+%llu",
                lnk01::kVersion, tag, u(c.regen_ticks_by_band[0]), u(c.regen_ticks_by_band[1]), u(c.regen_ticks_by_band[2]), u(c.regen_ticks_by_band[3]),
                u(c.regen_milli_by_band[0]), u(c.regen_milli_by_band[1]), u(c.regen_milli_by_band[2]), u(c.regen_milli_by_band[3]), u(c.regen_guard_ticks),
                u(c.regen_guard_milli), u(lg.guard_safe_ticks), u(lg.life_invalid_ticks), u(c.saturations), u(c.zeros), u(c.resets), lg.posture.milli() / 1000,
                lg.posture.milli() % 1000, inv ? 1 : 0, u(lg.direct_events_dropped), lg.direct_lines, u(lg.direct_dropped), lg.parry_lines, u(lg.parry_dropped),
                lg.s1_regen_lines, u(lg.s1_regen_dropped), lg.balance_lines, u(lg.balance_dropped));
  lnk_write(LOG_LEVEL_INFO, line);
}

void do_pre(daE_OC_c* a) noexcept {
  PreReads r;
  r.facts = read_facts(a);
  const bool candidate = is_candidate(admit(r.facts, g.eng.policy).adm);  // POST05-COMPAT: misma politica que el motor (V2Compat)
  if (candidate) {
    read_core(a, r.snap);
    r.raw0 = raw_hit_obj(a->mSphs_cc[0]);
    r.raw1 = raw_hit_obj(a->mSphs_cc[1]);
    r.aux = read_aux(a);
  }
  Events ev;
  const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(a));

  g.cur = Runtime::CallCtx{};
  g.cur.actor = reinterpret_cast<std::uintptr_t>(a);

  g.cur.id = id;

  g.cur.pre_hp = r.snap.health;
  g.cur.pre_mode = r.snap.mode;
  // Read the live player's sword/shield colliders by address equality only.
  auto* lp=dComIfGp_getLinkPlayer();
  if(candidate&&!g.rx.session_disabled()&&lp&&lp->profname==fpcNm_ALINK_e&&lp->state.init_state!=3){
    auto* link=static_cast<daAlink_c*>(lp);
    g.cur.link_known=true;g.cur.link_id=fopAcM_GetID(lp);g.cur.link_helm_proc=link->mProcID==daAlink_c::PROC_CUT_HEAD;
    const std::uintptr_t selected=(r.snap.tg_bits&2)?r.raw1:r.raw0;
    g.cur.selected_raw=selected;
    const auto contact=post06::classify_link_contact(selected,
      reinterpret_cast<std::uintptr_t>(static_cast<cCcD_Obj*>(&link->mAtCyl)),
      reinterpret_cast<std::uintptr_t>(static_cast<cCcD_Obj*>(&link->mAtCps[0])),
      reinterpret_cast<std::uintptr_t>(static_cast<cCcD_Obj*>(&link->mAtCps[1])),
      reinterpret_cast<std::uintptr_t>(static_cast<cCcD_Obj*>(&link->mAtCps[2])),
      reinterpret_cast<std::uintptr_t>(static_cast<cCcD_Obj*>(&link->mGuardAtCps)));
    g.cur.from_link=contact!=post06::LinkContact::None;
    g.cur.helm=contact==post06::LinkContact::Sword&&link->mProcID==daAlink_c::PROC_CUT_HEAD&&(lp->mCutType==5||lp->mCutType==8);
    if(g.cur.helm&&r.snap.cooldown==0&&!r.snap.cutdown){
      g.cur.link_id=fopAcM_GetID(lp);
      g.cur.helm_floor=daPy_py_c::checkMasterSwordEquip()?100:daPy_py_c::checkWoodSwordEquip()?0:50;

    }
  }
  if(candidate){
    for(unsigned i=0;i<post03::kCapacity;++i){const auto& old=g.eng.obs.ledger.slot(i);auto& h=g.helm_episodes[i];
      if(old.id!=id||old.ptr!=reinterpret_cast<std::uintptr_t>(a)||old.generation!=h.generation||h.id!=id)continue;
      h.observe(id,old.generation,g.cur.link_id,g.cur.link_helm_proc,g.cur.link_known);
      if(g.cur.helm&&h.seen&&h.link_id==g.cur.link_id&&r.snap.cooldown==0){helm_guard_duplicate(a);r.snap.cooldown=1;g.cur.helm_floor=0;}
    }
    if(g.cur.helm_floor&&lp&&lp->mCutType==5){helm_set_cut(lp,8);g.cur.mask_owned=true;}
  }
  const Observation o=g.pe.on_pre(reinterpret_cast<std::uintptr_t>(a),id,r,ev);
  g.cur.slot=o.slot;g.cur.generation=o.generation;g.cur.tracked=o.tracked&&candidate;
  if(candidate){g.cur.pre_live=read_live(a);g.cur.pre_aux=r.aux;post08_judge(a,r,lp);fx.cleanup.observe(g.cur.slot,g.cur.id,g.cur.generation,g.p8.tick,(g.cur.pin.at_hit[0]?1u:0u)|(g.cur.pin.at_hit[1]?2u:0u));post08_watch_feed(r);lnk_pre(a,r,lp);fix01_ai_feed(a,r,lp);}
  if(!g.cur.tracked&&g.cur.mask_owned&&lp&&lp->mCutType==8){helm_set_cut(lp,5);g.cur.mask_owned=false;g.cur.helm_floor=0;}
  if(o.tracked&&o.slot>=0){auto& h=g.helm_episodes[o.slot];if(h.id!=id||h.generation!=o.generation){h=post06::HelmEpisode{};h.id=id;h.generation=o.generation;}h.observe(id,o.generation,g.cur.link_id,g.cur.link_helm_proc,g.cur.link_known);}
  emit_events(ev);
  drain_posture();
}

void do_post(daE_OC_c* a) noexcept {
  PostReads r;
  r.alive=static_cast<int>(a->profname)==static_cast<int>(fpcNm_E_OC_e)&&a->state.init_state >= 0&&a->state.init_state!=3;
  if(r.alive)r.selected=reinterpret_cast<std::uintptr_t>(a->mAtInfo.mpCollider);
  if(r.selected!=g.cur.selected_raw)g.cur.from_link=false;
  bool cached_at=false;std::uint32_t cached_type=0;
  if(g.cur.actor==reinterpret_cast<std::uintptr_t>(a)&&g.cur.helm&&g.cur.helm_floor){
    const bool processed=r.alive&&a->field_0x6cc!=0&&a->mOldActionMode==g.cur.pre_mode&&(a->mActionMode==5||a->mActionMode==6);
    const bool token=g.cur.tracked&&g.cur.slot>=0&&g.eng.obs.ledger.valid(g.cur.slot,g.cur.generation)&&g.eng.obs.ledger.slot(static_cast<unsigned>(g.cur.slot)).id==g.cur.id;
    const bool fresh=g.cur.from_link&&r.selected==g.cur.selected_raw;
    if(r.alive&&token&&processed&&fresh)cached_at=read_at_type(r.selected,cached_type);
    const bool sword=cached_at&&classify_family(cached_type,g.eng.masks).family==Family::Sword;
    const bool confirmed=post06::helm_contact_verified(r.alive,token,processed,fresh,sword);
    if(confirmed){auto& h=g.helm_episodes[g.cur.slot];h.seen=true;h.link_id=g.cur.link_id;}
    const auto hp=post06::normalize_helm(g.cur.pre_hp,r.alive?a->health:g.cur.pre_hp,g.cur.helm_floor,confirmed);
    if(hp.write&&!g.reentered&&!g.rx.session_disabled()){write_vitality(a,hp.hp,hp.lethal);if(a->health!=hp.hp||(hp.lethal&&a->attention_info.flags!=0))g.rx.close_session();if(hp.lethal&&a->mActionMode!=6){auto t=read_live(a);t.mode=6;t.state=0;restore_reaction(a,t);}}
    if(g.cur.mask_owned){auto* lp=dComIfGp_getLinkPlayer();if(lp&&lp->profname==fpcNm_ALINK_e&&fopAcM_GetID(lp)==g.cur.link_id&&lp->mCutType==8)helm_set_cut(lp,5);else g.rx.close_session();}
  }
  r.alive = static_cast<int>(a->profname) == static_cast<int>(fpcNm_E_OC_e) && a->state.init_state >= 0 && a->state.init_state != 3;
  if (r.alive) {
    read_core(a, r.snap);
    r.aux = read_aux(a);
  }
  g.cur.continuation=lnks1fix::continuation(g.cur.pre_live.mode,g.cur.pre_live.state,g.cur.pre_aux,r.aux);
  Events ev;
  HitReport rep;
  const bool has_event = g.pe.on_post(reinterpret_cast<std::uintptr_t>(a), r,
      [&](std::uintptr_t selected,std::uint32_t& type){if(cached_at&&selected==r.selected){type=cached_type;return true;}return read_at_type(selected,type);},ev,rep);
  emit_events(ev);
  if (has_event) emit_hit(rep);
  drain_posture();
  if (has_event) {
    if (g.layout < 0) g.layout = layout_matches_binary(a) ? 1 : 0;
    if (g.layout == 0 && !g.rx.session_disabled()) g.rx.close_session();  // layout distinto del binario: cerrado (se informa en el resumen)
    post05::Apply ap = post05::Apply::NotAttempted;
    const post05::DecisionResult d = post05::process(g.rx, g.pe, rep, [a] { return read_live(a); },
                                                     [a](const post05::LiveMode& target) { restore_reaction(a, target); }, ap, g.cur.continuation==lnks1fix::Continuation::Safe);
    emit_decision(rep, d, ap);
    if(g.cur.tracked&&g.cur.slot>=0){
      const bool unsafe=d.reason==post05::Reason::UnsafeContinuation;
      if(ap==post05::Apply::Applied)fix01_cleanup_request();
      if(ap==post05::Apply::Applied||unsafe)fx.ai.intervention(g.cur.slot,g.cur.id,g.cur.generation,g.p8.tick,ap==post05::Apply::Applied,unsafe);
      if((unsafe||ap==post05::Apply::Applied)&&lnk_take(fx.ai_context_lines,fx.ai_context_dropped,96)){char line[kLogLineMax];std::snprintf(line,sizeof(line),"LNK_S1_FIX01_AI actor_id=%u generation=%llu intervention=%s reason=%s pre=%d/%d post=%d/%d frame=%.2f rate=%.2f attr=%u native_reaction=%s",g.cur.id,static_cast<unsigned long long>(g.cur.generation),unsafe?"SUPPRESSION_EXCLUDED":"SUPPRESSION_APPLIED",lnks1fix::name(g.cur.continuation),rep.pre.mode,rep.pre.state,rep.post.mode,rep.post.state,g.cur.pre_aux.anim_frame,g.cur.pre_aux.anim_rate,unsigned(g.cur.pre_aux.anim_attr),unsafe?"RETAINED":"SUPPRESSED");lnk_write(LOG_LEVEL_INFO,line);}
    }
    if (ap == post05::Apply::Applied && g.cur.tracked && g.cur.slot >= 0)  // FIX02: vigilar la IA tras cada supresion aplicada (solo lectura)
      post08_watch_out(g.watch.start(g.cur.slot, g.cur.id, g.cur.generation, g.p8.tick, post08::AiWatch::Cause::Suppression,
                                     r.aux.resource == static_cast<std::uint8_t>(NameKind::E_OC2)));
  }
  bool parry_crossed = false;
  if (r.alive) parry_crossed = post08_step(a, r);
  else if (g.cur.pv.result == post08::Result::Confirmed) g.parry.commit(g.cur.pin.slot, g.cur.pin.generation, g.cur.pin.id, g.cur.pv.token, false);
  lnk01_contact();  // LNK01: postura PROPIA de Link desde el MISMO veredicto POST08 (ya comprometido); solo el contador privado
  const int handoff = post08_break_handoff();  // FIX02
  if (r.alive) post06_step(a, r, has_event, rep, parry_crossed || handoff == 2, handoff == 1);
  // FIX02: tras TODAS las escrituras de esta llamada, la esfera At quedara armada en executeAttack si y solo si ATTACK 1|2 con el clip en [14,22].
  if (g.cur.tracked && g.cur.slot >= 0 && g.cur.actor == reinterpret_cast<std::uintptr_t>(a)) {
    const post05::LiveMode fin = r.alive ? read_live(a) : post05::LiveMode{};
    g.arm.note_post(g.cur.slot, g.cur.id, g.cur.generation, g.p8.tick,
                    r.alive && post08::ArmTrack::armed_now(fin.mode, fin.state, r.aux.anim_ok != 0, r.aux.anim_frame));
  }
}

HookAction on_pre(ModContext*,void* args,void*,void*) noexcept {
  if(!args||g.eng.obs.disabled)return HOOK_CONTINUE;
  if(g.depth++){g.reentered=true;g.rx.close_session();return HOOK_CONTINUE;}
  g.reentered=false;
  try{auto* a=mods::arg<daE_OC_c*>(args,0);if(a)do_pre(a);}catch(...){g.eng.obs.disable("EXCEPTION_IN_PRE");}
  return HOOK_CONTINUE;
}
void on_post(ModContext*,void* args,void*,void*) noexcept {
  if(!g.depth)return;
  if(g.depth>1){--g.depth;return;}
  try{auto* a=args?mods::arg<daE_OC_c*>(args,0):nullptr;if(a)do_post(a);}catch(...){g.eng.obs.disable("EXCEPTION_IN_POST");}
  g.depth=0;
}

// ---- consulta de existencia (SOLO en mod_update) -----------------------------------------------------------------------
// fopAcM_SearchByID(id,&out): fpcM_IsCreating(id) y, si no, recorrido de solo lectura de g_fopAcTg_Queue con fpcSch_JudgeByID.
// Todo actor entra en esa cola en su primera llamada a fopAc_Create y sale en fopAc_Delete tras el delete del actor
// (f_op_actor.cpp:453-454 / 421): ausente + no en creacion = actor ya borrado. `out` es un puntero VIVO recien devuelto, nunca el guardado.
Lookup lookup_actor(const Pick& p) noexcept {
  try {
    fopAc_ac_c* out = nullptr;
    const s32 rc = fopAcM_SearchByID(static_cast<fpc_ProcID>(p.id), &out);
    if (rc == 0) return Lookup::Absent;
    if (!out) return Lookup::Creating;
    if (reinterpret_cast<std::uintptr_t>(out) != p.ptr) return Lookup::PresentMismatchPtr;
    if (static_cast<int>(out->profname) != static_cast<int>(fpcNm_E_OC_e) || static_cast<std::uint32_t>(fopAcM_GetID(out)) != p.id)
      return Lookup::PresentMismatchProfile;
    return Lookup::PresentMatch;
  } catch (...) {
    return Lookup::Failed;
  }
}

// POST05-FINALIZE: UNICA consulta de servicio ajeno del modulo, SOLO de existencia y SOLO en mod_update (hilo del juego, fuera del hook).
// Bokoblin Expansion 0.17.0 importa este servicio de EVF como OBLIGATORIO; el SDK (mods/api.h) no carga un mod con un import obligatorio sin
// resolver. Por tanto "no publicado" => Expansion no esta activo => nadie decide x1.15/x2 alrededor de damage_check. El puntero devuelto nunca se
// usa: solo se compara con nulo. Cualquier otro resultado, excepcion o host sin get_service => EXPANSION_POSSIBLE. La presencia es pegajosa.
constexpr const char* kVariantAiRequiredService = "dev.devf.encounter_placement.v11";
constexpr unsigned kEnvProbePeriod = 30;
void probe_variant_ai_env() noexcept {
  if (g.env_possible || (g.env_ticks++ % kEnvProbePeriod) != 0) return;
  bool absent = false;
  try {
    const void* found = nullptr;
    const ModResult rc = svc_host->get_service ? svc_host->get_service(mod_ctx, kVariantAiRequiredService, 1, 0, &found) : MOD_ERROR;
    absent = rc == MOD_UNAVAILABLE && found == nullptr;
  } catch (...) {
    absent = false;
  }
  ++g.env_queries;
  const post05::E2Env before = g.rx.e2_env;
  if (!absent) g.env_possible = true;
  g.rx.e2_env = absent ? post05::E2Env::Absent : post05::E2Env::Possible;
  if (g.rx.e2_env != before || g.env_queries == 1) {
    if (g.env_lines >= 4) return;
    ++g.env_lines;
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "POST05_E2_ENV env=%s queries=%u sticky=%d basis=REQUIRED_IMPORT_OF_VARIANT_AI_ABSENT_OR_NOT", name(g.rx.e2_env), g.env_queries,
                  g.env_possible ? 1 : 0);
    emit(LogCat::Other, LOG_LEVEL_INFO, line);
  }
}

void do_update() noexcept {
  probe_variant_ai_env();
  post08_update();
  post08_observe_link();
  lnk01_update();  // LNK01: identidad de Link, regeneracion por tick de simulacion, medicion de rupturas nativas (solo lectura del juego)
  post08_watch_out(g.watch.expire(g.p8.tick));
  (void)fx.ai.expire_one(g.p8.tick);
  Observer& o = g.eng.obs;
  o.epoch.update(stage_token(dComIfGp_getStartStageName()));
  if (o.sweep_due()) {  // barrido de existencia por presupuesto (cada 10 mod_update): NO es el reloj de la postura
    for (unsigned q = 0; q < kSweepQuota; ++q) {
      Pick p;
      if (!o.ledger.next_sweep(p, o.sweep_stamp)) break;
      const Lookup r = lookup_actor(p);
      Events ev;
      o.ledger.apply_lookup(p, r, o.epoch.epoch, ev);
      emit_events(ev);
    }
  }
  g.pe.on_update();  // UN tick matematico por mod_update, siempre DESPUES del barrido (una retirada no recupera en ese tick)
  drain_posture();
}

const char* result_text(int r) noexcept {
  switch (r) {
    case MOD_OK: return "MOD_OK";
    case MOD_ERROR: return "MOD_ERROR";
    case MOD_UNAVAILABLE: return "MOD_UNAVAILABLE";
    case MOD_UNSUPPORTED: return "MOD_UNSUPPORTED";
    case MOD_CONFLICT: return "MOD_CONFLICT";
    case MOD_INVALID_ARGUMENT: return "MOD_INVALID_ARGUMENT";
    default: return "MOD_OTHER";
  }
}

// Diagnostico de solo lectura del manifiesto de simbolos (no es un hook ni instala nada).
void log_target_resolution() noexcept {
  char line[kLogLineMax];
  void* addr = nullptr;
  HookSymbolFlags flags = static_cast<HookSymbolFlags>(0);
  int r = MOD_ERROR;
  try {
    r = static_cast<int>(svc_hook->resolve(mod_ctx, "daE_OC_c::damage_check", &addr, &flags));
  } catch (...) {
    r = MOD_ERROR;
  }
  void* declared = EocDamageCheckPost05::resolved_target();
  const unsigned f = static_cast<unsigned>(flags);
  std::snprintf(line, sizeof(line),
                "POST03_TARGET resolve=%s declared_target_resolved=%d same_address=%d flags=0x%02x inline_sites_flag=%d "
                "multi_name_flag=%d (inline_sites_flag=1 => NOT_APPROVED)",
                result_text(r), declared ? 1 : 0, (declared && addr == declared) ? 1 : 0, f, (f & 0x20u) ? 1 : 0,
                (f & static_cast<unsigned>(HOOK_SYMBOL_MULTI_NAME)) ? 1 : 0);
  emit(LogCat::Other, (f & 0x20u) ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}

// ======================================== POST07: posture HUD (DISPLAY ONLY) ==========================================================
// Frame model (Enemy Health Bars CP06 notes, d_drawlist/f_pc sources): a simulation tick runs actor execute (damage_check) and then every draw
// handler (dMeter2_c::_draw included) while the 2D lists accept set(); presentation-only frames call _draw again (set() is a no-op) and the painter
// replays the lists of the LAST simulation tick. So: sample + queue in the first _draw POST after each _execute POST; the packet only paints value
// copies (hg.frame) and never touches an actor. Nothing here writes to an actor, to the posture, to POST05 or POST06: the only game memory this
// block touches is OUR packet pointer inside the 2D XLU list (dDlst_list_c::set through set2DXlu, and its removal in hud_drop_all).
constexpr unsigned kHudScanCap = 4096;  // bound on the live actor queue walk (read only)
constexpr unsigned kHudLogLines = 8;    // HUD lines outside the summaries
struct HudGlue {
  post07::Hud hud;
  post07::Anchors anchors;
  post07::Frame frame;
  post07::Sample samples[post07::kSlots]{};
  std::uint64_t recorded_key[post07::kSlots] = {};
  bool active = false, visible = false, interp_ok = false;
  lnk01::Visual player;    // LNK01: copia por valor de la barra de Link de este tick (el pintor solo lee esto)
  bool player_on = false;
  std::uint32_t sim_tick = 1, last_pass_tick = 0;
  unsigned lines = 0;
  std::uint64_t meter_executes = 0, meter_draws = 0, meter_deletes = 0, passes = 0, scanned_max = 0, matched_max = 0;
} hg;

void hud_line(const char* line, bool warn = false) noexcept {
  if (hg.lines >= kHudLogLines) return;
  ++hg.lines;
  emit(LogCat::Other, warn ? LOG_LEVEL_WARN : LOG_LEVEL_INFO, line);
}

const void* hud_interp_key(std::uint64_t key) noexcept { return reinterpret_cast<const void*>(0x7B07000000000000ull | (key & 0xFFFFFFFFFFFFull)); }
void hud_interp_record(std::uint64_t key, const post07::P3& w) noexcept {
  if (!hg.interp_ok || !key) return;
  InterpMtx m = {{1, 0, 0, w.x}, {0, 1, 0, w.y}, {0, 0, 1, w.z}};  // pure translation: the interpolation is a straight line
  svc_interp->record_mtx_keyed(mod_ctx, m, hud_interp_key(key));
}
void hud_interp_forget(std::uint64_t key) noexcept {
  if (hg.interp_ok && key) svc_interp->forget_mtx(mod_ctx, hud_interp_key(key));
}
bool hud_interp_lookup(std::uint64_t key, post07::P3& w) noexcept {
  if (!hg.interp_ok || !key) return false;
  InterpMtx m;
  if (!svc_interp->lookup_replacement_mtx(hud_interp_key(key), m)) return false;
  const post07::P3 r{m[0][3], m[1][3], m[2][3]};
  if (!post07::isfin(r.x) || !post07::isfin(r.y) || !post07::isfin(r.z)) return false;
  w = r;
  return true;
}

// Same conditions as the native HUD and as Enemy Health Bars: player alive, no pause, no event, meter not hidden, no menu window.
bool hud_allows(const dMeter2_c* meter) noexcept {
  return meter && g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA().getLife() != 0 && !g_dComIfG_gameInfo.play.isPauseFlag() &&
         !g_dComIfG_gameInfo.play.getEvent()->runCheck() && !(meter->mStatus & (0x4000 | 0x40 | 0x100)) && dMeter2Info_getWindowStatus() == 0;
}
bool hud_draw_allows() noexcept { return hud_allows(dMeter2Info_getMeterClass()) && dComIfGd_getView() != nullptr; }

bool hud_project(const post07::P3& w, post07::P3& o) noexcept {
  if (!dComIfGd_getView()) return false;
  Vec in{w.x, w.y, w.z}, res{};
  mDoLib_project(&in, &res);  // returns the painter's ortho-space point (x already offset by getMinXF)
  o = post07::P3{res.x, res.y, res.z};
  return true;
}
// Camera line of sight to the anchor, stopping 45 units short (read-only background query, as Enemy Health Bars).
bool hud_clear_line(const post07::P3& w) noexcept {
  if (!g_dComIfG_gameInfo.play.getCamera(0)) return true;
  dCamera_c* cam = dCam_getBody();
  if (!cam) return true;
  cXyz eye = cam->iEye();
  const cXyz delta(w.x - eye.x, w.y - eye.y, w.z - eye.z);
  const float distance = delta.abs();
  if (!(distance >= 1.0f) || !post07::isfin(distance)) return true;
  const float k = (distance - 45.0f) / distance;
  cXyz end(eye.x + delta.x * k, eye.y + delta.y * k, eye.z + delta.z * k);
  return !cam->lineBGCheck(&eye, &end, 0x40b7);
}

void hud_packet_paint() noexcept;
class HudPacket final : public dDlst_base_c {
 public:
  void draw() override { hud_packet_paint(); }
};
HudPacket hud_packet;

void hud_drop_all() noexcept {
  hg.visible = false;
  hg.frame.clear();
  for (unsigned i = 0; i < post07::kSlots; ++i) {
    hud_interp_forget(hg.recorded_key[i]);
    hg.recorded_key[i] = 0;
  }
  hg.hud.clear_all();
  hg.anchors.clear_all();
  hg.hud.c.scrubbed += post07::scrub_all(g_dComIfG_gameInfo.drawlist, static_cast<const dDlst_base_c*>(&hud_packet));
}

// LNK01: geometria de la barra de Link en el viewport ACTUAL (registro acotado cuando cambia; nada se escribe en el juego).
void hud_layout_note() noexcept {
  const float width = mDoGph_gInf_c::getWidthF(), height = mDoGph_gInf_c::getHeightF(), minx = mDoGph_gInf_c::getMinXF(), miny = mDoGph_gInf_c::getMinYF();
  if (width == lg.last_w && height == lg.last_h && minx == lg.last_minx) return;
  lg.last_w = width, lg.last_h = height, lg.last_minx = minx;
  ++lg.layout_changes;
  if (lg.layout_lines >= 6) return;
  ++lg.layout_lines;
  const lnk01::Layout L = lnk01::layout(minx, miny, width, height);
  const float px = L.ok ? 1.0f / L.u : 0.0f;  // pixeles de referencia (alto 1080) por unidad orto
  char line[kLogLineMax];
  std::snprintf(line, sizeof(line),
                "LNK01_HUD_LAYOUT viewport_ortho=%.1fx%.1f min=(%.1f,%.1f) normalized=(%.2f,%.2f) width_norm=%.2f_of_16:9_safe bar_units=%.1fx%.1f bar_px_at_1080p=%.0fx%.0f "
                "enemy_bar_units=%.0f layer=2D_XLU_AFTER_OPA_TOP(native_HUD_then_prompts_then_this_packet) ok=%d",
                width, height, minx, miny, lnk01::policy::kCenterX, lnk01::policy::kCenterY, lnk01::policy::kWidthOfSafe, L.half_w * 2.0f, L.frame_h * 2.0f,
                L.half_w * 2.0f * px, L.frame_h * 2.0f * px, lnk01::policy::kEnemyBarWidthUnits, L.ok ? 1 : 0);
  lnk_write(LOG_LEVEL_INFO, line);
}

// One simulation pass: match the live E_OC actors of THIS tick (read-only walk of the actor queue) to their ledger slot by address AND id, copy
// their posture/phase, compute the EHB-equivalent anchor and visibility, animate, publish value records and queue the packet.
void hud_sim_pass(dMeter2_c* meter) noexcept {
  ++hg.passes;
  const bool allowed = hud_allows(meter) && !g.eng.obs.disabled;
  for (unsigned i = 0; i < post07::kSlots; ++i) hg.samples[i] = post07::Sample{};
  unsigned scanned = 0, matched = 0;
  if (allowed) {
    fopAc_ac_c* player = g_dComIfG_gameInfo.play.getPlayer(0);
    const float width = mDoGph_gInf_c::getWidthF(), height = mDoGph_gInf_c::getHeightF(), minx = mDoGph_gInf_c::getMinXF(), miny = mDoGph_gInf_c::getMinYF();
    for (node_class* node = g_fopAcTg_Queue.mpHead; node && scanned < kHudScanCap; node = node->mpNextNode) {
      ++scanned;
      fopAc_ac_c* act = static_cast<fopAc_ac_c*>(reinterpret_cast<create_tag_class*>(node)->mpTagData);
      if (!act || act == player || static_cast<int>(act->profname) != static_cast<int>(fpcNm_E_OC_e)) continue;
      const std::uint32_t id = static_cast<std::uint32_t>(fopAcM_GetID(act));
      const std::uintptr_t ptr = reinterpret_cast<std::uintptr_t>(act);
      int slot = -1;
      for (unsigned i = 0; i < post03::kCapacity && slot < 0; ++i) {
        const Instance& L = g.eng.obs.ledger.slot(i);
        if (L.life == Life::Active && L.id == id && L.ptr == ptr) slot = static_cast<int>(i);
      }
      if (slot < 0) continue;  // not tracked by the posture system (65th actor, unadmitted, unverified...): no bar
      const unsigned i = static_cast<unsigned>(slot);
      const Instance& L = g.eng.obs.ledger.slot(i);
      const post04::Entry& e = g.pe.px.entry(i);
      if (!e.bound || e.generation != L.generation || e.id != id) continue;
      ++matched;
      post07::Sample& smp = hg.samples[i];
      smp.in.present = true;
      smp.in.id = id;
      smp.in.generation = L.generation;
      smp.in.posture = e.posture;
      smp.in.latched = e.latched;
      smp.in.dead = e.dead || act->health <= 0;
      smp.in.frozen = e.frozen;
      const post06::Breaker::Slot& bs = g.br.slot(i);
      smp.in.phase = (bs.bound && bs.id == id && bs.generation == L.generation) ? static_cast<post07::Ph>(static_cast<std::uint8_t>(bs.phase)) : post07::Ph::Unknown;
      const post07::P3 pos{act->current.pos.x, act->current.pos.y, act->current.pos.z}, eye{act->eyePos.x, act->eyePos.y, act->eyePos.z},
          attn{act->attention_info.position.x, act->attention_info.position.y, act->attention_info.position.z};
      if (!post07::isfin(pos.x) || !post07::isfin(pos.y) || !post07::isfin(pos.z)) continue;
      smp.anchor = hg.anchors.update(i, id, L.generation, pos, eye, attn);
      const bool eligible = !(act->actor_status & 0x1000000u) && !(act->actor_condition & 4u);
      const bool targetable = (act->attention_info.flags & 7u) != 0;
      post07::P3 o{};
      const bool onscreen = hud_project(smp.anchor, o) && post07::on_screen(post07::P3{o.x - minx, o.y - miny, o.z}, width, height);
      const bool clear = onscreen && targetable && hud_clear_line(smp.anchor);
      smp.distance = (onscreen && targetable && clear && player) ? fopAcM_searchActorDistance(player, act) : 4500.0f;
      smp.in.opacity = eligible ? post07::opacity(onscreen, targetable, clear, smp.distance) : 0.0f;
    }
  }
  if (scanned > hg.scanned_max) hg.scanned_max = scanned;
  if (matched > hg.matched_max) hg.matched_max = matched;
  post07::publish(hg.hud, hg.samples, allowed, hg.sim_tick, hg.frame);
  // interpolation keys: forget the key of every slot whose identity ended; record the anchors that will be painted
  for (unsigned i = 0; i < post07::kSlots; ++i) {
    const std::uint64_t key = hg.hud.slot(i).bound ? hg.hud.slot(i).key : 0;
    if (hg.recorded_key[i] && hg.recorded_key[i] != key) {
      hud_interp_forget(hg.recorded_key[i]);
      hg.recorded_key[i] = 0;
    }
  }
  for (unsigned k = 0; k < hg.frame.n; ++k) {
    const post07::DrawRec& rec = hg.frame.rec[k];
    for (unsigned i = 0; i < post07::kSlots; ++i)
      if (hg.hud.slot(i).bound && hg.hud.slot(i).key == rec.key) {
        hud_interp_record(rec.key, rec.anchor);
        hg.recorded_key[i] = rec.key;
      }
  }
  // LNK01: barra PROPIA de Link (display only): copia por valor del contador privado de LNK01; ningun actor, ninguna postura enemiga.
  lg.hud.tick(lg.posture.milli(), allowed, lg.link_human);
  hg.player = lg.hud.visual();
  hg.player_on = allowed && lg.hud.drawable();
  if (hg.player_on) hud_layout_note();
  hg.visible = lnk01::packet_needed(allowed, hg.frame.n, hg.player_on);
  if (!hg.visible) return;
  switch (post07::queue_after_hud(g_dComIfG_gameInfo.drawlist, static_cast<const dDlst_base_c*>(meter->mpMeterDraw), static_cast<dDlst_base_c*>(&hud_packet))) {
    case post07::Queue::Queued: ++hg.hud.c.queued; break;
    case post07::Queue::Refused: ++hg.hud.c.refused; break;
    default: ++hg.hud.c.hud_not_queued; break;
  }
  if (hg.hud.c.queued == 1 && hg.lines < kHudLogLines) {
    char line[kLogLineMax];
    std::snprintf(line, sizeof(line), "POST07_HUD_FIRST_QUEUE bars=%u interp=%s scanned=%u matched=%u", hg.frame.n, hg.interp_ok ? "PRESENT" : "ABSENT", scanned, matched);
    hud_line(line);
  }
}

JUtility::TColor hud_color(post07::Rgba c) noexcept { return JUtility::TColor(c.r, c.g, c.b, c.a); }

// Painter callback: runs in the 2D pass of every rendered frame. Uses ONLY hg.frame (value copies) plus the camera projection.
void hud_packet_paint() noexcept {
  ++hg.hud.c.packet_draws;
  try {
    if (!hg.active || !hg.visible || (hg.frame.n == 0 && !hg.player_on) || !hud_draw_allows()) {
      ++hg.hud.c.draw_skipped;
      return;
    }
    const float width = mDoGph_gInf_c::getWidthF(), height = mDoGph_gInf_c::getHeightF(), minx = mDoGph_gInf_c::getMinXF(), miny = mDoGph_gInf_c::getMinYF();
    if (!(width > 0.0f && height > 0.0f)) {
      ++hg.hud.c.draw_skipped;
      return;
    }
    J2DOrthoGraph ortho(0, 0, FB_WIDTH, FB_HEIGHT, -1, 1);  // same set-up as the native painter: physical viewport, widescreen logical bounds
    ortho.setOrtho(minx, miny, width, height, -1, 1);
    ortho.setPort();
    post07::Quad quads[post07::kMaxQuads];
    for (unsigned k = 0; k < hg.frame.n; ++k) {
      const post07::DrawRec& rec = hg.frame.rec[k];
      post07::P3 world = rec.anchor;
      if (hud_interp_lookup(rec.key, world)) ++hg.hud.c.interp_hits;
      else ++hg.hud.c.interp_misses;
      post07::P3 pt{};
      if (!hud_project(world, pt) || !post07::on_screen(post07::P3{pt.x - minx, pt.y - miny, pt.z}, width, height)) continue;
      const unsigned n = post07::build(rec.vis, pt.x, pt.y, quads, post07::kMaxQuads);
      for (unsigned q = 0; q < n; ++q) {
        const post07::J2DArgs c = post07::j2d_args(quads[q]);
        ortho.setColor(hud_color(c.tl), hud_color(c.tr), hud_color(c.br), hud_color(c.bl));
        ortho.fillBox(JGeometry::TBox2<f32>(quads[q].x0, quads[q].y0, quads[q].x1, quads[q].y1));
      }
      ++hg.hud.c.painted;
      hg.hud.c.quads += n;
    }
    // LNK01: la barra de Link, la ULTIMA (encima de las barras enemigas), desde la copia hg.player y la geometria normalizada del viewport actual.
    if (hg.player_on) {
      post07::Quad lq[lnk01::kMaxQuads];
      const unsigned ln = lnk01::build_bar(hg.player, lnk01::layout(minx, miny, width, height), lq, lnk01::kMaxQuads);
      for (unsigned q = 0; q < ln; ++q) {
        const post07::J2DArgs c = post07::j2d_args(lq[q]);
        ortho.setColor(hud_color(c.tl), hud_color(c.tr), hud_color(c.br), hud_color(c.bl));
        ortho.fillBox(JGeometry::TBox2<f32>(lq[q].x0, lq[q].y0, lq[q].x1, lq[q].y1));
      }
      ++lg.player_frames;
      lg.player_quads += ln;
    }
    if (J2DGrafContext* current = dComIfGp_getCurrentGrafPort()) {
      current->setPort();
      current->setup2D();
    }
  } catch (...) {
    hg.active = false;  // inert from now on; the packet is scrubbed on the next meter delete / shutdown
  }
}

void on_meter_execute_post(ModContext*, void* args, void*, void*) noexcept {
  if (args) action_ui_step(mods::arg<dMeter2_c*>(args, 0), 0);  // LNK01: tras toda la logica del HUD de este tick (prompts ocultos, independiente de la barra)
  if (!hg.active) return;
  ++hg.meter_executes;
  ++hg.sim_tick;
}
void on_meter_draw_post(ModContext*, void* args, void*, void*) noexcept {
  if (args) action_ui_step(mods::arg<dMeter2_c*>(args, 0), 1);  // LNK01: antes de que la lista 2D se pinte (tambien en frames de presentacion)
  if (!hg.active || !args) return;
  ++hg.meter_draws;
  if (hg.last_pass_tick == hg.sim_tick) return;  // presentation-only frame: the packet replays the last simulation tick
  hg.last_pass_tick = hg.sim_tick;
  try {
    hud_sim_pass(mods::arg<dMeter2_c*>(args, 0));
  } catch (...) {
    hg.active = false;
    hud_drop_all();
    hud_line("POST07_HUD_DISABLED reason=EXCEPTION_IN_HUD_PASS gameplay_untouched=1", true);
  }
}
HookAction on_meter_delete_pre(ModContext*, void*, void*, void*) noexcept {
  ++hg.meter_deletes;
  lg.ui.forget();  // LNK01: nada ligado a este HUD sobrevive a su destruccion
  try {
    hud_drop_all();  // the HUD meter goes away: our packet must not stay in the lists
  } catch (...) {
    hg.active = false;
  }
  return HOOK_CONTINUE;
}

void emit_hud_summary(const char* tag) noexcept {
  const post07::HudCounters& c = hg.hud.c;
  char line[kLogLineMax * 4];
  std::snprintf(line, sizeof(line),
                "POST07_HUD_SUMMARY v=%s tag=%s active=%d interp=%s meter_exec=%llu meter_draw=%llu meter_delete=%llu passes=%llu binds=%llu resets=%llu "
                "cleared_missing=%llu cleared_dead=%llu pulses=%llu hits=%llu recoveries=%llu frozen_hidden=%llu paused=%llu published=%llu queued=%llu refused=%llu "
                "hud_not_queued=%llu packet_draws=%llu skipped=%llu painted=%llu quads=%llu over_budget=%llu max_visible=%llu interp_hit=%llu interp_miss=%llu "
                "scrubbed=%llu scanned_max=%llu matched_max=%llu",
                post07::kHudVersion, tag, hg.active ? 1 : 0, hg.interp_ok ? "PRESENT" : "ABSENT", static_cast<unsigned long long>(hg.meter_executes),
                static_cast<unsigned long long>(hg.meter_draws), static_cast<unsigned long long>(hg.meter_deletes), static_cast<unsigned long long>(hg.passes),
                static_cast<unsigned long long>(c.binds), static_cast<unsigned long long>(c.identity_resets), static_cast<unsigned long long>(c.cleared_missing),
                static_cast<unsigned long long>(c.cleared_dead), static_cast<unsigned long long>(c.pulses), static_cast<unsigned long long>(c.hits),
                static_cast<unsigned long long>(c.recoveries), static_cast<unsigned long long>(c.frozen_hidden), static_cast<unsigned long long>(c.paused_ticks),
                static_cast<unsigned long long>(c.published), static_cast<unsigned long long>(c.queued), static_cast<unsigned long long>(c.refused),
                static_cast<unsigned long long>(c.hud_not_queued), static_cast<unsigned long long>(c.packet_draws), static_cast<unsigned long long>(c.draw_skipped),
                static_cast<unsigned long long>(c.painted), static_cast<unsigned long long>(c.quads), static_cast<unsigned long long>(c.over_budget),
                static_cast<unsigned long long>(c.max_visible), static_cast<unsigned long long>(c.interp_hits), static_cast<unsigned long long>(c.interp_misses),
                static_cast<unsigned long long>(c.scrubbed), static_cast<unsigned long long>(hg.scanned_max), static_cast<unsigned long long>(hg.matched_max));
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
}

void emit_summary(const char* tag) noexcept {
  Observer& o = g.eng.obs;
  char line[kLogLineMax * 4];
  fmt_summary(line, sizeof(line), o.ledger.c, o.ledger.active(), o.epoch.epoch, o.frames.pending_started(), tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  fmt_hit_summary(line, sizeof(line), o.ledger.c, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  fmt_collider_diag(line, sizeof(line), o.ledger.c, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  const Counters& c = o.ledger.c;
  std::snprintf(line, sizeof(line),
                "POST03_CONSERVATION v=0.4.0 tag=%s ok=%d started=%llu judged=%llu dup_serial_gate=%llu dup_post_orphan=%llu cross_generation=%llu "
                "discarded=%llu pending=%u",
                tag, g.eng.conserved() ? 1 : 0, static_cast<unsigned long long>(c.cand_started + c.finish_started),
                static_cast<unsigned long long>(g.eng.judged()), static_cast<unsigned long long>(c.dup_serial_gate),
                static_cast<unsigned long long>(c.dup_post_orphan), static_cast<unsigned long long>(c.cross_generation_drops),
                static_cast<unsigned long long>(c.cand_discarded + c.finish_discarded), o.frames.pending_started());
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  const post04::AuditResult au = g.pe.px.audit();
  post04::fmt_summary(line, sizeof(line), g.pe.px, au, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  post04::fmt_conservation(line, sizeof(line), g.pe.px, au, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  post05::fmt_summary(line, sizeof(line), g.rx, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  std::snprintf(line, sizeof(line), "POST05_LAYOUT tag=%s layout=%s", tag, g.layout < 0 ? "UNCHECKED" : (g.layout ? "MATCHES_EXE" : "MISMATCH_SUPPRESSION_CLOSED"));
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  post05::fmt_conservation(line, sizeof(line), g.rx, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  post06::fmt_summary(line, sizeof(line), g.br, tag);
  emit(LogCat::Summary, LOG_LEVEL_INFO, line);
  emit_hud_summary(tag);
  emit_parry_summary(tag);
  emit_lnk01_summary(tag);
  {  // LNK SESSION02
    const lnk05::Counters& bc = lb.m.c;
    const auto u = [](std::uint64_t x) { return static_cast<unsigned long long>(x); };
    const auto G = [&](lnk05::Gate x) { return u(bc.gate_refused[static_cast<unsigned>(x)]); };
    const bool inv = lg.posture.conserved() && bc.resets == bc.recovered_cancel + bc.recovered_end + bc.interrupted && lg.posture.c.break_resets == bc.resets &&
                     bc.entered + bc.native_adopted >= bc.resets;
    char s2[kLogLineMax * 4];
    std::snprintf(s2, sizeof(s2),
                  "LNK_S2_SUMMARY v=%s tag=%s state=%s enabled=%d saturating_impacts=%llu saturations(block/direct)=%llu/%llu parries_excluded=%llu "
                  "requested=%llu entered_by_mod=%llu native_adopted=%llu native_observed=%llu duplicate_requests=%llu request_rejected=%llu "
                  "cancelled_by_parry=%llu expired=%llu trigger_failed=%llu recovered(cancel/end)=%llu/%llu interrupted=%llu aborted=%llu stuck=%llu "
                  "posture_resets=%llu break_reset_milli=%llu hold_ticks=%llu active_ticks=%llu paused_active_ticks=%llu gate_checks=%llu "
                  "gate_refused(expired/link/not_human/dead/sim/event/already/proc/mode/damage)=%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu/%llu "
                  "native_calls=%llu hud_break_flashes=%llu link_writes=NATIVE_TRANSITION_ONLY(procGuardBreakInit) life_writes=0 counter_2fd5_writes=0 invariants=%d "
                  "lines(event/native/vuln/recovery/gate)=%u+%llu/%u+%llu/%u+%llu/%u+%llu/%u+%llu",
                  lnk05::kVersion, tag, lnk05::name(lb.m.state()), lb.enabled ? 1 : 0, u(bc.saturating_impacts), u(bc.saturations_block), u(bc.saturations_direct),
                  u(lg.posture.c.parries_excluded), u(bc.requested), u(bc.entered), u(bc.native_adopted), u(bc.native_observed), u(bc.duplicate_requests),
                  u(bc.request_rejected), u(bc.cancelled_by_parry), u(bc.expired), u(bc.trigger_failed), u(bc.recovered_cancel), u(bc.recovered_end),
                  u(bc.interrupted), u(bc.aborted), u(bc.stuck), u(bc.resets), u(lg.posture.c.break_reset_milli), u(lg.posture.c.hold_ticks), u(bc.active_ticks),
                  u(bc.paused_active_ticks), u(bc.gate_checks), G(lnk05::Gate::Expired), G(lnk05::Gate::LinkMismatch), G(lnk05::Gate::NotHuman),
                  G(lnk05::Gate::Dead), G(lnk05::Gate::SimInactive), G(lnk05::Gate::EventRunning), G(lnk05::Gate::AlreadyBreaking), G(lnk05::Gate::ProcNotAllowed),
                  G(lnk05::Gate::ModeFlags), G(lnk05::Gate::DamageThisCall), u(lb.native_calls), u(lg.hud.c.break_flashes), inv ? 1 : 0, lb.event_lines,
                  u(lb.event_dropped), lb.native_lines, u(lb.native_dropped), lb.vuln_lines, u(lb.vuln_dropped), lb.recovery_lines, u(lb.recovery_dropped), lb.gate_lines, u(lb.gate_dropped));
    lnk_write(inv ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, s2);
  }
  const auto&fc=fx.direct.c;const auto&ac=fx.ai.c;
  char cleanup_line[kLogLineMax];std::snprintf(cleanup_line,sizeof(cleanup_line),"LNK_S1_FIX01_AI_CLEANUP tag=%s requested=%llu leased=%llu paired=%llu next_tick_clear_verified=%llu failed=%llu expired_inconclusive=%llu",tag,static_cast<unsigned long long>(fx.cleanup.c.requested),static_cast<unsigned long long>(fx.cleanup.c.leased),static_cast<unsigned long long>(fx.cleanup.c.paired),static_cast<unsigned long long>(fx.cleanup.c.verified),static_cast<unsigned long long>(fx.cleanup.c.failed),static_cast<unsigned long long>(fx.cleanup.c.expired));lnk_write(LOG_LEVEL_INFO,cleanup_line);
  char fixline[kLogLineMax*3];std::snprintf(fixline,sizeof(fixline),"LNK_S1_FIX01_SUMMARY tag=%s native_calls=%llu native_damage=%llu native_rejected=%llu matched=%llu unmatched=%llu ambiguous=%llu duplicate_offers=%llu stale=%llu identity_lost=%llu reentrant=%llu suppressions=%llu unsafe_exclusions=%llu attack_starts=%llu actual_armed=%llu completed_cycles=%llu completed_by_variant=%llu/%llu/%llu/%llu/%llu aborts=%llu legitimate_cancel=%llu animation_restarts=%llu suspected=%llu recovered=%llu dead_before_recovery=%llu expired_inconclusive=%llu unresolved=%llu active_unverified=%u pending_anomalies=%u direct_log_dropped=%llu ai_log_dropped=%llu invariants=%d",
  tag,(unsigned long long)fc.native_calls,(unsigned long long)fc.native_damage,(unsigned long long)fc.native_rejected,(unsigned long long)fc.matched,(unsigned long long)fc.unmatched,(unsigned long long)fc.ambiguous,(unsigned long long)fc.duplicates,(unsigned long long)fc.stale,(unsigned long long)fc.identity_lost,(unsigned long long)fx.reentered,(unsigned long long)ac.suppressions,(unsigned long long)ac.unsafe,(unsigned long long)ac.starts,(unsigned long long)ac.armed,(unsigned long long)ac.completed,(unsigned long long)ac.by_variant[0],(unsigned long long)ac.by_variant[1],(unsigned long long)ac.by_variant[2],(unsigned long long)ac.by_variant[3],(unsigned long long)ac.by_variant[4],(unsigned long long)ac.aborts,(unsigned long long)ac.cancelled,(unsigned long long)ac.restarts,(unsigned long long)ac.suspected,(unsigned long long)ac.resolved,(unsigned long long)ac.dead,(unsigned long long)ac.expired,(unsigned long long)ac.unresolved,fx.ai.active(),fx.ai.pending_anomalies(),(unsigned long long)fx.direct_dropped,(unsigned long long)fx.ai_dropped,int(lg.posture.conserved()));lnk_write(LOG_LEVEL_INFO,fixline);
}

}  // namespace

extern "C" {

MOD_EXPORT ModResult mod_initialize(ModError* err) {
  char line[kLogLineMax];
  const bool log_ok = compatible(svc_log, 1, 0) && svc_log->write;
  if (!log_ok) return mods::set_error(err, MOD_UNSUPPORTED, "POST05: LogService incompatible");
  if (!mod_ctx || !compatible(svc_host, 2, 2) || !svc_host->version ||
      (std::strcmp(svc_host->version, "2.0.3") != 0 && std::strcmp(svc_host->version, "2.0.3.0") != 0)) {
    g.eng.obs.disable("HOST_NOT_2_0_3");
    std::snprintf(line, sizeof(line), "POST03_HOOK_FAILED stage=HOST_CHECK reason=HOST_NOT_2_0_3 host=%s",
                  (svc_host && svc_host->version) ? svc_host->version : "UNKNOWN");
    emit(LogCat::Other, LOG_LEVEL_ERROR, line);
    return mods::set_error(err, MOD_UNSUPPORTED, "POST05 requires Dusklight 2.0.3");
  }
  if (!compatible(svc_hook, 1, 1) || !svc_hook->install || !svc_hook->add_pre || !svc_hook->add_post || !svc_hook->dispatch_pre ||
      !svc_hook->dispatch_post || !svc_hook->uninstall || !svc_hook->resolve) {
    g.eng.obs.disable("HOOK_SERVICE_INCOMPATIBLE");
    emit(LogCat::Other, LOG_LEVEL_ERROR, "POST03_HOOK_FAILED stage=HOOK_SERVICE_CHECK reason=HOOK_SERVICE_INCOMPATIBLE");
    return mods::set_error(err, MOD_UNSUPPORTED, "POST05: HookService 1.1 required");
  }

  g.eng.masks = kMasks;  // (g.eng es g.pe.eng)
  g.eng.policy = AdmissionPolicy::V2Compat;  // POST05-COMPAT 0.5.1: E_OC 1/1|2/2 y E_OC2 {1,2,3,6} admitidos
  g.rx.e2_policy = post05::E2Policy::MatureBC;
  g.rx.stagger_policy = post05::StaggerPolicy::General;
  g.br.enabled = true;
  g.parry.enabled = true;  // POST08 0.8.0: parry de precision con Shield Attack  // POST06 0.6.0: ruptura real, castigo x1.20 fijo, caida/Finish/levantamiento nativos, rearme; recuperacion por banda de vida  // POST05-GENERAL 0.5.4: postura < 100 => sin tambaleo en cualquier modo normal  // POST05-RB 0.5.2: E_OC2 sigue las reglas de E_OC mas el gate de fase nativa (B/C maduro o frenada)
  char build_id[48] = {};
  for (unsigned i = 0; svc_host->build_id && i < svc_host->build_id_len && i < 20; ++i)
    std::snprintf(build_id + i * 2, sizeof(build_id) - i * 2, "%02x", svc_host->build_id[i]);
  std::snprintf(line, sizeof(line),
                "POST03_1_INIT id=dev.posture.post05_selective_reactions version=%s source_commit=%s host=%s host_build_id=%s passive=1 "
                "hook_target=daE_OC_c::damage_check pmf_bytes=%u capacity=%u sweep_period_ticks=%u sweep_quota=%u log_cap=%u "
                "lookup=fopAcM_SearchByID(mod_update_only) hit_signature=cooldown_set+reaction_committed collider_read=mAtInfo.mpCollider_after_confirmed_only "
                "deref_gate=eq_raw_hit_obj+align4 sizeof_cCcD_Obj=%u alignof_cCcD_Obj=%u",
                kModVersion, POST05_SOURCE_COMMIT, svc_host->version, build_id[0] ? build_id : "EMPTY",
                static_cast<unsigned>(sizeof(decltype(&daE_OC_c::damage_check))), kCapacity, kSweepPeriodTicks, kSweepQuota, kBudgetTotal,
                static_cast<unsigned>(sizeof(cCcD_Obj)), static_cast<unsigned>(alignof(cCcD_Obj)));
  emit(LogCat::Other, LOG_LEVEL_INFO, line);
  std::snprintf(line, sizeof(line),
                "POST04_INIT id=dev.posture.post05_selective_reactions version=%s source_commit=%s policy_id=%s posture_core_passive=1 "
                "hook_target=daE_OC_c::damage_check hook_pre=HOOK_CONTINUE_ALWAYS hook_post=passive capacity=%u key=slot+generation+id "
                "max_posture=%d threshold=%d weights(sword/iron/all_other)=%d/%d/0 recover_delay_ticks=%llu recover_period_ticks=%llu "
                "timebase=MOD_UPDATE_SIM_TICK timebase_nominal_hz=30 timebase_basis=source_game_clock_time_scale_1 TIMEBASE_PAUSE_UNVERIFIED balance=PROVISIONAL log_budget_posture=%u",
                post04::kPostureVersion, POST05_SOURCE_COMMIT, post04::policy::kPolicyId, post04::kCap, post04::policy::kMaxPosture, post04::policy::kThreshold,
                post04::policy::kWeightSword, post04::policy::kWeightIronBall, static_cast<unsigned long long>(post04::policy::kRecoverDelayTicks),
                static_cast<unsigned long long>(post04::policy::kRecoverPeriodTicks), post04::Posture::kLogBudget);
  emit(LogCat::Other, LOG_LEVEL_INFO, line);
  std::snprintf(line, sizeof(line),
                "POST05_INIT version=%s source_commit=%s policy_id=%s writes_to_game=1 write_set=mode+state+old+t6c0+t6c2+headlock+ccTgCo "
                "trigger=final_setActionMode_DAMAGE|BIG_DAMAGE stagger_policy=%s eligible=nonlethal_posture_after_lt_100 pre=ANY_NORMAL_MODE "
                "hook_skip_original=0 hp_untouched=1 admission_policy=%s e2_policy=%s e2_env=LOGGED_ONLY power_change=SKIP_UNSAFE",
                post05::kReactionVersion, POST05_SOURCE_COMMIT, post05::policy::kPolicyId, name(g.rx.stagger_policy), name(g.eng.policy), name(g.rx.e2_policy));
  emit(LogCat::Other, LOG_LEVEL_INFO, line);
  std::snprintf(line, sizeof(line),
                "POST06_INIT version=%s decisive_bonus=x1.50_once max_exec=%llu clock=OWN_ACTOR_EXECUTE_30HZ "
                "drop=CONFIRMED_DECISIVE_CONTACT_ONLY timeout=ON_FEET_50 finish=NATIVE getup=NATIVE rearm=after_native_getup "
                "regen_pts_per_s(>75/>50/>25/>10/>0)=5/4/3/2/1 regen_delay_ticks=%llu hp_max=OBSERVED_MAX_THIS_GENERATION hud=POST07 rearm_redirected=FIX_REARM50",
                post06::kBreakVersion,
                static_cast<unsigned long long>(post06::policy::kPunishMaxExec), static_cast<unsigned long long>(post04::policy::kRecoverDelayTicks));
  emit(LogCat::Other, LOG_LEVEL_INFO, line);
  std::snprintf(line, sizeof(line),
                "POST08_INIT version=%s parry=ON weight=%d window=[-%.0f,+%.0f]frames_of_ANM_SHIELD_ATTACK_and_<=%uticks_since_start (1_frame=1_tick_at_speed_1.0) "
                "link_state=SNAPSHOT_IN_MOD_UPDATE(state_during_previous_collision_pass) fury_black=REFUSED(rate>=%.1f) "
                "fix02=FRESH_ARMED_CONTACT_ONLY+NATIVE_DEFLECT(no_state3_when_link_guarded)+AI_WATCH_READ_ONLY",
                post08::kParryVersion, post04::policy::kParryPostureGain, post08::policy::kPreStartSlack, post08::policy::kWindowFrames,
                post08::policy::kWindowTicksCap, post08::policy::kFuryRateThreshold);
  p8_write(LOG_LEVEL_INFO, line);
  std::snprintf(line, sizeof(line),
                "POST08_INIT_DETECT same_pass=enemy_At_hit_on_live_Link_TgCyl+native_AtShieldHit+Link_PROC_GUARD_ATTACK_at_that_pass deflect=ATTACK_state3_from_PRE "
                "break=POST06_BREAK_BY_PARRY one_token_per_attack=1 hp_writes=0 link_writes=0 balance=PROVISIONAL");
  p8_write(LOG_LEVEL_INFO, line);
  log_target_resolution();

  if(mods::hook::add_post<CollisionCleanupFix01>(on_cc_post_fix01)!=MOD_OK ||
     mods::hook::add_pre<CollisionCleanupFix01>(on_cc_pre_fix01)!=MOD_OK ||
     mods::hook::add_post<LinkPolyFix01>(on_link_poly_post_fix01)!=MOD_OK ||
     mods::hook::add_post<LinkDamageFix01>(on_link_damage_post_fix01)!=MOD_OK ||
     mods::hook::add_pre<LinkDamageFix01>(on_link_damage_pre_fix01)!=MOD_OK ||
     mods::hook::add_post<AttackObserveFix01>(on_attack_post_fix01)!=MOD_OK ||
     mods::hook::add_pre<AttackObserveFix01>(on_attack_pre_fix01)!=MOD_OK)
    return mods::set_error(err,MOD_ERROR,"SESSION01 FIX01 hooks failed");
  // LNK SESSION02: SEGUNDO callback POST sobre el MISMO objetivo ya instalado (daAlink_c::checkDamageAction): ningun objetivo de hook nuevo. Se registra
  // despues del POST de FIX01 (lee su captura de este execute). Si falla, LNK05/06 quedan inertes y la postura se comporta como 0.10.1.
  const ModResult lnk05_hook = mods::hook::add_post<LinkDamageFix01>(on_link_damage_post_lnk05);
  lb.enabled = lnk05_hook == MOD_OK;
  if(mods::hook::add_post<EocShieldStunPost06>(on_stun_post)!=MOD_OK) return mods::set_error(err, MOD_ERROR, "POST06 shield stun hook failed");
  const InstallOutcome out = install_sequence(
      [] { return static_cast<int>(mods::hook::add_post<EocDamageCheckPost05>(on_post)); },
      [] { return static_cast<int>(mods::hook::add_pre<EocDamageCheckPost05>(on_pre)); },
      [] { return static_cast<int>(mods::hook::uninstall<EocDamageCheckPost05>()); });
  if (out.state != InstallState::Ok) {
    g.eng.obs.disable("HOOK_INSTALL_FAILED");  // aunque algun callback hubiese quedado registrado, no hara nada
    std::snprintf(line, sizeof(line), "POST03_HOOK_FAILED stage=%s post=%s pre=%s rollback=%s (no partial execution: callbacks disabled)",
                  name(out.state), result_text(out.post), result_text(out.pre), result_text(out.rollback));
    emit(LogCat::Other, LOG_LEVEL_ERROR, line);
    return mods::set_error(err, MOD_ERROR, "POST05: damage_check hook installation failed; mod inert");
  }
  g.installed = true;
  emit(LogCat::Other, LOG_LEVEL_INFO,
       "POST03_HOOK_INSTALLED target=daE_OC_c::damage_check post=MOD_OK pre=MOD_OK pre_returns=HOOK_CONTINUE_ALWAYS");
  // POST07: the HUD is optional for the posture system. A failed HUD hook leaves the posture system intact and every HUD callback inert.
  hg.interp_ok = compatible(svc_interp, INTERP_SERVICE_MAJOR, 0) && svc_interp->record_mtx_keyed && svc_interp->forget_mtx && svc_interp->lookup_replacement_mtx;
  const ModResult hx = mods::hook::add_post<MeterExecutePost07>(on_meter_execute_post);
  const ModResult hd = hx == MOD_OK ? mods::hook::add_post<MeterDrawPost07>(on_meter_draw_post) : MOD_ERROR;
  const ModResult hl = hd == MOD_OK ? mods::hook::add_pre<MeterDeletePost07>(on_meter_delete_pre) : MOD_ERROR;
  hg.active = hx == MOD_OK && hd == MOD_OK && hl == MOD_OK;
  std::snprintf(line, sizeof(line),
                "POST07_INIT version=%s hud=%s hooks=dMeter2_c::_execute(post)=%s,_draw(post)=%s,_delete(pre)=%s interp=%s display_only=1 writes_to_actors=0 "
                "anchor=EHB_CP06_EQUIVALENT(head+8) placement=BELOW_LIFE_BAR(sy+3.4..sy+6.0) fill=CENTER_TO_BOTH_ENDS colour=GOLD_AMBER_ORANGE_RED "
                "pulse=ONCE_PER_CROSSING slots=%u max_drawn=%u",
                post07::kHudVersion, hg.active ? "ON" : "OFF_HOOK_FAILED", result_text(hx), result_text(hd), result_text(hl), hg.interp_ok ? "PRESENT" : "ABSENT",
                post07::kSlots, post07::kMaxDrawn);
  emit(LogCat::Other, hg.active ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, line);
  // LNK01: la supresion de los prompts necesita los hooks del HUD (execute/draw POST, delete PRE); la mascara de brillos necesita el hook del draw del
  // prompt. El POST se instala ANTES que el PRE: un PRE sin su POST nunca puede quedar enmascarando. Cada parte falla cerrada por separado.
  lg.ui.enabled = hx == MOD_OK && hd == MOD_OK && hl == MOD_OK;
  ModResult hq = MOD_ERROR, hp = MOD_ERROR;
  if (lg.ui.enabled) {
    hq = mods::hook::add_post<PromptDrawLnk01>(on_prompt_draw_post);
    hp = hq == MOD_OK ? mods::hook::add_pre<PromptDrawLnk01>(on_prompt_draw_pre) : MOD_ERROR;
  }
  lg.sparkle_hook = hq == MOD_OK && hp == MOD_OK;
  void* paddr = nullptr;
  HookSymbolFlags pflags = static_cast<HookSymbolFlags>(0);
  int pres = MOD_ERROR;
  try {
    pres = static_cast<int>(svc_hook->resolve(mod_ctx, "dMeterButton_c::draw", &paddr, &pflags));
  } catch (...) {
    pres = MOD_ERROR;
  }
  char wide[kLogLineMax * 2];
  std::snprintf(wide, sizeof(wide),
                "LNK01_INIT version=%s source_commit=%s host=%s player_posture=ON max=%d block_gain(blue/red/black)=%d/%d/%d parry_cost=%d regen_delay_ticks=%u "
                "regen_pts_per_s=%d timebase=MOD_UPDATE_SIM_TICK_30HZ hud=%s bar=centre(%.2f,%.2f)_width_%.2f_of_16:9_safe button_hud=%s sparkle_mask=%s "
                "prompt_draw_hook(post/pre)=%s/%s resolve(dMeterButton_c::draw)=%s flags=0x%02x native_guard_break=UNTOUCHED(LNK05) writes=HUD_VISUAL_ONLY+PRIVATE_COUNTER",
                lnk01::kVersion, POST05_SOURCE_COMMIT, svc_host->version, lnk01::policy::kMaxPoints, lnk01::policy::kWeights[0].block, lnk01::policy::kWeights[1].block,
                lnk01::policy::kWeights[2].block, lnk01::policy::kParryCostPoints, lnk01::policy::kRegenDelayTicks, lnk01::policy::kRegenPointsPerSecond,
                hg.active ? "ON" : "OFF_HOOK_FAILED", lnk01::policy::kCenterX, lnk01::policy::kCenterY, lnk01::policy::kWidthOfSafe,
                lg.ui.enabled ? "HIDDEN_CANDIDATE" : "VISIBLE_HOOKS_FAILED", lg.sparkle_hook ? "ON" : "OFF", result_text(hq), result_text(hp), result_text(pres),
                static_cast<unsigned>(pflags));
  lnk_write(lg.ui.enabled && lg.sparkle_hook ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, wide);
  char build_hex[48] = {};
  for (unsigned i = 0; svc_host->build_id && i < svc_host->build_id_len && i < 20; ++i)
    std::snprintf(build_hex + i * 2, sizeof(build_hex) - i * 2, "%02x", svc_host->build_id[i]);
  const lnk01::policy::Weight* W = lnk01::policy::kWeights;
  std::snprintf(wide, sizeof(wide),
                "LNK_S1_INIT version=%s source_commit=%s host=%s host_build_id=%s components=LNK02_DIRECT+LNK03_PARRY+LNK04_REGEN+LNK07_VARIANTS "
                "weights_block/direct(blue/red/black/black_fury/unknown)=%d/%d,%d/%d,%d/%d,%d/%d,%d/%d special(expansion_group_power_x2)=+%d/+%d parry_link_cost=%d "
                "regen_delay_ticks=%u regen_pts_per_s_by_life(>=75/>=50/>=25/<25)=%d/%d/%d/%d guard_mul=%d/%d direct_evidence=NATIVE_DAMAGE_SCOPE+FIRST_CYLINDER+NO_POLY_OR_ELECTRIC+TIMER_EDGE+LIFE_LOSS "
                "variant_id=RESOURCE+AT_POWER+GENERATION_HISTORY hooks_new=4 hooks_total=10 abi=STATIC_ASSERTS_MEASURED_IN_EXE link_writes=0 physical_break=NONE(LNK05)",
                lnk01::kVersion, POST05_SOURCE_COMMIT, svc_host->version, build_hex[0] ? build_hex : "EMPTY", W[0].block, W[0].direct, W[1].block, W[1].direct,
                W[2].block, W[2].direct, W[3].block, W[3].direct, W[4].block, W[4].direct, lnk01::policy::kGroupPowerAdjust.block,
                lnk01::policy::kGroupPowerAdjust.direct, lnk01::policy::kParryCostPoints, lnk01::policy::kRegenDelayTicks, lnk01::policy::kBandPointsPerSecond[0],
                lnk01::policy::kBandPointsPerSecond[1], lnk01::policy::kBandPointsPerSecond[2], lnk01::policy::kBandPointsPerSecond[3], lnk01::policy::kGuardMulNum,
                lnk01::policy::kGuardMulDen);
  lnk_write(LOG_LEVEL_INFO, wide);
  lnk_write(LOG_LEVEL_INFO,"LNK_S1_FIX01_INIT native_damage_capture=READ_ONLY continuation_guard=ON monitor=ACTUAL_ARM_PLUS_COMPLETED_CYCLE enemy_writer=SCOPED_AT_REGISTRATION_MASK engine_clears_contact=ON human=HUMAN_RUNTIME_PENDING");
  std::snprintf(wide, sizeof(wide),
                "LNK_S2_INIT version=%s source_commit=%s components=LNK05_NATIVE_GUARD_BREAK+LNK06_NATIVE_RECOVERY state=%s trigger=daAlink_c::procGuardBreakInit "
                "(native export, called from the existing checkDamageAction POST inside Link's execute) anim=ANM_GUARD_BREAK proc=PROC_GUARD_BREAK(0x%x) "
                "cancel_frame=%.0f end_frame=%.0f block_window_ticks=%llu direct_window_ticks=%llu origins=GUARD_SLIP+FREE_GROUND_PROCS mode_block_mask=0x%x "
                "parry_breaks=0 regen_during_break=HELD reset=AFTER_OBSERVED_NATIVE_EXIT_ONLY native_counter_field_0x2fd5=UNTOUCHED new_hook_targets=0",
                lnk05::kVersion, POST05_SOURCE_COMMIT, lb.enabled ? "ON" : "OFF_HOOK_FAILED", static_cast<unsigned>(daAlink_c::PROC_GUARD_BREAK),
                static_cast<double>(lnk05::policy::kCancelFrame), static_cast<double>(lnk05::policy::kEndFrame),
                static_cast<unsigned long long>(lnk05::policy::kBlockTriggerTicks), static_cast<unsigned long long>(lnk05::policy::kDirectTriggerTicks),
                static_cast<unsigned>(lnk05::policy::kModeBlockedMask));
  lnk_write(lb.enabled ? LOG_LEVEL_INFO : LOG_LEVEL_WARN, wide);
  return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
  // Sin escrituras en el juego. Epoch de contexto, barrido por presupuesto y resumen periodico, fuera del camino caliente del hook.
  emit_disabled_once();
  if (g.eng.obs.disabled) return MOD_OK;
  try {
    do_update();
    if (g.eng.obs.summary_due()) emit_summary("PERIODIC");
  } catch (...) {
    g.eng.obs.disable("EXCEPTION_IN_UPDATE");
  }
  return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
  // POST07: the packet's vtable lives in this library: remove it from the game's 2D lists now (game thread, outside any painter run).
  hg.active = false;
  hud_drop_all();
  // SESSION_SHUTDOWN: no se consulta ni se desreferencia ninguna instancia.
  g.pe.on_shutdown();  // cierra carreras de recuperacion abiertas; solo aritmetica sobre el sidecar
  drain_posture();
  emit_summary("SHUTDOWN");
  emit(LogCat::Other, LOG_LEVEL_INFO, "POST03_SHUTDOWN hook_removal=HOST_DETACH instances_dereferenced=0");
  g.eng.obs.disable("SHUTDOWN");  // el host retira los hooks al descargar el mod (hook_remove_mod)
  return MOD_OK;
}

}  // extern "C"
