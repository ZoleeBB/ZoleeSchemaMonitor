"""A firmware által ismert elemkészlet. Új típus csak forrásmódosítással kerülhet ide."""

import json
from pathlib import Path

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
red-ecm-circulation-pump|Elektronikus keringető – piros kompakt|Compact electronic circulator – red
green-ecm-circulation-pump|Elektronikus keringető – zöld kompakt|Compact electronic circulator – green
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
electrothermal-manifold-4|Osztó-gyűjtő 4 kör – termoelektromos fejekkel|4 circuit manifold with electrothermal heads
"""),
    ("valves", "valve", """
ball-valve|Golyóscsap|Ball valve
shutoff-valve|Elzárószelep|Shut-off valve
check-valve|Visszacsapó szelep|Check valve
solenoid-valve|Mágnesszelep|Solenoid valve
motor-valve-2|Kétjáratú motoros szelep|Two-way motor valve
motor-valve-3|Háromjáratú motoros szelep|Three-way motor valve
black-rotary-valve-3|Forgatómotoros háromjáratú – menetes|Rotary three-way – threaded
black-union-rotary-valve-3|Forgatómotoros háromjáratú – hollanderes|Rotary three-way – union
black-flanged-rotary-valve-3|Forgatómotoros háromjáratú – karimás|Rotary three-way – flanged
orange-motor-ball-valve-2|Motoros golyóscsap – kétjáratú, narancs hajtás|Two-way ball valve – orange actuator
orange-motor-ball-valve-3|Motoros golyóscsap – háromjáratú, narancs hajtás|Three-way ball valve – orange actuator
spring-return-ball-valve-2|Rugó-visszatérítéses motoros golyóscsap|Spring-return motorized ball valve
compact-motor-ball-valve-2|Kompakt motoros zónagolyóscsap|Compact motorized zone ball valve
cartridge-zone-valve-3|Patronos háromjáratú motoros zónaszelep|Three-way cartridge motorized zone valve
electrothermal-zone-valve-2|Termoelektromos kétjáratú zónaszelep|Two-way electrothermal zone valve
compact-linear-control-valve-2|Kompakt lineáris hajtású szabályozószelep|Compact linear-actuated control valve
radiator-thermostatic-valve-straight|Termosztatikus radiátorszelep – egyenes|Straight thermostatic radiator valve
radiator-thermostatic-valve-angle|Termosztatikus radiátorszelep – sarok|Angle thermostatic radiator valve
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
    ("heating", "heat-pump", """
monobloc-heat-pump|Monoblokk levegő-víz hőszivattyú|Monobloc air-to-water heat pump
split-heat-pump|Split levegő-víz hőszivattyú|Split air-to-water heat pump
high-temperature-heat-pump|Magas hőmérsékletű hőszivattyú|High temperature heat pump
reversible-heat-pump|Reverzibilis hőszivattyú|Reversible heat pump
ground-source-heat-pump|Talajszondás hőszivattyú|Ground source heat pump
water-source-heat-pump|Víz-víz hőszivattyú|Water source heat pump
absorption-chiller|Abszorpciós folyadékhűtő|Absorption chiller
rooftop-chiller|Tetőre telepített folyadékhűtő|Rooftop chiller
dry-cooler|Szárazhűtő|Dry cooler
"""),
    ("heating", "boiler", """
floor-standing-boiler|Álló gázkazán|Floor standing gas boiler
oil-boiler|Olajtüzelésű kazán|Oil boiler
condensing-boiler|Kondenzációs kazán|Condensing boiler
wood-gasification-boiler|Faelgázosító kazán|Wood gasification boiler
wood-chip-boiler|Aprítéktüzelésű kazán|Wood chip boiler
biomass-boiler|Biomassza kazán|Biomass boiler
micro-chp|Mikro kapcsolt hő- és áramtermelő|Micro CHP unit
steam-boiler|Gőzkazán|Steam boiler
"""),
    ("heating", "radiator", """
panel-radiator|Acéllemez lapradiátor|Steel panel radiator
column-radiator|Oszlopradiátor|Column radiator
cast-iron-radiator|Öntöttvas radiátor|Cast iron radiator
trench-convector|Padlókonvektor|Trench convector
cassette-fan-coil|Kazettás fan-coil|Cassette fan coil
ducted-fan-coil|Légcsatornás fan-coil|Ducted fan coil
cassette-air-conditioner|Kazettás klíma beltéri|Cassette AC indoor unit
"""),
    ("storage", "tank", """
horizontal-buffer|Fekvő puffertartály|Horizontal buffer tank
rectangular-buffer|Szögletes puffertartály|Rectangular buffer tank
stratified-buffer|Rétegtárolós puffer|Stratified buffer tank
combi-storage|Kombinált hőtároló|Combined thermal store
indirect-water-heater|Indirekt HMV tároló|Indirect water heater
tank-in-tank|Tartály a tartályban tároló|Tank-in-tank store
solar-hot-water-tank|Szolár HMV tároló|Solar hot water tank
heat-pump-water-heater|Hőszivattyús vízmelegítő|Heat pump water heater
stainless-hot-water-tank|Rozsdamentes HMV tartály|Stainless hot water cylinder
heating-expansion-vessel|Fűtési membrános tágulási tartály|Heating membrane expansion vessel
potable-expansion-vessel|Ivóvizes tágulási tartály|Potable water expansion vessel
open-expansion-tank|Nyitott tágulási tartály|Open expansion tank
condensate-receiver|Kondenzátumgyűjtő tartály|Condensate receiver
brine-tank|Sóoldattartály|Brine tank
underground-cistern|Föld alatti víztároló|Underground cistern
"""),
    ("hydraulics", "pump", """
end-suction-pump|Végszívású centrifugálszivattyú|End suction centrifugal pump
long-coupled-pump|Hosszú tengelykapcsolós szivattyú|Long coupled pump
vertical-multistage-pump|Függőleges többfokozatú szivattyú|Vertical multistage pump
horizontal-multistage-pump|Vízszintes többfokozatú szivattyú|Horizontal multistage pump
twin-inline-pump|Iker inline szivattyú|Twin inline pump
jet-pump|Jet rendszerű szivattyú|Jet pump
self-priming-pump|Önfelszívó szivattyú|Self priming pump
piston-pump|Dugattyús szivattyú|Piston pump
diaphragm-pump|Membránszivattyú|Diaphragm pump
gear-pump|Fogaskerék-szivattyú|Gear pump
condensate-pump|Kondenzvízszivattyú|Condensate pump
sewage-lifting-station|Szennyvízátemelő telep|Sewage lifting station
macerator-pump|Darálós szennyvízszivattyú|Macerator pump
vertical-sump-pump|Függőleges aknaszivattyú|Vertical sump pump
hydraulic-ram-pump|Vízütéses szivattyú|Hydraulic ram pump
split-case-pump|Osztott házas szivattyú|Split case pump
vortex-pump|Örvénykerekes szivattyú|Vortex pump
"""),
    ("hydraulics", "exchanger", """
brazed-plate-exchanger|Forrasztott lemezes hőcserélő|Brazed plate exchanger
gasketed-plate-exchanger|Tömítéses lemezes hőcserélő|Gasketed plate exchanger
shell-and-tube-exchanger|Csőköteges hőcserélő|Shell and tube exchanger
coil-exchanger|Csőkígyós hőcserélő|Coil heat exchanger
tube-in-tube-exchanger|Cső a csőben hőcserélő|Tube in tube exchanger
freshwater-station|Frissvízmodul|Fresh water station
"""),
    ("hydraulics", "manifold", """
manifold-3|Osztó-gyűjtő 3 kör|3 circuit manifold
manifold-5|Osztó-gyűjtő 5 kör|5 circuit manifold
manifold-7|Osztó-gyűjtő 7 kör|7 circuit manifold
manifold-9|Osztó-gyűjtő 9 kör|9 circuit manifold
"""),
    ("hydraulics", "filter", """
duplex-strainer|Iker kosaras szűrő|Duplex strainer
basket-strainer|Kosaras szűrő|Basket strainer
prefilter|Előszűrő|Prefilter
backwash-filter|Visszamosható vízszűrő|Backwashing water filter
activated-carbon-filter|Aktívszén-szűrő|Activated carbon filter
iron-removal-filter|Vastalanító szűrő|Iron removal filter
reverse-osmosis|Fordított ozmózisos egység|Reverse osmosis unit
hydrocyclone|Hidrociklon|Hydrocyclone
combined-air-dirt-separator|Kombinált lég- és iszapleválasztó|Combined air and dirt separator
"""),
    ("valves", "valve", """
butterfly-valve|Kézi pillangószelep|Manual butterfly valve
motor-butterfly-valve|Motoros pillangószelep|Motorized butterfly valve
wafer-check-valve|Ostya visszacsapó szelep|Wafer check valve
swing-check-valve|Lengő visszacsapó szelep|Swing check valve
spring-check-valve|Rugós visszacsapó szelep|Spring check valve
double-check-valve|Kettős visszacsapó szelep|Double check valve
angle-shutoff-valve|Sarokelzáró|Angle shut-off valve
gate-valve|Tolózár|Gate valve
globe-control-valve|Ülékes szabályozószelep|Globe control valve
diaphragm-valve|Membránszelep|Diaphragm valve
needle-valve|Tűszelep|Needle valve
pinch-valve|Tömlős elzárószelep|Pinch valve
thermostatic-mixing-valve|Termosztatikus keverőszelep|Thermostatic mixing valve
thermostatic-diverting-valve|Termosztatikus váltószelep|Thermostatic diverting valve
mixing-valve-4|Négyjáratú keverőszelep|Four-way mixing valve
changeover-valve-6|Hatjáratú átváltószelep|Six-way changeover valve
pressure-independent-valve|Nyomásfüggetlen szabályozószelep|Pressure independent control valve
differential-pressure-valve|Nyomáskülönbség-szabályozó szelep|Differential pressure control valve
pressure-relief-valve|Nyomástartó lefúvatószelep|Pressure relief valve
float-ball-valve|Úszógolyós szelep|Float ball valve
tank-float-valve|Tartálytöltő úszószelep|Tank float valve
boiler-fill-valve|Automatikus kazántöltő|Automatic boiler fill valve
reduced-pressure-backflow|Csökkentett nyomású visszafolyásgátló|Reduced pressure backflow preventer
sewage-backwater-valve|Szennyvíz visszatorlódás-gátló|Sewage backwater valve
automatic-bypass-valve|Automatikus bypass szelep|Automatic bypass valve
"""),
    ("ventilation", "fan", """
wall-axial-fan|Fali axiálventilátor|Wall axial fan
centrifugal-fan|Radiálventilátor|Centrifugal fan
ec-inline-fan|EC csőventilátor|EC inline fan
mixed-flow-fan|Félradiális csőventilátor|Mixed flow duct fan
box-fan|Dobozventilátor|Box fan
smoke-extract-fan|Hő- és füstelvezető ventilátor|Smoke extract fan
destratification-fan|Légrétegződést megszüntető ventilátor|Destratification fan
grease-extract-fan|Konyhai zsíroslevegő-elszívó|Kitchen grease extract fan
"""),
    ("ventilation", "ahu", """
counterflow-heat-recovery|Ellenáramú hővisszanyerő|Counterflow heat recovery
crossflow-heat-recovery|Keresztáramú hővisszanyerő|Crossflow heat recovery
enthalpy-heat-recovery|Entalpiás hővisszanyerő|Enthalpy heat recovery
decentralized-heat-recovery|Decentralizált szellőztető|Decentralized heat recovery
rooftop-air-handler|Tető légkezelő|Rooftop air handling unit
modular-air-handler|Moduláris légkezelő|Modular air handling unit
supply-air-unit|Befúvó légkezelő|Supply air unit
extract-air-unit|Elszívó légkezelő|Extract air unit
steam-humidifier|Gőzpárásító|Steam humidifier
ultrasonic-humidifier|Ultrahangos párásító|Ultrasonic humidifier
desiccant-dehumidifier|Adszorpciós párátlanító|Desiccant dehumidifier
"""),
    ("ventilation", "damper", """
constant-air-regulator|Állandó térfogatáram-szabályozó|Constant air volume regulator
circular-vav|Kör keresztmetszetű VAV|Circular VAV box
rectangular-vav|Négyszögletes VAV|Rectangular VAV box
smoke-damper|Füstcsappantyú|Smoke damper
pressure-relief-damper|Túlnyomás-kiegyenlítő zsalu|Pressure relief damper
backdraft-damper|Visszacsapó légzsalu|Backdraft damper
ceiling-diffuser|Mennyezeti befúvó|Ceiling diffuser
slot-diffuser|Résbefúvó|Slot diffuser
swirl-diffuser|Örvénybefúvó|Swirl diffuser
weather-louver|Kültéri esővédő zsalu|Weather louver
acoustic-air-terminal|Akusztikus légszelep|Acoustic air terminal
"""),
    ("pool", "pool", """
overflow-pool|Feszített víztükrű medence|Overflow pool
spa|Pezsgőmedence|Spa
pool-cover|Motoros medencetakaró|Motorized pool cover
solar-pool-cover|Szolár medencetakaró|Solar pool cover
deck-jet|Perem vízsugár|Deck jet
waterfall|Vízeséselem|Waterfall feature
massage-jet|Masszázsbefúvó|Massage jet
pool-bubbler|Levegőbefúvó élményelem|Pool bubbler
"""),
    ("pool", "filter", """
top-mount-sand-filter|Felsőszelepes homokszűrő|Top mount sand filter
side-mount-sand-filter|Oldalszelepes homokszűrő|Side mount sand filter
diatomaceous-filter|Kovaföldszűrő|Diatomaceous earth filter
ozone-generator|Ózonfertőtlenítő|Ozone generator
chlorine-feeder|Klóradagoló tartály|Chlorine feeder
"""),
    ("pool", "pump", """
diaphragm-dosing-pump|Membrános vegyszeradagoló|Diaphragm dosing pump
variable-speed-pool-pump|Változó fordulatú medenceszivattyú|Variable speed pool pump
"""),
    ("pool", "valve", """
automatic-multiport-valve|Automatikus hatállású szelep|Automatic multiport valve
"""),
    ("pool", "heat-pump", """
inverter-pool-heat-pump|Inverteres medence hőszivattyú|Inverter pool heat pump
"""),
    ("irrigation", "sprinkler", """
rotary-nozzle|Forgó fúvóka|Rotary nozzle
impact-sprinkler|Kalapácsos szórófej|Impact sprinkler
bubbler-emitter|Buborékoltató öntöző|Bubbler emitter
dripper|Pontszerű csepegtető|Point drip emitter
drip-tape|Csepegtető szalag|Drip tape
porous-hose|Szivárogtató tömlő|Porous soaker hose
irrigation-hydrant|Öntözővíz-csatlakozó|Irrigation hydrant
irrigation-controller|Öntözésvezérlő|Irrigation controller
"""),
    ("irrigation", "valve", """
angle-irrigation-valve|Sarok öntözőszelep|Angle irrigation valve
latching-irrigation-valve|Bistabil öntözőszelep|Latching irrigation valve
quick-coupling-valve|Gyorscsatlakozó szelep|Quick coupling valve
fertigation-injector|Venturi tápoldatbekeverő|Venturi fertilizer injector
"""),
    ("sensors", "sensor", """
outdoor-temperature-sensor|Külső hőmérséklet-érzékelő|Outdoor temperature sensor
duct-temperature-sensor|Légcsatorna-hőmérséklet érzékelő|Duct temperature sensor
immersion-temperature-sensor|Merülő hőmérséklet-érzékelő|Immersion temperature sensor
strap-on-temperature-sensor|Csőre szerelt hőmérséklet-érzékelő|Strap-on temperature sensor
infrared-temperature-sensor|Infravörös hőmérséklet-érzékelő|Infrared temperature sensor
room-temperature-sensor|Szobahőmérséklet-érzékelő|Room temperature sensor
combined-climate-sensor|Kombinált hőmérséklet-páratartalom érzékelő|Combined temperature and humidity sensor
air-differential-pressure-sensor|Levegő nyomáskülönbség-érzékelő|Air differential pressure sensor
water-differential-pressure-sensor|Víz nyomáskülönbség-érzékelő|Water differential pressure sensor
differential-temperature-sensor|Hőmérséklet-különbség érzékelő|Differential temperature sensor
ultrasonic-flow-meter|Ultrahangos térfogatárammérő|Ultrasonic flow meter
magnetic-flow-meter|Elektromágneses térfogatárammérő|Electromagnetic flow meter
turbidity-sensor|Zavarosságérzékelő|Turbidity sensor
conductivity-sensor|Vezetőképesség-érzékelő|Conductivity sensor
salinity-sensor|Sótartalom-érzékelő|Salinity sensor
free-chlorine-sensor|Szabadklór-érzékelő|Free chlorine sensor
dissolved-oxygen-sensor|Oldottoxigén-érzékelő|Dissolved oxygen sensor
carbon-monoxide-sensor|Szén-monoxid-érzékelő|Carbon monoxide sensor
air-velocity-sensor|Légsebesség-érzékelő|Air velocity sensor
gas-meter|Gázmérő|Gas meter
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


# A jóváhagyott vásznakhoz regisztrált csonkok és mozgó rétegek.
# A világos/sötét oldal ugyanazt az átlátszó képet használhatja.
ARTWORK = {
    "circulation-pump": ("circulation-pump", [(0.05, 0.58), (0.93, 0.58)], ("rotor", 0.484, 0.572, 0.14)),
    "dhw-pump": ("dhw-pump", [(0.08, 0.61), (0.90, 0.61)], ("rotor", 0.489, 0.608, 0.13)),
    "well-pump": ("well-pump", [(0.49, 0.045), (0.50, 0.56)], ("status", 0.53, 0.82, 0.025)),
    "submersible-pump": ("submersible-pump", [(0.45, 0.84), (0.74, 0.46)], ("status", 0.45, 0.24, 0.025)),
    "booster-pump": ("booster-pump", [(0.08, 0.51), (0.36, 0.33)], ("status", 0.65, 0.31, 0.025)),
    "sewage-pump": ("sewage-pump", [(0.43, 0.85), (0.80, 0.61)], ("status", 0.43, 0.22, 0.025)),
    "pool-pump": ("pool-pump", [(0.06, 0.51), (0.47, 0.32)], ("status", 0.68, 0.38, 0.025)),
    "dosing-pump": ("dosing-pump", [(0.40, 0.84), (0.57, 0.84)], ("status", 0.66, 0.32, 0.025)),
}
ARTWORK.update({
    "buffer-tank": ("buffer-tank", [(0.265, 0.336), (0.715, 0.336), (0.265, 0.655), (0.715, 0.655)], ("none", 0, 0, 0)),
    "hot-water-tank": ("hot-water-tank", [(0.714, 0.257), (0.714, 0.755), (0.265, 0.48), (0.265, 0.675)], ("none", 0, 0, 0)),
    "dual-coil-tank": ("dual-coil-tank", [(0.685, 0.257), (0.685, 0.74), (0.30, 0.322), (0.30, 0.45), (0.30, 0.55), (0.30, 0.69)], ("none", 0, 0, 0)),
    "horizontal-buffer": ("horizontal-buffer", [(0.105, 0.42), (0.875, 0.42), (0.105, 0.56), (0.875, 0.56)], ("none", 0, 0, 0)),
})
# A tartályok légszelepe és szervizfedele nem vízcsatlakozó.
# A statikus metszeti képekre nem kerül forgó vagy áláramlási réteg.
ARTWORK.update({
    "electric-water-heater": ("electric-water-heater", [(0.425, 0.94), (0.545, 0.94)], ("none", 0, 0, 0)),
    "expansion-vessel": ("expansion-vessel", [(0.50, 0.89)], ("none", 0, 0, 0)),
    "hydrophore": ("hydrophore", [(0.50, 0.89)], ("none", 0, 0, 0)),
    "combi-storage": ("combi-storage", [(0.29, 0.245), (0.29, 0.625), (0.71, 0.415), (0.71, 0.755)], ("none", 0, 0, 0)),
})
# A membrános tartály előtöltő szelepe nem a rajzolt vízkör része.
ARTWORK.update({
    "heating-expansion-vessel": ("heating-expansion-vessel", [(0.50, 0.94)], ("none", 0, 0, 0)),
    "potable-expansion-vessel": ("potable-expansion-vessel", [(0.50, 0.085)], ("none", 0, 0, 0)),
    "open-expansion-tank": ("open-expansion-tank", [(0.91, 0.42), (0.50, 0.81)], ("none", 0, 0, 0)),
    "rainwater-tank": ("rainwater-tank", [(0.215, 0.27), (0.82, 0.81)], ("none", 0, 0, 0)),
})
# A HMV-csonkok és a hőcserélő körei külön pontok; a metszeti ablak nem külső cső.
ARTWORK.update({
    "indirect-water-heater": ("indirect-water-heater", [(0.84, 0.335), (0.84, 0.70), (0.165, 0.46), (0.165, 0.65)], ("none", 0, 0, 0)),
    "tank-in-tank": ("tank-in-tank", [(0.42, 0.08), (0.58, 0.08), (0.75, 0.365), (0.75, 0.65)], ("none", 0, 0, 0)),
    "solar-hot-water-tank": ("solar-hot-water-tank", [(0.30, 0.23), (0.30, 0.78), (0.70, 0.245), (0.70, 0.325), (0.70, 0.615), (0.70, 0.745)], ("none", 0, 0, 0)),
    "stainless-hot-water-tank": ("stainless-hot-water-tank", [(0.42, 0.055), (0.58, 0.055), (0.27, 0.445), (0.735, 0.445)], ("none", 0, 0, 0)),
})
# A rétegtároló három magasságban vezet ki külön csonkpárt.
ARTWORK.update({
    "rectangular-buffer": ("rectangular-buffer", [(0.28, 0.265), (0.72, 0.265), (0.28, 0.68), (0.72, 0.68)], ("none", 0, 0, 0)),
    "stratified-buffer": ("stratified-buffer", [(0.315, 0.29), (0.69, 0.29), (0.315, 0.50), (0.69, 0.50), (0.315, 0.71), (0.69, 0.71)], ("none", 0, 0, 0)),
    "water-tank": ("water-tank", [(0.87, 0.825)], ("none", 0, 0, 0)),
    "condensate-receiver": ("condensate-receiver", [(0.065, 0.56), (0.935, 0.56), (0.50, 0.30)], ("none", 0, 0, 0)),
})
# A vízmelegítő ventilátora zárt házban van: csak az előlap állapotjelzője animált.
# A felső két pont légcsatorna, az alsó oldalpár vízkör; nincs belső összekötés.
ARTWORK.update({
    "heat-pump-water-heater": ("heat-pump-water-heater", [(0.415, 0.07), (0.585, 0.07), (0.69, 0.775), (0.69, 0.855)], ("status", 0.44, 0.40, 0.015)),
    "brine-tank": ("brine-tank", [(0.79, 0.25)], ("none", 0, 0, 0)),
    "underground-cistern": ("underground-cistern", [(0.04, 0.42), (0.96, 0.42)], ("none", 0, 0, 0)),
})
# Zárt járókerékházaknál az aktivitást a motoron elhelyezett fény jelzi.
# A tengelykapcsoló és a villamos csatlakozódoboz nem vízoldali csonk.
ARTWORK.update({
    "end-suction-pump": ("end-suction-pump", [(0.15, 0.59), (0.315, 0.205)], ("status", 0.69, 0.34, 0.025)),
    "long-coupled-pump": ("long-coupled-pump", [(0.07, 0.555), (0.235, 0.31)], ("status", 0.81, 0.395, 0.025)),
    "vertical-multistage-pump": ("vertical-multistage-pump", [(0.30, 0.825), (0.70, 0.825)], ("status", 0.61, 0.20, 0.025)),
    "horizontal-multistage-pump": ("horizontal-multistage-pump", [(0.105, 0.55), (0.505, 0.315)], ("status", 0.745, 0.345, 0.025)),
})
# A dugattyús gép bal csonkszerű eleme hajtótengely, a fekete sapka olajbetöltő.
# A membránszivattyú két nagy gyűjtőcsöve a folyadékoldal; a középső szerelvény nem vízport.
ARTWORK.update({
    "jet-pump": ("jet-pump", [(0.10, 0.56), (0.40, 0.28)], ("status", 0.70, 0.33, 0.025)),
    "self-priming-pump": ("self-priming-pump", [(0.11, 0.59), (0.36, 0.16)], ("status", 0.71, 0.37, 0.025)),
    "piston-pump": ("piston-pump", [(0.68, 0.32), (0.96, 0.585)], ("status", 0.38, 0.45, 0.025)),
    "diaphragm-pump": ("diaphragm-pump", [(0.51, 0.09), (0.51, 0.91)], ("status", 0.50, 0.38, 0.025)),
})
# A fogaskerék-szivattyú bal oldali tengelye nem vízcsatlakozó.
# Az átemelő zárt fedelei és a darálós egység lezárt oldalsó sapkája kimaradnak.
ARTWORK.update({
    "gear-pump": ("gear-pump", [(0.89, 0.37), (0.89, 0.585)], ("status", 0.50, 0.225, 0.025)),
    "condensate-pump": ("condensate-pump", [(0.27, 0.40), (0.815, 0.24)], ("status", 0.61, 0.31, 0.025)),
    "sewage-lifting-station": ("sewage-lifting-station", [(0.125, 0.64), (0.90, 0.315)], ("status", 0.52, 0.525, 0.025)),
    "macerator-pump": ("macerator-pump", [(0.22, 0.55), (0.87, 0.17)], ("status", 0.56, 0.63, 0.025)),
})
# A merülőgépek alsó nyílása folyadékbelépés; a fogantyú nem csatlakozó.
# A vízütéses gépen a két csőcsonk mellett a szabad hulladékvíz-kilépés is külön végpont.
ARTWORK.update({
    "vertical-sump-pump": ("vertical-sump-pump", [(0.48, 0.96), (0.71, 0.20)], ("status", 0.48, 0.17, 0.025)),
    "hydraulic-ram-pump": ("hydraulic-ram-pump", [(0.145, 0.64), (0.94, 0.66), (0.335, 0.495)], ("status", 0.64, 0.75, 0.025)),
    "split-case-pump": ("split-case-pump", [(0.09, 0.675), (0.94, 0.675)], ("status", 0.50, 0.38, 0.025)),
    "vortex-pump": ("vortex-pump", [(0.49, 0.95), (0.79, 0.735)], ("status", 0.49, 0.35, 0.025)),
})
# Az iker inline gép közös aktivitásforrást használ, nem két független motorjelet.
ARTWORK.update({
    "twin-inline-pump": ("twin-inline-pump", [(0.09, 0.70), (0.94, 0.70)], ("status", 0.50, 0.79, 0.025)),
    "diaphragm-dosing-pump": ("diaphragm-dosing-pump", [(0.50, 0.38), (0.50, 0.91)], ("status", 0.39, 0.255, 0.025)),
    "variable-speed-pool-pump": ("variable-speed-pool-pump", [(0.085, 0.585), (0.50, 0.32)], ("status", 0.76, 0.37, 0.025)),
})
# A hőcserélők négy külső csonkja külön végpont; a leszorító rudak nem portok.
ARTWORK.update({
    "plate-exchanger": ("plate-exchanger", [(0.275, 0.265), (0.51, 0.265), (0.275, 0.725), (0.51, 0.725)], ("none", 0, 0, 0)),
    "brazed-plate-exchanger": ("brazed-plate-exchanger", [(0.34, 0.24), (0.59, 0.24), (0.34, 0.81), (0.59, 0.81)], ("none", 0, 0, 0)),
    "gasketed-plate-exchanger": ("gasketed-plate-exchanger", [(0.285, 0.275), (0.50, 0.275), (0.285, 0.73), (0.50, 0.73)], ("none", 0, 0, 0)),
    "tube-in-tube-exchanger": ("tube-in-tube-exchanger", [(0.065, 0.53), (0.94, 0.53), (0.275, 0.32), (0.735, 0.74)], ("none", 0, 0, 0)),
})
# A gombközép és a csonkok a végleges négyzetes PNG vászonhoz tartoznak.
ARTWORK.update({
    "black-rotary-valve-3": ("black-rotary-valve-3", [(0.06, 0.44), (0.94, 0.44), (0.49, 0.87)], ("position", 0.385, 0.49, 0.12)),
    "black-union-rotary-valve-3": ("black-union-rotary-valve-3", [(0.06, 0.43), (0.94, 0.43), (0.49, 0.87)], ("position", 0.39, 0.46, 0.12)),
    "black-flanged-rotary-valve-3": ("black-flanged-rotary-valve-3", [(0.08, 0.46), (0.92, 0.46), (0.5, 0.85)], ("position", 0.385, 0.505, 0.12)),
})
# A villamos kábelek, kézi karok és rögzítőfülek nem folyadékcsatlakozók.
# A zárt elektronikus keringetőkön nincs külső forgórész.
ARTWORK.update({
    "orange-motor-ball-valve-2": ("orange-motor-ball-valve-2", [(0.22, 0.82), (0.60, 0.68)], ("none", 0, 0, 0)),
    "orange-motor-ball-valve-3": ("orange-motor-ball-valve-3", [(0.23, 0.72), (0.64, 0.67), (0.42, 0.88)], ("none", 0, 0, 0)),
    "spring-return-ball-valve-2": ("spring-return-ball-valve-2", [(0.24, 0.71), (0.60, 0.71)], ("none", 0, 0, 0)),
    "compact-motor-ball-valve-2": ("compact-motor-ball-valve-2", [(0.05, 0.77), (0.88, 0.77)], ("none", 0, 0, 0)),
    "cartridge-zone-valve-3": ("cartridge-zone-valve-3", [(0.25, 0.62), (0.73, 0.62), (0.49, 0.90)], ("none", 0, 0, 0)),
    "electrothermal-zone-valve-2": ("electrothermal-zone-valve-2", [(0.07, 0.80), (0.93, 0.80)], ("none", 0, 0, 0)),
    "compact-linear-control-valve-2": ("compact-linear-control-valve-2", [(0.28, 0.74), (0.78, 0.74)], ("none", 0, 0, 0)),
    "radiator-thermostatic-valve-straight": ("radiator-thermostatic-valve-straight", [(0.12, 0.82), (0.87, 0.82)], ("none", 0, 0, 0)),
    "radiator-thermostatic-valve-angle": ("radiator-thermostatic-valve-angle", [(0.38, 0.92), (0.79, 0.69)], ("none", 0, 0, 0)),
    "red-ecm-circulation-pump": ("red-ecm-circulation-pump", [(0.045, 0.47), (0.95, 0.47)], ("status", 0.50, 0.66, 0.025)),
    "green-ecm-circulation-pump": ("green-ecm-circulation-pump", [(0.04, 0.45), (0.96, 0.45)], ("status", 0.50, 0.64, 0.025)),
    "electrothermal-manifold-4": ("electrothermal-manifold-4", [
        (0.055, 0.27), (0.055, 0.72),
        (0.28, 0.38), (0.43, 0.38), (0.58, 0.38), (0.73, 0.38),
        (0.28, 0.87), (0.43, 0.87), (0.58, 0.87), (0.73, 0.87),
    ], ("none", 0, 0, 0)),
})
# A kézi szár, tekercs, motorház és mérőcsonkok nem folyadékportok.
ARTWORK.update({
    "ball-valve": ("ball-valve", [(0.155, 0.615), (0.795, 0.615)], ("none", 0, 0, 0)),
    "shutoff-valve": ("shutoff-valve", [(0.18, 0.755), (0.815, 0.755)], ("none", 0, 0, 0)),
    "check-valve": ("check-valve", [(0.14, 0.575), (0.86, 0.575)], ("none", 0, 0, 0)),
    "solenoid-valve": ("solenoid-valve", [(0.16, 0.78), (0.84, 0.78)], ("none", 0, 0, 0)),
    "motor-valve-2": ("motor-valve-2", [(0.19, 0.72), (0.82, 0.72)], ("none", 0, 0, 0)),
    "motor-valve-3": ("motor-valve-3", [(0.17, 0.59), (0.79, 0.59), (0.50, 0.865)], ("none", 0, 0, 0)),
    "diverting-valve": ("diverting-valve", [(0.18, 0.52), (0.80, 0.52), (0.49, 0.81)], ("none", 0, 0, 0)),
    "thermostatic-valve": ("thermostatic-valve", [(0.16, 0.765), (0.85, 0.765)], ("none", 0, 0, 0)),
    "balancing-valve": ("balancing-valve", [(0.12, 0.70), (0.88, 0.70)], ("none", 0, 0, 0)),
    "pressure-reducer": ("pressure-reducer", [(0.14, 0.74), (0.86, 0.74)], ("none", 0, 0, 0)),
})
ARTWORK["single-coil-tank"] = ARTWORK["hot-water-tank"]
ARTWORK["heating-pump"] = ARTWORK["circulation-pump"]
ARTWORK["solar-pump"] = ARTWORK["circulation-pump"]


# Az öt ötvenes csomag tételesen felülvizsgált csatlakozásai.
INTEGRATED_ARTWORK = json.loads(Path(__file__).with_name("artwork-bindings.json").read_text())
PORT_CALIBRATION = json.loads(Path(__file__).with_name("port-calibration.json").read_text())
for identifier, entry in INTEGRATED_ARTWORK.items():
    if "procedural" not in entry:
        ARTWORK[identifier] = (entry.get("source", identifier), entry["ports"], entry["motion"])


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
            registration = INTEGRATED_ARTWORK.get(identifier, {})
            artwork = ARTWORK.get(identifier)
            if artwork:
                source, anchors, motion = artwork
                ports = [{"x": x, "y": y} for x, y in anchors]
                kind, x, y, radius = motion
                animation = {"kind": kind, "x": x, "y": y, "radius": radius}
            if not artwork and identifier in {"thermostatic-mixing-valve", "thermostatic-diverting-valve", "automatic-multiport-valve"}:
                ports.append({"x": 0.5, "y": 0.95})
            if not artwork and identifier == "mixing-valve-4":
                ports += [{"x": 0.5, "y": 0.05}, {"x": 0.5, "y": 0.95}]
            if not artwork and identifier == "changeover-valve-6":
                ports = [{"x": x, "y": y} for x, y in [(0.05, 0.3), (0.05, 0.7), (0.95, 0.3), (0.95, 0.7), (0.5, 0.05), (0.5, 0.95)]]
            if registration.get("procedural"):
                ports = [{"x": x, "y": y} for x, y in registration["ports"]]
                animation = {"kind": "none", "x": 0, "y": 0, "radius": 0}
            if "scale" in registration:
                animation["scaleX"], animation["scaleY"] = registration["scale"]
            if "sprayDirection" in registration:
                animation["direction"] = registration["sprayDirection"]
            if "rotors" in registration:
                animation["rotors"] = [{"x": x, "y": y, "radius": radius} for x, y, radius in registration["rotors"]]
            result.append({"id": identifier, "name": {"hu": hu, "en": en}, "category": category,
                           "family": family, "ports": ports, "animation": animation,
                           **({"proceduralManifold": registration["procedural"]} if registration.get("procedural") else {}),
                           **({"portNote": registration["note"]} if registration.get("note") else {}),
                           **({"artwork": artwork[0]} if artwork else {})})
    result.append({"id": "configurable-manifold", "name": {
        "hu": "Termofejes osztó-gyűjtő (2–16 kör)", "en": "Actuated manifold (2–16 circuits)"},
        "category": "valves", "family": "configurable-manifold", "ports": [
            {"x": .04, "y": .30}, {"x": .04, "y": .72},
            *[{"x": .16 + .78 * (i + .5) / 16, "y": y}
              for i in range(16) for y in [.46, .95]]], "animation": None})
    # A portindexek a mentett csőkapcsolatok részei; a kalibrálás nem rendezheti át őket.
    for element in result:
        calibration = PORT_CALIBRATION["elements"][element["id"]]
        if calibration["mode"] != "procedural":
            element["ports"] = [{"x": x / PORT_CALIBRATION["canvasPixels"],
                                 "y": y / PORT_CALIBRATION["canvasPixels"]}
                                for x, y in calibration["portsPx"]]
            if calibration.get("note") and not element.get("portNote"):
                element["portNote"] = calibration["note"]
    return result
