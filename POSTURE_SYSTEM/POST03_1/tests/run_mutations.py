"""POST03: mutaciones de prueba SOBRE COPIAS TEMPORALES (herramienta local, no forma parte del modulo).
Cada mutacion de logica debe hacer FALLAR hit_lab/lifecycle_lab; cada violacion de pasividad plantada en mod.cpp debe ser RECHAZADA por source_audit.
Uso: python3 -I tests/run_mutations.py   (requiere g++ C++20)"""
import json, shutil, subprocess, sys, tempfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
LOGIC = [
 ('M01 sin puerta de serial (duplicados pasan)', 'src/engine.hpp', 'if (!obs.ledger.try_emit(fr.slot, fr.generation, fr.serial)) return false;', 'obs.ledger.try_emit(fr.slot, fr.generation, fr.serial);'),
 ('M02 cooldown contado como candidato', 'src/hit_core.hpp', '  if (cooldown != 0) return PreClass::COOLDOWN_NO_PROCESS;\n', ''),
 ('M03 mpCollider obsoleto aceptado (src forzado)', 'src/hit_core.hpp', '    r.reason = ((b0 && !raw0) || (b1 && !raw1)) ? ColReason::RawNull : ColReason::NotEqualToRaw;\n    return r;', '    r.reason = ((b0 && !raw0) || (b1 && !raw1)) ? ColReason::RawNull : ColReason::NotEqualToRaw;\n    r.src = 0; r.deref_ok = true;\n    return r;'),
 ('M04 generacion constante (resurreccion)', 'src/lifecycle_core.hpp', 'n.generation = next_generation();', 'n.generation = 1;'),
 ('M05 sin validar generacion entre PRE y POST', 'src/engine.hpp', 'if (!valid) {  // retirada', 'if (false) {  // retirada'),
 ('M06 todo HIT_TYPE normal = espada', 'src/hit_core.hpp', '  if (type == 0) return r;\n', '  if (type == 0) return r;\n  r.family = Family::Sword; r.native_first = Family::Sword; r.groups = 1; r.reason = "NATIVE_BRANCH_ORDER"; return r;\n'),
 ('M07 confirmar sin proceso (sin cooldown)', 'src/hit_core.hpp', 'j.processed = pre.cooldown == 0 && post.cooldown != 0;', 'j.processed = true;'),
 ('M08 leer collider sin autorizacion', 'src/engine.hpp', 'if (j.read_collider) ok = read_at(r.selected, type);', 'ok = read_at(r.selected, type);'),
 ('M09 Finish nativo ignorado', 'src/hit_core.hpp', '    case PreClass::CUT_DOWN_FINISH_PATH:\n      if (post.health == 0', '    case PreClass::UNKNOWN:\n      if (post.health == 0'),
 ('M10 frames obsoletos no se descartan', 'src/lifecycle_core.hpp', 'const bool stale = frames_[i].actor == actor || frames_[i].serial + kStaleAge < serial_ + 1;', 'const bool stale = false;'),
 ('M11 firma sin mOldActionMode', 'src/hit_core.hpp', 'post.old_mode == pre.mode;', 'true;'),
 ('M12 serial repetido admitido (< en lugar de <=)', 'src/lifecycle_core.hpp', 'if (serial <= s.last_event_serial) {', 'if (serial < s.last_event_serial) {'),
 ('M13 fusion por actor distinto (dedup global)', 'src/lifecycle_core.hpp', 'if (serial <= s.last_event_serial) {', 'if (serial <= (last_global_ < s.last_event_serial ? s.last_event_serial : last_global_)) {'),
 ('M14 eviccion de ACTIVE', 'src/lifecycle_core.hpp', 'if (s_[i].life == Life::Unverified && (best < 0', 'if ((s_[i].life == Life::Unverified || s_[i].life == Life::Active) && (best < 0'),
 ('M15 direccion reciclada sin retiro', 'src/lifecycle_core.hpp', 'if (is_live(s) && s.ptr == ptr && s.id != id) {', 'if (false) {'),
 ('M16 dual TG cuenta pair_distinct siempre', 'src/engine.hpp', 'if (r.raw0 != r.raw1) ++c.pair_distinct;', '++c.pair_distinct;'),
]
LOGIC += [
 ('N01 alineacion de lectura no exigida', 'src/hit_core.hpp', 'return p >= kMinPlausible && (p & (kReadAlignBytes - 1u)) == 0;', 'return p >= kMinPlausible;'),
 ('N02 alineacion de 8 (regresion de POST03)', 'src/hit_core.hpp', 'return p >= kMinPlausible && (p & (kReadAlignBytes - 1u)) == 0;', 'return p >= kMinPlausible && (p & 7u) == 0;'),
 ('N03 coincidencia sin igualdad con el hit obj crudo', 'src/hit_core.hpp', 'if (b1 && raw1 == sel) {', 'if (b1) {'),
 ('N04 umbral minimo eliminado', 'src/hit_core.hpp', 'if (sel < kMinPlausible) {', 'if (false) {'),
 ('N05 cc1 deja de prevalecer', 'src/hit_core.hpp', 'if (b1 && raw1 == sel) {', 'if (b1 && raw1 == sel && !b0) {'),
 ('N06 se lee el collider equivocado (raw0)', 'src/engine.hpp', 'if (j.read_collider) ok = read_at(r.selected, type);', 'if (j.read_collider) ok = read_at(fr.raw0, type);'),
 ('N07 doble lectura por evento', 'src/engine.hpp', 'if (j.read_collider) ok = read_at(r.selected, type);', 'if (j.read_collider) { ok = read_at(r.selected, type); ok = read_at(r.selected, type); }'),
 ('N08 lectura sin autorizacion de judge', 'src/hit_core.hpp', 'j.read_collider = col.deref_ok;', 'j.read_collider = col.src >= -1;'),
 ('N09 desalineado coincidente se lee', 'src/hit_core.hpp', '  if (!deref_alignment_ok(sel)) {\n    r.reason = ColReason::SelectedAlignment;\n    return r;\n  }\n', ''),
]
SRC = [
 ('S01 escribir health', 'a->health = 1;'), ('S02 escribir mpCollider', 'a->mAtInfo.mpCollider = nullptr;'), ('S03 getter cacheador GetTgHitAc', '(void)a->mSphs_cc[0].GetTgHitAc();'),
 ('S04 HOOK_SKIP_ORIGINAL', 'return HOOK_SKIP_ORIGINAL;'), ('S05 lookup dentro de do_pre', '{ Pick q{}; (void)lookup_actor(q); }'),
 ('S06 leer mHitType', '(void)a->mAtInfo.mHitType;'), ('S07 segunda desreferencia de collider', 'type = reinterpret_cast<const cCcD_Obj*>(sel)->GetAtType();'),
 ('S08 ChkTgHit derivado', '(void)a->mSphs_cc[0].ChkTgHit();'), ('S09 setActionMode', 'a->setActionMode(6, 0);'),
 ('S11 IsBadReadPtr', '(void)IsBadReadPtr(nullptr, 1);'), ('S12 VirtualQuery', '(void)VirtualQuery(nullptr, nullptr, 0);'), ('S13 imprimir un puntero', 'char q[32]; std::snprintf(q, sizeof(q), "%p", (void*)a);'),
 ('S14 direccion en hex ancho', 'char q[32]; std::snprintf(q, sizeof(q), "%016llx", 0ull);'), ('S10 escribir en collider via out', 'fopAc_ac_c* out2 = nullptr; out2 = out; out->health = 0;'),
]
def sh(cmd, cwd): return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
def build_run(dirp):
    res = []
    for lab in ('hit_lab', 'lifecycle_lab', 'collider_lab'):
        b = sh(['g++', '-std=c++20', '-O1', f'tests/{lab}.cpp', '-o', f'{lab}.bin'], dirp)
        if b.returncode: return 'COMPILE_ERROR', b.stderr[:300]
        r = sh([f'./{lab}.bin'], dirp)
        res.append(r.returncode)
    return ('DETECTED' if any(res) else 'SURVIVED'), ''
def main():
    bad = 0; out = []
    for name, rel, old, new in LOGIC:
        with tempfile.TemporaryDirectory() as td:
            d = Path(td); shutil.copytree(ROOT / 'src', d / 'src'); shutil.copytree(ROOT / 'tests', d / 'tests')
            f = d / rel; s = f.read_text()
            if s.count(old) != 1: out.append((name, 'INVALID_MUTATION', f'pattern count {s.count(old)}')); bad += 1; continue
            if 'last_global_' in new: s = s.replace('  Instance s_[kCapacity]{};', '  Instance s_[kCapacity]{};\n  std::uint64_t last_global_ = 0;')
            s = s.replace(old, new); f.write_text(s)
            if 'last_global_' in new: f.write_text(f.read_text().replace('s.last_event_serial = serial;', 's.last_event_serial = serial; last_global_ = serial;'))
            st, why = build_run(d)
            out.append((name, st, why)); bad += st != 'DETECTED'
    mod = (ROOT / 'src/mod.cpp').read_text()
    for name, snippet in SRC:
        with tempfile.TemporaryDirectory() as td:
            d = Path(td); shutil.copytree(ROOT / 'src', d / 'src')
            s = (d / 'src/mod.cpp').read_text()
            anchor = 'void do_pre(daE_OC_c* a) noexcept {\n'
            if 'read_at_type' in snippet or 'GetAtType' in snippet: s = s.replace('  type = reinterpret_cast<const cCcD_Obj*>(sel)->GetAtType();  // inline const', '  type = reinterpret_cast<const cCcD_Obj*>(sel)->GetAtType();\n  ' + snippet)
            elif 'HOOK_SKIP' in snippet: s = s.replace('if (g.eng.obs.disabled || !args) return HOOK_CONTINUE;', 'if (g.eng.obs.disabled || !args) ' + snippet)
            else: s = s.replace(anchor, anchor + '  ' + snippet + '\n', 1)
            (d / 'src/mod.cpp').write_text(s)
            r = sh([sys.executable, '-I', str(ROOT / 'ci/source_audit.py'), str(d)], d)
            st = 'REJECTED' if r.returncode else 'SURVIVED'
            out.append((name, st, '')); bad += st != 'REJECTED'
    for name, st, why in out: print(f'{st:18s} {name} {why}')
    print(json.dumps({'logic_mutations': len(LOGIC), 'source_negatives': len(SRC), 'not_detected': bad}))
    return 1 if bad else 0
if __name__ == '__main__': sys.exit(main())
