# organ_axes/ -- das ORGAN-Kategorie-Home (S-18/#16 + F5, KON27-01/KON73-02)

DESIGN-ENTSCHEID (Owner F5, KON73-02, 15.08.2026): DREI Homes stehen als Geschwister in der
Fach-Zone `libs/cache_engine/` nebeneinander -- `mess_axes/` + `system_axes/` + `organ_axes/`.
Dieses Verzeichnis ist das ORGAN-Home; es hiess bis zum F5-Rename `axes/` (Include-Form seither
`<organ_axes/...>` statt `<axes/...>`, dieselbe Include-Wurzel `libs/cache_engine`).

DAS HOME-PRINZIP (Owner 12.08.2026, KON27-01, verbatim-Auszug):
"jede Achsen-Kategorie [braucht] ihr eigenes home, fuer das unter der Stempel-Mechanik
genau ein Waechter greift ... Die Organ-Achsen sind derzeit axes und es braucht noch die
Kategorien fuer Mess/System."

    Kategorie ORGAN    Home: dieses Verzeichnis (je Achse EIN Owner-Verzeichnis,
                       z. B. `alloc/`, `lookup/`, `node/`)
    Waechter           tools/axis_version_lock (EINE Wache, EIN Lock -- KON17-03;
                       Detail-Klasse OrganDetail traegt die Literal-MECHANIK,
                       Pruef-Syntax ZWEIPHASIG wie system)
    Schnitt-Quelle     builder/overlay_source_set.hpp (Kategorie organ, 18 Achsen
                       in der Ordnung kCompositionAxisNames) -- WER HIER DATEIEN
                       BEWEGT, zieht Schnitt-Pfade, Mindest-Nenner im Waechter,
                       Tripwire-Anker und den Lock-Regen im SELBEN bewussten
                       Change nach.

DIE DOPPELPFAD-REGEL (Schnitt-Kopf, am Objekt erhoben): eine Organ-Achse traegt im Schnitt
ZWEI Pfade -- das Owner-Verzeichnis HIER und das Andock-Verzeichnis unter `topics/`
(Weiterleitungs-Huelle). Beide sind Quelltext der Achse und stehen im Overlay-Glied [7].
AUSNAHME queuing_q1/q2 (#72, KON72-05): ihre Implementierung wohnt seit dem Umzug in
`axis_q1_queuing/` + `axis_q2_queuing/` HIER (42 Lock-Records) und traegt nur EINEN
Schnitt-Pfad; die Topic-Huelle (2 Dateien unter `topics/queuing/`) bleibt bewusst
ausserhalb des Schnitts (keine Achsen-Implementierung, Lock-Kopf D1).

NICHT IM SCHNITT (bewusst, Lock-Kopf D1): `axis_centric_namespaces.hpp` (Namensraum-Huelle),
`telemetry_axis/` (telemetry ist CEB-System-Achse geworden), `simd/` (Build-only, Glied [6]),
`cacheline/` (3 Dateien) -- keine Achsen-Implementierung im Sinne des Schnitts.
