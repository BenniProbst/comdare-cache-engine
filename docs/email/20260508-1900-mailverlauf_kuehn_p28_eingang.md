# Mailverlauf Roland Kuehn (TU Dortmund DBIS) — P28 DaMoN 2023

**Status:** Antwort eingegangen 2026-05-08 10:35 — Code-Zusage erhalten
**Werk:** "Towards Data-Based Cache Optimization of B+-Trees"
**Autoren:** Roland Kuehn, Daniel Biebert, Christian Hakert, Jian-Jia Chen, Jens Teubner
**Venue:** DaMoN 2023, ACM, Seattle (DOI 10.1145/3592980.3595316)

## Verlauf

1. **2026-05-07 02:45** — Anfrage an Kuehn (TU Dresden Diplomarbeit, Histogramm-pro-Knoten als Inspiration)
2. **2026-05-08 10:35** — Antwort Kuehn: Code-Zusage + neue Variante + Hinweise zur Cache-Kohaerenz
3. **2026-05-08 (heute)** — Antwort Probst: Plattform-Spezifikation, Bitte um Download-Link (Postfach 15 MB frei)

## Code-Status

**ZUGESAGT** — Roland will den Code nochmal "durchgehen, aufraeumen und dokumentieren". Anschliessend
wird ein Download-Link folgen (Probst hat darum gebeten, da Postfach voll).

**Charakter des Codes:** "experimenteller Evaluationscode, kein sauber gekapseltes Framework" —
muss vor Adapter-Konstruktion aufbereitet werden.

## ARCHITEKTUR-RELEVANTE ERKENNTNISSE (kritisch fuer PRT-ART Cache-Engine)

Drei substantielle technische Praezisierungen aus Kuehn's Antwort, die ueber das Paper hinausgehen:

### 1. Cache-Kohaerenz-Ping-Pong als Concurrency-Antagonist

> "Insbesondere auf die Knoten in den oberen Ebenen des Baums wird sehr haeufig zugegriffen,
>  sodass diese typischerweise in den Caches mehrerer Kerne liegen. Zusaetzliche Schreibzugriffe
>  fuehren dann durch die Cache-Kohaerenzmechanismen schnell zu starkem Cacheline-Ping-Pong
>  zwischen den Kernen, was die Performance entsprechend beeintraechtigt."

**Konsequenz:** Naive Pro-Knoten-Histogramm-Implementierung ist **unbrauchbar fuer parallele
Workloads** — Cache-Kohaerenz-Traffic frisst die Latenz-Vorteile auf.

**PRT-ART-Auswirkung:** ConcurrencyManager-Disziplin "MemoryAccessConcurrency::Write" muss
Cache-Kohaerenz-Kosten als Teil ihrer Cost-Funktion modellieren. Telemetrie auf
oberen Baum-Ebenen ist GEGEN-Pattern bei multi-core.

**Bausteine-Matrix Konsequenz:** Neue Concept-Constraint:
`requires NotInTopLevels<TelemetryStrategy>` — fuer Pro-Knoten-Histogramm-Strategien.

### 2. LeafOnly-Counter mit RetroactiveAggregation (NEU, nicht im Paper)

> "Wir haben daher mittlerweile auch eine vergleichsweise einfache Variante implementiert,
>  bei der die Zugriffsinformationen nicht in allen Knoten, sondern nur in den Blatt-Knoten
>  gespeichert werden. Vor dem eigentlichen Reordering werden dann alle Blaetter einmal bis
>  zur Wurzel traversiert, wobei die Zugriffszahlen fuer die inneren Knoten aufsummiert werden."

**Konsequenz:** Telemetrie-Erfassung **nur in Blaettern** + **retroaktive Aggregation** vor
Reordering vermeidet Cache-Kohaerenz-Problem komplett.

**PRT-ART-Auswirkung:** Neuer Bausteine-Typ in Bausteine_Matrix.txt Achse 7 (PREFETCH/TELEMETRY)
oder eigene Achse:
- `LeafOnlyAccessCounter` (per Leaf-Knoten 1 Counter)
- `RetroactiveAggregationStrategy` (Wurzel-Up-Traversal vor Reordering)

**Diese Variante ist NEU** und nicht im Original-DaMoN-2023-Paper enthalten — direkter
Methodik-Vorsprung gegenueber dem publizierten Stand.

### 3. Sampling-basierter Tradeoff (NEU, nicht im Paper)

> "Wir haben das Ganze zusaetzlich um eine Sampling-basierte Variante erweitert, bei der
>  nicht jeder Zugriff, sondern nur jeder n-te Zugriff im Blatt-Knoten gezaehlt wird."

**Konsequenz:** Konfigurierbarer **Genauigkeit-vs-Overhead-Tradeoff** ueber Sampling-Rate N.

**PRT-ART-Auswirkung:** Direkt mappbar auf F11 (Mikrobenchmark-Triggering: Sampled 1:N) —
aber jetzt auch fuer **Telemetrie-Erfassung**, nicht nur fuer Mess-Hooks. Neue Bausteine:
- `LeafOnlySampledCounter<N>` mit konfigurierbarem N (1, 100, 1000, ...)
- DecisionLambdaTree-Eintrag: bei hoher Last → Sampling-Rate N erhoehen (less measurement)

### 4. ART-Vorgaenger-Studie (Hintergrund)

> "Wir hatten tatsaechlich auch einmal eine Studienarbeit, die sich mit Runtime-Reordering
>  im Adaptive Radix Tree (ART) beschaeftigt hat. Damals sind wir allerdings nicht zu
>  besonders ueberzeugenden Ergebnissen gekommen und haben das Thema anschliessend nicht
>  weiter mit grossem Nachdruck verfolgt."

**Konsequenz:** Kuehn-Gruppe hat eigene ART-Reordering-Studie gemacht, ist aber nicht
publiziert worden (Ergebnisse "nicht ueberzeugend").

**PRT-ART-Auswirkung:** Bei Fehlschlag eigener ART-Reordering-Versuche koennte ein
Austausch mit Kuehn ueber damalige Schwierigkeiten wertvoll sein.
"Diplomarbeit-Update" hat Kuehn angeboten — Kontakt halten!

## Probst-Antwort-Inhalte (relevant fuer Architektur-Doku)

Probst hat in seiner Antwort an Kuehn folgende **Plattform-Spezifikation** offiziell festgehalten:

```
Plattform: Kubernetes-Cluster auf Talos OS
Workloads: Ryzen 9 9950X3D + i9-14900KS
RAM:       64 GB DDR5-5600, Latenz 36 Zyklen
```

Plus Selbst-Beschreibung des Forschungs-Frameworks:

> "Im Prinzip baue ich also ein Framework zu bewerten und Testen von Suchalgorithmen
>  mit dem Vergleich zwischen heuristischen und informierten Architektur-Entscheidungen
>  und automatischer Anpassung."

→ **Klare Forschungs-Mission-Statement** (heuristisch vs informiert + Auto-Adaption).
   Sollte als Forschungsthese-Klaerung in Architektur-Doku aufgenommen werden.

## Action Items

| Prio | Aktion | Verantwortlich |
|------|--------|----------------|
| ⏳ HOCH | Auf Kuehn's Code-Aufraeumung + Download-Link warten | Probst (passive) |
| 📝 MITTEL | LeafOnly-Counter + Sampling als neue Bausteine in `Bausteine_Matrix.txt` aufnehmen | Architektur (Task) |
| 📝 MITTEL | Cache-Kohaerenz-Ping-Pong als Concurrency-Antagonist im Domaenenmodell festhalten | Architektur (Task) |
| 📝 NIEDRIG | Plattform-Spezifikation aus Probst-Mail in Architektur-Doku aufnehmen | Architektur (Task) |
| 📨 LOW | Probst haelt Kuehn ueber Diplomarbeit-Stand auf dem Laufenden (Kuehn-Wunsch) | Probst (zukuenftig) |

## Verlauf-Vollzitat

(Mailverlauf vom 2026-05-08 10:35 zwischen Roland Kuehn und Benjamin-Elias Probst —
ungekuerzt aufbewahrt fuer kuenftige Nachverfolgung. Probst-Folge-Antwort gleichen Tag.)

**Anfrage Probst → Kuehn (2026-05-07 02:45):** Originalcode-Anfrage zu DaMoN 2023 mit
Histogramm-pro-Knoten-Mechanik fuer PRT-ART Cache-Engine.

**Antwort Kuehn → Probst (2026-05-08 10:35):** Du-Wechsel, Code-Zusage mit Aufraeumen,
Hinweis auf nicht-ueberzeugende ART-Reordering-Studie, technische Tiefe zu Cache-
Kohaerenz-Ping-Pong, neue LeafOnly-Sampling-Variante.

**Folge-Probst → Kuehn (2026-05-08 nachmittags):** Forschungs-Mission-Statement
(Framework heuristisch vs informiert), Plattform-Spezifikation (K8s/Talos, 9950X3D +
14900KS, 64 GB DDR5-5600), Bitte um Download-Link (Postfach 15 MB frei).
