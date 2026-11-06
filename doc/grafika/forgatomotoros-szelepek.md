# Forgatómotoros háromjáratú szelepek

A felhasználó által kért ESBE-jellegű kialakítások képi alapjai. A már meglévő motoros szelepeket nem helyettesítik. A katalógusregisztráció, a pontos portkoordináták és a futásidejű animáció bekötése a teljes képkészlet utáni integráció feladata; jelenleg nem kész működő funkció.

- [Szürke házas látványterv](latvanytervek/08a-forgatomotoros-szelepek.jpg)
- [Fekete házas, élő álláskijelzés látványterve](latvanytervek/08b-fekete-forgatomotoros-szelepek.jpg)

| PNG-forrás | Kialakítás |
| --- | --- |
| rotary-motor-valve-3 | Szürke forgatómotor, menetes sárgaréz test |
| union-rotary-motor-valve-3 | Szürke forgatómotor, hollanderes sárgaréz test |
| black-rotary-valve-3 | Fekete forgatómotor, menetes sárgaréz test |
| black-union-rotary-valve-3 | Fekete forgatómotor, hollanderes sárgaréz test |
| black-flanged-rotary-valve-3 | Fekete forgatómotor, kék karimás test |

Mindegyik három vízcsatlakozót kap (bal, jobb, alsó). A felső szervizfedél zárt, az elektromos tömszelence nem vízcsatlakozó.

## Élő álláskijelzés – megvalósítandó

A fekete házas alapképek gombfelülete üres. A rögzített piros ív az alapképen marad; a piros mutató külön vektoros réteg, a százalék külön élő felirat legyen. A látványterv 50%-a szemléltetés, nem beégetett mérési érték.

A helyiség- és funkciószűrős Loxone-választóban külön állásérték legyen megadható. Kötelező alsó/felső határ (alapból 0–100), felső > alsó. Megjelenítés: 100 × (érték − alsó)/(felső − alsó), 0–100%-ra korlátozva; egész százalék a gombon. A mutató alapból −45° és +45° között mozogjon (50% függőleges), iránya megfordítható legyen. A mutató forgása a gomb középpontjához igazodjon, a felirat ne forogjon vele. Méretezéskor mindkettő arányosan igazodjon.

Ismeretlen vagy érvénytelen adat esetén a mutató rejtett, az érték „—”; ez nem 0%. Az állás nem azonos a folyadékárammal vagy hőteljesítménnyel. Az aktivitás külön jel marad, pusztán a nyitottságból ne induljon csőáramlás. Bekötéskor ellenőrizendő 0/50/100%, fordított irány, más bemeneti tartomány, adathiány, méretezés és mindkét téma. Teszt és fizikai P4-próba még nem történt.

Tájékozódási alap: [ESBE VRG130 + ARA600](https://esbe.eu/group/products/rotary-units/vrg130-ara600-3-point). A grafikák saját illusztrációk, nem gyártói szerelési rajzok.

## Fejlesztési állapot – állásszámítás

A böngészős `positionState` és a firmware hordozható `sm_position` függvénye elkészült. Külön kötésből, az aktivitás módosítása nélkül számítanak 0–100%-os nyitottságot és −45…+45° mutatószöget. Az irányfordítás a szöget fordítja, a nyitottsági értéket nem. Hiányzó, érvénytelen vagy megszakadt kapcsolatból származó adatnál `known=false`; ilyenkor a rajzoló feladata a mutató elrejtése és a „—” felirat. A nulla százalék érvényes, ismert állapot.

Határérték-, tartomány-, irányfordítás- és adathiány-tesztesetek készültek C és JavaScript nyelven. Futtatásuk a felhasználó korábbi kérésére a teljes képkészlet utáni összevont ellenőrzésre marad. A szerkesztő beállítómezői, a projektformátum állásmezője, a Loxone-előfizetés és a két rajzoló bekötése még hátravan; ez a változás önmagában nem kapcsolja be az élő mutatót.

## Fejlesztési állapot – beállítás és import

A szerkesztőben külön állásforrás választható a meglévő helyiség-/funkciószűrős értékválasztóval. Hozzárendelés után alsó/felső határ és mutatóirány állítható. A `position` mező opcionális, a korábbi oldalak mező nélkül is elfogadhatók; az új mező mentéssel és oldalexporttal együtt megmarad. A C és JavaScript importellenőrzés csak érvényes kötést, növekvő véges tartományt és logikai irányjelzőt fogad el, tetszőleges további mezőt nem.

A felület jelzi, hogy a mutató rajzolása még fejlesztés alatt. A készülékes élő olvasás és a két rajzoló bekötése továbbra is hátravan. A tesztesetek bővültek, futtatásuk és az Actions ellenőrzése továbbra is halasztva.
