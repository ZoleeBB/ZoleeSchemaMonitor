# Diagnosztika

A kijelző **Info / Séma** gombja vált a rajz és a diagnosztika között. Üres első projektnél a diagnosztika az indulónézet. A panel és a web ugyanannak a mintavételnek az adatait mutatja. A nézet HU/EN nyelven működik; a panel a beállított világos/sötét témát és mind a négy tájolás méreteit követi. A kártyák álló módban két, fekvő módban három oszlopba rendeződnek. A panel eseménylistája függőlegesen görgethető.

## Kijelzett adatok

| Terület | Jelentés |
| --- | --- |
| CPU0 / CPU1 | A két minta közti idle-időből számolt foglaltság, legalább 2 s-os ablakban. Az első teljes ablak előtt ismeretlen, nem 0%. |
| Belső RAM | Szabad / teljes 8 bites belső heap, külön a bekapcsolás óta mért legkisebb szabad érték. |
| PSRAM | Szabad / teljes külső memória. A belső RAM-tól elkülönített keret. |
| DMA | A belső DMA-képes heap szabad mérete és legnagyobb összefüggő blokkja. |
| SD | Szabad / teljes kapacitás és a korábbi érvényes állapotból történt helyreállítás jelzése. Sikertelen mérésnél nincs foglaltsági sáv. |
| Wi-Fi / web | Állapot, SSID, IPv4/webcím és kapcsolódáskor RSSI. |
| Loxone | Közvetlen kapcsolat, az utolsó számszerű eseménycsomag kora és az ismert, érvényes cache-értékek száma. |
| Rendszer | Firmware-verzió, üzemidő és a falióra érvényessége. Az utóbbi nem külön NTP-protokollmérés. |
| Feladatok / verem | FreeRTOS feladatszám és **az app_main** legkisebb szabad vermének mérete bájtban. Nem az összes task veremlistája. |
| Kiadásfrissítés | A GitHub-kiadási worker fázisa, verziója és átvett bájtjai. A kézi bináris feltöltés kezdete/vége az események között látható. |
| Beállító Wi-Fi | Aktivitás és SSID; a jelszó kizárólag a fizikai kijelzőn jelenik meg, a mintavételi task hibája mellett is. |

A memória- és SD-sávok a használt hányadot mutatják; a felirat a szabad / teljes mennyiséget adja. A sáv 75%-tól figyelmeztető, 90%-tól piros. A magas foglaltság önmagában nem összeomlásjelzés; a minimum és a legnagyobb DMA-blokk is lényeges.

A Loxone eseményvezérelt: változatlan értéknél lehet régi a számszerű csomag, miközben a kapcsolat él és keepalive érkezik. A csomagkora ezért **nem** kapcsolatvesztési időkorlát. Új munkamenet törli a korábbi adatkor és értéktár érvényességét. Az ismeretlen mérés és a valóban nulla más állapot.

## Mintavétel és napló

A `schema_diag` task 8 KiB-os PSRAM-stackkel, másodpercenként mintáz. Az SD-kapacitást legfeljebb tíz másodpercenként olvassa; kijelzőzár alatt nincs fájlrendszeri vagy hálózati munka. A rajzoló és a HTTP-kérés rövid, zárvédett RAM-pillanatképet olvas. Elakadt vagy késő mintavételnél három másodperc fölött látható a minta kora, az utolsó adatok megmaradnak. A web nem helyettesíti azokat nullákkal.

A 15 bejegyzéses RAM-napló a legújabb eseményt mutatja elöl. Az időbélyeg a bekapcsolás óta eltelt idő, nem helyi falióra; NTP-időkorrekció nem rendezi át. Újraindításkor a napló kiürül. Boot, Wi-Fi és Loxone kapcsolatváltás, beállító AP, SD-indulás/helyreállítás/íráshiba, frissítés és alacsony memória/verem rögzül. A memória-mélypont 8 KiB, az app_main verem 1 KiB alá kerülése egyszeri figyelmeztetés; ezek fejlesztői küszöbök, fizikai kalibráció még szükséges.

A napló rögzített kódokat és előre megadott HU/EN szövegeket használ. Nincs szabadon formázott felhasználói szöveg, URL, jelszó vagy token. A webes pillanatkép hálózati adatokat (SSID, helyi IP) tartalmaz; a beállító Wi-Fi kulcsát nem tartalmazza.

## Korlátok és ellenőrzés

- A `/api/diagnostics` csak olvas; a még nem mért számok JSON `null` értéket kapnak. A fejlesztői kiszolgáló sem szimulál mért hardvermetrikát.
- A `sdMounted` a regisztrált állapottár jelzője, nem kártyakivétel-érzékelő. A `sdSpaceValid` a legutóbbi kapacitásmérés eredménye; automatikus SD-újracsatolás továbbra sincs.
- A CPU idle-időből származik; a fizikai kijelző FPS-e nem mért. A 20 FPS továbbra is cél.
- A panel és a web csak változott szöveget/sávértéket ír át. A panel rejtett diagnosztikája nem frissíti az LVGL mezőit, megnyitáskor az utolsó pillanatképet mutatja.
- Függő OTA-verzió véglegesítéséhez a diagnosztikai tasknak is el kell indulnia, és legfeljebb 10 s-os mintának rendelkezésre kell állnia. A 30/180 s-os bootellenőrzési szabály változatlan.
- A host teszt a CPU-ablakot, cache-t, SD-mérési ritkítást/hibát, mintaéletkort, memória/verem figyelmeztetést, körnaplót és titokmentes JSON-t vizsgálja. Valódi LVGL-lel ellenőrzött a két elrendezés, sávok, téma/nyelv, rejtett/változatlan nézet és a csak panelen megjelenő kulcs. Ezek nem fizikai panel-, SD-, FreeRTOS-időzítési vagy 20 FPS-mérések.
