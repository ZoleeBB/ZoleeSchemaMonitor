# Fejlesztői előnézet és tesztek

## Helyi webszerkesztő

Python 3.12 és Node.js 22 vagy újabb szükséges. A szerkesztőnek nincs npm futásidejű függősége.

```sh
python3 tools/build_assets.py
python3 tools/dev_server.py --demo
```

Nyisd meg a `http://127.0.0.1:8080/editor` címet. A `--demo` kizárólag szimulált, azonosítóval párosítható értéket ad; nélküle nincs élő adat. A fejlesztői kiszolgáló **nem kapcsolódik Miniserverhez**, nem frissít firmware-t, és minden hardvermetrikát nem mértnek jelöl.

A szerver alapból csak a helyi gépen figyel. A `.dev-data/` tartós helyi adatot tárol, és nem kerül Gitbe. Az importot a böngésző és a kiszolgáló ugyanazzal a validátorral ellenőrzi. A projekt revíziószáma megakadályozza a másik böngésző változtatásainak csendes felülírását.

## Tesztek

```sh
npm test
python3 -m unittest discover -s tests -p 'test_*.py' -v
gcc -std=c11 -Wall -Wextra -Werror -Icomponents/schema_core/include components/schema_core/schema_core.c tests/core_test.c -lm -o /tmp/schema-core-test
/tmp/schema-core-test
```

## Grafikai generálás

A forráskatalógus `catalog/elements.py`, az egységes vektoros forrásgenerátor `tools/build_assets.py`. A `web/catalog.json` generált fájl, kézzel nem szerkesztendő. A generált SVG-k nem kerülnek külön Gitbe; a fejlesztői indítás és a CI előállítja őket.

PNG csomaghoz CairoSVG szükséges:

```sh
python3 -m pip install -r tools/requirements-assets.txt
python3 tools/build_assets.py --png --output dist/graphics
```

A megjelenítő a statikus képet és az animáció mozgó részét külön kezeli. A katalógus animációs középpontot és csatlakozópontokat is tárol. A kezdeti közös rajzcsaládok nem bizonyítják automatikusan minden berendezés készüléken tesztelt grafikai viselkedését.
