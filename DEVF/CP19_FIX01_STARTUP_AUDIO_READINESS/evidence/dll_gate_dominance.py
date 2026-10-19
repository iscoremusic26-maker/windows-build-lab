# CP19 FIX01 DLL gate verifier: on `objdump -d -M intel --no-show-raw-insn mod.dll`, every call to the wave thunks and every read of the
# Z2AudioMgr::mAudioMgrPtr IAT slot must sit in a function that calls flags() and be dominated (CFG of that function) by the OK branches of
# all eight AudioFlags bytes, either directly or through a register that is zero only after all eight passed (`test R,R ; je ready`).
import re,sys,json
B=0x180000000
ins=[]
for l in open(sys.argv[1]).read().splitlines():
    m=re.match(r'\s+([0-9a-f]+):\s+(.*)',l)
    if m:ins.append((int(m.group(1),16),m.group(2).strip()))
addr=[a for a,_ in ins];idx={a:i for i,(a,_) in enumerate(ins)}
# functions = maximal runs between int3 padding
funcs=[];cur=[]
for a,t in ins:
    if t.startswith('int3'):
        if cur:funcs.append(cur);cur=[]
    else:cur.append((a,t))
if cur:funcs.append(cur)
# argv: asm erase_thunk status_thunk load_thunk flags_fn mgr_iat (VAs, base 0x180000000; flags_fn 0 if the DLL has no flags())
H=[int(x,16) for x in sys.argv[2:7]]
THUNK={H[0]:'eraseSeWave',H[1]:'getWaveLoadStatus',H[2]:'loadSeWave'};FLAGS=H[3];MGR_IAT=H[4]
res=[]
for f in funcs:
    calls=[(a,THUNK[int(m.group(1),16)]) for a,t in f for m in [re.match(r'call\s+0x([0-9a-f]+)$',t)] if m and int(m.group(1),16) in THUNK]
    # also every read of the Z2AudioMgr::mAudioMgrPtr IAT slot (wave/BGM/enemy-list access goes through it), except inside flags() itself
    if f[0][0]!=FLAGS:calls+=[(a,'mAudioMgrPtr_read') for a,t in f if t.endswith('# 0x%x'%MGR_IAT)]
    if not calls:continue
    fa={a for a,_ in f};lead={f[0][0]};succ={}
    for i,(a,t) in enumerate(f):
        m=re.match(r'(j\w+)\s+0x([0-9a-f]+)$',t)
        if m:
            tg=int(m.group(2),16)
            if tg in fa:lead.add(tg)
            if i+1<len(f):lead.add(f[i+1][0])
        elif t.startswith('ret') and i+1<len(f):lead.add(f[i+1][0])
    blocks=[];b=None
    for a,t in f:
        if a in lead:b=[a,[]];blocks.append(b)
        b[1].append((a,t))
    start={bl[0]:bl for bl in blocks};order=[bl[0] for bl in blocks]
    for k,bl in enumerate(blocks):
        a,t=bl[1][-1];s=[]
        m=re.match(r'(j\w+)\s+0x([0-9a-f]+)$',t)
        if m:
            tg=int(m.group(2),16)
            if tg in fa:s.append(tg)
            if m.group(1)!='jmp' and k+1<len(blocks):s.append(blocks[k+1][0])
        elif not t.startswith('ret') and k+1<len(blocks):s.append(blocks[k+1][0])
        succ[bl[0]]=s
    pred={x:[] for x in order}
    for x,ss in succ.items():
        for y in ss:pred[y].append(x)
    dom={x:set(order) for x in order};dom[order[0]]={order[0]}
    ch=True
    while ch:
        ch=False
        for x in order[1:]:
            ps=[dom[p] for p in pred[x]]
            nd=(set.intersection(*ps) if ps else set())|{x}
            if nd!=dom[x]:dom[x]=nd;ch=True
    # Semantic gate check: flags() returns AudioFlags {manager,init,heap,reset_flag,resetting,reset_data,shutdown,return_to_menu} (bytes 0..7).
    MUST={0:True,1:True,2:True,3:False,4:False,5:True,6:False,7:False}
    okblocks={k:set() for k in MUST}
    for k,bl in enumerate(blocks):
        ts=bl[1]
        if len(ts)<2:continue
        m=re.match(r'cmp\s+BYTE PTR \[(rax|rdx)(?:\+0x([0-7]))?\],(0x0|\w+)$',ts[-2][1]);n=re.match(r'(je|jne)\s+0x([0-9a-f]+)$',ts[-1][1])
        if not m or not n:continue
        K=int(m.group(2) or '0',16);tg=int(n.group(2),16);ft=blocks[k+1][0] if k+1<len(blocks) else None
        nonzero_side=tg if n.group(1)=='jne' else ft;zero_side=ft if n.group(1)=='jne' else tg
        okblocks[K].add(nonzero_side if MUST[K] else zero_side)
    has_flags=any(re.match(r'call\s+0x%x$'%FLAGS,t) for _,t in f)
    # Materialised gate: a byte register assigned 0 only in a block dominated by all eight OK branches, nonzero constants elsewhere; then
    # `test R,R ; je X` (X ready) or `jne X` (fallthrough ready). The register must not be written otherwise in the function.
    def blk0(a):
        for bl in blocks:
            if bl[1][0][0]<=a<=bl[1][-1][0]:return bl[0]
    mat=set()
    for R,W in (('dil','rdi|edi|di|dil'),('sil','rsi|esi|si|sil'),('bl','rbx|ebx|bx|bl')):
        assigns=[(a,tt) for a,tt in f if re.match(r'(mov|xor)\s+%s,'%R,tt)]
        zeros=[a for a,tt in assigns if re.match(r'xor\s+%s,%s$'%(R,R),tt) or re.match(r'mov\s+%s,0x0$'%R,tt)]
        nonzero=[a for a,tt in assigns if re.match(r'mov\s+%s,0x[1-9a-f][0-9a-f]*$'%R,tt)]
        tests=[a for a,tt in f if tt=='test   %s,%s'%(R,R)]
        if not zeros or not tests:continue
        lo,hi=min(a for a,_ in assigns),max(tests)
        # In the window [first assignment, last test] the register is written only by those constants (x64 calls preserve rdi/rsi/rbx).
        other=[a for a,tt in f if lo<=a<=hi and re.match(r'\w+\s+(%s)\b'%W,tt) and not tt.startswith(('push','pop','test','cmp')) and (a,tt) not in assigns]
        if other or len(zeros)+len(nonzero)!=len(assigns):continue
        if not all(all(dom[blk0(z)]&okblocks[K] for K in MUST) for z in zeros):continue
        for k,bl in enumerate(blocks):
            ts=bl[1]
            if len(ts)>=2 and ts[-2][1]=='test   %s,%s'%(R,R):
                m=re.match(r'(je|jne)\s+0x([0-9a-f]+)$',ts[-1][1])
                if m:mat.add(int(m.group(2),16) if m.group(1)=='je' else blocks[k+1][0])
    def blk(a):
        for bl in blocks:
            if bl[1][0][0]<=a<=bl[1][-1][0]:return bl[0]
    for a,name in calls:
        b=blk(a);per={K:sorted(hex(x-B) for x in dom[b]&okblocks[K]) for K in MUST};via=sorted(hex(x-B) for x in dom[b]&mat)
        res.append(dict(call_rva=hex(a-B),target=name,function_rva=hex(f[0][0]-B),function_calls_flags=has_flags,direct_gate_checks={str(K):v for K,v in per.items()},materialised_gate_ready_block=via,all_eight_checks=all(per[K] for K in MUST) or bool(via)))
print(json.dumps(res,indent=1))
ok=bool(res) and all(r['function_calls_flags'] and r['all_eight_checks'] for r in res)
print('ALL_AUDIO_CALLS_DOMINATED_BY_GATE_READY_BRANCH' if ok else 'GATING_NOT_PROVEN');sys.exit(0 if ok else 1)
