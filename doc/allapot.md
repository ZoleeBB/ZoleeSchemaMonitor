# Fejlesztési állapot

Az MS1 **nyitott**. Ez az állomány az alapozó fejlesztési állapotot írja le, nem PR-zárási verziódokumentum.

| Terület | Elkészült | Még szükséges |
| --- | --- | --- |
| Webszerkesztő | Oldalak, húzás, méretezés, negyedfordulatok, cső töréspontok, csatlakozópontok, szöveg és érték, HU/EN | Teljes használhatósági finomítás, csoportosítás/igazítás, natív készülék-API |
| Adatmodell | 10 oldal, 150 elem/oldal, szigorú import, titkoktól elkülönített export, aktivitási szabályok | Natív projektparser és sémafuttató |
| Adatpárosítás | Helyiség/funkció/név szűréses választó és szimulált élő előnézet | Valódi Loxone hitelesítés és állapotfolyam portolása |
| Grafika | B stílusreferencia, 138 elem metaadata, világos/sötét közös vektorcsaládok és PNG-generálás | Típusonkénti grafikai megkülönböztetés és animációk teljes ellenőrzése; ez még nem a végleges teljes készlet |
| Differenciális tár | Fájlonkénti SHA-256, sérülésjavítás, megszakítás utáni újrafelhasználás, tranzakciós aktiválás, gazdagépes tesztek | SD/HTTP/OTA készülékes port, firmware-rollback integráció |
| Mentés | Fejlesztői API tartós tranzakció és revízióütközés-kezelés; oldal/projekt export/import | Készülékes beállításmentés és teljes helyreállítás |
| P4 | Rögzített BSP/adapter, átemelt pufferjavítások, négy tájolás indítási útja, valós CPU/RAM/PSRAM/DMA diagnosztikai alap | Teljes diagnosztikai ablak, érintés/lapozás és oldalak natív renderelése, fizikai ellenőrzés |
| CI | Modell/tárolás/C/Chromium, grafikai csomag, P4 build és main-merge kiadási folyamat forrása; sikeres helyi P4 build | Távoli futás eredményének ellenőrzése, teljes MS1 kiadás előtti készülékes regresszió |

## Ellenőrzési határok

- A helyi fejlesztői API nem csatlakozik Miniserverhez, és nem állít sikeresnek valódi firmware-frissítést.
- A P4 platformfirmware felirata is jelzi, hogy a teljes készülékes integráció még fejlesztés alatt van.
- A CPU/RAM adatokat a natív firmware valóban méri. A számítógépes előnézet ezeket nem mértnek mutatja.
- A 20 FPS és a négy érintési tájolás fizikai panelen nem igazolt.
- Az MS1 bezárása előtt a [teljes ellenőrzési lista](ms1.md) minden pontját teljesíteni kell.
