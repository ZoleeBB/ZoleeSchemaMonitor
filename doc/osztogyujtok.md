# Termofejes osztó-gyűjtők és forgatás

A szerkesztőben a **Termofejes osztó-gyűjtő (2–16 kör)** elemet válaszd. Egyetlen paraméterezhető elemtípus minden egész körszámot támogat 2 és 16 között. A két sárgaréz sínen minden körhöz egy átfolyásmérő, egy termofej és két menetes csonk tartozik.

| Beállítás | Működés |
| --- | --- |
| Felhasználás | Fűtés, padlófűtés, mennyezetfűtés vagy mennyezethűtés. A felirat megkülönbözteti a változatokat; a hűtés előremenője kék. |
| Körszám | 2–16; változtatáskor az első megmaradó körök saját értéke és Loxone-hozzárendelése megmarad. A szélesség a körszámhoz igazodik, utána szabadon méretezhető. |
| Csőcsonkok és bekötési pontok | Kapcsolható csőbekötés; bekapcsolva két főcsatlakozás és minden körhöz egy előremenő, egy visszatérő végpont. 16 körnél 34 pont. |
| Körönkénti százalék | Kézzel 0–100%, vagy külön Loxone-állapot; a hozzárendelőben skálázás/eltolás is megadható. 0–1 forráshoz 100-as skála szükséges. |
| Sötét jelző | Ismert, 0%-os nyitási érték. |
| Zöld jelző | A kör ismert nyitási értéke szigorúan 0% fölött van. 0 és 0,5% között a felirat `<1%`, így pozitív jelhez nem társul 0%-os felirat. |
| Borostyán jelző és `—` | A hozzárendelt Loxone-érték hiányzik, érvénytelen vagy megszakadt a kapcsolat. A többi kör jelzése ettől független. |

A Loxone-értéket 0–100% közé korlátozzuk. A kézi érték hiányzó adatkapcsolat mellett is ismert marad; élő forráshoz nem használunk kézi tartalékértéket. A jelző a nyitási értéket szemlélteti, nem igazol mechanikai végállást vagy tényleges vízáramlást. A forrás hozzárendelése csak olvasás, nem szelepműködtetési parancs.

A csőcsonkok kikapcsolása vagy a körszám csökkentése leválasztja a megszűnő végpontokra kötött csöveket, de megtartja a megrajzolt csővonalat. A megmaradó csatlakozások követik a mozgatást, méretezést és forgatást. Az import mind a böngészőben, mind a firmware-ben elutasítja a nem létező vagy kikapcsolt végpontra mutató kötést.

## Forgatás

Minden eszköz, szöveg, értékmező és cső **0°, 90°, 180° vagy 270°** állásba forgatható a tulajdonságpanelen. A cső forgatása a töréspontjait forgatja el, és leválasztja a két korábbi kötést. A művelet visszavonható. Az oldalon el nem férő csőforgatást a szerkesztő elutasítja; az eszközök helyzetét szükség szerint a vászonra igazítja. A körszámok és a körönkénti százalékok olvashatóságuk érdekében vízszintesek maradnak.

## Grafika és ellenőrzés

A bal és jobb végdarab között egy 56 pixel széles, generált termékfotóból kivágott modult ismétlünk. A grafikai csomag készítésekor minden körszámhoz elkészül a fűtési és hűtési PNG. A képek szélessége 208–992 pixel, magasságuk 224 pixel. A szerkesztő és az LVGL ugyanazt a PNG-t illeszti aránytartóan az elem keretébe. A jelző és a nyitási százalék a fehér termofejre kerül. Kis keretben a felirat is arányosan kicsinyül.

Bekapcsolt csőbekötésnél a színes csonkok a menetes végektől indulnak. A két főcsatlakozás a bal szélen, a körök előremenő és visszatérő pontjai a megfelelő csonk végén vannak. A rajzoló és a portok ugyanazt a középpont körüli forgatást és méretarányt használják. Az új firmware-hez a hozzá tartozó SD-grafikai csomag szükséges.

![Osztó-gyűjtők világos és sötét háttéren](grafika/latvanytervek/08d-parameterezheto-osztok.jpg)

Az előnézet a szerkesztő tényleges rajzelemeiből készül: `python3 tools/build_manifold_preview.py`. A 331 korábbi PNG-forrást tartalmazó [vizuális felülvizsgálat](grafika/vizualis-felulvizsgalat.md) ettől külön nyilvántartás.

A JavaScript-tesztek minden körszámot és felhasználást, a körönkénti állapotot és a csőforgatás pixelgeometriáját ellenőrzik. A C/JS validálás 214 paritásesete és a valódi LVGL-lel futó gazdagépes próba sikeres. A Chromium-próba ellenőrizte mind a 15 körszámot, a 34. port tényleges bekötését, a jelzőket, a forgatást, a csőleválasztást és a mentést/visszatöltést. Ezek gazdagépes ellenőrzések; fizikai P4-, Miniserver- és 20 FPS-mérés még szükséges.

A körönkénti futásidejű állapotok a P4 PSRAM-jában vannak; az animáció nem foglal dinamikus memóriát, és csak változott szöveget/színt ír vissza. A belső/DMA-pufferbeállítások és a 32 KiB tartalék változatlanok.

## Rögzített körszámú korábbi osztótípusok

A `manifold-2`–`manifold-10` és `manifold-12` stabil azonosítók megmaradnak. Ezek a típusok is a fotómodulokból összeállított képet kapják. Két főcsonk után a korábbi sorrendű visszatérő pontok, majd az előremenő pontok következnek. A külön körönkénti százalékhoz és termofejjelzéshez továbbra is a **Termofejes osztó-gyűjtő (2–16 kör)** elemet válaszd.
