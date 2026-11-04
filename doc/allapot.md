# Fejlesztési állapot

Az MS1 **nyitott**, ez nem PR-zárási jegyzőkönyv. A célhardver SKU 33150; a teljes kiadás feltételeit az [MS1-lista](ms1.md) tartalmazza.

| Terület | Bekötött működés | Még szükséges |
| --- | --- | --- |
| Szerkesztő | 10 oldal, 150 elem/oldal; húzás, méretezés, forgatás, csövek/portok, szöveg/érték, import/export, undo, HU/EN | Használhatósági finomítás és készülékes regresszió |
| Modell és mentés | Közös JS/C ellenőrzés, revízióvédelem, titokmentes export, kétpéldányos CRC-s SD-állapot, AES-GCM beállítások | Teljes eszközmentés, kártya nélküli helyreállítás és SD újracsatolás |
| Natív nézet | PNG-k SHA/méret-ellenőrzéssel, csövek, szövegek, értékek, lapozás és diagnosztikaváltás | Sötét téma, háttérválasztás és pontos formázási egyezés, fizikai érintéspróba |
| Animáció | Irányhelyes csőpontok, rotoros üzemjelző, explicit aktivitás elsőbbsége, teljesítményből aktivitás/sebesség, ismeretlen állapot | Permet és további animációk, fizikai 20 FPS/memória mérés |
| Loxone | Egy worker: HTTP nyilvános kulcs, RSA/AES, JWT hitelesítés/megújítás, WS numerikus események, katalógus, web/native közös cache | Gen2/17.x fizikai bejelentkezési, TLS-, megszakadási és tartósüzemi próba; szöveges események |
| Grafika | B irány, 138 típus metaadata, közös világos/sötét SVG-családok és PNG-generálás | Típusonkénti végleges megkülönböztetés, teljes készlet felhasználói ellenőrzése |
| Frissítés | SHA-címzett, újrafelhasználható és tranzakciós grafikai tár gazdagépes referenciája | Készülékes grafikai/firmware OTA, kompatibilitás és rollback együtt |
| P4/web | PSRAM pufferjavítás, négy tájolás, Wi-Fi/mDNS, védett beállító AP, beágyazott web, valódi erőforrás-metrikák | Teljes átemelt diagnosztikai ablak, nappali/éjszakai témaváltás/fényerő |
| CI | Host JS/C/Python, valódi Chromium/LVGL, grafikai csomag, P4 build, main-merge kiadási folyamat | Minden MS1-elfogadási feltétel és fizikai regresszió |

## Loxone-kliens

A testvérprojekt `2dabb18fe493280a62068ab75b43a6313b9440a1` commitjának hitelesítési és olvasási transzportjára alapozunk, energia-, kaputelefon-, master/slave- és vezérlési függőségek nélkül. A worker a mentett profillal indul. A HTTP katalógus-fallback csak WS 404 után aktiválódik; TLS mellett a beállított CA ellenőrzése aktív, átirányítást nem követ. A felhasználónak alkalmazásstruktúra-hozzáférés szükséges.

A `/api/loxone/catalog`, `/api/live` és a natív adatolvasó azonos zárvédett RAM-állapotot használ. A cache 8192 UUID-s hash-tábla PSRAM-ban. Ismeretlen nem nulla, nem véges adat nem érvényes érték. Hibás csomag/betelt tár érvényteleníti az egész cache-t; új munkamenet törli a korábbi értékeket. A katalógus korlátja 512 KiB, 2048 állapot, nyolc beágyazási szint; a subControls hiányzó helyiség/funkció adatait a szülőtől örökli. Korláttúllépésnél nincs részleges katalógus. Parancs-UUID-k nem kerülnek a választóba.

Keepalive 15 s, válasz nélküli bontás 45 s, újrapróbálás 2/4/8/16/30 s. NTP segíti a TLS-időellenőrzést és a tokenmegújítás időzítését. Profilmentés után a meglévő kontrollált újraindítás alkalmazza a beállításokat. A webes nézet új kapcsolatnál újratölti a párosítható forrásokat. Nincs felhasználói vezérlőparancs-API, és a token nem kerül mentésbe vagy sémaexportba.

## Natív megjelenítés

A rajz és animáció nagy tárolói, valamint a Loxone-worker stackje PSRAM-ban vannak. A 32 KiB belső tartalék és a bevált kijelzőpufferek változatlanok. Az animáció 50 ms célütemmel, tényleges eltelt időből számol; megakadás után legfeljebb 200 ms-ot lép. Nincs SD/hálózat/JSON a mozgásciklusban, az alkalmazás fix tárolókat és statikus címkeszövegeket használ. Változatlan értéket nem ír újra; a diagnosztikai nézet szünetelteti az animációt.

## Ellenőrzési határok

- A gazdagépes cache-/protokoll-/HTTP-/LVGL-teszt és a P4-fordítás külön bizonyíték; egyik sem helyettesít valódi Miniservert vagy fizikai panelt.
- Ebben a munkamenetben nincs fizikai Miniserver-próba, tartósüzemi mérés vagy mért 20 FPS.
- A számítógépes `--demo` külön szimulált előnézet. A készülék nem szimulál adatot: kapcsolat nélkül ismeretlent mutat.
- A készülékes OTA nincs kész; telepítés USB-n, a grafikai csomag kézzel az SD-re másolandó.
- A CI host LVGL-je a P4-hez hasonlóan C könyvtári memóriafoglalót használ. A korábbi kis belső tesztheap miatti hibát reprodukáltuk és javítottuk.
