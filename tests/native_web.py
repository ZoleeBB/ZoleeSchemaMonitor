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
    def call(path, body=None, override=None):
        request = {"path":path, "method":"GET" if body is None else "POST", "headers":override if override is not None else headers, "body":json.dumps(body) if body is not None else ""}
        result = subprocess.run([binary], cwd=root, input=json.dumps(request), capture_output=True, text=True, check=True)
        response = json.loads(result.stdout)
        data = bytes.fromhex(response.pop("bodyHex"))
        if response["headers"].get("Content-Encoding") == "gzip": data = gzip.decompress(data)
        response["body"] = data
        return response
    def decoded(response): return json.loads(response["body"])
    assert call("/editor")["status"] == "200 OK"
    assert b"new Editor" in call("/app.js")["body"]
    assert len(decoded(call("/catalog.json"))["elements"]) == 138
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
    settings["wifi"]["password"] = "short"
    assert call("/api/settings", settings)["status"].startswith("400")
    assert call("/api/update", {})["status"].startswith("501")
    assert call("/graphics/light/../../schema-state/state-1.bin")["status"].startswith("404")
    assert call("/nonexistent")["status"].startswith("404")
    capabilities = decoded(call("/api/device"))["capabilities"]
    assert capabilities["ota"] is False and capabilities["loxone"] is False
print("Native HTTP: embedded editor, import/export, CAS, secret filtering, Host/Origin, disabled OTA OK")
