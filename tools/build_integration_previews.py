"""Öt ötvenes csomag világos/sötét előnézete a tényleges bekötési adatokból."""
import argparse
import json
import importlib.util
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('preview_catalog', ROOT/'catalog/elements.py')
catalog = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog)
items = {e['id']: e for e in catalog.elements()}
parser = argparse.ArgumentParser()
parser.add_argument('batch', type=int, choices=range(1, 6))
args = parser.parse_args()
ids = json.loads((ROOT/'catalog/integration-batches.json').read_text())[args.batch-1]
font = ImageFont.truetype('DejaVuSans.ttf', 10)
heading = ImageFont.truetype('DejaVuSans.ttf', 18)
out = Image.new('RGB', (1280, 960))
draw = ImageDraw.Draw(out)
for dark in range(2):
    top = dark*480
    background, ink = ('#17232f', '#dfecf4') if dark else ('#edf4f8', '#354e60')
    draw.rectangle((0, top, 1279, top+479), fill=background)
    draw.text((10,top+3), f'{args.batch}. csomag · 50 típus · ' + ('sötét' if dark else 'világos'), fill=ink, font=heading)
    for i, identifier in enumerate(ids):
        item = items[identifier]
        x, y = i%10*128, top+29+i//10*90
        if 'artwork' in item:
            source = Image.open(ROOT/'assets/elements/universal'/f'{item["artwork"]}.png').convert('RGBA')
        else:
            source = Image.open(ROOT/'web/graphics'/('dark' if dark else 'light')/f'{identifier}.png').convert('RGBA')
        source.thumbnail((68,68), Image.Resampling.LANCZOS)
        out.paste(source, (x+30+(68-source.width)//2,y+(68-source.height)//2), source)
        # A mozgó réteg nyugalmi helyzete; nem módosítja az elem PNG-forrását.
        animation = item.get('animation') or {}
        if animation.get('kind') == 'rotor':
            for marker in animation.get('rotors', [animation]):
                cx, cy, radius = x+30+68*marker['x'], y+68*marker['y'], 68*marker['radius']*animation.get('scaleX',1)
                draw.line((cx-radius,cy,cx+radius,cy),fill='#9cb0be',width=2)
        name = item['name']['hu']
        if len(name)>23: name=name[:22]+'…'
        draw.text((x+64,y+69), f'{i+1}. {name}', fill=ink, font=font, anchor='mt')
output = ROOT/f'doc/grafika/latvanytervek/13-bekotes-{args.batch:02}.jpg'
out.save(output, quality=94)
print(output)
