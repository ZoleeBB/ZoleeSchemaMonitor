# MS1 – kipróbálható teljes első változat

Állapot: **nyitott**. Az alapozó commitok és az automatikus ellenőrzések sikere nem zárja a mérföldkövet.

## Elfogadási feltételek

- [ ] P4 firmware sikeresen fordul, flash/OTA partícióméretek ellenőrizve.
- [ ] Indulás, Wi-Fi, közvetlen Gen2/17.x Loxone, hitelesítés és tokenmegújítás fizikai eszközön működik.
- [ ] Legfeljebb 10 oldal szerkesztése, tartós mentése, oldalak/projektek exportja és importja működik.
- [ ] Húzás, méretezés, forgatás, csőrajzolás, csatlakozópontok, szöveg- és értékmezők működnek.
- [ ] Helyiség/funkció szűrés és minden elem Loxone-párosítása működik.
- [ ] Szivattyú aktivitási elsőbbség és teljesítményhatárok tesztelve.
- [ ] Jóváhagyott grafikai stílus és **teljes** HVAC/medence/öntözés/vízellátási készlet elkészült.
- [ ] Világos/sötét grafika, 0/90/180/270° kép és érintés fizikai panelen ellenőrizve.
- [ ] Oldallapozás és átemelt diagnosztikai ablak működik.
- [ ] Kezdőlap, támogatási blokk új ikonnal, beállítások, diagnosztika, mentés/visszaállítás és OTA működik HU/EN nyelven.
- [ ] Hash szerinti differenciális grafikafrissítés; megszakítás, újrapróbálás és firmware rollback tesztelve.
- [ ] Main merge release-t készít, firmware és manifest elérhető, CI-artefaktumok megőrzése 7 nap.
- [ ] README, használati/telepítési/fejlesztői dokumentáció és első PR-verziódokumentum kész.
- [ ] 150 elemes, sok aktív csöves oldal 20 FPS körüli célját képkockaidőkkel fizikai P4-en igazoltuk.
- [ ] Legalább 24 órás járatás, memória-mélypontok, kapcsolatvesztés, SD-hiba és újraindítás tesztelve.

## Sorrend

1. Követelmények, formátum és hordozható állapotlogika.
2. Grafikai stílusminták bemutatása; közben működési szerkesztő és tárolási/frissítési alapok. **A B irány kiválasztva.**
3. P4-platform és Loxone/web integráció, semleges helyőrzőkkel.
4. Jóváhagyott teljes grafika, natív animáció, diagnosztika.
5. CI/release, dokumentáció, készülékes regresszió és teljesítménymérés.

A saját fűtés–HMV rendszer első próbaoldal; nem helyettesíti a teljes készletet.
