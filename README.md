# ZoleeSchemaMonitor

Önálló, animált Loxone gépészeti séma kijelző a Waveshare ESP32-P4-WIFI6-Touch-LCD-10.1 panelhez. A böngészős szerkesztőben legfeljebb tíz oldal tervezhető fűtési, hűtési, szellőzési, vízellátási, medence- és öntözőrendszerekhez.

**Állapot: MS1 fejlesztés alatt. Ez még nem lezárt, készüléken igazolt kiadás.** A működési webszerkesztő kipróbálható, a készülékes integráció és a végleges teljes grafikai készlet még nincs kész.

A **B, térhatásosabb grafikai irány kiválasztva**. Elkészült a működési webszerkesztő első változata, a 138 típust tartalmazó törzskatalógus és a hash szerinti grafikai tár gazdagépes referencia-megvalósítása. A natív firmware-ben már a helyi webszerkesztő, a készülékes HTTP API, a védett beállító Wi-Fi és a titkosított, kétpéldányos SD-mentés is benne van. A natív sémanézet és az adatolvasó felületre épített animációs motor is bekötve; a Loxone-kliens is bekötve. Valódi Miniserverrel a helyszíni ellenőrzés még szükséges, a firmware-OTA azonos grafikai készlettel bekötve, a közös firmware–grafika frissítés még hátravan. Élő kapcsolat nélkül a készülék ismeretlen értékeket jelez, nem szimulált működést. [Készülékes indítás](doc/keszulek.md).

A kipróbálható számítógépes előnézet indítása:

```sh
python3 tools/build_assets.py
python3 tools/dev_server.py --demo
```

Ezután: `http://127.0.0.1:8080/editor`. Az előnézetben az adatok szimuláltak. Python 3.12 és Node.js 22+ szükséges.

| Tulajdonság | Cél |
| --- | --- |
| Hardver | Waveshare ESP32-P4 10.1″, SKU 33150 |
| Adatkapcsolat | Közvetlen Loxone Gen2/17.x, Wi-Fi |
| Oldalak | Legfeljebb 10 |
| Felbontás | 1280×800 vagy 800×1280 |
| Tájolás | 0°, 90°, 180°, 270° |
| Animáció | 20 FPS célérték, készülékes igazolás szükséges |
| Nyelv | Magyar és angol |
| Grafika | Firmware által meghatározott készlet, SD-n tárolható erőforrások |

- [Dokumentáció](doc/README.md)
- [MS1 terv és elfogadási feltételek](doc/ms1.md)
- [Rögzített követelmények](doc/kovetelmenyek.md)
- [Felépítés és adatmodell](doc/architektura.md)
- [Fejlesztés, előnézet és tesztek](doc/fejlesztes.md)
- [Tényleges fejlesztési állapot](doc/allapot.md)
- [Verziótörténet](doc/verziotortenet/README.md)

Az alapul szolgáló projekt: [ZoleeEnergyFlowDisplay](https://github.com/ZoleeBB/ZoleeEnergyFlowDisplay).
