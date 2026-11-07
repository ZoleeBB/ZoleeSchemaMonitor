# Öntözési permetanimáció

A szórófejek meglévő aktivitás- és opcionális teljesítményforráshoz párosíthatók. Az ismert aktív állapot négy mozgó kék cseppet és zöld üzemjelzőt mutat. Ismert kikapcsoláskor a cseppek eltűnnek, a jelző szürke. Hiányzó vagy érvénytelen párosított adatnál nincs permet; a jelző borostyán, a készülék halványabb. Párosítás nélkül nincs automatikus szórás.

| Bekötött típus | Animáció |
| --- | --- |
| Rotoros szórófej | A fej felső nyílásától induló cseppek |
| Spray szórófej | A felső fúvókától induló cseppek |
| Mikroszórófej | A felső szórórészből induló cseppek |
| Forgó fúvóka | A fúvóka tetejétől induló cseppek |
| Kalapácsos szórófej | Az oldalsó fúvóka irányába induló cseppek |

A vízcseppek jelképes üzemjelzések: nem valós szórástávot, vízmennyiséget vagy gyártói szórásképet ábrázolnak. A buborékoló, csepegtető, tömlő és vezérlő nem kapott permetezéshez hasonló mozgást. A jóváhagyott PNG-k és kalibrált csőcsatlakozók változatlanok.

## Adatok és forgatás

Az explicit aktivitásjel dönt, pozitív teljesítmény mellett is kikapcsolhatja a permetet. Aktivitásjel hiányában az ismert teljesítmény alsó határ feletti értéke aktivál. A teljesítménytartomány az animáció sebességét is szabályozza, a meglévő korlátozással. Ismeretlen adatot nem tekintünk nullának.

A cseppek forráspontja és iránya a képhez rögzített. Az elem 0/90/180/270°-os forgatásával a teljes permetkép is elfordul. Az animáció a felső/oldalsó fúvókánál a kijelölt elem keretén túl is rajzolhat; a képernyő széle levágja a kilógó részt. A natív diagnosztikai nézet alatt a mozgás szünetel.

## Megvalósítás és ellenőrzés

A katalógus `spray` animációja a forráspontot, méretet és irányt rögzíti. A C- és JS-megjelenítő azonos relatív csepppályát használ. A natív oldal legfeljebb 150 fej × 4 csepp fix PSRAM-tárolóval dolgozik; az LVGL-objektumok csak jelenetépítéskor jönnek létre. Az állandó szórási és forgatási szögek trigonometrikus értékeit is jelenetépítéskor számoljuk ki; a natív cseppmozgatás ezeket használja. Az animációban nincs saját memóriafoglalás, SD- vagy hálózati művelet. A cseppobjektumok a képkockák között megmaradnak; inaktív állapotban csak állapotváltozáskor rejtjük el őket. A meglévő kijelzőpufferek és a 32 KiB belső tartalék beállítása változatlan.

Ellenőrzés: mind az öt típus valódi Chromiumban aktív/kikapcsolt/adathiányos állapottal és négy forgatással; közös C/JS pályaszámítás 120 bemenettel; natív LVGL-geometria, pozitív teljesítményt felülíró explicit kikapcsolás, teljesítményből aktivitás/sebesség, diagnosztikai szünet és 150 egyidejű szórófej. A szélsőséges fázis/irány és hibás paraméterek a hordozható mag tesztjeiben szerepelnek.

A gazdagépes kapacitáspróba nem P4-en mért képkockasebesség vagy memória-mélypont. A fizikai 20 FPS, SD-, érintés- és tartósüzemi vizsgálatok továbbra is az MS1 nyitott feltételei.
