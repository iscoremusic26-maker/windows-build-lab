"""LIZALFOS synthetic cost of the brain callbacks per simulated Execute for 0/1/4/8/16 actors in the transcribed native model (same scenario with the brain removed as the reference).
NOT an FPS or game measurement. Usage: bench_liz.py <POST05 tree> [ticks]"""
from pathlib import Path
import os, subprocess, sys, tempfile
tree = Path(sys.argv[1]).resolve()
ticks = sys.argv[2] if len(sys.argv) > 2 else '200000'
with tempfile.TemporaryDirectory() as td:
    p = Path(td)
    exe = p / ('bench.exe' if os.name == 'nt' else 'bench')
    src = tree / 'tests/liz_lab.cpp'
    cmd = ['cl', '/nologo', '/std:c++20', '/EHsc', '/utf-8', '/O2', '/bigobj', str(src), '/Fe' + str(exe), '/Fo' + str(p / 'bench.obj')] if os.name == 'nt' else ['g++', '-std=c++20', '-O2', '-o', str(exe), str(src)]
    c = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    if c.returncode:
        print(c.stdout[-3000:] + c.stderr[-3000:])
        sys.exit(c.returncode)
    c = subprocess.run([str(exe), '--bench', ticks], capture_output=True, text=True, timeout=600)
    print(c.stdout + c.stderr[-2000:])
    sys.exit(c.returncode)
