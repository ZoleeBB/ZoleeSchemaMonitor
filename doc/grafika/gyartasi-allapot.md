# Képkészítés állapota – 2026. november 6.

A gyártás közben a végleges, kicsinyített forrásképek kerülnek fel kis csomagokban. A katalógushoz illesztés, a forgórészek regisztrálása és az összevont ellenőrzés a képkészlet elkészülte után következik. A forráskép megléte nem jelenti, hogy a firmware már ezt jeleníti meg.

| Csoport | Külön PNG-forrás | Lefedett katalógustípus | Állapot |
| --- | ---: | ---: | --- |
| Szivattyúk | 27 | 29 | Minden forrás elkészült; 12 forrás/14 típus bekötve |
| Tárolók és HMV | 23 | 24 | Mind a 23 forrás / 24 típus bekötve; vizuális és készülékes próba hátravan |
| Hőtermelők, hőszivattyúk és hűtők | 31 | 31 | Minden forrás elkészült; bekötés később |
| Radiátorok, konvektorok és klímabeltérik | 12 | 12 | Minden forrás elkészült; bekötés később |
| Felületi hőleadók, körök és napkollektor | 7 | 7 | Minden forrás elkészült; bekötés később |
| Hidraulikai hőcserélők, egységek, osztók és szűrők | 36 | 36 | Minden forrás elkészült; bekötés később |
| Szelepek és visszafolyásgátlók | 42 | 42 | Minden forrás elkészült; bekötés később |
| Szellőzési ventilátorok | 12 | 12 | Minden ventilátorforrás elkészült; bekötés később |
| Légkezelők, hővisszanyerők és légkezelési modulok | 14 | 14 | Részben elkészült; bekötés később |
| **Összesen** | **204** | **207** | **Az eredeti katalógusból 38 típus bekötve; további 169 típushoz a PNG már megvan** |

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

## Forgatómotoros kiegészítések integrációja

A három fekete házas szelep a katalógusba és a két rajzolóba bekötve; a katalógus így 320 típusú. Az alapképek száma változatlanul 209. A korábbi, 317 típusra vonatkozó gyártási táblázat mellé három új, saját PNG-vel regisztrált típus került; a két szürke házas képi változat regisztrációja továbbra is hátravan. A futásidejű grafikával bekötött típusok száma 18, külön forrásaik száma 15. Ez kódállapot, nem tesztelt készülékes eredmény.

## Tartályok integrációja – második csomag

A villanybojler, zárt tágulási tartály, hidrofor és kombinált tároló meglévő forrása bekötve. A légszelepek és zárt szervizfedelek nem csatlakozók. A villanybojler két alsó vízcsonkot, a tágulási tartály és hidrofor egy-egy alsó vízcsonkot, a kombinált tároló négy oldalsó vízcsonkot kapott. Forgó animáció egyikre sem kerül. A csonkok vizuális illeszkedésének szerkesztős és készülékes próbája még szükséges.

A katalógus továbbra is 320 típusú, az elkészült források száma 209; a bekötés 22 típust fed le 19 külön PNG-ből. A tárolócsoportban így 9 típushoz 8 forrás van bekötve. Ez kódállapot, a tesztek és az Actions ellenőrzése továbbra is halasztva.

## Tartályok integrációja – harmadik csomag

A fűtési membrános és ivóvizes tágulási tartály, nyitott tágulási tartály és esővíztartály saját PNG-je bekötve. A membrános tartályokon csak a vízoldali csatlakozás köthető: a fűtésin alul, az ivóvizesen felül; az előtöltő légszelep kimarad. A nyitott és esővíztartály két-két külső csatlakozási pontot kapott, a belső úszószelep és a fedél nem külön vízcsonk. Nincs rájuk rajzolt forgó animáció.

Összesen 26 típushoz 23 külön forrás van bekötve; ebből 13 tárolótípushoz 12 PNG. A teljes forráskészlet változatlanul 209 képes. A tesztfuttatás, fordítás, Actions-ellenőrzés és fizikai illeszkedéspróba még hátravan.

## Tartályok integrációja – negyedik csomag

Az indirekt, tartály-a-tartályban, szolár és rozsdamentes HMV-tároló saját képe bekötve. Az indirekt, belső tartályos és rozsdamentes változat négy-négy, a szolártároló hat külső csatlakozási pontot kapott. A belső spirál rajza nem alakít ki beégetett külső csőkapcsolatot. A hőmérő, érzékelőhüvely és szervizfedél nem vízcsatlakozás; egyik képre sem kerül forgó animáció.

A bekötés összesen 30 típust fed le 27 külön PNG-ből; a tárolócsoportban 17 típushoz 16 forrás használható. A teljes kész PNG-forráskészlet továbbra is 209 képes. A generálás elkészült, de tesztek, fordítás, Actions-ellenőrzés és készülékes vizuális próba még nem történt.

## Tartályok integrációja – ötödik csomag

A szögletes puffer, rétegtárolós puffer, víztartály és kondenzátumgyűjtő négy meglévő grafikája bekötve. A szögletes puffer négy, a rétegtároló három szinten hat külön csatlakozót kapott. A víztartály egy látható alsó kivezetése köthető; a zárt fedél nem port. A kondenzátumgyűjtő két oldalsó és egy felső csonkja külön pont. Nincs külső összekötés vagy forgó réteg.

A bekötés 34 típust fed le 31 külön PNG-ből; a tárolócsoportban 21 típushoz 20 forrás van bekötve. A teljes forráskészlet változatlanul 209 képes. Tesztfuttatás, fordítás, Actions-ellenőrzés és készülékes vizuális próba még hátravan.

## Tartályok integrációja – hatodik csomag

A sóoldattartály, föld alatti ciszterna és hőszivattyús vízmelegítő bekötve, így a tárolócsoport mind a 24 típusa saját PNG-t használ, 23 külön forrásból. A sóoldattartály egy felső oldalsó kivezetést, a ciszterna két oldalsó végpontot kapott; a zárt fedelek kimaradnak. A vízmelegítőn két felső légcsatlakozó és két alsó oldalsó vízcsonk külön pont. A szerkesztő nem ellenőrzi a médiumok egyezését, és nem köt össze automatikusan semmilyen pontot. A ventilátor zárt burkolata miatt csak az előlapi állapotjelző animált, külső forgó lapát nincs.

Az összesített integráció 37 típust fed le 34 külön PNG-ből. A teljes katalógus 320 típusú, az elkészült forráskészlet változatlanul 209 képes. A fenti korábbi csomagbejegyzések történeti állapotok. A tesztek, fordítás, Actions-ellenőrzés és fizikai illeszkedéspróba továbbra is hátravan.

## Szivattyúváltozatok integrációja – első csomag

A végszívású, hosszú tengelykapcsolós és két többfokozatú (álló/fekvő) szivattyú meglévő képe bekötve. Két külön vízcsonkot kaptak; a villamos doboz és tengelykapcsoló nem vízcsatlakozó. A zárt burkolat miatt motorra helyezett állapotjelző van, külső rotor nincs. A jelző a meglévő futásidejű aktivitási logikát követi (aktív pulzálás, kikapcsolt és ismeretlen állapot).

Összesen 41 típushoz 38 külön forrás van bekötve, ebből 14 szivattyútípushoz 12 kép. A teljes PNG-forráskészlet 209 képes. Teszt, fordítás, Actions-ellenőrzés és fizikai illeszkedéspróba továbbra is hátravan.
