"""A szerkesztő és a natív validátor azonos, firmware-ben rögzített katalógusa."""
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("catalog", ROOT / "catalog/elements.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
quote = lambda value: json.dumps(value, ensure_ascii=False)
lines = ["/* Generált fájl: tools/generate_catalog.py. Kézzel nem szerkesztendő. */", "const sm_asset_t sm_catalog[] = {"]
for item in module.elements():
    ports = ", ".join("{%sf, %sf}" % (float(p["x"]), float(p["y"])) for p in item["ports"])
    animation = item["animation"] or {"kind": "none", "x": 0, "y": 0, "radius": 0}
    lines.append("    {%s, %s, %s, %s, %d, {%s}, %d, %sf, %sf, %sf}," % (
        quote(item["id"]), quote(item["name"]["hu"]), quote(item["name"]["en"]), quote(item["family"]),
        len(item["ports"]), ports, {"none": 0, "rotor": 1, "spray": 2, "status": 3}[animation["kind"]],
        float(animation["x"]), float(animation["y"]), float(animation["radius"])))
lines += ["};", "const unsigned sm_catalog_count = sizeof(sm_catalog) / sizeof(sm_catalog[0]);", ""]
(ROOT / "components/schema_model/schema_catalog.inc").write_text("\n".join(lines))
