"""A készülék HTTP-kezelőjének tesztje valódi tárolással, sockethelyettesítővel."""
import gzip
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

binary = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix="schema-web-") as directory:
    root = Path(directory); (root / "state").mkdir()
    headers = {"Host":"192.168.2.123", "Origin":"http://192.168.2.123", "Content-Type":"application/json", "X-Zolee-Schema":"1"}
    def call(path, body=None, override=None, **env):
        request = {"path":path, "method":"GET" if body is None else "POST", "headers":override if override is not None else headers, "body":json.dumps(body) if body is not None else ""}
        result = subprocess.run([binary], cwd=root, env={**os.environ, **env}, input=json.dumps(request), capture_output=True, text=True, check=True)
        response = json.loads(result.stdout)
        data = bytes.fromhex(response.pop("bodyHex"))
        if response["headers"].get("Content-Encoding") == "gzip": data = gzip.decompress(data)
        response["body"] = data
        return response
    def decoded(response): return json.loads(response["body"])
    (root / "graphics/light").mkdir(parents=True)
    for extension, data, mime in [("png", b"\x89PNG\r\n\x1a\nfixture", "image/png"),
                                  ("svg", b'<svg xmlns="http://www.w3.org/2000/svg"/>', "image/svg+xml")]:
        (root / f"graphics/light/circulation-pump.{extension}").write_bytes(data)
        image = call(f"/graphics/light/circulation-pump.{extension}")
        assert image["status"] == "200 OK" and image["body"] == data
        assert image["headers"]["Content-Type"] == mime
    for invalid in ["circulation-pump.jpg", "unknown.png", "../circulation-pump.png"]:
        assert call("/graphics/light/" + invalid)["status"].startswith("404")
    assert call("/api/storage/remount", {})["status"].startswith("400")
    assert call("/api/storage/remount", {"path":"other"}, SD_FAIL="1")["status"].startswith("400")
    assert call("/api/storage/remount", {}, SD_FAIL="1", BACKUP_BUSY="1")["status"].startswith("423")
    assert call("/api/storage/remount", {}, {**headers, "Origin":"http://evil.example"}, SD_FAIL="1")["status"].startswith("403")
    retry = call("/api/storage/remount", {}, SD_FAIL="1")
    assert retry["status"] == "202 Accepted" and retry["restart"]
    assert call("/editor")["status"] == "200 OK"
    assert b"new Editor" in call("/app.js")["body"]
    for resource, marker in [("/common.css", b"box-sizing"), ("/schema.css", b".editor-layout")]:
        css = call(resource)
        assert css["status"] == "200 OK" and marker in css["body"]
        assert css["headers"]["Content-Type"].startswith("text/css")
    assert len(decoded(call("/catalog.json"))["elements"]) >= 300
    assert call("/api/settings", override={"Host":"evil.example"})["status"].startswith("403")
    project = decoded(call("/api/project"))["project"]
    payload = {"project":project, "revision":0}
    for changes in [{"Origin":"http://evil.example"}, {"X-Zolee-Schema":""}, {"Content-Type":"text/plain"}]:
        assert call("/api/project", payload, {**headers, **changes})["status"].startswith("403")
    assert decoded(call("/api/project", payload))["revision"] == 1
    assert call("/api/project", payload)["status"].startswith("409")
    export = call("/api/project/export")
    assert "attachment" in export["headers"]["Content-Disposition"]
    assert "password" not in export["body"].decode()
    settings = decoded(call("/api/settings")); settings.pop("loxone")
    settings["wifi"] = {"ssid":"Test", "password":"12345678"}
    saved = call("/api/settings", settings)
    assert saved["status"] == "200 OK" and saved["restart"]
    assert decoded(saved)["wifi"]["hasPassword"]
    assert "password" not in decoded(saved)["wifi"]
    # A hordozható mentés az eredeti készülék NVS-kulcsa nélkül is működik.
    password = "Backup password 123!"
    assert call("/api/backup/export", {"passphrase": password}, BACKUP_BUSY="1")["status"].startswith("423")
    envelope = decoded(call("/api/backup/export", {"passphrase": password}))
    assert envelope["format"] == "zolee-backup" and envelope["iterations"] == 600000
    assert "12345678" not in json.dumps(envelope) and password not in json.dumps(envelope)
    assert call("/api/backup/export", {"passphrase":"short"})["status"].startswith("400")
    assert call("/api/backup/export", {"passphrase":password}, {**headers,"Origin":"http://evil.example"})["status"].startswith("403")
    assert decoded(call("/api/backup/status"))["revision"] == 1
    (root / "state").rename(root / "source-state")
    (root / "state").mkdir()
    (root / "key.bin").rename(root / "source-key.bin")
    assert decoded(call("/api/backup/status"))["revision"] == 0
    assert (root / "key.bin").read_bytes() != (root / "source-key.bin").read_bytes()
    payload = {"backup":envelope, "passphrase":"Wrong password 123!", "revision":0}
    rejected = call("/api/backup/restore", payload)
    assert rejected["status"].startswith("400") and not rejected["restart"]
    assert decoded(call("/api/project"))["revision"] == 0
    payload["passphrase"] = password
    payload["revision"] = 50
    rejected = call("/api/backup/restore", payload)
    assert rejected["status"].startswith("409") and not rejected["restart"]
    payload["revision"] = 0
    restored = call("/api/backup/restore", payload)
    assert restored["status"] == "200 OK" and restored["restart"]
    assert decoded(restored)["revision"] == 1
    assert decoded(call("/api/project"))["project"] == project
    target_settings = decoded(call("/api/settings"))
    assert target_settings["wifi"]["ssid"] == "Test" and target_settings["wifi"]["hasPassword"]
    assert "12345678" not in json.dumps(target_settings)
    for file in (root / "state").glob("*"):
        assert b"12345678" not in file.read_bytes()
    settings["wifi"]["password"] = "short"
    assert call("/api/settings", settings)["status"].startswith("400")
    assert call("/api/update", {})["status"].startswith("403")
    rejected = call("/api/update", {}, {**headers, "Content-Type":"application/octet-stream"})
    assert rejected["status"].startswith("400") and rejected["result"] != 0 and not rejected["restart"]
    assert call("/api/update", {}, {**headers, "Content-Type":"application/octet-stream", "Origin":"http://evil.example"})["status"].startswith("403")
    assert call("/graphics/light/../../schema-state/state-1.bin")["status"].startswith("404")
    assert call("/nonexistent")["status"].startswith("404")
    diagnostics = decoded(call("/api/diagnostics"))
    assert diagnostics["cpu0"] is None and diagnostics["internalFree"] is None
    assert diagnostics["displayFps"] is None and isinstance(diagnostics["events"], list)
    assert "password" not in json.dumps(diagnostics)
    assert call("/api/graphics/refresh", {})["status"].startswith("202")
    assert call("/api/update/release", {})["status"].startswith("202")
    assert call("/api/update/release", {"url":"https://evil.example"})["status"].startswith("400")
    assert call("/api/update/release", {}, {**headers, "Origin":"http://evil.example"})["status"].startswith("403")
    assert decoded(call("/api/update/status"))["phase"] == "release"
    capabilities = decoded(call("/api/device"))["capabilities"]
    assert capabilities["ota"] is True and capabilities["loxone"] is True
print("Native HTTP: embedded editor, import/export, CAS, secret filtering, Host/Origin, OTA boundary OK")
