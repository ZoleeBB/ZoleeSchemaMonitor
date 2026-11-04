"""A webalkalmazást tömörítve a firmware-be ágyazza; az SD-n csak grafikák vannak."""
import gzip
import hashlib
import json
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
MIME = {'.html':'text/html; charset=utf-8', '.js':'text/javascript; charset=utf-8', '.css':'text/css; charset=utf-8', '.svg':'image/svg+xml', '.json':'application/json; charset=utf-8'}
lines = ['/* Generált: tools/embed_web.py. */']
entries = []
for path in sorted((ROOT / 'web').iterdir()):
    if path.suffix not in MIME or not path.is_file(): continue
    data = gzip.compress(path.read_bytes(), mtime=0)
    name = 'web_' + hashlib.sha256(path.name.encode()).hexdigest()[:12]
    lines.append(f'static const unsigned char {name}[] = {{' + ','.join(str(b) for b in data) + '};')
    entries.append('{%s,%s,%s,sizeof(%s)}' % (json.dumps('/' + path.name), json.dumps(MIME[path.suffix]), name, name))
lines += ['static const web_file_t s_web_files[] = {' + ','.join(entries) + '};', '']
Path(sys.argv[1]).write_text('\n'.join(lines))
