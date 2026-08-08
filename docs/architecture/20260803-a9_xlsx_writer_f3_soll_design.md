# A9 DESIGN-DOSSIER (NEUFASSUNG) — xlsx-Writer (F3): Vendoring, API, Dateinamen, Lager-Ablage, Scheiben-Plan

Datum: 03.08.2026 · Design-Agent Fable 5 xhigh · Status: DESIGN-NEUFASSUNG (read-only, kein Commit)
Ersetzt: Erstfassung 02.08. (existierte NIE als Repo-Datei; Inhalt nur im Backup-JSON
`super docs/sessions/backups/20260802-welle3-parallel-workflows/00-welle3-result.json`,
result.designs.designs[2], key `A9-xlsx-writer-design-dossier-20260802`, 23.837 Zeichen).
Alle Fundstellen dieser Neufassung sind am ce-Ist **0d7a0d92** (dev-Stand NACH Welle-2d-Landung)
NEU verifiziert — die Erstfassungs-Anker waren durch Welle 2d (Transport/Bestandslog/Flag-Grammatik)
teils verschoben oder trugen falsche Pfad-Praefixe (profile_facade/heuristik liegen unter
`libs/cache_engine/`, NICHT unter `builder/`); alles unten ist korrigiert.

ZWEITVERIFIKATION 03.08. nachmittags (unabhaengiger Zweitlauf, alle Anker erneut am Ist geprueft):
development-HEAD = **1cb1efbf** = 0d7a0d92 + EIN reiner cppcheck-Suppression-Lint-Commit (keine
Logik). Die im Erstlauf zitierten Doc-Commits 743078fa/5c1ac257 liegen aktuell auf KEINEM Branch
(`git branch -a --contains 5c1ac257` leer, kein Vorfahr von HEAD) — Landungs-/Merge-Frage gehoert
dem Lead (2d-Landungs-Task), beruehrt dieses Design nicht. Einzige Anker-Korrektur des Zweitlaufs:
`axis_error.hpp` hat 487 Zeilen — der Erstlauf-Anker ":511-531" existierte nicht und ist unten
durch die korrekten Token-Anker `:121-129` ("failed", nie Null) + `:148-156` ("gesperrt", D1)
ersetzt. Alle uebrigen Anker: bestaetigt.

Paket: Katalog A9 (`super docs/sessions/20260802-KATALOG-offene-strecke-gesamtplan.md:25`),
Kategorie A = trigger-blockierend (`:13`); Bauplan `:43` ("Pflicht vor Trigger: ... A9 xlsx");
GATE 9 (`super docs/sessions/backups/20260802-kontext-neugruendung-abend-workflows/00-SYNTHESE-lage-dossier.md:236`);
ABGLEICH 03.08. fuehrt A9 OFFEN (`ce docs/sessions/20260803-ABGLEICH-katalog-gesamtplan-ist-stand.md:20`).
Owner-Spezifikation F3 (01.08.): "eigener Writer, Fork/Orientierung libxlsxwriter zulaessig;
xlsx = Default-Auswerteformat, Unter-Achsen im Dateinamen."

Pfad-Konvention: ce = `/home/comdare/Projekte/Research/probst-diplomarbeit-cache-engine/Code/external/comdare-cache-engine`;
super = `/home/comdare/Projekte/Research/probst-diplomarbeit-cache-engine`. Anker unten ce-relativ, wenn nicht anders markiert.

---

## NACHTRAG 08.08.2026 — DIE BLATTFORM IST ENTSCHIEDEN (Owner)

**Dieses Dossier vom 03.08. liess die Blattform offen; es beschrieb Vendoring, API, Dateinamen und
Ablage.** Am 08.08. hat der Owner die Blattform entschieden und noch am selben Tag praezisiert. Der
Nachtrag steht hier, weil dieses Dokument die **technische Quelle im CE-Scope** ist — der Bau findet
den Entscheid sonst nicht. **Primaerquelle und Verbatim:** `super
docs/DIPLOMARBEIT-ZIELE-OFFENE-PUNKTE-LEDGER.md`, Abschnitte *OWNER-ENTSCHEID: DIE BLATTFORM*
(super `3326ac7a`) und *OWNER-NACHTRAG … Prozess UND Thread* (super `f94345c8`, Punkte N-1..N-7).

### Die Blaetter

| Ebene | Sheets | Zeilen | Spalten |
|---|---|---|---|
| compare | 1 je Last-Messungs-Rekombination | Funktionsaufrufe | Verweis + akkumulierter Wert |
| Macro | **1 je FUNKTION** | Aufruf-Zeitpunkte | Aufrufer · Zeitpunkt · Messwerte |
| Micro | **1 je ACHSE** | Achsen-Aufrufe | Aufrufer · Zeitpunkt · Messwerte |

**Blattzahl = `1 + |Funktionen| + |Achsen|` — fest.** Sie waechst **nicht** mit der Lauflaenge und
nicht mit der Nebenlaeufigkeit. Das war der Grund des Entscheids: ein Blatt je Mess-Bereich je Ebene
waeren zu viele. Stattdessen werden **die Aufrufe zu Zeilen** und sequentiell in das Blatt ihrer Achse
bzw. ihrer Funktion geschrieben. Verbunden ueber **interne Hyperlinks** zum Hinabsteigen
(compare -> Funktion -> Achse).

**CSV bleibt flach** — eine Datei je Blatt, **Ebene anfuehrend** und **Zeitraum parsebar** im Namen.
`CSV xor xlsx` bleibt Strategy Pattern (Abschnitt 4 unten unveraendert gueltig).

**Fuer den Writer heisst das: eine zweite Blattsorte in derselben Mappe** — gleiche Factory, gleiche
31-Zeichen-Wache, gleiches INFO-Sheet. **Kein zweiter Writer.**

### Der Aufrufer ist ein Paar, kein Name

> *„Der Aufrufer ist also nicht uniform, sondern ein Prozess UND einer von dessen Threads."*

| Spalte | Inhalt |
|---|---|
| **Prozess** | der Prozess, unter dem der Thread laeuft |
| **Thread** | die Thread-Kennung — Filterschluessel fuer die Zerlegung |
| **Aufrufer** | die Stack-Kante: aus welcher Mess-Ebene der Besuch kam |
| **Checkpoint** | `IN` (Interface-enter) oder `OUT` (return) |
| **Zeitpunkt** | ueber Threads hinweg vergleichbare Zeitbasis |

Der Thread ist **systemisch ein Visitor** ueber alle Mess-Ebenen unter einem Prozess und bewegt sich
**immer von einer hoeheren Mess-Stufe in eine tiefere und auf dem Stack sauber zurueck**. Das ist
GoF-Visitor und fuegt sich in die Doktrin *nur Lehrbuch-Entwurfsmuster*.

### Je Aufruf ZWEI Zeilen — und warum das keine Notationsfrage ist

`IN` und `OUT` werden **einzeln** erfasst, die Paarung erst beim Lesen gebildet. Das folgt zwingend aus
der Invariante: **eine Zeile je *abgeschlossenem* Aufruf wuerde einen Aufruf, der nie zurueckkehrt,
ueberhaupt keine Zeile schreiben lassen** — und genau der ist der Fall, der erkannt werden soll:

> *„Wenn ein Thread ein Interface betritt, aber es nicht wieder verlaesst, ist das eine Regression."*

Je `(Prozess, Thread, Interface)` muss die Checkpoint-Folge **balanciert** sein. Ein Ueberhang an `IN`
am Ende eines Laufs ist **kein fehlender Messwert, sondern ein Befund**.

### Mehrere Threads schreiben eingedampft, werden aber je Thread zerlegt

Die Zeilenfolge im Blatt ist die **Ankunftsfolge**, nicht die Folge eines Threads. Damit die Zerlegung
beim Lesen moeglich bleibt, muessen **Thread-Kennung und Zeitpunkt in jeder Zeile** stehen — kein
Weglassen „weil es sich aus der vorigen Zeile ergibt"; bei Nebenlaeufigkeit ergibt es sich nicht.

### Wo compare liegt — die Aufrufer-Kette ist geschlossen

Die **compare-Ebene liegt IN DER CEB, noch vor dem Pruefdock**. Die CEB fuehrt die Last-Sequenz durch;
innerhalb der Sequenz ist **die CEB — genauer einer ihrer Threads — am Pruefdock der Aufrufende einer
Tier-Binary-Interface-Funktion**:

```
CEB-Thread (compare, in der CEB, vor dem Pruefdock)
   ruft am Pruefdock -> Interface-Funktion der Tier-Binary   (macro)
                          ruft -> Achsen-Aufrufe             (micro)
```

Die Spalte *Aufrufer* eines **Macro**-Blatts nennt also einen **CEB-Thread**; die eines **Micro**-Blatts
die **Interface-Funktion samt Thread**, aus der der Achsen-Aufruf kam.

### Die Falle beim Macro-Wert

Die **Gattungs-Interface-Ebene verbindet** die Achsen-Aufrufe, **ueberwacht sie aber nicht**. Ihr Anteil
ist *Macro-Gesamt minus Summe der Micros* und muss **separat gemessen** werden. **Wer Macro aus den
Micros errechnet, verliert genau diesen Anteil und schreibt ihn faelschlich den Achsen zu.**

### Offen (Bau-Detail, kein Owner-Entscheid)

Die **Zeitbasis**: fuer die Zerlegung je Thread genuegt ein monotoner Zaehler je Prozess; fuer das
Erkennen echter **Gleichzeitigkeit** ueber Threads hinweg genuegt er **nicht**. Wird beim Bau
entschieden und hier nachgetragen.

---

## 0. GATE-EINORDNUNG — BENANNTE ABWEICHUNG (Review-Befund befunde[6], KORRIGIERT)

**Befund der Review (VERDIKT NACHBESSERN, genau EIN benannter Punkt):** Die Erstfassung stufte A9
STILL als "NICHT trigger-blockierend, aber PFLICHT vor der Auswertung (E.4)" ein. Das war ein
stiller Umbau der dokumentierten Einordnung und ist unzulaessig.

**Quellenlage (widerspruchsfrei erhoben):**

1. KATALOG `:13` fuehrt A9 in **Kategorie A** = "Trigger-blockierend (Pflicht VOR dem
   Voll-Bau-4-Trigger)"; `:25` = die A9-Zeile selbst.
2. Bauplan `:43` woertlich: "Nicht auf dem kritischen Pfad, aber **Pflicht vor Trigger**: A8
   Benchmarking-Schnitt, **A9 xlsx**, A10 P4-P6, A11 — werden in die Parallel-Spuren eingetaktet".
3. Die Gate-Kette fuehrt **GATE 9** (Restpakete inkl. "A9 xlsx-Writer (existiert im Code
   NIRGENDS)", Synthese `:236`) **VOR GATE 10** (Vorlauf J-Kette/heavy/Kalibrier, `:238`) und
   **GATE 11** (Trigger, `:240`).
4. **Bauplan-INTERNER Widerspruch:** Bauplan `:40` terminiert "xlsx-Writer-Start" als
   Parallel-Spur auf **Do 07.** — den Trigger-Tag selbst. Ein Paket, das erst am Trigger-Tag
   STARTET, kann nicht vor dem Trigger fertig sein. Dieser Widerspruch ist im Bauplan
   unaufgeloest und wird hier BENANNT (kein stiller Umbau in irgendeine Richtung).
5. Synthese `:259` (Widerspruch 10): F2/A8 + A9 = "Pflicht-Pakete, tauchen aber in keiner
   Welle 1-3 auf"; R9 (Owner 02.08.): "ALLES genannte wird voll gebaut" — Streichen ausgeschlossen.

**Technische Analyse (Grund der Erstfassungs-Lesart, bleibt als Analyse gueltig):** A9 beruehrt
KEINE Binary-Identitaet — kein Stempel, kein SHA512-Fingerprint, kein golden, kein ABI, keine
Mess-Zeit-I/O (Abschnitt 3.3/7). Der Voll-Bau-4 und die Messung koennen technisch ohne xlsx-Writer
laufen; gebraucht wird er zwingend erst fuer die Auswertung (KATALOG E.4: "Auswertung
(xlsx-Lager-Baum, ...)", `:68`). Die Lesart ist technisch plausibel — sie AENDERT aber die
dokumentierte Gate-Einordnung und ist damit **Manager-/Owner-Entscheid, keine Design-Entscheidung**.

**Bindende Einordnung dieser Neufassung (konservativ, Bauplan-Wortlaut):** A9 gilt als
**Pflicht vor Trigger** (Kategorie A, GATE 9, Parallel-Spur) — so lange, bis der vorgelegte
Entscheid (Eskalations-Vorlage **V-A9-5**, Abschnitt 10) anders entscheidet. Der Scheiben-Plan
(Abschnitt 6) ist so geschnitten, dass beide Ausgaenge bedienbar sind: S1/S2 sind sofort und
unabhaengig baubar; S3-S5 haben ihre einzige harte Fremd-Abhaengigkeit an A13-M3 (INFO-Blatt-
Fingerprint-Form) bzw. A12 (NAS-Pfad, rein konfigurativ).

## 1. Befund-Bestaetigung (neu erhoben 03.08. am Ist 0d7a0d92): xlsx existiert im Code NIRGENDS

Voll-grep ueber `libs/`, `tools/`, `apps/`, `ext/`: genau **EIN** Treffer, ein Kommentar-Vorgriff in
`libs/cache_engine/include/cache_engine/measurement/axis_error.hpp:420` ("... die xlsx-/Lager-Strecke
uebernimmt es ueber dieselbe Vokabel"). Kein Writer, kein Vendor-Verzeichnis (`ext/io/` enthaelt nur
`liburing/`), kein CMake-Ziel, kein zlib im Bau (grep CMakeLists/cmake: 0 Treffer). Deckungsgleich:
Reviewer-Ist-Bestaetigung 03.08. und das codex-doppelreview-Backup (0 xlsx-Treffer; einziger
"A9"-Hit dort = unverwandtes Code-Label P5/A9a). A9 ist ein **Vollbau-Paket ab Null**.

## 2. Ist-Kartierung: CSV-/Mess-Ausgabe-Naehte (Anker NEU am 0d7a0d92, inkl. Welle-2d-Deltas)

| Naht | Ort (verifiziert 03.08.) | Rolle fuer A9 |
|---|---|---|
| WIDE-CSV-Schema (Single-Source, Semikolon) | `libs/cache_engine/builder/experiment_tree/cache_engine_builder_iterator.hpp:359` (`lazy_csv_header()`), `:462` (`format_csv_row`) | EINZIGE Datenquelle des xlsx-Writers; wird NICHT angefasst (Resume-/Header-Vergleich `:827-834` haengt daran) |
| Per-Binary `result.csv` + `.stamp` | Resume-Pruefung `:827-834` (`result.csv` + `result.csv.stamp`); **NEU (2d, CX-W4/F-B10):** Fehlbau-Zweig `:1540-1650` — Alt-Stand wird NIE ueberschrieben, sondern nach `result.csv.stale` UMBENANNT (Messdaten-nie-loeschen), Stamp-Entfernung fail-closed | bleibt CSV (Maschinen-/Resume-Format). A9 liest NIE stamps und NIE `.stale` (kein Resume-Anspruch, gescheiterter Alt-Stand — explizite Nicht-Input-Regel) |
| Offizielle Gesamt-CSV `e4_xml/measurements.csv` | `libs/cache_engine/profile_facade/profile_run_entry.hpp:880-882` + `experiment_run_entry.hpp:406` (+ Pfad-Doku `:106/:158`); Alt-Treiber `libs/cache_engine/builder/experiment_driver/experiment_driver.cpp:515` | Haupt-Input des A9-CLI |
| Mess-Sink-Naht (Storage #51, No-Op-Default) | `MeasurementSinkFn` definiert in `libs/cache_engine/builder/artifact_transport/artifact_cache.hpp:179-181`, Alias Iterator `:79`, Struct-Member `:190`; Feuerpunkte Iterator `:1837-1841` und `profile_run_entry.hpp:882` | Vorbild fuer die A9-Ablage-Naht: additiv, leer = No-Op = golden/CI byte-identisch |
| **NEU (2d, CX-W1):** Mess-Pfad-Push-Haertung | `mess_pfad_synchron_push` Iterator `:908` (Definition), `:1835` (Aufruf): synchroner Push, bei Wurf klassifiziert loggen + unbestaetigten Bestandslog-Eintrag VERWERFEN | bestaetigt die A9-Doktrin "Mess-Zeit bleibt xlsx-frei": die Naht ist synchron und eng budgetiert; A9 haengt sich NICHT hier ein, sondern arbeitet NACH der Messung |
| Auswerte-Leser (CSV-konsumierend) | `libs/cache_engine/heuristik/measurement_curve_loader.hpp:3-6` ("Header-only, kein Python"; fuehrt Verwerf-Tokens woertlich), `libs/cache_engine/builder/best_binary_selector/best_binary_selector_main.cpp:22` (header-getrieben, "lazy_csv_header WIDE-Schema"), `libs/cache_engine/builder/experiment_tree/result_ingest.hpp:33` | bleiben CSV-Leser; A9 ergaenzt die menschliche Auswerte-AUSGABE |
| Zell-Token-Vokabular (zementiert) | `axis_error.hpp:121-129` ("failed" nie 0/still, sicherer Default), `:148-156` (D1-Token "gesperrt"), `:96-97` (Token-Abgrenzung woertlich), `:410-421` (`nicht_gebaut` per static_assert zementiert; `:420` verlangt woertlich, dass die xlsx-/Lager-Strecke DIESELBE Vokabel uebernimmt) | Writer reicht `failed`/`gesperrt`/`nicht_gebaut` 1:1 durch — NIE uminterpretieren, NIE leeren |
| Legacy-Zweitkopf (RFC-4180, Komma) | `libs/cache_engine/builder/commands/result_aggregator.hpp:92` (`result_csv_header()`, Ausgabe `:112`) | NICHT Andock-Punkt; Alt-Format, LED-75-Kandidaten-Pruefung (nicht Teil von A9) |
| Werkzeug-Praezedenz CSV->Bericht | `tools/latex_anhang/` (CSV -> LaTeX-booktabs; `PROJECT_LAYER_MAP.md`) | Layer-Beleg: Bericht-WRITER = ce-Mechanik ("WIE"), Auswerte-INHALT = super |
| Vendoring-Praezedenz | `ext/io/liburing/COMDARE-VENDOR-PROVENANCE.md` (Stufe-1-Snapshot, kein Submodule, Prune-Protokoll, Consumer-only-Target; **Nachtrag 23.07.:** upstream `.gitignore` im Snapshot ENTFERNEN — schloss generierte Header aus und brach CI) | Blaupause fuer A9-S1 inkl. der gitignore-Lehre |
| Achsen-Segment-Grammatik | `libs/cache_engine/builder/experiment_tree/axis_path_serialization.hpp:40-50` (`kCompositionAxisNames` 18 Achsen inkl. `persistence_target`; `serialize_axis_segment` = "axis=value") | Single-Source fuer Organ-Segmente; A9-kv-Grammatik nutzt dasselbe `=`-Muster |
| Archiv-Erstbeleg fuer S5 | super `measurement/20260726-164259-d03-strukt-r-erstbeleg/` (verifiziert vorhanden) | read-only Test-Input (Alt-Mess-CSV = Archiv) |

Kern-Konsequenz (unveraendert, KERN 26.07. §6 `:91` nennt die CSV-Doktrin woertlich "Vorstufe"):
Die CSV-Strecke bleibt byte-unberuehrt. A9 ist eine rein ADDITIVE Ablage-/Auswerte-Stufe dahinter:
dieselben Zeilen, gerendert als xlsx in den Messdaten-Lager-Baum; CSV = Fallback derselben Factory (F3).
**Flag-Grammatik-Delta (2d, CX-W5/W6):** die Versions-Flag-Grammatik `vX.Y.Z`+HW-Flag ist seit der
2d-Landung im Planer gebaut (Wachen ueber volle Organ-Population, Commit 43a1b65a) — das INFO-Blatt
(Abschnitt 4) zitiert Versionen in der GERENDERTEN Form nach Q10 (`@X.Y.Zc`, ohne 'v').

## 3. Vendoring-Entscheid libxlsxwriter

### 3.1 Lizenz-Check (online verifiziert 02.08.; beim Vendoring LIVE nachziehen)

- **libxlsxwriter**: "FreeBSD license" = **BSD-2-Clause**, Copyright John McNamara 2014-2026.
  Neuestes Release zum Pruefzeitpunkt **1.2.4 (06.01.2026)**. Einzige externe Abhaengigkeit: **zlib**.
- Gebuendelte third_party (libxlsxwriter.github.io/license.html): queue.h/tree.h (BSD),
  **minizip** (zlib-Lizenz), **tmpfileplus** (**MPL-2.0** — vermeidbar, s.u.), **dtoa Milo Yip**
  (MIT, optional `USE_DTOA_LIBRARY`), **Openwall MD5** (Public Domain, abschaltbar).
- Vertraeglichkeit: BSD-2/zlib/MIT/PD sind mit der ce-Dual-Lizenz (Owner 02.08.: Rechte BEP Venture
  UG, Forschung/Lehre frei; Historie Apache-2.0) problemlos; Doktrin-Deckung explizit: "vendored
  ext/ behaelt eigene Lizenzen" (Synthese `:72`). MPL-2.0 (tmpfileplus) waere file-level-Copyleft —
  wir kompilieren es NICHT (`USE_STANDARD_TMPFILE`, POSIX `tmpfile()`) und prunen die Datei im Snapshot.

### 3.2 Entscheid (Empfehlung, F3-gedeckt): Stufe-1-Vendor

Nach Doktrin vendor->faithful->selfcontained (Memory `feedback_vendoring_doctrine...`), exakt nach
liburing-Blaupause (`ext/io/liburing/COMDARE-VENDOR-PROVENANCE.md`):

1. `ext/io/libxlsxwriter/` — Snapshot Tag `v1.2.4` (**Tag-Objekt + Commit-Hash beim Vendoring LIVE
   erheben und ins Provenance-Doc schreiben; NIE aus diesem Dossier abschreiben** — nie-raten-Doktrin),
   `.git/` entfernt (Haus-Doktrin: keine Submodules), **upstream `.gitignore` entfernen**
   (liburing-Nachtrag 23.07.: funktionslos im Snapshot, schadet aktiv), geprunt auf `src/` +
   `include/` + `third_party/{minizip,md5,dtoa}` + Lizenzdateien; ENTFERNT: `test/`, `examples/`,
   `docs/`, Sprach-Bindings, `third_party/tmpfileplus/` (MPL, ungenutzt), CI-/Paket-Dateien.
   Eigene `CMakeLists.txt` + `COMDARE-VENDOR-PROVENANCE.md` (inkl. Prune-Protokoll).
2. `ext/io/zlib/` — zlib ist im ce-Bau am 0d7a0d92 weiterhin NICHT vorhanden (grep CMakeLists/cmake:
   0 Treffer, neu verifiziert). Empfehlung: **mit-vendorn** (zlib-Lizenz, ~25 C-Dateien, gleiches
   Snapshot-Muster) statt `find_package(ZLIB)` — self-contained ueber die 8er-Docker-Matrix, keine
   dev-Header-Annahme (sauberster, nicht einfachster Weg). Umkehrbar per CMake-Option
   `COMDARE_SYSTEM_ZLIB` (Default OFF). [Vorlage V-A9-2]
3. CMake-Target `comdare_vendored_xlsxwriter` (static, Consumer-only wie liburing: wird NUR gezogen,
   wenn ein Consumer es braucht — `ext/io/liburing/COMDARE-VENDOR-PROVENANCE.md:45-46` Praezedenz)
   mit Defines: `USE_STANDARD_TMPFILE` (kein MPL-Code), `USE_DTOA_LIBRARY` (MIT; locale-unabhaengige,
   byte-deterministische double-Ausgabe — Determinismus-Hauswert), MD5 bleibt PD-Bundled.
4. Buchfuehrung: `LICENSE_AUDIT_EXT.md` + `NOTICE` um beide Snapshots ergaenzen (beide am Ist
   vorhanden, verifiziert); kein Python, keine Fremdsprache in der Buildchain (libxlsxwriter-Bau =
   reines C; Python nur upstream fuer Docs/Tests, die geprunt sind) — C++/Drogon-only-Doktrin
   unberuehrt (C-Vendor-Praezedenz: liburing).

### 3.3 Fork-Entscheid: Snapshot statt GitHub-Fork, statt Eigenbau (Begruendung)

- **Kein GitHub-Fork/kein neues Repo:** Vendoring in `ext/` ist ein Snapshot im BESTEHENDEN Repo —
  es entsteht KEIN Repo, KEIN Remote, KEIN Submodule; die Remote-Anlage-nur-mit-je-Repo-GO-Regel
  greift nicht. F3 ("Fork ... zulaessig") wird als Stufe-1-Snapshot mit lokalen Patches (falls je
  noetig, im Provenance-Doc protokolliert) eingeloest. [Vorlage V-A9-1]
- **Kein Eigenbau des xlsx-Formats:** xlsx = ZIP-Container + OOXML-Sheets; ein self-contained
  Minimal-Writer waere Neuimplementierung von ZIP(deflate)+XML-Escaping+Format-Details und verstiesse
  gegen die GEORDNETE Vendoring-Doktrin (vendoren, wenn moeglich — exakt die Begruendung, mit der die
  rohe io_uring-UAPI-Fassung zugunsten liburings verworfen wurde, Provenance-Doc `:48-53`).
  libxlsxwriter ist C (wie liburing), dependency-arm (nur zlib), BSD-2, aktiv gepflegt.
- **Eigener Writer im Sinne von F3** = unsere C++-Wrapper-Schicht (Abschnitt 4): Interfaces,
  Factory, Dateinamen-/Baum-Grammatik, INFO-Blatt, Fehlerklassen — das ist Comdare-Code; der Vendor
  liefert nur die xlsx-Serialisierung darunter.

## 4. Writer-API (Ziel-Schnittstelle zur Auswertung)

### 4.1 Einordnung und Bauform

- Ort: `libs/cache_engine/builder/lager_ablage/` (Schwester von `builder/bestandslog/` — beides
  Lager-Strecke; am Ist verifiziert: `bestandslog/` existiert mit 10+ Headern, `lager_ablage/`
  existiert noch nicht). Naming nach ce-Hausstil (PascalCase-Domain-Klassen, snake_case-Methoden,
  deutsche Domaenen-Begriffe wie in der Bestandslog-Strecke; die Erstfassungs-Referenz
  `NAMING_CONVENTIONS.md` existiert NICHT im Repo und ist gestrichen — Stil-Beleg ist der
  Bestandslog-/Transport-Bestand selbst).
- **header-only-Wrapper? Antwort: geteilt.** Interfaces + Factory + CSV-Fallback = header-only
  (ce-Hausform). Das xlsx-Backend = EIGENE TU (`xlsx_ergebnis_writer.cpp`) in einem kleinen static
  Target `comdare_lager_ablage`, das `comdare_vendored_xlsxwriter` **PRIVATE** linkt. Grund: die
  C-Header (`xlsxwriter.h`) duerfen nicht transitiv in Consumer lecken, und Nicht-Nutzer
  (golden-/CI-Pfad) duerfen keinen Link-Zwang auf den Vendor bekommen.
- Format-Wahl = Laufzeit-Draht (Haupt=CT/Unter=RT-Regel: Rueckschrieb-Methoden sind UNTER-Achsen
  der Mess-Tooling-Haupt-Achse, LED-54-T2) => GoF **Abstract Factory** mit Runtime-Auswahl
  xlsx|csv. **KEIN std::variant** — Verbotskanon; die einzige definierte Ausnahme ist die
  Hybrid-Tier-Stufe hinter der CEB (Owner-E1 02.08.), und A9 ist Werkzeug-Layer, nicht Tier:
  hier schlichte Interface-Polymorphie.

### 4.2 Schnittstellen (Soll)

```cpp
namespace comdare::cache_engine::builder::lager_ablage {

enum class ErgebnisFormat { xlsx /*DEFAULT (F3)*/, csv /*Fallback derselben Factory*/ };

struct SheetSchluessel {         // EINE gewaehlte Unter-Achsen-Permutation (Reihenfolge = bindende
    std::string mess_unter;      // Sortierung: Mess-Unter -> System-Unter -> Organ-Unter)
    std::string system_unter;
    std::string organ_unter;
};

class IErgebnisBlatt {           // ein Sheet (xlsx) bzw. eine Datei (csv-Fallback)
  public:
    virtual ~IErgebnisBlatt() = default;
    virtual void kopf(std::span<std::string const> spalten) = 0;   // Spalten aus lazy_csv_header()-Split
    virtual void zeile(std::span<std::string const> felder) = 0;   // Felder aus format_csv_row-Split;
};                                                                 // failed/gesperrt/nicht_gebaut 1:1

class IErgebnisMappe {           // EINE xlsx-Datei bzw. EIN CSV-Dateisatz am selben Baum-Blatt
  public:
    virtual ~IErgebnisMappe() = default;
    virtual void info_blatt(MaschinenSysinfo const&,          // machine_identity.hpp, Provenienz-Kette,
                            HauptAchsenBelegung const&,       //   "n/a statt Null"
                            KonstantenMeta const&) = 0;       // weggelassene konstante Spalten (Doktrin)
    virtual IErgebnisBlatt& blatt(SheetSchluessel const&) = 0; // 1 Sheet je Unter-Achsen-Permutation
    virtual void schliessen() = 0;                             // atomar: tmp-Datei + rename; wirft
};                                                             //   ErgebnisSchreibFehler (#29-Framework)

class ErgebnisMappenFactory {    // Abstract Factory; Default xlsx, csv einstellbar (XML/CLI)
  public:
    [[nodiscard]] static std::unique_ptr<IErgebnisMappe>
    oeffne(std::filesystem::path const& blatt_verzeichnis,     // Baum-Blatt (Abschnitt 6)
           std::string const& dateiname_stamm,                 // Abschnitt 5 (ohne Endung)
           ErgebnisFormat f = ErgebnisFormat::xlsx);
};
} // namespace
```

Verhaltensvertraege:

1. **Quelle bleibt CSV**: Der Writer konsumiert ausschliesslich das WIDE-Schema
   (`lazy_csv_header()`-Spalten, header-getrieben wie `best_binary_selector_main.cpp:22`); er
   definiert KEINE eigene Spaltenmenge. Fehlende Spalten alter CSVs => leer/n-a
   (Datenerhaltungs-Muster der bestehenden Leser). `result.csv.stale` ist NIE Input (2d-Regel).
2. **Zell-Token-Treue**: `failed` (nie 0/still), `gesperrt` (D1-Vorrang), `nicht_gebaut` werden
   woertlich durchgereicht — das Token-Vokabular ist per static_assert zementiert
   (`axis_error.hpp:410-421`) und `:420` verlangt die Vokabel-Wiederverwendung durch genau diese
   Strecke.
3. **xlsx-Limits als Fehlerklassen**: Sheet-Name hart <=31 Zeichen und ohne `[]:*?/\` => Sheets
   heissen deterministisch `S001..Snnn` (Reihenfolge = mixed-radix ueber die
   Unter-Achsen-Permutationen in bindender Sortierung), Klartext-Zuordnung in der Sheet-Legende des
   INFO-Blatts [Geschmacksfrage V-A9-6]. Zeilenlimit 1.048.576 => `ErgebnisSchreibFehler{zeilenlimit}`
   statt stillem Truncate (honest-Doktrin).
4. **Fehlerklassen-Anschluss AKTUALISIERT (Owner-E6 + A15-Ist):** Das #29-Framework ist seit 02.08.
   real im Bau — FK-0/1/2 FERTIG (ABGLEICH `:26`), Heimat `axis_error.hpp` (+`machine_identity.hpp`,
   `operating_system_probe.hpp`). `ErgebnisSchreibFehler` (zeilenlimit/namenslimit/zip_fehler) wird
   in DIESE Framework-Welt eingeordnet (eigene Domaene der Lager-/Ablage-Strecke, disjunkt zu
   D1/InfraErrorClass nach dem INC-29.2-Drift-Guard-Muster `axis_error.hpp:423-425`), NICHT als
   Insel-Enum erfunden. ID-Abstimmung mit dem A15-Strang bleibt Pflicht VOR A9-S3.
5. **CSV-Fallback = dieselbe Factory**: je Sheet EINE Datei `<stamm>__S001.csv` + `<stamm>__INFO.csv`
   (Owner-KERN 26.07. §6: "CSVs werden im FACTORY PATTERN je Sheet einzeln gebaut").
   Semikolon-Konvention wie `lazy_csv_header()`.
6. **Determinismus**: dtoa-Backend + feste Sheet-/Spalten-Reihenfolge => gleicher Input ergibt
   byte-gleiche Sheets-XML (ZIP-Zeitstempel auf Epoche gepinnt); testbar (Locale-Negativprobe
   LC_NUMERIC=de_DE).
7. **INFO-Blatt** (Pflichtinhalt, an die Owner-Entscheide 02.08. angepasst):
   - Sysinfo der messenden Maschine (HW-Erkennung, Provenienz configured_measured >
     spd_jedec_base > declared, "n/a statt Null"; `machine_identity.hpp`).
   - Verwendete HAUPT-Achsen aller drei Typen (Mess/System/Organ) + konstante (weggelassene)
     Unter-Achsen als Meta-Eintraege + Sheet-Legende + Quell-CSV-Pfade + Schema-/Writer-Version.
   - **Stempel-Zitate in A13-Form (Owner-E2/Q1/Q2/Q3/Q10):** KEINE merge-Zeile (existiert nicht
     mehr); Meta-Meta-Eintraege stehen dynamisch am ENDE der jeweiligen Realm-Zeile; Ebenen-Kodierung
     = Klammer-Form (Q1); Algorithmus-Versionen in der GERENDERTEN Form `@X.Y.Zc`/`@X.Y.Zce`
     (Q3-Flag-Grammatik + Q10 ohne 'v'; 'e' = experimenteller Pruefling-Algorithmus); erweiterte
     hierarchische Algorithmus-Namen nach dem Muster `prt-art.memory.abc@1.0.0c` (Q2) werden 1:1
     zitiert.
   - **SHA512-Fingerprint-Referenzen der vermessenen Binaries = post-M3-Form:** A13-M3 stellt den
     Fingerprint-Preimage um (POD v6, merge-Zeile faellt). Der INFO-Blatt-Teil, der Fingerprints
     zitiert, wird gegen die **A13-Lese-API** gebaut und laeuft erst NACH gelandetem M3 scharf
     (Sequenz-Regel in Abschnitt 7) — keine eigene Fingerprint-Berechnung in A9.

### 4.3 Anschluss an die Auswertung (Konsument)

- Haupt-Konsument = neues CLI `tools/mess_report/` (`comdare_mess_report`): offizielle Mess-CSV(s)
  -> xlsx-Baum. Subcommand-Stil wie Planer-CLI (clig.dev): `report render|plan|version`; `plan` =
  dry-run und druckt den Ziel-Baum literal (Kein-Haken-ohne-Ausgabe-tauglich).
- Heuristik-Strecke (`heuristik/measurement_curve_loader.hpp`, `break_even.hpp`, `axis_spline.hpp`)
  bleibt CSV-Leser — xlsx ist AUSGABE-Default der Auswertung, nicht deren Eingabeformat.
- **Mess-Zeit bleibt xlsx-frei** (durch 2d bekraeftigt): `mess_pfad_synchron_push` + measurement_sink
  + result.csv-Pfade unveraendert (Mess-I/O-Doktrin: synchron + eng, keine zusaetzliche
  I/O-Contention in der Messphase). Die xlsx-Erzeugung laeuft NACH der Messung
  (Auswerte-/Einlagerungsschritt). Lesart-Bestaetigung als Vorlage V-A9-4 (KERN §6 nennt die
  CSV-Doktrin ausdruecklich "Vorstufe").

## 5. Dateinamens-Schema mit Unter-Achsen (Single-Source `ergebnis_dateiname.hpp`)

Doktrin (KERN 26.07. §6 `:91-93`): Name = NUR Datum + Uhrzeit + dynamische
Unter-Achsen-Variablen (aus der CoR-Filterkette der Auswertung); Haupt-Achsen liegen im
ORDNER-Pfad und als Metadaten IN der Datei; nie sich aendernde Variablen werden WEGGELASSEN
(Meta-Eintrag im INFO-Blatt).

Grammatik (EBNF-artig, ASCII, shell-sicher):

```
dateiname   := datum "-" zeit "_" kvkette "." endung
datum       := YYYYMMDD                  (UTC)
zeit        := HHMMSS                    (UTC; kollisionsfrei je Blatt-Verzeichnis)
kvkette     := kv ("+" kv)*              (Reihenfolge: Mess-Unter -> System-Unter -> Organ-Unter,
                                          innerhalb des Typs bindende Achsen-Sortierung)
kv          := unterachse "=" wert       ("=" wie serialize_axis_segment,
                                          axis_path_serialization.hpp:45-50;
                                          "+" wie build_version-Suffixe +cxx=/+opt=)
wert        := [a-z0-9._-]+              (sanitisiert; "sweep" wenn die Variable im File selbst
                                          ueber Sheets laeuft)
endung      := "xlsx" | "csv"
```

Wachen: Gesamtname <= 200 Bytes (ext4/NAS-Komponentenlimit 255 mit Reserve); Ueberlauf =>
`ErgebnisSchreibFehler{namenslimit}` + deterministischer Kurzname
`<datum>-<zeit>_H=<sha512-hex16-der-kvkette>.xlsx` mit Voll-Kette im INFO-Blatt (nie stilles
Kuerzen).

Beispiele (drei, mit Ordner-Kontext aus Abschnitt 6):

1. Wallclock-Sweep ueber die Arbeitsmenge (Mess-Unter-Variablen dynamisch, Rest konstant=>weggelassen):
   `20260812-093011_measurement_category=wallclock+workload=ycsb_a+working_set_n=sweep.xlsx`
   — Sheets S001..S00n = je eine (workload x working_set)-Unter-Permutation; Legende im INFO-Blatt.
2. System-Unter-Gruppe (Compiler-Gruppe R-D) im Opt-Level-Vergleich auf EINEM OS:
   `20260812-101500_opt_level=sweep+atomic128=on.xlsx`
   — die OS-Unter-Achsen os_version/kernel/build (A14 FINALE DREI; **seit 02./03.08. GEBAUT:
   U1-U3 fertig, U4 offen**, ABGLEICH `:25`) sind hier maschinenkonstant => NICHT im Namen,
   sondern INFO-Metadaten; bei Cross-OS-Aggregat dagegen im Namen:
   `...+os_version=24.04+kernel=6.17.0-35+build=noble...`
3. Organ-Unter-Variable (Knoten-Shape) unter fixem Haupt-Blatt:
   `20260812-114205_node_fanout=sweep+prefetch_distance=4.xlsx`
   CSV-Fallback derselben Factory dazu:
   `20260812-114205_node_fanout=sweep+prefetch_distance=4__S001.csv` + `..__INFO.csv`

Abgrenzungen (an die Owner-Entscheide 02.08. angepasst):
- Dateinamen tragen KEINE Stempel und KEINE Versions-Flag-Grammatik — `vX.Y.Zc`/`@X.Y.Zc` (Q3/Q10)
  lebt in Stempeln/Registry/INFO-Blatt, NIE im Dateinamen.
- Unter-Achsen erscheinen NIE im Stempel (Stempel-Doktrin: "Unter-Achsen nur in Mess-CSV/xlsx") —
  genau diese Datei ist ihr Zuhause.
- Die Plattform-Erweiterung (Owner-E4: RISC-V, macOS M1, macOS x86 kommen JETZT) aendert am Schema
  NICHTS: target_isa/operating_system sind HAUPT-Achsen (Ordner-Ebene 2 + INFO-Metadaten), neue
  Plattformen sind neue Werte, keine neuen Namens-Segmente.

## 6. Ablage im Lager-Baum (beide Realm-Wurzeln)

### 6.1 Messdaten-Realm (HIER wohnt xlsx; Default-Ziel NAS measure-drop, konfigurierbar; git-Archiv super `measurement/<ts>/`)

Kaskade exakt nach KERN 26.07. §4 (`:56-61`) + §5 (`:65-79`):

```
<messdaten-wurzel>/                                        (NAS measure-drop; A12-Creds-Paket E-14)
  mess=vereint+load_framework=on/                          (Ebene 1: Mess-Kombinatorik der BEIDEN
                                                            Mess-Haupt-Achsen Mess-Tooling x load_framework)
  target_isa=amd64_v3+operating_system=linux+external_utils=avx2/
                                                           (Ebene 2: EIN Ordner-Level = GESAMTE System-
                                                            Haupt-Rekombination; Meta-Metas HINTEN
                                                            angehaengt, hinter load_framework on/off)
    01_read_path=search_algo-prt_art+cache_traversal-bfs/  (Ebenen 3-7: die 5 Organ-Gruppen-Ordner
    02_layout=node_type-b_tree+memory_layout-soa+.../       01..05 GESCHACHTELT in Speicherhierarchie-
    03_placement=.../04_execution=.../05_write_path_io=.../ Reihenfolge; Ordnername traegt die Werte-
                                                            Rekombination der Gruppen-Achsen)
      blatt/                                               (Ebene 8: Haupt-Blatt = vollstaendige
                                                            Haupt-Achsen-Bindung aller drei Typen)
        mess_unter=<...>/system_unter=<...>/organ_unter=<...>/
                                                           (Ebenen 9-11: die 3 Unter-Ebenen gegen
                                                            Sheet-Explosion, nur soweit noetig)
          20260812-093011_<kvkette>.xlsx                   (Abschnitt 5)
```

Regeln: Ordner-Segmente nutzen DIESELBE kv-Grammatik (`=` innen, `+` als Joiner) — Single-Source
`lager_pfad_grammatik.hpp`, geteilt mit dem A1-LB-Schnitt-Baum-Writer (GATE 6). **Ownership =
Manager-Entscheid** (Abschnitt 9, Punkt 2): wer zuerst landet, stellt die Single-Source, der andere
konsumiert — KEIN Fork der Grammatik. `complete-heuristik.log` je Knoten + SHA512-Overlay +
Truncate-Zustandsmaschine gehoeren dem A1-Baum-Writer (Abnahmen 1-6); A9 legt NIE selbst Knoten an,
sondern bezieht das Blatt-Verzeichnis vom Baum-Writer/Resolver (ein Schreiber je Knoten,
Lock-Regime der Lager-Strecke).

### 6.2 Binaries-Realm (KEIN xlsx)

Wurzel = System-Achse direkt -> Organ -> Mess-Typ als LETZTER/tiefster (KERN §4 `:62-63`; Default
minio, Ebene B async). Inhalt: Binaries + Sidecars + Test-Logs (LED-68b) + `complete-heuristik.log`.
A9 schreibt hier NICHTS; die Verbindung beider Realms ist rein referenziell: das INFO-Blatt jeder
xlsx zitiert `binary_id` + SHA512-Fingerprint (post-M3-Form, Abschnitt 4.2 Punkt 7) der vermessenen
Binaries (Replay-Schluessel-Schichtung LED-62). Gemeinsam ist AUSSCHLIESSLICH die
Grammatik-Single-Source (`axis_path_serialization.hpp` fuer Organ-Segmente,
`lager_pfad_grammatik.hpp` fuer kv/Joiner).

**Owner-E2-Kontext:** "Die Realm Trennung ist damit derzeit defekt" (Stempel-Regression) — die
Reparatur gehoert A13, nicht A9. A9 setzt auf der Datei-Ebene die konstruktiv saubere Trennung um
und referenziert Stempel nur LESEND ueber die A13-API.

**Hybrid-Auflage K1 (Owner-E1 + Bauplan `:29`):** solange die Lager-Identitaet der Hybrid-.so offen
ist (K1 = offener B-Phasen-Entscheid; Hybrid-DESIGN ist fertig, BAU = Auswertungsphase, ABGLEICH
`:28`), nimmt KEIN Lager-Schema Hybrid-Knoten an — die Pfad-Grammatik reserviert bewusst KEIN
Hybrid-Segment und wird bei K1-Entscheid additiv erweitert.

## 7. Scheiben-Plan + Gates

Einordnung: **Pflicht vor Trigger, Parallel-Spur GATE 9** (Abschnitt 0; Umstufung nur via V-A9-5).
Prozess nach Arbeitsweise v2.1 + Owner-Ruege 03.08. (bindend): **jede Scheibe laeuft ueber das
Workflow-Tool** (kein Einzel-Agent-Kanal), Modell je Phase explizit; je Scheibe ultracode VOR+NACH;
Kadenz: Mojibake-0, cf22 --Werror==0, Doppellauf ctest mit N (Basis am 0d7a0d92: 331 lokal —
N immer mit Namens-Diff belegen, nie nackt), gitleaks, beide Remotes, CI hart gruen,
super-Sub-Build Pflicht, ASCII-Diff-Selbstcheck.

| Scheibe | Inhalt | Worker (v2.1) | Gates (literal) |
|---|---|---|---|
| A9-S1 Vendor | `ext/io/zlib/` + `ext/io/libxlsxwriter/` Snapshots (Tag live pinnen), Provenance-Docs (inkl. gitignore-Entfernung), CMake-Targets consumer-only, `USE_STANDARD_TMPFILE`+`USE_DTOA_LIBRARY`, tmpfileplus geprunt, `LICENSE_AUDIT_EXT.md`+`NOTICE` | Opus 4.8 (leicht/Infra-artig) | Vendor-Target baut lokal standalone+super-Sub-Build; ctest N unveraendert (kein Default-Link); golden byte-neutral; CI beide Hosts gruen; gitleaks 0 |
| A9-S2 Grammatik | `lager_pfad_grammatik.hpp` + `ergebnis_dateiname.hpp` (Single-Source, Wachen 200-Byte/31-Zeichen/Zeichenklasse), golden-string Unit-Tests | Opus 5 (Struktur nach Design) | Golden-String-Tests gruen (N mitfuehren); Abnahme-Review durch A1-LB-Schnitt-Strang (gemeinsame Single-Source, kein Fork) — Ownership-Entscheid VORHER (Abschnitt 9 Punkt 2) |
| A9-S3 Writer-Kern | `builder/lager_ablage/`: Interfaces (header-only), `XlsxErgebnisMappe` (TU, PRIVATE-Link), `CsvErgebnisMappe`, Factory, Fehlerklassen-Anschluss (#29-IDs mit A15 abgestimmt), INFO-Blatt (Stempel-Zitat-Teil gegen A13-Lese-API, scharf erst nach M3) | Codex (schwere/neue Logik); **prod1-bwrap defekt => Opus-5-Fallback mit Fable-Adversarial-Review** (Owner-Nachtrag ~21:1x) + Dual-Review | Unit-Tests: ZIP-Struktur-Selbstpruefung per eigenem PK-Signatur-Scanner im Test (kein Python, kein externes Tool); Locale-Determinismus-Test (LC_NUMERIC=de_DE => byte-identisch); Zeilen-/Namenslimit-Negativproben literal; Token-Treue-Test (failed/gesperrt/nicht_gebaut woertlich) |
| A9-S4 CLI + Dual-Weg | `tools/mess_report/` (`comdare_mess_report render|plan|version`, header-getriebener CSV-Leser, CoR-Filter fuer dynamische Variablen, --realm-root/--format/--dry-run; `.stale` explizit ausgeschlossen), MANUAL_RUN.md-Abschnitt, CI-Job rules-gated inert (CI-Gruen != Auswertung) | Codex-Klasse (Fallback wie S3) | LED-61-Dual-Weg-Beleg: einmal CI-inert-Struktur + einmal lokal ueber offizielles CMake-Target mit literalem Baum-ls; Plan-dump deterministisch (2 Laeufe diff==0) |
| A9-S5 Erstbeleg | Render des Archiv-Bestands super `measurement/20260726-164259-d03-strukt-r-erstbeleg/measurements.csv` (read-only Quelle, am Ist verifiziert vorhanden) in einen lokalen Ziel-Baum; Beleg = literaler Baum + Sheet-Katalog | Manager-Abnahme | Quelle unangetastet (Alt-Mess-CSV=Archiv; Messdaten nie loeschen — nur ADDITIV neue Artefakte); Owner-NUR-KENNTNIS-Meldung mit Beleg-Ausgabe |

Sequenz/Abhaengigkeiten:
- **S1 sofort startbar** (keinerlei Fremd-Abhaengigkeit). **S2 nach Ownership-Entscheid** mit A1
  (Manager). **S3 nach S1+S2**; der INFO-Blatt-Fingerprint-Teil zusaetzlich **nach A13-M3**
  (M3 ist startklar, Gate: 2d-Landung + Q-M3-GATE; ABGLEICH `:24`) — S3 kann vorher alles ausser
  dem Stempel-Zitat-Teil bauen (Naht als eigene kleine Schnittstelle geschnitten). **S4/S5 nach S3.**
- NAS-Zielpfad-Inbetriebnahme haengt an **A12** (NAS-Creds E-14); bis dahin lokaler Wurzel-Pfad per
  Konfiguration — Umschaltung ist reine Konfiguration, kein Umbau.
- Beruehrpunkte NULL mit: `lazy_csv_header` (Schema-Freeze), Stempel-SCHREIB-Strecke (A13),
  golden-Fixtures, Pruef-Dock, Mess-Zeit-I/O (`mess_pfad_synchron_push`).

## 8. Konformitaets-Selbstpruefung gegen die Doktrinen (Stand 03.08.)

- Vendoring-Doktrin Stufe 1 eingehalten (echter Upstream-Code, liburing-Blaupause inkl.
  gitignore-Lehre) — kein faithful/self-contained noetig; kein neues Repo/Remote/Submodule.
- Kein Python in der Buildchain (Vendor-Bau = reines C; ZIP-Struktur-Test = eigener C++-Scanner).
- C++/Drogon-only: C-Vendor wie liburing zulaessig; Wrapper/CLI C++23.
- xlsx=Default, CSV=Fallback derselben Factory; Unter-Achsen im Dateinamen; Haupt-Achsen als
  Ordner+Metadaten; konstante Variablen weggelassen (Meta) — KERN 26.07. §4/§6 woertlich umgesetzt.
- Kein std::variant (Ausnahme-Scope Hybrid-Tier-Stufe betrifft A9 nicht, Owner-E1); statischer
  Dispatch wo CT (Grammatik/Serialisierung), RT-Factory nur am Unter-Achsen-Draht (Haupt=CT/Unter=RT).
- golden-/CI-Neutralitaet: rein additiv, No-Op ohne Consumer, Mess-Pfad byte-unberuehrt
  (2d-Haertung `mess_pfad_synchron_push` bleibt unangetastet).
- Messdaten nie loeschen: Writer schreibt atomar-additiv (tmp+rename), ueberschreibt nie,
  invalidiert nie; `.stale`-Bestaende werden weder gelesen noch angefasst.
- Fehlerklassen-Pflicht: Anschluss ans reale #29-Framework (FK-0/1/2 gebaut) statt Insel-Loesung.
- Stempel-Kanon Owner-E2/Q1/Q2/Q3/Q10: nur LESEND via A13-API; keine merge-Zeile, Klammer-Form,
  gerenderte `@X.Y.Zc`-Versionen, hierarchische Algorithmus-Namen 1:1.
- E-21/V7.6: Rueckfragen unten VERBOSE (ID+Kontext+Optionen+Empfehlung+Default).

## 9. Offene Punkte (aktualisiert; Erstfassungs-Stand -> Neufassungs-Stand)

1. **Tag-Pin libxlsxwriter v1.2.4** beim Vendoring LIVE verifizieren (Tag-Objekt+Commit-Hash ins
   Provenance-Doc; NIE aus diesem Dossier abschreiben; nie-raten-Doktrin). — unveraendert offen.
2. **Manager-Entscheid Ownership `lager_pfad_grammatik.hpp`** (A9-S2 vs. A1-LB-Schnitt-Baum-Writer
   GATE 6): kein Fork, ein Schreiber je Knoten (gilt auch fuer A9-S5). Am 0d7a0d92 existiert die
   Datei in KEINEM der beiden Straenge (verifiziert) — der Entscheid ist noch frei und muss VOR
   S2-Start fallen. — offen, VOR S2.
3. **FK-IDs `ErgebnisSchreibFehler`** mit A15/#29 abstimmen VOR A9-S3 — AKTUALISIERT: #29 ist
   real im Bau (FK-0/1/2 FERTIG, Heimat `axis_error.hpp`); Abstimmung ist jetzt konkret moeglich
   (Domaenen-Disjunktheit nach INC-29.2-Muster). — offen, VOR S3.
4. **NAS-Wurzelpfad** haengt an A12 (NAS-Creds backup1/backup2, E-14; ABGLEICH `:23` "Infra-nah");
   bis dahin lokaler konfigurierbarer Wurzel-Pfad. — offen, blockiert NUR die NAS-Inbetriebnahme.
5. **Hybrid-K1** offen (B-Phasen-Entscheid; Hybrid-Design FERTIG, Bau Auswertungsphase): kein
   Lager-Schema nimmt Hybrid-Knoten an; Pfad-Grammatik reserviert bewusst KEIN Hybrid-Segment,
   additive Erweiterung nach K1. — unveraendert offen, blockiert A9 nicht.
6. **Sheet-Benennung** S001..Snnn + Legende (Default) vs. Klartext = Owner-Geschmacksfrage —
   als V-A9-6 im Vorlagen-Block (Abschnitt 10). — offen.
7. **`result_aggregator.hpp:92`** (`result_csv_header()`, RFC-4180-Alt-Kopf; am Ist verifiziert) =
   ZWEITER CSV-Kopf neben `lazy_csv_header` — nicht Teil von A9; LED-75-Kandidatenliste
   (pruefen, ob noch Leser existieren). — offen, fortschreiben.
8. **Plattform-/Tmp-Strategie** — AKTUALISIERT (Owner-E4: RISC-V + macOS M1 + macOS x86 kommen
   JETZT): `USE_STANDARD_TMPFILE` (POSIX `tmpfile()`) traegt Linux (x86/ARM/RISC-V) UND macOS;
   Windows ist NICHT in der Flotte — erst bei einer etwaigen Windows-Erweiterung braucht es eine
   eigene Tmp-Strategie. Bei E-19-Fleet-Nachtrag (Stufe 2) gegenpruefen. — entschaerft, Merkposten.
9. **Doku-Nachzug nach Bau**: MANUAL_RUN.md-Abschnitt (S4), INDEX-/Architektur-Doc fuer
   `builder/lager_ablage/`, Ledger-Nachtrag im gebuendelten A11-Paket (nicht solo; A11-Nachtrag ist
   laut ABGLEICH `:22` ohnehin gewachsen). — offen, nach Bau.

## 10. Entscheidungs-Vorlagen (Task #22; Stand: ALLE unversendet — Prozess-Hinweis unten)

Prozess-Hinweis: Das Owner-GO ~17:4x ("volles Go fuer ALLE geplante offene Arbeit wie vorgegeben")
deckt DOKUMENTIERTE Empfehlungs-Defaults. Die V-A9-Vorlagen waren zum GO-Zeitpunkt UNVERSENDET
(ZWISCHENSTAND `:140`) — der Versand bleibt Pflicht (E-21 VERBOSE; NUR-KENNTNIS-Vorlagen duerfen
mit Default laufen, RUECKFRAGEN brauchen die Vorlage vor dem betroffenen Scheiben-Start).

- **V-A9-1 [NUR ZUR KENNTNIS, Default: ausfuehren] Vendoring-GO libxlsxwriter:** F3 (01.08.)
  erlaubt "eigener Writer, Fork/Orientierung libxlsxwriter". Beleg: liburing-Praezedenz
  `ext/io/liburing/COMDARE-VENDOR-PROVENANCE.md` (Stufe-1-Snapshot ohne .git, kein Submodule).
  Geprueft: Snapshot im BESTEHENDEN Repo — KEIN neues Repo/Remote/Submodule, Remote-GO-Regel greift
  nicht. Vorhaben: Snapshot Tag v1.2.4 (Tag+Commit beim Vendoring live verifizieren) nach
  `ext/io/libxlsxwriter/` inkl. LICENSE_AUDIT_EXT.md- und NOTICE-Nachtrag. Umkehrbarkeit: voll.
  Optionen: (a) ausfuehren [Empfehlung+Default, F3-gedeckt], (b) self-contained Minimal-Writer
  (verstiesse gegen die geordnete Vendoring-Doktrin).
- **V-A9-2 [RUECKFRAGE, Default: Option A] zlib-Strategie:** libxlsxwriter braucht zlib; grep ueber
  ce CMakeLists.txt + cmake/ = 0 Treffer (neu verifiziert am 0d7a0d92), zlib ist heute KEINE
  ce-Abhaengigkeit. Optionen: (A) zlib mit-vendorn als `ext/io/zlib/` (zlib-Lizenz, gleiches
  Snapshot-Muster; self-contained ueber die 8er-Docker-Matrix) [Empfehlung+Default — sauberster,
  nicht einfachster Weg]; (B) System-zlib via find_package(ZLIB) (weniger Repo-Gewicht, aber alle
  8 Docker-Images + beide Hosts + neue Plattformen (RISC-V/macOS!) brauchen zlib-dev — neue
  Infra-Annahme + Infra-Handout). Umkehrbarkeit: hoch (CMake-Option COMDARE_SYSTEM_ZLIB).
- **V-A9-3 [NUR ZUR KENNTNIS, Default: so bauen] Third-party-Lizenzhygiene im Snapshot:**
  tmpfileplus (MPL-2.0) wird per USE_STANDARD_TMPFILE NICHT kompiliert und GEPRUNT; dtoa (MIT) per
  USE_DTOA_LIBRARY AKTIVIERT (byte-deterministische double-Ausgabe); Openwall-MD5 (PD) bleibt;
  minizip (zlib-Lizenz) ist der ZIP-Packer. Deckung: Lizenz-Endstand 02.08. ("vendored ext/ behaelt
  eigene Lizenzen"). Umkehrbarkeit: voll (Build-Flags + Prune-Protokoll).
- **V-A9-4 [RUECKFRAGE zur Lesart-Bestaetigung, Default: so bauen] Naht-Aufteilung CSV/xlsx:**
  KERN 26.07. §6 nennt die CSV-Doktrin woertlich "Vorstufe" und setzt "xlsx = kuenftig DEFAULT,
  CSV einstellbar + Fallback" (Factory). Design-Lesart: per-Binary result.csv (+Resume-Stamp) und
  offizielle e4_xml/measurements.csv bleiben das MASCHINEN-/Resume-Format der Mess-Zeit
  (Mess-I/O-Doktrin, durch 2d-Haertung bekraeftigt); xlsx ist der DEFAULT der
  Lager-ABLAGE/AUSWERTUNG dahinter. Optionen: (a) Lesart bestaetigen [Empfehlung+Default];
  (b) xlsx zusaetzlich an der Mess-Zeit-Naht erzeugen (mehr I/O im Messfenster, widerspricht der
  Contention-Doktrin). Umkehrbarkeit: hoch (Factory-Konfiguration, kein Formatbruch).
- **V-A9-5 [NEU — ESKALATION Gate-Einordnung, Default: Option A] A9-Einstufung Trigger vs.
  Auswertung:** Kontext: Bauplan `:43` sagt woertlich "Pflicht vor Trigger ... A9 xlsx" und die
  Gate-Kette fuehrt GATE 9 vor GATE 10/11; ZUGLEICH terminiert Bauplan `:40` den
  "xlsx-Writer-Start" auf Do 07. = Trigger-Tag (bauplan-interner Widerspruch), und technisch
  beruehrt A9 keine Binary-Identitaet (kein Stempel/Fingerprint/golden/ABI) — zwingend gebraucht
  wird xlsx erst zur Auswertung (E.4). Die Erstfassung hatte STILL umgestuft; das wird hiermit als
  BENANNTE Abweichung vorgelegt. Optionen: (A) A9 bleibt formal Pflicht VOR Trigger
  (Kategorie-A-Wortlaut; S1/S2 sofort in Parallel-Spuren, S3-S5 sobald A13-M3/A12 es zulassen; der
  Trigger wartet NICHT auf A9-S4/S5, wohl aber die Auswertung) [Empfehlung+Default — kein stiller
  Umbau, Bauplan-Wortlaut respektiert, Widerspruch :40/:43 zugunsten :43 aufgeloest];
  (B) formale Umstufung auf "Pflicht vor der Auswertung (E.4)" mit Korrektur-Vermerk in
  KATALOG+Bauplan (technisch sauber, aber Aenderung der dokumentierten Gate-Kette = Owner-/
  Manager-Hoheit). Umkehrbarkeit: voll (reine Einordnung, kein Code).
- **V-A9-6 [RUECKFRAGE Geschmacksfrage, Default: Option A] Sheet-Benennung:** xlsx-Limit: Sheet-Name
  <=31 Zeichen, ohne `[]:*?/\`. Optionen: (A) deterministische Kurzcodes `S001..Snnn` +
  Klartext-Legende im INFO-Blatt [Empfehlung+Default — kollisionsfrei, deterministisch, limitfest];
  (B) Klartext-Sheetnamen mit verlustbehafteter Kuerzungsregel (lesbarer im Tab, aber
  Kuerzungs-Kollisionen moeglich => zusaetzliche Wache noetig). Umkehrbarkeit: hoch
  (Render-Detail, Factory-Option).

## Quellen (gelesen/verifiziert 03.08.2026)

1. Erstfassungs-Inhalt: super `docs/sessions/backups/20260802-welle3-parallel-workflows/00-welle3-result.json`
   (result.designs.designs[2], key `A9-xlsx-writer-design-dossier-20260802`).
2. super `docs/sessions/20260802-KATALOG-offene-strecke-gesamtplan.md:13` (Kategorie A), `:25` (A9),
   `:68` (E.4 Auswertung).
3. super `docs/sessions/20260726-SESSION-achsen-neuordnung-lager-baeume-xlsx-regressionen.md:54-63`
   (§4 Lager-Baeume), `:65-79` (§5 Organ-Gruppen), `:89-98` (§6 xlsx-Doktrin).
4. super `docs/sessions/20260802-BAUPLAN-owner-entscheide-synthese-kritischer-pfad.md:40` (Do 07
   xlsx-Start), `:43` (Pflicht vor Trigger), `:29` (Hybrid-K1), `:47-55` (Q1-Q10-Stand), `:59`
   (Zuweisungs-Matrix v2.1).
5. super `docs/sessions/backups/20260802-kontext-neugruendung-abend-workflows/00-SYNTHESE-lage-dossier.md:236`
   (GATE 9), `:238/:240` (GATE 10/11), `:55-56` (Lager/Storage-Doktrin), `:72` (Lizenz-Endstand),
   `:259` (Widerspruch 10).
6. super `docs/sessions/20260802-OWNER-entscheide-hybrid-tier-stempel-regression-os-unterachsen.md`
   (E1-E6/R7-R9 verbatim + Nachtraege Q1/Q2 ~12:1x, Q3 ~17:0x, GO ~17:4x/~18:4x, Q10 ~19:5x,
   Codex-GO ~19:3x, 2d-Fortsetzung ~21:1x, M3-Reihenfolge ~21:2x, Resume 03.08., Owner-Ruege
   Arbeitsweise 03.08.).
7. ce `docs/sessions/20260803-ABGLEICH-katalog-gesamtplan-ist-stand.md:20` (A9 OFFEN), `:24` (A13
   M1+M1b+M2 fertig, M3 startklar), `:25` (A14 U1-U3 fertig), `:26` (A15 FK-0/1/2 fertig), `:28`
   (A17 Design fertig), `:50-52` (kritischer Pfad).
8. super `docs/sessions/20260802-ZWISCHENSTAND-vor-kompaktierung-welle3-codex-integritaet.md:15`
   (Task #22 Design-Nachbesserung), `:140` (V-A9-Vorlagen unversendet).
9. ce-Ist 0d7a0d92 (alle Anker in Abschnitt 1/2 einzeln live verifiziert; u.a.
   `cache_engine_builder_iterator.hpp:359/:462/:827-834/:908/:1540-1650/:1835/:1837-1841`,
   `axis_error.hpp:410-421/:420`, `profile_run_entry.hpp:107/:541/:880-882`,
   `experiment_run_entry.hpp:406`, `experiment_driver.cpp:515`, `result_aggregator.hpp:92/:112`,
   `measurement_curve_loader.hpp:3-6`, `best_binary_selector_main.cpp:22`, `result_ingest.hpp:33`,
   `axis_path_serialization.hpp:40-50`, `artifact_cache.hpp:179-181`,
   `ext/io/liburing/COMDARE-VENDOR-PROVENANCE.md` inkl. Nachtrag 23.07., `LICENSE_AUDIT_EXT.md`,
   `NOTICE`, `docs/architecture/20260802-hybrid_tier_stufe_soll_design.md` als Zielort-Praezedenz).
10. super `measurement/20260726-164259-d03-strukt-r-erstbeleg/` (Archiv-Erstbeleg, vorhanden).
11. Memory-Doktrinen: `project_lager_baeume_xlsx_doktrin.md`,
    `feedback_vendoring_doctrine_ordered_vendor_faithful_selfcontained.md`,
    `feedback_no_python_in_buildchain.md`, `feedback_hybrid_tier_stufe_hinter_ceb_variant_ausnahme.md`,
    `feedback_versions_flag_grammatik_dreistellig_hw_flag.md`,
    `feedback_stempel_regression_keine_merge_zeile_meta_meta_realm_ende.md`,
    `feedback_todo_stack_lifo_verbose_owner_fragen_vollpersistenz.md`,
    `feedback_claude_code_arbeitsweise_v2_konsolidiert.md`.
12. Online-Verifikation 02.08. (Erstfassung): github.com/jmcnamara/libxlsxwriter (Releases) +
    libxlsxwriter.github.io/license.html — beim Vendoring LIVE zu wiederholen.

## Zielort im Repo (Empfehlung)

`ce docs/architecture/20260803-a9_xlsx_writer_f3_soll_design.md` — Praezedenz: das A17-Design
liegt als `docs/architecture/20260802-hybrid_tier_stufe_soll_design.md` im ce (Code-Design wohnt
beim Code); super `docs/sessions/` erhaelt nur den ueblichen Session-/Ledger-Verweis. Commit als
Teil der Task-#22-Design-Nachtrags-Landung (Lead), zusammen mit dem Versand der Vorlagen
V-A9-1..V-A9-6.
