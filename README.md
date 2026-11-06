# ZoleeSchemaMonitor

Önálló, animált Loxone gépészeti séma kijelző a Waveshare ESP32-P4-WIFI6-Touch-LCD-10.1 panelhez. A böngészős szerkesztőben legfeljebb tíz oldal tervezhető fűtési, hűtési, szellőzési, vízellátási, medence- és öntözőrendszerekhez.

**Állapot: MS1 fejlesztés alatt. Ez még nem lezárt, készüléken igazolt kiadás.** A működési webszerkesztő kipróbálható, a készülékes integráció és a végleges teljes grafikai készlet még nincs kész.

A **B, térhatásosabb grafikai irány kiválasztva**. A webszerkesztő, a 333 típusú törzskatalógus, a natív animált nézet, a Loxone-kliens, az SD-mentés és a közös firmware–grafika frissítés bekötve. 72 típushoz 69 egyedi PNG-forrás már bekötve (szivattyúk, tárolók, hőcserélők és motoros szelepek); [készletnyilvántartás](doc/grafika/keszlet.md). A [2–16 körös termofejes osztók és a 90°-os elemforgatás](doc/osztogyujtok.md) is bekötve. A további egyedi grafikák, a valódi Miniserveres és készülékes próbák még szükségesek. Élő adat nélkül a készülék ismeretlen állapotot jelez. [Részletes állapot](doc/allapot.md) · [Készülékes indítás](doc/keszulek.md).

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
- [Mentés és visszaállítás](doc/mentes.md)
- [Diagnosztika és eseménynapló](doc/diagnosztika.md)
- [Tényleges fejlesztési állapot](doc/allapot.md)
- [Verziótörténet](doc/verziotortenet/README.md)

Az alapul szolgáló projekt: [ZoleeEnergyFlowDisplay](https://github.com/ZoleeBB/ZoleeEnergyFlowDisplay).
