// LIGHT FADES TWICE 0.13.8 DKR01 integration laboratory: the PRODUCTION adapter (src/dkr_adapter.inc, included verbatim) and core, registered through
// its own dkr_install(), run against a transcription of the native paths it touches (TwilitRealm/dusklight@40457c6 src/d/actor/d_a_b_tn.cpp: execute
// timers, action() order, damage_check's ACT_ENDING return, setDamage's second-phase accumulator, executeZakoEnding modes 0..3, calcSwordMoveB,
// executeChaseL mode 0 / initChaseL, _delete; src/d/actor/d_a_alink_cut.inc: checkDownAttackState, checkDoCutAction, procCutDownInit, procCutDownLandInit,
// procCutDownLand -> procBackJumpInit) and the real clip bytes (res/darknut/resurrection_A_agil_natural_FIX02.bck, through the same structural check).
// Declared limits: clip lengths of the native BCKs are a MODEL (BCK_TN2B_DIE = 79 frames, the end Fable measured, "DIE(78.999)"); Link is scripted
// (his procedure lengths are a model); the native loader is a stand-in that records its calls and the in-place swap. A laboratory, not a game run.
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "../src/dkr_core.hpp"
static unsigned long long g_checks = 0;
#define CHECK(c)                                                  \
  do {                                                            \
    ++g_checks;                                                   \
    if (!(c)) {                                                   \
      std::printf("DKR_WORLD_LAB_FAIL line=%d %s\n", __LINE__, #c); \
      std::fflush(stdout);                                        \
      std::exit(1);                                               \
    }                                                             \
  } while (0)
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;
using u32 = std::uint32_t;
using f32 = float;
using BOOL = int;
inline constexpr unsigned kLogLineMax = 512;
// ---------------------------------------------------------------- SDK surface the adapter touches
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
enum ModResult { MOD_OK = 0, MOD_ERROR = 1 };
struct ModContext {};
using PreFn = HookAction (*)(ModContext*, void*, void*, void*);
using PostFn = void (*)(ModContext*, void*, void*, void*);
struct DkrEnding { static inline PreFn pre = nullptr; static inline PostFn post = nullptr; static inline bool fail = false; };
struct DkrDelete { static inline PreFn pre = nullptr; static inline PostFn post = nullptr; static inline bool fail = false; };
namespace mods {
template <class T> T arg(void* a, int n) { return *static_cast<T*>(static_cast<void**>(a)[n]); }
namespace hook {
template <class H> ModResult add_pre(PreFn f) { if (H::fail) return MOD_ERROR; H::pre = f; return MOD_OK; }
template <class H> ModResult add_post(PostFn f) { if (H::fail) return MOD_ERROR; H::post = f; return MOD_OK; }
template <class H> ModResult uninstall() { H::pre = nullptr, H::post = nullptr; return MOD_OK; }
}  // namespace hook
}  // namespace mods
struct ResourceBuffer { std::uint32_t struct_size; void* data; std::size_t size; };
#define RESOURCE_BUFFER_INIT {sizeof(ResourceBuffer), nullptr, 0u}
// ---------------------------------------------------------------- math / J3D
struct cXyz {
  f32 x = 0, y = 0, z = 0;
  cXyz() = default;
  cXyz(f32 a, f32 b, f32 c) : x(a), y(b), z(c) {}
  f32 abs() const { return std::sqrt(x * x + y * y + z * z); }
};
using MtxP = f32 (*)[4];
struct J3DAnmTransform {
  virtual ~J3DAnmTransform() {}
  virtual int getKind() const { return 8; }
  u8 mAttribute = 0, field_0x5 = 0;
  s16 mFrameMax = 0;
  f32 mFrame = 0;
  f32* mScaleData = nullptr;
  s16* mRotData = nullptr;
  f32* mTransData = nullptr;
  s16 field_0x18 = 0, field_0x1a = 0;
  u16 field_0x1c = 0, field_0x1e = 0;
  s16 getFrameMax() const { return mFrameMax; }
  u8 getAttribute() const { return mAttribute; }
};
struct J3DAnmTransformKey : J3DAnmTransform {
  int mDecShift = 0;
  void* mAnmTable = nullptr;
};
struct mDoExt_transAnmBas : J3DAnmTransformKey {
  explicit mDoExt_transAnmBas(s16 frames, void* bas) { mFrameMax = frames, mBas = bas, field_0x1e = 29; }
  void* mBas;
};
static int g_set_resource = 0, g_double_swap = 0;
static const void* g_swapped_buffer = nullptr;
struct J3DAnmLoaderDataBase {
  // Stand-in of the native loader for one ANK1 block: header fields into the object, then the key pools are byte-swapped IN PLACE (recorded).
  static void setResource(J3DAnmTransform* a, const void* data) {
    ++g_set_resource;
    const unsigned char* d = static_cast<const unsigned char*>(data);
    if (dkr::be32(d) != 0x4A334431u || dkr::be32(d + 4) != 0x62636B31u) return;
    const unsigned char* b = d + 0x20;
    a->field_0x1e = dkr::be16(b + 0x0C), a->mFrameMax = static_cast<s16>(dkr::be16(b + 0x0A)), a->mAttribute = b[8], a->mFrame = 0;
    a->mScaleData = reinterpret_cast<f32*>(const_cast<unsigned char*>(b + dkr::be32(b + 0x18)));
    static_cast<J3DAnmTransformKey*>(a)->mDecShift = b[9];
    if (g_swapped_buffer == data) ++g_double_swap;
    g_swapped_buffer = data;
    const_cast<unsigned char*>(d)[0x40] ^= 0xFF;  // the in-place swap makes the bytes differ from the shipped file
  }
};
struct J3DModel {
  f32 mtx[29][3][4] = {};
  MtxP getAnmMtx(int i) { return mtx[i]; }
};
// J3DFrameCtrl, once mode: frame += rate; at the end the frame clamps and the stop bit is set (checkState(1)).
struct mDoExt_McaMorfSO {
  J3DModel model;
  J3DAnmTransform* mpAnm = nullptr;
  f32 frame = 0, prev = -1, end = 0, rate = 1;
  int attr = 0;
  bool stop = false;
  unsigned set_count = 0;
  J3DModel* getModel() { return &model; }
  J3DAnmTransform* getAnm() { return mpAnm; }
  void setAnm(J3DAnmTransform* a, int at, f32, f32 r, f32 start, f32 e) {
    mpAnm = a, attr = at, rate = r, frame = start, prev = start - 1, end = e < 0 ? (a ? a->getFrameMax() : 0) : e, stop = false, ++set_count;
  }
  BOOL isStop() { return stop || rate == 0.0f; }
  f32 getEndFrame() { return end; }
  void setFrame(f32 f) { frame = f; }
  bool checkFrame(f32 f) { return prev < f && f <= frame; }
  void play() {
    prev = frame;
    if (stop) return;
    frame += rate;
    if (attr == 2) {
      if (frame >= end) frame -= end;
    } else if (frame >= end) {
      frame = end, stop = true;
    }
  }
};
struct dCcD_Sph {
  bool at = false;
  void OffAtSetBit() { at = false; }
};
// ---------------------------------------------------------------- actors
enum { fpcNm_B_TN_e = 0x111, fpcNm_ALINK_e = 0x2 };
struct actor_attention_types { u32 flags = 0; };
inline constexpr u32 fopAc_AttnFlag_BATTLE_e = 0x4;
struct fopAc_ac_c {
  virtual ~fopAc_ac_c() {}
  u32 id = 0, param = 0;
  s16 name = 0;
  cXyz pos_current;
  struct { cXyz pos; } current;
  f32 speedF = 0;
  s16 health = 0;
  actor_attention_types attention_info;
};
struct fopEn_enemy_c : fopAc_ac_c {
  u16 mFlags = 0;
  cXyz mDownPos;
  void onDownFlg() { mFlags |= 1; }
  void offDownFlg() { mFlags &= static_cast<u16>(~(1 | 2 | 4 | 0x10)); }
  BOOL checkDownFlg() { return mFlags & 1; }
  void onCutDownHitFlg() { mFlags |= 2; }
  BOOL checkCutDownHitFlg() const { return mFlags & 2; }
  void offHeadLockFlg() { mFlags &= static_cast<u16>(~0x80); }
  void setDownPos(const cXyz* p) { mDownPos = *p; }
  cXyz& getDownPos() { return mDownPos; }
};
inline u32 fopAcM_GetID(const void* a) { return static_cast<const fopAc_ac_c*>(a)->id; }
inline u32 fopAcM_GetParam(const void* a) { return static_cast<const fopAc_ac_c*>(a)->param; }
inline s16 fopAcM_GetName(const void* a) { return static_cast<const fopAc_ac_c*>(a)->name; }
static std::vector<fopAc_ac_c*> g_world;
int fopAcM_SearchByID(u32 id, fopAc_ac_c** out) {
  for (auto* p : g_world)
    if (p->id == id) return *out = p, 1;
  return *out = nullptr, 0;
}
struct daPy_actorKeep_c {
  fopAc_ac_c* mActor = nullptr;
  fopAc_ac_c* getActor() const { return mActor; }
};
struct daAlink_c : fopAc_ac_c {
  enum { PROC_WAIT = 0x1, PROC_BACK_JUMP = 0x12, PROC_BACK_JUMP_LAND = 0x13, PROC_CUT_DOWN = 0x2b, PROC_CUT_DOWN_LAND = 0x2c, PROC_CUT_JUMP = 0x2e };
  int mProcID = PROC_WAIT;
  daPy_actorKeep_c field_0x280c;
};
// Native SE / effect records of the transcribed Darknut
struct Rec {
  int se_foot = 0, se_hiza = 0, se_fall = 0, voice_dead2 = 0, disappear_body = 0, disappear_sword = 0, deleted = 0, switch_on = 0, sword_at_off = 0, sword_land = 0;
};
enum { ACT_DAMAGEL = 0xD, ACT_ENDING = 0xE, ACT_CHASEL = 0x9, ACT_YOROKE = 0xF };
static int g_bas_token = 0;  // native B_tn clips may carry animation sound data (mBas): the clone must not
static mDoExt_transAnmBas g_die(79, &g_bas_token), g_wait_b1(60, &g_bas_token), g_wait_b2(60, &g_bas_token), g_walk(40, &g_bas_token), g_damage_l(40, &g_bas_token), g_atk(50, &g_bas_token);
struct daB_TN_c : fopEn_enemy_c {
  int mActionMode1 = ACT_CHASEL, mActionMode2 = 0;
  int field_0x6fc = 0, field_0x700 = 360;
  f32 mMaterialColor = 0;
  u16 mStates[16] = {};
  int mTimer1 = 0;
  bool mUpdateNeckAngle = true, field_0xa91 = true, mCutFlag = false;
  u8 mSwitchNo = 0xFF, mType = 1, mType2 = 0;
  bool mStayNo2Flag = false, mDisappear = false, mInactive = false;
  bool sword_at = false, deleted = false;
  int sword_y = 100, sword_vy = 0;  // calcSwordMoveB: the dropped sword falling to the ground
  mDoExt_McaMorfSO* mpModelMorf2 = nullptr;
  mDoExt_McaMorfSO morf;
  dCcD_Sph mSphC;
  Rec rec;
  std::vector<f32> color_trace;
  void setSwordAtBit(int on) { sword_at = on != 0, rec.sword_at_off += on ? 0 : 1; }
  void setBck(J3DAnmTransform* a, int attr) { mpModelMorf2->setAnm(a, attr, 3.0f, 1.0f, 0.0f, -1.0f); }
  // d_a_b_tn.cpp L4359-4431, transcribed
  void executeZakoEnding_native() {
    switch (mActionMode2) {
      case 0:
        attention_info.flags = 0;
        setBck(&g_die, 0);
        ++rec.voice_dead2;
        mActionMode2 = 1;
        speedF = 0.0f;
        mUpdateNeckAngle = false;
        [[fallthrough]];
      case 1:
        if (mpModelMorf2->checkFrame(3.0f)) mStates[15] = 2, ++rec.se_foot;
        if (mpModelMorf2->checkFrame(33.0f)) ++rec.se_hiza;
        if (mpModelMorf2->checkFrame(68.0f)) ++rec.se_fall;
        mMaterialColor = mMaterialColor - 0.3f < -50.0f ? -50.0f : mMaterialColor - 0.3f;  // cLib_chaseF(&mMaterialColor, -50, 0.3)
        if (mpModelMorf2->isStop()) mActionMode2 = 2, mTimer1 = 10;
        break;
      case 2:
        mMaterialColor = mMaterialColor - 0.3f < -50.0f ? -50.0f : mMaterialColor - 0.3f;
        if (mTimer1 == 0) {
          ++rec.disappear_body;
          if (mStayNo2Flag) {
            mTimer1 = 60, mInactive = true, mActionMode2 = 3;
          } else {
            if (mSwitchNo != 0xFF) ++rec.switch_on;
            deleted = true;  // fopAcM_delete
          }
        }
        break;
      case 3:
        if (mTimer1 == 0) deleted = true;
        break;
    }
  }
  void executeZakoEnding();  // through the hook dispatcher
  // d_a_b_tn.cpp L687-736: the dropped sword (mType 1: disappears where it lands)
  void calcSwordMoveB() {
    switch (mStates[15]) {
      case 2: sword_y = 100, sword_vy = 0, mStates[15] = 3; [[fallthrough]];
      case 3:
      case 4:
        sword_vy -= 5, sword_y += sword_vy;
        if (sword_y <= 0) {
          ++rec.sword_land;
          if (mStates[15] == 3) sword_vy = 10, sword_y = 1, mStates[15] = 4;
          else ++rec.disappear_sword, mStates[15] = 7;
        }
        break;
      default: break;
    }
  }
  // initChaseL / executeChaseL mode 0 (L3110, L3382-3402): circle (WAIT_B_1/2) or walk, sword AT off, field_0xa91 true
  void executeChaseL() {
    if (mActionMode2 == 0 || mActionMode2 == 1) {
      mpModelMorf2->setAnm(mActionMode2 == 0 ? &g_wait_b1 : &g_wait_b2, 2, 10.0f, 1.0f, 0.0f, -1.0f);
      mActionMode2 = 4, speedF = 4.8f;
      setSwordAtBit(0);
      field_0xa91 = true;
    }
  }
  // the second-phase branch of setDamage (L1195-1222), sword hit of power `atp`
  int pending_atp = 0;
  void damage_check() {
    const int atp = pending_atp;
    pending_atp = 0;                         // a Tg hit lives one collision pass
    if (mActionMode1 == ACT_ENDING) return;  // L1267: no damage of any kind while ending
    if (!atp) return;
    health = 100;
    health = static_cast<s16>(health - atp);
    if (field_0x6fc > field_0x700 - 50.0f && mCutFlag) field_0x6fc += 50;
    else field_0x6fc += 100 - health;
    if (field_0x6fc >= field_0x700) mActionMode1 = ACT_ENDING, mActionMode2 = 0, offHeadLockFlg();
    else mActionMode1 = ACT_DAMAGEL, mActionMode2 = 0, offHeadLockFlg();
  }
  void action() {
    damage_check();
    field_0x700 = 360;
    switch (mActionMode1) {
      case ACT_CHASEL: executeChaseL(); break;
      case ACT_DAMAGEL:
        if (mActionMode2 == 0) mpModelMorf2->setAnm(&g_damage_l, 0, 3.0f, 1.0f, 0.0f, -1.0f), mActionMode2 = 2, setSwordAtBit(0);
        else if (mpModelMorf2->isStop()) mActionMode1 = ACT_CHASEL, mActionMode2 = 0, field_0xa91 = true;
        break;
      case ACT_ENDING: executeZakoEnding(); break;
      default: break;
    }
    calcSwordMoveB();
    mpModelMorf2->play();
  }
  void execute() {
    if (deleted) return;
    if (mTimer1 != 0) --mTimer1;
    action();
    color_trace.push_back(mMaterialColor);
  }
  int _delete();
};
// ---------------------------------------------------------------- bridge + adapter
static std::uint64_t g_tick = 0;
static daAlink_c g_link;
static std::vector<std::string> g_log;
static bool g_resource = true, g_resource_missing = false;
static std::vector<unsigned char> g_file;
static int g_loads = 0, g_frees = 0;
static void* g_live_buffer = nullptr;
std::uint64_t dkrb_tick() noexcept { return g_tick; }
daAlink_c* dkrb_link() noexcept { return &g_link; }
void dkrb_write(bool, const char* line) noexcept { g_log.emplace_back(line); }
bool dkrb_resource_ready() noexcept { return g_resource; }
bool dkrb_load(const char* path, ResourceBuffer& out) noexcept {
  if (!g_resource || g_resource_missing || std::strcmp(path, "darknut/resurrection_A_agil_natural_FIX02.bck") != 0) return false;
  ++g_loads;
  out.data = std::malloc(g_file.size()), out.size = g_file.size();
  std::memcpy(out.data, g_file.data(), g_file.size());
  g_live_buffer = out.data;
  return true;
}
void dkrb_free(ResourceBuffer& b) noexcept {
  if (!b.data) return;
  ++g_frees;
  if (b.data == g_live_buffer) g_live_buffer = nullptr;
  std::free(b.data);
  b.data = nullptr, b.size = 0;
}
namespace {
#include "../src/dkr_adapter.inc"
}
void daB_TN_c::executeZakoEnding() {
  daB_TN_c* self = this;
  void* args[] = {&self};
  bool skip = false;
  if (DkrEnding::pre) skip = DkrEnding::pre(nullptr, args, nullptr, nullptr) == HOOK_SKIP_ORIGINAL;
  if (!skip) executeZakoEnding_native();
  if (DkrEnding::post) DkrEnding::post(nullptr, args, nullptr, nullptr);
}
int daB_TN_c::_delete() {
  daB_TN_c* self = this;
  void* args[] = {&self};
  if (DkrDelete::pre) DkrDelete::pre(nullptr, args, nullptr, nullptr);
  return 1;
}
// ---------------------------------------------------------------- the world
static u32 g_next_id = 100;
struct Darknut {
  daB_TN_c a;
  explicit Darknut(u32 param = 0x000001FFu) {
    a.id = g_next_id++, a.param = param, a.name = fpcNm_B_TN_e;
    a.mType = static_cast<u8>(param >> 8 & 0xFF) == 1 ? 1 : 0, a.mType2 = static_cast<u8>(param >> 24) == 1 ? 1 : 0, a.mSwitchNo = static_cast<u8>(param & 0xFF);
    a.mpModelMorf2 = &a.morf;
    a.mStates[13] = 7, a.mStates[14] = 7, a.mStates[15] = 1;  // second phase: mace thrown, scabbard gone, sword B in the hand
    a.attention_info.flags = fopAc_AttnFlag_BATTLE_e, a.health = 100;
    a.mpModelMorf2->setAnm(&g_wait_b1, 2, 0, 1, 0, -1);
    a.mActionMode1 = ACT_CHASEL, a.mActionMode2 = 4;
    for (int j = 0; j < 29; ++j) a.morf.model.mtx[j][0][3] = 0, a.morf.model.mtx[j][1][3] = 30, a.morf.model.mtx[j][2][3] = 0;
    g_world.push_back(&a);
  }
  ~Darknut() { remove(); }
  void remove() {
    for (size_t i = 0; i < g_world.size(); ++i)
      if (g_world[i] == &a) {
        a._delete();
        g_world.erase(g_world.begin() + long(i));
        return;
      }
  }
  void kill_blow() { a.pending_atp = 100; }   // one hit that reaches field_0x700 from 0 (100 - (100-100)... 4 hits of 100 in the native: modelled as 4 below)
  void lethal() {                               // hits until the native accumulator crosses 360 (each hit of atp 100 adds 100)
    while (a.mActionMode1 != ACT_ENDING) {
      a.pending_atp = 100;
      a.execute();
      for (int k = 0; k < 45 && a.mActionMode1 == ACT_DAMAGEL; ++k) a.execute();
    }
  }
};
// Link's Ending Blow (d_a_alink_cut.inc L799-822, L766-778, L2058-2245), scripted lengths
struct LinkScript {
  fopEn_enemy_c* target = nullptr;  // mTargetedActor (lock-on)
  bool learned = true;               // F_0339 (the Ending Blow hidden skill)
  int t = 0, jump = 18, stab = 32, stop = 20, back = 14, back_land = 8, miss_anim = 60;
  bool miss = false, interrupt = false, success = false;
  int finishes = 0, jump_attacks = 0;
  bool down_attack_state() {
    if (!target || !target->checkDownFlg() || !learned) return false;
    const cXyz& d = target->getDownPos();
    const f32 dx = d.x - g_link.current.pos.x, dz = d.z - g_link.current.pos.z;
    return dx * dx + dz * dz < 800.0f * 800.0f;
  }
  // A press of A: checkDoCutAction re-checks the state at the press
  void press() {
    if (g_link.mProcID != daAlink_c::PROC_WAIT) return;
    if (down_attack_state()) {
      g_link.mProcID = daAlink_c::PROC_CUT_DOWN, g_link.field_0x280c.mActor = target, t = 0, success = false, ++finishes;
    } else {
      g_link.mProcID = daAlink_c::PROC_CUT_JUMP, t = 0, ++jump_attacks;
    }
  }
  void step() {
    ++t;
    switch (g_link.mProcID) {
      case daAlink_c::PROC_CUT_JUMP:
        if (t >= 30) g_link.mProcID = daAlink_c::PROC_WAIT;
        break;
      case daAlink_c::PROC_CUT_DOWN:
        if (interrupt && t == 6) {
          g_link.mProcID = daAlink_c::PROC_WAIT, g_link.field_0x280c.mActor = nullptr;  // e.g. a damage reaction cancels the jump
          break;
        }
        if (t >= jump) {
          auto* e = static_cast<fopEn_enemy_c*>(g_link.field_0x280c.getActor());
          // procCutDownLandInit: the receipt needs the down flag, the tip within 100 (XZ) of the down position and |dy| < 100
          const cXyz tip = miss ? cXyz(e->getDownPos().x + 160.0f, e->getDownPos().y, e->getDownPos().z) : e->getDownPos();
          const f32 dx = tip.x - e->getDownPos().x, dz = tip.z - e->getDownPos().z;
          success = e && e->checkDownFlg() && dx * dx + dz * dz < 100.0f * 100.0f && std::fabs(e->current.pos.y - g_link.current.pos.y) < 100.0f;
          if (success) e->onCutDownHitFlg();
          g_link.mProcID = daAlink_c::PROC_CUT_DOWN_LAND, t = 0;
        }
        break;
      case daAlink_c::PROC_CUT_DOWN_LAND:
        if (success ? t >= stab + stop : t >= stab + miss_anim) {
          if (success) g_link.mProcID = daAlink_c::PROC_BACK_JUMP, t = 0;  // procBackJumpInit(1)
          else g_link.mProcID = daAlink_c::PROC_WAIT;
        }
        break;
      case daAlink_c::PROC_BACK_JUMP:
        if (t >= back) g_link.mProcID = daAlink_c::PROC_BACK_JUMP_LAND, t = 0;
        break;
      case daAlink_c::PROC_BACK_JUMP_LAND:
        if (t >= back_land) g_link.mProcID = daAlink_c::PROC_WAIT, g_link.field_0x280c.mActor = nullptr;
        break;
      default: break;
    }
  }
};
static LinkScript L;
static bool g_link_first = true;
struct Frame {
  std::vector<Darknut*> ds;
  bool rising_during_finish = false, press_pending = false;
  void link_exec() {
    if (press_pending) L.press(), press_pending = false;
    L.step();
  }
  void tick() {
    ++g_tick;
    if (g_link_first) link_exec();
    for (auto* d : ds)
      if (!d->a.deleted) d->a.execute();
    if (!g_link_first) link_exec();
    for (auto* d : ds) {
      const bool ours = d->a.morf.getAnm() == dkr_clip();
      const int p = g_link.mProcID;
      if (ours && (p == daAlink_c::PROC_CUT_DOWN || p == daAlink_c::PROC_CUT_DOWN_LAND) && d->a.morf.frame <= 1.0f && d->a.morf.set_count && d->a.mActionMode1 == ACT_ENDING) rising_during_finish = true;
      if (d->a.deleted && !d->a.rec.deleted) d->a.rec.deleted = 1, d->remove();
    }
  }
};
static void reset_world() {
  CHECK(g_live_buffer == nullptr);  // every scenario released the clip buffer (dkr_shutdown) before the next one
  dz = DkrState{};
  std::memset(dkr_clip_store, 0, sizeof(dkr_clip_store));
  DkrEnding::pre = nullptr, DkrEnding::post = nullptr, DkrDelete::pre = nullptr, DkrDelete::post = nullptr, DkrEnding::fail = DkrDelete::fail = false;
  g_set_resource = 0, g_double_swap = 0, g_swapped_buffer = nullptr, g_loads = 0, g_frees = 0, g_resource = true, g_resource_missing = false;
  g_link = daAlink_c{}, g_link.name = fpcNm_ALINK_e, g_link.current.pos = cXyz(0, 0, 400);
  L = LinkScript{}, g_link_first = true, g_log.clear();
}
static bool logged(const char* ev) {
  for (auto& s : g_log)
    if (s.find(ev) != std::string::npos) return true;
  return false;
}
static int count_log(const char* ev) {
  int n = 0;
  for (auto& s : g_log) n += s.find(ev) != std::string::npos;
  return n;
}
static std::vector<unsigned char> read_file(const char* p) {
  std::vector<unsigned char> v;
  if (FILE* f = std::fopen(p, "rb")) {
    unsigned char b[4096];
    size_t n;
    while ((n = std::fread(b, 1, sizeof(b), f)) > 0) v.insert(v.end(), b, b + n);
    std::fclose(f);
  }
  return v;
}
// Run one Darknut to the end of its first fall; returns the ticks the fall took.
static int fall(Frame& w, Darknut& d) {
  d.lethal();
  CHECK(d.a.mActionMode1 == ACT_ENDING);
  int n = 0;
  while (d.a.mActionMode2 != dkr::kHeldMode && n < 400) w.tick(), ++n;
  return n;
}
// ---------------------------------------------------------------- tests
static void t18_clip_and_install() {
  reset_world();
  CHECK(g_file.size() == dkr::kBckBytes && dkr::check_bck(g_file.data(), g_file.size()) == dkr::Bck::Ok);
  dkr_install();
  CHECK(dz.enabled && dz.clip_ok && !dz.clip_bound && g_loads == 1 && DkrEnding::pre && DkrEnding::post && DkrDelete::pre && logged("event=INSTALL") && logged("enabled=1"));
  CHECK(g_set_resource == 0);  // bound lazily, from a native transAnmBas, at the first lethal transition
  dkr_shutdown();
  CHECK(g_frees == 1 && g_live_buffer == nullptr && logged("DKR01_SUMMARY why=SHUTDOWN"));
  // a corrupted / missing / unserved clip: nothing installed, every Darknut native
  for (int k = 0; k < 4; ++k) {
    reset_world();
    std::vector<unsigned char> keep = g_file;
    if (k == 0) g_file[0x5000] ^= 1;
    if (k == 1) g_file.pop_back();
    if (k == 2) g_resource_missing = true;
    if (k == 3) g_resource = false;
    dkr_install();
    CHECK(!dz.enabled && !DkrEnding::pre && !DkrDelete::pre && g_loads == g_frees);
    g_file = keep;
  }
  // one hook refused: all or nothing, buffer released
  reset_world();
  DkrDelete::fail = true;
  dkr_install();
  CHECK(!dz.enabled && !DkrEnding::pre && !DkrEnding::post && g_frees == 1);
  // the clip binds exactly once for any number of Darknuts (the native loader swaps the pools in place: a second bind would corrupt them)
  reset_world();
  dkr_install();
  Frame w;
  Darknut d1, d2, d3;
  w.ds = {&d1, &d2, &d3};
  fall(w, d1), fall(w, d2), fall(w, d3);
  CHECK(g_set_resource == 1 && g_double_swap == 0 && dz.clip_bound && dkr_clip()->getFrameMax() == 88 && dkr_clip()->field_0x1e == 29 && dkr_clip()->getAttribute() == 0);
  CHECK(static_cast<mDoExt_transAnmBas*>(dkr_clip())->mBas == nullptr && dkr_clip()->getKind() == 8);
  d1.remove(), d2.remove(), d3.remove();
  dkr_shutdown();
  CHECK(g_frees == 1 && g_live_buffer == nullptr);
}
// T01-T04, T06, T10: the timer path, then the native final death
static void t06_timer_path_then_native_death(bool link_first) {
  reset_world();
  g_link_first = link_first;
  dkr_install();
  Frame w;
  Darknut d;
  w.ds = {&d};
  const int ft = fall(w, d);
  CHECK(ft >= 79 && ft <= 82);  // the whole native DIE clip played
  // the native fall ran (voice, SEs, shakes) without darkening, without the sword drop, without vanishing
  CHECK(d.a.rec.voice_dead2 == 1 && d.a.rec.se_foot == 1 && d.a.rec.se_hiza == 1 && d.a.rec.se_fall == 1);
  for (f32 c : d.a.color_trace) CHECK(c == 0.0f);
  CHECK(d.a.mStates[15] == 1 && d.a.rec.sword_land == 0 && d.a.rec.disappear_sword == 0 && d.a.rec.disappear_body == 0 && !d.a.deleted);
  CHECK(!d.a.sword_at && d.a.rec.sword_at_off >= 1 && !d.a.mSphC.at);           // the lying body hurts nobody
  CHECK(d.a.attention_info.flags == fopAc_AttnFlag_BATTLE_e);                   // still lockable: the Ending Blow needs the lock
  CHECK(d.a.checkDownFlg() && d.a.mActionMode1 == ACT_ENDING && d.a.mActionMode2 == dkr::kHeldMode);
  CHECK(std::fabs(d.a.getDownPos().y - 75.0f) < 0.01f && std::fabs(d.a.getDownPos().z - 30.0f) < 0.01f);  // joint 2 + 45 up, 30 toward Link
  L.target = &d.a;
  CHECK(L.down_attack_state());                                                 // the Finish prompt (checkDownAttackState) is offered
  // no Finish: the body waits exactly kReviveDelayTicks after the fall, hits are ignored by the native damage_check
  for (unsigned k = 1; k < dkr::kReviveDelayTicks; ++k) {
    if (k % 25 == 0) d.a.pending_atp = 100;
    w.tick();
    CHECK(d.a.morf.getAnm() == &g_die && d.a.mActionMode2 == dkr::kHeldMode && d.a.field_0x6fc >= 360 && d.a.checkDownFlg());
  }
  w.tick();
  CHECK(d.a.morf.getAnm() == dkr_clip() && !d.a.checkDownFlg() && d.a.mActionMode1 == ACT_ENDING);  // 300 ticks = 10 s: the resurrection starts
  CHECK(!L.down_attack_state());
  CHECK(logged("path=TIMER"));
  int rise = 0;
  while (d.a.mActionMode1 == ACT_ENDING && rise < 200) {
    d.a.pending_atp = 100;  // hits during the rise do nothing either
    w.tick(), ++rise;
    if (d.a.mActionMode1 == ACT_ENDING) CHECK(d.a.morf.getAnm() == dkr_clip() && !d.a.sword_at);
  }
  CHECK(rise >= 88 && rise <= 90);  // the whole 88-frame clip, once
  CHECK(d.a.mActionMode1 == ACT_CHASEL && d.a.mActionMode2 == 0 && d.a.field_0x6fc == 0 && d.a.health == 100 && d.a.field_0xa91 && d.a.mUpdateNeckAngle);
  CHECK(d.a.mStates[15] == 1 && d.a.attention_info.flags == fopAc_AttnFlag_BATTLE_e && d.a.mMaterialColor == 0.0f);
  d.a.pending_atp = 0;
  w.tick();  // the native chase picks its own clip: our clip is no longer referenced by the model
  CHECK(d.a.morf.getAnm() == &g_wait_b1 && d.a.mActionMode1 == ACT_CHASEL);
  CHECK(logged("RESURRECTED_SECOND_PHASE_100PCT"));
  // the accumulator starts from zero: three hits of 100 do not kill, the fourth does (native 360)
  for (int h = 0; h < 3; ++h) {
    d.a.pending_atp = 100, w.tick();
    for (int k = 0; k < 45 && d.a.mActionMode1 == ACT_DAMAGEL; ++k) w.tick();
    CHECK(d.a.mActionMode1 != ACT_ENDING);
  }
  // SECOND defeat: the whole native death, no second chance
  d.a.pending_atp = 100, w.tick();
  CHECK(d.a.mActionMode1 == ACT_ENDING);
  CHECK(logged("SECOND_DEFEAT_NATIVE_DEATH"));
  const size_t c0 = d.a.color_trace.size();
  int n = 0;
  while (!d.a.rec.deleted && n < 300) w.tick(), ++n;
  CHECK(d.a.rec.deleted && d.a.rec.disappear_body == 1 && d.a.rec.voice_dead2 == 2);  // disappear effect + fopAcM_delete
  CHECK(d.a.rec.disappear_sword == 1);                                                 // the sword fell and vanished natively
  CHECK(d.a.color_trace.back() < -20.0f && d.a.color_trace[c0] < 0.0f);                // the native darkening ran
  CHECK(!d.a.checkDownFlg() && count_log("RESURRECTION_START") == 1 && count_log("DOWN_FINISH_AVAILABLE") == 1);
  CHECK(dz.t.live() == 0 && logged("ACTOR_DELETED_SLOT_RETIRED"));
  dkr_shutdown();
}
// T05, T09: the Ending Blow path, then the native final death
static void t05_finish_path_then_native_death(bool link_first) {
  reset_world();
  g_link_first = link_first;
  dkr_install();
  Frame w;
  Darknut d;
  w.ds = {&d};
  fall(w, d);
  L.target = &d.a;
  for (int k = 0; k < 40; ++k) w.tick();
  const unsigned timer_before = dz.t.find(d.a.id)->timer;
  L.press();
  CHECK(g_link.mProcID == daAlink_c::PROC_CUT_DOWN && L.finishes == 1);
  bool rose_mid = false;
  int n = 0;
  while (d.a.mActionMode1 == ACT_ENDING && d.a.morf.getAnm() != dkr_clip() && n < 400) {
    w.tick(), ++n;
    const int p = g_link.mProcID;
    if (d.a.morf.getAnm() == dkr_clip() && p != daAlink_c::PROC_WAIT) rose_mid = true;
    if (dz.t.find(d.a.id)->phase == dkr::Phase::FinishHit) CHECK(!d.a.checkDownFlg() && !d.a.checkCutDownHitFlg() && !L.down_attack_state());  // no second Finish
  }
  CHECK(L.success && !rose_mid && g_link.mProcID == daAlink_c::PROC_WAIT);  // the rise starts only after the stab, the hold, the back flip and its landing
  CHECK(d.a.morf.getAnm() == dkr_clip() && logged("FINISH_HIT_NATIVE_RECEIPT") && logged("path=FINISH") && !d.a.checkDownFlg() && !d.a.checkCutDownHitFlg());
  CHECK(dz.t.find(d.a.id)->timer == timer_before);  // frozen during the whole Ending Blow
  CHECK(n < int(dkr::kReviveDelayTicks));  // well before the timer
  int rise = 0;
  while (d.a.mActionMode1 == ACT_ENDING && rise < 200) w.tick(), ++rise;
  CHECK(rise >= 88 && rise <= 90 && d.a.mActionMode1 == ACT_CHASEL && d.a.field_0x6fc == 0 && d.a.health == 100);
  // no second Ending Blow on the risen Darknut; a press is an ordinary jump attack
  L.press();
  CHECK(g_link.mProcID == daAlink_c::PROC_CUT_JUMP && L.finishes == 1);
  for (int k = 0; k < 40; ++k) w.tick();
  // second defeat: native
  d.lethal();
  int m = 0;
  while (!d.a.rec.deleted && m < 300) w.tick(), ++m;
  CHECK(d.a.rec.deleted && d.a.rec.disappear_body == 1 && d.a.rec.disappear_sword == 1 && count_log("RESURRECTION_START") == 1 && logged("SECOND_DEFEAT_NATIVE_DEATH"));
  dkr_shutdown();
}
// T07: the timer about to expire when the Ending Blow starts; both orders of Link vs the Darknut in the tick
static void t07_race(bool link_first, int press_at_left) {
  reset_world();
  g_link_first = link_first;
  dkr_install();
  Frame w;
  Darknut d;
  w.ds = {&d};
  fall(w, d);
  L.target = &d.a;
  dkr::Ctl* c = dz.t.find(d.a.id);
  while (c->timer > unsigned(press_at_left)) w.tick();
  w.press_pending = true;  // A is pressed in Link's execute of the next tick: before or after the Darknut's, checkDoCutAction re-checks the down flag then
  w.tick();
  const bool committed_jump = L.finishes == 1;
  CHECK(committed_jump || (L.jump_attacks == 1 && press_at_left == 1 && !link_first));  // only a press AFTER the expiring tick misses the Finish
  int n = 0;
  while (d.a.mActionMode1 == ACT_ENDING && n < 600) {
    w.tick(), ++n;
    if (d.a.morf.getAnm() == dkr_clip()) {
      const int p = g_link.mProcID;
      CHECK(p != daAlink_c::PROC_CUT_DOWN && p != daAlink_c::PROC_CUT_DOWN_LAND);  // never rising under the jump, the stab or the recovery
      if (committed_jump) CHECK(p == daAlink_c::PROC_WAIT);
    }
  }
  CHECK(d.a.mActionMode1 == ACT_CHASEL && count_log("RESURRECTION_START") == 1);
  CHECK(!w.rising_during_finish);
  if (committed_jump) CHECK(L.success && logged("path=FINISH"));
  else CHECK(logged("path=TIMER") && L.finishes == 0);
  dkr_shutdown();
}
// T08: a missed / interrupted Ending Blow: no receipt, the timer resumes, the body never stays down forever
static void t08_miss_and_interrupt(bool interrupt) {
  reset_world();
  dkr_install();
  Frame w;
  Darknut d;
  w.ds = {&d};
  fall(w, d);
  L.target = &d.a, L.miss = !interrupt, L.interrupt = interrupt;
  for (int k = 0; k < 100; ++k) w.tick();
  const unsigned before = dz.t.find(d.a.id)->timer;
  L.press();
  CHECK(g_link.mProcID == daAlink_c::PROC_CUT_DOWN);
  int n = 0;
  while (g_link.mProcID != daAlink_c::PROC_WAIT && n < 300) {
    w.tick(), ++n;
    CHECK(d.a.morf.getAnm() == &g_die && d.a.checkDownFlg() && !d.a.checkCutDownHitFlg());
  }
  CHECK(!L.success && dz.t.find(d.a.id)->timer + 1 >= before);  // frozen during the whole attempt (the tick Link is back resumes it)
  w.tick();
  CHECK(dz.t.find(d.a.id)->phase == dkr::Phase::Down && dz.t.find(d.a.id)->misses == 1 && logged("FINISH_MISSED_TIMER_RESUMES"));
  // a second, successful Ending Blow is still possible
  L.miss = false, L.interrupt = false;
  L.press();
  int m = 0;
  while (d.a.mActionMode1 == ACT_ENDING && m < 600) w.tick(), ++m;
  CHECK(L.success && logged("path=FINISH") && d.a.mActionMode1 == ACT_CHASEL);
  dkr_shutdown();
  // missed again and never retried: the timer still raises it
  reset_world();
  dkr_install();
  Frame w2;
  Darknut d2;
  w2.ds = {&d2};
  fall(w2, d2);
  L.target = &d2.a, L.miss = true;
  for (int k = 0; k < 280; ++k) w2.tick();
  L.press();
  int q = 0;
  while (d2.a.mActionMode1 == ACT_ENDING && q < 600) w2.tick(), ++q;
  CHECK(logged("path=TIMER") && d2.a.mActionMode1 == ACT_CHASEL && !w2.rising_during_finish);
  dkr_shutdown();
}
// T11: hits and repeated calls during the fall / down / rise change nothing
static void t11_no_double_activation() {
  reset_world();
  dkr_install();
  Frame w;
  Darknut d;
  w.ds = {&d};
  d.lethal();
  for (int k = 0; k < 400 && d.a.mActionMode1 == ACT_ENDING; ++k) {
    d.a.pending_atp = 100;
    const int acc = d.a.field_0x6fc;
    w.tick();
    if (d.a.mActionMode1 == ACT_ENDING) CHECK(d.a.field_0x6fc == acc);
  }
  CHECK(count_log("FIRST_DEFEAT_NATIVE_FALL") == 1 && count_log("DOWN_FINISH_AVAILABLE") == 1 && count_log("RESURRECTION_START") == 1);
  dkr_shutdown();
  // a duplicated mode-0 PRE (never native) is ignored while the body is held
  reset_world();
  dkr_install();
  Frame w2;
  Darknut e;
  w2.ds = {&e};
  fall(w2, e);
  dkr::Ctl* c = dz.t.find(e.a.id);
  CHECK(c->on_lethal(true) == dkr::Step::None && c->phase == dkr::Phase::Down);
  dkr_shutdown();
}
// T12, T17: several Darknuts, crossed timings, one rising while another dies natively; slots do not leak to a new actor
static void t12_many() {
  reset_world();
  dkr_install();
  Frame w;
  Darknut a, b;
  w.ds = {&a, &b};
  fall(w, a);  // A down
  for (int k = 0; k < 200; ++k) w.tick();
  // B already revived once (simulate its whole first cycle quickly), then its second defeat while A is about to rise
  fall(w, b);
  L.target = &b.a;
  L.press();
  int n = 0;
  while (b.a.mActionMode1 == ACT_ENDING && n < 600) w.tick(), ++n;
  CHECK(b.a.mActionMode1 == ACT_CHASEL);
  CHECK(a.a.mActionMode1 == ACT_CHASEL || a.a.morf.getAnm() == dkr_clip());  // A's timer ran out meanwhile (frozen only during B's Ending Blow)
  b.lethal();
  int m = 0;
  while ((!b.a.rec.deleted || a.a.mActionMode1 == ACT_ENDING) && m < 600) w.tick(), ++m;
  CHECK(b.a.rec.deleted && b.a.rec.disappear_body == 1 && a.a.mActionMode1 == ACT_CHASEL && a.a.field_0x6fc == 0);
  CHECK(dz.t.live() == 1 && dz.t.find(a.a.id) && dz.t.find(a.a.id)->consumed() && !dz.t.find(b.a.id));
  // A's second defeat: native
  a.lethal();
  while (!a.a.rec.deleted && m < 1200) w.tick(), ++m;
  CHECK(a.a.rec.deleted && dz.t.live() == 0 && count_log("SECOND_DEFEAT_NATIVE_DEATH") == 2 && count_log("RESURRECTION_START") == 2);
  // a new Darknut (new process id, even at a reused address) starts armed
  Darknut c;
  w.ds = {&c};
  fall(w, c);
  CHECK(dz.t.find(c.a.id)->revivals == 0 && dz.t.find(c.a.id)->phase == dkr::Phase::Down);
  // T17: five full sequences in a row, alternating paths
  for (int s = 0; s < 5; ++s) {
    Darknut x;
    w.ds = {&x};
    fall(w, x);
    if (s % 2 == 0) L.target = &x.a, L.miss = false, L.press();
    int q = 0;
    while (x.a.mActionMode1 == ACT_ENDING && q < 700) w.tick(), ++q;
    CHECK(x.a.mActionMode1 == ACT_CHASEL && dz.t.find(x.a.id)->revivals == 1);
    x.lethal();
    q = 0;
    while (!x.a.rec.deleted && q < 300) w.tick(), ++q;
    CHECK(x.a.rec.deleted && !dz.t.find(x.a.id));
  }
  CHECK(g_set_resource == 1 && g_double_swap == 0);
  c.remove();
  dkr_shutdown();
}
// T13: deletion (room change / scene reset) at every held phase; unload while held or rising: native vanish, never our clip after the buffer is gone
static void t13_lifetime() {
  for (int when = 0; when < 4; ++when) {
    reset_world();
    dkr_install();
    Frame w;
    auto* d = new Darknut();
    w.ds = {d};
    d->lethal();
    for (int k = 0; k < (when == 0 ? 20 : when == 1 ? 120 : when == 2 ? 300 + 81 : 300 + 81 + 40); ++k) w.tick();
    const u32 id = d->a.id;
    delete d;  // _delete: slot retired
    CHECK(!dz.t.find(id) && dz.t.live() == 0);
    w.ds.clear();
    dkr_shutdown();
    CHECK(g_live_buffer == nullptr);
  }
  for (int when = 0; when < 3; ++when) {
    reset_world();
    dkr_install();
    Frame w;
    Darknut d;
    w.ds = {&d};
    fall(w, d);
    if (when >= 1) for (int k = 0; k < (when == 1 ? 150 : 330); ++k) w.tick();
    const bool was_rising = d.a.morf.getAnm() == dkr_clip();
    CHECK(when != 2 || was_rising);
    dkr_shutdown();  // hooks gone, the clip memory about to go
    CHECK(d.a.morf.getAnm() != dkr_clip() && d.a.mActionMode2 == dkr::kModeVanish && !d.a.checkDownFlg() && g_live_buffer == nullptr);
    DkrEnding::pre = nullptr, DkrEnding::post = nullptr, DkrDelete::pre = nullptr;  // the host removed the hooks
    int n = 0;
    while (!d.a.rec.deleted && n < 100) w.tick(), ++n;
    CHECK(d.a.rec.deleted && d.a.rec.disappear_body == 1);  // a controlled native death, never an immortal body
  }
}
// T14: every other Darknut is untouched (the Sword Darknut 0x010001FF, a placed one with a switch, a Hyrule Castle stay-no-2)
static void t14_other_variants() {
  for (int v = 0; v < 3; ++v) {
    reset_world();
    dkr_install();
    Frame w;
    Darknut d(v == 0 ? 0x010001FFu : v == 1 ? 0x00000112u : 0x000001FFu);
    if (v == 2) d.a.mStayNo2Flag = true;
    w.ds = {&d};
    d.lethal();
    int n = 0;
    while (!d.a.rec.deleted && !d.a.mInactive && n < 400) w.tick(), ++n;
    CHECK((d.a.rec.deleted || d.a.mInactive) && d.a.rec.disappear_body == 1 && dz.t.live() == 0 && !logged("FIRST_DEFEAT") && dz.ineligible == 1);
    CHECK(v != 1 || d.a.rec.switch_on == 1);
    if (d.a.mInactive) {
      while (!d.a.rec.deleted && n < 600) w.tick(), ++n;
      CHECK(d.a.rec.deleted);
    }
    dkr_shutdown();
  }
}
// T03: killed in the middle of an attack (the sword AT is live at the lethal hit): the lying body never hurts Link
static void t03_killed_attacking() {
  reset_world();
  dkr_install();
  Frame w;
  Darknut d;
  w.ds = {&d};
  d.a.field_0x6fc = 300, d.a.sword_at = true, d.a.mSphC.at = true, d.a.mActionMode1 = 0xA, d.a.mActionMode2 = 3;  // ACT_ATTACKL, swinging
  d.a.pending_atp = 100;
  d.a.execute();
  CHECK(d.a.mActionMode1 == ACT_ENDING && !d.a.sword_at && !d.a.mSphC.at && d.a.rec.sword_at_off == 1);
  int n = 0;
  while (d.a.mActionMode2 != dkr::kHeldMode && n < 200) {
    w.tick(), ++n;
    CHECK(!d.a.sword_at);
  }
  dkr_shutdown();
}
// Without the Ending Blow skill (or as Wolf): no Finish prompt, the timer path still raises it once
static void t04_no_skill() {
  reset_world();
  dkr_install();
  Frame w;
  Darknut d;
  w.ds = {&d};
  fall(w, d);
  L.target = &d.a, L.learned = false;
  CHECK(!L.down_attack_state());
  L.press();
  CHECK(g_link.mProcID == daAlink_c::PROC_CUT_JUMP && L.finishes == 0);
  int n = 0;
  while (d.a.mActionMode1 == ACT_ENDING && n < 600) w.tick(), ++n;
  CHECK(logged("path=TIMER") && d.a.mActionMode1 == ACT_CHASEL);
  dkr_shutdown();
}
int main() {
  g_file = read_file(DKR_BCK_FILE);
  CHECK(!g_file.empty());
  t18_clip_and_install();
  for (int o = 0; o < 2; ++o) {
    t06_timer_path_then_native_death(o == 0);
    t05_finish_path_then_native_death(o == 0);
    for (int left : {1, 2, 3, 5, 30}) t07_race(o == 0, left);
  }
  t08_miss_and_interrupt(false);
  t08_miss_and_interrupt(true);
  t11_no_double_activation();
  t12_many();
  t13_lifetime();
  t14_other_variants();
  t04_no_skill();
  t03_killed_attacking();
  std::printf("DKR_WORLD_LAB checks=%llu RESULT=LAB_PASS failures=0\n", g_checks);
  return 0;
}
