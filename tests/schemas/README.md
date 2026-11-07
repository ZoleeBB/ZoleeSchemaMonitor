# MS1 próbasémák

A mappákban a `projekt.json` a teljes projekt, a `lap-N.json` egyetlen önállóan importálható oldal. Mind a 26 fájlt a szerkesztő exportgombjaival mentettük. Chromiumban mindegyiket visszaimportáltuk, majd az új exportot strukturálisan összehasonlítottuk az eredetivel. A tízoldalas projekt 1500 elemet tartalmaz. A teljes készlet 20 oldalon 2120 elemet fed le.

| Projekt | Oldal × elem | Leglassabb oldal P95, fekvő | Leglassabb oldal P95, álló |
| --- | ---: | ---: | ---: |
| Kazánház | 1 × 20 | 8,81 ms | 4,40 ms |
| Medence | 1 × 40 | 10,72 ms | 12,18 ms |
| Öntözés | 2 × 60 | 16,25 ms | 16,19 ms |
| Légtechnika | 2 × 60 | 16,92 ms | 14,02 ms |
| Padlófűtési osztók | 4 × 80 | 47,44 ms | 38,91 ms |
| Vegyes terhelés | 10 × 150 | 39,17 ms | 42,02 ms |

## Kipróbálás

Indítsd el a [fejlesztői szervert](../../doc/fejlesztes.md) a `--demo` kapcsolóval, majd a szerkesztő Import gombjával válassz fájlt. A próbák a demószerver generált numerikus jelét használják az aktivitáshoz, teljesítményhez, szelepálláshoz és kijelzett értékhez. Saját Miniserveren a kötéseket a saját jelforrásokhoz kell társítani.

A teljes böngészős export és visszaimport újra lefuttatható egy külön ideiglenes mappába. A helyi szerver már fusson. A Playwright és Chromium indítási változói azonosak a `tests/browser.mjs` változóival.

```sh
node tests/export_schemas.mjs http://127.0.0.1:8080 /tmp/schema-exports
```

## Gazdagépes LVGL-mérés

A 2026. november 7-i próba az éles C-rajzolót futtatta 1280×800 és 800×1280 felbontással, RGB565 módban, három teljes képpufferre foglalt hellyel, 64 soros részleges rajzpufferrel és 4 MiB LVGL képcache-sel. Az éles kód ellenőrizte a PNG-k méretét és SHA-ját, az LVGL szoftveresen rajzolt. A teszt minden oldalon indulást, változó értékeket, adatvesztést és visszatérést vizsgált, a kiírt értékmezőket is ellenőrizve. Oldalanként az első felépítés után 20 frissítésből számoltuk a P95-öt. A táblázat frissítési és rajzolási időt tartalmaz. A panel átviteli ideje és az érintés késleltetése külön mérendő.

| Erőforrás | Megfigyelt maximum | Viszonyítási keret |
| --- | ---: | ---: |
| Főfeladat megjelenítési útjának vereme | 4360 B | 10 240 B |
| LVGL rajzolási verem | 6520 B | 32 768 B |
| Linkelt rajzoló, modell és LVGL dinamikus foglalásai, pufferekkel | 17,35 MiB | 32 MiB |

A veremadat a Linux x86-64 hívási lánc festett vermének mélypontja. A heap-számláló a követett glibc-blokkok tényleges foglalását méri. A statikus adatok, hálózati feladatok, ESP-IDF, DMA és a belső RAM/PSRAM szerinti eloszlás a fizikai próbához tartoznak. Az 50 ms-os cél teljesülését a P4-en külön kell igazolni.

Az ismétléshez a [natív teszteknél](../../doc/fejlesztes.md) használt cJSON, Mbed TLS és LVGL 9.5.0 forrás szükséges. A gazdagépes LVGL Release build C flagjei legyenek `-DLV_CONF_SKIP=1 -DLV_USE_STDLIB_MALLOC=LV_STDLIB_CLIB -DLV_USE_LODEPNG=1 -DLV_CACHE_DEF_SIZE=4194304`. Állítsd be az `LVGL_SOURCE`, `LVGL_HOST_BUILD`, `CJSON_PATH` és `MBEDTLS_PATH` változókat.

```sh
python3 tools/build_assets.py --png --output /tmp/schema-graphics
bash tools/test_export_scenes.sh /tmp/schema-graphics
```
