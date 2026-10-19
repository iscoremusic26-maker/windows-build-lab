import io,json,os,urllib.request,urllib.error,zipfile
from pathlib import Path
import sys
from common import *
def api(path):
 req=urllib.request.Request("https://api.github.com/repos/iscoremusic26-maker/codex-cp00-inputs/"+path,headers={"Authorization":"Bearer "+os.environ["GH_TOKEN"],"Accept":"application/vnd.github+json"})
 return json.load(urllib.request.urlopen(req))
class NoRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,*a):return None
def download(artifact):
 url=artifact["archive_download_url"]
 try:
  urllib.request.build_opener(NoRedirect).open(urllib.request.Request(url,headers={"Authorization":"Bearer "+os.environ["GH_TOKEN"]}))
  raise RuntimeError("Expected archive redirect")
 except urllib.error.HTTPError as e:
  if e.code!=302:raise
  signed=e.headers["Location"]
 with urllib.request.urlopen(signed) as response:data=response.read(400_000_001)
 assert len(data)<=400_000_000
 return data
run=os.environ['GITHUB_RUN_ID'];arts=api('actions/runs/'+run+'/artifacts')['artifacts']
artifact=next(a for a in arts if a['name']=='DEVF_CP20_EXPLICIT_BLUE_RED_BLACK_TEST_'+run)
evidence=next(a for a in arts if a['name']=='DEVF_CP20_TEXT_'+run)
with zipfile.ZipFile(io.BytesIO(download(evidence))) as z:
 assert z.testzip() is None;result=json.loads(z.read('result.json'))
assert result['status']=='WINDOWS_MSVC_PASS_DOWNLOAD_PENDING' and result['game_executed'] is False
assert all(p['exit']==0 for p in result['phases'])
with zipfile.ZipFile(io.BytesIO(download(artifact))) as z:
 assert z.testzip() is None and z.namelist()==[BUNDLE_NAME];bundle=z.read(BUNDLE_NAME)
assert len(bundle)==result['zip']['bytes'] and sha(bundle)==result['zip']['sha256']
pin=json.loads((TASK/'SOURCE_CI_CP20.json').read_text());work=Path(os.environ['RUNNER_TEMP'])/'cp20-independent';work.mkdir()
import audit,provider_audit
with zipfile.ZipFile(io.BytesIO(bundle)) as z:
 assert z.testzip() is None and len(z.namelist())==len(set(z.namelist()))==len({n.casefold() for n in z.namelist()})
 assert all(not n.startswith(('/','\\')) and '..' not in n.split('/') and '\\' not in n for n in z.namelist())
 manifest=json.loads(z.read('VERIFICATION_MANIFEST.json'))
 assert manifest['source_commit']==result['source_commit']==pin['source_commit']
 assert manifest['ci_commit']==result['ci_commit']==os.environ['EXPECTED_CI_SHA']
 assert manifest['human']=='HUMAN_RUNTIME_PENDING' and manifest['game_executed'] is False
 assert manifest['required_pair']==dict(provider='0.14.0',expansion='0.16.3',editor='0.14.0')
 assert set(z.namelist())==set(manifest['files'])|{'VERIFICATION_MANIFEST.json','SHA256SUMS.txt'}
 for name,m in manifest['files'].items():
  raw=z.read(name);assert len(raw)==m['bytes'] and sha(raw)==m['sha256'],name
 assert z.read('SHA256SUMS.txt').decode().splitlines()==[m['sha256']+'  '+n for n,m in manifest['files'].items()]
 assert set(n for n in z.namelist() if n.endswith('.dusk'))=={DUSK_NAME,PROVIDER_NAME}
 assert set(n for n in z.namelist() if n.endswith('.exe'))=={EXE_NAME}
 assert not any(n.lower().endswith(('.dll','.arc','.kcl','.plc','.bmd','.bti','.ciso','.iso')) for n in z.namelist())
 for name,role in [(DUSK_NAME,'expansion'),(PROVIDER_NAME,'provider')]:
  data=z.read(name);path=work/name;path.write_bytes(data)
  with zipfile.ZipFile(path) as pz:
   audit.safe_members(pz);raw=pz.read('lib/windows-amd64/mod.dll')
  dll=work/(role+'.dll');dll.write_bytes(raw)
  checked=audit.verify_package(path,dll,TASK) if role=='expansion' else provider_audit.verify(path,dll)
  assert checked['dll_sha256']==manifest[role]['dll_sha256'] and checked['dll_bytes']==manifest[role]['dll_bytes']
 exe=work/EXE_NAME;exe.write_bytes(z.read(EXE_NAME));assert app_pe(exe)==result['editor_pe']
 with zipfile.ZipFile(io.BytesIO(z.read('SOURCE_COMPILED.zip'))) as src:
  assert src.testzip() is None and set(src.namelist())==set(result['source_ledger'])==set(pin['source_blobs'])
  for name,h in result['source_ledger'].items():assert sha(src.read(name))==h and sha((REPO/name).read_bytes())==h and git_blob(src.read(name))==pin['source_blobs'][name],name
 assert json.loads(z.read('SOURCE_PIN.json'))==pin
 tr=json.loads(z.read('TEST_RESULTS.json'));assert tr['cp20_lab_cases']>=137 and tr['editor_tests']['cp20']['status']=='CP20_EDITOR_LAB_PASS'
 assert tr['freeze']['status']=='CP20_SOURCE_FREEZE_PASS' and tr['source_abi']['status']=='CP19F2_MSVC_COFF_SOURCE_ABI_PASS'
 assert tr['baseline']['cp19f2_lab_cases']==40 and tr['baseline']['cp19f1_lab_cases']==73 and tr['baseline']['cp18_lab_cases']==43
 report=dict(status='INDEPENDENT_DOWNLOAD_PASS',source_commit=manifest['source_commit'],ci_commit=manifest['ci_commit'],run_id=run,artifact_id=artifact['id'],expires_at=artifact['expires_at'],zip=result['zip'],outputs={n:manifest['files'][n] for n in [DUSK_NAME,PROVIDER_NAME,EXE_NAME]},source_ledger_verified=True,PE_imports_hooks_ABI='PASS',CRC='PASS',human='HUMAN_RUNTIME_PENDING',game_executed=False)
out=Path(os.environ['RUNNER_TEMP'])/'cp20-download-evidence';out.mkdir();(out/'download-result.json').write_text(json.dumps(report,indent=2)+'\n')
print('CP20_DOWNLOAD_RESULT='+json.dumps(report,separators=(',',':')),flush=True)
