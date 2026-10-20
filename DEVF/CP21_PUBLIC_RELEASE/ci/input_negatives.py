"""Exercise the same input gate CMake invokes. All mutations are confined to temporary copies."""
import json,subprocess,sys,tempfile,shutil
from pathlib import Path
TASK=Path(__file__).resolve().parents[1]
def run():
 cases=[]
 with tempfile.TemporaryDirectory(prefix='cp21-input-negatives-') as tmp:
  root=Path(tmp);shutil.copytree(TASK/'inputs',root/'inputs');(root/'ci').mkdir();shutil.copyfile(TASK/'ci/generate_config.py',root/'ci/generate_config.py')
  for n in ['provider','expansion']:(root/n).mkdir()
  raw=(root/'inputs/encounters.json').read_bytes();j=json.loads(raw)
  def gate():return subprocess.run([sys.executable,root/'ci/generate_config.py',root/'out.hpp'],capture_output=True).returncode
  assert gate()==0
  mutations={
   'count':lambda d:d['encounters'].pop(),
   'identity':lambda d:d['encounters'][0].update(id='bokoblin-999'),
   'float32-coordinate':lambda d:d['encounters'][0].update(x=d['encounters'][0]['x']+1),
   'yaw':lambda d:d['encounters'][0].update(yaw_s16=1),
   'room':lambda d:d['encounters'][0].update(room=3),
   'layer':lambda d:d['encounters'][0].update(layer=5),
   'enabled':lambda d:d['encounters'][0].update(enabled=False),
   'resource':lambda d:d['encounters'][0].update(resource='r09-00'),
   'variant':lambda d:d['encounters'][0].update(bokoblin_variant='red'),
   'hp':lambda d:next(e for e in d['encounters'] if e['bokoblin_variant']=='black').update(hp=441),
   'authorization':lambda d:d['resources'][0].update(authorization='RUNTIME_VERIFIED'),
   'order':lambda d:d['encounters'].reverse(),
   'metadata':lambda d:d.update(revision=1)}
  for name,mutate in mutations.items():
   d=json.loads(raw);mutate(d);(root/'inputs/encounters.json').write_text(json.dumps(d));assert gate()!=0,name;cases.append(name)
  (root/'inputs/encounters.json').write_bytes(raw+b' ');assert gate()!=0;cases.append('predefined-hash-whitespace')
  (root/'inputs/encounters.json').write_bytes(raw.replace(b'\n',b'\r\n'));assert gate()==0;cases.append('CRLF-equivalence-positive')
  (root/'inputs/encounters.json').write_bytes(raw)
  image=root/'inputs/assets/provider/icon.png';b=image.read_bytes();image.write_bytes(b+b'x');assert gate()!=0;cases.append('PNG-hash')
  image.write_bytes(b);image.unlink();assert gate()!=0;cases.append('PNG-missing')
 return dict(status='FROZEN_INPUT_BUILD_GATE_PASS',cases=cases)
if __name__=='__main__':print(json.dumps(run(),indent=2))
