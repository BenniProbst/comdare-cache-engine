# Folge-Mail an Prof. Habich — Klarstellung mit wissenschaftlichen Vollangaben

**Stand:** 2026-05-08 nach Treffen
**Anlass:** Habich hat die erste Anfrage nicht zuordnen koennen (P-IDs + Kurzformen unklar fuer Aussenstehende, selbst Co-Autoren).
**Loesung:** Folge-Mail mit voller Bibliografie pro Werk.

---

## Vorgeschlagener Email-Text

```
Betreff: Folge-Anfrage Originalcode — Klarstellung mit Werkangaben (Diplomarbeit PRT-ART)

Sehr geehrter Herr Prof. Habich,

vielen Dank fuer das heutige Treffen. Mir ist klar geworden, dass meine
vorherige Anfrage zu drei Werken nicht ausreichend praezise benannt war.
Hiermit die Klarstellung mit vollen wissenschaftlichen Angaben:


WERK 1
   Titel:    "Overview on Hardware Optimizations for Database Engines"
   Autoren:  Annett Ungethuem, Dirk Habich, Tomas Karnagel, Sebastian Haas,
             Eric Mier, Gerhard Fettweis, Wolfgang Lehner
   Venue:    BTW 2017 (Datenbanksysteme fuer Business, Technologie und Web),
             Lecture Notes in Informatics, Gesellschaft fuer Informatik, Bonn

WERK 2
   Titel:    "To stride or not to stride the memory access?"
   Autoren:  Lennart Schmidt, Roland Kuehn, Matti Krause, Jens Teubner,
             Wolfgang Lehner, Dirk Habich
   Venue:    DIMES 2025, 3rd Workshop on Disruptive Memory Systems, Seoul
   DOI:      10.1145/3764862.3768174

WERK 3
   Titel:    "VAMPIR — Virtualized Non-Functional Memory Properties for
              Data-Pipeline Scheduling"
   Autoren:  Andre Berthold, Lennart Schmidt, Dirk Habich, Wolfgang Lehner,
             Horst Schirmeier
   Venue:    DFG SPP 2377 "Disruptive Memory Technologies",
             Phase-1-KICKOFF Poster, 2023


ANFRAGE
Existiert fuer eines dieser drei Werke ein Repository (auch privat / TUD-
intern), das ich gemaess Ihrer Direktive zur exakt-kopierten Originalcode-
Anbindung via Adapter-Pattern fuer die Diplomarbeit nutzen darf? Falls ja:
welche Compiler-Version wurde fuer die jeweiligen Originalmessungen
verwendet?

Die drei Werke gehoeren zur direkten TUD-Forschungslinie meiner
Diplomarbeit (PRT-ART Cache-Engine), insbesondere:
   • Werk 1 als methodische Quelle fuer Hardware/Software-Co-Design
   • Werk 2 als empirische Stuetze fuer plattform-bewusste Access-Pattern-
     Wahl (Sapphire Rapids HBM)
   • Werk 3 als NFP-Sprache fuer die Cache-Engine-Optimierungsziele

Mit freundlichen Gruessen,
Benjamin-Elias Probst
s2631336@tu-dresden.de
```

---

## Lessons Learned (aus Habich-Treffen)

**Regel fuer alle kuenftigen Outbound-Mails:**
- KEINE P-IDs (P01-P33) im Mail-Text
- KEINE Kurzformen wie "Ungethuem 2017", "Schmidt 2025", "VAMPIR 2023"
- IMMER Werk-Block mit:
  1. Titel (in Anfuehrungszeichen)
  2. Alle Autoren (Vor + Nachname)
  3. Venue + Jahr (mit Konferenz/Journal-Vollnamen)
  4. DOI (wo verfuegbar)

**Begruendung:** Selbst Co-Autoren haben Dutzende Veroeffentlichungen pro Jahr und koennen ohne Vollangaben nicht zuordnen, welches Werk gemeint ist. Externe Empfaenger fuehlen sich angesprochen wie an einer internen Wiki, die sie nicht kennen.

**Diese Regel ist im auto-memory** unter `feedback_outbound_scientific_references.md` festgehalten und gilt fuer alle Folge-Anfragen.
