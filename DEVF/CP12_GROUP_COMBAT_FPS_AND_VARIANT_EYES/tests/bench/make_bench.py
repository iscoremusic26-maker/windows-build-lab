"""CP11 local bench/parity harness generator (not built by CI). Usage: python3 -I make_bench.py <mod.cpp> <out.cpp>
Inserts the CURRENT mod.cpp regions into bench_template.cpp so old and new versions are measured with identical fakes and inputs."""
import sys
from pathlib import Path
here=Path(__file__).resolve().parent
tpl=(here/"bench_template.cpp").read_text(encoding="utf-8")
mod=Path(sys.argv[1]).read_text(encoding="utf-8").split("\n")
def span(start,end,inclusive=False):
    a=[i for i,l in enumerate(mod) if l.startswith(start)];b=[i for i,l in enumerate(mod) if l.startswith(end)]
    assert len(a)==1 and len(b)==1,(start,end,a,b)
    return "\n".join(mod[a[0]:b[0]+(1 if inclusive else 0)])
glob=span("// CP10.6D: Blue and Black share","constexpr unsigned kCurveSampleTick",True)
res=span("const char* res_name(","int res_slot(",True)
eb0=[i for i,l in enumerate(mod) if l.startswith("// Builds the 7 private variants")] or [i for i,l in enumerate(mod) if l.startswith("// CP10.6A: seven private purple variants")]
assert len(eb0)==1,eb0
eb1=[i for i,l in enumerate(mod) if l.startswith("bool build_eye_set(")];assert len(eb1)==1
eb2=next(i for i in range(eb1[0],len(mod)) if mod[i]=="}")
eyebuild="\n".join(mod[eb0[0]:eb2+1])
vis=span("// Aura lines: one line","// ---- CP10.5 base attack power")
kv=[l for l in mod if l.startswith("constexpr const char* kVersion=")];assert len(kv)==1
lines=tpl.split("\n")
rep={"//@@GLOBALS@@":glob,"//@@RESNAME@@":res,"//@@VISUAL@@":vis,"//@@EYEBUILD@@":eyebuild}
assert all(sum(1 for l in lines if l==k)==1 for k in rep),"each marker exactly once"
out="\n".join(rep.get(l,l) for l in lines)
out=out.replace('constexpr const char* kVersion="0.8.7";',kv[0]).replace('constexpr const char* kVersion="0.8.8";',kv[0])
out=out.replace("@@EXPDIR@@",str(Path(sys.argv[1]).resolve().parent))
out=out.replace("@@DEVFDIR@@",str(Path(sys.argv[1]).resolve().parent.parent.parent))
Path(sys.argv[2]).write_text(out,encoding="utf-8")
