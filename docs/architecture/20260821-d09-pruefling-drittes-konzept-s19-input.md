# D-09 -- PRUEFLING = DRITTES KONZEPT: S-19-INPUT (P-H/#89, 2026-08-21)

Traeger: Wellenplan v2 Par.19.4 D-09 ("Pruefling = drittes Konzept, XML-beschraenkbar 1..3,
ERWEITERT den Permutationssatz (KON30-02/KON31) -> S-19-Input + prt-art-Rolle").
Quellen-Verbatim: Ledger KON30-02 (12.08.2026, Owner-R-2) + KON31 (Praezisierung).
Dieses Dokument ist der geforderte S-19-INPUT: es beschreibt die STRUKTUR, mit der die
Planungs-Simulation (S-19, Strang wt-ce-s19) den Pruefling-Anteil des Permutationssatzes
rechnen MUSS. Es legt KEINE Zahlen vor (Nenner-Doktrin: Mess-Permutation DYNAMISCH >32,
NUR S-19 RECHNET -- Owner 15.08.).

## 1. Das dritte Konzept (KON30-02, Owner-verbatim-gestuetzt)

Das PRUEFLINGS-TEST-KONZEPT ist eine EIGENE Achse der Verbund- und Teststruktur --
weder Traeger-Stufe (Binary) noch Phase (Modi):

- IST: der Pruefling (PRT-ART; seit P-H/#89 foermlich ALLE 33 Paper als Prueflinge,
  `profile_facade/paper_pruefling_registry.hpp`) bietet je ORGAN-Achse ZUSAETZLICHE
  Achsen-Algorithmen an -- im Pruefling verbucht und versioniert, MIT EIGENEM STEMPEL
  UND ALLEM (I-5-Mechanik: Achsen-Tokens in der Organ-Stempel-Zeile,
  `profile_facade/pruefling_stempel_farben.hpp`; KEINE Merge-Zeile).
- XML: beschraenkbar auf BELIEBIGE 1 BIS 3 angebotene Varianten (KON30-02: "in der XML
  auf beliebige 1 bis 3 angebotene Varianten beschraenkt werden kann").
- REISEWEG (KON31-Praezisierung): Unter-Achse ZUR LAUFZEIT in den MESS-ACHSEN DES
  PLANERS -> in der CEB kommuniziert (zu DEREN Laufzeit) -> gibt die TIER-BINARY-COMPILES
  frei. Die erweiterten Organ-Algorithmen selbst bleiben ZWEIPHASIG freigegeben
  (System-Achse -> Compile-Time der Tier-Binaries).
- WIRKUNG: ERWEITERT den Permutationssatz eines Experiment-Rahmens.
- GRENZE: betrifft NUR die Organ-Achsen-Erweiterung zur Compile-Time der Tier-Binaries
  und zur Laufzeit der CEB, nach Planer-XML-Plan.

## 2. Struktur-Formel fuer S-19 (KEINE Zahlen -- S-19 rechnet)

Der Pruefling-Anteil geht als MULTIPLIKATIVER, per XML gedeckelter Faktor in den
Permutationssatz ein, und zwar JE ORGAN-ACHSE, die der Pruefling erweitert:

- Je Organ-Achse a der CEB: |Angebot(a)| = |CE-Varianten(a)| + |Pruefling-Zusatz(a)|.
  Der Pruefling-Zusatz ist die im Pruefling verbuchte Varianten-Menge dieser Achse
  (heute real: die 3 abstrakten Marker P08 concurrency/olc_optimistic,
  P09 memory_layout/memory_layout_packed_bitmap, P33 allocator/vampir_nfp --
  `kPaperFarbTokens`; volle Prueflinge tragen ganze Kompositionen und treten als
  EIGENE Lebewesen auf, nicht als Achsen-Zusatz).
- XML-DECKEL: der Experiment-Rahmen darf den Pruefling-Zusatz je Achse auf 1..3
  angebotene Varianten BESCHRAENKEN (D-09). Ohne Beschraenkung gilt das volle
  verbuchte Angebot. Die Beschraenkung ist ein LIMIT (Teilmenge), nie eine Erweiterung.
- VERBUND-DIMENSION: der Pruefling-Verbund selbst ist die dritte Dimension
  (PrueflingVerbundStrategy Verbund1_CeOnly/Verbund2_Replace/Verbund3_Union,
  `anatomy/pruefling_merge.hpp`); Abwesenheit von <phases> im Experiment-XML leitet
  ALLE DREI ab (KERN-A; die 33 Paper-Experiment-XMLs
  `algorithm_profiles/paper_experiments/*.experiment.xml` nutzen genau das).
- LAGER-SKIP (KON110-05 R-3): Verbund1 ist bei vollstaendigem Lager (Binaries+Messwerte)
  skippbar, Verbund2+3 werden IMMER gemessen -- S-19 darf den Verbund1-Anteil nur unter
  dieser Bedingung aus dem Rechen-Nenner nehmen (heute NICHT gebaut, kein automatischer
  Lager-Abgleich -- als Simulation-Annahme ausweisen, nicht als Ist).

## 3. prt-art-Rolle

prt-art ist das PRUEFLINGS-Organ-Angebot (D8/KON30-02): die Reimplementierungen liegen in
prt-art `prt_art/legacy_reimpl/P<nn>-*` (14 Baeume) + `prt_art/algorithm_profiles/`
(prtart_pruefling.profile.xml, prt_art_axis_registry.xml, permutation_axes_extension.xml).
Die Karten-Formulierung "ext/paper-Organisation" existiert am Objekt NICHT als Pfad
(Explore-Befund TEIL 1.3 der Strang-Ergebnisdatei); die Organisation ist legacy_reimpl/.
Der Pruefling reicht seine Achsen-Zusaetze ueber die IPrueflingFactory-/Slot-Form
(prt_art_merge_reference.hpp) in die CEB-Organ-Achsen -- der 1..3-Deckel wirkt auf GENAU
diese Zusatz-Menge.

## 4. Abgrenzung / Nicht-Gegenstand

- KEINE statischen Nenner in diesem Dokument (NUR S-19 rechnet; Owner 15.08.).
- Die XML-Grammatik des 1..3-Deckels ist s13-Schema-Zug-Arbeit (Kollisions-Auflage #89);
  der Bedarf ist in der Strang-Ergebnisdatei als Schema-Bedarf Nr. 6 gelistet.
- ORT != ZEIT: die Unter-Achsen-Verortung (Planer-Mess-Achsen) impliziert KEINE
  Phasigkeit (KON8-12/KON13).
