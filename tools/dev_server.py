"""Helyi, kizárólag fejlesztésre szolgáló HTTP API és webszerkesztő.

Nem emulál sikeres firmware-frissítést vagy valódi Loxone-kapcsolatot.
A projektellenőrzés a böngészővel közös JavaScript-modellt használja.
"""

import argparse
import copy
import hashlib
import json
import math
import mimetypes
import re
import subprocess
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote, urlsplit

from asset_store import atomic_write, canonical

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / "web"
MAX_BODY = 1048576
DEFAULT_SETTINGS = {
    "project": {"title": "ZoleeSchemaMonitor", "language": "hu"},
    "display": {"rotation": 270, "dayBrightness": 85, "nightBrightness": 15, "nightSource": None},
    "wifi": {"ssid": "", "password": ""},
    "loxone": {"host": "", "port": 80, "tls": False, "ca_pem": "", "username": "", "password": ""},
}
DEFAULT_PROJECT = {"format": "zolee-schema", "version": 1, "title": "ZoleeSchemaMonitor",
                   "pages": [{"id": "page-1", "name": "Gépészet", "background": "cool", "elements": []}]}
DEMO_UUID = "12345678-1234-5678-0102030405060708"


def validate_project(value):
    raw = canonical(value)
    if len(raw) > MAX_BODY:
        raise ValueError("project.size")
    result = subprocess.run(["node", str(ROOT / "tools/validate_project.mjs")],
                            input=raw, capture_output=True, timeout=5, check=False)
    if result.returncode:
        raise ValueError(result.stderr.decode()[:500])
    return json.loads(result.stdout)


def settings_public(settings):
    result = copy.deepcopy(settings)
    for key in ["wifi", "loxone"]:
        result[key]["hasPassword"] = bool(result[key].pop("password", ""))
    return result


def settings_update(saved, request):
    if not isinstance(request, dict) or set(request) != {"project", "display", "wifi"}:
        raise ValueError("settings.fields")
    if set(request["project"]) != {"title", "language"} or set(request["display"]) != {"rotation", "dayBrightness", "nightBrightness", "nightSource"}:
        raise ValueError("settings.fields")
    if not isinstance(request["project"]["title"], str) or len(request["project"]["title"]) > 80:
        raise ValueError("project.title")
    if request["project"]["language"] not in {"hu", "en"}:
        raise ValueError("project.language")
    display = request["display"]
    if type(display["rotation"]) is not int or display["rotation"] not in {0, 90, 180, 270}:
        raise ValueError("display.rotation")
    for key, minimum in [("dayBrightness", 1), ("nightBrightness", 0)]:
        if type(display[key]) is not int or not minimum <= display[key] <= 100:
            raise ValueError(key)
    binding = display["nightSource"]
    if binding is not None:
        if not isinstance(binding, dict) or set(binding) != {"uuid", "scale", "offset"}:
            raise ValueError("display.nightSource")
        if not isinstance(binding["uuid"], str) or not re.fullmatch(r"[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{16}", binding["uuid"], re.I):
            raise ValueError("display.nightSource.uuid")
        for key in ["scale", "offset"]:
            if type(binding[key]) not in {float, int} or not math.isfinite(binding[key]) or abs(binding[key]) > 1000000:
                raise ValueError("display.nightSource." + key)
    wifi = request["wifi"]
    if set(wifi) != {"ssid", "password"} or not isinstance(wifi["ssid"], str) or len(wifi["ssid"].encode()) > 32:
        raise ValueError("wifi.ssid")
    if wifi["password"] is not None and (not isinstance(wifi["password"], str) or len(wifi["password"].encode()) > 64):
        raise ValueError("wifi.password")
    result = copy.deepcopy(saved)
    result.update(copy.deepcopy(request))
    if wifi["password"] is None:
        result["wifi"]["password"] = saved["wifi"]["password"]
    return result


class State:
    def __init__(self, path):
        self.path = Path(path)
        self.lock = threading.Lock()
        self.started = time.monotonic()
        self.settings = copy.deepcopy(DEFAULT_SETTINGS)
        self.project = copy.deepcopy(DEFAULT_PROJECT)
        self.revision = 0
        if self.path.exists():
            data = json.loads(self.path.read_text())
            self.project = validate_project(data["project"])
            self.settings = data["settings"]
            self.revision = data["revision"]

    def persist(self, *, project=None, settings=None, revision=None):
        candidate = {"project": project if project is not None else self.project,
                     "settings": settings if settings is not None else self.settings,
                     "revision": revision if revision is not None else self.revision}
        atomic_write(self.path, canonical(candidate))
        self.project, self.settings, self.revision = candidate["project"], candidate["settings"], candidate["revision"]


def handler_for(state, demo):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, format, *args):
            # A napló URL-lel és hibakóddal dolgozik, kérés törzsét nem írja ki.
            super().log_message(format, *args)

        def respond(self, value, status=200):
            raw = canonical(value)
            self.send_response(status)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(raw)))
            self.send_header("Cache-Control", "no-store")
            self.send_header("X-Content-Type-Options", "nosniff")
            self.end_headers()
            self.wfile.write(raw)

        def do_GET(self):
            path = urlsplit(self.path).path
            with state.lock:
                if path == "/api/settings":
                    return self.respond(settings_public(state.settings))
                if path == "/api/project":
                    return self.respond({"project": state.project, "revision": state.revision})
                if path == "/api/project/export":
                    return self.respond(state.project)
            if path == "/api/device":
                return self.respond({"development": True, "firmware": "0.1.0-ms1-dev", "ip": "127.0.0.1",
                                     "wifi": {"rssi": None}, "connected": demo, "uptime": int(time.monotonic() - state.started),
                                     "revolut": "https://revolut.me/zoleebb", "events": []})
            if path == "/api/backup/status":
                return self.respond({"available": False, "revision": state.revision, "development": True})
            if path == "/api/diagnostics":
                return self.respond({"mode":"development", "measured":False, "cpuValid":False,
                                     "firmware":"0.1.0-ms1-dev", "uptime":None, "cpu0":None, "cpu1":None,
                                     "internalFree":None, "internalTotal":None, "internalMinimum":None,
                                     "psramFree":None, "psramTotal":None, "dmaFree":None, "dmaLargest":None,
                                     "sdMounted":None, "sdSpaceValid":False, "sdTotal":None, "sdFree":None,
                                     "mainStackFree":None, "taskCount":None, "sampleAge":None, "displayFps":None,
                                     "wifi":{}, "loxone":{}, "update":{}, "events":[]})
            if path == "/api/live":
                value = 50 + math.sin((time.monotonic() - state.started) / 5) * 45
                return self.respond({"connected": demo, "values": {DEMO_UUID: value} if demo else {},
                                     "status": "Szimulált adat / Simulated data" if demo else "Nincs valódi Loxone kapcsolat / No real Loxone connection"})
            if path == "/api/loxone/catalog":
                sources = [{"uuid": DEMO_UUID, "name": "Teszt szivattyú / Demo pump", "room": "Gépészet", "category": "Fűtés", "state": "value"}] if demo else []
                return self.respond({"catalog": sources})
            if path == "/api/graphics":
                return self.respond(json.loads((WEB / "graphics/manifest.json").read_text()))
            if path.startswith("/api/"):
                return self.respond({"error": "Not found"}, 404)
            if path in {"/", "/editor", "/settings", "/loxone", "/graphics", "/backup", "/update", "/diagnostics"}:
                file = WEB / "index.html"
            else:
                file = (WEB / unquote(path).lstrip("/")).resolve()
                if WEB.resolve() not in file.parents:
                    return self.respond({"error": "Not found"}, 404)
            if not file.is_file():
                return self.respond({"error": "Not found"}, 404)
            raw = file.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", mimetypes.guess_type(file.name)[0] or "application/octet-stream")
            self.send_header("Content-Length", str(len(raw)))
            self.send_header("X-Content-Type-Options", "nosniff")
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()
            self.wfile.write(raw)

        def do_POST(self):
            try:
                origin = self.headers.get("Origin")
                if origin and urlsplit(origin).netloc != self.headers.get("Host"):
                    return self.respond({"error": "Origin rejected"}, 403)
                if self.headers.get("X-Zolee-Schema") != "1":
                    return self.respond({"error": "Missing request header"}, 403)
                length = int(self.headers.get("Content-Length", "0"))
                if not 0 < length <= MAX_BODY:
                    return self.respond({"error": "Request too large or empty"}, 413)
                if not self.headers.get("Content-Type", "").startswith("application/json"):
                    return self.respond({"error": "JSON required"}, 415)
                data = json.loads(self.rfile.read(length), parse_constant=lambda value: (_ for _ in ()).throw(ValueError(value)))
                path = urlsplit(self.path).path
                with state.lock:
                    if path == "/api/project":
                        if not isinstance(data, dict) or set(data) != {"project", "revision"} or type(data["revision"]) is not int:
                            raise ValueError("project.request")
                        if data["revision"] != state.revision:
                            return self.respond({"error": "revision conflict"}, 409)
                        project = validate_project(data["project"])
                        state.persist(project=project, revision=state.revision + 1)
                        return self.respond({"revision": state.revision})
                    if path == "/api/settings":
                        result = settings_update(state.settings, data)
                        state.persist(settings=result)
                        return self.respond(settings_public(result))
                    if path == "/api/loxone":
                        return self.respond({"error": "A fejlesztői kiszolgáló nem csatlakozik Miniserverhez / Development server does not connect to a Miniserver"}, 501)
                    if path == "/api/graphics/refresh":
                        return self.respond({"message": "Helyi fejlesztői készlet. OTA-integráció készül / Local development library. OTA integration in progress."})
                return self.respond({"error": "Not implemented in development preview"}, 501)
            except (ValueError, KeyError, TypeError, subprocess.TimeoutExpired) as error:
                self.respond({"error": str(error)[:500]}, 400)
            except OSError:
                self.respond({"error": "A tartós mentés sikertelen / Durable save failed"}, 500)

    return Handler


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--data", default=str(ROOT / ".dev-data/state.json"))
    parser.add_argument("--demo", action="store_true")
    args = parser.parse_args()
    if not (WEB / "graphics/manifest.json").exists():
        subprocess.run([sys.executable, str(ROOT / "tools/build_assets.py")], check=True)
    server = ThreadingHTTPServer((args.host, args.port), handler_for(State(args.data), args.demo))
    print(f"ZoleeSchemaMonitor fejlesztői előnézet: http://{args.host}:{server.server_port}", flush=True)
    server.serve_forever()
