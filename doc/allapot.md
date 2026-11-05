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
| Frissítés | Natív SHA-címzett SD-tár, kiadási HTTPS worker, csak hiányzó/hibás fájlok, közös firmware/grafika ellenőrzés és firmware-hez kötött készletválasztás | Első valódi kiadásból végzett hálózati és fizikai megszakítás/rollback próba; tárkarbantartás |
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
- A közös frissítő az első tényleges GitHub-kiadás után használható online. Az MS1 CI-artefaktum nem kiadás. Az első telepítéshez USB és az aktuális teljes SD-csomag szükséges.
- A CI host LVGL-je a P4-hez hasonlóan C könyvtári memóriafoglalót használ. A korábbi kis belső tesztheap miatti hibát reprodukáltuk és javítottuk.


## Nappali/éjszakai megjelenítés

A `nightSource` numerikus Loxone-párosítás pozitív értéke éjszakai módot választ; nulla vagy negatív értéke nappalit. Induláskor nappali a biztonságos alap. Ismeretlen jel esetén az utolsó ismert téma és fényerő marad, nem vált vissza nappalra. A nappali/éjszakai fényerőt a mentett beállítások adják, a BSP csak változáskor kap új értéket, a kijelzőzáron kívül. A 0%-os éjszakai fényerő szándékosan is választható.

A natív nézet a megfelelő `graphics/light/` vagy `graphics/dark/` PNG-t tölti be, a manifest méret- és SHA-adataival ellenőrizve. Azonos hash és méret esetén az aktuális oldal elemei egy PNG-puffert és egy LVGL cache-forrást osztanak meg; felszabadításkor csak a tulajdonos szabadít. A meglévő oldal az új képek előtöltése alatt megmarad, SD-művelet nincs a kijelzőzár alatt.

A sima, hűvös és meleg háttér színei a szerkesztő palettáját követik. A natív átmenet függőleges, a böngészőé átlós; a sima háttérnél nincs átmenet. A készülék nyelvbeállítása az Info/Séma feliratot és a hiányzó grafika helyén megjelenő elemnevet is választja. Téma-, nyelv- vagy projektváltozáskor újraépül az oldal, változatlan állapotnál nem.

A virtuális LVGL-próba a háttérstílusokat, a nappali/éjszakai módot és fényerőhívásokat, az ismeretlen jel megtartását, a nyelvet és a megosztott PNG-forrást ellenőrzi. Ez nem a fizikai panel fényerejének, a tényleges SD-PNG dekódolásnak vagy a 20 FPS-nek a mérése.

## Firmware és grafika közös frissítése

A Frissítés oldal a projekt saját, legújabb GitHub-kiadásából induló `update.json` leírót használja. A kérést a készülék végzi külön, 16 KiB belső stackű workerben; a böngésző nem tölt le release-fájlokat, és nem adhat meg URL-t. A GitHub, a release-assets.githubusercontent.com és az objects.githubusercontent.com HTTPS-átirányításai engedélyezettek, legfeljebb öt átirányítással. CA-ellenőrzés aktív; nincs token vagy hitelesítő adat a kiadási kérésekben. A kiadásnak nyilvánosan elérhetőnek kell lennie. Érvénytelen óra mellett legfeljebb 30 s időszinkron-várakozás történik; TLS-ellenőrzést nem kapcsolunk ki.

A leíró rögzíti a kiadási verziót, a grafikai forrásazonosítót, a manifest és a firmware méretét/SHA-256 lenyomatát. A manifest csak a leíróban megjelölt, rögzített tagből tölthető le. Importált projektből vagy HTTP-kérésből URL nem vehető át. Ezek a hashek integritást és fájlösszerendelést biztosítanak, nem digitális aláírást; a megbízhatóság a projekt GitHub-kiadására és a TLS-re épül.

Az SD-tár útvonala `graphics/blobs/<sha256>`. Minden meglévő fájlt méret és hash alapján ellenőrzünk; az ép közös tartalmat nem töltjük le újra. A hiányzó vagy sérült fájl `.part` néven streamelődik, tartós lezárás és teljes méret/hash-ellenőrzés után nevezhető át. A részleges fájl nem számít késznek, a következő kísérlet azt újrakezdi; a teljes, jó blobokat megtartja. A manifest korlátja 256 KiB, 1024 bejegyzés, 4 MiB/fájl, összesen 128 MiB; tiltott útvonal és hash/méret-ellentmondás nem fogadható el.

Csak teljes készlethez írunk `graphics/sets/<forrás-hash>.json` kötést. Érvényes kötés fájllistája nem írható át ugyanazon forrásazonosító alatt; hibás kötés a teljes készlet ellenőrzése után javítható. Nincs globális aktív mutató: a firmware saját beégetett azonosítója választja a készletet, így bootloader-visszaálláskor a régi firmware saját grafikát választ. A régi kötések és blobok megmaradnak; automatikus szemétgyűjtés még nincs. Kevés SD-helynél a frissítés hibával leáll, nem törli a korábbi készletet.

A firmware streamelése csak az előkészített készlet után indul a nem futó, 8 MiB-os partícióra. A projektazonosító és az egyéni leíró ellenőrzése a törlés előtt, az IDF image/hash/chip/revízió ellenőrzése és a kiadási SHA/forrásazonosító ellenőrzése a bootcél váltása előtt történik. A manuális bináris feltöltéshez az SD-n a binárishoz tartozó teljes készlet szükséges, azonos forrásazonosító esetén is. Siker után további firmware-feltöltés az újraindulásig nem indulhat.

A grafikai oldalon indított frissítés az aktuális firmware azonosítójához tartozó készletet javítja/frissíti. Eltérő grafikai azonosítónál a közös firmware-frissítéshez irányít. Azonos firmware-verzió és grafikai azonosító mellett a közös frissítő nem tölti újra a binárist és nem indít újra; a grafikai készletet ellenőrzi. Az aktuális készlet telepítése/javítása revíziót jelez a natív rajzolónak. A web és a kijelző ugyanazokat a hash-címzett fájlokat használja.

A függő alkalmazás 30 s uptime után véglegesíthető, ha a web, az SD-állapottár és a saját teljes grafikai készlet elérhető. A Loxone adatkapcsolata nem feltétel. 180 s után sikertelen ellenőrzés visszaállást kér; kijelzőindítási hiba azonnal sikertelen indulásnak számít. A bootloader a függő verzió újraindulásakor a saját rollback-szabályát alkalmazza. A pollalapú ellenőrzés nem teljes rendszer-watchdog. Az SD FAT-fájlrendszere és a kártya áramelvételi viselkedése miatt a logikai írási sorrend mellett is szükséges a fizikai megszakításpróba.

Az első, régebbi MS1 buildről áttéréshez USB és az új teljes SD-csomag kell. A kézzel másolt kezdeti készlet `identity.txt` fájlja is kötelező: eltérő vagy hiányzó azonosítóval a legacy manifest nem választódik ki. Ezután a közös frissítés kezeli az új készleteket a legacy könyvtár átírása nélkül.
