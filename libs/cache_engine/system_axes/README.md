# system_axes/ -- das SYSTEM-Kategorie-Home (S-18/#16, KON27-01)

DESIGN-ENTSCHEID (#16, laut deklariert): der konkrete Home-Pfad war in keiner Planquelle
festgelegt (Wellenplan Par. 15.9/17.5, KON27-01, Zielstruktur Par. 1/2c nennen nur
"FEHLT (anzulegen)", Fach-Zone neben `organ_axes/`). Gewaehlt: `libs/cache_engine/system_axes/`
-- Geschwister des ORGAN-Homes `organ_axes/` in der Fach-Zone, Include-Form `<system_axes/...>`
(dieselbe Include-Wurzel `libs/cache_engine`, die `<organ_axes/...>` traegt). KEIN Traeger-
Verzeichnis (`libs/traeger/` ordnet TRAEGER, Homes ordnen KATEGORIEN -- zwei orthogonale
Ordnungen, Zielstruktur-Schnitt 20260813).

DAS HOME-PRINZIP (Owner 12.08.2026, KON27-01, verbatim-Auszug):
"jede Achsen-Kategorie [braucht] ihr eigenes home, fuer das unter der Stempel-Mechanik
genau ein Waechter greift ... Die Organ-Achsen sind derzeit axes und es braucht noch die
Kategorien fuer Mess/System."

    Kategorie SYSTEM   Home: dieses Verzeichnis (flach, Praefix-Familien)
    Waechter           tools/axis_version_lock (EINE Wache, EIN Lock -- KON17-03;
                       Detail-Klasse SystemDetail, Pruef-Syntax ZWEIPHASIG)
    Schnitt-Quelle     builder/overlay_source_set.hpp (Kategorie system, 8 Praefix-
                       Eintraege) -- WER HIER DATEIEN BEWEGT, zieht Schnitt-Pfade,
                       Mindest-Nenner im Waechter, Tripwire-Anker und den Lock-Regen
                       im SELBEN bewussten Change nach.

DIE DREI SYSTEM-HAUPT-ACHSEN (Ordnung == kSystemAxisOrder, abi/system_axis_order.hpp):

    target_isa         target_isa_*  +  scheduling_system_axis*   (scheduling ist
                       Unter-Achse des target_isa-Komplexes, system_axis_order.hpp:21)
    operating_system   operating_system_*  (Achse + Probe-Familie)
    external_utils     external_utils_*  +  extension_hardware_*  (Vor-Rename-Name)
                       +  compiler_*  +  simd_sub_axis*  +  optimization_level_sub_axis*

UMZUGSKARTE (Task #16, 15.08.2026): die 16 Dateien dieser Familien lagen bis dahin FLACH
in `include/cache_engine/measurement/` (Include-Form `<cache_engine/measurement/...>`).
Die uebrigen measurement-Header (algo_semver, Registries, Probes ausserhalb der
Praefix-Familien, Registry-XML-Spiegel) sind QUERSCHNITT und bleiben dort.

STUFE-1-AUDIT (erst laute Compile-Fehler, dann verschieben -- Hausdoktrin 11.08.):
stille Bruchklassen fuer diesen Umzug am Objekt gemessen = KEINE (kein file(GLOB) auf
measurement/, Registry-Generatoren lesen per #include/Compile-Time-Reflektion, der
Waechter bricht fail-closed bei fehlendem Schnitt-Pfad/Praefix ohne Treffer, der
Tripwire-ctest bei fehlendem Quellbaum). Alle #include-Brueche sind laut; ihre
LESBARKEIT tragen die Wegweiser-Header an den Alt-Pfaden (#error mit neuer Include-Form).
