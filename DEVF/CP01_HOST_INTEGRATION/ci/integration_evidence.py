"""Derived from the verified baseline collector. Check the exact patched tree; retain metadata only."""
import argparse,json,hashlib,subprocess,struct,os,datetime
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('mode',choices=['preflight','collect']);p.add_argument('--source',type=Path,required=True);p.add_argument('--vcpkg',type=Path,required=True);p.add_argument('--evidence',type=Path,required=True);p.add_argument('--lock',type=Path,required=True);a=p.parse_args();lock=json.loads(a.lock.read_text());manifest=json.loads((Path(__file__).resolve().parents[2]/'CP01_HOST_EXPERIMENT/SOURCE_MANIFEST.json').read_text());a.evidence.mkdir(parents=True,exist_ok=True)
def run(cmd,cwd=None):return subprocess.check_output(cmd,cwd=cwd,text=True,encoding='utf-8',errors='replace').strip()
def sha(f):
 with f.open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def write(name,r):(a.evidence/name).write_text(json.dumps(r,indent=2)+'\n')
def verify_source():
 root=run(['git','rev-parse','HEAD'],a.source);assert root==lock['root_commit'];tag=run(['git','rev-parse',lock['root_tag']+'^{commit}'],a.source);assert tag==root
 r={'root_commit':root,'tag':lock['root_tag'],'tree':run(['git','rev-parse','HEAD^{tree}'],a.source),'gitlinks':[],'patched_files':[],'only_expected_tracked_changes':False}
 for path,expected in lock['submodules'].items():
  gitlink=run(['git','ls-tree','HEAD',path],a.source).split()[2];head=run(['git','rev-parse','HEAD'],a.source/path);assert head==expected==gitlink
  status=run(['git','status','--porcelain','--untracked-files=no'],a.source/path);assert not status,(path,status)
  r['gitlinks'].append({'path':path,'commit':head,'url':run(['git','remote','get-url','origin'],a.source/path),'tracked_clean':True})
 for item in manifest['files']:
  path=item['path'];actual=sha(a.source/path);assert actual==item['after_sha256'],path;r['patched_files'].append({'path':path,'sha256':actual})
 changes=run(['git','diff','--name-only','HEAD'],a.source).splitlines();assert set(changes)=={i['path'] for i in manifest['files']},changes
 assert not run(['git','diff','--cached','--name-only'],a.source),'Unexpected staged changes'
 r['only_expected_tracked_changes']=True;r['changed_paths']=changes;r['original_patch_sha256']=manifest['patch_sha256']
 assert run(['git','rev-parse','HEAD'],a.vcpkg)==lock['infrastructure']['vcpkg_commit'];r['vcpkg_commit']=lock['infrastructure']['vcpkg_commit'];r['patch_applied']=True;return r
if a.mode=='preflight':
 r=verify_source();r['tools']={}
 for name,cmd in {'cmake':['cmake','--version'],'ninja':['ninja','--version'],'python':['python','--version'],'git':['git','--version'],'sccache':['sccache','--version'],'vcpkg':[str(a.vcpkg/'vcpkg.exe'),'version']}.items():r['tools'][name]=run(cmd)
 r['msvc_banner']=subprocess.run(['cl'],capture_output=True,text=True).stderr.strip();r['msvc_tools_version']=os.getenv('VCToolsVersion');r['windows_sdk_version']=os.getenv('WindowsSDKVersion');r['target_arch']=os.getenv('VSCMD_ARG_TGT_ARCH');assert r['target_arch']=='x64' or r['target_arch']=='amd64'
 write('patched-source-preflight.json',r);print('PATCHED_HOST_PREFLIGHT_PASS exact root/gitlinks and 12 patched hashes; MSVC x64');raise SystemExit(0)
r={'source_after':None,'source_verification_error':None,'archives':[],'fetched_git_dependencies':[],'installed_host':[],'phases':{},'status':'PATCHED_HOST_BUILD_BLOCKED','engine_execution':'NOT_TESTED','DEVF':'DISARMED_NO_REGISTRATION','patch_applied':True,'collected_at_utc':datetime.datetime.now(datetime.timezone.utc).isoformat()}
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
# Observe the full generated graph and actual Ninja include dependencies, metadata only.
commands=build/'compile_commands.json'
if commands.exists():
 entries=json.loads(commands.read_text());patched_cpp={x['path'] for x in manifest['files'] if x['path'].endswith('.cpp')}
 present={x['path']:sum(e['file'].replace('\\','/').endswith('/'+x['path'])for e in entries)for x in manifest['files']if x['path'].endswith('.cpp')}
 assert all(present.values()),present
 r['compile_graph']={'translation_units':len(entries),'patched_cpp_entries':present,'headers_recompiled_together':True,'basis':'fresh runner build; Ninja dependency scan below'}
 if r['phases'].get('build',{}).get('exit_code')==0:
  deps=run(['ninja','-t','deps'],build);headers={x['path']:[] for x in manifest['files']if not x['path'].endswith('.cpp')};obj=None
  for line in deps.splitlines():
   if not line.startswith(' ') and ': #deps 'in line:obj=line.split(': #deps ',1)[0]
   elif obj:
    normalized=line.strip().replace('\\','/')
    for h in headers:
     if normalized.endswith('/'+h)or normalized==h:headers[h].append(obj)
  r['compile_graph']['actual_header_consumers']=headers
  assert all(headers.values()),headers
if all(r['phases'].get(k,{}).get('exit_code')==0 for k in ['configure','build'])and r['source_after']and r['installed_host']:r['status']='PATCHED_HOST_BUILD_PASS'
write('integration-result.json',r);print(r['status']+' engine_execution=NOT_TESTED patch_applied=true')
