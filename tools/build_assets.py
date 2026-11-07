"""Egységes B stílusú vektoros készlet és hash-manifest előállítása."""

import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from asset_store import atomic_write, canonical
from ota_identity import identity

spec = importlib.util.spec_from_file_location("catalog", ROOT / "catalog/elements.py")
catalog = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog)


def illustration(element, dark):
    """Natív vektorforrás: a mozgó részt a két megjelenítő külön rajzolja rá."""
    family, identifier = element["family"], element["id"]
    edge = "#adc3d3" if dark else "#394b58"
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="160" height="160" viewBox="0 0 160 160"><defs>'
             '<linearGradient id="metal"><stop stop-color="#64717d"/><stop offset=".22" stop-color="#e9eff3"/>'
             '<stop offset=".45" stop-color="#fff"/><stop offset=".73" stop-color="#a7b4c0"/><stop offset="1" stop-color="#596674"/></linearGradient>'
             '<linearGradient id="red" x2="0" y2="1"><stop stop-color="#ff7c72"/><stop offset=".4" stop-color="#df3b38"/><stop offset="1" stop-color="#822727"/></linearGradient>'
             '<linearGradient id="blue"><stop stop-color="#62d7f1"/><stop offset=".5" stop-color="#1489bf"/><stop offset="1" stop-color="#17476f"/></linearGradient>'
             '<linearGradient id="brass"><stop stop-color="#795b28"/><stop offset=".25" stop-color="#f4d893"/><stop offset=".6" stop-color="#c9a455"/><stop offset="1" stop-color="#85622c"/></linearGradient>'
             f'</defs><g stroke="{edge}" stroke-width="2" stroke-linejoin="round">']
    if family == "configurable-manifold" or element.get("proceduralManifold"):
        parts += ['<rect x="12" y="43" width="142" height="12" rx="4" fill="url(#metal)"/>',
                  '<rect x="12" y="111" width="142" height="12" rx="4" fill="url(#metal)"/>']
        count = element.get("proceduralManifold", 4)
        for x in [25 + 120 * (i + .5) / count for i in range(count)]:
            parts += [f'<path d="M{x} 55V74M{x} 123V151" stroke="#c9a455" stroke-width="7"/>',
                      f'<rect x="{x-min(7,35/count)}" y="23" width="{min(14,70/count)}" height="20" rx="3" fill="#b9d9e5"/>',
                      f'<rect x="{x-min(10,45/count)}" y="80" width="{min(20,90/count)}" height="27" rx="5" fill="url(#metal)"/>',
                      f'<rect x="{x-min(4,20/count)}" y="88" width="{min(8,40/count)}" height="5" fill="#26333d"/>']
    elif family == "pump":
        parts += ['<path d="M8 72H152V104H8Z" fill="url(#red)"/><rect x="8" y="63" width="10" height="50" rx="3" fill="url(#metal)"/>',
                  '<rect x="142" y="63" width="10" height="50" rx="3" fill="url(#metal)"/><rect x="52" y="16" width="56" height="34" rx="5" fill="#303b45"/>',
                  '<circle cx="80" cy="88" r="42" fill="url(#red)"/><circle cx="80" cy="88" r="30" fill="#26343f"/><circle cx="80" cy="88" r="7" fill="url(#metal)"/>']
        for x, y in [(49, 58), (111, 58), (49, 118), (111, 118)]:
            parts.append(f'<circle cx="{x}" cy="{y}" r="3" fill="url(#metal)"/>')
    elif family == "heat-pump":
        parts += ['<rect x="8" y="35" width="144" height="100" rx="6" fill="url(#metal)"/><path d="M24 135V145H39V135M121 135V145H136V135" fill="#485764"/>',
                  '<circle cx="80" cy="88" r="37" fill="#2d3d49"/><circle cx="80" cy="88" r="32" fill="none" stroke="#8d9ca8"/>']
        parts += [f'<path d="M{12 if x < 80 else 124} {y}h24" stroke="#738390"/>' for x in [20, 140] for y in range(51, 125, 10)]
        if identifier == "heat-pump-dual":
            parts.append('<circle cx="80" cy="50" r="10" fill="#263743"/>')
    elif family == "boiler":
        parts += ['<path d="M68 13V4H92V13" fill="url(#metal)"/><rect x="35" y="14" width="90" height="124" rx="8" fill="url(#metal)"/>',
                  '<rect x="43" y="106" width="74" height="22" rx="3" fill="#293944"/><rect x="55" y="112" width="27" height="10" rx="2" fill="#58b3c4"/>',
                  '<circle cx="103" cy="117" r="6" fill="url(#metal)"/><path d="M55 139V155M80 139V155M105 139V155" stroke="#bd9853" stroke-width="8"/>']
    elif family == "tank":
        parts += ['<rect x="37" y="17" width="86" height="125" rx="20" fill="url(#metal)"/><ellipse cx="80" cy="19" rx="42" ry="10" fill="#535f6a"/>',
                  '<path d="M40 137Q80 155 120 137" fill="none"/><path d="M49 145V155M111 145V155" stroke-width="8"/>',
                  '<path d="M123 42H151M123 117H151M9 88H37" stroke="#b89551" stroke-width="10"/>',
                  '<circle cx="80" cy="43" r="11" fill="#edf3f6"/><path d="M80 43L86 38"/>']
        if "coil" in identifier or identifier == "hot-water-tank":
            parts += ['<rect x="60" y="62" width="40" height="64" rx="8" fill="#34424d"/>',
                      '<path d="M68 70H92Q101 76 92 82H68Q59 88 68 94H92Q101 100 92 106H68Q59 112 68 118H92" fill="none" stroke="#e76646" stroke-width="5"/>']
    elif family == "valve":
        parts += ['<path d="M8 72H152V104H8Z" fill="url(#brass)"/><path d="M24 65V110M136 65V110" stroke-width="8" stroke="#bda267"/>',
                  '<rect x="57" y="60" width="46" height="53" rx="9" fill="url(#brass)"/>']
        if len(element["ports"]) == 3:
            parts.append('<path d="M65 111V152H95V111" fill="url(#brass)"/>')
        if identifier in {"ball-valve", "shutoff-valve", "garden-tap"}:
            parts += ['<path d="M80 61V40" stroke-width="10"/><rect x="51" y="26" width="71" height="16" rx="5" fill="url(#red)"/>']
        else:
            parts += ['<rect x="47" y="19" width="66" height="42" rx="7" fill="#33424d"/><circle cx="80" cy="39" r="10" fill="url(#metal)"/><path d="M80 31V46" stroke="#eabf67" stroke-width="3"/>']
    elif family == "manifold":
        circuits = len(element["ports"]) - 2
        parts += ['<rect x="8" y="56" width="144" height="19" rx="6" fill="url(#metal)"/><rect x="8" y="91" width="144" height="19" rx="6" fill="url(#metal)"/>']
        for n in range(circuits):
            x = 160 * (n + 1) / (circuits + 1)
            parts.append(f'<path d="M{x} 40V56M{x} 110V128" stroke="#b59755" stroke-width="6"/><rect x="{x-4}" y="36" width="8" height="13" rx="2" fill="url(#red)"/>')
    elif family == "radiator":
        parts += ['<path d="M9 57H151M9 119H151" stroke="#9baebc" stroke-width="9"/>']
        count = 5 if identifier == "towel-radiator" else 9
        for index in range(count):
            x = 20 + index * 120 / max(1, count - 1)
            parts.append(f'<rect x="{x-5}" y="35" width="10" height="103" rx="5" fill="url(#metal)"/>')
        if identifier in {"fan-coil", "air-conditioner-indoor", "convector"}:
            parts.append('<rect x="16" y="39" width="128" height="94" rx="9" fill="url(#metal)"/><path d="M28 107H132M28 115H132M28 123H132"/>')
    elif family == "surface":
        parts += ['<rect x="12" y="18" width="136" height="125" rx="8" fill="url(#metal)"/>',
                  '<path d="M8 88H24V35H136V52H42V69H136V86H42V103H136V124H24V151" stroke="#3aa3cb" fill="none" stroke-width="6"/>',
                  '<path d="M8 78H17V27H143V59H50V76H143V93H50V110H143V136H17V151" stroke="#df604e" fill="none" stroke-width="4"/>']
    elif family == "fan":
        parts += ['<rect x="22" y="30" width="116" height="116" rx="10" fill="url(#metal)"/><circle cx="80" cy="88" r="46" fill="#2b3d49"/><circle cx="80" cy="88" r="40" fill="none" stroke="#90a7b4"/>']
    elif family == "ahu":
        parts += ['<rect x="12" y="23" width="136" height="116" rx="6" fill="url(#metal)"/>',
                  '<path d="M8 48H34M126 48H152M8 112H34M126 112H152" stroke="#7a8c98" stroke-width="18"/>',
                  '<path d="M56 40L104 120M104 40L56 120" stroke="#507c99" stroke-width="15"/><path d="M34 33V129M126 33V129"/>',
                  '<path d="M65 42L100 102" stroke="#ce6756" stroke-width="5"/><path d="M60 59L88 113" stroke="#4ca7ce" stroke-width="5"/>']
    elif family == "damper":
        parts += ['<rect x="8" y="40" width="144" height="90" rx="4" fill="url(#metal)"/>']
        for y in [53, 76, 99]:
            parts.append(f'<path d="M27 {y}L133 {y+14}" stroke="#657b8d" stroke-width="9"/>')
        parts.append('<rect x="62" y="13" width="36" height="25" rx="4" fill="#35424b"/>')
    elif family == "exchanger":
        parts += ['<path d="M8 48H30M130 48H152M8 112H30M130 112H152" stroke="#b89551" stroke-width="10"/>',
                  '<rect x="30" y="24" width="100" height="112" rx="7" fill="url(#metal)"/>']
        parts += [f'<path d="M{x} 34V126" stroke="#627685" stroke-width="3"/>' for x in range(40, 125, 9)]
    elif family == "filter":
        color = "blue" if identifier in {"sand-filter", "water-filter", "cartridge-filter"} else "metal"
        parts += [f'<rect x="37" y="38" width="86" height="106" rx="29" fill="url(#{color})"/>',
                  '<path d="M8 88H37M123 88H152M80 12V38" stroke="#a3b3bd" stroke-width="12"/><rect x="62" y="9" width="36" height="19" rx="4" fill="#34434e"/>',
                  '<ellipse cx="80" cy="63" rx="33" ry="12" fill="none" stroke-opacity=".5"/><path d="M60 144V152M100 144V152" stroke-width="8"/>']
    elif family == "pool":
        parts += ['<path d="M12 43H148V137H12Z" fill="url(#metal)"/><path d="M20 57Q35 45 50 57T80 57T110 57T140 57V126H20Z" fill="url(#blue)"/>',
                  '<circle cx="44" cy="90" r="12" fill="url(#metal)"/><circle cx="115" cy="90" r="12" fill="url(#metal)"/><path d="M8 88H20M140 88H152" stroke="#a5b3bf" stroke-width="10"/>']
    elif family == "sprinkler":
        parts += ['<path d="M18 136Q50 118 80 136T145 136" stroke="#5b9d59" stroke-width="9" fill="none"/>',
                  '<rect x="65" y="71" width="30" height="69" rx="5" fill="#303f49"/><rect x="59" y="59" width="42" height="18" rx="4" fill="url(#metal)"/>',
                  '<path d="M80 59V44" stroke="#5f7788" stroke-width="9"/>']
        if identifier in {"drip-line", "hose"}:
            parts.append('<path d="M20 95Q40 40 80 85T140 100M20 110Q40 55 80 100T140 115" stroke="#285a49" stroke-width="8" fill="none"/>')
    else:
        parts += ['<path d="M8 88H42M118 88H152" stroke="#ae945d" stroke-width="15"/><circle cx="80" cy="78" r="44" fill="url(#metal)"/><circle cx="80" cy="78" r="34" fill="#edf4f5"/>',
                  '<path d="M80 78L101 57" stroke="#d65b46" stroke-width="4"/><circle cx="80" cy="78" r="5" fill="#3a5366"/>']
        parts += [f'<path d="M{x} {y}l{dx} {dy}" stroke-width="3"/>' for x, y, dx, dy in [(53, 78, 6, 0), (80, 51, 0, 6), (107, 78, -6, 0)]]
    parts.append('</g></svg>')
    return ''.join(parts).encode()


def build(output, png=False, version="0.1.0-ms1"):
    output = Path(output)
    elements = catalog.elements()
    identifiers = [element["id"] for element in elements]
    if len(set(identifiers)) != len(identifiers):
        raise ValueError("Ismétlődő elemazonosító")
    files = []
    for element in elements:
        element["graphics"] = {}
        source = element.pop("artwork", None)
        element["graphicsStatus"] = "ready" if source or element["family"] == "configurable-manifold" or element.get("proceduralManifold") else "placeholder"
        raster = None
        if source:
            from image_policy import png_size
            if not source.replace("-", "").isalnum():
                raise ValueError("Érvénytelen képforrás")
            raster = (ROOT / "assets/elements/universal" / (source + ".png")).read_bytes()
            if png_size(raster) != (160, 160) or raster[24:26] != b"\x08\x06":
                raise ValueError("160×160-as RGBA PNG szükséges")
        for mode in ["light", "dark"]:
            if raster is not None:
                # Egyetlen forráskép: azonos hash esetén nincs dupla letöltés.
                name = f'{mode}/{element["id"]}.png'
                atomic_write(output / name, raster)
                files.append({"path": name, "sha256": hashlib.sha256(raster).hexdigest(), "size": len(raster)})
                element["graphics"][mode] = name
                continue
            svg = illustration(element, mode == "dark")
            name = f'{mode}/{element["id"]}.svg'
            atomic_write(output / name, svg)
            files.append({"path": name, "sha256": hashlib.sha256(svg).hexdigest(), "size": len(svg)})
            element["graphics"][mode] = name
            if png:
                import cairosvg
                image = cairosvg.svg2png(bytestring=svg, output_width=160, output_height=160)
                name = name.replace(".svg", ".png")
                atomic_write(output / name, image)
                files.append({"path": name, "sha256": hashlib.sha256(image).hexdigest(), "size": len(image)})
    data = {"format": "zolee-catalog", "version": 1, "assetAbi": 1,
            "categories": catalog.CATEGORIES, "elements": elements}
    atomic_write(ROOT / "web/catalog.json", canonical(data))
    atomic_write(output / "catalog.json", canonical(data))
    raw = canonical(data)
    files.append({"path": "catalog.json", "sha256": hashlib.sha256(raw).hexdigest(), "size": len(raw)})
    manifest = {"format": "zolee-assets", "version": version, "assetAbi": 1, "files": files}
    # A kiadás minden tartalmat hash szerinti külön fájlként is tartalmaz:
    # így a készüléknek nem kell egy teljes ZIP-et újra letöltenie.
    for entry in files:
        blob = output / "blobs" / entry["sha256"]
        if not blob.exists():
            atomic_write(blob, (output / entry["path"]).read_bytes())
    atomic_write(output / "manifest.json", canonical(manifest))
    atomic_write(output / "identity.txt", identity().encode())
    # Újrageneráláskor a leváltott SVG/PNG és korábbi blob ne kerüljön a ZIP-be.
    required = {entry["path"] for entry in files} | {"blobs/" + entry["sha256"] for entry in files}
    for generated in output.rglob("*"):
        relative = generated.relative_to(output)
        if generated.is_file() and relative.parts[0] in {"light", "dark", "blobs"} and relative.as_posix() not in required:
            generated.unlink()
    print(f'{len(elements)} elemtípus, {len(files)} fájl, {sum(item["size"] for item in files)} bájt')


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", default=str(ROOT / "web/graphics"))
    parser.add_argument("--png", action="store_true")
    parser.add_argument("--version", default="0.1.0-ms1")
    args = parser.parse_args()
    build(args.output, args.png, args.version)
