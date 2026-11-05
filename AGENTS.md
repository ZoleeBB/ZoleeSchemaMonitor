# ZoleeSchemaMonitor – fejlesztési szabályok

- A fejlesztési ág `MS1`. A mérföldkő csak a `doc/ms1.md` összes elfogadási feltételének teljesülése után zárható.
- A kézzel írt kód olvasható, négy szóközös behúzású legyen. A magyarázó kommentek magyarul készüljenek. Generált/minimalizált erőforrásokat a generátoruk kezel.
- Célhardver: Waveshare ESP32-P4-WIFI6-Touch-LCD-10.1, SKU 33150, önálló Wi-Fi-kapcsolat, Loxone Gen2/17.x. Nincs master/slave, MQTT, kaputelefon vagy általános otthonvezérlés.
- Legfeljebb 10 oldal, 150 elem/oldal a tervezett keret. A 20 FPS cél fizikai mérésig nem állítható igazoltnak.
- Minden felhasználói felület HU/EN. Tájolás 0/90/180/270°, a natív panel 800×1280; 90/270° esetén 1280×800. A tájolásváltás ellenőrzött újraindításkor lép életbe.
- Grafikai stílusmintát a teljes grafikai készlet elkészítése előtt meg kell mutatni a felhasználónak. Jóváhagyásig csak semleges műszaki helyőrzők használhatók a működés tesztelésére.
- Új elemfajtát csak firmware-frissítés vezethet be. A grafikák SD-n, SHA-256 szerint címezve tárolhatók; a sémaimport nem tartalmazhat futtatható kódot, tetszőleges SVG-t vagy hitelesítő adatot.
- Ismeretlen Loxone-állapot nem egyenlő nullával/kikapcsolttal. Az explicit aktivitásjel elsőbbséget élvez a teljesítményből származtatott aktivitással szemben.
- Frissítésnél csak hiányzó vagy hibás tartalmú grafikát töltsünk le; félbeszakadáskor a korábbi teljes készlet maradjon használható. A firmware és a grafikai készlet kompatibilitását aktiválás előtt ellenőrizzük.
- A kijelzőzár alatt nincs SD/hálózati munka. Változatlan szöveget és stílust ne írjunk vissza minden ciklusban. Animációban nincs dinamikus memóriafoglalás.
- A belső RAM és a DMA-képes memória külön korlát. Az átemelt P4 pufferjavításokat és a 32 KiB belső tartalékot mérések nélkül ne módosítsuk.
- Import/mentés/frissítés tranzakciós legyen; a sikerüzenet tartós mentést jelentsen. Hibánál a korábbi érvényes adat megmarad.
- Minden érdemi változáshoz futtassuk a célzott teszteket. A gazdagépes teszt, CI-fordítás és fizikai panelteszt eredményét külön nevezzük meg.
- README: rövid alapadatok és linkek a `doc/` alá. PR-zárásonként a verzióváltozások a `doc/verziotortenet/` könyvtárba kerüljenek.

- A nagy felbontású grafikai munkaképek nem kerülhetnek a Gitbe. Raszter csak a `catalog/image-sizes.json` jegyzékben rögzített pixelmérettel és célból vehető fel; ellenőrzés: `python3 tools/image_policy.py` (a Git-indexet olvassa). Csoportonként külön világos/sötét látványterv szükséges a végleges képek bekötése előtt.
