# MS1 – első kiadás előtti összefoglaló

Az MS1 fejlesztési ág kipróbálható a Waveshare ESP32-P4-WIFI6-Touch-LCD-10.1, SKU 33150 panelen. A kiadási verziót a main ágba egyesített PR után a CI határozza meg.

## Funkciók és kompatibilitás

- HU/EN webszerkesztő, legfeljebb tíz oldallal és oldalanként 150 elemmel. A szövegmezők, csővégpontok, forgatás és Loxone-kötések az exportban megmaradnak.
- 333 gépészeti típus, világos és sötét megjelenés, animált berendezések, áramlás és állítható osztó-gyűjtők.
- Közvetlen Loxone Gen2/17.x kapcsolat. Az ismeretlen érték külön állapot, az explicit aktivitásjel elsőbbséget élvez a teljesítményből következtetett állapottal szemben.
- SD-re mentett projekt, beállítások és helyreállítás. A sémák projekt- vagy oldalexporttal vihetők át. A Wi-Fi- és Loxone-beállítások másik készülékre költöztetéséhez jelszavas teljes mentés szükséges.
- USB-s első telepítés, összeillő firmware és SD-grafika, későbbi differenciális grafikafrissítés és OTA.

## Telepítés és próba

Használd ugyanazon sikeres MS1-futás firmware- és SD-artefaktumát. Az USB-csomag FLASH.txt fájlja tartalmazza a buildhez tartozó címeket. A grafikai ZIP könyvtárszerkezete kerüljön a FAT32 microSD gyökerébe. A készülék beállító hálózatának jelszava a kijelzőn olvasható. [Telepítési leírás](../keszulek.md) · [Mentés és visszaállítás](../mentes.md)

A hat tesztprojekt mindegyikéhez tartozik közvetlenül importálható JSON, szerkesztőkép és natív kijelzőkép. A gazdagépes ellenőrzés lefedi a böngészős oda-vissza importot, az új folyamatban visszaolvasott SD-állapotot, a natív renderelést és a veremhasználatot. [Próbasémák és mérések](../../tests/schemas/README.md)

## Készülékes átvétel

| Próba | Rögzítendő eredmény |
| --- | --- |
| Indulás, Wi-Fi, Loxone | Panelváltozat, firmware, Miniserver-verzió, bejelentkezés, tokenmegújítás, hálózatvesztés és visszatérés. |
| Szerkesztés és kijelző | Mind a négy tájolás, érintési célpontok, lapozás, nappali/éjszakai mód, feliratok, mentés utáni újraindítás és import. |
| Terhelés | A tízoldalas vegyes projekt egy 150 elemes oldalán kösd az aktivitást valós aktív Loxone-forráshoz. Maradjon a séma látható a panelen. Rögzítsd a befejezett képkockák ütemét, a rajzolási időt és a memória-mélypontokat. |
| Legalább 24 órás járatás | Külső gépről indítsd a lent megadott naplózást. Jegyezd fel a szándékos megszakítások idejét. Vizsgáld meg a memória és a verem alakulását, az üzemidő visszaesését, az elmaradt vagy elöregedett mintákat és a kapcsolat helyreállását. |
| SD és OTA | Kártya nélküli indulás, visszahelyezés és helyreállítás. Megszakított letöltés, ismételt indítás, sérült grafika újraletöltése, firmware-visszaállás és a korábbi készlet olvashatósága. |
| Kiadás után | A main kiadás címkéje, publikált manifest és firmware letölthetősége, csomagazonosítók és ellenőrző összegek. |

A naplózó Python 3-mal, külön csomag nélkül fut. Az URL a saját készülék címe. Az alapértelmezett mintaköz egy másodperc. Minden sor azonnal fájlba kerül, az elérési hibák külön státuszt kapnak. A hiányzó minták és az ismételten lekért azonos készülékminták a kiértékelésnél figyelembe veendők.

```sh
python3 tools/record_device_diagnostics.py http://zolee-schema-monitor.local ms1-24h.csv
```

Az aktív, 150 elemes terhelési oldal naplóját az alábbi paranccsal értékeld ki. A program összesíti a különböző mérési ablakokat, a mért időre jutó FPS-t és az 50 ms feletti rajzolásokat. Külön jelzi az ismételt mintákat, a mérés nélküli sorokat, a megfigyelt ablakok közötti időhiányt és az észlelt üzemidő-visszaeséseket. A statikus oldalon mért 0 FPS is beleszámít az átlagba, ezért egy aktív terhelési méréshez hagyd végig ugyanazt az oldalt a panelen.

```sh
python3 tools/summarize_frame_windows.py ms1-24h.csv
```

A fizikai próbák és a kiadás utáni ellenőrzések eredménye még nyitott. [MS1 elfogadási lista](../ms1.md)
