# libs/traeger/tier -- Traeger-Stufe 3 (TIER)

CMake-Ziel: `comdare_tier_emission` (leeres INTERFACE-Ziel, Skeleton-Commit DESIGN #29
Par. 4.2); linkt `comdare_ceb` (Stufenkette N+1 -> N, compile-hart).

**Besitzkarte / Design:** super-Repo
`docs/plaene/20260813-DESIGN-zielstruktur-vier-traeger-unterprojekte.md`
(Commit `85c1174d`, DESIGN #29). Par. 2a: heutige Tier-Substanz = `mess/`
(6 Dateien / 1.140 Zeilen: genus_kaskade, steuer_dock, mess_naht) + `harness/`
(2 D. / 612 Z.: drift_gated_cell, perm_runner) im Monolithen `libs/cache_engine/`.
Das TIER-Unterprojekt besitzt Emission + Modul-Skelett + Harness, NICHT die
Fach-Substanz -- Tier-Module stecken zur LAUFZEIT ABI-stabil in der CEB (KON25-08),
es gibt keine Compile-Kante CEB->TIER (Design Par. 1).

**Stufenkette** (KON43-01): planner (1) <- ceb (2) <- tier (3) <- hybrid (4).
tier/ bleibt reines Skelett -- kein Ketten-Bau an einer hoeheren Stufe, bevor die
niedrigere steht (KON43-01/1). KEIN Datei-Umzug des Bestands vor W7/#88.

**NICHT VERWECHSELN:** `modules/` (Repo-Wurzel) ist eine ANDERE, abgeloeste Ordnung
(V41.E4-Altskelett, 6 "funktionale Saeulen", nur README) -- Traeger-Projekte NIE dort
hineinbauen; `modules/` wird bei #88 deprecatet (nie geloescht, Doku-Policy).
