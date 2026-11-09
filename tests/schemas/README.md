# MS1 próbasémák

A mappákban a `projekt.json` a teljes projekt, a `lap-N.json` egyetlen önállóan importálható oldal. Mind a 26 fájlt a szerkesztő exportgombjaival mentettük. Chromiumban mindegyiket visszaimportáltuk, majd az új exportot strukturálisan összehasonlítottuk az eredetivel. Mind a hat projektet az éles C-állapottárba is elmentettük. Új folyamatból betöltve a revízió és a teljes projekttartalom megmaradt. A normalizált koordinátáknál 10⁻¹⁵ abszolút tűréssel ellenőriztük a cJSON számainak visszaolvasását. A tízoldalas projekt 1500 elemet tartalmaz. A teljes készlet 20 oldalon 2120 elemet fed le.

| Projekt | Oldal × elem | Leglassabb oldal P95, fekvő | Leglassabb oldal P95, álló |
| --- | ---: | ---: | ---: |
| Kazánház | 1 × 20 | 15,63 ms | 6,14 ms |
| Medence | 1 × 40 | 15,16 ms | 6,14 ms |
| Öntözés | 2 × 60 | 22,52 ms | 9,41 ms |
| Légtechnika | 2 × 60 | 23,47 ms | 13,95 ms |
| Padlófűtési osztók | 4 × 80 | 53,39 ms | 29,73 ms |
| Vegyes terhelés | 10 × 150 | 113,71 ms | 29,54 ms |

## Képernyőképek

Mindkét kép ugyanazon projekt első oldalát és a 65%-os demójelet mutatja. A szerkesztőkép Chromiumban készült. A kijelzőkép a firmware LVGL-rajzolójának 1280×800 pixeles RGB565 képpufferéből származik.

| Projekt | Webszerkesztő | Kijelző |
| --- | --- | --- |
| Kazánház | [Szerkesztő](kazan/szerkeszto.jpg) | [LVGL](kazan/kijelzo.jpg) |
| Medence | [Szerkesztő](medence/szerkeszto.jpg) | [LVGL](medence/kijelzo.jpg) |
| Öntözés | [Szerkesztő](ontozes/szerkeszto.jpg) | [LVGL](ontozes/kijelzo.jpg) |
| Légtechnika | [Szerkesztő](legtechnika/szerkeszto.jpg) | [LVGL](legtechnika/kijelzo.jpg) |
| Padlófűtési osztók | [Szerkesztő](osztok/szerkeszto.jpg) | [LVGL](osztok/kijelzo.jpg) |
| Vegyes terhelés | [Szerkesztő](terheles/szerkeszto.jpg) | [LVGL](terheles/kijelzo.jpg) |

## Csőáramlási próba

Az [önállóan importálható próbaoldal](csoaramlas/aramlasi-proba.json) hat szivattyú és szelep között mutatja az előre és visszafelé haladó áramlást. A csövek 1, 4, 8, 12, 20 és 30 kijelzőpixel vastagok. A demószerver egyetlen aktivitásjele mind a hat kört működteti. A 7,5 másodperces felvételen a jel először aktív, majd nulla, megszakad a kapcsolat, végül visszatér az aktív jel. A képkockák a gazdagépen futtatott firmware LVGL-képpufferéből származnak.

[Szerkesztőkép](csoaramlas/szerkeszto.jpg) · [Kijelzőkép](csoaramlas/kijelzo.jpg) · [Áramlási animáció](csoaramlas/aramlas.mp4)

A [szerkesztő és a kijelző összehasonlító felvételén](animaciok/osszehasonlitas.mp4) azonos jellel, 50 ms-os időlépésekben látszik a keringető, a kétventilátoros hőszivattyú, a háromventilátoros szárazhűtő, a csőventilátor, a szórófej és a motoros háromjáratú szelep. A felső sor a webszerkesztőből, az alsó a firmware LVGL-képpufferéből származik.

## Animált eszközök egy oldalon

Az [importálható animációs projekt](animaciok/projekt.json) 68 különböző készüléket mutat egy oldalon, köztük az összes animációval rendelkező katalógustípust és egy állítható osztó-gyűjtőt. A harmadik sor bal szélén a kétventilátoros tetőhűtő látható. A forgástengelyek igazítása a két ventilátoragyhoz a demószerver változó jelével ellenőrizhető. Ez a projekt is bekerül a két felbontáson futó LVGL-próbába. A [hat másodperces kijelzőfelvétel](animaciok/kijelzo.mp4) a fekvő LVGL-képpufferből készült. A 75%-os demójelet leállás, kapcsolatvesztés és 95%-os újraindulás követi.

[Szerkesztőkép 75%-os jellel](animaciok/szerkeszto.jpg) · [Kijelzőkép azonos jellel](animaciok/kijelzo.jpg)

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
| Főfeladat megjelenítési útjának vereme | 3432 B | 10 240 B |
| LVGL rajzolási verem | 6520 B | 32 768 B |
| Linkelt rajzoló, modell és LVGL dinamikus foglalásai, pufferekkel | 15,12 MiB | 32 MiB |

A fenti mérés a széles, moduláris osztóképekkel készült, külön futtatott gazdagépes tesztben.

A veremadat a Linux x86-64 hívási lánc festett vermének mélypontja. A heap-számláló a követett glibc-blokkok tényleges foglalását méri. A statikus adatok, hálózati feladatok, ESP-IDF, DMA és a belső RAM/PSRAM szerinti eloszlás a fizikai próbához tartoznak. Az 50 ms-os cél teljesülését a P4-en külön kell igazolni.

Az ismétléshez a [natív teszteknél](../../doc/fejlesztes.md) használt cJSON, Mbed TLS és LVGL 9.5.0 forrás szükséges. A gazdagépes LVGL Release build C flagjei legyenek `-DLV_CONF_SKIP=1 -DLV_USE_STDLIB_MALLOC=LV_STDLIB_CLIB -DLV_USE_LODEPNG=1 -DLV_CACHE_DEF_SIZE=4194304`. Állítsd be az `LVGL_SOURCE`, `LVGL_HOST_BUILD`, `CJSON_PATH` és `MBEDTLS_PATH` változókat.

```sh
python3 tools/build_assets.py --png --output /tmp/schema-graphics
bash tools/test_export_scenes.sh /tmp/schema-graphics
```
