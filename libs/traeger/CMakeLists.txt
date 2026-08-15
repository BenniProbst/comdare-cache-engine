# libs/traeger/ -- DESIGN #29 Skeleton (super-Repo docs/plaene/
# 20260813-DESIGN-zielstruktur-vier-traeger-unterprojekte.md @ Commit 85c1174d, Par. 1 + 4.2):
# die vier Traeger-Unterprojekte in Stufenform (KON43-01: Stufe N+1 haengt NUR an Stufe N,
# nie umgekehrt). Der Monolith libs/cache_engine/ bleibt daneben stehen bis zum Split W7/#88.
# NICHT verwechseln mit modules/ (V41.E4-Altskelett, abgeloeste Ordnung, nur README).
#
# add_subdirectory-Kette in STUFENREIHENFOLGE -- die Reihenfolge ist Teil des Vertrags:
# jede Stufe N+1 prueft per FATAL_ERROR, dass ihre Vorstufe N als Target existiert, und
# linkt sie dann ("erst laute Compile-Fehler, dann verschieben").

add_subdirectory(planner)   # Stufe 1: comdare_planner
add_subdirectory(ceb)       # Stufe 2: comdare_ceb           -> linkt comdare_planner
add_subdirectory(tier)      # Stufe 3: comdare_tier_emission -> linkt comdare_ceb
add_subdirectory(hybrid)    # Stufe 4: comdare_hybrid        -> linkt comdare_tier_emission
