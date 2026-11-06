"""A széles típuslista és a már jóváhagyott képek összerendelésének szerződése."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from asset_store import validate_manifest, validate_release_graphics
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

    def test_closed_pump_housings_have_status_and_two_water_ports(self):
        identifiers = ['end-suction-pump', 'long-coupled-pump',
                       'vertical-multistage-pump', 'horizontal-multistage-pump',
                       'jet-pump', 'self-priming-pump', 'piston-pump', 'diaphragm-pump',
                       'gear-pump', 'condensate-pump', 'sewage-lifting-station', 'macerator-pump']
        elements = {item['id']: item for item in catalog.elements()}
        for identifier in identifiers:
            with self.subTest(identifier=identifier):
                item = elements[identifier]
                self.assertEqual(item['artwork'], identifier)
                self.assertEqual(len(item['ports']), 2)
                self.assertEqual(item['animation']['kind'], 'status')

    def test_piston_drive_and_diaphragm_air_fittings_are_not_water_ports(self):
        elements = {item['id']: item for item in catalog.elements()}
        # A dugattyús gép folyadékoldala a jobb oldali rézfejen van, nem a bal tengelyen.
        self.assertTrue(all(p['x'] > .6 for p in elements['piston-pump']['ports']))
        # A membránszivattyú folyadékcsatlakozói a két külső gyűjtőn vannak.
        ports = elements['diaphragm-pump']['ports']
        self.assertTrue(any(p['y'] < .15 for p in ports))
        self.assertTrue(any(p['y'] > .85 for p in ports))

    def test_gear_drive_and_macerator_closed_cap_are_not_flow_ports(self):
        elements = {item['id']: item for item in catalog.elements()}
        self.assertTrue(all(p['x'] > .8 for p in elements['gear-pump']['ports']))
        # Az alsó jobb oldali lezárt sapka helyett az elülső beömlő és felső könyök köthető.
        self.assertTrue(all(p['y'] < .6 for p in elements['macerator-pump']['ports']))

    def test_storage_ports_exclude_air_valves_and_service_caps(self):
        # A zárt tágulási tartályon egy vízcsatlakozó van; a felső sapka nem második ág.
        expected = {'electric-water-heater': 2, 'expansion-vessel': 1,
                    'hydrophore': 1, 'combi-storage': 4, 'heating-expansion-vessel': 1,
                    'potable-expansion-vessel': 1, 'open-expansion-tank': 2, 'rainwater-tank': 2,
                    'indirect-water-heater': 4, 'tank-in-tank': 4,
                    'solar-hot-water-tank': 6, 'stainless-hot-water-tank': 4,
                    'rectangular-buffer': 4, 'stratified-buffer': 6,
                    'water-tank': 1, 'condensate-receiver': 3,
                    'brine-tank': 1, 'underground-cistern': 2}
        elements = {item['id']: item for item in catalog.elements()}
        for identifier, count in expected.items():
            with self.subTest(identifier=identifier):
                item = elements[identifier]
                self.assertEqual(item['artwork'], identifier)
                self.assertEqual(len(item['ports']), count)
                self.assertEqual(item['animation']['kind'], 'none')
        self.assertLess(elements['potable-expansion-vessel']['ports'][0]['y'], .1)
        for identifier in ['expansion-vessel', 'hydrophore', 'heating-expansion-vessel']:
            self.assertGreater(elements[identifier]['ports'][0]['y'], .8)

    def test_storage_group_uses_artwork_and_closed_fan_has_status_only(self):
        storage = [item for item in catalog.elements() if item['category'] == 'storage']
        self.assertTrue(all('artwork' in item for item in storage))
        heater = next(item for item in storage if item['id'] == 'heat-pump-water-heater')
        self.assertEqual(heater['animation']['kind'], 'status')
        # A víz- és légoldal külön végpontokkal szerkeszthető; nincs belső csőkapcsolat.
        self.assertEqual(len(heater['ports']), 4)
        self.assertTrue(all(p['y'] < .1 for p in heater['ports'][:2]))
        self.assertTrue(all(p['y'] > .7 for p in heater['ports'][2:]))

    def test_rotary_valves_have_independent_position_overlay(self):
        ids = {'black-rotary-valve-3', 'black-union-rotary-valve-3', 'black-flanged-rotary-valve-3'}
        valves = [item for item in catalog.elements() if item['id'] in ids]
        self.assertEqual(len(valves), 3)
        for valve in valves:
            self.assertEqual(len(valve['ports']), 3)
            self.assertEqual(valve['animation']['kind'], 'position')
            self.assertEqual(valve['artwork'], valve['id'])
            self.assertTrue(0 < valve['animation']['radius'] < .2)

    def test_real_sources_are_registered_rgba_and_allow_theme_dedup(self):
        elements = [item for item in catalog.elements() if 'artwork' in item]
        self.assertGreaterEqual(len(elements), 15)
        self.assertGreaterEqual(len({item['artwork'] for item in elements}), 12)
        policy = json.loads((ROOT / 'catalog/image-sizes.json').read_text())['images']
        for item in elements:
            path = f"assets/elements/universal/{item['artwork']}.png"
            self.assertEqual(policy[path]['role'], 'element')
            with Image.open(ROOT / path) as image:
                self.assertEqual(image.size, (160, 160))
                self.assertEqual(image.mode, 'RGBA')
                self.assertEqual(image.getchannel('A').getextrema(), (0, 255))
        self.assertTrue(identity().startswith('ZSM-ASSETS-1:'))

    def test_release_accepts_png_sources_and_requires_both_themes(self):
        elements = [{'id': 'pump', 'artwork': 'pump'}, {'id': 'valve'}]
        paths = ['catalog.json', 'light/pump.png', 'dark/pump.png',
                 'light/valve.svg', 'dark/valve.svg', 'light/valve.png', 'dark/valve.png']
        entries = [{'path': path, 'sha256': 'a' * 64, 'size': 1} for path in paths]
        manifest = {'format': 'zolee-assets', 'version': '0.1.0', 'assetAbi': 1, 'files': entries}
        validate_release_graphics(manifest, elements)
        for missing in ['dark/pump.png', 'light/valve.svg']:
            invalid = {**manifest, 'files': [entry for entry in entries if entry['path'] != missing]}
            with self.assertRaises(ValueError):
                validate_release_graphics(invalid, elements)
        invalid = {**manifest, 'files': entries + [{'path': 'light/pump.svg', 'sha256': 'a' * 64, 'size': 1}]}
        with self.assertRaises(ValueError):
            validate_release_graphics(invalid, elements)

    def test_full_size_release_manifest_is_within_device_limits(self):
        # A tényleges generálást a CI csomaglépése végzi. Itt a teljes katalógusnyi
        # maximális bejegyzésszámot is ellenőrizzük, nem csak a már bekötött képeket.
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
