"""The vendored Expansion identity policy is byte for byte the 0.17.0 source (see tests/vendor/bokoblin_expansion_0_17_0/PROVENANCE.md)."""
import hashlib, json, sys
from pathlib import Path
f = Path(sys.argv[1]) / 'tests/vendor/bokoblin_expansion_0_17_0/variant_policy.hpp'
want = 'c27a33e15ecc68c636d92a008d9f7b04026ccad2a062ceedce81c886fc790642'
got = hashlib.sha256(f.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
print(json.dumps({'status': 'VENDOR_PASS' if got == want else 'VENDOR_FAIL', 'sha256': got, 'source_commit': 'fe47c8b828939829914d466a425cb1e2c0223cd9'}))
sys.exit(0 if got == want else 1)
