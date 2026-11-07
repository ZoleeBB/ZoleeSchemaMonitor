"""A becsomagolt osztótestek a webszerkesztő jelzőivel és csonkjaival."""
import io
import json
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont
from build_assets import manifold_png

ROOT = Path(__file__).resolve().parents[1]
source = """
import {newManifold, manifoldParts, manifoldCenter, manifoldHeadState} from './web/model.js';
console.log(JSON.stringify([2,4,8,16].map((n,row)=>{
    const m=newManifold(n);
    if(row===3) m.variant='ceiling-cool';
    m.heads.forEach((h,i)=>{h.percent=[0,25,50,100][i%4];});
    m.heads[n-1].source={uuid:'12345678-1234-5678-0102030405060708',scale:1,offset:0};
    return {n, parts:manifoldParts(m), heads:m.heads.map((h,i)=>({
        x:manifoldCenter(n,i), ...manifoldHeadState(h,{})}))};
})));
"""
rows = json.loads(subprocess.check_output(["node", "--input-type=module", "-e", source], cwd=ROOT))
image = Image.new("RGB", (1280,960), "#edf4f8")
draw = ImageDraw.Draw(image)
def font(size):
    return ImageFont.truetype("DejaVuSans.ttf", size)
layouts = [(0,0,480,310), (480,0,800,310), (0,310,1280,315), (0,625,1280,335)]
titles = ["Fűtés", "Padlófűtés", "Mennyezetfűtés", "Mennyezethűtés"]
for index, (row, (x,y,w,h)) in enumerate(zip(rows,layouts)):
    dark = index in (1,3)
    bg, ink = ("#182733","#ecf2f6") if dark else ("#edf4f8","#273a48")
    draw.rectangle((x,y,x+w-1,y+h-1), fill=bg)
    n = row["n"]
    draw.text((x+32,y+20), f"{titles[index]} · {n} kör", font=font(23), fill=ink)
    body = Image.open(io.BytesIO(manifold_png(n,index==3)))
    bw,bh = body.size
    left,top=x+(w-bw)//2,y+(h-bh)//2+8
    image.paste(body,(left,top),body)
    for part in row["parts"]:
        color = part["color"]
        if "head" in part:
            head=row["heads"][part["head"]]
            color="#d9a23a" if not head["known"] else "#36c678" if head["active"] else "#26333d"
        px=round(left+part["x"]*bw); py=round(top+part["y"]*bh)
        draw.rounded_rectangle((px,py,px+round(part["w"]*bw)-1,py+round(part["h"]*bh)-1),
                               radius=1,fill=color,outline="#67747d")
    for i,head in enumerate(row["heads"]):
        cx=round(left+head["x"]*bw)
        draw.text((cx,top+26),str(i+1),anchor="ms",font=font(11),fill=ink)
        text=f"{head['percent']:.0f}%" if head["known"] else "—"
        draw.text((cx,top+161),text,anchor="ms",font=font(13),fill="#26333d")
image.save(ROOT/"doc/grafika/latvanytervek/08d-parameterezheto-osztok.jpg", quality=93)
