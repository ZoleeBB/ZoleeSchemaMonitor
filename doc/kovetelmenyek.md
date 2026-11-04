# Rögzített követelmények

Forrás: a felhasználó 2026-11-04-i specifikációja. A költségcél 50–60 ezer Ft komplett eszközönként; ez cél, nem beszerzési ajánlat.

## Kijelző

- Az ismert Waveshare P4 10.1″ panel, Wi-Fi, önálló eszközök, nincs master/slave.
- Loxone Gen2/17.x, egy Miniserver eszközönként. Kezdetben csak olvasás; a későbbi célzott vezérléshez bővíthető modell.
- Legfeljebb 10, alapból teljes képernyőn elférő oldal. 50–150 elem/oldal; 20 FPS cél.
- 0/90/180/270° tájolás. A webszerkesztő a mentett tájolásnak megfelelő álló/fekvő munkalapot mutatja.
- Lapozás és diagnosztikai ablak érintéssel. A diagnosztika a korábbi projektet követi: IP, Wi-Fi, Loxone, üzemidő, verzió, magonkénti CPU, belső RAM, PSRAM, DMA, SD és események.
- Világos/sötét megjelenítés, egyszerű választható háttér és halvány színátmenet. Nincs ház-/falrajzolás.
- Kezdetben csak valós idejű adatok. A későbbi, mezőnként bekapcsolható statisztika és részletes grafikon nem MS1-funkció.

## Szerkesztő

- Számítógépes, egérrel használható böngészős felület. Oldalak, eszköztár, húzás, méretezés, forgatás, csatlakozópontok, csövek, szövegek és értékmezők.
- Cső: szín (első palettaszínek piros/kék), vastagság, útvonal, irány, egyszerű aktivitásjel, áramlásanimáció.
- Szöveg/érték: betűméret, szín, igazítás; értéknél Loxone-forrás, mértékegység, pontosság, szorzó/eltolás.
- Minden forrásválasztó helyiség/funkció szűrőt és keresést ad.
- Szivattyú: opcionális aktivitásjel és teljesítményforrás. Teljesítményforráshoz kötelező véges alsó/felső érték, felső > alsó.
- Explicit aktivitásjel esetén az dönt. Ennek hiányában az ismert teljesítmény > alsó határ jelent aktivitást. A felső határ feletti érték is aktív; csak az animációsebesség telítődik. Ismeretlen érték nem kikapcsolt állapot.
- Export/import egy oldalra és teljes projektre. A támogatáshoz elküldhető export nem tartalmazhat Wi-Fi-/Loxone-/webjelszót vagy tokent.
- Nincs felhasználói SVG/képfeltöltés. A teljes, kategóriákra bontott elemkészlet MS1-feltétel; az elemtípusokat firmware határozza meg.

## Web, tárolás és frissítés

- A korábbi projekt kezdőoldala, támogatási kártyája, beállítás-, mentés-, OTA- és diagnosztikai megközelítése adaptálandó; saját projektikon kerül a támogatási blokkba.
- Nincs kaputelefon, MQTT, általános Otthon vezérlés vagy energiaáramlási témakezelés.
- A sablonkezelő helyett grafikai készlet kezelője. Elemenként/fájlonként SHA-256, csak változott/új/hibás fájlok letöltése.
- OTA aktualizálja a firmware-höz tartozó grafikai csomagot. Félbeszakítás és verzióvisszaállás alatt is megmarad a kompatibilis készlet.
- HU/EN felület. Új kiadás main-be merge után, a másik projekt mintájára. Kód magyar kommentekkel és olvasható formában.
- README rövid; további dokumentáció `doc/`, PR-zárási verziódokumentumok `doc/verziotortenet/`.

## Grafikai döntési pont

A teljes készlet készítése előtt összehasonlítható látványterveket mutattunk be. A felhasználó a **B, térhatásosabb** irányt választotta. A determinisztikus vektoros rajzcsaládok ennek kezdeti műszaki megvalósításai; a teljes katalógus minden változatának egyedi, végleges grafikai ellenőrzése továbbra is MS1-feltétel.
