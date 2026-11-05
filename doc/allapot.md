# Fejlesztési állapot

Az MS1 **nyitott**, ez nem PR-zárási jegyzőkönyv. A célhardver SKU 33150; a teljes kiadás feltételeit az [MS1-lista](ms1.md) tartalmazza.

| Terület | Bekötött működés | Még szükséges |
| --- | --- | --- |
| Szerkesztő | 10 oldal, 150 elem/oldal; húzás, méretezés, forgatás, csövek/portok, szöveg/érték, import/export, undo, HU/EN | Használhatósági finomítás és készülékes regresszió |
| Modell és mentés | Közös JS/C ellenőrzés, revízióvédelem, titokmentes export, kétpéldányos CRC-s SD-állapot, AES-GCM beállítások | Teljes eszközmentés, kártya nélküli helyreállítás és SD újracsatolás |
| Natív nézet | PNG-k SHA/méret-ellenőrzéssel, csövek, szövegek, értékek, lapozás és diagnosztikaváltás | Pontos formázási egyezés, fizikai érintéspróba |
| Animáció | Irányhelyes csőpontok, rotoros üzemjelző, explicit aktivitás elsőbbsége, teljesítményből aktivitás/sebesség, ismeretlen állapot | Permet és további animációk, fizikai 20 FPS/memória mérés |
| Loxone | Egy worker: HTTP nyilvános kulcs, RSA/AES, JWT hitelesítés/megújítás, WS numerikus események, katalógus, web/native közös cache | Gen2/17.x fizikai bejelentkezési, TLS-, megszakadási és tartósüzemi próba; szöveges események |
| Grafika | B irány, 138 típus metaadata, közös világos/sötét SVG-családok és PNG-generálás | Típusonkénti végleges megkülönböztetés, teljes készlet felhasználói ellenőrzése |
| Frissítés | Streamelt firmware OTA azonos grafikai készlettel, inaktív partíció, IDF-validálás és indulási rollback; SHA-címzett grafikai tár gazdagépes referenciája | Készülékes differenciális grafikai frissítés, közös aktiválás/rollback és fizikai megszakításpróba |
| P4/web | PSRAM pufferjavítás, négy tájolás, Wi-Fi/mDNS, védett beállító AP, beágyazott web, valódi erőforrás-metrikák | Teljes átemelt diagnosztikai ablak és fizikai fényerőpróba |
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
- A firmware-OTA csak azonos grafikai forrásazonosítóval használható; első telepítés és grafikai változás USB + kézi SD-csomag. A közös firmware–grafika frissítés még nincs kész.
- A CI host LVGL-je a P4-hez hasonlóan C könyvtári memóriafoglalót használ. A korábbi kis belső tesztheap miatti hibát reprodukáltuk és javítottuk.


## Nappali/éjszakai megjelenítés

A `nightSource` numerikus Loxone-párosítás pozitív értéke éjszakai módot választ; nulla vagy negatív értéke nappalit. Induláskor nappali a biztonságos alap. Ismeretlen jel esetén az utolsó ismert téma és fényerő marad, nem vált vissza nappalra. A nappali/éjszakai fényerőt a mentett beállítások adják, a BSP csak változáskor kap új értéket, a kijelzőzáron kívül. A 0%-os éjszakai fényerő szándékosan is választható.

A natív nézet a megfelelő `graphics/light/` vagy `graphics/dark/` PNG-t tölti be, a manifest méret- és SHA-adataival ellenőrizve. Azonos hash és méret esetén az aktuális oldal elemei egy PNG-puffert és egy LVGL cache-forrást osztanak meg; felszabadításkor csak a tulajdonos szabadít. A meglévő oldal az új képek előtöltése alatt megmarad, SD-művelet nincs a kijelzőzár alatt.

A sima, hűvös és meleg háttér színei a szerkesztő palettáját követik. A natív átmenet függőleges, a böngészőé átlós; a sima háttérnél nincs átmenet. A készülék nyelvbeállítása az Info/Séma feliratot és a hiányzó grafika helyén megjelenő elemnevet is választja. Téma-, nyelv- vagy projektváltozáskor újraépül az oldal, változatlan állapotnál nem.

A virtuális LVGL-próba a háttérstílusokat, a nappali/éjszakai módot és fényerőhívásokat, az ismeretlen jel megtartását, a nyelvet és a megosztott PNG-forrást ellenőrzi. Ez nem a fizikai panel fényerejének, a tényleges SD-PNG dekódolásnak vagy a 20 FPS-nek a mérése.

## Firmware OTA – első készülékes megvalósítás

A webes Frissítés oldal binárisan streameli a firmware-t a nem futó 8 MiB-os partícióra, 4 KiB-os munkapufferrel. Az első törlés előtt ellenőrzi az image/leíró jelét, a projektnevet és a saját grafikai forrásazonosítót. Az IDF véglegesíti a teljes bináris és hash, chip és revízió ellenőrzését; csak siker után állít bootcélt. A csomagoló a tényleges bináris 288. bájtjától ellenőrzi az egyéni leírót. A grafikai azonosító jelenleg a katalógus és a generátor forrásának SHA-256 lenyomata: konzervatív kompatibilitási korlát, nem aláírás vagy az SD tartalmának ellenőrzése.

Sikertelen stream/flash írás abortál, hibás végellenőrzés nem állít bootcélt. Újrapróbálás lehetséges; siker után az újraindulásig további feltöltés nem indulhat. Hibás feltöltésnél az HTTP-kapcsolat lezárul az olvasatlan törzs miatt. A Host/Origin/egyedi fejléc ellenőrzés a bináris végpontra is vonatkozik.

A függő új alkalmazás 30 s uptime után véglegesíthető, ha a web és az SD-állapottár elérhető; Loxone-adatkapcsolat nem feltétel. 180 s után sikertelen ellenőrzésnél visszaállást kér. Kijelzőindítási hiba azonnal sikertelen indulásnak számít. Összeomlás/újraindulás esetén az IDF bootloader függő verzióra vonatkozó rollback-szabálya érvényes. Ez nem teljes rendszer-watchdog: egy beragadt, újra nem induló taskot ez a pollalapú ellenőrzés nem indít újra. Fizikai áramelvételes/rollback-próba még szükséges.
