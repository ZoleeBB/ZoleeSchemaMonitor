"""A szerkesztő tényleges rajzelemeiből összeállított világos/sötét csoportelőnézet."""
import io
import json
import subprocess
from pathlib import Path
from xml.sax.saxutils import escape

import cairosvg
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
# A készülék és a böngésző is ugyanezt a geometriát használja.
source = """
import {newManifold,manifoldParts,manifoldHeadState} from './web/model.js';
const variants=['heating','floor','ceiling-heat','ceiling-cool'];
console.log(JSON.stringify([2,4,8,16].map((n,i)=>{
    const m=newManifold(n);m.variant=variants[i];m.connections=i!==1;
    m.heads.forEach((h,j)=>{h.percent=j===1?25:j>2?100:0;});
    if(n>2)m.heads[2].source={uuid:'12345678-1234-5678-0102030405060708',scale:1,offset:0};
    return {m,parts:manifoldParts(m),states:m.heads.map(h=>manifoldHeadState(h,{}))};
})));
"""
rows = json.loads(subprocess.check_output(["node", "--input-type=module", "-e", source], cwd=ROOT))
svg = ['<svg xmlns="http://www.w3.org/2000/svg" width="1280" height="960">']
labels = ['Fűtés · 2 kör', 'Padlófűtés · 4 kör · csőcsonkok nélkül',
          'Mennyezetfűtés · 8 kör', 'Mennyezethűtés · 16 kör']
for theme in range(2):
    left = theme * 640
    bg, text = ('#17232f', '#dfecf4') if theme else ('#edf4f8', '#354e60')
    svg.append(f'<rect x="{left}" width="640" height="960" fill="{bg}"/>')
    def title(x, y, value, size=16):
        svg.append(f'<text x="{x}" y="{y}" fill="{text}" font-size="{size}" font-family="DejaVu Sans">{escape(value)}</text>')
    title(left+20, 30, 'Termofejes osztó-gyűjtők · ' + ('sötét' if theme else 'világos'), 21)
    for row, data in enumerate(rows):
        y = 65 + row*210
        title(left+20, y, labels[row])
        m = data['m']
        scale = min(1, 600/(80+40*m['circuits']))
        w, h = (80+40*m['circuits'])*scale, 180*scale
        x0, y0 = left+(640-w)/2, y+15
        for p in data['parts']:
            fill = p['color']
            if 'head' in p:
                state = data['states'][p['head']]
                fill = '#d9a23a' if not state['known'] else '#36c678' if state['active'] else '#26333d'
            svg.append(f'<rect x="{x0+p["x"]*w}" y="{y0+p["y"]*h}" width="{p["w"]*w}" height="{p["h"]*h}" rx="2" fill="{fill}" stroke="#67747d" stroke-width=".6"/>')
        for i, state in enumerate(data['states']):
            x = x0+(.16+.78*(i+.5)/m['circuits'])*w
            for value, ytext, size in [(str(i+1), y0+.12*h, 11),
                                       (f'{state["percent"]:.0f}%' if state['known'] else '—', y0+.65*h, max(9,min(16,(80+40*m['circuits'])*.16/m['circuits']))*scale)]:
                svg.append(f'<text x="{x}" y="{ytext}" text-anchor="middle" fill="{text if ytext == y0+.12*h else "#26333d"}" font-family="DejaVu Sans" font-size="{size}">{value}</text>')
    title(left+20, 937, 'Sötét: 0% · Zöld: >0% · Borostyán / —: nincs élő adat', 13)
svg.append('</svg>')
image = Image.open(io.BytesIO(cairosvg.svg2png(bytestring=''.join(svg).encode()))).convert('RGB')
output = ROOT/'doc/grafika/latvanytervek/08d-parameterezheto-osztok.jpg'
image.save(output, quality=94)
print(output)
