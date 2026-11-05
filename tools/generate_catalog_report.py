"""A típusszámot és a tényleges képkészültséget külön vezeti a dokumentációban."""
from collections import Counter
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('report_catalog', ROOT / 'catalog/elements.py')
catalog = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog)
items = catalog.elements()
ready = [item for item in items if 'artwork' in item]
counts = Counter(item['category'] for item in items)
finished = Counter(item['category'] for item in ready)
lines = ['# Grafikai készletnyilvántartás', '',
         f'**{len(items)} katalógustípus; {len(ready)} típushoz bekötött egyedi kép, '
         f'{len({item["artwork"] for item in ready})} külön PNG-forrásból.**', '',
         'A típusszám nem a kész egyedi képek száma. A többi típus műszaki helyőrzővel próbálható ki; '
         'az összes csoport végleges grafikája továbbra is MS1-feltétel.', '',
         '| Csoport | Típus | Egyedi képpel | Még helyőrző |', '| --- | ---: | ---: | ---: |']
for category, names in catalog.CATEGORIES.items():
    lines.append(f'| {names["hu"]} | {counts[category]} | {finished[category]} | {counts[category] - finished[category]} |')
lines += ['', 'A fűtési, szolár és általános keringető azonos fizikai szivattyúházat használhat. '
          'Ezt a három szerepet külön típusnév jelöli, a közös képet egyszer tároljuk.', '']
for category, names in catalog.CATEGORIES.items():
    lines += [f'## {names["hu"]}', '', '| Azonosító | Magyar név | English | Grafika |', '| --- | --- | --- | --- |']
    for item in items:
        if item['category'] != category:
            continue
        status = f'[Egyedi kép](../../assets/elements/universal/{item["artwork"]}.png)' if 'artwork' in item else 'Műszaki helyőrző'
        lines.append(f'| `{item["id"]}` | {item["name"]["hu"]} | {item["name"]["en"]} | {status} |')
    lines.append('')
lines += ['A nyilvántartást a `python3 tools/generate_catalog_report.py` frissíti a firmware-katalógusból.', '']
(ROOT / 'doc/grafika/keszlet.md').write_text('\n'.join(lines))
