"""Strict, narrow B2 decision fixture. No raw log or arbitrary strings exported."""
import collections,hashlib,json,pathlib,re,sys
p=pathlib.Path(sys.argv[1]);out=pathlib.Path(sys.argv[2]);b=p.read_bytes()
assert (len(b),len(b.splitlines()),hashlib.sha256(b).hexdigest())==(801100,3936,'7d103dc70a196f900d4412c4a1dd47d16b57767d6cc5a1597df73aaa44ac1f1d')
fields={'id','generation','event','family','decision','reason','pre_mode','pre_state','post_mode','post_state','cd','posture','action','resource','atp','admission'}
rows=[]
for l in b.decode().splitlines():
 if '| dev.posture.post05_selective_reactions] POST05_DECISION ' not in l:continue
 body=l.split('POST05_DECISION ',1)[1];d=dict(re.findall(r'(\w+)=(\S+)',body));assert set(d)==fields
 assert all(re.fullmatch(r'(?:-?\d+(?:>-?\d+)?|[A-Z][A-Z0-9_]*)',v) for v in d.values())
 rows.append(d)
assert len(rows)==94 and len({d['event'] for d in rows})==94
applied=[int(d['event']) for d in rows if d['action']=='SUPPRESSION_APPLIED'];e2=[d for d in rows if d['resource']=='E_OC2'];assert applied==[5,6,42,43,44,69] and len(e2)==9
assert all(d['action']=='NONE' for d in e2)
summary={'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b),'splitlines':len(b.splitlines()),'confirmed':len(rows),'applied_events':applied,'eoc2_events':[int(d['event']) for d in e2],'reasons':dict(collections.Counter(d['reason'] for d in rows)),'new_shadow_replay':'ANIMATION_AND_PROFILE_NOT_RECORDED: no new eligibility can be inferred','label':'R_BASELINE_ONLY'}
out.mkdir(parents=True,exist_ok=True);(out/'B2_DECISIONS_SANITIZED.json').write_text(json.dumps(rows,indent=2)+'\n');(out/'B2_SUMMARY_SANITIZED.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary,indent=2))
