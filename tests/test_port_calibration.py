"""A kézi képaudit lenyomata képváltáskor új ellenőrzést követel."""
import hashlib
import importlib.util
import json
from pathlib import Path
import unittest
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('ports_catalog',ROOT/'catalog/elements.py')
catalog=importlib.util.module_from_spec(spec);spec.loader.exec_module(catalog)

class PortCalibrationTests(unittest.TestCase):
    def test_review_covers_every_type_and_is_bound_to_the_reviewed_source_bytes(self):
        calibration=json.loads((ROOT/'catalog/port-calibration.json').read_text())
        items=catalog.elements()
        self.assertEqual(set(calibration['elements']),{e['id'] for e in items})
        for e in items:
            audit=calibration['elements'][e['id']]
            self.assertEqual(len(audit['portsPx']),len(e['ports']),e['id'])
            if audit['mode']=='procedural':
                module=ROOT/'assets/elements/manifold/assembly.png'
                self.assertEqual(audit['sourceSha256'],hashlib.sha256(module.read_bytes()).hexdigest())
            if 'artwork' not in e:continue
            source=ROOT/f'assets/elements/universal/{e["artwork"]}.png'
            self.assertEqual(audit['sourceSha256'],hashlib.sha256(source.read_bytes()).hexdigest(),e['id'])
            if audit['mode']=='projected':self.assertTrue(e.get('portNote'),e['id'])
            if audit['mode']!='visible':continue
            # Ez csak az üres háttérre csúszott pontokat szűri; a nyílás azonosítása kézi audit.
            with Image.open(source) as image:
                for x,y in audit['portsPx']:
                    crop=image.getchannel('A').crop((max(0,round(x)-3),max(0,round(y)-3),
                                                   min(160,round(x)+4),min(160,round(y)+4)))
                    self.assertGreaterEqual(crop.getextrema()[1],100,e['id'])

    def test_radiator_landmarks_are_the_four_side_connector_mouths(self):
        radiator=next(e for e in catalog.elements() if e['id']=='radiator')
        self.assertEqual([(p['x']*160,p['y']*160) for p in radiator['ports']],
                         [(11,53),(149,53),(11,109),(149,109)])
