# Készülékes fejlesztői build

Ez az MS1 részállapota. A helyi szerkesztő és a mentés bekötve, a kijelzőn statikus sémanézet és diagnosztikai lap is látszik. A numerikus Loxone-kliens és az animációk bekötve, de a fizikai Miniserver-próba még szükséges. Első telepítéshez USB szükséges; a közös firmware–grafika frissítés bekötve, fizikai próbája még szükséges.

## Első indítás

1. A CI `schema-firmware` csomagjának USB ZIP-je és `FLASH.txt` útmutatója használható a SKU 33150 panelhez. A 32 MB flash/PSRAM-os, újabb P4 hardverváltozat a cél.
2. A FAT32 microSD-re a `schema-graphics.zip` teljes `graphics/` könyvtárát másold a gyökérbe. Ezt a build a szerkesztő ikonjaihoz használja.
3. Üres Wi-Fi-beállításnál kb. egy másodperc után `ZoleeSchema-XXXXXX` nevű WPA2 hálózat indul. A véletlen, eszközönként eltérő jelszó a kijelzőn olvasható; a webes API nem adja vissza.
4. Csatlakozás után nyisd meg a `http://192.168.4.1/` címet. Állítsd be az otthoni Wi-Fi-t a Beállítások oldalon. A sikeres tartós mentés után három másodperccel ellenőrzött újraindítás következik.
5. A saját hálózaton a kijelzőn látható IP vagy a `http://zolee-schema-monitor.local/` használható. A tájolás szintén újraindításkor lép életbe. Sikertelen kapcsolódás esetén kb. 45 másodperc múlva ismét elérhető a védett beállító hálózat. Ha egyszer elindult, az adott futás során elérhető marad.

A szerkesztő valódi készülékes mentést végez. SD hiányában olvasás és beállító web működhet, de a mentés hibát jelez; nem helyettesíti azt átmeneti RAM-sikerrel. Ha az SD hiányzott induláskor vagy olvashatatlanná vált, helyezd vissza, majd használd a Diagnosztika oldal **SD újracsatolása újraindítással** gombját. [Helyreállítás SD-hiba után](mentes.md#helyreállítás-sd-hiba-után).

## Mentés és helyreállítás

Az SD `schema-state/state-0.bin` és `state-1.bin` fájlja váltott generációt tartalmaz. A fejléc generációja, hossza és a teljes törzs CRC-védett. Új állapot először `.part` fájlba kerül, `fsync` után nevezzük át; az előző példány megmarad. Induláskor a legújabb **teljes és érvényes** példány töltődik be. Egy sérült új mentés nem nyomja el az előzőt. A böngésző revíziója védi a párhuzamos szerkesztéseket.

A sémában csak az oldalak és az értékpárosítások vannak. A beállítások AES-256-GCM borítékban kerülnek az SD-re; a véletlen kulcs az adott kijelző NVS-ében él. Az API a Wi-Fi/Loxone jelszó helyett csak `hasPassword` jelzőt küld, a `null` jelszó megtartja a korábbit. Másik kijelzőre költözés vagy az NVS törlése előtt készíts [jelszavas teljes eszközmentést](mentes.md). Az SD-fájlok közvetlen visszaolvasásához az eredeti NVS-kulcs szükséges. Az ügyfélnek átadható sémaexportot külön mentsd el.

A helyi adminisztráció a saját hálózat és a WPA2 beállító hálózat bizalmára épül. Nincs adminbelépés vagy HTTPS a beállító weben. A Host-ellenőrzés, az Origin és az egyedi írási fejléc az idegen weboldalakból indított módosításokat korlátozza; ne tedd közvetlenül internetre.

## Még szükséges

A panel és a Wi-Fi/C6/SD együttműködésének fizikai próbája, a natív renderelő és a Loxone-kliens fizikai ellenőrzése, a közös OTA és a diagnosztikai ablak fizikai ellenőrzése, valamint a 333 típus megjelenésének készülékes ellenőrzése. A gazdagépes tesztek nem helyettesítik ezeket.


Statikus natív sémanézet: a mentett projekt aktuális oldala már LVGL objektumokként megjelenik. A PNG-ket a `graphics/light/` könyvtárból, a manifest méret- és SHA-256-adataival ellenőrizve olvassa; hibás/hiányzó grafikánál az elem neve látszik. Csövek, szövegek, értékhelyőrzők és a negyedfordulatos képek bekötve. Lapozás a felső nyilakkal vagy vízszintes gesztussal; az Info/Séma gomb a diagnosztikát váltja. Üres első projektnél a diagnosztika marad az indulónézet. A háttérben mentett új revízió automatikusan betöltődik, változatlan projektnél nincs újrarajzolás vagy projektmásolás.

Ez még korlátozott működési változat: világos/sötét nézet és háromféle háttér, 20 px-es Unicode betűkészlet skálázása; az élő numerikus érték és animáció a Loxone-klienshez csatlakozik, fizikai próba még szükséges. A szerkesztő összes formázási és témalehetőségének pontos natív egyezése, a PNG dekódolás és gesztusok fizikai próbája még szükséges. A host LVGL-próba 150 csövet, álló/fekvő koordinátákat, lapozást, revízió-újrafelhasználást és diagnosztikaváltást igazol; a 20 FPS-t nem méri.


### Loxone-profil kipróbálása

A Loxone oldalon a Miniserver címe, portja, felhasználója és jelszava menthető. WSS/TLS esetén a megfelelő CA PEM is szükséges. Sikeres tartós mentés után a készülék újraindul és önállóan csatlakozik. A kapcsolati státuszt a webes Loxone oldalon lehet követni. Kapcsolódás után a séma értékválasztója az alkalmazásstruktúra helyiség/funkció szűrőit és állapot-UUID-jait kínálja. Fizikai Miniserveres ellenőrzés még szükséges, ezért a jelenlegi build továbbra is fejlesztői változat.


### Éjszakai nézet és fényerő

A Beállítások oldalon válassz éjszakai Loxone-jelet, és állítsd be a két fényerőt. Pozitív jel éjszakát, nulla vagy negatív jel nappalt jelent. Kapcsolatvesztéskor a legutóbbi mód marad; az első érvényes jel előtt nappali a nézet. A fényerő fizikai ellenőrzése még szükséges. A natív sötét grafikákhoz az SD-n a `graphics/dark/` PNG-k is legyenek meg.

### Firmware és grafika frissítése

Első telepítésnél az aktuális USB build és a hozzá tartozó teljes `schema-graphics.zip` szükséges. A ZIP `graphics/` könyvtárát az `identity.txt`, a `manifest.json` és a `blobs/` könyvtárral együtt másold az SD-re, kikapcsolt készüléken. A régebbi MS1 OTA-ból még nem lehet erre a frissítési rendszerre átállni; az első áttéréshez is USB szükséges.

A Frissítés oldalon a **Legújabb kiadás telepítése** a projekt legújabb nyilvános GitHub-kiadásának firmware-ét és grafikai készletét kezeli. A folyamat állapota, verziója és az átvett bájtok mennyisége megjelenik. Új firmware esetén a teljes ellenőrzés után körülbelül három másodperccel újraindul a készülék; ezután töltsd újra a weboldalt. Ha a verzió és a grafikai azonosító már egyezik, csak a grafika ellenőrzése/javítása történik, újraindítás nélkül.

Az MS1 ágra készített CI-artefaktum nem nyilvános kiadás. Az első main-be egyesített, kiadást létrehozó PR előtt az online frissítő helyesen jelezheti, hogy nincs elérhető kiadás. Addig az USB/SD csomag használható. A frissítő nyilvános release-fájlokat használ, privát repóhoz nincs beépített GitHub-bejelentkezés.

A Grafikai készlet oldalon a Frissítés csak az aktuális firmware-hez illő kiadási készletet használhatja; eltérésnél a közös frissítést kell indítani. A hiányzó/hibás fájlok töltődnek le, a jó fájlokat újrahasználja. Megszakítás után újra indítható: csak a teljes, hash alapján ellenőrzött fájlokat tartja késznek. A futó verziót és annak készletét a frissítés előkészítése nem írja felül.

A kézi `.bin` feltöltés megmaradt. A binárishoz tartozó teljes készletnek az SD-n kell lennie; hiányos készlettel azonos grafikai azonosító esetén sem aktivál firmware-t. Általános grafikai fájlfeltöltő vagy egyedi URL megadása nincs.

Az új alkalmazás a web, az SD-állapottár és a saját teljes grafikai készlet sikeres alapindulása, legalább 30 s uptime után válik véglegessé. A Loxone pillanatnyi elérhetősége ezt nem akadályozza. A készülékes OTA, valódi GitHub-hálózat és áramelvételes visszaállítás fizikai tesztje még szükséges; az MS1 továbbra is nyitott.

### Diagnosztika

Az Info / Séma gomb a kártyás, világos/sötét diagnosztikát nyitja. Memória- és SD-sávok, Wi-Fi/Loxone állapot, számszerű csomag kora, üzemidő, app_main verem és 15 esemény látható; a napló függőlegesen görgethető. A webes Diagnosztika ugyanezt másodpercenként frissíti. Ismeretlen mérésnél nem mutat 0%-ot. A beállító AP jelszava csak a panelen olvasható. [Értelmezés és korlátok](diagnosztika.md).
