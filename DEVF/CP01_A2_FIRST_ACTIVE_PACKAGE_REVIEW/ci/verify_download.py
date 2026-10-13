"""Independent read-only verification of the real private Actions download."""
import argparse,io,json,zipfile
from pathlib import Path
from verify_package import safe_members,contents,sha,pe_exports
from audit import pe_audit
p=argparse.ArgumentParser();p.add_argument('--artifact',type=Path,required=True);p.add_argument('--runner-result',type=Path,required=True);p.add_argument('--project',type=Path,required=True);p.add_argument('--work',type=Path,required=True);p.add_argument('--github-digest',required=True);a=p.parse_args()
r=json.loads(a.runner_result.read_text());pv=r['package_verification'];a.work.mkdir(parents=True,exist_ok=True)
raw=a.artifact.read_bytes();digest='sha256:'+sha(raw)
if digest!=a.github_digest:raise RuntimeError('GitHub artifact digest mismatch')
with zipfile.ZipFile(io.BytesIO(raw)) as z:
 outer=safe_members(z,{r['filename'],'DOWNLOAD_MANIFEST.json'},set())
 package=z.read(r['filename']);manifest_raw=z.read('DOWNLOAD_MANIFEST.json');m=json.loads(manifest_raw)
if sha(package)!=pv['package_sha256'] or len(package)!=pv['package_bytes']:raise RuntimeError('Downloaded package hash/size differs')
for k,expected in {'filename':r['filename'],'source_commit':r['source_commit'],'workflow_commit':r['ci_head'],'workflow_run_id':r['run_id'],'creation_capable':True,'provider_armed_in_binary':True,'installed':False,'runtime_tested':False,'execution_authorized':False,'installation_authorized':False,'G5':'NO_GO','CP02':'NO_GO'}.items():
 if m.get(k)!=expected:raise RuntimeError('External manifest drift '+k)
with zipfile.ZipFile(io.BytesIO(package)) as z:inside=z.read('lib/windows-amd64/mod.dll')
if len(inside)!=pv['dll_bytes'] or sha(inside)!=pv['dll_sha256']:raise RuntimeError('Downloaded DLL hash/size differs')
items,inside,parsed=contents(io.BytesIO(package),(a.project/'mod.json').read_bytes(),inside)
if items!=pv['members'] or parsed!=pv['manifest']:raise RuntimeError('Member/manifest drift')
# Extraction is analysis in scratch, never LoadLibrary or installation.
dll=a.work/'downloaded-inspection.dll';dll.write_bytes(inside);dll.chmod(0o600)
pe=pe_audit(dll);exports=pe_exports(dll)
if pe!=pv['pe_audit'] or exports!=pv['exports']:raise RuntimeError('Independent PE audit differs')
if m['dll_sha256']!=sha(inside) or m['package_sha256']!=sha(package) or m['dll_bytes']!=len(inside) or m['package_bytes']!=len(package):raise RuntimeError('External digest inconsistency')
result={'status':'DOWNLOAD_VERIFIED','artifact_zip_bytes':len(raw),'artifact_zip_sha256':sha(raw),'github_digest_matched':True,'outer_members':outer,'package_bytes':len(package),'package_sha256':sha(package),'dll_bytes':len(inside),'dll_sha256':sha(inside),'inner_members':items,'manifest':m,'external_manifest_sha256':sha(manifest_raw),'pe_audit':pe,'exports':exports,'whole_dll_hash_matches_audited_loose_binary':True,'dll_loaded':False,'game_executed':False,'installed':False,'runtime':'NOT_TESTED'}
(a.work/'download-verification.json').write_text(json.dumps(result,indent=2)+'\n');(a.work/'DOWNLOAD_MANIFEST.json').write_bytes(manifest_raw)
print(json.dumps({k:result[k] for k in ['status','artifact_zip_bytes','package_bytes','package_sha256','dll_bytes','dll_sha256']},indent=2))
