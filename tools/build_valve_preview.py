"""Az első tíz alapszelep tényleges képeiből készülő világos/sötét előnézet."""
import importlib.util
import textwrap
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('catalog', ROOT/'catalog/elements.py')
catalog = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog)
items = {e['id']: e for e in catalog.elements()}
IDENTIFIERS = ['ball-valve', 'shutoff-valve', 'check-valve', 'solenoid-valve',
               'motor-valve-2', 'motor-valve-3', 'diverting-valve', 'thermostatic-valve',
               'balancing-valve', 'pressure-reducer']
font = ImageFont.truetype('DejaVuSans.ttf', 14)
title = ImageFont.truetype('DejaVuSans.ttf', 22)
image = Image.new('RGB', (1280, 960))
draw = ImageDraw.Draw(image)
for mode in range(2):
    start = mode * 640
    bg, ink = ('#17232f', '#dfecf4') if mode else ('#edf4f8', '#354e60')
    draw.rectangle((start, 0, start+639, 959), fill=bg)
    draw.text((start+20, 15), 'Alapszelepek · ' + ('sötét' if mode else 'világos'), font=title, fill=ink)
    for index, identifier in enumerate(IDENTIFIERS):
        item = items[identifier]
        x, y = start + index % 2 * 320, 52 + index // 2 * 178
        source = Image.open(ROOT/'assets/elements/universal'/f'{item["artwork"]}.png').convert('RGBA')
        source.thumbnail((140, 140), Image.Resampling.LANCZOS)
        image.paste(source, (x+90, y), source)
        for line, value in enumerate(textwrap.wrap(item['name']['hu'], width=34)):
            draw.text((x+160, y+140+line*17), value, font=font, fill=ink, anchor='mt')
output = ROOT/'doc/grafika/latvanytervek/08e-alapszelepek.jpg'
image.save(output, quality=94)
print(output)
