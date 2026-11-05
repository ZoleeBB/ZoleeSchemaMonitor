# Grafikai készlet és képméretek

A kiválasztott irány a **B: finoman térhatásos, egységes műszaki illusztráció**. A véglegesítés csoportonként történik: világos/sötét látványterv, felhasználói ellenőrzés, egyedi elemképek, majd bekötés és animációpróba. A látványterv nem vágólapként használt sprite sheet; a végleges elemképek külön, átlátszó vásznon készülnek.

## Első csoport: szivattyúk

**Állapot: látványterv, még nem végleges futásidejű grafika.**

![Szivattyúk – világos és sötét látványterv](latvanytervek/01-szivattyuk.jpg)

A két panelen ugyanaz a nyolc fizikai kialakítás látszik. Balról jobbra, soronként:

| Hely | Kialakítás | Katalógusazonosító |
| --- | --- | --- |
| 1 | Karimás keringető | `circulation-pump`, fűtési alkalmazás: `heating-pump`, szolár alkalmazás: `solar-pump` |
| 2 | Kompakt bronz HMV-keringető | `dhw-pump` |
| 3 | Hosszú, csőkútba merülő kútszivattyú | `well-pump` |
| 4 | Fogantyús merülőszivattyú | `submersible-pump` |
| 5 | Vízszintes, többfokozatú nyomásfokozó | `booster-pump` |
| 6 | Széles nyomócsonkos szennyvízszivattyú | `sewage-pump` |
| 7 | Előszűrős medenceszivattyú | `pool-pump` |
| 8 | Perisztaltikus vegyszeradagoló | `dosing-pump` |

A fűtési és szolár alkalmazás azonos fizikai keringetőt is használhat; nem kap mesterségesen eltérő szivattyúházat pusztán a neve miatt. A típus és a felirat továbbra is külön katalóguselem.

A látványtervben szereplő forgórész csak a tervezett megjelenést jelzi. A végleges állóképbe nem kerülhet beforgatott animáció: a mozgó réteget a megjelenítő külön rajzolja. A középpontot, a csonkok helyét és az animáció fajtáját minden eltérő fizikai kialakításhoz külön kell beállítani; a jelenlegi általános pumpa-overlay nem tekinthető ezekre véglegesnek.

## Mi kerül a Gitbe?

| Cél | Hely | Formátum és méret | Fájlkorlát |
| --- | --- | --- | --- |
| Jóváhagyott futásidejű elemkép | `assets/elements/` | Átlátszó PNG, regisztrált 160×160 vászon | 128 KiB/kép |
| Csoportos dokumentációs látványterv | `doc/grafika/latvanytervek/` | Legfeljebb 1280×960, aránytartó JPEG vagy PNG | 2 MiB/kép |
| Nagy munkakép, félkész változat | `artwork-work/` vagy repón kívül | Nem verziózzuk | – |

A szivattyús terv tényleges tárolt mérete **1280×853**, körülbelül **176 KiB**. A nagy generált munkakép nem kerül a repóba. A dokumentációs JPEG nem része a panel grafikai csomagjának, nem foglal kijelzőmemóriát.

A 160×160 a jelenlegi PNG-készlet forrásvászna, nem az elem szerkesztőbeli méretkorlátja. A rajzra helyezett elem továbbra is méretezhető. Nagyított elemnél a raszter élességét valós panelen még ellenőrizni kell; nagyobb forrásméret csak mért igény és a memóriahatás felülvizsgálata után vezethető be.

## Előkészítés és automatikus ellenőrzés

```sh
python3 -m pip install -r tools/requirements-assets.txt
python3 tools/prepare_image.py artwork-work/pump.png assets/elements/light/circulation-pump.png --role element
python3 tools/prepare_image.py artwork-work/pumps-sheet.png doc/grafika/latvanytervek/01-szivattyuk.jpg --role preview
```

Az elemforrás négyzetes legyen, legalább 160×160 méretű. A segédprogram nem vág körbe, nem nyújt, nem nagyít fel kis forrást; a teljes vászon regisztrációja megmarad. LANCZOS kicsinyítést alkalmaz, megtartja az alfát és eltávolítja a munkametaadatokat. JPEG csak teljesen fedő dokumentációs látványtervhez engedélyezett.

Minden Gitbe kerülő rasztert pontos útvonallal, céllal és pixelmérettel fel kell venni a `catalog/image-sizes.json` jegyzékbe. Új felbontás nem engedélyezhető egyszerűen egy nagyobb szám beírásával: a program külön szerepkorlátot is ellenőriz.

```sh
git add catalog/image-sizes.json doc/grafika/latvanytervek/01-szivattyuk.jpg
python3 tools/image_policy.py
```

Az ellenőrzés a **Git-index tartalmát** olvassa, nem a munkapéldányt. Így ugyanazt vizsgálja helyben és a CI-ben, ami a commitba kerül. A nem jegyzett képet, a hiányzó jegyzett fájlt, az eltérő pixelméretet, a túl nagy fájlt és a hibás PNG-fejlécet elutasítja. A runtime elemkép csak PNG lehet; dokumentációban megengedett a kis JPEG is. Az SVG-források vektorosak, nem tartoznak a raszterjegyzékbe.

A generált `web/graphics/` és `dist/graphics/` készletek továbbra sem verziózott fájlok. A jelenlegi generátor 160×160 PNG-ket állít elő; a jóváhagyott egyedi képek átvételét és OTA-forrásazonosítóba kötését a későbbi bekötésnek kell biztosítania. Ebben a változásban a futásidejű rajzcsaládok és az OTA grafikai azonosítója nem változott.
