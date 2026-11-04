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
