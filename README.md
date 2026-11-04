# ZoleeSchemaMonitor

Önálló, animált Loxone gépészeti séma kijelző a Waveshare ESP32-P4-WIFI6-Touch-LCD-10.1 panelhez. A böngészős szerkesztőben legfeljebb tíz oldal tervezhető fűtési, hűtési, szellőzési, vízellátási, medence- és öntözőrendszerekhez.

**Állapot: MS1 fejlesztés alatt. Ez még nem lezárt, készüléken igazolt kiadás.** A teljes grafikai készlet stílusjóváhagyásra vár; a működési prototípus semleges helyőrzőket használhat.

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
- [Verziótörténet](doc/verziotortenet/README.md)

Az alapul szolgáló projekt: [ZoleeEnergyFlowDisplay](https://github.com/ZoleeBB/ZoleeEnergyFlowDisplay).
