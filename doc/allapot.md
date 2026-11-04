# Fejlesztési állapot

Az MS1 **nyitott**. Ez az állomány az alapozó fejlesztési állapotot írja le, nem PR-zárási verziódokumentum.

| Terület | Elkészült | Még szükséges |
| --- | --- | --- |
| Webszerkesztő | Oldalak, húzás, méretezés, negyedfordulatok, cső töréspontok, csatlakozópontok, szöveg és érték, HU/EN | Teljes használhatósági finomítás, csoportosítás/igazítás, fizikai készülékes ellenőrzés |
| Adatmodell | 10 oldal, 150 elem/oldal, szigorú import, titkoktól elkülönített export, aktivitási szabályok | Sémafuttató; a szigorú natív parser már kész |
| Adatpárosítás | Helyiség/funkció/név szűréses választó és szimulált élő előnézet | Valódi Loxone hitelesítés és állapotfolyam portolása |
| Grafika | B stílusreferencia, 138 elem metaadata, világos/sötét közös vektorcsaládok és PNG-generálás | Típusonkénti grafikai megkülönböztetés és animációk teljes ellenőrzése; ez még nem a végleges teljes készlet |
| Differenciális tár | Fájlonkénti SHA-256, sérülésjavítás, megszakítás utáni újrafelhasználás, tranzakciós aktiválás, gazdagépes tesztek | SD/HTTP/OTA készülékes port, firmware-rollback integráció |
| Mentés | Fejlesztői és készülékes API, revízióütközés, kétpéldányos CRC-s SD-napló, AES-GCM beállítások, titokmentes export/import | Teljes eszközmentés és kártya nélküli helyreállítás |
| P4 | Rögzített BSP/adapter, átemelt pufferjavítások, négy tájolás indítási útja, valós CPU/RAM/PSRAM/DMA diagnosztikai alap, DHCP/mDNS, védett beállító AP, tömörített beágyazott web | Teljes diagnosztikai ablak, érintés/lapozás és oldalak natív renderelése, fizikai ellenőrzés |
| CI | Modell/tárolás/C/Chromium, grafikai csomag, P4 build és main-merge kiadási folyamat forrása; sikeres helyi és korábbi távoli P4 build, natív HTTP/tárolás/modell teszt | Új commit távoli eredménye, teljes MS1 kiadás előtti készülékes regresszió |

## Ellenőrzési határok

- A helyi fejlesztői API nem csatlakozik Miniserverhez, és nem állít sikeresnek valódi firmware-frissítést.
- A P4 firmware a valódi webes API-t szolgálja ki; a kijelző és a webes fejléc jelzi a hiányzó Loxone/séma/OTA integrációt. Ezek végpontjai nem adnak szimulált sikert.
- A CPU/RAM adatokat a natív firmware valóban méri. A számítógépes előnézet ezeket nem mértnek mutatja.
- A 20 FPS és a négy érintési tájolás fizikai panelen nem igazolt.
- Az MS1 bezárása előtt a [teljes ellenőrzési lista](ms1.md) minden pontját teljesíteni kell.
