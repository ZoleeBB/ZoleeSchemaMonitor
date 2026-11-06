# Képkészítés állapota – 2026. november 6.

A gyártás közben a végleges, kicsinyített forrásképek kerülnek fel kis csomagokban. A katalógushoz illesztés, a forgórészek regisztrálása és az összevont ellenőrzés a képkészlet elkészülte után következik. A forráskép megléte nem jelenti, hogy a firmware már ezt jeleníti meg.

| Csoport | Külön PNG-forrás | Lefedett katalógustípus | Állapot |
| --- | ---: | ---: | --- |
| Szivattyúk | 27 | 29 | Minden forrás elkészült; első 8 forrás/10 típus bekötve |
| Tárolók és HMV | 23 | 24 | Minden forrás elkészült; első 4 forrás/5 típus bekötve |
| Hőtermelők, hőszivattyúk és hűtők | 31 | 31 | Minden forrás elkészült; bekötés később |
| Radiátorok, konvektorok és klímabeltérik | 12 | 12 | Minden forrás elkészült; bekötés később |
| Felületi hőleadók, körök és napkollektor | 7 | 7 | Minden forrás elkészült; bekötés később |
| Hidraulikai hőcserélők, egységek, osztók és szűrők | 36 | 36 | Minden forrás elkészült; bekötés később |
| Szelepek és visszafolyásgátlók | 42 | 42 | Minden forrás elkészült; bekötés később |
| Szellőzési ventilátorok | 12 | 12 | Minden ventilátorforrás elkészült; bekötés később |
| Légkezelők, hővisszanyerők és légkezelési modulok | 14 | 14 | Részben elkészült; bekötés később |
| **Összesen** | **204** | **207** | **15 típus bekötve; további 192 típushoz a PNG már megvan** |

Az induló katalógus 317 típust tartalmaz. A még hiányzó egyedi források száma 110. A három valódi közös kialakítás: a fűtési és szolár keringető a keringetőszivattyú képét, az egyspirálos tároló a HMV-tároló képét használja. Ezekkel a teljes készlet várhatóan 314 külön PNG-ből áll.

## Csoportlátványtervek

- [01 – Szivattyúk](latvanytervek/01-szivattyuk.jpg)
- [02 – Tárolók és HMV](latvanytervek/02-tarolok-hmv.jpg)
- [03 – Hőtermelők](latvanytervek/03-hotermelok.jpg)
- [04 – Hőleadók](latvanytervek/04-holeadok.jpg)
- [05 – Felületi hőleadók](latvanytervek/05-feluleti-holeadok.jpg)
- [06 – Szivattyúváltozatok](latvanytervek/06-szivattyuvaltozatok.jpg)
- [07 – Hidraulika](latvanytervek/07-hidraulika.jpg)
- [08 – Szelepek](latvanytervek/08-szelepek.jpg)
- [09 – Szellőzés és légtechnika](latvanytervek/09-szellozes-legtechnika.jpg)

A fűtés/hűtés kategória mind a 48 típusához, a tárolók mind a 24 típusához és az összes 29 szivattyútípushoz, továbbá a hidraulika kategória mind a 61 típusához megvan a képi alap. A források bekerültek a repóba; a további bekötés és animációpróba a teljes képkészlet után következik.

## Hátralévő képek

| Katalóguscsoport | Még hiányzó típus | Következő munka |
| --- | ---: | --- |
| Hidraulika | 0 | A teljes képi alap elkészült |
| Szelepek | 0 | A teljes képi alap elkészült |
| Szellőzés és légtechnika | 25 | A 12 ventilátor és 14 légkezelési modul kész; további gépváltozatok és légoldali szerelvények következnek |
| Medencetechnika | 27 | A szivattyúk és hőszivattyúk már elkészültek; a többi szerelvény még hátravan |
| Öntözés | 23 | Szórófejek, zónák és szerelvények |
| Érzékelők | 35 | Érték- és állapotérzékelők |
| **Összesen** | **110** | **A legalább 300 eszközös teljes induló készlet még nincs kész** |

## Műszaki felülvizsgálat és hátralévő bekötés

A távhő-fogadóállomás első rajzának hibás, a primer és szekunder oldalt külsőleg összekötő csövezése ki lett cserélve. A javított képen négy külön ág csatlakozik a hőcserélőhöz; a motoros szelep a primer visszatérő ágon van. A vezérlő vezetékei vékony fekete elektromos kábelek.

Az abszorpciós hűtő illusztrációja melegvíz-hajtású kialakítást jelöl: külön hőforrás-, hűtővíz- és hűtöttvíz-csatlakozópárral. A külső köröket nem köti össze díszcső. Gyártói tájékozódási alap: [Yazaki – vízhajtású abszorpciós hűtők](https://www.yazakienergy.com/waterfiredfeatures.htm). Az ikon nem gyártói szerelési rajz.

A ventilátoros gépek alapképein nincs beégetett lapát. A készlet bekötésekor a két- és háromventilátoros, illetve a felülnézetben ellipszisként látszó ventilátorokhoz külön mozgó réteg és pontos vászonkoordináta szükséges. A ventilátor nélküli beltéri és abszorpciós gépek nem kaphatnak általános forgó jelzést.

Az osztó-gyűjtők 2, 3, 4, 5, 6, 7, 8, 9, 10 és 12 körös változatban készültek el. A két fejléc között nincs beégetett csőkapcsolat; a köri csonkok a rögzítőelemek alól a szabad vászonba nyúlnak. A 12 körös változat bekötésekor összesen 26 csatlakozási pont szükséges (két fő és 24 köri); a firmware és a szerkesztő csatlakozókorlátját ehhez külön ellenőrizni kell. A leválasztók és szűrők szervizfedelei, illetve légtelenítő-sapkái nem tekinthetők nyitott vízcsatlakozónak. A metszeti ablakok szemléltetik a szűrőanyagot; az ikonok nem gyártói szerelési rajzok.

A Gitben a futásidejű források 160×160 pixeles, átlátszó PNG-k. A nagy munkaképek nincsenek verziózva. A csoportlátványtervek legfeljebb 1280×960 pixeles dokumentációs képek, nem részei az SD-csomagnak.

A felhasználó kérésére a képkészítés közben nem futtatunk teszteket és nem vizsgáljuk az Actions eredményeit; a képcsomag-commitok `[skip ci]` jelölést kapnak. Az összevont teszt, fordítás és grafikai integráció ellenőrzése még hátravan. A fizikai P4-mérés ettől külön feladat marad.

## Kiegészítés – forgatómotoros szelepek

További öt 160×160-as forráskép elkészült: összesen 209 PNG. Ezek új, a jelenlegi 317 típuson felüli változatjelöltek; a katalógusregisztrációjuk még hátravan, ezért a fenti lefedettségi táblázat nem számolja őket kész katalógustípusként. [Látványtervek és az élő mutató követelményei](forgatomotoros-szelepek.md).

## Ventilátorok animációs bekötése – hátralévő

A látható forgórészű ventilátorok alapképe sötét üres felületet tartalmaz; a lapátok külön futásidejű réteget igényelnek. A fali, cső-, befúvó, elszívó, radiál-, EC-, félradiális, füstelvezető és légrétegződést megszüntető változatnál a forgórész középpontját és két tengelyét képenként kell felvenni a végleges 160×160-as vásznon. A perspektivikus nyílások ellipszisek; kör alakú közös sablon nem feltétlenül illeszkedik. A doboz-, tető- és konyhai ventilátor járókereke zárt burkolat mögött van, ezek állapotjelzést igényelnek, nem kívülre rajzolt lapátokat. A csoportlátványterven látható lapátok szemléltető animációs rétegek. A Loxone-aktivitás, fordulatszámhoz igazított sebesség és adathiány viselkedésének bekötése, ellenőrzése még hátravan.

## Légkezelési modulok – hátralévő bekötés

A hővisszanyerős szellőztető, forgódobos, ellenáramú, keresztáramú és entalpiás modul négy külön légcsatlakozót igényel. A metszeti ablakok az alkatrészek felismerését szolgálják, nem gyártói szerelési rajzok. A forgódobos alap üres körfelületére külön forgó hőcserélőmátrix-réteg szükséges; ventilátorlapát nem használható helyette. A lemezes és membrános betétek nem forognak.

A kaloriferek két légcsatlakozója mellett két külön vízcsatlakozó van; a hűtőkaloriferen kondenzvíz-elvezető is található. A párásítón külön kis vízbetáp és ürítő van, a párátlanítón kondenzvízcsonk. A csonkok médiumát és pontos helyét az integrációkor képenként kell megadni. A rögzítőcsavarok és szervizfedelek nem csatlakozási pontok. Zárt burkolatú légkezelőre nem kerülhet kívül látható általános rotor.

## Következő képcsomag

A tető-légkezelő és befúvó légkezelő képe elkészült és felkerült. A decentralizált szellőztető és a moduláris légkezelő generálása szolgáltatói képgenerálási keretkorlátba ütközött, ezért ezekhez még nincs kész PNG. Folytatás: decentralized-heat-recovery, modular-air-handler, extract-air-unit, steam-humidifier; utána ultrasonic-humidifier és desiccant-dehumidifier, majd a légoldali szerelvények. A tető-légkezelő felső rotorhelyét perspektivikus, ellipszis alakú animációs réteghez kell bekötni.
