# Architektúra

## Felelősségek

1. A böngésző végzi a szerkesztést, előnézetet és a séma ellenőrzését.
2. A P4 helyi HTTP API-t szolgál ki, tárolja a projektet és közvetlenül csatlakozik a Loxone Miniserverhez.
3. A natív LVGL megjelenítő kizárólag a látható oldal változó részeit animálja. A hálózat és az SD nem dolgozhat a kijelzőzár alatt.
4. A stabil, verziózott projektformátum független a webszerkesztő belső állapotától. A firmware minden importot újraellenőriz.

## Tájolás

A panel natív koordinátái 800×1280. 0/180° álló, 90/270° fekvő. A projektoldalak normalizált koordinátákat tárolnak; a megjelenítő és a szerkesztő ugyanazt a vászonméretet használja. A tájolás mentése után ellenőrzött újraindítás történik; a futás közbeni driver-forgatás a korábbi projektből ismert hibaút miatt kerülendő.

## Állapotok

Minden kötés állapot-UUID-t és opcionális átalakítást tartalmaz. A vezérlő parancs-UUID-ja külön fogalom, az MS1 nem küld parancsot. Ugyanaz az állapot több elemhez rendelhető. Kapcsolatváltáskor a korábbi munkamenet értékei ismeretlenné válnak.

A teljesítményből számított aktivitás: `ismert && érték > minimum`. Explicit aktivitásjel esetén az az elsődleges, ismeretlensége nem kerülhető meg a teljesítményjellel. Az animációsebesség aránya `clamp((érték-min)/(max-min), 0, 1)`.

## Grafikai készlet

A firmware által ismert elemtípusokból összeállított, HU/EN metaadatokat tartalmazó katalógus különül el a grafikai fájloktól. A készletmanifest formátumverziót, kompatibilitási azonosítót, fájlméretet és SHA-256 értéket tartalmaz.

A natív SD-tár `graphics/blobs/<sha256>` fájlokat és `graphics/sets/<forrás-hash>.json` kötéseket használ. Csak a hiányzó vagy hibás blob töltődik le `.part` fájlba; ellenőrzés és tartós lezárás után nevezhető át. Kötés csak teljes készlethez készül, a firmware saját azonosítója választja. Nincs globális aktív mutató: rollbackkor a firmware-rel együtt a készletválasztás is visszaáll. Régi fájlt nem gyűjtünk automatikusan. A nyilvános export nem tartalmaz grafikai fájlt vagy titkot. A gazdagépes `AssetStore` korábbi, aktív mutatós algoritmusa referencia; a készülék az itt leírt kötéseket használja.

Az eszköz a saját projekt legújabb kiadásának `update.json` leírójából és annak rögzített tagjéből tölthet grafikát; importált oldal nem adhat meg letöltési URL-t. A fájlonkénti hash integritást biztosít, önmagában nem digitális aláírás.

## Átemelési határok

A forrásprojektből a P4 BSP/adapter és ESP-Hosted javítások, Wi-Fi, Loxone hitelesítés, diagnosztika és a webes megjelenési alapok használhatók. Az energiaadatokra szabott UI és adatmodell, BLE, kaputelefon, statisztika és MQTT nem kerülhet az új firmware függőségi körébe. Minden átemelés pontos forráscommitja dokumentálandó.

A kiadáscsomagoló ellenőrzi az összes ismert elemtípus világos/sötét SVG és PNG útvonalának meglétét, a firmware/manifest verzióegyezését és a binárisba linkelt grafikai azonosítót. Ezután készül a firmware- és manifest-hasheket tartalmazó `update.json`; a pipeline a firmware, a leíró, a manifest és a hash-címzett blobok együttesét adja ki.
