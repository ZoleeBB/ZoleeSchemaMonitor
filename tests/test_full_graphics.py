"""A teljes bekötés közös regressziója, nem a köztes ötvenes csomagok tesztje."""
import importlib.util
import json
import sys
import unittest
import tempfile
from unittest.mock import patch
import shutil
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import build_assets
import ota_identity
spec = importlib.util.spec_from_file_location('full_catalog', ROOT/'catalog/elements.py')
catalog = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog)
from build_assets import illustration


class FullGraphicsTests(unittest.TestCase):
    def test_no_catalog_type_is_left_as_a_placeholder(self):
        items = catalog.elements()
        self.assertEqual(len(items), 333)
        self.assertTrue(all('artwork' in e or e.get('proceduralManifold') or e['family']=='configurable-manifold' for e in items))
        self.assertEqual(sum('artwork' in e for e in items), 322)
        self.assertEqual(len({e['artwork'] for e in items if 'artwork' in e}), 319)
        built = json.loads((ROOT/'web/catalog.json').read_text())
        self.assertTrue(all(e['graphicsStatus']=='ready' for e in built['elements']))

    def test_five_batches_cover_exactly_the_250_remaining_types(self):
        batches = json.loads((ROOT/'catalog/integration-batches.json').read_text())
        self.assertEqual([len(b) for b in batches], [50]*5)
        identifiers = [e for b in batches for e in b]
        self.assertEqual(len(set(identifiers)), 250)
        self.assertEqual(set(identifiers), set(catalog.INTEGRATED_ARTWORK))

    def test_legacy_manifolds_have_exact_head_counts_and_keep_return_port_order(self):
        ns={'s':'http://www.w3.org/2000/svg'}
        for e in catalog.elements():
            if not e.get('proceduralManifold'):continue
            n=e['proceduralManifold']
            self.assertEqual(len(e['ports']), 2+2*n)
            self.assertEqual(len({(p['x'],p['y']) for p in e['ports']}), 2+2*n)
            self.assertTrue(all(p['y']>.9 for p in e['ports'][2:2+n]))
            self.assertTrue(all(.4<p['y']<.5 for p in e['ports'][2+n:]))
            for dark in [False,True]:
                xml=ET.fromstring(illustration(e,dark))
                heads=[p for p in xml.findall('.//s:rect',ns) if p.get('y')=='80']
                meters=[p for p in xml.findall('.//s:rect',ns) if p.get('y')=='23']
                self.assertEqual(len(heads),n); self.assertEqual(len(meters),n)
                self.assertTrue(all(float(p.get('width'))>0 for p in heads))
            self.assertFalse((ROOT/f'assets/elements/universal/{e["id"]}.png').exists())

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
            for name in ['catalog/elements.py','catalog/artwork-bindings.json','tools/build_assets.py','tools/requirements-assets.txt']:
                target=root/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text('initial')
            with patch.object(ota_identity,'ROOT',root):
                before=ota_identity.identity()
                (root/'catalog/artwork-bindings.json').write_text('changed anchor')
                self.assertNotEqual(before,ota_identity.identity())
