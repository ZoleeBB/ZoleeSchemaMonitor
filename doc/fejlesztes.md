# Fejlesztői előnézet és tesztek

## Helyi webszerkesztő

Python 3.12 és Node.js 22 vagy újabb szükséges. A szerkesztőnek nincs npm futásidejű függősége.

```sh
python3 tools/build_assets.py
python3 tools/dev_server.py --demo
```

Nyisd meg a `http://127.0.0.1:8080/editor` címet. A `--demo` kizárólag szimulált, azonosítóval párosítható értéket ad; nélküle nincs élő adat. A fejlesztői kiszolgáló **nem kapcsolódik Miniserverhez**, nem frissít firmware-t, és minden hardvermetrikát nem mértnek jelöl.

A szerver alapból csak a helyi gépen figyel. A `.dev-data/` tartós helyi adatot tárol, és nem kerül Gitbe. Az importot a böngésző és a kiszolgáló ugyanazzal a validátorral ellenőrzi. A projekt revíziószáma megakadályozza a másik böngésző változtatásainak csendes felülírását.

A kijelölt cső vastagsága a Tulajdonságok → Csővastagság mezőben csövenként 1–30 kijelzőpixel között állítható. A mentett értéket a webes és a natív rajzoló is használja. A [csatlakozópont-audit](grafika/csatlakozopont-audit.md) külön rögzíti a látható és a takart csatlakozásokat; kalibrálás után a kötött csővégek automatikusan az új pontokra illeszkednek.

## Tesztek

```sh
npm test
python3 -m unittest discover -s tests -p 'test_*.py' -v
gcc -std=c11 -Wall -Wextra -Werror -Icomponents/schema_core/include components/schema_core/schema_core.c tests/core_test.c -lm -o /tmp/schema-core-test
/tmp/schema-core-test
```

A [hat importálható próbaséma, a 20 oldalexport és a veremmérés](../tests/schemas/README.md) a demószerverrel vagy a natív LVGL-rajzolóval próbálható ki.

## Grafikai generálás

A forráskatalógus `catalog/elements.py`, az egységes vektoros forrásgenerátor `tools/build_assets.py`. A `web/catalog.json` és a `components/schema_model/schema_catalog.inc` generált fájlok, kézzel nem szerkesztendők. Katalógusváltozáskor a `python3 tools/generate_catalog.py` parancsot is futtasd. A generált SVG-k nem kerülnek külön Gitbe; a fejlesztői indítás és a CI előállítja őket.

PNG csomaghoz CairoSVG szükséges:

```sh
python3 -m pip install -r tools/requirements-assets.txt
python3 tools/build_assets.py --png --output dist/graphics
```

A megjelenítő a statikus képet és az animáció mozgó részét külön kezeli. A katalógus animációs középpontot és csatlakozópontokat is tárol. A kezdeti közös rajzcsaládok nem bizonyítják automatikusan minden berendezés készüléken tesztelt grafikai viselkedését.

## Készülékes C-kód gazdagépes vizsgálata

```sh
IDF_PATH=/az/esp-idf/helye MBEDTLS_PATH=/az/esp-idf/helye/components/mbedtls/mbedtls python3 tools/test_native.py
```

A teszt az IDF saját cJSON és Mbed TLS forrásaival fordítja az éles `schema_model`, `app_store`, `app_web`, `app_ota`, `app_assets` és `app_release` C-kódot. A BSP/NVS/FreeRTOS és a HTTP socket réteget helyettesíti; a JSON-ellenőrzés, a CRC, az AES-GCM, a fájlműveletek, a titokszűrés, a revízióvédelem és az API-kezelők az éles kódból futnak. A CI ugyanazokat a pontos függőség-commitokat használja. Ez nem Wi-Fi-driver- vagy fizikai SD-teszt.

## Natív LVGL-próba

A managed LVGL 9.5.0 forrásból készíts gazdagépes buildet `LV_CONF_SKIP=1` fordítási definícióval, kikapcsolt demókkal/példákkal. Ezután:

```sh
IDF_PATH=/az/esp-idf/helye LVGL_HOST_BUILD=/az/lvgl/host/build bash tools/test_scene.sh
```

Ez az éles `app_scene.c` rajzolót futtatja valódi LVGL objektumokon, virtuális kijelzővel; SD-PNG dekódolást és fizikai panelt nem helyettesít.


A gazdagépes LVGL build és a teszt **azonos** `-DLV_CONF_SKIP=1 -DLV_USE_STDLIB_MALLOC=LV_STDLIB_CLIB` fordítási definíciókat használjon. A PNG-képes CI-buildhez a CMake és a `tools/test_scene.sh` azonos `LVGL_HOST_CFLAGS` értéket kap. A `-DLV_USE_LODEPNG=1 -DLV_CACHE_DEF_SIZE=4194304` további definíciókkal a hat exportált projekt is lefut a csomagoláskor előállított grafikákkal. A futás 40 oldal- és felbontáskombinációjának eredménye a `schema-scene-metrics` CSV-artefaktumban található. A P4 konfigurációja szintén a C könyvtár memóriafoglalóját választja. Az alapértelmezett kis belső LVGL-heap a 150 elemes animációs próbához nem megfelelő, és nem tükrözi a firmware működését. A CI ezért ezt explicit rögzíti; korábbi buildkönyvtár helyett szükség esetén tiszta host build készüljön.

## P4-fordítás és frissítőcsomag

ESP-IDF 5.5.4 alatt a CI-vel azonos `idf-component-manager==2.5.2` verzió használatos. Az exportált IDF-környezetben `python -m pip install idf-component-manager==2.5.2`, majd `idf.py build` futtatható.

A firmware csomagolása előtt `python3 tools/build_assets.py --png --output dist/graphics --version <firmware-verzió>` és `python3 tools/package_preview.py` szükséges. A firmware-verzió a `build/project_description.json` fájlban látható. Ezután `python3 tools/package_firmware.py` ellenőrzi a verzió- és készletegyezést, és létrehozza az `update.json` leírót is. A CI a saját ellenőrzési jobjának manifestjét adja át a firmware-jobnak; más futás csomagját nem használja.
