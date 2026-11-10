"""A grafikai források lenyomata a firmware saját OTA-leírójában."""
import hashlib
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
def identity():
    digest = hashlib.sha256()
    sources = ['catalog/elements.py', 'catalog/artwork-bindings.json', 'catalog/port-calibration.json','catalog/boiler-brands.json', 'tools/build_assets.py', 'tools/requirements-assets.txt']
    sources += sorted(path.relative_to(ROOT).as_posix() for path in (ROOT / 'assets/elements').rglob('*.png'))
    for name in sources:
        digest.update(name.encode() + b'\0' + (ROOT / name).read_bytes() + b'\0')
    return 'ZSM-ASSETS-1:' + digest.hexdigest()
if __name__ == '__main__':
    Path(sys.argv[1]).write_text('#define SCHEMA_OTA_IDENTITY "' + identity() + '"\n')
