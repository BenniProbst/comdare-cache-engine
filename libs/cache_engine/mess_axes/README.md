# mess_axes/ -- das MESS-Kategorie-Home (S-18/#16, KON27-01)

DESIGN-ENTSCHEID (#16, laut deklariert): der konkrete Home-Pfad war in keiner Planquelle
festgelegt. Gewaehlt: `libs/cache_engine/mess_axes/` -- Geschwister des ORGAN-Homes
`organ_axes/` in der Fach-Zone, Include-Form `<mess_axes/...>`. NICHT zu verwechseln mit
`libs/cache_engine/mess/` (Tier-Laufzeit-Messkaskade, Zielstruktur Par. 2a: TIER-Zone,
KEIN Achsen-Home).

DAS HOME-PRINZIP (Owner 12.08.2026, KON27-01):

    Kategorie MESS     Home: dieses Verzeichnis (flach, Praefix-Familie)
    Waechter           tools/axis_version_lock (EINE Wache, EIN Lock -- KON17-03;
                       Detail-Klasse MessDetail, Pruef-Syntax DREIPHASIG)
    Schnitt-Quelle     builder/overlay_source_set.hpp (Kategorie mess, 1 Praefix-
                       Eintrag measurement_tooling) -- Datei-Bewegungen ziehen
                       Schnitt-Pfade, Mindest-Nenner, Tripwire-Anker und Lock-Regen
                       im SELBEN bewussten Change nach.

DIE EINE MESS-HAUPT-ACHSE: measurement_tooling (strukturell genau eine; pro Binary wird
genau eine Auspraegung gewaehlt -- es gibt hier nichts zu sortieren, Schnitt-Kommentar).

BENANNTE LEERSTELLE (kein stiller Default): die DREIPHASIG-Form-Wachen der G-1-Grammatik
(`mess_form_ist_dreiphasig()`, `mess_version_is_wellformed()`, Katalog
`kMessGrammarCatalog` -- G-1-Design Par. 6 Stufe B/C) sind BEWUSST noch nicht gebaut;
ihr Bau folgt exakt dem publizierten G-1-Design (Task #17/G-2 schliesst danach die
Stempel-Strecke). Der Katalog `kMessGrammarCatalog` bekommt sein Zuhause per Design in
DIESEM Home. Bis dahin prueft der Waechter Mess-Literale ueber den EINEN Bestands-Parser
(measurement/algo_semver.hpp, NIE Zweitgrammatik) auf Parsebarkeit; der Hardware-Katalog
wird auf Mess-Zeilen ABSICHTLICH NICHT erzwungen (G-1: die Mess-Grammatik ist ein Profil
der v2 MIT m-Vokabular, das der Hardware-Katalog nicht kennt).

UMZUGSKARTE (Task #16, 15.08.2026): measurement_tooling_registry.hpp lag bis dahin in
`include/cache_engine/measurement/` (Querschnitt); die uebrigen measurement-Header
bleiben dort.
