# Készülékes fejlesztői build

Ez az MS1 részállapota. A helyi szerkesztő és a mentés bekötve, a kijelzőn statikus sémanézet és diagnosztikai lap is látszik. A numerikus Loxone-kliens és az animációk bekötve, de a fizikai Miniserver-próba még szükséges. Első telepítéshez USB szükséges; a firmware-OTA első változata azonos grafikai készlettel használható. A közös firmware–grafika frissítés még nincs kész.

## Első indítás

1. A CI `schema-firmware` csomagjának USB ZIP-je és `FLASH.txt` útmutatója használható a SKU 33150 panelhez. A 32 MB flash/PSRAM-os, újabb P4 hardverváltozat a cél.
2. A FAT32 microSD-re a `schema-graphics.zip` teljes `graphics/` könyvtárát másold a gyökérbe. Ezt a build a szerkesztő ikonjaihoz használja.
3. Üres Wi-Fi-beállításnál kb. egy másodperc után `ZoleeSchema-XXXXXX` nevű WPA2 hálózat indul. A véletlen, eszközönként eltérő jelszó a kijelzőn olvasható; a webes API nem adja vissza.
4. Csatlakozás után nyisd meg a `http://192.168.4.1/` címet. Állítsd be az otthoni Wi-Fi-t a Beállítások oldalon. A sikeres tartós mentés után három másodperccel ellenőrzött újraindítás következik.
5. A saját hálózaton a kijelzőn látható IP vagy a `http://zolee-schema-monitor.local/` használható. A tájolás szintén újraindításkor lép életbe. Sikertelen kapcsolódás esetén kb. 45 másodperc múlva ismét elérhető a védett beállító hálózat. Ha egyszer elindult, az adott futás során elérhető marad.

A szerkesztő valódi készülékes mentést végez. SD hiányában olvasás és beállító web működhet, de a mentés hibát jelez; nem helyettesíti azt átmeneti RAM-sikerrel. Az SD-t használat közben ne távolítsd el: az automatikus újracsatolás még nincs bekötve.

## Mentés és helyreállítás

Az SD `schema-state/state-0.bin` és `state-1.bin` fájlja váltott generációt tartalmaz. A fejléc generációja, hossza és a teljes törzs CRC-védett. Új állapot először `.part` fájlba kerül, `fsync` után nevezzük át; az előző példány megmarad. Induláskor a legújabb **teljes és érvényes** példány töltődik be. Egy sérült új mentés nem nyomja el az előzőt. A böngésző revíziója védi a párhuzamos szerkesztéseket.

A sémában csak az oldalak és az értékpárosítások vannak. A beállítások AES-256-GCM borítékban kerülnek az SD-re; a véletlen kulcs az adott kijelző NVS-ében él. Az API a Wi-Fi/Loxone jelszó helyett csak `hasPassword` jelzőt küld, a `null` jelszó megtartja a korábbit. Ez nem hardveres kulcsvédelem, és a teljes eszközmentés sem készült még el: **az NVS törlése vagy másik kijelzőre helyezett SD esetén a beállítások nem fejthetők vissza**. Az ügyfélnek átadható sémaexportot külön mentsd el.

A helyi adminisztráció a saját hálózat és a WPA2 beállító hálózat bizalmára épül. Nincs adminbelépés vagy HTTPS a beállító weben. A Host-ellenőrzés, az Origin és az egyedi írási fejléc az idegen weboldalakból indított módosításokat korlátozza; ne tedd közvetlenül internetre.

## Még szükséges

A panel és a Wi-Fi/C6/SD együttműködésének fizikai próbája, a natív renderelő és a Loxone-kliens fizikai ellenőrzése, a differenciális grafikai OTA, a teljes diagnosztikai ablak és az MS1 teljes eszközkészletének véglegesítése. A gazdagépes tesztek nem helyettesítik ezeket.


Statikus natív sémanézet: a mentett projekt aktuális oldala már LVGL objektumokként megjelenik. A PNG-ket a `graphics/light/` könyvtárból, a manifest méret- és SHA-256-adataival ellenőrizve olvassa; hibás/hiányzó grafikánál az elem neve látszik. Csövek, szövegek, értékhelyőrzők és a negyedfordulatos képek bekötve. Lapozás a felső nyilakkal vagy vízszintes gesztussal; az Info/Séma gomb a diagnosztikát váltja. Üres első projektnél a diagnosztika marad az indulónézet. A háttérben mentett új revízió automatikusan betöltődik, változatlan projektnél nincs újrarajzolás vagy projektmásolás.

Ez még korlátozott működési változat: világos/sötét nézet és háromféle háttér, 20 px-es Unicode betűkészlet skálázása; az élő numerikus érték és animáció a Loxone-klienshez csatlakozik, fizikai próba még szükséges. A szerkesztő összes formázási és témalehetőségének pontos natív egyezése, a PNG dekódolás és gesztusok fizikai próbája még szükséges. A host LVGL-próba 150 csövet, álló/fekvő koordinátákat, lapozást, revízió-újrafelhasználást és diagnosztikaváltást igazol; a 20 FPS-t nem méri.


### Loxone-profil kipróbálása

A Loxone oldalon a Miniserver címe, portja, felhasználója és jelszava menthető. WSS/TLS esetén a megfelelő CA PEM is szükséges. Sikeres tartós mentés után a készülék újraindul és önállóan csatlakozik. A kapcsolati státuszt a webes Loxone oldalon lehet követni. Kapcsolódás után a séma értékválasztója az alkalmazásstruktúra helyiség/funkció szűrőit és állapot-UUID-jait kínálja. Fizikai Miniserveres ellenőrzés még szükséges, ezért a jelenlegi build továbbra is fejlesztői változat.


### Éjszakai nézet és fényerő

A Beállítások oldalon válassz éjszakai Loxone-jelet, és állítsd be a két fényerőt. Pozitív jel éjszakát, nulla vagy negatív jel nappalt jelent. Kapcsolatvesztéskor a legutóbbi mód marad; az első érvényes jel előtt nappali a nézet. A fényerő fizikai ellenőrzése még szükséges. A natív sötét grafikákhoz az SD-n a `graphics/dark/` PNG-k is legyenek meg.

### Firmware frissítése

A Frissítés oldalon a CI `schema-firmware` csomagjából származó `ZoleeSchemaMonitor.bin` választható. A feltöltés teljes ellenőrzése után a készülék körülbelül három másodperccel újraindul. Ezután töltsd újra a böngészőoldalt. Elsőként az OTA-t már tartalmazó buildet USB-n kell telepíteni.

Jelenleg csak a futó firmware-ével azonos katalógus/grafikagenerátor-forrásazonosító fogadható el. Grafikai változást tartalmazó buildnél marad az USB-telepítés és a hozzá tartozó teljes `graphics/` könyvtár SD-re másolása kikapcsolt készüléken. Ez átmeneti korlát a differenciális grafikai frissítés és a közös visszaállítás elkészültéig. A forrásazonosító nem helyettesíti az SD-fájlok ellenőrzését vagy a megbízható firmware-forrást.

Megszakadt vagy elutasított feltöltésnél a futó partíció marad aktív, újrapróbálás lehetséges. Az új verzió a web és az SD-állapottár sikeres alapindulása, legalább 30 s uptime után válik véglegessé. A Loxone pillanatnyi elérhetősége ezt nem akadályozza. A készülékes OTA és áramelvételes visszaállítás fizikai tesztje még szükséges; az MS1 továbbra is nyitott.
