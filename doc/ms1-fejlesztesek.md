# MS1 – megvalósított fejlesztések és ellenőrzési státusz

**Utolsó dokumentációs felülvizsgálat: 2026. november 12.** Ez az MS1-ág funkcióleltára, nem lezárt release vagy fizikai átvételi jegyzőkönyv. A követelmények: [MS1 elfogadási lista](ms1.md); a konkrét még nyitott feladatok: [MS1 hátralévők](ms1-hatralevo.md).

## Szerkesztő és felhasználói felület

- A webszerkesztő HU/EN felületű, kizárólag PC-s tervezésre készült. Maximum 10 oldal és 150 elem/oldal használható; húzás, méretezés, visszavonás, 0°/90°/180°/270° forgatás, szöveg-/értékmező és balra/középre/jobbra igazítás működik.
- Elemkereső, kategóriaválasztó, előnézeti kép és csoportosított tulajdonságok segítik az összeállítást. A lokális CSS Grid/Flex kialakítás offline is elérhető; Bootstrap CDN szándékosan nem szükséges. [Webfelület és részletek](webfelulet.md).
- A csövek színe és vastagsága állítható (1–30 px), a csonkokhoz kalibrált végpontokra kapcsolhatók. Két cső metszése elkerülő ívként vagy T-/keresztező csomópontként állítható; a csomópont közös aktivitás-/áramlásadatot használ. [Keresztezések](webfelulet.md#csőkeresztezés-és-leágazás) · [Portaudit](grafika/csatlakozopont-audit.md).
- Projekt-/oldalimport és -export, revízióvédett mentés és visszatöltés működik. A hordozható sémaexport nem tartalmaz jelszavakat. [Mentés és visszaállítás](mentes.md).

## Grafikai katalógus és látvány

| Terület | Elkészült megoldás | Bizonyíték |
| --- | --- | --- |
| Teljes gépészeti katalógus | **333/333 eszköztípus bekötve**; 322 PNG-s típus / 319 használt külön képfájl, továbbá 11 számított osztó | [Teljes bekötés](grafika/teljes-bekotes.md) |
| Kazánpaletta | Magyarországon elterjedt gyártók és kondenzációs, hagyományos, kombinált, HMV-tárolós, vegyestüzelésű stb. kapcsolási változatok; gyártónként közös jellegzetes kép és HU/EN gyártónévszűrés | [Gyártó-adatmodell](../catalog/boiler-brands.json) · [Képes áttekintés](grafika/latvanytervek/kazangyartok.jpg) |
| Osztó-gyűjtők | **2–16 kör**; változó szélességű, részletgazdag fotómodulok, fűtés/hűtés és körönként saját termofejjelző | [Osztók](osztogyujtok.md) |
| Motoros szelepek | Két- és háromjáratú típusok, állítómotorok, százalékos szelepállás; Loxone-párosítással együtt | [Szelepes látványtervek](grafika/latvanytervek/08a-forgatomotoros-szelepek.jpg) |
| Mozgó elemek | Szivattyúrotorok, ventilátorok, szelepek, szórófejek és irányhelyes csőáramlás; mozgó réteg az álló képtől elkülönítve | [Összehasonlító videó](../tests/schemas/animaciok/osszehasonlitas.mp4) |
| Csatlakozások | 333 típus átnézve; 246 típus 544 pontja kalibrálva a csonkokhoz, portindexeket megőrizve | [Csatlakozópont-audit](grafika/csatlakozopont-audit.md) |

A grafika finoman térhatásos **B** irányú. A katalógustípus, az egyedi PNG-forrás és a generált osztó **nem azonos számláló**; a 333 típus nem 333 külön képfájlt jelent. Az eszközök fizikai megjelenését a panelen még ellenőrizni kell.

## Firmware, Loxone és adatkezelés

- Közvetlen Loxone Gen2/17.x numerikus állapotolvasás, helyiség-/funkció-katalógus, UUID-párosítás, hitelesítés/megújítás, megosztott adatminta és visszakapcsolódási logika van bekötve. Fizikai Miniserveres TLS és kapcsolat-helyreállítási próba még hiányzik. [Állapot](allapot.md).
- Ismeretlen érték ≠ nulla. Az explicit aktivitásjel elsőbbséget élvez a teljesítményből következtetett aktivitással szemben. A kijelző nem küld Loxone-vezérlőparancsokat.
- Natív LVGL-megjelenítés négy tájolásban, HU/EN és világos/sötét nézettel, oldallapozással és Info/Séma diagnosztikával. CPU0/CPU1, belső RAM, PSRAM, DMA, SD, verem és képkockaidő mérési felület. [Diagnosztika](diagnosztika.md).
- SD-n kétpéldányos állapottár, AES-GCM titkosított beállítások és külön jelszavas, hordozható teljes eszközmentés. [Mentés](mentes.md).
- Hash-alapú differenciális grafikafrissítés, készlet–firmware összerendelés, hiányzó/sérült fájlok javítása, kiadásleíró és OTA-visszaállási védelem fejlesztése elkészült gazdagépes ellenőrzéssel; tényleges hálózati frissítésből és rollbackből még szükséges fizikai próba. [Részletek](allapot.md#firmware-és-grafika-közös-frissítése).

## Tesztanyagok és ellenőrzési határ

A hat importálható gépészeti tesztprojekt (kazánház, medence, öntözés, légtechnika, osztók, vegyes terhelés), a külön animációs próba és a csőáramlási jelenet megtalálható a [tesztsémák között](../tests/schemas/README.md). A szerkesztőből exportált JSON-ok mellett Chromium-képernyőképek és valódi LVGL-gazdagépes képpufferek is vannak. A tízoldalas terhelési projekt 1500 elemet tartalmaz.

**Mérés, nem ígéret:** a gazdagépes teszt leglassabb 150 elemes fekvő oldalának P95 ideje **113,71 ms** volt (a 2026. november 7-i dokumentált futásban). Ez nem a P4 készülék FPS-e; a 20 FPS cél fizikai mérés nélkül **nem igazolt**. [Gazdagépes mérés és verem](../tests/schemas/README.md#gazdagépes-lvgl-mérés).

A dokumentáció felülvizsgálata nem egyenlő új tesztfuttatással. A korábbi sikeres [MS1 GitHub Actions-futás](https://github.com/ZoleeBB/ZoleeSchemaMonitor/actions/runs/42908714252) 2026. november 10-i; az arculati/documentációs commit CI-eredményét külön kell értékelni.

## Ami még az MS1 lezárásához kell

1. Valódi Waveshare ESP32-P4 bekapcsolás, érintés, forgatás, nappali/éjszakai mód, SD, UI és szerkesztés/import/export regresszió.
2. Valódi Loxone Gen2/17.x hitelesítés, tokenmegújítás, TLS, kapcsolatvesztés/újracsatlakozás és élő állapotok ellenőrzése.
3. Aktív, **150 elemes** oldal **20 FPS céljának** mérése; memória, DMA, feladat-verem, legalább **24 órás** üzem, megszakított hálózat és SD-kivétel.
4. Nyilvános első kiadás után differenciális grafikafrissítés, megszakítás és újrapróbálás, OTA-firmware rollback.
5. MS1 verzió/kiadás, fizikai jegyzőkönyv és elfogadási lista teljesítése. [Kiadási tervezet](verziotortenet/ms1-kiadasi-tervezet.md).

Későbbre külön tartott lehetőség a Loxone szöveges események megjelenítése és a megőrzött régi grafikai készletek tárkarbantartása; ezek nem bizonyítottan elkészült MS1-funkciók.
