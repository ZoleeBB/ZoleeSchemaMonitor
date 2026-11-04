import copy
import json
import sys
import tempfile
import threading
import unittest
import urllib.error
import urllib.request
from http.server import ThreadingHTTPServer
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from dev_server import DEFAULT_PROJECT, DEFAULT_SETTINGS, State, handler_for, settings_public, settings_update


class ServerTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.state = State(Path(self.directory.name) / "state.json")
        self.server = ThreadingHTTPServer(("127.0.0.1", 0), handler_for(self.state, False))
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.base = f"http://127.0.0.1:{self.server.server_port}"

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()
        self.directory.cleanup()

    def request(self, path, data=None, headers=None):
        request = urllib.request.Request(self.base + path,
            data=json.dumps(data).encode() if data is not None else None,
            headers=headers if headers is not None else {"Content-Type": "application/json", "X-Zolee-Schema": "1"})
        try:
            with urllib.request.urlopen(request) as response:
                return response.status, json.load(response)
        except urllib.error.HTTPError as error:
            return error.code, json.load(error)

    def test_project_save_is_durable_and_rejects_stale_revision(self):
        project = copy.deepcopy(DEFAULT_PROJECT)
        project["pages"][0]["name"] = "Fűtés"
        status, result = self.request("/api/project", {"project": project, "revision": 0})
        self.assertEqual((status, result["revision"]), (200, 1))
        reloaded = State(self.state.path)
        self.assertEqual(reloaded.project["pages"][0]["name"], "Fűtés")
        self.assertEqual(self.request("/api/project", {"project": DEFAULT_PROJECT, "revision": 0})[0], 409)
        self.assertEqual(self.request("/api/project")[1]["revision"], 1)

    def test_invalid_import_and_cross_origin_do_not_change_project(self):
        project = copy.deepcopy(DEFAULT_PROJECT)
        project["password"] = "must not be imported"
        self.assertEqual(self.request("/api/project", {"project": project, "revision": 0})[0], 400)
        self.assertEqual(self.state.revision, 0)
        headers = {"Content-Type": "application/json", "X-Zolee-Schema": "1", "Origin": "https://unrelated.invalid"}
        self.assertEqual(self.request("/api/project", {"project": DEFAULT_PROJECT, "revision": 0}, headers)[0], 403)
        self.assertEqual(self.state.revision, 0)

    def test_write_failure_keeps_previous_memory_state(self):
        with patch("dev_server.atomic_write", side_effect=OSError("disk full")):
            self.assertEqual(self.request("/api/project", {"project": DEFAULT_PROJECT, "revision": 0})[0], 500)
        self.assertEqual(self.state.revision, 0)
        self.assertFalse(self.state.path.exists())

    def test_settings_preserve_password_and_never_return_it(self):
        old = copy.deepcopy(DEFAULT_SETTINGS)
        old["wifi"]["password"] = "wifi-secret"
        old["loxone"]["password"] = "loxone-secret"
        request = {key: copy.deepcopy(old[key]) for key in ["project", "display", "wifi"]}
        request["wifi"]["password"] = None
        request["display"]["rotation"] = 180
        updated = settings_update(old, request)
        self.assertEqual(updated["wifi"]["password"], "wifi-secret")
        visible = json.dumps(settings_public(updated))
        self.assertNotIn("wifi-secret", visible)
        self.assertNotIn("loxone-secret", visible)
        request["display"]["nightSource"] = {"uuid": "bad", "scale": 1, "offset": 0}
        with self.assertRaises(ValueError):
            settings_update(old, request)


if __name__ == "__main__":
    unittest.main()
