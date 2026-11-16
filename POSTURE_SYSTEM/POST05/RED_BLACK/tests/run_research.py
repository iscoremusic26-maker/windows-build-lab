"""Run exact CP21 policies and offline phase proposal. No mod build or gameplay."""
import argparse,hashlib,json,os,pathlib,shutil,subprocess,tempfile
p=argparse.ArgumentParser();p.add_argument('--cp21',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True);p.add_argument('--sanitize',action='store_true');a=p.parse_args()
root=pathlib.Path(__file__).resolve().parent;a.out.mkdir(parents=True,exist_ok=True)
inc=a.cp21/'DEVF/CP21_PUBLIC_RELEASE/runtime/DEVF/CP19_FIX02_UNIVERSAL_AUDIO_BLACK_SHIELD_CHIP/expansion'
manifest=json.loads((a.cp21.parent/'CP21_MANIFEST.json').read_text())
assert manifest['source_commit']=='192a7731623d69baacdd37011db60ce894d9de18'
for item in manifest['files']:
 b=(a.cp21/item['path']).read_bytes()
 assert hashlib.sha256(b).hexdigest()==item['sha256']
 assert hashlib.sha1(b'blob '+str(len(b)).encode()+b'\0'+b).hexdigest()==item['git_blob_sha1']
compiler='cl' if os.name=='nt' else 'g++' 
def run(src,folder,include=None,sanitize=False):
 exe=folder/('probe.exe' if os.name=='nt' else 'probe')
 if os.name=='nt':cmd=['cl','/nologo','/std:c++20','/EHsc','/utf-8',str(src),'/Fe:'+str(exe),'/Fo:'+str(folder/'probe.obj')]+(['/I'+str(include)] if include else [])
 else:cmd=['g++','-std=c++20','-O1',str(src),'-o',str(exe)]+(['-I',str(include)] if include else [])+(['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'] if sanitize else [])
 b=subprocess.run(cmd,capture_output=True,text=True)
 if b.returncode:raise RuntimeError(b.stdout+b.stderr)
 return subprocess.run([str(exe)],capture_output=True,text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
results={}
for name,include in [('interaction_probe',inc),('shadow_lab',None)]:
 with tempfile.TemporaryDirectory() as td:r=run(root/(name+'.cpp'),pathlib.Path(td),include,a.sanitize)
 (a.out/(name+'.txt')).write_text(r.stdout+r.stderr);results[name]={'exit':r.returncode,'output':r.stdout.strip()};assert r.returncode==0,results[name]
mutations=[('common','!x.common_verified','false'),('state0','x.state==0','false'),('frame12','!(x.frame>2)','false'),('mode','x.mode!=4','false'),('stop','x.stopped','false'),('playmode','x.playmode!=0','false'),('rate','!(x.rate==1.0f||x.rate==1.15f||x.rate==1.30f||x.rate==2.0f)','false'),('profile','!x.profile_verified','false'),('power3','x.power_pre==3','false'),('powerchange','x.power_pre!=x.power_post','false'),('release','release_enabled=false','release_enabled=true')]
out=[]
for name,old,new in mutations:
 with tempfile.TemporaryDirectory() as td:
  td=pathlib.Path(td);text=(root/'shadow_policy.hpp').read_text();assert old in text
  (td/'shadow_policy.hpp').write_text(text.replace(old,new,1));shutil.copy(root/'shadow_lab.cpp',td/'shadow_lab.cpp');r=run(td/'shadow_lab.cpp',td);out.append({'name':name,'detected':r.returncode!=0})
results['mutations']=out;results['release_enabled']=False;results['labels']=['LAB_ONLY','HUMAN_RUNTIME_PENDING'];results['lsan_disabled_reason']='LeakSanitizer unsupported under traced executor; ASAN and UBSAN enabled when requested.'
(a.out/'research-results.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(results,indent=2));assert all(x['detected'] for x in out)
