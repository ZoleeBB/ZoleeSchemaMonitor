# Csatlakozópont-audit – 2026-11-07

333 típus teljes kézi áttekintése portjelölős kontaktlapokon. 246 típusnál 544 pont helye változott. A PNG-k változatlanok; a csonknyílások közepét a 160×160-as forrásvásznon rögzítettük. A radiator négy pontja: (11,53), (149,53), (11,109), (149,109).

A `catalog/port-calibration.json` az érvényes koordinátaforrás. A raszter SHA-256 lenyomata megakadályozza, hogy egy későbbi képcsere ellenőrzés nélkül megtartsa a régi pontokat. A korábbi regisztrációk portindexei megmaradtak; a szerkesztő betöltéskor, a natív rajzoló jelenetépítéskor újrailleszti a kötött végpontokat. A szabad végpontok és köztes törések megmaradnak.

A hátoldalon takart csatlakozások és a tengelyirányból mutatott karimák pontjai sematikus vetületek. Ezeknél a meglévő képen nem látható minden csonk; nem állítjuk, hogy látható nyíláshoz illeszkednek. Az érzékelők kábelei, rögzítőfülek, zárt sapkák és villamos csatlakozók nem új csőportok.

Az osztók új kalibrációja a sorban megadott, szélesebb forrásvászonra vonatkozik; a szabadon állítható osztó minden 2–16 körszámmal külön ellenőrzött.

Csővastagság: **1–30 kijelzőpixel**, csövenként a Tulajdonságok panelen. A felirat tartalmazza az egységet és a tartományt. Chromiumban 1/12/30 px rajzolás és tartós mentés, valódi LVGL-objektumon ugyanezek vonalvastagsága ellenőrzött. Minden katalógusport négy forgatásban, fekvő/álló vásznon kap geometriai ellenőrzést. Ez gazdagépes ellenőrzés, a fizikai panelpróbát nem helyettesíti.

| Típus | Ellenőrzés | Portok (forráspixel, indexsorrend) | Javítva |
| --- | --- | --- | --- |
| `gas-boiler` | Látható csonk / nyílás | 64,140; 96,140 | igen |
| `electric-boiler` | Látható csonk / nyílás | 66,137; 93,137 | igen |
| `solid-fuel-boiler` | Takart vagy vetületi csatlakozás | 40,112; 115.2,112 | — |
| `pellet-boiler` | Takart vagy vetületi csatlakozás | 40,112; 115.2,112 | — |
| `district-heating` | Látható csonk / nyílás | 19.2,39.2; 140,38.4; 19.2,118.4; 140,118.4 | — |
| `heat-pump` | Látható csonk / nyílás | 145,72; 145,90 | igen |
| `heat-pump-dual` | Takart vagy vetületi csatlakozás | 121.6,136; 132.8,136 | — |
| `chiller` | Látható csonk / nyílás | 139.2,87.2; 139,97 | igen |
| `air-conditioner-outdoor` | Látható csonk / nyílás | 144,81.6; 143,102 | igen |
| `heat-pump-indoor` | Látható csonk / nyílás | 72,13; 86,13 | igen |
| `hydrobox` | Látható csonk / nyílás | 57,139; 80,139; 101,139 | igen |
| `electric-heater` | Nincs csőport | — | — |
| `radiator` | Látható csonk / nyílás | 11,53; 149,53; 11,109; 149,109 | igen |
| `towel-radiator` | Látható csonk / nyílás | 34,147; 125,147 | igen |
| `convector` | Látható csonk / nyílás | 9,89; 9,98 | igen |
| `fan-coil` | Látható csonk / nyílás | 12,97; 12,106 | igen |
| `air-conditioner-indoor` | Takart vagy vetületi csatlakozás | 140.8,101.6; 145.6,108 | — |
| `underfloor-heating` | Látható csonk / nyílás | 19,36; 141,120 | igen |
| `wall-heating` | Látható csonk / nyílás | 46,138; 114,138 | igen |
| `ceiling-heating` | Látható csonk / nyílás | 147,74; 145,90 | igen |
| `ceiling-cooling` | Látható csonk / nyílás | 147.2,67.2; 147.2,97.6 | — |
| `heating-circuit` | Látható csonk / nyílás | 55,145; 105,145 | igen |
| `cooling-circuit` | Látható csonk / nyílás | 11,46; 11,122 | igen |
| `solar-collector` | Látható csonk / nyílás | 10,101; 147,52 | igen |
| `buffer-tank` | Látható csonk / nyílás | 42.4,53.76; 114.4,53.76; 42.4,104.8; 114.4,104.8 | — |
| `hot-water-tank` | Látható csonk / nyílás | 114.24,41.12; 114.24,120.8; 42.4,76.8; 42.4,108 | — |
| `single-coil-tank` | Látható csonk / nyílás | 114.24,41.12; 114.24,120.8; 42.4,76.8; 42.4,108 | — |
| `dual-coil-tank` | Látható csonk / nyílás | 109.6,41.12; 109.6,118.4; 48,51.52; 48,72; 48,88; 48,110.4 | — |
| `electric-water-heater` | Látható csonk / nyílás | 65,142; 93,142 | igen |
| `expansion-vessel` | Látható csonk / nyílás | 80,140 | igen |
| `hydrophore` | Látható csonk / nyílás | 80,140 | igen |
| `rainwater-tank` | Látható csonk / nyílás | 34,45; 131.2,129.6 | igen |
| `water-tank` | Látható csonk / nyílás | 136,130 | igen |
| `circulation-pump` | Látható csonk / nyílás | 11,95; 149,95 | igen |
| `red-ecm-circulation-pump` | Látható csonk / nyílás | 7.2,75.2; 152,75.2 | — |
| `green-ecm-circulation-pump` | Látható csonk / nyílás | 6.4,72; 153.6,72 | — |
| `heating-pump` | Látható csonk / nyílás | 11,95; 149,95 | igen |
| `dhw-pump` | Látható csonk / nyílás | 15,99; 145,99 | igen |
| `well-pump` | Látható csonk / nyílás | 78.4,7.2; 80,89.6 | — |
| `submersible-pump` | Látható csonk / nyílás | 72,134.4; 119,76 | igen |
| `booster-pump` | Látható csonk / nyílás | 14,83; 58,57 | igen |
| `sewage-pump` | Látható csonk / nyílás | 70,139; 128,97.6 | igen |
| `solar-pump` | Látható csonk / nyílás | 11,95; 149,95 | igen |
| `plate-exchanger` | Látható csonk / nyílás | 44,42.4; 80,43; 43,115; 80,115 | igen |
| `tubular-exchanger` | Látható csonk / nyílás | 8,80; 151,80; 47,52; 102,107 | igen |
| `hydraulic-separator` | Látható csonk / nyílás | 41,53; 121,53; 41,106; 121,106 | igen |
| `mixing-unit` | Látható csonk / nyílás | 11,61; 149,61; 12,120; 149,120 | igen |
| `manifold-2` | Fotómodulok · 208×224 | 4,99; 4,191; 84,217; 140,217; 84,124; 140,124 | igen |
| `manifold-4` | Fotómodulok · 320×224 | 4,99; 4,191; 84,217; 140,217; 196,217; 252,217; 84,124; 140,124; 196,124; 252,124 | igen |
| `manifold-6` | Fotómodulok · 432×224 | 4,99; 4,191; 84,217; 140,217; 196,217; 252,217; 308,217; 364,217; 84,124; 140,124; 196,124; 252,124; 308,124; 364,124 | igen |
| `manifold-8` | Fotómodulok · 544×224 | 4,99; 4,191; 84,217; 140,217; 196,217; 252,217; 308,217; 364,217; 420,217; 476,217; 84,124; 140,124; 196,124; 252,124; 308,124; 364,124; 420,124; 476,124 | igen |
| `manifold-10` | Fotómodulok · 656×224 | 4,99; 4,191; 84,217; 140,217; 196,217; 252,217; 308,217; 364,217; 420,217; 476,217; 532,217; 588,217; 84,124; 140,124; 196,124; 252,124; 308,124; 364,124; 420,124; 476,124; 532,124; 588,124 | igen |
| `manifold-12` | Fotómodulok · 768×224 | 4,99; 4,191; 84,217; 140,217; 196,217; 252,217; 308,217; 364,217; 420,217; 476,217; 532,217; 588,217; 644,217; 700,217; 84,124; 140,124; 196,124; 252,124; 308,124; 364,124; 420,124; 476,124; 532,124; 588,124; 644,124; 700,124 | igen |
| `electrothermal-manifold-4` | Látható csonk / nyílás | 12,55; 12,127; 44,72; 68,72; 93,72; 117,72; 44,143; 68,143; 93,143; 117,143 | igen |
| `ball-valve` | Látható csonk / nyílás | 26,97; 127,97 | igen |
| `shutoff-valve` | Látható csonk / nyílás | 27,120; 132,120 | igen |
| `check-valve` | Látható csonk / nyílás | 20,91; 141,91 | igen |
| `solenoid-valve` | Látható csonk / nyílás | 23,124; 136,124 | igen |
| `motor-valve-2` | Látható csonk / nyílás | 28,116; 133,116 | igen |
| `motor-valve-3` | Látható csonk / nyílás | 24,95; 131,95; 80,143 | igen |
| `black-rotary-valve-3` | Látható csonk / nyílás | 12,72; 147,72; 80,136 | igen |
| `black-union-rotary-valve-3` | Látható csonk / nyílás | 13,71; 147,71; 79,135 | igen |
| `black-flanged-rotary-valve-3` | Látható csonk / nyílás | 15,73; 146,73; 81,135 | igen |
| `orange-motor-ball-valve-2` | Látható csonk / nyílás | 30,127; 98,113 | igen |
| `orange-motor-ball-valve-3` | Látható csonk / nyílás | 34,117; 104,110; 68,142 | igen |
| `spring-return-ball-valve-2` | Látható csonk / nyílás | 39,115; 98,115 | igen |
| `compact-motor-ball-valve-2` | Látható csonk / nyílás | 9,125; 141,125 | igen |
| `cartridge-zone-valve-3` | Látható csonk / nyílás | 20,106; 124,106; 65,147 | igen |
| `electrothermal-zone-valve-2` | Látható csonk / nyílás | 11.2,128; 148.8,128 | — |
| `compact-linear-control-valve-2` | Látható csonk / nyílás | 47,119; 124.8,118.4 | igen |
| `radiator-thermostatic-valve-straight` | Látható csonk / nyílás | 22,132; 138,132 | igen |
| `radiator-thermostatic-valve-angle` | Látható csonk / nyílás | 66,145; 127,108 | igen |
| `mixing-valve` | Látható csonk / nyílás | 21,71; 141,71; 80,133; 80,28 | igen |
| `diverting-valve` | Látható csonk / nyílás | 28,75; 133,75; 80,131 | igen |
| `thermostatic-valve` | Látható csonk / nyílás | 25.6,122.4; 136,122.4 | — |
| `balancing-valve` | Látható csonk / nyílás | 17,112; 143,112 | igen |
| `pressure-reducer` | Látható csonk / nyílás | 23,120; 138,120 | igen |
| `safety-valve` | Látható csonk / nyílás | 80,141; 120,86 | igen |
| `drain-valve` | Látható csonk / nyílás | 27,90; 122,124 | igen |
| `fill-valve` | Látható csonk / nyílás | 17,108; 144,108 | igen |
| `zone-valve` | Látható csonk / nyílás | 34,109; 128,109 | igen |
| `backflow-preventer` | Látható csonk / nyílás | 13,90; 146,90 | igen |
| `automatic-drain` | Látható csonk / nyílás | 24,105; 87,145 | igen |
| `strainer` | Látható csonk / nyílás | 14,61; 147,61 | igen |
| `dirt-separator` | Látható csonk / nyílás | 34,53; 125,53; 80,144 | igen |
| `magnetic-filter` | Látható csonk / nyílás | 24,53; 136,53; 80,144 | igen |
| `air-separator` | Látható csonk / nyílás | 25,79; 135,79 | igen |
| `automatic-air-vent` | Látható csonk / nyílás | 80,147 | igen |
| `water-filter` | Látható csonk / nyílás | 23,49; 136,49 | igen |
| `water-softener` | Látható csonk / nyílás | 32,27; 91.2,26.4 | igen |
| `fan` | Takart vagy vetületi csatlakozás | 20.8,83.2; 139.2,83.2 | — |
| `duct-fan` | Takart vagy vetületi csatlakozás | 34,89; 140,79 | igen |
| `supply-fan` | Takart vagy vetületi csatlakozás | 76,83; 129,29 | igen |
| `extract-fan` | Takart vagy vetületi csatlakozás | 76.8,84.8; 118,22 | igen |
| `roof-fan` | Takart vagy vetületi csatlakozás | 48,137.6; 112,137.6 | — |
| `heat-recovery` | Takart vagy vetületi csatlakozás | 16,60; 16,104; 147,60; 147,104 | igen |
| `air-handling-unit` | Takart vagy vetületi csatlakozás | 14,84; 144,84 | igen |
| `rotary-heat-recovery` | Takart vagy vetületi csatlakozás | 16,66; 16,104; 145,66; 145,104 | igen |
| `air-filter` | Takart vagy vetületi csatlakozás | 21,83; 140,83 | igen |
| `heating-coil` | Látható csonk / nyílás | 16,95; 144,95; 104,41; 121,45 | igen |
| `cooling-coil` | Látható csonk / nyílás | 19,94; 141,94; 104,33; 121,33; 80,134 | igen |
| `humidifier` | Látható csonk / nyílás | 18,84; 143,84; 125,51; 80,127 | igen |
| `dehumidifier` | Látható csonk / nyílás | 18,83; 142,83; 80,128 | igen |
| `silencer` | Takart vagy vetületi csatlakozás | 35,97; 140,75 | igen |
| `damper` | Takart vagy vetületi csatlakozás | 24,96; 134.4,96 | — |
| `motor-damper` | Takart vagy vetületi csatlakozás | 32,76.8; 110.4,76.8 | — |
| `fire-damper` | Takart vagy vetületi csatlakozás | 24,91.2; 118.4,91.2 | — |
| `supply-terminal` | Látható csonk / nyílás | 80,24 | — |
| `extract-terminal` | Takart vagy vetületi csatlakozás | 80,80 | — |
| `vav-box` | Takart vagy vetületi csatlakozás | 48,92; 139,63 | igen |
| `duct-junction` | Takart vagy vetületi csatlakozás | 29,32; 132,32; 80,137 | igen |
| `pool` | Takart vagy vetületi csatlakozás | 48,104; 112,104 | — |
| `balance-tank` | Látható csonk / nyílás | 22,71; 138,118 | igen |
| `skimmer` | Látható csonk / nyílás | 102,146 | igen |
| `pool-inlet` | Látható csonk / nyílás | 22,84 | igen |
| `bottom-drain` | Látható csonk / nyílás | 130,124 | igen |
| `pool-light` | Nincs csőport | — | — |
| `counter-current` | Látható csonk / nyílás | 152,42; 152,78 | igen |
| `sand-filter` | Látható csonk / nyílás | 24,56; 141,56 | igen |
| `cartridge-filter` | Látható csonk / nyílás | 38,65; 119,124 | igen |
| `uv-disinfection` | Látható csonk / nyílás | 26,52; 115,52 | igen |
| `salt-electrolysis` | Látható csonk / nyílás | 16,93; 145.6,92.8 | igen |
| `pool-pump` | Látható csonk / nyílás | 12,81; 78,51 | igen |
| `dosing-pump` | Látható csonk / nyílás | 65,139; 93,139 | igen |
| `multiport-valve` | Látható csonk / nyílás | 12,99; 148,99; 80,128 | igen |
| `pool-bypass` | Látható csonk / nyílás | 36,36; 124,36; 8,116; 152,116 | igen |
| `pool-heat-pump` | Látható csonk / nyílás | 143,68; 143,98 | igen |
| `rotor-sprinkler` | Látható csonk / nyílás | 80,153 | igen |
| `spray-sprinkler` | Látható csonk / nyílás | 80,153 | igen |
| `micro-sprinkler` | Látható csonk / nyílás | 94,66 | igen |
| `drip-line` | Látható csonk / nyílás | 12,113; 147,113 | igen |
| `irrigation-zone` | Látható csonk / nyílás | 8,113; 152,113 | igen |
| `garden-tap` | Látható csonk / nyílás | 149,87; 27,143 | igen |
| `hose` | Látható csonk / nyílás | 34,144; 135,144 | igen |
| `irrigation-valve` | Látható csonk / nyílás | 12,126; 148,126 | igen |
| `master-valve` | Látható csonk / nyílás | 12,119; 149,119 | igen |
| `valve-box` | Nincs csőport | — | — |
| `compressor-port` | Látható csonk / nyílás | 20,92; 144,92 | igen |
| `temperature-sensor` | Nincs csőport | — | — |
| `pressure-sensor` | Látható csonk / nyílás | 73,154 | igen |
| `flow-sensor` | Látható csonk / nyílás | 9,104; 151,104 | igen |
| `water-meter` | Látható csonk / nyílás | 9,99; 152,99 | igen |
| `heat-meter` | Látható csonk / nyílás | 25,129; 136,129 | igen |
| `level-sensor` | Nincs csőport | — | — |
| `humidity-sensor` | Nincs csőport | — | — |
| `co2-sensor` | Nincs csőport | — | — |
| `ph-sensor` | Nincs csőport | — | — |
| `redox-sensor` | Nincs csőport | — | — |
| `rain-sensor` | Nincs csőport | — | — |
| `soil-moisture-sensor` | Nincs csőport | — | — |
| `leak-sensor` | Nincs csőport | — | — |
| `frost-sensor` | Nincs csőport | — | — |
| `filter-pressure-switch` | Látható csonk / nyílás | 37,148; 108,148 | igen |
| `monobloc-heat-pump` | Látható csonk / nyílás | 10,65; 10,92 | igen |
| `split-heat-pump` | Látható csonk / nyílás | 111,123; 123,123 | igen |
| `high-temperature-heat-pump` | Látható csonk / nyílás | 131,103; 131,116 | igen |
| `reversible-heat-pump` | Látható csonk / nyílás | 85,124; 100,124 | igen |
| `ground-source-heat-pump` | Látható csonk / nyílás | 61,12; 74,12; 86,12; 98,12 | igen |
| `water-source-heat-pump` | Látható csonk / nyílás | 32,39; 32,51; 32,67; 32,79 | igen |
| `absorption-chiller` | Takart vagy vetületi csatlakozás | 30.4,44; 30.4,56.8; 140,44; 140,57; 30,100; 140,109 | igen |
| `rooftop-chiller` | Látható csonk / nyílás | 123,115; 137,115 | igen |
| `dry-cooler` | Látható csonk / nyílás | 154,78; 154,96 | igen |
| `floor-standing-boiler` | Látható csonk / nyílás | 37,65; 37,84 | igen |
| `oil-boiler` | Takart vagy vetületi csatlakozás | 40,112; 115.2,112 | — |
| `condensing-boiler` | Látható csonk / nyílás | 55,142; 104,142 | igen |
| `wood-gasification-boiler` | Takart vagy vetületi csatlakozás | 40,112; 115.2,112 | — |
| `wood-chip-boiler` | Takart vagy vetületi csatlakozás | 40,112; 115.2,112 | — |
| `biomass-boiler` | Takart vagy vetületi csatlakozás | 40,112; 115.2,112 | — |
| `micro-chp` | Látható csonk / nyílás | 15,67; 15,81 | igen |
| `steam-boiler` | Takart vagy vetületi csatlakozás | 80,23.2; 44,124.8 | — |
| `panel-radiator` | Látható csonk / nyílás | 123,116; 136,115.2 | igen |
| `column-radiator` | Látható csonk / nyílás | 21,24; 140,131 | igen |
| `cast-iron-radiator` | Takart vagy vetületi csatlakozás | 15,31; 148,115 | igen |
| `trench-convector` | Látható csonk / nyílás | 7,80; 7,87 | igen |
| `cassette-fan-coil` | Látható csonk / nyílás | 36,17.6; 48.8,17.6 | — |
| `ducted-fan-coil` | Látható csonk / nyílás | 7,81; 7,94 | igen |
| `cassette-air-conditioner` | Takart vagy vetületi csatlakozás | 24,28.8; 32,28.8 | — |
| `horizontal-buffer` | Látható csonk / nyílás | 16.8,67.2; 141,68; 17,92; 141,92 | igen |
| `rectangular-buffer` | Látható csonk / nyílás | 44.8,42.4; 115.2,42.4; 45,107; 115,107 | igen |
| `stratified-buffer` | Látható csonk / nyílás | 47,46; 110.4,46.4; 47,80; 110.4,80; 47,112; 110,112 | igen |
| `combi-storage` | Látható csonk / nyílás | 46.4,39.2; 112,66; 46,98; 112,120 | igen |
| `indirect-water-heater` | Látható csonk / nyílás | 134.4,53.6; 134.4,112; 26.4,73.6; 26,102 | igen |
| `tank-in-tank` | Látható csonk / nyílás | 66,14; 94,14; 120,58.4; 120,104 | igen |
| `solar-hot-water-tank` | Látható csonk / nyílás | 48,36.8; 48,124.8; 110,38; 110,51; 110,97; 110,117 | igen |
| `heat-pump-water-heater` | Látható csonk / nyílás | 66.4,11.2; 93.6,11.2; 109,122; 109,134 | igen |
| `stainless-hot-water-tank` | Látható csonk / nyílás | 67.2,8.8; 92.8,8.8; 42,70; 118,70 | igen |
| `heating-expansion-vessel` | Látható csonk / nyílás | 79,149 | igen |
| `potable-expansion-vessel` | Látható csonk / nyílás | 80,13.6 | — |
| `open-expansion-tank` | Látható csonk / nyílás | 143,68; 78,132 | igen |
| `condensate-receiver` | Látható csonk / nyílás | 10,88; 150,88; 80,48 | igen |
| `brine-tank` | Látható csonk / nyílás | 125,41 | igen |
| `underground-cistern` | Látható csonk / nyílás | 9,65; 151,65 | igen |
| `end-suction-pump` | Látható csonk / nyílás | 26,94; 50,31 | igen |
| `long-coupled-pump` | Látható csonk / nyílás | 11.2,88.8; 37,48 | igen |
| `vertical-multistage-pump` | Látható csonk / nyílás | 49,131; 112,132 | igen |
| `horizontal-multistage-pump` | Látható csonk / nyílás | 17,86; 81,48 | igen |
| `twin-inline-pump` | Látható csonk / nyílás | 12,111; 150.4,112 | igen |
| `jet-pump` | Látható csonk / nyílás | 15,88; 64,43 | igen |
| `self-priming-pump` | Látható csonk / nyílás | 17.6,94.4; 57.6,25.6 | — |
| `piston-pump` | Látható csonk / nyílás | 108,50; 154,96 | igen |
| `diaphragm-pump` | Látható csonk / nyílás | 80,13; 80,145 | igen |
| `gear-pump` | Látható csonk / nyílás | 144,61; 144,96 | igen |
| `condensate-pump` | Látható csonk / nyílás | 42,62; 130.4,38.4 | igen |
| `sewage-lifting-station` | Látható csonk / nyílás | 19,100; 140,51 | igen |
| `macerator-pump` | Látható csonk / nyílás | 34,90; 142,25 | igen |
| `vertical-sump-pump` | Látható csonk / nyílás | 76,152; 112,31 | igen |
| `hydraulic-ram-pump` | Látható csonk / nyílás | 21,104; 150,104; 53,74 | igen |
| `split-case-pump` | Látható csonk / nyílás | 14.4,108; 150.4,108 | — |
| `vortex-pump` | Látható csonk / nyílás | 73,146; 126,116 | igen |
| `brazed-plate-exchanger` | Látható csonk / nyílás | 53,39; 92,39; 53,130; 92,130 | igen |
| `gasketed-plate-exchanger` | Látható csonk / nyílás | 45.6,44; 77,44; 45,115; 77,115 | igen |
| `shell-and-tube-exchanger` | Látható csonk / nyílás | 13.6,79.2; 150,79; 48,46; 103,115 | igen |
| `coil-exchanger` | Látható csonk / nyílás | 36,13; 124,148 | igen |
| `tube-in-tube-exchanger` | Látható csonk / nyílás | 8,84; 153,84; 44,51.2; 117,115 | igen |
| `freshwater-station` | Látható csonk / nyílás | 10,64; 10,114; 150,64; 150,114 | igen |
| `manifold-3` | Fotómodulok · 264×224 | 4,99; 4,191; 84,217; 140,217; 196,217; 84,124; 140,124; 196,124 | igen |
| `manifold-5` | Fotómodulok · 376×224 | 4,99; 4,191; 84,217; 140,217; 196,217; 252,217; 308,217; 84,124; 140,124; 196,124; 252,124; 308,124 | igen |
| `manifold-7` | Fotómodulok · 488×224 | 4,99; 4,191; 84,217; 140,217; 196,217; 252,217; 308,217; 364,217; 420,217; 84,124; 140,124; 196,124; 252,124; 308,124; 364,124; 420,124 | igen |
| `manifold-9` | Fotómodulok · 600×224 | 4,99; 4,191; 84,217; 140,217; 196,217; 252,217; 308,217; 364,217; 420,217; 476,217; 532,217; 84,124; 140,124; 196,124; 252,124; 308,124; 364,124; 420,124; 476,124; 532,124 | igen |
| `duplex-strainer` | Látható csonk / nyílás | 15,86; 148,86 | igen |
| `basket-strainer` | Látható csonk / nyílás | 12,83.2; 144,83.2 | — |
| `prefilter` | Látható csonk / nyílás | 24,41.6; 136,41.6 | — |
| `backwash-filter` | Látható csonk / nyílás | 22,45; 138,45; 79,141 | igen |
| `activated-carbon-filter` | Látható csonk / nyílás | 47,28; 116,28; 80,151 | igen |
| `iron-removal-filter` | Látható csonk / nyílás | 48,27.2; 116,27 | igen |
| `reverse-osmosis` | Látható csonk / nyílás | 14,53; 145,53; 145,120 | igen |
| `hydrocyclone` | Látható csonk / nyílás | 37,55; 85,17; 85,149 | igen |
| `combined-air-dirt-separator` | Látható csonk / nyílás | 40,62; 123,62; 80,149 | igen |
| `butterfly-valve` | Takart vagy vetületi csatlakozás | 36.8,102.4; 108.8,102.4 | — |
| `motor-butterfly-valve` | Takart vagy vetületi csatlakozás | 44,104; 108,104 | igen |
| `wafer-check-valve` | Takart vagy vetületi csatlakozás | 28.8,83.2; 131.2,83.2 | — |
| `swing-check-valve` | Látható csonk / nyílás | 15,91; 147,91 | igen |
| `spring-check-valve` | Látható csonk / nyílás | 13,87; 147,87 | igen |
| `double-check-valve` | Látható csonk / nyílás | 13,96; 148,96 | igen |
| `angle-shutoff-valve` | Látható csonk / nyílás | 19,102; 95,24 | igen |
| `gate-valve` | Látható csonk / nyílás | 41,126; 119,126 | igen |
| `globe-control-valve` | Látható csonk / nyílás | 38,129; 120,129 | igen |
| `diaphragm-valve` | Látható csonk / nyílás | 20.8,118.4; 142,119 | igen |
| `needle-valve` | Látható csonk / nyílás | 24,123; 134,123 | igen |
| `pinch-valve` | Látható csonk / nyílás | 20,120; 142,120 | igen |
| `thermostatic-mixing-valve` | Látható csonk / nyílás | 35,95; 125,95; 80,143 | igen |
| `thermostatic-diverting-valve` | Látható csonk / nyílás | 23,89; 135,89; 80,142 | igen |
| `mixing-valve-4` | Látható csonk / nyílás | 15,81; 145,81; 80,17; 80,143 | igen |
| `changeover-valve-6` | Látható csonk / nyílás | 36,80; 36,107; 36,135; 122,80; 122,107; 122,135 | igen |
| `pressure-independent-valve` | Látható csonk / nyílás | 14,123; 147,123 | igen |
| `differential-pressure-valve` | Látható csonk / nyílás | 12,125; 100,125 | igen |
| `pressure-relief-valve` | Látható csonk / nyílás | 76,142; 115,104 | igen |
| `float-ball-valve` | Takart vagy vetületi csatlakozás | 8,87; 48,110 | igen |
| `tank-float-valve` | Takart vagy vetületi csatlakozás | 48,150; 70,56 | igen |
| `boiler-fill-valve` | Látható csonk / nyílás | 14,100; 146,100 | igen |
| `reduced-pressure-backflow` | Látható csonk / nyílás | 13,84; 147,84; 80,114 | igen |
| `sewage-backwater-valve` | Látható csonk / nyílás | 13,94; 146,94 | igen |
| `automatic-bypass-valve` | Látható csonk / nyílás | 42,96; 94,142 | igen |
| `wall-axial-fan` | Takart vagy vetületi csatlakozás | 30.4,80; 129.6,80 | — |
| `centrifugal-fan` | Takart vagy vetületi csatlakozás | 78,80; 123,25 | igen |
| `ec-inline-fan` | Takart vagy vetületi csatlakozás | 21,90; 139,83 | igen |
| `mixed-flow-fan` | Takart vagy vetületi csatlakozás | 24,86; 139,87 | igen |
| `box-fan` | Takart vagy vetületi csatlakozás | 15,82; 147,82 | igen |
| `smoke-extract-fan` | Takart vagy vetületi csatlakozás | 40,86.4; 120,86.4 | — |
| `destratification-fan` | Takart vagy vetületi csatlakozás | 40,108.8; 120,108.8 | — |
| `grease-extract-fan` | Takart vagy vetületi csatlakozás | 15,88; 69,21 | igen |
| `counterflow-heat-recovery` | Takart vagy vetületi csatlakozás | 13,72; 13,103; 147,72; 147,103 | igen |
| `crossflow-heat-recovery` | Takart vagy vetületi csatlakozás | 16,64; 16,105; 144,64; 144,105 | igen |
| `enthalpy-heat-recovery` | Takart vagy vetületi csatlakozás | 11,69; 11,107; 149,69; 149,107 | igen |
| `decentralized-heat-recovery` | Takart vagy vetületi csatlakozás | 41,83; 133,66 | igen |
| `rooftop-air-handler` | Takart vagy vetületi csatlakozás | 19,86; 149,83; 94,56 | igen |
| `modular-air-handler` | Takart vagy vetületi csatlakozás | 16,86; 145,86 | igen |
| `supply-air-unit` | Takart vagy vetületi csatlakozás | 12,82; 148,82 | igen |
| `extract-air-unit` | Takart vagy vetületi csatlakozás | 13,85; 149,85 | igen |
| `steam-humidifier` | Látható csonk / nyílás | 82,14; 61,150; 87,150 | igen |
| `ultrasonic-humidifier` | Látható csonk / nyílás | 47,27; 147,109; 147,122 | igen |
| `desiccant-dehumidifier` | Takart vagy vetületi csatlakozás | 12,62; 12,102; 148,62; 148,102 | igen |
| `constant-air-regulator` | Takart vagy vetületi csatlakozás | 48,88; 128,70 | igen |
| `circular-vav` | Takart vagy vetületi csatlakozás | 38,99; 132,83 | igen |
| `rectangular-vav` | Takart vagy vetületi csatlakozás | 49,87; 139,62 | igen |
| `smoke-damper` | Takart vagy vetületi csatlakozás | 48,82; 139,61 | igen |
| `pressure-relief-damper` | Takart vagy vetületi csatlakozás | 19.2,84.8; 128,84.8 | — |
| `backdraft-damper` | Takart vagy vetületi csatlakozás | 32,88; 128,88 | — |
| `ceiling-diffuser` | Látható csonk / nyílás | 80,30 | igen |
| `slot-diffuser` | Látható csonk / nyílás | 80,66 | igen |
| `swirl-diffuser` | Látható csonk / nyílás | 80,27 | igen |
| `weather-louver` | Takart vagy vetületi csatlakozás | 19.2,80; 140.8,80 | — |
| `acoustic-air-terminal` | Takart vagy vetületi csatlakozás | 12,82; 107,75 | igen |
| `overflow-pool` | Takart vagy vetületi csatlakozás | 48,112; 112,112 | — |
| `spa` | Takart vagy vetületi csatlakozás | 40,124.8; 120,124.8 | — |
| `pool-cover` | Nincs csőport | — | — |
| `solar-pool-cover` | Nincs csőport | — | — |
| `deck-jet` | Látható csonk / nyílás | 80,151 | igen |
| `waterfall` | Látható csonk / nyílás | 88,49 | igen |
| `massage-jet` | Látható csonk / nyílás | 107,17; 145,85; 58,88 | igen |
| `pool-bubbler` | Látható csonk / nyílás | 79,152 | igen |
| `top-mount-sand-filter` | Látható csonk / nyílás | 51.2,40; 107,41; 80,55 | igen |
| `side-mount-sand-filter` | Látható csonk / nyílás | 148,63; 148,79; 148,95 | igen |
| `diatomaceous-filter` | Látható csonk / nyílás | 137,70; 137,125 | igen |
| `ozone-generator` | Látható csonk / nyílás | 121,99; 121,112 | igen |
| `chlorine-feeder` | Látható csonk / nyílás | 24,123; 140,123 | igen |
| `diaphragm-dosing-pump` | Látható csonk / nyílás | 80,54; 80,147 | igen |
| `variable-speed-pool-pump` | Látható csonk / nyílás | 14,89; 80,48 | igen |
| `automatic-multiport-valve` | Látható csonk / nyílás | 14,92; 148,92; 76,122 | igen |
| `inverter-pool-heat-pump` | Látható csonk / nyílás | 11,66; 11,96 | igen |
| `rotary-nozzle` | Látható csonk / nyílás | 80,155 | igen |
| `impact-sprinkler` | Látható csonk / nyílás | 83,150 | igen |
| `bubbler-emitter` | Látható csonk / nyílás | 80,151 | igen |
| `dripper` | Látható csonk / nyílás | 131,84 | igen |
| `drip-tape` | Látható csonk / nyílás | 10,95; 150,95 | igen |
| `porous-hose` | Látható csonk / nyílás | 19,139; 141,139 | igen |
| `irrigation-hydrant` | Látható csonk / nyílás | 76,85; 133,134 | igen |
| `irrigation-controller` | Nincs csőport | — | — |
| `angle-irrigation-valve` | Látható csonk / nyílás | 72,149; 130,97 | igen |
| `latching-irrigation-valve` | Látható csonk / nyílás | 14,124; 148,124 | igen |
| `quick-coupling-valve` | Takart vagy vetületi csatlakozás | 76,151; 76,48 | igen |
| `fertigation-injector` | Látható csonk / nyílás | 11,75; 150,75; 80,111 | igen |
| `outdoor-temperature-sensor` | Nincs csőport | — | — |
| `duct-temperature-sensor` | Nincs csőport | — | — |
| `immersion-temperature-sensor` | Nincs csőport | — | — |
| `strap-on-temperature-sensor` | Nincs csőport | — | — |
| `infrared-temperature-sensor` | Nincs csőport | — | — |
| `room-temperature-sensor` | Nincs csőport | — | — |
| `combined-climate-sensor` | Nincs csőport | — | — |
| `air-differential-pressure-sensor` | Látható csonk / nyílás | 37,132; 123,132 | igen |
| `water-differential-pressure-sensor` | Látható csonk / nyílás | 18,127; 145,127 | igen |
| `differential-temperature-sensor` | Nincs csőport | — | — |
| `ultrasonic-flow-meter` | Látható csonk / nyílás | 12,120; 147,120 | igen |
| `magnetic-flow-meter` | Látható csonk / nyílás | 33,120; 130,120 | igen |
| `turbidity-sensor` | Nincs csőport | — | — |
| `conductivity-sensor` | Nincs csőport | — | — |
| `salinity-sensor` | Nincs csőport | — | — |
| `free-chlorine-sensor` | Nincs csőport | — | — |
| `dissolved-oxygen-sensor` | Nincs csőport | — | — |
| `carbon-monoxide-sensor` | Nincs csőport | — | — |
| `air-velocity-sensor` | Nincs csőport | — | — |
| `gas-meter` | Látható csonk / nyílás | 40,13; 120,13 | igen |
| `configurable-manifold` | Fotómodulok · 992×224 | 0,99; 0,191; 84,129; 84,222; 140,129; 140,222; 196,129; 196,222; 252,129; 252,222; 308,129; 308,222; 364,129; 364,222; 420,129; 420,222; 476,129; 476,222; 532,129; 532,222; 588,129; 588,222; 644,129; 644,222; 700,129; 700,222; 756,129; 756,222; 812,129; 812,222; 868,129; 868,222; 924,129; 924,222 | igen |
