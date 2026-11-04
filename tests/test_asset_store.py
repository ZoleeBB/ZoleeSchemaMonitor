import copy
import hashlib
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from asset_store import AssetStore, validate_manifest


def manifest(contents):
    return {"format": "zolee-assets", "version": "0.1.0", "assetAbi": 1, "files": [
        {"path": name, "sha256": hashlib.sha256(data).hexdigest(), "size": len(data)}
        for name, data in contents.items()
    ]}


class AssetStoreTests(unittest.TestCase):
    def test_unchanged_files_are_not_downloaded_and_corruption_is_repaired(self):
        with tempfile.TemporaryDirectory() as directory:
            store = AssetStore(directory)
            data = {"pump.svg": b"pump-v1", "fan.svg": b"fan-v1"}
            calls = []

            def fetch(sha, size):
                calls.append(sha)
                return [next(value for value in data.values() if hashlib.sha256(value).hexdigest() == sha)]

            first = store.prepare(manifest(data), fetch)
            store.activate(first["manifest"])
            self.assertEqual(len(calls), 2)
            calls.clear()
            self.assertEqual(store.prepare(manifest(data), fetch)["downloaded"], [])
            self.assertEqual(calls, [])
            data["pump.svg"] = b"pump-v2"
            next_version = store.prepare(manifest(data), fetch)
            self.assertEqual(next_version["downloaded"], ["pump.svg"])
            self.assertEqual(store.active(), first["manifest"])
            sha = manifest(data)["files"][1]["sha256"]
            (store.root / "blobs" / sha).write_bytes(b"broken")
            self.assertEqual(store.prepare(manifest(data), fetch)["downloaded"], ["fan.svg"])
            store.activate(next_version["manifest"])
            self.assertNotEqual(store.active(), first["manifest"])

    def test_interrupted_download_keeps_old_set_and_reuses_completed_files(self):
        with tempfile.TemporaryDirectory() as directory:
            store = AssetStore(directory)
            old = store.prepare(manifest({"old.svg": b"old"}), lambda *_: [b"old"])
            store.activate(old["manifest"])
            incoming = manifest({"a.svg": b"A", "b.svg": b"B"})

            def interrupted(sha, _):
                if sha == hashlib.sha256(b"B").hexdigest():
                    raise OSError("connection lost")
                return [b"A"]

            with self.assertRaises(OSError):
                store.prepare(incoming, interrupted)
            self.assertEqual(store.active(), old["manifest"])
            calls = []

            def resumed(sha, _):
                calls.append(sha)
                return [b"B"]

            store.prepare(incoming, resumed)
            self.assertEqual(len(calls), 1)
            with self.assertRaises(ValueError):
                store.prepare(manifest({"x.svg": b"X"}), lambda *_: [b"wrong"])
            self.assertEqual(store.active(), old["manifest"])

    def test_manifest_rejects_paths_sizes_conflicts_and_firmware_mismatch(self):
        good = manifest({"pump.svg": b"pump"})
        for field, value in [("path", "../secret.svg"), ("path", "/bad.svg"),
                             ("size", True), ("size", 9000000), ("sha256", "bad")]:
            bad = copy.deepcopy(good)
            bad["files"][0][field] = value
            with self.assertRaises(ValueError):
                validate_manifest(bad, 1)
        with self.assertRaises(ValueError):
            validate_manifest(good, 2)
        bad = copy.deepcopy(good)
        bad["files"].append(bad["files"][0])
        with self.assertRaises(ValueError):
            validate_manifest(bad, 1)


if __name__ == "__main__":
    unittest.main()
