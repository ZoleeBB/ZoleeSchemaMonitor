"""Valódi app_store.c és AES-GCM, csak BSP/NVS/FreeRTOS helyettesítőkkel."""
import json
import os
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

binary = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix="schema-store-") as directory:
    root = Path(directory)
    (root / "state").mkdir()
    def call(command, data=None, revision=0, **env):
        result = subprocess.run([binary, command, str(revision)], cwd=root, env={**os.environ, **env}, input=json.dumps(data) if data is not None else "", text=True, capture_output=True, check=True)
        return json.loads(result.stdout)
    initial = call("get")
    project = initial["project"]
    assert initial["revision"] == 0 and "password" not in initial["settings"]["wifi"]
    assert call("project", project)["status"] == 0
    assert call("project", project)["status"] != 0
    assert call("get")["revision"] == 1
    settings = initial["settings"]
    settings.pop("loxone")
    settings["wifi"] = {"ssid":"TestWiFi", "password":"DoNotLeak-12345"}
    assert call("settings", settings)["status"] == 0
    public = call("get")["settings"]
    assert public["wifi"]["hasPassword"] and "password" not in public["wifi"]
    assert not any(b"DoNotLeak" in p.read_bytes() for p in (root / "state").iterdir())
    settings["wifi"]["password"] = None
    assert call("settings", settings)["status"] == 0
    assert call("get")["settings"]["wifi"]["hasPassword"]
    # Hibás új állapot nem változtatja meg az aktív mentést.
    bad = json.loads(json.dumps(project)); bad["password"] = "forbidden"
    assert call("project", bad, 1)["status"] != 0
    project["title"] = "Second"
    assert call("project", project, 1)["status"] == 0
    assert call("get")["revision"] == 2
    # Ép CRC mellett is hibás lehet az új rekord. Az egyetlen érvényes,
    # régebbi példány a következő mentéskor is érintetlen maradjon.
    latest = max((root / "state").glob("*.bin"), key=lambda p:int.from_bytes(p.read_bytes()[8:12], "little"))
    data = bytearray(latest.read_bytes())
    record = json.loads(data[24:]); record["version"] = 2
    body = json.dumps(record, ensure_ascii=False, separators=(",", ":")).encode()
    data = data[:24] + body
    data[12:16] = len(body).to_bytes(4, "little")
    data[16:20] = zlib.crc32(data[8:16] + body).to_bytes(4, "little")
    latest.write_bytes(data)
    assert call("get")["revision"] == 1
    previous = next(p for p in (root / "state").glob("*.bin") if p != latest)
    previous_bytes = previous.read_bytes()
    assert call("project", project, 1)["status"] == 0
    assert previous.read_bytes() == previous_bytes
    # CRC-sérülés után ugyanez a visszaállás történik.
    data = bytearray(latest.read_bytes()); data[30] ^= 2; latest.write_bytes(data)
    recovered = call("get")
    assert recovered["revision"] == 1 and recovered["recovered"]
    assert recovered["settings"]["wifi"]["hasPassword"]
    # Más NVS-kulccsal egyetlen titkosított állapot sem olvasható vissza.
    (root / "key.bin").write_bytes(bytes(32))
    assert call("get")["revision"] == 0
    # Elérhetetlen SD mellett nincs hamis siker, a RAM-állapot is marad.
    assert call("project", project, SD_FAIL="1")["status"] != 0
print("Native store: persistence, conflict, encryption, rollback, SD failure OK")
