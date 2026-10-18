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
vis=span("// Aura lines: one line","// ---- CP10.5 base attack power")
kv=[l for l in mod if l.startswith("constexpr const char* kVersion=")];assert len(kv)==1
lines=tpl.split("\n")
rep={"//@@GLOBALS@@":glob,"//@@RESNAME@@":res,"//@@VISUAL@@":vis}
assert all(sum(1 for l in lines if l==k)==1 for k in rep),"each marker exactly once"
out="\n".join(rep.get(l,l) for l in lines)
out=out.replace('constexpr const char* kVersion="0.8.7";',kv[0]).replace('constexpr const char* kVersion="0.8.8";',kv[0])
Path(sys.argv[2]).write_text(out,encoding="utf-8")
