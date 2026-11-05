"""A két állapot együtt cserélődik; konfliktus/SD-hiba esetén egyik sem változik."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

binary = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='schema-restore-') as directory:
    root = Path(directory)
    (root / 'state').mkdir()
    def call(command, value=None, revision=0):
        result = subprocess.run([binary, command, str(revision)], cwd=root,
                                input=json.dumps(value) if value is not None else '',
                                capture_output=True, text=True, check=True)
        return json.loads(result.stdout)
    before = call('snapshot')['state']
    replacement = json.loads(json.dumps(before))
    replacement['settings']['wifi'] = {'ssid': 'Restored network', 'password': 'RestoredSecret123'}
    replacement['project']['pages'][0]['name'] = 'Restored page'
    assert call('restore', replacement, 99)['error'] == 'revision.conflict'
    assert call('snapshot')['state'] == before
    failed = call('restore-fail', replacement)
    assert failed['status'] != 0 and failed['error'] == 'storage.write'
    assert failed['after'] == before
    assert call('snapshot')['state'] == before
    bad = json.loads(json.dumps(replacement))
    bad['settings']['wifi']['password'] = 'short'
    assert call('restore', bad)['status'] != 0
    assert call('snapshot')['state'] == before
    result = call('restore', replacement)
    assert result['status'] == 0 and result['revision'] == 1
    assert call('snapshot')['state'] == replacement
    assert call('get')['revision'] == 1
    assert call('restore', before, 0)['error'] == 'revision.conflict'
    assert call('snapshot')['state'] == replacement
    for file in (root / 'state').glob('*'):
        assert b'RestoredSecret123' not in file.read_bytes()
print('Native backup store: atomic project/settings restore, validation, stale revision and encrypted persistence OK')
