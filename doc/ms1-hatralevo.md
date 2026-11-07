# MS1 – rövid hátralévő lista

Az MS1 nyitott. A grafikai készlet (333 típus), a szerkesztő, osztó-gyűjtők, animációk, igazítás, mentés és gazdagépes regresszió elkészült. Az öntözési animációt tartalmazó [CI és P4-fordítás sikeres](https://github.com/ZoleeBB/ZoleeSchemaMonitor/actions/runs/42357491467); a main-kiadás ezen az ágon szándékosan nem futott le.

| Hátralévő terület | Konkrét következő igazolás vagy feladat |
| --- | --- |
| Szoftveres végek | SD-visszatérés/újracsatolás és kártya nélküli helyreállítási út; szövegtördelés és betűméret vizuális egyezése a web és az LVGL között. A numerikus formázást a mostani javítás egységesíti. |
| Valódi P4 és Loxone | Indulás, Wi-Fi és helyreállító AP; Gen2/17.x bejelentkezés, TLS, tokenmegújítás, megszakadás/újracsatlakozás; grafika, fényerő, mind a négy tájolás és érintés; szerkesztés–mentés–újraindítás–import/export. |
| Terhelés és tartós üzem | 150 elemes, sok aktív csöves oldal képkockaidői és a 20 FPS cél tényleges mérése; legalább 24 órás járatás, memória-mélypont, SD-hiba és hálózati hiba. |
| OTA és első kiadás | Valódi letöltésből végzett megszakítás/újrapróbálás, sérült grafika javítása, firmware rollback és a régi grafika megtartása; első main-merge kiadás és a publikált firmware/manifest letölthetősége. |
| Lezárás | Végleges telepítési/használati leírás, első PR-verziódokumentum, mérési jegyzőkönyv, majd a teljes [elfogadási lista](ms1.md) lezárása. |

A fizikai próbákhoz célpanel, SD-kártya és a Miniserver elérése kell. Ezek eredményét nem helyettesíti CI, szimulált adat vagy virtuális LVGL-kijelző. A már működő funkcióknál ezért több elfogadási jelölő még nyitott.

Külön nyilvántartott további bővítés a Loxone szöveges eseményeinek kezelése és a régi grafikai készletek tárkarbantartása. A jelenlegi értékmezők numerikusak; a régi grafikai kötések megőrzése jelenleg a rollback biztonságát szolgálja.
