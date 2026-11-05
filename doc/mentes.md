# Mentés és visszaállítás

A **Mentés** oldalon két külön fájltípus használható.

| Művelet | Tartalom | Felhasználás |
| --- | --- | --- |
| Sémaexport | Oldalak, elemek, csövek, formázás és Loxone-párosítások; hitelesítő adatok nélkül | Ügyfél rajzának elkérése, hibakeresés; import a szerkesztőben |
| Titkosított eszközmentés | Az összes oldal és a teljes mentett beállítás, a Wi-Fi és Loxone jelszavával együtt | Azonos vagy másik kijelző teljes konfigurációjának visszaállítása |

Az eszközmentésben nincs firmware, grafikai csomag, statisztika, készülékhez kötött NVS-kulcs, beállító AP-kulcs vagy Loxone munkamenettoken. A szükséges firmware-t és grafikát külön kell telepíteni. A mentés nem kötődik az eredeti SD-kártyához vagy készülékkulcshoz; működő SD-állapottárú célkészülék szükséges hozzá. A számítógépes előnézet az eszközmentést letiltja, kriptográfiát nem szimulál.

## Használat

1. Nyisd meg a készülék webfelületének Mentés oldalát.
2. Adj meg külön mentési jelszót kétszer, majd töltsd le a titkosított JSON-fájlt. Minimum 12 Unicode-karakter, maximum 128 UTF-8 bájt; a szóközök és az ékezetek számítanak. A jelszót a készülék nem tárolja, elvesztése esetén a fájl nem állítható vissza.
3. Visszaállításhoz válaszd ki a fájlt és írd be a hozzá tartozó mentési jelszót. A **Visszaállítás és újraindítás** az összes jelenlegi oldalt és beállítást lecseréli.
4. Sikeres tartós írás után a készülék újraindul. A visszaállított Wi-Fi-, IP-, nyelvi és Loxone-beállítások lépnek életbe; szükség lehet az új címen történő megnyitásra.

A titkosítás/dekódolás ideje alatt a felület várakozást jelez és letiltja a két műveletet, és szünetelteti a webes háttérlekérdezéseket. A firmware a kulcsszármaztatás idejére alacsony taskprioritást használ; eközben nincs kijelző- vagy állapottárzár. A P4 tényleges futásidejét és az animáció folyamatosságát fizikai próbával még mérni kell.

## Védelmek és korlátok

A hordozható fájl AES-256-GCM titkosítású. A 256 bites kulcsot a Mbed TLS PBKDF2-HMAC-SHA256 függvénye származtatja 600 000 iterációval és mentésenként véletlen 16 bájtos sóval. A GCM nonce 12 bájt, a hitelesítő tag 16 bájt; rögzített verziózott AAD védi a formátum értelmezését. A fájl csak a verziót, a KDF paramétereit, a sót, a nonce-ot, a taget és a Base64 kódolt titkosított tartalmat mutatja. Ismeretlen algoritmust, verziót, iterációszámot és többletmezőt a firmware elutasít; a fájl nem kényszeríthet tetszőleges költségű KDF-futtatást.

Hibás jelszó vagy sérült hitelesítés esetén nincs állapotváltozás és újraindítás. A dekódolt projekt és beállítás teljes modellvalidáláson megy át. A kliens a Mentés oldal betöltésekor olvasott projektrevíziót küldi; közben másik szerkesztőből mentett projekt esetén konfliktus keletkezik. Ilyenkor töltsd újra az oldalt, és ellenőrizd az aktuális állapotot.

Az oldalak és a beállítások **egyetlen** kétpéldányos SD-journal műveletben cserélődnek. Csak sikeres tartós írás után változik a RAM-állapot és nő a projektrevízió. Írási hiba esetén mindkét korábbi állapot megmarad. A beállításokat a célkészülék saját NVS-kulcsával titkosítjuk újra. Az SD fizikai áramelvételi viselkedését ez nem helyettesíti: megszakításpróba még szükséges.

Frissítés vagy függő újraindítás alatt az eszközmentés nem indítható. A HTTP végpontokra ugyanaz a Host/Origin és írási fejlécvédelem vonatkozik, mint a többi beállításra. A helyi HTTP-forgalom továbbra sem TLS-titkosított: a mentési jelszó átvitele megbízható helyi hálózatot feltételez. A titkosított fájl védelme nem hálózati titkosítás.

## Ellenőrzés

A natív teszt valódi, a firmware-rel azonos Mbed TLS-forrást használ. Független PBKDF2-vektort, véletlen só/nonce eltérést, helyes visszafejtést, hibás jelszót, só/nonce/tag/adat módosítását és méretkorlátot ellenőriz. Az állapottárteszt validálási hibát, revíziókonfliktust és írás közben eltűnő SD-könyvtárat vizsgál. A HTTP-teszt külön NVS-kulcsú célkészüléket modellez, ellenőrzi a visszaállítást, újraindításkérést és a nyilvános válaszok titokmentességét. A Chromium-próba a jelszóismétlést, a fájlletöltést és a hibás/sikeres visszaállítás kezelőfelületét ellenőrzi; a böngészős API-válaszok helyettesítettek, a kriptográfiát a natív teszt méri.
