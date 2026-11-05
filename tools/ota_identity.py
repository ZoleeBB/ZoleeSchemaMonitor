"""A grafikai források lenyomata a firmware saját OTA-leírójában."""
import hashlib
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
def identity():
    digest = hashlib.sha256()
    for name in ['catalog/elements.py', 'tools/build_assets.py']:
        digest.update(name.encode() + b'\0' + (ROOT / name).read_bytes() + b'\0')
    return 'ZSM-ASSETS-1:' + digest.hexdigest()
if __name__ == '__main__':
    Path(sys.argv[1]).write_text('#define SCHEMA_OTA_IDENTITY "' + identity() + '"\n')
