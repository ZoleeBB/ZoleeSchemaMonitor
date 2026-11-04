# Fejlesztési állapot

Az MS1 **nyitott**. Ez az állomány az alapozó fejlesztési állapotot írja le, nem PR-zárási verziódokumentum.

| Terület | Elkészült | Még szükséges |
| --- | --- | --- |
| Webszerkesztő | Oldalak, húzás, méretezés, negyedfordulatok, cső töréspontok, csatlakozópontok, szöveg és érték, HU/EN | Teljes használhatósági finomítás, csoportosítás/igazítás, fizikai készülékes ellenőrzés |
| Adatmodell | 10 oldal, 150 elem/oldal, szigorú import, titkoktól elkülönített export, aktivitási szabályok | Animált sémafuttató; a szigorú parser és statikus nézet már kész |
| Adatpárosítás | Helyiség/funkció/név szűréses választó és szimulált élő előnézet | Valódi Loxone hitelesítés és állapotfolyam portolása |
| Grafika | B stílusreferencia, 138 elem metaadata, világos/sötét közös vektorcsaládok és PNG-generálás | Típusonkénti grafikai megkülönböztetés és animációk teljes ellenőrzése; ez még nem a végleges teljes készlet |
| Differenciális tár | Fájlonkénti SHA-256, sérülésjavítás, megszakítás utáni újrafelhasználás, tranzakciós aktiválás, gazdagépes tesztek | SD/HTTP/OTA készülékes port, firmware-rollback integráció |
| Mentés | Fejlesztői és készülékes API, revízióütközés, kétpéldányos CRC-s SD-napló, AES-GCM beállítások, titokmentes export/import | Teljes eszközmentés és kártya nélküli helyreállítás |
| P4 | Rögzített BSP/adapter, átemelt pufferjavítások, négy tájolás indítási útja, valós CPU/RAM/PSRAM/DMA diagnosztikai alap, DHCP/mDNS, védett beállító AP, tömörített beágyazott web | Teljes diagnosztikai ablak, érintés/lapozás és animációk, megjelenítési egyezés és fizikai ellenőrzés |
| CI | Modell/tárolás/C/Chromium, grafikai csomag, P4 build és main-merge kiadási folyamat forrása; sikeres helyi és korábbi távoli P4 build, natív HTTP/tárolás/modell teszt | Új commit távoli eredménye, teljes MS1 kiadás előtti készülékes regresszió |

## Ellenőrzési határok

- A helyi fejlesztői API nem csatlakozik Miniserverhez, és nem állít sikeresnek valódi firmware-frissítést.
- A P4 firmware a valódi webes API-t szolgálja ki; a kijelző és a webes fejléc jelzi a hiányzó animáció/Loxone/OTA integrációt. Ezek végpontjai nem adnak szimulált sikert.
- A CPU/RAM adatokat a natív firmware valóban méri. A számítógépes előnézet ezeket nem mértnek mutatja.
- A 20 FPS és a négy érintési tájolás fizikai panelen nem igazolt.
- Az MS1 bezárása előtt a [teljes ellenőrzési lista](ms1.md) minden pontját teljesíteni kell.


Statikus natív sémanézet: a mentett projekt aktuális oldala már LVGL objektumokként megjelenik. A PNG-ket a `graphics/light/` könyvtárból, a manifest méret- és SHA-256-adataival ellenőrizve olvassa; hibás/hiányzó grafikánál az elem neve látszik. Csövek, szövegek, értékhelyőrzők és a negyedfordulatos képek bekötve. Lapozás a felső nyilakkal vagy vízszintes gesztussal; az Info/Séma gomb a diagnosztikát váltja. Üres első projektnél a diagnosztika marad az indulónézet. A háttérben mentett új revízió automatikusan betöltődik, változatlan projektnél nincs újrarajzolás vagy projektmásolás.

Ez még korlátozott működési változat: világos nézet, fix alapszínű háttér, 20 px-es Unicode betűkészlet skálázása; nincs élő érték vagy animáció. A szerkesztő összes formázási és témalehetőségének pontos natív egyezése, a PNG dekódolás és gesztusok fizikai próbája még szükséges. A host LVGL-próba 150 csövet, álló/fekvő koordinátákat, lapozást, revízió-újrafelhasználást és diagnosztikaváltást igazol; a 20 FPS-t nem méri.


## Natív érték- és animációs motor

Az `app_scene_set_reader` RAM-adatolvasó felületen keresztül kapott jeleket a natív nézet már feldolgozza. A Miniserver-adapter még nincs csatlakoztatva: alaphelyzetben nincs szimulált aktivitás, az értékek ismeretlenek. A gazdagépes teszt célzottan ad jeleket ennek a felületnek.

A csövek három pontjának mozgása a töréspontokból számolt pixelhosszt követi, megadott irányban. A rotoros eszközök külön forgó üzemjelzőt kapnak. Az explicit aktivitás elsőbbséget élvez; teljesítményből csak az alsó határ fölött következik aktivitás. A sebesség a beállított tartományból normalizált, legalább 15%, legfeljebb 100%. A megjelenített teljesítményszázalék szintén a tartományhoz viszonyított. Az ismeretlen jel megállítja a mozgást, az ismeretlen aktivitás halványítja az elemet. Az értékmezők skálázást, eltolást, tizedesszámot és mértékegységet használnak.

A főciklus 50 ms-os célütemmel hívja a motort, tényleges eltelt idővel; hosszú megakadás után legfeljebb 200 ms-ot lép. A mozgásciklus nem olvas JSON-t, SD-t vagy hálózatot. Az alkalmazás fix tárolókat és statikus címkeszövegeket használ; változatlan értéket nem ír újra. A nagy geometriai, állapot- és pillanatkép-tárolók a P4-en PSRAM BSS-ben vannak. Diagnosztikai nézetben az animáció szünetel. A fizikai 20 FPS és az LVGL teljes rajzolási memóriaigénye még nem mért.

Ez nem zárja le a korábbi korlátozásokat: sötét téma, pontos formázási egyezés, permetanimációk, végleges grafikák, Loxone-kapcsolat és OTA még szükségesek.


## Loxone protokollfeldolgozás előkészítése

A `schema_loxone` hordozható modul teljes WS-üzenetekből ellenőrzi a Loxone-fejlécet és a pontos payloadhosszt. A becsült fejlécet nem használja foglalásra, a hibás vagy túl nagy üzenetből nem ad részleges eseményt. A transzport feladata marad a WebSocket-fragmentumok összeállítása és a hibánál történő bontás.

Az alkalmazásstruktúra-parser csak olvasható állapotokat ad vissza, parancs-UUID-kat nem. A subControls elemek a hiányzó helyiség/funkció adatait a szülőtől öröklik. Jelenlegi korlát: 512 KiB struktúra, 2048 állapotbejegyzés, nyolc beágyazási szint; túllépéskor nem aktivál részleges listát. A gazdagépes teszt az üzenetkeretezést és egy szintetikus LoxAPP-struktúrát ellenőriz. A hálózati adapter, kulcscsere és bejelentkezés továbbra is hátravan; a készülék még nem kapcsolódik Miniserverhez.

A CI animációs próbáját az LVGL memóriafoglalójának egységesítésével javítottuk: a host teszt ugyanúgy C könyvtári mallocot használ, mint a P4. A böngésző-képek feltöltése csak akkor fut, ha készültek képek; korábban a megelőző teszthiba után egy másodlagos hiányzó-artefaktum hiba is elfedte az első okot.
