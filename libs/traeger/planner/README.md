# libs/traeger/planner -- Traeger-Stufe 1 (PLANNER)

CMake-Ziel: `comdare_planner` (leeres INTERFACE-Ziel, Skeleton-Commit DESIGN #29 Par. 4.2).

**Besitzkarte / Design:** super-Repo
`docs/plaene/20260813-DESIGN-zielstruktur-vier-traeger-unterprojekte.md`
(Commit `85c1174d`, DESIGN #29). Par. 2a: heutige Planer-Substanz = `profile_facade/`
(32 Dateien / 15.437 Zeilen, davon `profile_facade/planner/` 10 D. / 4.721 Z.) im
Monolithen `libs/cache_engine/`. Par. 4.3: Bruecken-Regel fuer S-8-Neubauten
(`// TRAEGER-BRUECKE(#88): <Zielort>`).

**Stufenkette** (KON43-01, Stufe N+1 haengt NUR an Stufe N, nie umgekehrt):
planner (1) <- ceb (2) <- tier (3) <- hybrid (4). S-8 baut HIER
(IPlanBuilder/CMakeGraphBuilder/CiYamlBuilder/2-Pass); KEIN Datei-Umzug des
Bestands vor dem Monolith-Split W7/#88.

**NICHT VERWECHSELN:** `modules/` (Repo-Wurzel) ist eine ANDERE, abgeloeste Ordnung
(V41.E4-Altskelett, 6 "funktionale Saeulen", nur README) -- Traeger-Projekte NIE dort
hineinbauen; `modules/` wird bei #88 deprecatet (nie geloescht, Doku-Policy).
