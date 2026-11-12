"""Valódi PNG-méretezés és a Git indexelt tartalmának ellenőrzése."""
from io import BytesIO
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from PIL import Image, PngImagePlugin

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from image_policy import check_images, check_index, png_size, read_policy, POLICY_PATH
from prepare_image import prepare

PREVIEW = "doc/grafika/latvanytervek/test.png"
ELEMENT = "assets/elements/light/circulation-pump.png"


def png(width, height):
    output = BytesIO()
    Image.new("RGBA", (width, height), (200, 30, 30, 180)).save(output, format="PNG")
    return output.getvalue()


def policy(path=ELEMENT, width=160, height=160, role="element"):
    return json.dumps({"format": "zolee-image-sizes", "version": 1,
                       "images": {path: {"role": role, "width": width, "height": height}}}).encode()


class ImagePolicyTests(unittest.TestCase):
    def test_exact_pixels_and_declared_images_required(self):
        files = {POLICY_PATH: policy(), ELEMENT: png(160, 160)}
        self.assertEqual(check_images(set(files), files.__getitem__), 1)
        files[ELEMENT] = png(1024, 1024)
        with self.assertRaisesRegex(ValueError, "pixelmérete"):
            check_images(set(files), files.__getitem__)
        files[ELEMENT] = png(160, 160)
        files["raw.png"] = png(1024, 1024)
        with self.assertRaisesRegex(ValueError, "Nem jegyzett"):
            check_images(set(files), files.__getitem__)
        files.pop("raw.png")
        files.pop(ELEMENT)
        with self.assertRaisesRegex(ValueError, "hiányzó"):
            check_images(set(files), files.__getitem__)

    def test_policy_cannot_authorize_large_runtime_frames(self):
        for data in [policy(width=1024, height=1024), policy(width=80),
                     policy(path="assets/elements/../raw.png"),
                     policy(path=PREVIEW, width=True, role="preview"),
                     policy(path=PREVIEW, width=1536, role="preview")]:
            with self.assertRaises(ValueError):
                read_policy(data)
        with self.assertRaisesRegex(ValueError, "Ismétlődő"):
            read_policy(b'{"format":1,"format":2}')

    def test_fake_png_crc_and_size_budget_rejected(self):
        image = bytearray(png(160, 160))
        image[16] ^= 1
        with self.assertRaises(ValueError):
            png_size(image)
        files = {POLICY_PATH: policy(), ELEMENT: png(160, 160) + bytes(128 * 1024)}
        with self.assertRaisesRegex(ValueError, "Túl nagy képfájl"):
            check_images(set(files), files.__getitem__)
        files[ELEMENT] = b"Not a PNG"
        with self.assertRaisesRegex(ValueError, "PNG-fejléc"):
            check_images(set(files), files.__getitem__)

    def test_preparation_preserves_alpha_canvas_and_strips_metadata(self):
        info = PngImagePlugin.PngInfo()
        info.add_text("prompt", "Discard this working metadata")
        source = BytesIO()
        Image.new("RGBA", (640, 640), (200, 40, 30, 128)).save(source, format="PNG", pnginfo=info)
        source.seek(0)
        data, size = prepare(source, "element")
        self.assertEqual(size, (160, 160))
        with Image.open(BytesIO(data)) as result:
            self.assertEqual(result.mode, "RGBA")
            self.assertEqual(result.getpixel((80, 80))[3], 128)
            self.assertNotIn("prompt", result.info)
        self.assertNotIn(b"Discard", data)
        for size in [(640, 320), (80, 80)]:
            with self.assertRaises(ValueError):
                prepare(BytesIO(png(*size)), "element")
        data, size = prepare(BytesIO(png(1536, 1024)), "preview")
        self.assertEqual(size, (1280, 853))
        self.assertEqual(png_size(data), size)

    def test_branding_avif_requires_genuine_file_and_declared_dimensions(self):
        filename = "assets/branding/readme-banner.avif"
        output = BytesIO()
        Image.new("RGB", (960, 320), (24, 38, 58)).save(output, format="AVIF", quality=35)
        data = output.getvalue()
        files = {POLICY_PATH: policy(filename, 960, 320, role="branding-banner"),
                 filename: data}
        self.assertEqual(check_images(set(files), files.__getitem__), 1)
        files[filename] = png(960, 320)
        with self.assertRaisesRegex(ValueError, "Nem valódi AVIF"):
            check_images(set(files), files.__getitem__)
        files[filename] = data
        files[POLICY_PATH] = policy(filename, 960, 319, role="branding-banner")
        with self.assertRaisesRegex(ValueError, "pixelmérete"):
            check_images(set(files), files.__getitem__)

    def test_documentation_jpeg_is_small_and_cannot_replace_runtime_png(self):
        source = BytesIO()
        Image.new("RGB", (1536, 1024), (225, 230, 235)).save(source, format="PNG")
        source.seek(0)
        data, size = prepare(source, "preview", "JPEG")
        path = PREVIEW.replace(".png", ".jpg")
        files = {POLICY_PATH: policy(path, *size, role="preview"), path: data}
        self.assertEqual(check_images(set(files), files.__getitem__), 1)
        self.assertLess(len(data), 100000)
        files = {POLICY_PATH: policy(), ELEMENT: data}
        with self.assertRaisesRegex(ValueError, "PNG-fejléc"):
            check_images(set(files), files.__getitem__)
        with self.assertRaises(ValueError):
            prepare(BytesIO(png(640, 640)), "element", "JPEG")
        with self.assertRaises(ValueError):
            prepare(BytesIO(png(640, 640)), "preview", "JPEG")

    def test_schema_screenshot_preparation_matches_index_policy(self):
        source = BytesIO()
        Image.new("RGB", (1600, 1180), "white").save(source, format="PNG")
        source.seek(0)
        data, size = prepare(source, "schema-screenshot", "JPEG")
        path = "tests/schemas/kazan/szerkeszto.jpg"
        files = {POLICY_PATH: policy(path, *size, role="schema-screenshot"), path: data}
        self.assertEqual(check_images(set(files), files.__getitem__), 1)
        source.seek(0)
        with self.assertRaises(ValueError):
            prepare(source, "schema-screenshot", "PNG")

    def test_indexed_large_file_cannot_hide_behind_small_working_copy(self):
        with tempfile.TemporaryDirectory(prefix="schema-image-policy-") as directory:
            root = Path(directory)
            def git(*args):
                subprocess.run(["git", *args], cwd=root, check=True, capture_output=True)
            git("init")
            (root / POLICY_PATH).parent.mkdir(parents=True)
            (root / POLICY_PATH).write_bytes(policy())
            image = root / ELEMENT
            image.parent.mkdir(parents=True)
            image.write_bytes(png(160, 160))
            git("add", ".")
            self.assertEqual(check_index(root), 1)
            image.write_bytes(png(1024, 1024))
            # A még nem indexelt munkakép nem kerül a következő commitba.
            self.assertEqual(check_index(root), 1)
            git("add", ELEMENT)
            image.write_bytes(png(160, 160))
            with self.assertRaisesRegex(ValueError, "pixelmérete"):
                check_index(root)
