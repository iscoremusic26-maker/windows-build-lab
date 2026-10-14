"""Verify actual private Actions downloads without executing/loading the DLL."""
import argparse,json,stat,zipfile
from pathlib import Path
from audit import old,sha,verify_package
package=old.package

def verify(delivery,text,project,work,delivery_sha,text_sha):
 if sha(delivery.read_bytes())!=delivery_sha or sha(text.read_bytes())!=text_sha:raise RuntimeError('GitHub artifact digest mismatch')
 with zipfile.ZipFile(text) as z:
  names=z.namelist()
  if len(names)!=len(set(names)) or any('/' in n or not n.endswith(('.log','.json')) for n in names) or z.testzip():raise RuntimeError('Unsafe/invalid text artifact')
  ledger=json.loads(z.read('evidence-sha256.json'))
  if set(names)!=(set(ledger)|{'evidence-sha256.json'}):raise RuntimeError('Evidence member ledger mismatch')
  for name,item in ledger.items():
   raw=z.read(name)
   if len(raw)!=item['bytes'] or sha(raw)!=item['sha256']:raise RuntimeError('Evidence digest mismatch '+name)
  result=json.loads(z.read('result.json'));expected_delivery_manifest=z.read('DOWNLOAD_MANIFEST.json')
 if result['status']!='CP03_PACKAGE_PASS_DOWNLOAD_PENDING':raise RuntimeError('Build gates not passed')
 with zipfile.ZipFile(delivery) as z:
  external_raw=z.read('DOWNLOAD_MANIFEST.json')
  if external_raw!=expected_delivery_manifest:raise RuntimeError('External delivery manifest byte mismatch')
  meta=json.loads(external_raw);name=meta['filename']
  outer=package.safe_members(z,expected_files={name,'DOWNLOAD_MANIFEST.json'},allowed_dirs=set());raw=z.read(name)
 if len(raw)!=meta['package_bytes'] or sha(raw)!=meta['package_sha256']:raise RuntimeError('Downloaded package hash/size mismatch')
 for key in ['source_commit','ci_head','run_id']:
  mk={'source_commit':'source_commit','ci_head':'workflow_commit','run_id':'workflow_run_id'}[key]
  if str(result[key])!=str(meta[mk]):raise RuntimeError('Source/workflow provenance mismatch')
 for key,expected in {'adds_native_actor_entry':True,'read_only':False,'ActorService_used':False,'registered_custom_profiles':False,'runtime_tested':False,'installed':False,'execution_authorized':False,'CP03_RUNTIME':'HUMAN_RUNTIME_NOT_TESTED','CP04':'NOT_AUTHORIZED','HookService_used':True,'expected_initial_hp_by_code':150}.items():
  if meta[key]!=expected:raise RuntimeError('Delivery safety metadata drift')
 for rel,h in result['source_ledger'].items():
  if sha((project/rel).read_bytes().replace(b'\r\n',b'\n'))!=h:raise RuntimeError('Published source does not match compiled ledger '+rel)
 if name!='DEVF_CP03_STAGE_HOOK_HP150_windows-amd64_'+meta['source_commit']+'.dusk':raise RuntimeError('Package provenance filename mismatch')
 work.mkdir(parents=True,exist_ok=True);dusk=work/name;dusk.write_bytes(raw)
 with zipfile.ZipFile(dusk) as z: dll=work/'downloaded-own-inspection.dll';dll.write_bytes(z.read('lib/windows-amd64/mod.dll'))
 inspection=verify_package(dusk,dll,project,work)
 for k in ['package_bytes','package_sha256','dll_bytes','dll_sha256','manifest_sha256']:
  if inspection[k]!=result['package_verification'][k]:raise RuntimeError('Compiled/downloaded evidence mismatch '+k)
 if inspection['pe_audit']!=result['binary_audit'] or inspection['pe_audit']['exports']!=result['binary_audit']['exports']:raise RuntimeError('Downloaded PE changed')
 return {'status':'DOWNLOAD_VERIFIED','outer_artifact_members':outer,'delivery_artifact_sha256':delivery_sha,'text_artifact_sha256':text_sha,'all_CRC_and_hashes_pass':True,'source_ledger_verified':True,'compiled_DLL_sha256_matches':True,'package':inspection,'source_commit':meta['source_commit'],'workflow_commit':meta['workflow_commit'],'run_id':meta['workflow_run_id'],'dll_loaded':False,'game_executed':False,'installed':False,'execution_authorized':False,'read_only':False,'CP03_RUNTIME':'HUMAN_RUNTIME_NOT_TESTED','CP04':'NOT_AUTHORIZED','HookService_used':True,'expected_initial_hp_by_code':150}

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--delivery',type=Path,required=True);p.add_argument('--text',type=Path,required=True);p.add_argument('--work',type=Path,required=True);p.add_argument('--delivery-sha256',required=True);p.add_argument('--text-sha256',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 result=verify(a.delivery,a.text,Path(__file__).resolve().parents[1],a.work,a.delivery_sha256,a.text_sha256);a.output.write_text(json.dumps(result,indent=2)+'\n');print(result['status'],result['package']['package_sha256'])
