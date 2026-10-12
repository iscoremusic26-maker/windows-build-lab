"""Packaging is conditional on the exact reviewed own code and official source, not only a build PASS."""
import hashlib
import json
from pathlib import Path

def verify_review(project, source):
    review=json.loads((project/'evidence/pre-package-review.json').read_text(encoding='utf-8'))
    if review['decision']!='PASS_STATIC_FOR_PACKAGING_REVIEW' or review['unresolved_critical_risks']:
        raise RuntimeError('Safety audit has not cleared packaging')
    for item in review['own_reviewed_files']:
        raw=(project/item['path']).read_bytes().replace(b'\r\n',b'\n')
        if hashlib.sha256(raw).hexdigest()!=item['lf_sha256']:
            raise RuntimeError(f"Own file changed after review: {item['path']}")
    for item in review['official_references']:
        raw=(source/item['path']).read_bytes().replace(b'\r\n',b'\n')
        if hashlib.sha256(raw).hexdigest()!=item['lf_sha256']:
            raise RuntimeError(f"Official safety reference changed: {item['path']}")
    return {'reviewed_code_and_source_match':True,'decision':review['decision'],
            'runtime_safety':'NOT_VERIFIED','review_lf_sha256':hashlib.sha256(
                (project/'evidence/pre-package-review.json').read_bytes().replace(b'\r\n',b'\n')).hexdigest()}
