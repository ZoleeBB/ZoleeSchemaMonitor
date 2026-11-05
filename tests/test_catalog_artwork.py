"""A széles típuslista és a már jóváhagyott képek összerendelésének szerződése."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from asset_store import validate_manifest
from ota_identity import identity
from PIL import Image

spec = importlib.util.spec_from_file_location('art_catalog', ROOT / 'catalog/elements.py')
catalog = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog)


class CatalogArtworkTests(unittest.TestCase):
    def test_all_groups_and_native_ports_are_complete(self):
        elements = catalog.elements()
        self.assertGreaterEqual(len(elements), 300)
        self.assertEqual(len(elements), len({item['id'] for item in elements}))
        self.assertEqual({item['category'] for item in elements}, set(catalog.CATEGORIES))
        for item in elements:
            self.assertTrue(item['name']['hu'] and item['name']['en'])
            self.assertLessEqual(len(item['ports']), 16)
            for port in item['ports']:
                self.assertTrue(0 <= port['x'] <= 1 and 0 <= port['y'] <= 1)
        valve = next(item for item in elements if item['id'] == 'changeover-valve-6')
        self.assertEqual(len(valve['ports']), 6)

    def test_real_sources_are_registered_rgba_and_allow_theme_dedup(self):
        elements = [item for item in catalog.elements() if 'artwork' in item]
        self.assertEqual(len(elements), 10)
        self.assertEqual(len({item['artwork'] for item in elements}), 8)
        policy = json.loads((ROOT / 'catalog/image-sizes.json').read_text())['images']
        for item in elements:
            path = f"assets/elements/universal/{item['artwork']}.png"
            self.assertEqual(policy[path]['role'], 'element')
            with Image.open(ROOT / path) as image:
                self.assertEqual(image.size, (160, 160))
                self.assertEqual(image.mode, 'RGBA')
                self.assertEqual(image.getchannel('A').getextrema(), (0, 255))
        self.assertTrue(identity().startswith('ZSM-ASSETS-1:'))

    def test_full_size_release_manifest_is_within_device_limits(self):
        # A tényleges generálást a CI csomaglépése végzi. Itt a 317 típusnyi
        # maximális bejegyzésszámot is ellenőrizzük, nem csak a 10 kész rajzot.
        entries = []
        for item in catalog.elements():
            for theme in ['light', 'dark']:
                for extension in ['svg', 'png']:
                    entries.append({'path': f"{theme}/{item['id']}.{extension}",
                                    'sha256': hashlib.sha256(b'fixture').hexdigest(), 'size': 7})
        manifest = {'format': 'zolee-assets', 'version': '0.1.0', 'assetAbi': 1, 'files': entries}
        validate_manifest(manifest, 1)
        self.assertLess(len(json.dumps(manifest).encode()), 256 * 1024)
        entries.extend({'path': f'overflow/{i}.png', 'sha256': 'a' * 64, 'size': 1} for i in range(2049 - len(entries)))
        with self.assertRaises(ValueError):
            validate_manifest(manifest, 1)
