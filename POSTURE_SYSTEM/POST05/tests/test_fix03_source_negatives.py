"""Source-gate negatives; each modified production tree must be rejected."""
from pathlib import Path
import tempfile,shutil,sys,json,subprocess
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'ci'));import source_audit
cases=[('foreign_geometry_erase', 'if(!c.owned||!c.mask||pane->getTypeID()==16)', 'if(false)'), ('slot_not_invading', 's={true,hudfix01::intersects(bounds,hf.present.band),code,bounds}', 's={true,true,code,bounds}'), ('slot_geometry_disabled', 's={true,hudfix01::intersects(bounds,hf.present.band),code,bounds}', 's={true,false,code,bounds}'), ('owner_leak', 'uf.owner[uf.owner_depth]=UiFix02::Owner{};', '(void)uf.owner[uf.owner_depth];'), ('pikari_not_masked', 'ctx.owned[i]=ctx.mask[i]=true;++uf.fx_suppressed;', 'ctx.owned[i]=ctx.mask[i]=false;++uf.fx_suppressed;'), ('pikari_foreign_owner', 'frame==&o->live->field_0x2e8[s]', 'frame!=nullptr'), ('foreign_picture_erased', 'if(ui03_primitive(key))', 'if(true)'), ('foreign_outfont_erased', 'if(ui03_invading(ui03_slots(key)))', 'if(true)'), ('owner_check_lost', 'meter->mpEmpButton==self', 'true'), ('tags_ignored', 'pane->mInfoTag!=tag', 'false'), ('screen_guard_lost', 'if((ctx&&c.key==ctx->root)||pane->getTypeID()==8)', 'if(false)'), ('old_generic_mask', 'ctx.owned[i]=ctx.mask[i]=false;', 'ctx.owned[i]=ctx.mask[i]=true;')]
r=[]
for name,old,new in cases:
 with tempfile.TemporaryDirectory() as td:
  p=Path(td)/'p';shutil.copytree(root,p,ignore=shutil.ignore_patterns('__pycache__'));f=p/'src/mod.cpp';s=f.read_text(encoding='utf-8');assert old in s;f.write_text(s.replace(old,new,1),encoding='utf-8');c=subprocess.run([sys.executable,str(root/'ci/source_audit.py'),str(p)],capture_output=True,text=True,timeout=60);errors=json.loads(c.stdout)['errors'];assert c.returncode==1 and errors,name;r.append({'name':name,'rejected':True})
print(json.dumps({'status':'NEGATIVES_PASS','count':len(r),'cases':r},indent=2))
