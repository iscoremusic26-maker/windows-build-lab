"""Reuse the established pinned source input. Extract only files needed by this experiment."""
import hashlib,json,zipfile,argparse,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--work',type=Path,required=True);a=p.parse_args()
here=a.root/'DEVF/CP01_HOST_EXPERIMENT';manifest=json.loads((here/'SOURCE_MANIFEST.json').read_text());archive=a.root/'dusklight-2.0.3.zip'
with archive.open('rb') as f:assert hashlib.file_digest(f,'sha256').hexdigest()==manifest['source_archive_sha256'],'Pinned source ZIP mismatch or LFS pointer'
source=a.work/'dusklight-2.0.3';source.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(archive) as z:
 names=[x['path'] for x in manifest['files']]+[n.removeprefix('dusklight-2.0.3/') for n in z.namelist() if n.startswith('dusklight-2.0.3/sdk/include/') and not n.endswith('/')]
 for path in set(names):
  dst=(source/path).resolve();assert dst.is_relative_to(source.resolve());dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(z.read('dusklight-2.0.3/'+path))
subprocess.run(['python',str(here/'ci/prepare_experiment.py'),'--source',str(source),'--out',str(a.work/'functions')],check=True)
print('HOST_FULL_BUILD_BLOCKED: exact Borealis/Aurora full submodules absent; lab compiles extracted functions only')
