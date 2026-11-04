# Alapozó ellenőrzések – 2026-11-04

Ez nem MS1-zárási jegyzőkönyv. A készülékes integráció hiányai az [állapotlapon](allapot.md) szerepelnek.

| Ellenőrzés | Helyi eredmény |
| --- | --- |
| Hordozható C-mag, GCC figyelmeztetések hibaként | Sikeres |
| JavaScript modell | 4 sikeres teszt |
| Python tárolási/API/kiadási tesztek | 9 sikeres teszt |
| Valódi Chromium | Sikeres: elem, cső, párosítás, teljesítményhatár, mentés/visszatöltés, export, álló vászon, HU/EN |
| SVG + PNG csomagolás | 138 elem metaadata, 553 manifestbejegyzés; közös tartalmak hash szerint deduplikálhatók |
| ESP-IDF 5.5.4, ESP32-P4 platformfirmware | Sikeres fordítás/linkelés |
| Platformfirmware bináris | 987136 bájt; a 8 MiB alkalmazáspartícióba belefér, 88% szabad |
| Fizikai panel / Miniserver / készülékes OTA | Nem végzett |
| Készülékes 20 FPS / tartós memóriahasználat | Nem mért |

A helyi buildkörnyezetben a folyamatok PID-névterét a komponenskezelő nem látta. Ezt kizárólag helyi Python-indítási kompatibilitási beállítással kezeltük; a repó forrásait és a managed komponenseket emiatt nem módosítottuk. A CI szabványos Ubuntu/ESP-IDF környezetet használ.

A böngészős teszt a tesztkiszolgálót maga indítja, külön ideiglenes adatfájllal, és leállításkor eltakarítja. Nem használ ügyféladatot.
