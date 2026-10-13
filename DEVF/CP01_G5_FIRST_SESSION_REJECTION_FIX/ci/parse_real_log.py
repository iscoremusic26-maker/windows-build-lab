"""Sanitize only whitelisted scalar event fields. Never copy source log, pointers or paths."""
import argparse,hashlib,json,re,collections
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--private-file',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();b=a.private_file.read_bytes();lines=b.decode('utf-8').splitlines()
allowed={'event','state','reason','policy_reason','event_seq','update_index','capture_sequence','attempt_id','request_id','node','child','previous','room','measurement','link_id','link_profile','link_room','link_owner','link_xz','link_d3','link_dy','offender_id','offender_profile','offender_room','offender_owner','offender_xz','suppressed','sink_failures'}
events=[]
for n,line in enumerate(lines,1):
 if 'DEVF_PILOT event=' in line:
  fields={k:v for k,v in re.findall(r'(\w+)=([^\s]+)',line) if k in allowed}
  if any('/' in v or '\\' in v or '0x' in v for v in fields.values()):raise RuntimeError('Unexpected unsafe scalar')
  events.append({'line':n,**fields})
expected='06fd8f7e5a3124dd44fefe78d0b9ec11e327b6fc0236e56b900e21ff774400e0'
result={'scope':'USER_REAL_SESSION_G5; never synthetic lab','received_bytes':len(b),'received_sha256':hashlib.sha256(b).hexdigest(),'line_count':len(lines),'claimed_original_bytes':186833,'claimed_original_sha256':expected,'in_memory_leading_bracket_reconstruction_matches_original':hashlib.sha256(b'['+b).hexdigest()==expected,'original_or_received_log_rewritten_or_published':False,'events':events,'event_counts':dict(collections.Counter(e['event'] for e in events)),'policy_failures':dict(collections.Counter(e['policy_reason'] for e in events if e['event']=='LOCATION_POLICY_FAIL')),'all_attempts_zero':all(e['attempt_id']=='0' for e in events),'all_request_IDs_unknown':all(e['request_id']=='4294967295' for e in events),'speeds_recorded':False,'native_E_OC_creations_in_host_log':sum('fpcBs_Create: pid=' in l and 'profname=fpcNm_E_OC_e (510)' in l for l in lines),'native_DEVF_request_observed':False,'runtime_create_render_collision':'NOT_TESTED','next_G5_authorized':False}
if len(events)!=27 or len(lines)!=2182 or not result['in_memory_leading_bracket_reconstruction_matches_original']:raise RuntimeError('Evidence does not match handoff')
a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS sanitized27 events,2182lines,exact bracket-only reconstruction SHA; log stays private')
