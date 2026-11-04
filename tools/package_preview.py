"""Újra előállítható SD grafikai készlet és önálló fejlesztői forráscsomag."""

import zipfile
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
DIST.mkdir(exist_ok=True)
shutil.copyfile(DIST / "graphics/manifest.json", DIST / "asset-manifest.json")
shutil.copytree(DIST / "graphics/blobs", DIST / "assets", dirs_exist_ok=True)

with zipfile.ZipFile(DIST / "schema-graphics.zip", "w", zipfile.ZIP_DEFLATED) as archive:
    for file in sorted((DIST / "graphics").rglob("*")):
        if file.is_file():
            archive.write(file, "graphics/" + str(file.relative_to(DIST / "graphics")))

with zipfile.ZipFile(DIST / "schema-web-preview.zip", "w", zipfile.ZIP_DEFLATED) as archive:
    for directory in ["web", "catalog", "tools", "doc", "tests"]:
        for file in sorted((ROOT / directory).rglob("*")):
            if file.is_file() and "__pycache__" not in file.parts and "graphics" not in file.relative_to(ROOT).parts:
                archive.write(file, str(file.relative_to(ROOT)))
    for name in ["README.md", "package.json"]:
        archive.write(ROOT / name, name)
print("Grafikai és webszerkesztő előnézeti csomag elkészült.")
