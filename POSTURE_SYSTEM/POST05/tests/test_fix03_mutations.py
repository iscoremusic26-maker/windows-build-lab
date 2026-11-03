"""Bounded extra FIX03 behavior mutations; compile errors/timeouts FAIL."""
from pathlib import Path
import tempfile,shutil,subprocess,sys,json
root=Path(__file__).resolve().parents[1]
cases=[('foreign_geometry_erase', 'if(!c.owned||!c.mask||pane->getTypeID()==16)', 'if(false)'), ('slot_not_invading', 's={true,hudfix01::intersects(bounds,hf.present.band),code,bounds}', 's={true,true,code,bounds}'), ('slot_geometry_disabled', 's={true,hudfix01::intersects(bounds,hf.present.band),code,bounds}', 's={true,false,code,bounds}'), ('owner_leak', 'uf.owner[uf.owner_depth]=UiFix02::Owner{};', '(void)uf.owner[uf.owner_depth];'), ('pikari_not_masked', 'ctx.owned[i]=ctx.mask[i]=true;++uf.fx_suppressed;', 'ctx.owned[i]=ctx.mask[i]=false;++uf.fx_suppressed;'), ('pikari_foreign_owner', 'frame==&o->live->field_0x2e8[s]', 'frame!=nullptr'), ('foreign_picture_erased', 'if(ui03_primitive(key))', 'if(true)'), ('foreign_outfont_erased', 'if(ui03_invading(ui03_slots(key)))', 'if(true)')]
positive=subprocess.run([sys.executable,'-I',str(root/'tests/test_hudfix03_adapter.py')],capture_output=True,text=True,timeout=180)
assert positive.returncode==0 and 'RESULT=LAB_PASS' in positive.stdout,'unmutated adapter must pass'
results=[]
for name,old,new in cases:
 assert old in (root/'src/mod.cpp').read_text(encoding='utf-8'),name
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);shutil.copytree(root/'src',p/'src');shutil.copytree(root/'tests',p/'tests');f=p/'src/mod.cpp';f.write_text(f.read_text(encoding='utf-8').replace(old,new,1),encoding='utf-8')
  c=subprocess.run([sys.executable,'-I',str(p/'tests/test_hudfix03_adapter.py')],capture_output=True,text=True,timeout=180);out=c.stdout+c.stderr
  killed=c.returncode!=0 and 'UI03_FAIL' in out
  results.append({'mutation':name,'status':'KILLED' if killed else 'FAIL','timeout':False})
print(json.dumps({'positive_control':'PASS','count':len(results),'detected':sum(x['status']=='KILLED' for x in results),'timeouts':0,'mutations':results},indent=2));sys.exit(any(x['status']!='KILLED' for x in results))
