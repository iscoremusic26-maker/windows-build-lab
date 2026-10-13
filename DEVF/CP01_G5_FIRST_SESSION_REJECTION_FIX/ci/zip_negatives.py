import importlib.util
from pathlib import Path
from audit import safe_members
f=Path(__file__).resolve().parents[2]/'CP01_G4B_PACKAGE_DELIVERY/ci/verify_negative_cases.py'
spec=importlib.util.spec_from_file_location('old_zip_negatives',f);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
def run():return m.run()
