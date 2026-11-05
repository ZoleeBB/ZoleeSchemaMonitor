# Átemelések nyilvántartása

Forrás: `ZoleeBB/ZoleeEnergyFlowDisplay`, commit `2dabb18fe493280a62068ab75b43a6313b9440a1`.

A webes közös CSS, a P4 kijelzőpuffer burkolója, az adapter buildmásolatán végzett javítások és az ESP-Hosted SDIO javítások ebből a pillanatképből származnak. Az új program ezekhez saját adatmodellt és felületet épít. A forrásprojekt kódja a felhasználó saját projektje, és kifejezett kérésre emeljük át.

A grafikai stílus 2026-11-04-én kiválasztott iránya: **B**, térhatásosabb műszaki elemek. A bemutatott kép stílusreferencia; a futásidejű készlet saját, determinisztikus vektorforrásból épül, külön animációs résszel.

A diagnosztikai ablak CPU idle-mérése, memória-/SD-sávjai és 15 soros naplója a fenti commit `app_diagnostics.c`, `app_template_ui.inc` és `app_events.c` működési mintájából adaptált. Az új projekt saját kártyanézetet és rögzített eseménykódokat használ; BLE/energia/kaputelefon mezők nélkül. A mintavétel külön taskban fut, a közös JSON a még nem mért értékeket `null`-ként adja vissza.
