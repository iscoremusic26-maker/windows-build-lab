"""Bounded mutations of active production FIX02 callbacks and CP06 replica.
Compilation errors/timeouts are FAIL, never a killed mutation. Authentic oracle remains untouched.
"""
from pathlib import Path
import tempfile,shutil,subprocess,sys,json
root=Path(__file__).resolve().parents[1]
cases=[
 ('ui_disabled','src/mod.cpp','return uf.enabled&&hf.present.inside&&hf.present.on;','return false;','test_hudfix02_adapter.py'),
 ('ui_no_pane_suppression','src/mod.cpp','mods::arg_ref<bool>(args,5)=false;','mods::arg_ref<bool>(args,5)=true;','test_hudfix02_adapter.py'),
 ('ui_no_restore','src/mod.cpp','mods::arg_ref<bool>(args,5)=c.visible;','mods::arg_ref<bool>(args,5)=false;','test_hudfix02_adapter.py'),
 ('ui_outfont_leak','src/mod.cpp','++uf.outfont_suppressed;return HOOK_SKIP_ORIGINAL;','++uf.outfont_suppressed;return HOOK_CONTINUE;','test_hudfix02_adapter.py'),
 ('ui_direct_picture_leak','src/mod.cpp','++uf.picture_suppressed;return HOOK_SKIP_ORIGINAL;','++uf.picture_suppressed;return HOOK_CONTINUE;','test_hudfix02_adapter.py'),
 ('ui_string_leak','src/mod.cpp','font->mColor1.a=0;','font->mColor1.a=255;','test_hudfix02_adapter.py'),
 ('ui_font_restore_wrong','src/mod.cpp','font->mColor1=lease.colors[0];','font->mColor1.a=0;','test_hudfix02_adapter.py'),
 ('mirror_context_reset_lost','src/alignment_core.hpp','if(changed){clear(env);','if(changed){','test_alignment_pair.py'),
 ('mirror_native_handle_retained','src/alignment_core.hpp','views[i].handle=nullptr;','(void)views[i].handle;','test_alignment_pair.py'),
]
# Exact actual tracker anchors are preflighted before running any mutations.
s=(root/'src/cp06/tracker.cpp').read_text(encoding='utf-8')
for old,new,name in [('+ 8,','+ 9,','mirror_wrong_offset')]:
 if old in s:cases.append((name,'src/cp06/tracker.cpp',old,new,'test_alignment_pair.py'))
cases.append(('mirror_no_alpha_reset','src/cp06/reused/normal_model.h','s.anchorHeight=0;', 's.anchorHeight=20;', 'test_alignment_pair.py')) if 's.anchorHeight=0;' in (root/'src/cp06/reused/normal_model.h').read_text() else None
for name,fn,old,new,lab in cases:
 assert old in (root/fn).read_text(encoding='utf-8'),name
results=[]
for name,fn,old,new,lab in cases:
 with tempfile.TemporaryDirectory() as td:
  td=Path(td);shutil.copytree(root/'src',td/'src');shutil.copytree(root/'tests',td/'tests');p=td/fn;p.write_text(p.read_text(encoding='utf-8').replace(old,new,1),encoding='utf-8')
  c=subprocess.run([sys.executable,'-I',str(td/'tests'/lab)],capture_output=True,text=True,timeout=180)
  out=c.stdout+c.stderr;detected=c.returncode!=0 and ('UI02_FAIL' in out or 'ALIGN02_FAIL' in out or 'AssertionError:' in out)
  results.append({'mutation':name,'status':'KILLED' if detected else 'FAIL','detected':detected,'timeout':False})
print(json.dumps({'count':len(results),'detected':sum(x['detected'] for x in results),'timeouts':0,'mutations':results},indent=2));sys.exit(0 if all(x['detected'] for x in results) else 1)
