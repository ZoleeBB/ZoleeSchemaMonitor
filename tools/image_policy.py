"""A Gitbe kerülő raszterképek felhasználási és pixelméret-korlátai."""
import argparse
import json
from io import BytesIO
from pathlib import Path, PurePosixPath
import struct
import subprocess
import zlib

ROOT = Path(__file__).resolve().parents[1]
POLICY_PATH = "catalog/image-sizes.json"
RASTER_SUFFIXES = {".png", ".jpg", ".jpeg", ".webp", ".gif", ".bmp", ".tif", ".tiff", ".ico", ".avif", ".apng"}
# A látványterv dokumentáció; a panel kizárólag a kis elemképeket kapja.
PROFILES = {
    "manifold-module": {"prefix": "assets/elements/manifold/", "width": 152, "height": 224, "extensions": (".png",), "bytes": 128 * 1024},
    "element": {"prefix": "assets/elements/", "width": 160, "height": 160, "extensions": (".png",), "bytes": 128 * 1024},
    "schema-screenshot": {"prefix": "tests/schemas/", "width": 1600, "height": 1180, "extensions": (".jpg",), "bytes": 512 * 1024},
    "preview": {"prefix": "doc/grafika/latvanytervek/", "width": 1280, "height": 960, "extensions": (".png", ".jpg"), "bytes": 2 * 1024 * 1024},
}


def png_size(data):
    """Az IHDR-t és CRC-jét olvassa; képpontokhoz nem foglal dekódoló puffert."""
    if (len(data) < 33 or data[:8] != b"\x89PNG\r\n\x1a\n" or
            data[8:16] != b"\0\0\0\rIHDR" or
            zlib.crc32(data[12:29]) != int.from_bytes(data[29:33], "big")):
        raise ValueError("Érvénytelen PNG-fejléc")
    return struct.unpack(">II", data[16:24])


def read_policy(data):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"Ismétlődő mező: {key}")
            result[key] = value
        return result

    policy = json.loads(data, object_pairs_hook=unique)
    if (not isinstance(policy, dict) or set(policy) != {"format", "version", "images"} or
            policy["format"] != "zolee-image-sizes" or type(policy["version"]) is not int or
            policy["version"] != 1 or not isinstance(policy["images"], dict)):
        raise ValueError("Érvénytelen képméret-jegyzék")
    for path, entry in policy["images"].items():
        if (not isinstance(entry, dict) or set(entry) != {"role", "width", "height"} or
                not isinstance(entry["role"], str) or entry["role"] not in PROFILES):
            raise ValueError(f"Hiányos képméret: {path}")
        profile = PROFILES[entry["role"]]
        if (not path.startswith(profile["prefix"]) or not path.endswith(profile["extensions"]) or
                "\\" in path or ".." in PurePosixPath(path).parts or
                str(PurePosixPath(path)) != path):
            raise ValueError(f"Nem engedélyezett képútvonal: {path}")
        for axis in ("width", "height"):
            value = entry[axis]
            if type(value) is not int or not 0 < value <= profile[axis]:
                raise ValueError(f"Túlméretes vagy hibás {axis}: {path}")
            if entry["role"] == "element" and value != profile[axis]:
                raise ValueError(f"Az elemkép regisztrált vászna 160×160: {path}")
    return policy["images"]


def check_images(files, read):
    """A fájllista és a tartalom ugyanabból a Git-indexből származzon."""
    entries = read_policy(read(POLICY_PATH))
    images = {path for path in files if PurePosixPath(path).suffix.lower() in RASTER_SUFFIXES}
    if images != set(entries):
        missing = sorted(images - set(entries))
        stale = sorted(set(entries) - images)
        raise ValueError(f"Nem jegyzett képek: {missing}; hiányzó jegyzett képek: {stale}")
    for path in sorted(images):
        entry = entries[path]
        data = read(path)
        if len(data) > PROFILES[entry["role"]]["bytes"]:
            raise ValueError(f"Túl nagy képfájl: {path}")
        if path.endswith(".png"):
            size = png_size(data)
        else:
            from PIL import Image
            with Image.open(BytesIO(data)) as image:
                if image.format != "JPEG":
                    raise ValueError(f"Nem valódi JPEG: {path}")
                size = image.size
        if size != (entry["width"], entry["height"]):
            raise ValueError(f"A kép valódi pixelmérete eltér a jegyzéktől: {path}")
    return len(images)


def check_index(root):
    def git(*args):
        return subprocess.check_output(["git", *args], cwd=root)

    files = git("ls-files", "-z").decode("utf-8").split("\0")
    return check_images(set(files) - {""}, lambda path: git("show", ":" + path))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args()
    try:
        count = check_index(args.root)
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Képméret-ellenőrzés sikertelen: {error}\n")
    print(f"Git-index: {count} regisztrált, megfelelő méretű raszterkép; nagy munkaképek nélkül.")


if __name__ == "__main__":
    main()
