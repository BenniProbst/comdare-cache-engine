# EMAIL-KONTAKTE — Anfragen fuer fehlende Code-Quellen

**Phase 4.B Vorbereitung** · TU Dresden Diplomarbeit PRT-ART
**Stand:** 2026-05-08 (REV 2 nach Habich-Treffen — Vollangaben-Regel)
**Ziel:** Originalcode der Paper besorgen + Lizenz-Klaerungen.

## ⚠️ REGEL FUER ALLE OUTBOUND-MAILS (Lessons Learned 2026-05-08)

Habich konnte die Anfrage zu drei Werken, an denen er Co-Autor war, NICHT
zuordnen, weil die Mail nur P-IDs ("P31 Ungethuem 2017") enthielt. Selbst
Co-Autoren haben Dutzende Veroeffentlichungen pro Jahr.

**Regel:** Pro Werk im Mail-Body einen klaren Werk-Block am Anfang:

```
WERK
   Titel:    "..."
   Autoren:  Vorname1 Nachname1, Vorname2 Nachname2, ...
   Venue:    Konferenz/Journal Vollname + Jahr (+ Ort/DOI)
```

NIEMALS verwenden in Outbound-Mails:
- Interne P-IDs (P01-P33)
- Kurzformen wie "Ungethuem 2017" / "Schmidt 2025" / "VAMPIR 2023"

Memory-Anker: `feedback_outbound_scientific_references.md`

## Klassifikation

| Klasse | Anzahl | Strategie |
|--------|--------|-----------|
| 🟢 PRIORITAET HOCH (TUD/Habich-direkt) | 3 | Sofort via Habich klaeren |
| 🟡 PRIORITAET MITTEL (TUM/HPI/Dortmund) | 2 | Email mit Habich abstimmen |
| 🟠 PRIORITAET MITTEL-NIEDRIG (Junge Paper, Autoren erreichbar) | 4 | Email an Autoren |
| 🔵 PRIORITAET NIEDRIG (Alte Paper, Re-Impl praktischer) | 11 | Re-Implementation Default |
| 🟣 NEU: LIZENZ-KLAERUNG (geklont, aber GPL-3 oder unklar) | 4 | Re-Lizenzierung anfragen |

## ⚠️ SPRACH-REGEL FUER OUTBOUND-MAILS (Lessons Learned 2026-05-08 20:00)

Heuristik fuer die Mail-Sprache:
- **DACH-Raum** (TU Dresden / TUM / TU Dortmund / HPI Potsdam etc.) → **Deutsch**
- **Alle anderen** (USA / UK / China / international) → **Englisch**
- **Co-Autoren mit anglo-amerikanischer Affiliation in CC** → **Englisch zwingend**

Memory-Anker: `feedback_outbound_scientific_references.md` (Update 2026-05-08 20:00)

Pro Empfaenger eine eigenstaendige Mail-Vorlage (nicht nur deutsche Vorlage uebersetzen).

## Status der Email-Anfragen (Update 2026-05-08)

**Hinweis:** Der Architekt hat klargestellt, dass die heutigen Email-VORLAGEN
(P26, P27, plus die Lizenz-Klaerungs-Anfragen P04/P07/P25) NICHT verschickt
wurden. Diese Tabelle differenziert "VERSCHICKT" vs "VORBEREITUNG".

Zusaetzliche Architekt-Direktive (2026-05-08): GPL-3.0 / unklare Lizenzen sind
**kein Hinderungsgrund**, da modularisierte Bruchstuecke + C++23-Metaprogrammierung
technisch neuen Code erzeugen. Re-Lizenzierungs-Anfragen sind daher NICHT
Pflicht, sondern nur optionale Hoeflichkeit.

| Anfrage | Status |
|---------|--------|
| 🟢 Habich (P31/P32/P33 direkt) | ✅ VERSCHICKT |
| 🟡 Schmeisser/Schuele (P06 B²-tree Code-Anfrage) | ✅ VERSCHICKT, Code erhalten 2026-05-08 (b2-tree-master + bart-master) |
| 🟡 Roland Kuehn (P28 DAMON Code-Anfrage) | ✅ VERSCHICKT — **POSITIVE ANTWORT 2026-05-08:** Code-Zusage nach Aufraeumen + neue Variante (LeafOnly+Sampling) + ART-Reordering-Hintergrund (siehe `email/20260508-1900-mailverlauf_kuehn_p28_eingang.md`). Probst hat um Download-Link gebeten. |
| 🟠 Qian Zhang (P26 FGCS 2024) | 📝 EN-VORLAGE ERSTELLT (`email/20260508-2000-email_p26_qzhang_en.md`) — nicht verschickt |
| 🟠 Tingji Zhang (P27 ASPLOS 2025) | 📝 EN-VORLAGE ERSTELLT (`email/20260508-2000-email_p27_tzhang_en.md`) — nicht verschickt; Repo gefunden (BSD-3, gem5-Fork) |
| 🟣 P04 Boffa (CoCo-Trie Re-Lizenzierung) | ❌ NICHT MEHR NOETIG (Architekt-Direktive: GPL ignorieren) |
| 🟣 P07 Wu/Ni/Jiang (Wormhole Re-Lizenzierung) | ❌ NICHT MEHR NOETIG (Architekt-Direktive: GPL ignorieren) |
| 🟣 P25 Mahling/Rabl (Lizenz-Festlegung) | ❌ NICHT MEHR NOETIG (Architekt-Direktive: ohne Lizenz = freie akademische Nutzung) |
| 🟣 P30 Huang Jiahua (haz_ptr Lizenz) | ❌ NICHT NOETIG — Hazard Pointers raus aus PRT-ART (F12-K) |

---

## 🟢 PRIORITAET HOCH — TUD-INTERN, DIREKT VIA HABICH

### P31 — Ungethuem 2017 (TUD Hardware Optimizations Survey)

| Feld | Wert |
|------|------|
| **Paper** | Overview on Hardware Optimizations for Database Engines (BTW 2017) |
| **Autoren** | Annett Ungethuem, **Dirk Habich**, Tomas Karnagel, Sebastian Haas, Eric Mier, Gerhard Fettweis, **Wolfgang Lehner** |
| **Status** | Code existiert vermutlich (Tomahawk-DBA-Implementierung) |
| **Empfohlener Kontakt** | **Prof. Dirk Habich** (DIREKT) |
| **Email Habich** | dirk.habich@tu-dresden.de |
| **Strategie** | Persoenlich in der naechsten Sprechstunde ansprechen — er ist Hauptbetreuer und Co-Autor |

**Vorgeschlagener Email-Text (REV 2 mit Vollangaben — siehe auch
`docs/email/20260508-1700-email_habich_folge_klarstellung.md`):**

> Betreff: Originalcode-Anfrage fuer drei TUD-Werke — Diplomarbeit PRT-ART
>
> Sehr geehrter Herr Prof. Habich,
>
> im Rahmen meiner Diplomarbeit "Active Cache-Aware Hardware Adaptation Cache
> Engine for Trie-Based Index Structures" moechte ich Originalcode-Bausteine
> aus den folgenden drei von Ihnen co-autorisierten Werken integrieren —
> gemaess Ihrer Direktive zur exakt-kopierten Originalcode-Anbindung via
> Adapter-Pattern:
>
> WERK 1
>    Titel:    "Overview on Hardware Optimizations for Database Engines"
>    Autoren:  Annett Ungethuem, Dirk Habich, Tomas Karnagel, Sebastian Haas,
>              Eric Mier, Gerhard Fettweis, Wolfgang Lehner
>    Venue:    BTW 2017 (Datenbanksysteme fuer Business, Technologie und Web),
>              Lecture Notes in Informatics, Gesellschaft fuer Informatik, Bonn
>
> WERK 2
>    Titel:    "To stride or not to stride the memory access?"
>    Autoren:  Lennart Schmidt, Roland Kuehn, Matti Krause, Jens Teubner,
>              Wolfgang Lehner, Dirk Habich
>    Venue:    DIMES 2025, 3rd Workshop on Disruptive Memory Systems, Seoul
>    DOI:      10.1145/3764862.3768174
>
> WERK 3
>    Titel:    "VAMPIR — Virtualized Non-Functional Memory Properties for
>               Data-Pipeline Scheduling"
>    Autoren:  Andre Berthold, Lennart Schmidt, Dirk Habich, Wolfgang Lehner,
>              Horst Schirmeier
>    Venue:    DFG SPP 2377 "Disruptive Memory Technologies",
>              Phase-1-KICKOFF Poster, 2023
>
> Frage: Existiert fuer eines dieser drei Werke ein Repository (auch privat /
> TUD-intern), das ich fuer die Adapter-Konstruktion nutzen darf? Falls ja,
> welche Compiler-Version wurde fuer die Originalmessungen verwendet?
>
> Mit freundlichen Gruessen,
> Benjamin-Elias Probst (s2631336@tu-dresden.de)

---

### P32 — Schmidt 2025 (TUD To Stride or Not to Stride, DIMES 2025)

| Feld | Wert |
|------|------|
| **Paper** | To stride or not to stride the memory access? (DIMES 2025) |
| **Autoren** | Lennart Schmidt, Roland Kuehn, Matti Krause, Jens Teubner, Wolfgang Lehner, **Dirk Habich** |
| **Status** | Mikrobenchmark-Code vermutlich verfuegbar (HPI-aehnlich publiziert) |
| **Empfohlener Kontakt** | **Prof. Dirk Habich** (DIREKT) zu Lennart Schmidt vermitteln lassen |
| **Email Schmidt (TUD)** | lennart.schmidt@tu-dresden.de (zu verifizieren) |
| **Email Habich** | dirk.habich@tu-dresden.de |
| **Strategie** | Habich konsultieren; ggf. direkt an Lennart Schmidt |

---

### P33 — VAMPIR Poster 2023 (TUD/SPP2377 Kickoff)

| Feld | Wert |
|------|------|
| **Paper** | VAMPIR — Virtualized Non-Functional Memory Properties for Data-Pipeline Scheduling (SPP2377 Kickoff) |
| **Autoren** | Andre Berthold, Lennart Schmidt, **Dirk Habich**, Wolfgang Lehner, Horst Schirmeier |
| **Status** | Phase-1-Kickoff Poster — vermutlich KEIN Code (noch nicht implementiert) |
| **Empfohlener Kontakt** | **Prof. Dirk Habich** (DIREKT) — falls aktuelle Phase-2-Implementation existiert |
| **Email Habich** | dirk.habich@tu-dresden.de |
| **Strategie** | Vermutlich keine Code-Anfrage noetig, sondern KONZEPTBEZUG (NFP-Modell als Inspiration fuer PRT-ART-Cache-Engine). Falls Phase-2-Code existiert: anfragen. |

---

## 🟡 PRIORITAET MITTEL — INSTITUTIONS-EXTERN

### P06 — B²-tree (Schmeisser 2022, TUM)

| Feld | Wert |
|------|------|
| **Paper** | B²-Tree: Page-Based String Indexing in Concurrent Environments (Datenbank-Spektrum 2022) |
| **Autoren** | Josef Schmeisser, Maximilian E. Schuele, Viktor Leis, Thomas Neumann, Alfons Kemper |
| **Status** | TUM-intern, vermutlich nicht oeffentlich publiziert |
| **Primaerkontakt** | **Maximilian E. Schuele** (Erstautor-aktiv, jetzt Prof. an FSU Jena) |
| **Email Schuele** | maximilian.schuele@uni-jena.de (Lehrstuhl Datenbanken FSU Jena) |
| **Sekundaerkontakt** | Josef Schmeisser (TUM) |
| **Email Schmeisser** | schmeisser@in.tum.de (zu verifizieren — Status unklar) |
| **Sekundaerkontakt 2** | Prof. Viktor Leis (TUM, jetzt LMU?) |
| **Email Leis** | leis@in.tum.de (oder leis@dbs.ifi.lmu.de) |
| **Strategie** | Email an Schuele (Erstautor-aktiv); CC an Habich |

**Vorgeschlagener Email-Text (REV 2 mit Vollangaben):**

> Betreff: Code-Anfrage zu B²-tree-Originalcode — Diplomarbeit TU Dresden
>
> Sehr geehrter Herr Prof. Schuele,
>
> ich arbeite an einer Diplomarbeit an der TU Dresden unter Prof. Dr. Dirk Habich
> mit dem Titel "Active Cache-Aware Hardware Adaptation Cache Engine for Trie-Based
> Index Structures" (Eigenname PRT-ART). Ein zentraler Bestandteil ist eine
> vollstaendige Cross-Algorithm-Permutations-Studie ueber bekannte Index-Bausteine.
>
> WERK
>    Titel:    "B²-Tree: Page-Based String Indexing in Concurrent Environments"
>    Autoren:  Josef Schmeisser, Maximilian E. Schuele, Viktor Leis,
>              Thomas Neumann, Alfons Kemper
>    Venue:    Datenbank-Spektrum 22, 11–22, Springer, Februar 2022
>    DOI:      10.1007/s13222-022-00409-y
>
> Ihr B²-Tree mit der Decision/Span-Page-Architektur ist als Concept-Anker fuer
> die Embedded-Tree-Page in unserem Bausteine-Korpus relevant. Die Habich-Direktive
> verlangt, dass Originalcode EXAKT KOPIERT integriert wird (Anbindung ueber
> Adapter-Pattern), um die Vergleichsmessungen authentisch zu halten.
>
> Frage: Ist der B²-tree-Originalcode aus dem 2022-Paper (idealerweise inkl. der
> Concurrent-Variante) fuer akademische Diplomarbeit-Zwecke verfuegbar?
>
> Apache-2.0 oder vergleichbare Lizenz waere ideal; wir behalten die Originallizenz
> pro `ext/<paper>/` bei. Ich kann gerne den Architektur-Kontext (Bausteine-Quer-
> Permutation, Habich-Direktive) im Detail schildern.
>
> Mit freundlichen Gruessen,
> Benjamin Probst (s2631336@tu-dresden.de)
> CC: Prof. Dr. Dirk Habich (dirk.habich@tu-dresden.de)

---

### P28 — Kuehn DAMON 2023 (TU Dortmund DBIS)

| Feld | Wert |
|------|------|
| **Paper** | Towards Data-Based Cache Optimization of B+-Trees (DaMoN 2023) |
| **Autoren** | Roland Kuehn, Daniel Biebert, Christian Hakert, Jian-Jia Chen, Jens Teubner |
| **Status** | TU Dortmund, vermutlich nicht oeffentlich |
| **Primaerkontakt** | **Roland Kuehn** (Erstautor) |
| **Email Kuehn** | roland.kuehn@tu-dortmund.de |
| **Sekundaerkontakt** | Prof. Jens Teubner (DBIS Lehrstuhl) |
| **Email Teubner** | jens.teubner@tu-dortmund.de |
| **Strategie** | Email an Kuehn; ggf. CC an Teubner |
| **Hinweis** | Kuehn ist auch Co-Autor von P32 (Schmidt 2025) — TUD-Verbindung! |

**Vorgeschlagener Email-Text (REV 2 mit Vollangaben):**

> Betreff: Code-Anfrage zu B+-Tree-Cache-Optimierung — Diplomarbeit TU Dresden
>
> Sehr geehrter Herr Kuehn,
>
> ich arbeite an einer Diplomarbeit an der TU Dresden unter Prof. Dr. Dirk Habich
> mit dem Titel "Active Cache-Aware Hardware Adaptation Cache Engine for Trie-
> Based Index Structures".
>
> WERK
>    Titel:    "Towards Data-Based Cache Optimization of B+-Trees"
>    Autoren:  Roland Kuehn, Daniel Biebert, Christian Hakert, Jian-Jia Chen,
>              Jens Teubner
>    Venue:    DaMoN 2023, 19th International Workshop on Data Management on
>              New Hardware, ACM, Seattle, WA, USA
>    DOI:      10.1145/3592980.3595316
>
> Eine der zentralen Forschungsluecken meiner Arbeit adressiert genau das, was
> Sie in dem oben genannten Werk als bewusst aussen vor gelassen genannt haben:
> Online Re-Layouting ueber Histogramm-basierte Hot-Path-Detection.
>
> Ihr Histogramm-pro-Knoten-Mechanismus (Sec.3) ist eine direkte Inspiration
> fuer die Cache-Engine meiner Arbeit. Daher waere mir der Originalcode (mit
> den Histogramm-Strukturen + B+-Tree-Integration) sehr wertvoll fuer die
> Adapter-Konstruktion und Vergleichsmessungen.
>
> Frage: Ist Ihr DaMoN-2023-Code fuer akademische Diplomarbeit-Zwecke verfuegbar?
> Ich behalte Ihre Originallizenz pro `ext/<paper>/` bei.
>
> Falls eine Code-Freigabe schwierig ist, wuerde mich auch ein Pseudocode-Auszug
> aus Ihren internen Notizen fuer eine sauberer Re-Implementation reichen.
>
> Mit freundlichen Gruessen,
> Benjamin Probst (s2631336@tu-dresden.de)
> CC: Prof. Dr. Dirk Habich (dirk.habich@tu-dresden.de)

---

## 🟠 PRIORITAET MITTEL-NIEDRIG — JUNGE PAPER, AUTOREN ERREICHBAR

### P26 — Zhang FGCS 2024 (Prefetching Indexing Scheme)

| Feld | Wert |
|------|------|
| **Paper** | A prefetching indexing scheme for in-memory database systems (FGCS 2024, vol. 156) |
| **DOI** | 10.1016/j.future.2024.02.018 (zu verifizieren) |
| **Autoren** | Qian Zhang (Erstautorin), Haoyun Song, Kaiyan Zhou, Jianhao Wei, Chuqiao Xiao |
| **Affiliation Erstautorin** | East China Normal University (ECNU), Software Engineering Institute, Shanghai |
| **Profil Qian Zhang** | B.Sc.+M.Sc. HeNan University (2012/2014), aktuell Ph.D.-Studentin am SEI ECNU |
| **Email Qian Zhang** | (zu verifizieren; uebliche ECNU-Format: 51184501NNN@stu.ecnu.edu.cn ODER qzhang@sei.ecnu.edu.cn) |
| **Empfohlener Kontakt** | **Qian Zhang** (Erstautorin) + ggf. **Chuqiao Xiao** (Senior-Co-Autor) |
| **Strategie** | Email an Qian Zhang; ECNU-spezifische Email-Format-Verifikation noetig |
| **Hinweis** | **Path-Prefetcher + Jump-Pointer-Prefetcher** direkt fuer PRT-ART relevant (Hot-Path-Erkennung) |
| **Repo-Status** | KEIN oeffentliches Repo (DBLP/ResearchGate/GitHub negativ) |

**ENGLISCHE Mail-Version vor Versand verwenden** (siehe `email/20260508-2000-email_p26_qzhang_en.md`).
Die deutsche Vorlage unten dient nur als Arbeits-Entwurf:

**Vorgeschlagener Email-Text P26 (REV 2 mit Vollangaben — DE-Entwurf, NICHT verschicken):**

> Betreff: Code-Anfrage zu Prefetching-Indexing-Scheme — Diplomarbeit TU Dresden
>
> Sehr geehrte Frau Zhang,
>
> ich arbeite an einer Diplomarbeit an der TU Dresden unter Prof. Dr. Dirk Habich
> mit dem Titel "Active Cache-Aware Hardware Adaptation Cache Engine for Trie-
> Based Index Structures".
>
> WERK
>    Titel:    "A prefetching indexing scheme for in-memory database systems"
>    Autoren:  Qian Zhang, Haoyun Song, Kaiyan Zhou, Jianhao Wei, Chuqiao Xiao
>    Venue:    Future Generation Computer Systems (FGCS), Volume 156,
>              Elsevier, 2024
>    DOI:      10.1016/j.future.2024.02.018
>
> Ihr Path-Prefetcher mit Jump-Pointer-Mechanik in dem oben genannten Werk ist
> ein zentraler Vergleichspunkt fuer eine unserer Hauptforschungsluecken
> (Hot-Path-basierte Cache-Adaption ueber Probability-Hints).
>
> Frage: Ist der Originalcode (Path-Prefetcher + Jump-Pointer-Prefetcher fuer
> versionierte Tupel) fuer akademische Diplomarbeit-Zwecke verfuegbar?
>
> Wir folgen einer Habich-Direktive der EXAKT-KOPIERTEN Originalcode-Anbindung
> via Adapter-Pattern, sodass Ihre Implementierung authentisch in einer
> Bausteine-Quer-Permutations-Studie verglichen werden kann.
>
> Mit freundlichen Gruessen,
> Benjamin Probst (s2631336@tu-dresden.de)
> CC: Prof. Dr. Dirk Habich (dirk.habich@tu-dresden.de)

---

### P27 — Zhang ASPLOS 2025 (Hierarchical Prefetching)

| Feld | Wert |
|------|------|
| **Paper** | Hierarchical Prefetching: A Software-Hardware Instruction Prefetcher for Server Applications (ASPLOS 2025, DOI 10.1145/3676641.3716260) |
| **Autoren** | Tingji Zhang, Boris Grot, Wenjian He, Yashuai Lv, Peng Qu, Fang Su, Wenxin Wang, Guowei Zhang, Xuefeng Zhang, Youhui Zhang |
| **Primaerkontakt** | **Tingji Zhang** (Erstautor) |
| **Affiliation** | CRAFT Lab, Tsinghua University, Beijing + Zhongguancun National Laboratory |
| **Lab-URL** | https://craft.cs.tsinghua.edu.cn/author/tingji-zhang/ |
| **Email Tingji Zhang** | zhangtj22@mails.tsinghua.edu.cn (THU-Standard-Format; zu verifizieren) |
| **Sekundaerkontakt** | Prof. Boris Grot (University of Edinburgh, EASE Lab) |
| **Email Grot** | boris.grot@ed.ac.uk |
| **Sekundaerkontakt 2** | Prof. Youhui Zhang (THU, Senior-Autor) |
| **Email Youhui Zhang** | zyh02@tsinghua.edu.cn (zu verifizieren) |
| **EASE Lab Webseite** | https://ease-lab.github.io/ |
| **Repo-Status** | ✅ **GEFUNDEN!** https://github.com/CRAFT-THU/gem5-hp |
| **Repo-Lizenz** | **BSD-3-Clause** (vom gem5-Basis-Projekt geerbt) — kompatibel mit Apache-2.0! |
| **Repo-Typ** | gem5-Fork (built upon gem5-fdp / Fetch Directed Instruction Prefetching) |
| **Hinweis** | **Achtung:** Hardware-Instruction-Prefetcher (kein DB-Index!). Code als gem5-Fork ~Hunderte MB. Konzept-Quelle, NICHT direkter Baustein. **Bundles-Mechanik** fuer PRT-ART Cache-Engine als Konzept-Inspiration relevant. |
| **Empfohlene Aktion** | KEIN klonen noetig (gem5-Fork zu schwer). Konzept aus Paper extrahieren + ggf. kurze Email an Tingji Zhang fuer Methodik-Detail. |

**ENGLISCHE Mail-Version vor Versand verwenden** (siehe `email/20260508-2000-email_p27_tzhang_en.md`).
Die deutsche Vorlage unten dient nur als Arbeits-Entwurf:

**Vorgeschlagener Email-Text P27 (REV 2 mit Vollangaben — DE-Entwurf, NICHT verschicken):**

> Betreff: Methodik-Anfrage zu Hierarchical-Prefetching-Bundle-Mechanik
>
> Sehr geehrter Herr Zhang,
>
> ich arbeite an einer Diplomarbeit an der TU Dresden unter Prof. Dr. Dirk Habich
> mit dem Titel "Active Cache-Aware Hardware Adaptation Cache Engine for Trie-
> Based Index Structures".
>
> WERK
>    Titel:    "Hierarchical Prefetching: A Software-Hardware Instruction
>               Prefetcher for Server Applications"
>    Autoren:  Tingji Zhang, Boris Grot, Wenjian He, Yashuai Lv, Peng Qu,
>              Fang Su, Wenxin Wang, Guowei Zhang, Xuefeng Zhang, Youhui Zhang
>    Venue:    ASPLOS 2025, 30th ACM International Conference on Architectural
>              Support for Programming Languages and Operating Systems, Volume 2
>    DOI:      10.1145/3676641.3716260
>
> Ihre Bundle-Mechanik fuer Hierarchical Prefetching ist eine direkte Konzept-
> Inspiration fuer eine der Prefetch-Strategien meiner Cache-Engine
> (Bausteine-Quer-Permutation auf Achse 7 — Prefetch).
>
> Ich habe Ihr Repository CRAFT-THU/gem5-hp gefunden (BSD-3-Clause), das ein
> gem5-Fork ist. Da meine Arbeit CPU-only auf Datenbank-Indexstrukturen
> fokussiert (kein Instruction-Prefetcher), brauche ich nicht das gem5-System,
> sondern moechte das Bundle-Konzept als Software-Prefetch-Strategie auf
> Datenstrukturen uebertragen.
>
> Frage: Gibt es eine isolierte Beschreibung des Bundle-Selection-Algorithmus
> (z.B. als Pseudocode oder kurzes Code-Snippet ausserhalb der gem5-
> Integration), die ich fuer eine PRT-ART-eigene Implementation als Adapter
> verwenden darf?
>
> Mit freundlichen Gruessen,
> Benjamin-Elias Probst (s2631336@tu-dresden.de)
> CC: Prof. Dr. Dirk Habich (dirk.habich@tu-dresden.de)

### P23 — Khan 2010 (Adaptive Prefetch via Code-Specializer)

| Feld | Wert |
|------|------|
| **Paper** | Data Cache Prefetching With Dynamic Adaptation (2010) |
| **Empfohlener Kontakt** | Khan (Affiliation in Paper-Header pruefen) |
| **Strategie** | Email; bei Nicht-Erreichbarkeit: Re-Implementation |

### P24 — Naderan-Tahan 2016 (Useless Prefetch)

| Feld | Wert |
|------|------|
| **Paper** | Why Does Data Prefetching Not Work for Modern Workloads (2016) |
| **Autoren** | M. Naderan-Tahan, H. Sarbazi-Azad |
| **Strategie** | Re-Implementation prinzipiell ausreichend (Studie/Analyse-Paper, kein Hauptcode-Beitrag) |

---

## 🔵 PRIORITAET NIEDRIG — RE-IMPLEMENTATION DEFAULT

Fuer folgende Paper ist eine **Re-Implementation in `prt_art/legacy_reimpl/`** gegenueber einer Email-Anfrage wahrscheinlich praktischer:

| P-ID | Paper | Begruendung |
|------|-------|-------------|
| P11 | CSS-tree (Rao/Ross 1999) | Pre-Github, Code vermutlich nicht mehr verfuegbar |
| P12 | CSB+-tree (Rao/Ross 2000) | Pre-Github |
| P13 | Hankins (2003) | Pre-Github, Konzept gut dokumentiert |
| P14 | Samuel CSB-Conscious (2005) | Pre-Github |
| P16 | Bender Tree Layout (2002) | Theorie-Paper, Algorithm gut dokumentiert |
| P17 | Bender Cache-Oblivious (2005) | Theorie-Paper |
| P18 | Saikkonen Multi-Level (2008) | Pre-Github, lange her |
| P19 | Saikkonen Layout-Invariant (2016) | Vermutlich kein Repo |
| P21 | Chen Prefetching B+ (2001) | Pre-Github |
| P22 | Chen Fractal (2002) | Pre-Github |

**Optional auch fuer diese:** Falls Email-Anfrage gewuenscht, sind die Autoren ueber DBLP / ResearchGate erreichbar. Aber Re-Implementation ist meist effizienter.

---

## 🟣 NEU: LIZENZ-KLAERUNGS-ANFRAGEN

Drei Repositories sind geklont, aber rechtlich problematisch fuer Apache-2.0-
Linkage (siehe `docs/lizenzen/20260508-1500-lizenzen_uebersicht.md`). Diese Anfragen sind WICHTIG vor
Phase 5 (Implementation):

### P04 — CoCo-Trie (Boffa et al. 2024) — GPL-3.0 Re-Lizenzierung

| Feld | Wert |
|------|------|
| **Repo** | https://github.com/aboffa/CoCo-trie (geklont, GPL-3.0) |
| **Erstautor** | Antonio Boffa |
| **Affiliation** | Universita di Pisa (CS Department) |
| **Email Boffa** | antonio.boffa@unipi.it (zu verifizieren) |
| **Senior-Co-Autor** | Prof. Paolo Ferragina (boss@unipi.it) |
| **Problem** | GPL-3.0 inkompatibel mit Apache-2.0 bei statischem Linken (F-EXTRA-1!) |
| **Strategie** | Bitte um Re-Lizenzierung als MIT/Apache-2.0/BSD fuer akademische Diplomarbeit |

**Vorgeschlagener Email-Text P04:**

> Subject: Re-Lizenzierungs-Anfrage CoCo-Trie fuer akademische Diplomarbeit TU Dresden
>
> Sehr geehrter Herr Boffa,
>
> ich arbeite an einer Diplomarbeit an der TU Dresden unter Prof. Dr. Dirk Habich
> mit dem Titel "Active Cache-Aware Hardware Adaptation Cache Engine for Trie-
> Based Index Structures". Eine zentrale Forschungsthese ist die Cross-Algorithm-
> Bausteine-Permutation, fuer die ich Originalcode mehrerer Paper EXAKT KOPIERT
> via Adapter-Pattern in eine Apache-2.0-lizenzierte Build-Maschinerie integriere.
>
> Ihr CoCo-Trie ist als Macro-Node-Encoding-Pool eine zentrale Quelle fuer
> meine Arbeit. Allerdings ist die GPL-3.0-Lizenz Ihres GitHub-Repos
> aboffa/CoCo-trie inkompatibel mit Apache-2.0 bei statischem Linken.
>
> Frage: Waeren Sie bereit, eine MIT/Apache-2.0/BSD-Lizenz-Variante fuer den
> akademischen Diplomarbeits-Zweck zu gewaehren? Alternative: Dual-Lizenz
> (GPL-3.0 + Apache-2.0)?
>
> Falls eine Re-Lizenzierung nicht moeglich ist, muss ich auf Re-Implementation
> aus dem Originalpaper ausweichen — was den authentischen Vergleichswert
> mindert.
>
> Mit freundlichen Gruessen,
> Benjamin Probst (s2631336@tu-dresden.de)
> CC: Prof. Dr. Dirk Habich (dirk.habich@tu-dresden.de)

---

### P07 — Wormhole (Wu/Ni/Jiang 2019) — GPL-3.0 Re-Lizenzierung

| Feld | Wert |
|------|------|
| **Repo** | https://github.com/wuxb45/wormhole (geklont, GPL-3.0) |
| **Erstautor** | Xingbo Wu |
| **Affiliation** | University of Illinois at Chicago (UIC), Computer Science |
| **Email Wu** | xingbow@uic.edu (zu verifizieren) |
| **Co-Autor** | Fan Ni, Song Jiang (Wayne State University) |
| **Email Jiang** | sjiang@wayne.edu |
| **Problem** | GPL-3.0 inkompatibel mit Apache-2.0 bei statischem Linken |
| **Strategie** | Re-Lizenzierungs-Anfrage analog zu P04 |

**Vorgeschlagener Email-Text P07:** (analog P04, mit Wormhole-spezifischem
Bezug auf MetaTrieHT + Anchor Keys)

---

### P25 — Mahling Fill Buffer (HPI 2025) — Lizenz-Festlegung

| Feld | Wert |
|------|------|
| **Repo** | https://github.com/hpides/prefetching (geklont, KEINE LICENSE) |
| **Erstautor** | Fabian Mahling |
| **Affiliation** | Hasso-Plattner-Institut (HPI), Universitaet Potsdam |
| **Email Mahling** | fabian.mahling@hpi.de (zu verifizieren) |
| **Senior-Autor** | Prof. Tilmann Rabl (tilmann.rabl@hpi.de) |
| **Problem** | README erwaehnt "Cite our work", aber keine LICENSE-Datei vorhanden — formal "All Rights Reserved" |
| **Strategie** | Bitte um explizite LICENSE-Festlegung (Apache-2.0/MIT) — fuer akademische Diplomarbeit-Verwendung |

**Vorgeschlagener Email-Text P25:**

> Subject: Lizenz-Festlegung "hpides/prefetching" Repository fuer akademische Diplomarbeit
>
> Sehr geehrter Herr Mahling,
>
> ich nutze Ihr "Fetch Me If You Can"-Microbenchmark-Repository (DaMoN 2025)
> als Vergleichsbasis in meiner Diplomarbeit an der TU Dresden unter
> Prof. Dr. Dirk Habich. Ihr Repository hpides/prefetching enthaelt jedoch
> aktuell keine LICENSE-Datei.
>
> Frage: Koennten Sie eine Lizenz-Datei (idealerweise Apache-2.0 oder MIT)
> festlegen? Das wuerde mir die saubere Adapter-Integration in unser
> Apache-2.0-lizenziertes Projekt comdare-cache-engine ermoeglichen.
>
> Falls eine LICENSE-Festlegung schwierig ist: Reicht es, wenn ich Ihr
> Paper zitiere (gemaess dem Citation-Block in Ihrer README) und bei
> akademischer Verwendung dokumentiere, dass die Microbenchmarks von Ihnen
> stammen?
>
> Mit freundlichen Gruessen,
> Benjamin Probst (s2631336@tu-dresden.de)
> CC: Prof. Dr. Dirk Habich (dirk.habich@tu-dresden.de)

---

## EMAIL-VERSAND-CHECKLISTE

Vor dem Versenden jeder Email:

- [ ] **Mit Habich abgestimmt?** (Email-Text, Kontaktauswahl)
- [ ] **TU-Dresden-Email-Account verwendet** (s2631336@tu-dresden.de)
- [ ] **Habich im CC** (dirk.habich@tu-dresden.de)
- [ ] **Diplomarbeit-Titel + Kontext angegeben**
- [ ] **Habich-Direktive (Originalcode-Bevorzugung) erwaehnt**
- [ ] **Lizenz-Klausel (Apache-2.0-kompatibel oder Originallizenz behalten)**
- [ ] **Antwort-Frist genannt (z.B. 2 Wochen)**

## SUCCESS-METRIK

| Status | Erwartung |
|--------|-----------|
| ✅ Code erhalten + Lizenz-OK | Direkt unter `ext/<paper>/` ablegen, Adapter konstruieren |
| ⏳ Code erhalten, Lizenz-Klaerung | Mit Habich Lizenz-Pruefung; ggf. nicht-distributiv unter `ext/<paper>/` |
| ❌ Keine Antwort nach 4 Wochen | Re-Implementation in `prt_art/legacy_reimpl/<paper>/` |
| ❌ Code nicht freigegeben | Re-Implementation in `prt_art/legacy_reimpl/<paper>/`, Hinweis im Logbuch |

## ZEITPLAN

- **Woche 1 (jetzt):** Email-Texte mit Habich abstimmen (Sprechstunde)
- **Woche 2:** Emails versenden (P31/P32/P33 zuerst, da TUD-direkt)
- **Woche 3-4:** Antworten sammeln, ggf. Nachfragen
- **Woche 5+:** Re-Implementation der nicht-erhaltbaren Paper starten
