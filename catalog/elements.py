"""A firmware által ismert elemkészlet. Új típus csak forrásmódosítással kerülhet ide."""

# Kategória, rajzcsalád, majd stabil azonosító és a két felhasználói név.
GROUPS = [
    ("heating", "boiler", """
gas-boiler|Gázkazán|Gas boiler
electric-boiler|Elektromos kazán|Electric boiler
solid-fuel-boiler|Vegyes tüzelésű kazán|Solid fuel boiler
pellet-boiler|Pelletkazán|Pellet boiler
district-heating|Távhő fogadóállomás|District heating station
"""),
    ("heating", "heat-pump", """
heat-pump|Hőszivattyú kültéri|Outdoor heat pump
heat-pump-dual|Kétventilátoros hőszivattyú|Dual fan heat pump
chiller|Folyadékhűtő|Chiller
air-conditioner-outdoor|Klíma kültéri egység|Outdoor AC unit
"""),
    ("heating", "boiler", """
heat-pump-indoor|Hőszivattyú beltéri|Indoor heat pump
hydrobox|Hidrobox|Hydrobox
electric-heater|Elektromos fűtőbetét|Electric immersion heater
"""),
    ("heating", "radiator", """
radiator|Radiátor|Radiator
towel-radiator|Törölközőszárító|Towel radiator
convector|Konvektor|Convector
fan-coil|Fan-coil|Fan coil
air-conditioner-indoor|Klíma beltéri egység|Indoor AC unit
"""),
    ("heating", "surface", """
underfloor-heating|Padlófűtés|Underfloor heating
wall-heating|Falfűtés|Wall heating
ceiling-heating|Mennyezetfűtés|Ceiling heating
ceiling-cooling|Mennyezethűtés|Ceiling cooling
heating-circuit|Fűtési kör|Heating circuit
cooling-circuit|Hűtési kör|Cooling circuit
solar-collector|Napkollektor|Solar thermal collector
"""),
    ("storage", "tank", """
buffer-tank|Puffertartály|Buffer tank
hot-water-tank|HMV tartály|Domestic hot water tank
single-coil-tank|Egyspirálos tároló|Single coil tank
dual-coil-tank|Kétspirálos tároló|Dual coil tank
electric-water-heater|Villanybojler|Electric water heater
expansion-vessel|Tágulási tartály|Expansion vessel
hydrophore|Hidrofor tartály|Pressure vessel
rainwater-tank|Esővíztartály|Rainwater tank
water-tank|Víztartály|Water tank
"""),
    ("hydraulics", "pump", """
circulation-pump|Keringetőszivattyú|Circulation pump
heating-pump|Fűtési szivattyú|Heating pump
dhw-pump|HMV cirkulációs szivattyú|DHW circulation pump
well-pump|Kútszivattyú|Well pump
submersible-pump|Merülőszivattyú|Submersible pump
booster-pump|Nyomásfokozó szivattyú|Booster pump
sewage-pump|Szennyvízszivattyú|Sewage pump
solar-pump|Szolárszivattyú|Solar pump
"""),
    ("hydraulics", "exchanger", """
plate-exchanger|Lemezes hőcserélő|Plate heat exchanger
tubular-exchanger|Csöves hőcserélő|Tubular heat exchanger
hydraulic-separator|Hidraulikus váltó|Hydraulic separator
mixing-unit|Keverőegység|Mixing unit
"""),
    ("hydraulics", "manifold", """
manifold-2|Osztó-gyűjtő 2 kör|2 circuit manifold
manifold-4|Osztó-gyűjtő 4 kör|4 circuit manifold
manifold-6|Osztó-gyűjtő 6 kör|6 circuit manifold
manifold-8|Osztó-gyűjtő 8 kör|8 circuit manifold
manifold-10|Osztó-gyűjtő 10 kör|10 circuit manifold
manifold-12|Osztó-gyűjtő 12 kör|12 circuit manifold
"""),
    ("valves", "valve", """
ball-valve|Golyóscsap|Ball valve
shutoff-valve|Elzárószelep|Shut-off valve
check-valve|Visszacsapó szelep|Check valve
solenoid-valve|Mágnesszelep|Solenoid valve
motor-valve-2|Kétjáratú motoros szelep|Two-way motor valve
motor-valve-3|Háromjáratú motoros szelep|Three-way motor valve
mixing-valve|Keverőszelep|Mixing valve
diverting-valve|Váltószelep|Diverting valve
thermostatic-valve|Termosztatikus szelep|Thermostatic valve
balancing-valve|Beszabályozó szelep|Balancing valve
pressure-reducer|Nyomáscsökkentő|Pressure reducing valve
safety-valve|Biztonsági szelep|Safety valve
drain-valve|Ürítőcsap|Drain valve
fill-valve|Töltőszelep|Fill valve
zone-valve|Zónaszelep|Zone valve
backflow-preventer|Visszafolyásgátló|Backflow preventer
automatic-drain|Automata ürítőszelep|Automatic drain valve
"""),
    ("hydraulics", "filter", """
strainer|Y-szűrő|Y strainer
dirt-separator|Iszapleválasztó|Dirt separator
magnetic-filter|Mágneses szűrő|Magnetic filter
air-separator|Légleválasztó|Air separator
automatic-air-vent|Automata légtelenítő|Automatic air vent
water-filter|Vízszűrő|Water filter
water-softener|Vízlágyító|Water softener
"""),
    ("ventilation", "fan", """
fan|Ventilátor|Fan
duct-fan|Csőventilátor|Duct fan
supply-fan|Befúvó ventilátor|Supply fan
extract-fan|Elszívó ventilátor|Extract fan
roof-fan|Tetőventilátor|Roof fan
"""),
    ("ventilation", "ahu", """
heat-recovery|Hővisszanyerős szellőztető|Heat recovery ventilation
air-handling-unit|Légkezelő|Air handling unit
rotary-heat-recovery|Forgódobos hővisszanyerő|Rotary heat recovery
air-filter|Légszűrő|Air filter
heating-coil|Fűtőkalorifer|Heating coil
cooling-coil|Hűtőkalorifer|Cooling coil
humidifier|Párásító|Humidifier
dehumidifier|Párátlanító|Dehumidifier
silencer|Hangcsillapító|Silencer
"""),
    ("ventilation", "damper", """
damper|Légzsalu|Air damper
motor-damper|Motoros légzsalu|Motorized damper
fire-damper|Tűzcsappantyú|Fire damper
supply-terminal|Befúvó|Supply terminal
extract-terminal|Elszívó|Extract terminal
vav-box|VAV szabályozó|VAV box
duct-junction|Légcsatorna elágazás|Duct junction
"""),
    ("pool", "pool", """
pool|Medence|Pool
balance-tank|Kiegyenlítő tartály|Balance tank
skimmer|Szkimmer|Skimmer
pool-inlet|Medence befúvó|Pool inlet
bottom-drain|Fenékürítő|Bottom drain
pool-light|Medencevilágítás|Pool light
counter-current|Ellenáramoltató|Counter-current unit
"""),
    ("pool", "filter", """
sand-filter|Homokszűrő|Sand filter
cartridge-filter|Patronos szűrő|Cartridge filter
uv-disinfection|UV fertőtlenítő|UV disinfection
salt-electrolysis|Sóbontó|Salt chlorinator
"""),
    ("pool", "pump", """
pool-pump|Medenceszivattyú|Pool pump
dosing-pump|Vegyszeradagoló|Dosing pump
"""),
    ("pool", "valve", """
multiport-valve|Hatállású váltószelep|Six-way multiport valve
pool-bypass|Medence bypass|Pool bypass
"""),
    ("pool", "heat-pump", """
pool-heat-pump|Medence hőszivattyú|Pool heat pump
"""),
    ("irrigation", "sprinkler", """
rotor-sprinkler|Rotoros szórófej|Rotor sprinkler
spray-sprinkler|Spray szórófej|Spray sprinkler
micro-sprinkler|Mikroszórófej|Micro sprinkler
drip-line|Csepegtető kör|Drip irrigation
irrigation-zone|Öntözési zóna|Irrigation zone
garden-tap|Kerti csap|Garden tap
hose|Locsolótömlő|Garden hose
"""),
    ("irrigation", "valve", """
irrigation-valve|Öntözési mágnesszelep|Irrigation solenoid valve
master-valve|Mesterszelep|Master valve
valve-box|Szelepdoboz|Valve box
compressor-port|Kompresszor csatlakozás|Compressor port
"""),
    ("sensors", "sensor", """
temperature-sensor|Hőmérséklet-érzékelő|Temperature sensor
pressure-sensor|Nyomásérzékelő|Pressure sensor
flow-sensor|Térfogatáram-érzékelő|Flow sensor
water-meter|Vízóra|Water meter
heat-meter|Hőmennyiségmérő|Heat meter
level-sensor|Szintérzékelő|Level sensor
humidity-sensor|Páratartalom-érzékelő|Humidity sensor
co2-sensor|CO₂-érzékelő|CO2 sensor
ph-sensor|pH-érzékelő|pH sensor
redox-sensor|Redox érzékelő|Redox sensor
rain-sensor|Esőérzékelő|Rain sensor
soil-moisture-sensor|Talajnedvesség-érzékelő|Soil moisture sensor
leak-sensor|Szivárgásérzékelő|Leak sensor
frost-sensor|Fagyérzékelő|Frost sensor
filter-pressure-switch|Szűrő nyomáskapcsoló|Filter pressure switch
"""),
]

CATEGORIES = {
    "heating": {"hu": "Fűtés / hűtés", "en": "Heating / cooling"},
    "storage": {"hu": "Tárolás / HMV", "en": "Storage / hot water"},
    "hydraulics": {"hu": "Hidraulika / vízellátás", "en": "Hydraulics / water supply"},
    "valves": {"hu": "Szelepek", "en": "Valves"},
    "ventilation": {"hu": "Szellőzés / légtechnika", "en": "Ventilation / air handling"},
    "pool": {"hu": "Medencetechnika", "en": "Pool equipment"},
    "irrigation": {"hu": "Öntözés", "en": "Irrigation"},
    "sensors": {"hu": "Mérések / érzékelők", "en": "Measurements / sensors"},
}


def elements():
    result = []
    for category, family, rows in GROUPS:
        for row in rows.strip().splitlines():
            identifier, hu, en = row.split("|")
            ports = [{"x": 0.05, "y": 0.55}, {"x": 0.95, "y": 0.55}]
            if family == "valve" and identifier in {"motor-valve-3", "mixing-valve", "diverting-valve", "multiport-valve"}:
                ports.append({"x": 0.5, "y": 0.95})
            if family == "manifold":
                circuits = int(identifier.rsplit("-", 1)[1])
                ports += [{"x": (index + 1) / (circuits + 1), "y": 0.8} for index in range(circuits)]
            if family in {"ahu", "exchanger"}:
                ports = [{"x": 0.05, "y": 0.3}, {"x": 0.95, "y": 0.3},
                         {"x": 0.05, "y": 0.7}, {"x": 0.95, "y": 0.7}]
            animation = None
            if family in {"pump", "fan", "heat-pump"}:
                animation = {"kind": "rotor", "x": 0.5, "y": 0.55, "radius": 0.16}
            if family == "sprinkler":
                animation = {"kind": "spray", "x": 0.5, "y": 0.28, "radius": 0.3}
            result.append({"id": identifier, "name": {"hu": hu, "en": en}, "category": category,
                           "family": family, "ports": ports, "animation": animation})
    return result
