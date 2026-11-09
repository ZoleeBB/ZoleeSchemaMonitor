# MS1 – rövid hátralévő lista

Az MS1 nyitott. A grafikai készlet (333 típus), a szerkesztő, osztó-gyűjtők, animációk, igazítás, mentés és gazdagépes regresszió elkészült. Az öntözési animációt tartalmazó [CI és P4-fordítás sikeres](https://github.com/ZoleeBB/ZoleeSchemaMonitor/actions/runs/42357491467); a main-kiadás ezen az ágon szándékosan nem futott le.

| Hátralévő terület | Konkrét következő igazolás vagy feladat |
| --- | --- |
| Szoftveres végek | [SD-helyreállítás vezérelt újraindítással](mentes.md) elkészült, fizikai kártyapróba szükséges. A webszerkesztő a natív betűkészlet karakterlépéseivel tördeli a szöveg- és értékmezőket. A HU/EN sortörés, betűkép és méret végső vizuális ellenőrzése fizikai panelen szükséges. |
| Valódi P4 és Loxone | Indulás, Wi-Fi és helyreállító AP; Gen2/17.x bejelentkezés, TLS, tokenmegújítás, megszakadás/újracsatlakozás; grafika, fényerő, mind a négy tájolás és érintés; szerkesztés–mentés–újraindítás–import/export. |
| Terhelés és tartós üzem | [Hat terhelési tesztprojekt, animációs próbaoldal és gazdagépes mérések](../tests/schemas/README.md) elkészült. A készülékes diagnosztika méri az LVGL-képkockákat. A [24 órás adatgyűjtés és készülékes próbalap](verziotortenet/ms1-kiadasi-tervezet.md) rendelkezésre áll. Hátravan a 150 elemes, sok aktív csöves oldal képkockaidejeinek mérése és a 20 FPS cél ellenőrzése; legalább 24 órás járatás, memória-mélypont, SD-hiba és hálózati hiba. |
| OTA és első kiadás | Valódi letöltésből végzett megszakítás/újrapróbálás, sérült grafika javítása, firmware rollback és a régi grafika megtartása; első main-merge kiadás és a publikált firmware/manifest letölthetősége. |
| Lezárás | Az első PR-verziódokumentum tervezete elkészült. A fizikai mérési jegyzőkönyv, a kiadási verzió és csomagok rögzítése után következhet a teljes [elfogadási lista](ms1.md) lezárása. |

A 2026. november 9-i gazdagépes ellenőrzésen a hét projekt mentése és új folyamatból visszaolvasása, valamint a 21 oldal LVGL-rajzolása mindkét felbontáson sikerült. Az [animációs próbaoldal és felvétel](../tests/schemas/README.md#animált-eszközök-egy-oldalon) 68 készüléket mutat. A külön képpufferteszt 67 animált katalógustípust és 33 forgástengelyt vizsgált négy tájolásban és öt jelállapotban.

A fizikai próbákhoz célpanel, SD-kártya és a Miniserver elérése kell. Ezek eredményét nem helyettesíti CI, szimulált adat vagy virtuális LVGL-kijelző. A már működő funkcióknál ezért több elfogadási jelölő még nyitott.

Külön nyilvántartott további bővítés a Loxone szöveges eseményeinek kezelése és a régi grafikai készletek tárkarbantartása. A jelenlegi értékmezők numerikusak; a régi grafikai kötések megőrzése jelenleg a rollback biztonságát szolgálja.
