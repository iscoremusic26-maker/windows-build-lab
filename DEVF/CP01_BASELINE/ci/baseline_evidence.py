"""Check the official unmodified source and retain metadata only, never binaries/assets."""
import argparse,json,hashlib,subprocess,struct,os,datetime
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('mode',choices=['preflight','collect']);p.add_argument('--source',type=Path,required=True);p.add_argument('--vcpkg',type=Path,required=True);p.add_argument('--evidence',type=Path,required=True);p.add_argument('--lock',type=Path,required=True);a=p.parse_args();lock=json.loads(a.lock.read_text());a.evidence.mkdir(parents=True,exist_ok=True)
def run(cmd,cwd=None):return subprocess.check_output(cmd,cwd=cwd,text=True,encoding='utf-8',errors='replace').strip()
def sha(f):
 with f.open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def write(name,r):(a.evidence/name).write_text(json.dumps(r,indent=2)+'\n')
def verify_source():
 root=run(['git','rev-parse','HEAD'],a.source);assert root==lock['root_commit'];tag=run(['git','rev-parse',lock['root_tag']+'^{commit}'],a.source);assert tag==root
 r={'root_commit':root,'tag':lock['root_tag'],'tree':run(['git','rev-parse','HEAD^{tree}'],a.source),'gitlinks':[],'unpatched_files':[],'tracked_clean':False}
 for path,expected in lock['submodules'].items():
  gitlink=run(['git','ls-tree','HEAD',path],a.source).split()[2];head=run(['git','rev-parse','HEAD'],a.source/path);assert head==expected==gitlink
  status=run(['git','status','--porcelain','--untracked-files=no'],a.source/path);assert not status,(path,status)
  r['gitlinks'].append({'path':path,'commit':head,'url':run(['git','remote','get-url','origin'],a.source/path),'tracked_clean':True})
 for path,expected in lock['unpatched_file_hashes'].items():
  actual=sha(a.source/path);assert actual==expected,path;r['unpatched_files'].append({'path':path,'sha256':actual})
 status=run(['git','status','--porcelain','--untracked-files=no'],a.source);assert not status,status;r['tracked_clean']=True
 assert run(['git','rev-parse','HEAD'],a.vcpkg)==lock['infrastructure']['vcpkg_commit'];r['vcpkg_commit']=lock['infrastructure']['vcpkg_commit'];r['patch_applied']=False;return r
if a.mode=='preflight':
 r=verify_source();r['tools']={}
 for name,cmd in {'cmake':['cmake','--version'],'ninja':['ninja','--version'],'python':['python','--version'],'git':['git','--version'],'sccache':['sccache','--version'],'vcpkg':[str(a.vcpkg/'vcpkg.exe'),'version']}.items():r['tools'][name]=run(cmd)
 r['msvc_banner']=subprocess.run(['cl'],capture_output=True,text=True).stderr.strip();r['msvc_tools_version']=os.getenv('VCToolsVersion');r['windows_sdk_version']=os.getenv('WindowsSDKVersion');r['target_arch']=os.getenv('VSCMD_ARG_TGT_ARCH');assert r['target_arch']=='x64' or r['target_arch']=='amd64'
 write('source-preflight.json',r);print('OFFICIAL_BASELINE_PREFLIGHT_PASS root/gitlinks/unpatched files clean; MSVC x64');raise SystemExit(0)
r={'source_after':None,'source_verification_error':None,'archives':[],'fetched_git_dependencies':[],'installed_host':[],'phases':{},'status':'BASELINE_BUILD_BLOCKED','engine_execution':'NOT_TESTED','DEVF':'DISARMED_NO_REGISTRATION','patch_applied':False,'collected_at_utc':datetime.datetime.now(datetime.timezone.utc).isoformat()}
try:r['source_after']=verify_source()
except Exception as e:r['source_verification_error']=str(e)
for phase in ['vcpkg','configure','build']:
 f=a.evidence/(phase+'.json')
 if f.exists():r['phases'][phase]=json.loads(f.read_text())
build=a.source/'build/x-windows-ci-msvc'
for root in [build/'_deps',a.vcpkg/'downloads']:
 if not root.exists():continue
 for f in root.rglob('*'):
  if f.is_file() and any(f.name.endswith(ext)for ext in ['.zip','.tar.gz','.tar.xz','.7z','.tgz']):r['archives'].append({'path':str(f.relative_to(root)),'root':str(root.name),'bytes':f.stat().st_size,'sha256':sha(f)})
 if root.name=='_deps':
  for f in root.rglob('.git'):
   repo=f.parent
   try:r['fetched_git_dependencies'].append({'path':str(repo.relative_to(root)),'commit':run(['git','rev-parse','HEAD'],repo),'url':run(['git','remote','get-url','origin'],repo),'recursive_gitlinks':run(['git','submodule','status','--recursive'],repo)})
   except subprocess.CalledProcessError:pass
try:r['vcpkg_installed']=run([str(a.vcpkg/'vcpkg.exe'),'list','--x-json'])
except Exception:r['vcpkg_installed']='unavailable'
cache=build/'CMakeCache.txt';allowed={}
if cache.exists():
 for line in cache.read_text(errors='replace').splitlines():
  if not line or line.startswith(('#','//')) or '='not in line:continue
  key_type,value=line.split('=',1);key=key_type.split(':')[0]
  if key in ['CMAKE_C_COMPILER','CMAKE_CXX_COMPILER','CMAKE_BUILD_TYPE','CMAKE_GENERATOR','CMAKE_TOOLCHAIN_FILE','CMAKE_INSTALL_PREFIX','VCPKG_TARGET_TRIPLET','CMAKE_MSVC_RUNTIME_LIBRARY','CMAKE_MSVC_DEBUG_INFORMATION_FORMAT','BOREALIS_ENABLE_SENTRY']or key.startswith(('AURORA_DAWN_','AURORA_SDL3_','AURORA_NOD_')):allowed[key]=value
r['selected_cache']=allowed
exe=a.source/'build/install/Dusklight.exe'
if exe.exists():
 data=exe.read_bytes();assert data[:2]==b'MZ';pe=struct.unpack_from('<I',data,0x3c)[0];assert data[pe:pe+4]==b'PE\0\0';machine=struct.unpack_from('<H',data,pe+4)[0];dll=bool(struct.unpack_from('<H',data,pe+22)[0]&0x2000);assert machine==0x8664 and not dll
 r['installed_host']=[{'path':'build/install/Dusklight.exe','bytes':len(data),'sha256':sha(exe),'machine':'AMD64','dll':dll,'executed':False}]
if all(r['phases'].get(k,{}).get('exit_code')==0 for k in ['configure','build'])and r['source_after']and r['installed_host']:r['status']='BASELINE_BUILD_PASS'
write('baseline-result.json',r);print(r['status']+' engine_execution=NOT_TESTED patch_applied=false')
