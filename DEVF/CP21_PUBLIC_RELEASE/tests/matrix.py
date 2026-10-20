"""One byte-identical parser corpus consumed independently by C# and C++. Synthetic lab data only."""
import copy,json,sys
from pathlib import Path
def generate(folder):
 folder=Path(folder);folder.mkdir(parents=True,exist_ok=True);cases=[]
 resource=dict(key='r01-00',stage='F_SP121',room=1,variant='00',path='Stage/F_SP121/R01_00.arc',arc_sha256='a'*64,kcl_sha256='b'*64,plc_sha256='c'*64,plc_room=1,bounds=[-30000,30000,0,0,-30000,30000],authorization='STATIC_QA_NOT_RUNTIME_VERIFIED')
 encounter=dict(id='legacy',enabled=True,enemy='E_OC2',resource='r01-00',stage='F_SP121',room=1,layer=6,x=1000,y=5,z=1000,yaw_s16=-2421,native_profile=510,parameters=4294902016,qa_confirmed=True,layer_state='QA_CANDIDATE')
 base=dict(document_type='devf_map_encounters',schema_version=2,disc_id='GZ2E01',revision=0,resources=[resource],encounters=[encounter])
 def add(name,d,valid=True):
  raw=d if isinstance(d,bytes) else json.dumps(d,separators=(',',':'),ensure_ascii=True).encode()
  (folder/(name+'.json')).write_bytes(raw);cases.append((name,valid))
 def change(name,fields,valid):
  d=copy.deepcopy(base);d['encounters'][0].update(fields);add(name,d,valid)
 add('legacy-absent',base);change('legacy-normal',dict(bokoblin_variant='normal'),True)
 change('existing-black',dict(bokoblin_variant='black',hp=150),True)
 change('blue',dict(bokoblin_variant='blue',enemy='E_OC'),True);change('red',dict(bokoblin_variant='red'),True);change('black',dict(bokoblin_variant='black',hp=440),True)
 for vi,v in enumerate(['blue','red','black','normal','purple','BLUE','',None,3]):
  for enemy in ['E_OC','E_OC2','E_OC3']:
   fields=dict(bokoblin_variant=v,enemy=enemy)
   if v=='black':fields['hp']=440
   valid=v in ['blue','red','black','normal'] and enemy==('E_OC' if v=='blue' else 'E_OC2')
   change('variant-'+str(vi)+'-'+str(v)+'-'+enemy,fields,valid)
 for v in ['normal','blue','red','black']:
  for hp in [0,1,999,1000,None,40.5,'440']:
   change('hp-'+v+'-'+str(hp),dict(bokoblin_variant=v,enemy='E_OC' if v=='blue' else 'E_OC2',hp=hp),v=='black' and type(hp)==int and 1<=hp<=999)
 change('unknown-key',dict(unknown=1),False);change('wrong-profile',dict(native_profile=511),False);change('wrong-params',dict(parameters=0),False)
 change('active-unconfirmed',dict(qa_confirmed=False),False);change('disabled-default',dict(enabled=False,qa_confirmed=False,bokoblin_variant='blue',enemy='E_OC'),True)
 for field,value in [('room',64),('layer',15),('yaw_s16',32768),('y',6)]:change('bad-'+field,{field:value},False)
 for n in [32,33]:
  d=copy.deepcopy(base);d['encounters']=[dict(encounter,id='e-'+str(i),x=1000+i*400) for i in range(n)];add('capacity-'+str(n),d,n==32)
 d=copy.deepcopy(base);d['encounters'].append(dict(encounter,x=1400));add('duplicate-id',d,False)
 d['encounters'][1]['id']='other';d['encounters'][1]['x']=1100;add('separation-100',d,False)
 d['encounters'][1]['x']=1300;add('separation-300',d,True)
 d['encounters'][1]['x']=1000;add('exact-collision',d,False)
 d['encounters'][1]['layer']=5;add('other-layer',d,True)
 mixed=copy.deepcopy(base);mixed['encounters']=[]
 for i,v in enumerate([None,'blue','red','black']):
  e=dict(encounter,id='mix-'+str(i),x=1000+i*400,enemy='E_OC' if v=='blue' else 'E_OC2')
  if v:e['bokoblin_variant']=v
  if v=='black':e['hp']=440
  mixed['encounters'].append(e)
 add('mixed',mixed)
 old=dict(document_type='devf_map_encounters',schema_version=1,geometry_sha256='dc53106667811c4cde18b35edb5279357f00ebecaef2f9de546aea9a5a2b06af',encounters=[dict(id='old',enabled=True,enemy='E_OC2',stage='F_SP121',room=3,layer=6,x=-2000,y=-6300,z=59200,yaw_s16=0)])
 add('schema1-legacy',old)
 old['encounters'][0]['bokoblin_variant']='blue';add('schema1-no-extension',old,False)
 for n in [131072,131073]:
  raw=json.dumps(base,separators=(',',':')).encode();add('bytes-'+str(n),raw+b' '*(n-len(raw)),n==131072)
 raw=json.dumps(base,separators=(',',':')).encode();add('duplicate-key',raw.replace(b'"revision":0',b'"revision":0,"revision":0'),False)
 add('partial',raw[:-4],False);add('bom',b'\xef\xbb\xbf'+raw,False)
 (folder/'matrix.tsv').write_text(''.join(name+'\t'+str(int(valid))+'\n' for name,valid in cases))
 assert len({n.casefold() for n,_ in cases})==len(cases),'CASE_INSENSITIVE_FIXTURE_COLLISION'
 print('CP20_MATRIX_GENERATED',len(cases));return mixed
if __name__=='__main__':generate(sys.argv[1])
