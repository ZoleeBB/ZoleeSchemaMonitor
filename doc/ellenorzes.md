# Alapozó ellenőrzések – 2026-11-05

Ez nem MS1-zárási jegyzőkönyv. A készülékes integráció hiányai az [állapotlapon](allapot.md) szerepelnek.

| Ellenőrzés | Helyi eredmény |
| --- | --- |
| Hordozható C-mag, GCC figyelmeztetések hibaként | Sikeres |
| JavaScript modell | 4 sikeres teszt |
| Python tárolási/API/kiadási tesztek | 9 sikeres teszt |
| Valódi Chromium | Sikeres: elem, cső, párosítás, teljesítményhatár, mentés/visszatöltés, export, álló vászon, HU/EN |
| SVG + PNG csomagolás | 317 elem metaadata, 1239 manifestbejegyzés; közös tartalmak hash szerint deduplikálhatók |
| ESP-IDF 5.5.4, ESP32-P4 platformfirmware | Sikeres fordítás/linkelés |
| Platformfirmware bináris | OTA-val 1 693 376 bájt; a 8 MiB alkalmazáspartícióban 80% szabad |
| Natív JS/C modellparitás | 22 eset, köztük a 10×150 elemes projekt |
| Natív SD-állapot, valódi AES-GCM | Tartós mentés, konfliktus, jelszórejtés, sérülés utáni visszaállás, más kulcs, SD-hiba sikeres |
| Natív HTTP-kezelő | Beágyazott szerkesztő, import/export, CAS, Host/Origin, titokszűrés sikeres; socket helyettesítve |
| Fizikai panel / Miniserver / készülékes OTA | Nem végzett |
| Készülékes 20 FPS / tartós memóriahasználat | Nem mért |

A helyi buildkörnyezetben a folyamatok PID-névterét a komponenskezelő nem látta. Ezt kizárólag helyi Python-indítási kompatibilitási beállítással kezeltük; a repó forrásait és a managed komponenseket emiatt nem módosítottuk. A CI szabványos Ubuntu/ESP-IDF környezetet használ.

A böngészős teszt a tesztkiszolgálót maga indítja, külön ideiglenes adatfájllal, és leállításkor eltakarítja. Nem használ ügyféladatot.


Natív animációs motor: valódi LVGL-lel, virtuális kijelzőn sikeres az explicit aktivitás elsőbbsége, szigorú teljesítményküszöb, minimális sebesség, fordított csőirány, forgó üzemjelző, százalékkijelzés, skálázott hőmérséklet és adatvesztés kezelése. Az értékeket a teszt RAM-adatolvasója adja; ez nem Miniserver- vagy fizikai FPS-próba. A korábbi statikus nézet tesztjei szintén megmaradtak.


Loxone-kliens bekötése: sikeres helyi ESP-IDF 5.5.4 P4-fordítás/linkelés, kb. 1,68 MB alkalmazás (8 MiB OTA-partíció, 80% szabad). A host cache-próba ellenőrzi az ismeretlen/nulla különbségét, felülírást, NaN-kezelést, csonka csomagot, munkamenet-törlést és a 8192 helyes tár túltöltésének teljes érvénytelenítését. A HTTP API teszt a Loxone backend helyettesítőjét használja; az RSA/JWT/WebSocket hálózati hitelesítést még nem vizsgálja valódi Miniserverrel. Ilyen helyszíni próba ebben a munkamenetben nem történt.


Nappali/éjszakai nézet: a valódi LVGL-objektumokon futó host próba ellenőrzi a palettát, a sima háttér átmenetmentességét, a módváltást, a 85/15%-os BSP-hívásokat, a változatlan állapotnál elmaradó fényerőírást/újraépítést, az ismeretlen éjszakai jel megtartását, az angol feliratot és a hash+méret szerinti PNG-megosztást. A BSP és az SD fájlok helyettesítettek; fizikai fényerő- és PNG-dekódolási próba továbbra sem történt.

Firmware OTA: az éles C-kezelő host tesztje 7 bájtos streamdarabokkal bájtpontos írást, eltérő projekt/grafika és hibás fejléc törlés előtti elutasítását, túlméretet/csonka prefixet, megszakítást, újrapróbálást, begin/write/end/bootcél hibákat és párhuzamos feltöltés tiltását ellenőrzi. Külön vizsgálja a 30/180 s-os véglegesítési/rollback döntést, a véglegesítési hiba utáni visszaállást és a már érvényes alkalmazás változatlanságát. Az SDK flash/hash/chip ellenőrzése és a bootloader helyettesítve van; ezek működését a host teszt nem bizonyítja. A HTTP-próba az OTA bináris fejlécvédelmét és az elutasításkor kért kapcsolatlezárást is ellenőrzi.

Az OTA-s változat helyi ESP-IDF 5.5.4 fordítása és bináris csomagolása sikeres: 1 693 376 bájt. A csomagoló a valódi binárisban is megtalálta a 288. bájtnál a kompatibilitási leírót. Külön, git archívumból készített munkakönyvtárban, előzetes sdkconfig/build/managed_components nélkül is sikeres a konfigurálás `idf-component-manager==2.5.2` mellett. A CI első kísérlete a konfigurálásban három USB-manifest Kconfig-változót hiányolt a rögzített ESP-Hosted 1.4.7 mellett; ezért a pipeline azonos, ellenőrzött csomagkezelő-verziót telepít. A panel/SDIO konfigurációja emiatt nem változik.

Közös grafikai/firmware frissítés: az éles C SD-tárat valódi fájlműveletekkel és Mbed TLS SHA-256-tal vizsgáljuk. A próba rövid olvasásokat, hibás hash-t/méretet, megszakítást, `fsync` hibát, sérült blob/manifest javítását, közös fájlok újrafelhasználását, változatlan régi kötést és az aktuális firmware készletválasztását ellenőrzi. A kiadási worker külön próbája éles HTTP-vezérlőkóddal, helyettesített socket/TLS/flash réteggel fut: engedélyezett/tiltott átirányítás, chunked törzs, csonka válasz, grafikai és firmware-hash, firmware-azonosító, párhuzamos indítás tiltása, újrapróbálás és az újraindítás sorrendje. A firmware OTA saját natív próbája ellenőrzi az új készlet hiányának elutasítását és a külső hash-ellenőrzés hibája után változatlan bootcélt is.

A Chromium-próba helyettesített frissítési API-val vizsgálja a közös frissítés indítását, a gombok zárolását, a folyamatállapotot és a hiba utáni újrapróbálhatóságot HU/EN nyelven. Ez nem valódi GitHub-letöltés, SD-hardver vagy flash-próba. A teljes kiadási lánc és a fizikai áramelvétel továbbra is kipróbálandó.

A közös frissítőt tartalmazó helyi P4 build és csomagolás sikeres, körülbelül 1,78 MB (8 MiB-os partíció, 79% szabad). A teljes SVG/PNG-csomaggal a firmware csomagolása és az `update.json` létrehozása is sikeres; a leíró firmware/manifest méretet és SHA-256-ot, valamint grafikai forrásazonosítót tartalmaz.

Diagnosztika: az éles C-mintavétel és JSON tesztje ellenőrzi a CPU első ismeretlen állapotát, 2 s-os ablakát és clampelését, az API SD/CPU-olvasás nélküli cache-ét, tíz másodperces SD-lekérdezést és annak hibáját, a minta korát, egyszeri memória/verem figyelmeztetést, 15 soros körnaplót, taskindítás hibáját és a jelszó nélküli JSON-t. Valódi LVGL-próba igazolja az ismeretlen sávok elrejtését, százalékokat, változatlan/rejtett nézetnél elmaradó írást, HU/EN és téma váltását, álló oszlopméretet, naplót és a csak panelen megjelenő AP-kulcsot. A platformmetrikák és a BSP helyettesítettek; fizikai diagnosztikai és érintéspróba még szükséges.

## 2026-11-06: vizuális audit, osztók és negyedfordulat

14 JavaScript- és 31 Python-teszt sikeres. A natív C/JS modell 214 esetben azonos eredményt ad, az új esetek minden 2–16 körszámot és üzemmódot, a 34. portot és a hibás konfigurációkat is lefedik. A gazdagépen valódi LVGL 9.5.0-val futó jelenetteszt ellenőrzi a 16 külön kör jelzését, a kapcsolatvesztést és mind a négy forgatást. A Chromium 133-as helyi próbában a 15 körszám, a 34. port csőbekötése, a körönkénti forrás, a nulla/pozitív/hiányzó állapot, a csőleválasztás és a mentés/visszatöltés sikeres. A készülék HTTP-kezelőjén a regisztrált PNG-k megfelelő `image/png` típussal, az SVG-k továbbra is `image/svg+xml` típussal elérhetők; ezt a natív HTTP-teszt ellenőrzi.

Ezek nem fizikai paneles vagy valódi Miniserveres mérések. Új helyi ESP-IDF firmware-fordítás ezen a körön nem történt; a CI eredménye külön kezelendő. Az MS1 és a 20 FPS fizikai igazolása továbbra is nyitott.
