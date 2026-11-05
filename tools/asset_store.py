"""SHA-256 alapú, fájlonként folytatható grafikai tár referencia-megvalósítása."""

import hashlib
import json
import os
import re
from pathlib import Path

MAX_FILES = 2048
MAX_FILE_BYTES = 4 * 1024 * 1024
MAX_TOTAL_BYTES = 128 * 1024 * 1024
SHA = re.compile(r"^[0-9a-f]{64}$")


def canonical(value):
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode()


def digest_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_manifest(manifest, expected_abi):
    if not isinstance(manifest, dict) or set(manifest) != {"format", "version", "assetAbi", "files"}:
        raise ValueError("Érvénytelen grafikai manifest")
    if manifest["format"] != "zolee-assets" or type(manifest["assetAbi"]) is not int or manifest["assetAbi"] != expected_abi:
        raise ValueError("A grafikai készlet nem kompatibilis a firmware-rel")
    if not isinstance(manifest["version"], str) or not re.fullmatch(r"[a-zA-Z0-9._-]{1,64}", manifest["version"]):
        raise ValueError("Érvénytelen készletverzió")
    files = manifest["files"]
    if not isinstance(files, list) or not 1 <= len(files) <= MAX_FILES:
        raise ValueError("Érvénytelen fájlszám")
    names, sizes, total = set(), {}, 0
    for entry in files:
        if not isinstance(entry, dict) or set(entry) != {"path", "sha256", "size"}:
            raise ValueError("Érvénytelen fájlbejegyzés")
        path, sha, size = entry["path"], entry["sha256"], entry["size"]
        if not isinstance(path, str) or not re.fullmatch(r"[a-zA-Z0-9_/-]+\.(svg|png|json)", path):
            raise ValueError("Tiltott grafikai útvonal")
        if path.startswith("/") or ".." in path or "//" in path or path in names:
            raise ValueError("Ismétlődő vagy tiltott útvonal")
        if not isinstance(sha, str) or not SHA.fullmatch(sha):
            raise ValueError("Érvénytelen SHA-256")
        if type(size) is not int or not 0 < size <= MAX_FILE_BYTES:
            raise ValueError("Túl nagy grafikai fájl")
        if sha in sizes and sizes[sha] != size:
            raise ValueError("Azonos hash eltérő mérettel")
        names.add(path)
        sizes[sha] = size
        total += size
    if total > MAX_TOTAL_BYTES:
        raise ValueError("Túl nagy grafikai készlet")
    return manifest


def atomic_write(path, data):
    """A régi fájl az új tartós lezárásáig megmarad; nincs korai sikerjelzés."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    with temporary.open("wb") as stream:
        stream.write(data)
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, path)
    if os.name != "nt":
        descriptor = os.open(path.parent, os.O_RDONLY)
        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)


class AssetStore:
    def __init__(self, root):
        self.root = Path(root)
        (self.root / "blobs").mkdir(parents=True, exist_ok=True)
        (self.root / "manifests").mkdir(exist_ok=True)

    def valid_blob(self, entry):
        path = self.root / "blobs" / entry["sha256"]
        return path.is_file() and path.stat().st_size == entry["size"] and digest_file(path) == entry["sha256"]

    def prepare(self, manifest, fetch, expected_abi=1):
        """A fetch hash-t és méretet kap; importált URL-t soha nem követünk."""
        validate_manifest(manifest, expected_abi)
        downloaded = []
        for entry in manifest["files"]:
            if self.valid_blob(entry):
                continue
            target = self.root / "blobs" / entry["sha256"]
            temporary = target.with_suffix(".part")
            digest, size = hashlib.sha256(), 0
            try:
                with temporary.open("wb") as stream:
                    for chunk in fetch(entry["sha256"], entry["size"]):
                        size += len(chunk)
                        if size > entry["size"]:
                            raise ValueError("A letöltött grafika túl nagy")
                        digest.update(chunk)
                        stream.write(chunk)
                    stream.flush()
                    os.fsync(stream.fileno())
                if size != entry["size"] or digest.hexdigest() != entry["sha256"]:
                    raise ValueError("A grafika mérete vagy hash-e hibás")
                os.replace(temporary, target)
                if os.name != "nt":
                    descriptor = os.open(target.parent, os.O_RDONLY)
                    try:
                        os.fsync(descriptor)
                    finally:
                        os.close(descriptor)
                downloaded.append(entry["path"])
            finally:
                temporary.unlink(missing_ok=True)
        manifest_id = hashlib.sha256(canonical(manifest)).hexdigest()
        atomic_write(self.root / "manifests" / (manifest_id + ".json"), canonical(manifest))
        return {"manifest": manifest_id, "downloaded": downloaded}

    def activate(self, manifest_id, expected_abi=1):
        if not SHA.fullmatch(manifest_id):
            raise ValueError("Érvénytelen manifest-azonosító")
        raw = (self.root / "manifests" / (manifest_id + ".json")).read_bytes()
        if hashlib.sha256(raw).hexdigest() != manifest_id:
            raise ValueError("Sérült manifest")
        manifest = validate_manifest(json.loads(raw), expected_abi)
        if not all(self.valid_blob(entry) for entry in manifest["files"]):
            raise ValueError("Hiányos készlet nem aktiválható")
        atomic_write(self.root / "active.json", canonical({"manifest": manifest_id}))

    def active(self):
        pointer = self.root / "active.json"
        return json.loads(pointer.read_text())["manifest"] if pointer.exists() else None


def validate_release_graphics(manifest, elements):
    """Minden típusnak mindkét témában legyen PNG-je; SVG csak a vektorforrásnál kell."""
    validate_manifest(manifest, 1)
    required = {"catalog.json"}
    for element in elements:
        kinds = ["png"] if element.get("artwork") else ["svg", "png"]
        required.update(f"{mode}/{element['id']}.{kind}" for mode in ["light", "dark"] for kind in kinds)
    if {entry["path"] for entry in manifest["files"]} != required:
        raise ValueError("Hiányos vagy eltérő világos/sötét grafikai csomag")
    return manifest
