"""A tényleges ESP-IDF flashlistából készít telepítőcsomagot és OTA-binárist."""

import hashlib
import json
import shutil
import zipfile
from pathlib import Path
from ota_identity import identity
from asset_store import validate_release_graphics
from build_assets import catalog

ROOT = Path(__file__).resolve().parents[1]
BUILD, DIST = ROOT / "build", ROOT / "dist/firmware"
DIST.mkdir(parents=True, exist_ok=True)
args = json.loads((BUILD / "flasher_args.json").read_text())
app = BUILD / "zolee_schema_monitor.bin"
if not app.is_file() or app.stat().st_size > 8 * 1024 * 1024:
    raise SystemExit("A firmware hiányzik vagy túllépi a 8 MiB OTA partíciót")
# Az IDF saját leírója után közvetlenül kell állnia az OTA kompatibilitási jelnek.
expected = identity().encode() + b"\0"
if app.read_bytes()[288:288 + len(expected)] != expected:
    raise SystemExit("Hiányzó vagy rossz helyre linkelt OTA kompatibilitási leíró")
shutil.copyfile(app, DIST / "ZoleeSchemaMonitor.bin")
manifest = ROOT / "dist/asset-manifest.json"
if not manifest.is_file():
    raise SystemExit("A firmware csomagolásához a saját grafikai manifest is szükséges")
def file_description(file):
    return {"sha256": hashlib.sha256(file.read_bytes()).hexdigest(), "size": file.stat().st_size}
version = json.loads((BUILD / "project_description.json").read_text())["project_version"]
try:
    graphics = validate_release_graphics(json.loads(manifest.read_bytes()), catalog.elements())
except ValueError as error:
    raise SystemExit(str(error)) from error
if graphics["version"] != version:
    raise SystemExit("Eltérő firmware/grafika kiadási verzió")
(DIST / "update.json").write_text(json.dumps({"format":"zolee-update", "version":version,
    "identity":identity(), "manifest":file_description(manifest), "firmware":file_description(app)},
    separators=(",", ":")) + "\n")
with zipfile.ZipFile(DIST / "ZoleeSchemaMonitor-usb.zip", "w", zipfile.ZIP_DEFLATED) as archive:
    for offset, name in args["flash_files"].items():
        file = (BUILD / name).resolve()
        if BUILD.resolve() not in file.parents or not file.is_file():
            raise SystemExit("Hibás flash-fájllista")
        archive.write(file, name)
    archive.writestr("flasher_args.json", json.dumps(args, indent=2))
    pairs = " ".join(f'{offset} "{name}"' for offset, name in args["flash_files"].items())
    archive.writestr("FLASH.txt", f"MS1 fejlesztői build: helyi webszerkesztő, SD-mentés, Wi-Fi és diagnosztika.\nNatív sémanézet, animáció és numerikus Loxone-kliens bekötve. Fizikai Miniserveres próba szükséges; közös OTA frissítés a Frissítés oldalon, fizikai próba szükséges.\nAz SD-csomag graphics/ könyvtárát másold a FAT32 kártya gyökerébe.\nElső beállítás: a kijelzőn látható védett Wi-Fi, majd http://192.168.4.1/\npython -m esptool --chip esp32p4 write_flash {pairs}\n")
with (DIST / "SHA256SUMS.txt").open("w") as stream:
    for file in sorted(DIST.iterdir()):
        if file.suffix in {".zip", ".bin"}:
            stream.write(hashlib.sha256(file.read_bytes()).hexdigest() + "  " + file.name + "\n")
print(f"Firmware: {app.stat().st_size} bájt / 8388608 bájt")
