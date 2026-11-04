"""A tényleges ESP-IDF flashlistából készít telepítőcsomagot és OTA-binárist."""

import hashlib
import json
import shutil
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD, DIST = ROOT / "build", ROOT / "dist/firmware"
DIST.mkdir(parents=True, exist_ok=True)
args = json.loads((BUILD / "flasher_args.json").read_text())
app = BUILD / "zolee_schema_monitor.bin"
if not app.is_file() or app.stat().st_size > 8 * 1024 * 1024:
    raise SystemExit("A firmware hiányzik vagy túllépi a 8 MiB OTA partíciót")
shutil.copyfile(app, DIST / "ZoleeSchemaMonitor.bin")
with zipfile.ZipFile(DIST / "ZoleeSchemaMonitor-usb.zip", "w", zipfile.ZIP_DEFLATED) as archive:
    for offset, name in args["flash_files"].items():
        file = (BUILD / name).resolve()
        if BUILD.resolve() not in file.parents or not file.is_file():
            raise SystemExit("Hibás flash-fájllista")
        archive.write(file, name)
    archive.writestr("flasher_args.json", json.dumps(args, indent=2))
    pairs = " ".join(f'{offset} "{name}"' for offset, name in args["flash_files"].items())
    archive.writestr("FLASH.txt", f"MS1 fejlesztői build: helyi webszerkesztő, SD-mentés, Wi-Fi és diagnosztika.\nNatív sémanézet, animáció és numerikus Loxone-kliens bekötve. Fizikai Miniserveres próba szükséges; OTA még nincs.\nAz SD-csomag graphics/ könyvtárát másold a FAT32 kártya gyökerébe.\nElső beállítás: a kijelzőn látható védett Wi-Fi, majd http://192.168.4.1/\npython -m esptool --chip esp32p4 write_flash {pairs}\n")
with (DIST / "SHA256SUMS.txt").open("w") as stream:
    for file in sorted(DIST.iterdir()):
        if file.suffix in {".zip", ".bin"}:
            stream.write(hashlib.sha256(file.read_bytes()).hexdigest() + "  " + file.name + "\n")
print(f"Firmware: {app.stat().st_size} bájt / 8388608 bájt")
