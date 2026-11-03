"""FIX02 regression entry, revised for FIX03 owner-first specification.
The historical fixture used Screen type17 and expected unowned direct draws
suppressed by geometry. Those two assumptions reproduce the human regression.
The shared replacement tests the exact base callbacks and current callbacks on
one native-type8 scene, keeps the full22 codes and direct emission/restore cases.
"""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).with_name('test_hudfix03_adapter.py')),run_name='__main__')
