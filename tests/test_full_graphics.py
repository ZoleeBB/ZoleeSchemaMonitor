"""A teljes bekötés közös regressziója, nem a köztes ötvenes csomagok tesztje."""
import importlib.util
import json
import sys
import unittest
import tempfile
from unittest.mock import patch
import shutil
from io import BytesIO
from PIL import Image
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import build_assets
import ota_identity
spec = importlib.util.spec_from_file_location('full_catalog', ROOT/'catalog/elements.py')
catalog = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog)
from build_assets import manifold_png


class FullGraphicsTests(unittest.TestCase):
    def test_spray_is_registered_only_for_the_five_reviewed_spraying_heads(self):
        items=catalog.elements()
        sprays=[e for e in items if (e.get('animation') or {}).get('kind')=='spray']
        self.assertEqual({e['id'] for e in sprays}, {'rotor-sprinkler','spray-sprinkler',
                         'micro-sprinkler','rotary-nozzle','impact-sprinkler'})
        for e in sprays:
            a=e['animation']
            self.assertTrue(0<a['radius']<=.3)
            self.assertTrue(0<=a['x']<=1 and 0<=a['y']<=1)
            self.assertTrue(-180<=a['direction']<=180)
            self.assertEqual(len(e['ports']),1)

    def test_no_catalog_type_is_left_as_a_placeholder(self):
        items = catalog.elements()
        variant_count = sum(len(brand["types"]) for brand in catalog.BOILER_BRANDS.values())
        self.assertEqual(len(items), 340 + variant_count)
        self.assertTrue(all('artwork' in e or e.get('proceduralManifold') or e['family']=='configurable-manifold' for e in items))
        self.assertEqual(sum('artwork' in e for e in items), 329 + variant_count)
        self.assertEqual(len({e['artwork'] for e in items if 'artwork' in e}), 325 + len(catalog.BOILER_BRANDS))
        built = json.loads((ROOT/'web/catalog.json').read_text())
        self.assertTrue(all(e['graphicsStatus']=='ready' for e in built['elements']))

    def test_branded_boiler_types_use_one_picture_and_named_hydraulic_ports(self):
        items = {e["id"]: e for e in catalog.elements()}
        artwork = set()
        for brand_id, brand in catalog.BOILER_BRANDS.items():
            for kind, source in brand["types"].items():
                with self.subTest(brand=brand_id, kind=kind):
                    item = items[f"boiler-{brand_id}-{kind}"]
                    self.assertEqual(item["artwork"], f"boiler-{brand_id}")
                    self.assertTrue(source.startswith("https://"))
                    self.assertTrue(item["name"]["hu"].startswith(brand["name"] + " – "))
                    expected = 4 if kind in {"combi", "gas-combi", "storage", "cylinder"} else 2
                    self.assertEqual(len({p["name"]["hu"] for p in item["ports"]}), expected)
                    artwork.add(item["artwork"])
        self.assertEqual(len(artwork), len(catalog.BOILER_BRANDS))

    def test_five_batches_cover_exactly_the_250_remaining_types(self):
        batches = json.loads((ROOT/'catalog/integration-batches.json').read_text())
        self.assertEqual([len(b) for b in batches], [50]*5)
        identifiers = [e for b in batches for e in b]
        self.assertEqual(len(set(identifiers)), 250)
        self.assertEqual(set(identifiers), set(catalog.INTEGRATED_ARTWORK))

    def test_legacy_manifolds_have_exact_head_counts_and_keep_return_port_order(self):
        for n in range(2, 17):
            for cooling in (True, False):
                image = Image.open(BytesIO(manifold_png(n, cooling)))
                self.assertEqual(image.size, (96 + 56*n, 224))
                for i in range(n):
                    x = 84 + 56*i
                    for y in (70, 122, 148, 215):
                        self.assertGreater(image.getpixel((x, y))[3], 180)
                    for y in (122, 148, 215):
                        self.assertLess(image.getpixel((x+27, y))[3], 64)
                for y in (99, 191):
                    self.assertGreater(image.getpixel((5, y))[3], 160)
        for e in catalog.elements():
            n = e.get('proceduralManifold')
            if n:
                width = 96 + 56*n
                self.assertEqual([(round(p['x']*width), round(p['y']*224)) for p in e['ports']],
                                 [(4,99),(4,191)] + [(84+56*i,y) for y in (217,124) for i in range(n)])

    def test_rooftop_chiller_rotors_match_the_two_visible_hubs(self):
        chiller = next(item for item in catalog.elements() if item['id'] == 'rooftop-chiller')
        animation = chiller['animation']
        with Image.open(ROOT / 'assets/elements/universal/rooftop-chiller.png') as image:
            hubs = [(round(rotor['x'] * image.width), round(rotor['y'] * image.height))
                    for rotor in animation['rotors']]
        self.assertEqual(hubs, [(50, 34), (106, 34)])
        self.assertEqual((animation['x'], animation['y']),
                         (animation['rotors'][0]['x'], animation['rotors'][0]['y']))

    def test_multiple_rotors_and_oblique_projection_stay_inside_the_picture(self):
        items={e['id']:e for e in catalog.elements()}
        self.assertEqual(len(items['dry-cooler']['animation']['rotors']),3)
        for id in ['heat-pump-dual','chiller','high-temperature-heat-pump','rooftop-chiller']:
            self.assertEqual(len(items[id]['animation']['rotors']),2)
        for e in items.values():
            a=e.get('animation') or {}
            if a.get('kind')!='rotor':continue
            sx,sy=a.get('scaleX',1),a.get('scaleY',1)
            self.assertTrue(0<sx<=1 and 0<sy<=1)
            markers=a.get('rotors',[a]);self.assertLessEqual(len(markers),8)
            for m in markers:
                self.assertTrue(0<=m['x']-m['radius']*sx<m['x']+m['radius']*sx<=1)
                self.assertTrue(0<=m['y']-m['radius']*sy<m['y']+m['radius']*sy<=1)

    def test_probes_electrical_devices_and_empty_enclosures_have_no_fake_water_ports(self):
        items={e['id']:e for e in catalog.elements()}
        for id in ['pool-light','pool-cover','solar-pool-cover','valve-box','irrigation-controller','electric-heater',
                   'temperature-sensor','rain-sensor','level-sensor','co2-sensor','carbon-monoxide-sensor',
                   'conductivity-sensor','salinity-sensor','free-chlorine-sensor','air-velocity-sensor']:
            self.assertEqual(items[id]['ports'],[],id)
        self.assertEqual(len(items['pressure-sensor']['ports']),1)
        self.assertEqual(len(items['gas-meter']['ports']),2)
        self.assertEqual(len(items['air-differential-pressure-sensor']['ports']),2)

    def test_rebuilding_removes_obsolete_generated_graphics_but_preserves_unowned_files(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);output=root/'graphics'
            (root/'assets/elements/universal').mkdir(parents=True)
            shutil.copyfile(ROOT/'assets/elements/universal/ball-valve.png',root/'assets/elements/universal/ball-valve.png')
            for name in ['light/old.svg','dark/old.png','blobs/obsolete','notes.txt']:
                target=output/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text('old')
            item=next(e for e in catalog.elements() if e['id']=='ball-valve')
            with patch.object(build_assets,'ROOT',root),patch.object(build_assets.catalog,'elements',return_value=[item]):
                build_assets.build(output)
            for name in ['light/old.svg','dark/old.png','blobs/obsolete']:
                self.assertFalse((output/name).exists())
            self.assertEqual((output/'notes.txt').read_text(),'old')
            self.assertTrue((output/'light/ball-valve.png').exists())

    def test_binding_metadata_is_part_of_the_firmware_graphics_identity(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            for name in ['catalog/elements.py','catalog/artwork-bindings.json','catalog/port-calibration.json','catalog/boiler-brands.json','tools/build_assets.py','tools/requirements-assets.txt']:
                target=root/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text('initial')
            with patch.object(ota_identity,'ROOT',root):
                before=ota_identity.identity()
                (root/'catalog/artwork-bindings.json').write_text('changed anchor')
                self.assertNotEqual(before,ota_identity.identity())
                before=ota_identity.identity()
                (root/'catalog/port-calibration.json').write_text('recalibrated port')
                self.assertNotEqual(before,ota_identity.identity())
