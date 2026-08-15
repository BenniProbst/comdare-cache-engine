# libs/traeger/hybrid -- Traeger-Stufe 4 (HYBRID)

CMake-Ziel: `comdare_hybrid` (leeres INTERFACE-Ziel, Skeleton-Commit DESIGN #29
Par. 4.2); linkt `comdare_tier_emission` (Stufenkette N+1 -> N, compile-hart).

**Besitzkarte / Design:** super-Repo
`docs/plaene/20260813-DESIGN-zielstruktur-vier-traeger-unterprojekte.md`
(Commit `85c1174d`, DESIGN #29). Par. 2a: heutige Hybrid-Substanz = `hybrid/`
(4 Dateien / 955 Zeilen: heuristik_adapter_* -- Heuristik-Adapter-Gattung,
Reroute-Genus) + Vorschlag `heuristik/` (6 D. / 1.961 Z.: break_even, axis_spline,
workload_cluster; Planer-Mitnutzung offen, Design Par. 6) im Monolithen
`libs/cache_engine/`.

**Stufenkette** (KON43-01): planner (1) <- ceb (2) <- tier (3) <- hybrid (4).
hybrid/ bleibt reines Skelett -- kein Ketten-Bau an einer hoeheren Stufe, bevor die
niedrigere steht (KON43-01/1). KEIN Datei-Umzug des Bestands vor W7/#88.

**NICHT VERWECHSELN:** `modules/` (Repo-Wurzel) ist eine ANDERE, abgeloeste Ordnung
(V41.E4-Altskelett, 6 "funktionale Saeulen", nur README) -- Traeger-Projekte NIE dort
hineinbauen; `modules/` wird bei #88 deprecatet (nie geloescht, Doku-Policy).
