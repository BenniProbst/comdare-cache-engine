# libs/traeger/ceb -- Traeger-Stufe 2 (CEB)

CMake-Ziel: `comdare_ceb` (leeres INTERFACE-Ziel, Skeleton-Commit DESIGN #29 Par. 4.2);
linkt `comdare_planner` (Stufenkette N+1 -> N, compile-hart).

**Besitzkarte / Design:** super-Repo
`docs/plaene/20260813-DESIGN-zielstruktur-vier-traeger-unterprojekte.md`
(Commit `85c1174d`, DESIGN #29). Par. 2a: heutige CEB-Substanz = `builder/`
(169 Dateien / 38.528 Zeilen) im Monolithen `libs/cache_engine/`. Par. 4.3:
Bruecken-Regel fuer S-9-Neubauten, die Bestand aus `builder/` wiederverwenden
(`// TRAEGER-BRUECKE(#88): <Zielort>`, K1-Naht-Familien).

**Stufenkette** (KON43-01, Stufe N+1 haengt NUR an Stufe N, nie umgekehrt):
planner (1) <- ceb (2) <- tier (3) <- hybrid (4). Die massiven Kanten
builder->anatomy/topics/axes sind CEB->FACH, nicht CEB->TIER (Design Par. 1).
KEIN Datei-Umzug des Bestands vor dem Monolith-Split W7/#88.

**NICHT VERWECHSELN:** `modules/` (Repo-Wurzel) ist eine ANDERE, abgeloeste Ordnung
(V41.E4-Altskelett, 6 "funktionale Saeulen", nur README) -- Traeger-Projekte NIE dort
hineinbauen; `modules/` wird bei #88 deprecatet (nie geloescht, Doku-Policy).
