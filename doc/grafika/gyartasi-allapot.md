# Képkészítés állapota – 2026. november 5.

A gyártás közben a végleges, kicsinyített forrásképek kerülnek fel kis csomagokban. A katalógushoz illesztés, a forgórészek regisztrálása és az összevont ellenőrzés a képkészlet elkészülte után következik. A forráskép megléte nem jelenti, hogy a firmware már ezt jeleníti meg.

| Csoport | Külön PNG-forrás | Lefedett katalógustípus | Állapot |
| --- | ---: | ---: | --- |
| Szivattyúk első csomagjai | 8 | 10 | Bekötve; további szivattyúváltozatok még készülnek |
| Tárolók és HMV | 23 | 24 | Minden forrás elkészült; első 4 forrás/5 típus bekötve |
| Hőtermelők, hőszivattyúk és hűtők | 31 | 31 | Minden forrás elkészült; bekötés később |
| Hőleadók | 0 | 0 | Világos/sötét csoportlátványterv elkészült |
| **Összesen** | **62** | **65** | **15 típus bekötve; további 50 típushoz a PNG már megvan** |

Az induló katalógus 317 típust tartalmaz. A még hiányzó egyedi források száma 252. A három valódi közös kialakítás: a fűtési és szolár keringető a keringetőszivattyú képét, az egyspirálos tároló a HMV-tároló képét használja. Ezekkel a teljes készlet várhatóan 314 külön PNG-ből áll.

## Csoportlátványtervek

- [01 – Szivattyúk](latvanytervek/01-szivattyuk.jpg)
- [02 – Tárolók és HMV](latvanytervek/02-tarolok-hmv.jpg)
- [03 – Hőtermelők](latvanytervek/03-hotermelok.jpg)
- [04 – Hőleadók](latvanytervek/04-holeadok.jpg)

## Műszaki felülvizsgálat és hátralévő bekötés

A távhő-fogadóállomás első rajzának hibás, a primer és szekunder oldalt külsőleg összekötő csövezése ki lett cserélve. A javított képen négy külön ág csatlakozik a hőcserélőhöz; a motoros szelep a primer visszatérő ágon van. A vezérlő vezetékei vékony fekete elektromos kábelek.

Az abszorpciós hűtő illusztrációja melegvíz-hajtású kialakítást jelöl: külön hőforrás-, hűtővíz- és hűtöttvíz-csatlakozópárral. A külső köröket nem köti össze díszcső. Gyártói tájékozódási alap: [Yazaki – vízhajtású abszorpciós hűtők](https://www.yazakienergy.com/waterfiredfeatures.htm). Az ikon nem gyártói szerelési rajz.

A ventilátoros gépek alapképein nincs beégetett lapát. A készlet bekötésekor a két- és háromventilátoros, illetve a felülnézetben ellipszisként látszó ventilátorokhoz külön mozgó réteg és pontos vászonkoordináta szükséges. A ventilátor nélküli beltéri és abszorpciós gépek nem kaphatnak általános forgó jelzést.

A Gitben a futásidejű források 160×160 pixeles, átlátszó PNG-k. A nagy munkaképek nincsenek verziózva. A csoportlátványtervek legfeljebb 1280×960 pixeles dokumentációs képek, nem részei az SD-csomagnak.

A felhasználó kérésére a képkészítés közben nem futtatunk teszteket és nem vizsgáljuk az Actions eredményeit; a képcsomag-commitok `[skip ci]` jelölést kapnak. Az összevont teszt, fordítás és grafikai integráció ellenőrzése még hátravan. A fizikai P4-mérés ettől külön feladat marad.
