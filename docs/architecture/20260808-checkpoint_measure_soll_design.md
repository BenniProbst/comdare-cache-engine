# `checkpoint_measure(..., IN|OUT)` — SOLL-DESIGN

**08.08.2026 · Owner-Entscheid · Status: SPEZIFIKATION mit gemessenen Kosten, noch nicht gebaut**

Primärquelle und Verbatim: `super docs/DIPLOMARBEIT-ZIELE-OFFENE-PUNKTE-LEDGER.md`, Abschnitte
*OWNER-ENTSCHEID … checkpoint_measure ist EINE uniforme Funktion* (super `f2767525`), *OWNER-NACHTRAG …
Prozess UND Thread* (super `f94345c8`, N-1..N-7) und der Blattform-Entscheid (super `3326ac7a`).

---

## 1. Was es ist

Eine **inline Steuerzeile** an den Mess-Punkten, die auf einen **prozessweiten Speicher-Stack**
schreibt. Der Stack nimmt Parameter und Zeitpunkte **je der drei Mess-Ebenen** auf und dokumentiert den
Lauf **zur Laufzeit**.

> *„Ein checkpoint loggt also den Aufrufenden, die gelandete Zielfunktion, die Systemzeit und alle dafür
> spezifischen compile time Parameter der/aller Achsen zu diesem Zeitpunkt und ein checkpoint flag
> „IN". checkpoint OUT unterscheidet sich also in der checkpoint Funktion NICHT, sondern hat nur ein
> compile time „OUT" flag als Tag. Die Funktion für `checkpoint_measure(...,IN bzw. OUT)` ist uniform.“*

## 2. Eine Funktion, kein Paar

Es gibt **nicht** `enter()` und `leave()`, sondern **einen** Aufruf mit unterschiedlichem Tag. Zwei
Einträge entstehen, weil die Zeile **zweimal steht**.

**`IN`/`OUT` ist ein compile-time Tag** — kein Laufzeit-Enum, keine Verzweigung. Das fügt sich in die
Doktrin *statischer Dispatch, kein Laufzeit-`switch`*, und erlaubt, den `OUT`-Pfad per `if constexpr`
anders zu übersetzen, ohne dass im Hot-Path je ein Sprung entsteht.

## 3. Der Aufrufende ist ein TRIPEL

Owner, Präzisierung vom 08.08.:

> *„Der Aufrufende ist der Name des aufrufenden Prozesses, der Name der aufrufenden Funktion auf dem
> Stack, und die Thread Nummer des aufrufenden Programm-Flusses.“*

| Bestandteil | Woher | Kosten im Hot-Path |
|---|---|---|
| Name des **Prozesses** | **einmal beim Start** ermitteln, danach Verweis | **null** je Checkpoint |
| Name der **aufrufenden Funktion auf dem Stack** | `std::source_location`, durchgereicht — s. §5 | **~1 ns** |
| **Thread-Nummer** des Programm-Flusses | eigene thread-lokale laufende Nummer | **~1 ns** |

Dazu die **gelandete Zielfunktion** (neu gegenüber N-1..N-7), die **Systemzeit**, die **compile-time
Achsen-Parameter** und das **Flag**.

**Erst das Paar (Aufrufer, Ziel) ist eine vollständige Stack-Kante.** Der Aufrufer sagt, woher der
Besuch kam; das Ziel, wo er ankam. Die Blattform bekommt dadurch **eine Spalte dazu**.

## 4. Sparsamkeit ist harte Randbedingung, nicht Stil

> *„Das Verfahren muss besonders sparsam sein, weil wir Latenzen nicht in der Messung dulden können.“*

**Der Messfühler ist ein Verbraucher.** Was der Checkpoint kostet, misst er mit — darum gibt es
überhaupt sechs CEB-Varianten mit ein- und ausgebauten Messgeräten. Drei Bau-Regeln sind **zwingend**:

**R1 — Die compile-time-Achsen-Parameter werden zur Laufzeit weder zusammengebaut noch kopiert.** In den
Stack gehört **ein Verweis auf einen statischen Deskriptor** (Zeiger oder Index), nicht die
ausgeschriebene Achsen-Kette. Wer die volle Beschreibung je Checkpoint schreibt, erzeugt die verbotene
Latenz **proportional zur Achsenzahl** — also am stärksten dort, wo am feinsten gemessen wird.

**R2 — Kein Speicher wird im Hot-Path angefordert.** Vorab alloziert, feste Kapazität.

**R3 — Kein I/O im Hot-Path.** Der Stack ist *memory*; geschrieben wird beim Auslesen.

## 5. C++23 `<stacktrace>` — recherchiert, verfügbar, und **im Hot-Path ausgeschlossen**

Der Owner hat ausdrücklich verlangt, das zu recherchieren. Ergebnis in drei Teilen:

### (a) Es existiert und ist auf **dieser** Maschine verfügbar

Am Objekt geprüft, nicht angenommen:

| Compiler | stdlib | `<stacktrace>` | `__cpp_lib_stacktrace` |
|---|---|---|---|
| GCC 15.3.0 | libstdc++ 15 | **ja** | `202011` |
| clang 22.1.8 | libstdc++ 16 (Default) | **ja** | `202011` |

Der verbreitete Vorbehalt *„libc++ hat `<stacktrace>` nicht"* **greift hier nicht**: clang baut auf
dieser Maschine gegen libstdc++, und libc++ ist gar nicht installiert (`-stdlib=libc++` scheitert schon
an `<cstdio>`). Linkflag bei GCC: `-lstdc++exp`.

### (b) Es ist für den Hot-Path um Größenordnungen zu teuer — **gemessen**

Eigener Mikrobenchmark, `-O2`, 200 000 Aufrufe je Variante, `volatile` Senke gegen Wegoptimieren,
**warme** Werte aus drei übereinstimmenden Wiederholungen (GCC 15.3):

| Kandidat | ns/Aufruf | relativ |
|---|---|---|
| `std::source_location` als Default-Argument | **0,92** | 1× |
| eigene Thread-Nummer (hier: `get_id()`+hash als obere Schranke) | 3,5 | 3,8× |
| `__rdtsc()` | **6,8** | 7,4× |
| `steady_clock::now()` | **16,5** | 18× |
| `std::stacktrace::current(1, 1)` — **ein** Frame, ohne Auflösung | **343** | **373×** |
| `std::stacktrace` + `description()` (Symbolauflösung) | **~26 900** | **~29 000×** |

**Rechnung, die den Ausschlag gibt.** Auf der Micro-Ebene sitzt vor und nach *jedem* Achsen-Aufruf ein
Checkpoint. Bei 19 Achsen sind das 38 Checkpoints je Interface-Aufruf:

* mit `source_location` + `__rdtsc` + Store: **rund 380 ns** je Interface-Aufruf
* mit `std::stacktrace::current(1,1)`: **rund 13 µs** — mehr als die gemessene Operation selbst

**Damit ist `std::stacktrace` im Hot-Path ausgeschlossen.** Das deckt sich mit der Nutzungsempfehlung
der Bibliothek selbst: *gedacht für Fehlerpfade, nicht für performance-kritischen Code.*

### (c) Sein Platz ist der **Fehlerpfad** — und dort ist er wertvoll

Wenn die IN/OUT-Balance verletzt ist (§7) oder der Stack überläuft (§6), will man **genau** einen
echten, aufgelösten Stack. Dort sind 27 µs bedeutungslos, weil der Lauf ohnehin einen Befund meldet.
**Also: `<stacktrace>` einbauen — aber ausschließlich im Diagnosezweig, nie in der Steuerzeile.**

### (d) **EIGENER Stacktrace, nicht der der Standardbibliothek** — Owner-Entscheid 08.08.

> *„Die Spannung lässt sich lösen, wenn auf dem custom Stack in einer gefilterten Ebene höher (Macro zu
> Micro, Compare zu Macro) auf dem globalen custom stack die aufrufende Funktion mit deren IN-Checkpoint
> notiert wird. Also müssen wir zur compile time cmake Flags setzen, die automatisch in die
> checkpoint_measure einkompiliert werden, in welcher Mess-Ebene sich der checkpoint befindet, sodass er
> rückwärts den nächstgelegenen checkpoint eine Ebene höher finden und als aufrufende Funktion uniform
> identifizieren kann. Wir verwenden also ein eigenes C++ stacktrace und nicht das standard
> stacktrace.“*

**Der Aufrufer wird nicht ermittelt, sondern rekonstruiert.** Das ist der Kern und macht die ganze
Frage aus §5(b)/(c) gegenstandslos:

1. Jeder Checkpoint trägt **compile-time seine Mess-Ebene** (`compare` / `macro` / `micro`), gesetzt
   über **CMake-Flags**, die in `checkpoint_measure` einkompiliert werden — genau wie das `IN`/`OUT`-Tag.
2. Der **Aufrufer** eines Checkpoints ist der **nächstgelegene, rückwärts liegende `IN`-Checkpoint der
   nächsthöheren Ebene**. Macro zu Micro, Compare zu Macro.
3. Aufgelöst wird das **beim Auslesen**, nicht beim Schreiben.

**Was das kostet: nichts.** Die Ebene ist eine compile-time-Konstante wie das Flag. Im Hot-Path schreibt
ein Checkpoint weiterhin nur *Deskriptor-Verweis · Thread-Nr · Zeit · Tags*. Es gibt **kein**
Durchreichen durch die Aufrufkette, **keinen** `__builtin_return_address`, **keinen**
`std::stacktrace::current()`.

**Und es ist genauer als jeder Stacktrace der Standardbibliothek.** `std::stacktrace` kennt nur
Maschinen-Frames; es weiß nichts von Mess-Ebenen und würde jede Zwischenfunktion mitliefern, die
architektonisch bedeutungslos ist. Der eigene Stack trägt **die Semantik, um die es geht** — die drei
Mess-Ebenen. Deshalb ist er hier nicht der Notbehelf, sondern das schärfere Werkzeug.

**Damit ist auch der Fehlerpfad abgedeckt.** Ich hatte vorgeschlagen, `std::stacktrace` wenigstens bei
verletzter IN/OUT-Balance zu ziehen. Das ist **nicht nötig**: der eigene Stack weiß bereits, *welcher*
`IN` offen blieb — samt Funktion, Ebene, Thread und Zeitpunkt. Das ist genau die Information, die der
Befund braucht. **`std::stacktrace` wird also gar nicht verwendet.** Die Recherche bleibt trotzdem im
Dokument, weil sie die Entscheidung belegt.

### Die Rekonstruktion ist ein Durchlauf, keine Suche

Eine wörtlich genommene *Rückwärtssuche je Eintrag* wäre beim Auslesen quadratisch — bei Millionen
Einträgen untragbar. Sie ist auch nicht nötig:

**Ein einziger Vorwärts-Durchlauf mit je einem offenen Stapel pro Ebene** löst alle Aufrufer in **O(n)**
auf. Ein `IN` legt seinen Eintrag auf den Stapel seiner Ebene; ein `OUT` nimmt ihn herunter; jeder
Checkpoint bekommt als Aufrufer die **Spitze des Stapels der nächsthöheren Ebene**. Am Ende
verbliebene Einträge auf einem Stapel **sind exakt die Regressionen aus §7** — die Invariante fällt
also als Nebenprodukt derselben Auswertung ab, ohne eigenen Durchlauf.

### Die Rückwärtssuche verlangt Ordnung je Thread — und entscheidet damit §9

Ein Micro-Aufruf in Thread A wird von einem Macro-`IN` **in Thread A** gerufen, nie von einem in
Thread B. Auf einem *geteilten* Stack, in den mehrere Threads durcheinander schreiben, würde die
Rückwärtssuche **den falschen Aufrufer finden** — sie träfe den zeitlich nächsten fremden `IN`.

**Damit ist die Empfehlung aus §9 nicht mehr nur eine Sparsamkeits-Frage, sondern eine
Richtigkeits-Frage:** thread-lokale Puffer machen die Rekonstruktion *korrekt*, nicht bloß schnell. Wer
einen echt geteilten Stack baut, muss bei der Auflösung zusätzlich nach Thread filtern — und hat dann
die Sperren im Hot-Path umsonst bezahlt.

### `std::source_location` bleibt — für das Ziel

Für die **gelandete Zielfunktion** ist es weiterhin das richtige Werkzeug:
`std::source_location::current()` ist **`static consteval`**, als Default-Argument an der Aufrufstelle
ausgewertet. Der Name entsteht beim Übersetzen; die gemessenen 0,92 ns sind bereits das Weiterreichen
und Ablegen, nicht das Ermitteln. Im Probelauf belegt: der Default-Argument-Trick liefert genau die
Funktion, **in der die Zeile steht**.

**Die Arbeitsteilung ist damit sauber:** `source_location` liefert das **Ziel** (compile-time, an der
Zeile), der eigene Stack liefert den **Aufrufer** (beim Auslesen, aus der Ebenen-Ordnung).

## 6. Die Zeitquelle — und eine Korrektur meiner eigenen Schätzung

**KORREKTUR.** Ich hatte im Ledger (super `f2767525`, §C-5b) geschätzt, `clock_gettime` koste
*„grob 20–25 ns"* und ein Zykluszähler *„etwa eine Größenordnung weniger"*. **Beide Zahlen sind
falsch.** Gemessen: `steady_clock::now()` **16,5 ns**, `__rdtsc()` **6,8 ns** — Faktor **2,4**, nicht
10. Der Zykluszähler ist also billiger, aber nicht dramatisch.

**Folge für das Design:** die Zeitquelle bleibt der **teuerste unvermeidbare Posten** eines
Checkpoints — mehr als Aufrufer, Ziel und Thread-Nummer zusammen. Die Wahl zwischen beiden lohnt sich,
ist aber kein Größenordnungssprung. Empfehlung unverändert: **billiger monotoner Zähler im Hot-Path,
einmal je Lauf gegen die Systemzeit verankert.** Die Stempel bleiben absolut lesbar, und *ein* Anker je
Prozess macht sie über Threads hinweg vergleichbar — **damit ist die aus N-7 offene Zeitbasis-Frage
beantwortet**.

**Und es bleibt messbar.** Die sechs CEBs existieren genau dafür: die Differenz „mit Fühler" zu „ohne
Fühler" **ist** der Messfehler. Diese Wahl muss nicht geglaubt werden.

**Messvorbehalt, damit niemand die Zahlen überstrapaziert:** der **erste** Lauf jeder Binary war
durchweg rund doppelt so teuer wie die drei folgenden (`steady_clock` 37,7 statt 16,5;
`stacktrace` 666 statt 343). Das ist derselbe Kalt-Cache-Effekt, der im Haus als Runner-Instabilität
bekannt ist. **Die Tabelle nennt warme Werte.** Für absolute Aussagen gehört die Messung in die
CEB-Maschinerie, nicht in einen Ad-hoc-Benchmark — hier trägt sie nur die *Rangfolge*, und die ist über
beide Compiler stabil.

## 7. Die Invariante

> *„Wenn ein Thread ein Interface betritt, aber es nicht wieder verlässt, ist das eine Regression.“*

Je `(Prozess, Thread, Interface)` muss die Checkpoint-Folge **balanciert** sein. Ein Überhang an `IN` am
Ende eines Laufs ist **kein fehlender Messwert, sondern ein Befund** — und der Moment, in dem
`std::stacktrace` seinen Auftritt hat.

**Das ist der Grund, warum `IN` und `OUT` einzeln erfasst werden:** würde je *abgeschlossenem* Aufruf
ein Eintrag geschrieben, hinterließe ein Aufruf, der nie zurückkehrt, **keine Spur** — genau der Fall,
der erkannt werden soll.

## 8. Der Überlauf ist ein Befund, kein Verlust

| Weg | Warum falsch |
|---|---|
| still verwerfen | die Messung lügt, ohne es zu sagen |
| blockieren, bis Platz ist | erzeugt die verbotene Latenz |

**Weiterlaufen, den Überlauf zählen, beim Auslesen melden** — wie ein `IN` ohne `OUT`.

## 9. „global" + mehrere Threads + „sparsam"

Ein *geteilter* Stack braucht Synchronisation; jede Sperre im Hot-Path ist die ausgeschlossene Latenz,
unter Contention sogar unbegrenzt. Zugleich schreiben laut N-7 **mehrere Threads gleichzeitig und
durcheinander**.

**Empfehlung: thread-lokale Puffer unter einem prozessweit erreichbaren Sammler.** Aus Sicht des Lesers
bleibt es *ein globaler Stack* — wie beauftragt —, aber im Hot-Path schreibt jeder Thread
**unsynchronisiert in sein eigenes Stück**. Zusammengeführt wird **beim Auslesen**, wo Latenz nichts
kostet; und das ist ohnehin nötig, weil die Blattform nach **Ankunftsfolge** sortiert und die Zerlegung
je Thread erst beim Lesen geschieht.

## 10. Wo das andockt

`libs/cache_engine/profile_facade/mess_achsen_naht.hpp:81-95` nennt die eigene Grenze: **macro und micro
sind heute nicht trennbar, G2 und G3 teilen sich ein Gate.** Solange das gilt, sind von sechs
CEB-Varianten **höchstens zwei** herstellbar; das Herauslösen von G3 ist ein benanntes Folgepaket und
berührt `abi_adapter.hpp` im Hot-Path.

**Kein Widerspruch, sondern die Reihenfolge:** `checkpoint_measure` kann vor G3 gebaut werden — aber der
*Nachweis* des Mess-Overheads über die CEB-Differenz braucht G3. Wer §4 belegen will, braucht es.

---

## Offene Punkte

1. **Ein Deskriptor für Ziel, Ebene *und* Achsen-Parameter?** Alle drei sind compile-time bekannt.
   Lassen sie sich zu **einem** statischen Deskriptor zusammenfassen, schreibt ein Checkpoint nur noch
   *Deskriptor-Verweis · Thread-Nr · Zeit · Tag* — vier Werte, alle in Registern.
2. **Ist die Mess-Ebene je Target eindeutig?** Die CMake-Flag-Lösung setzt die Ebene pro Übersetzungs-
   einheit oder Target. Das trägt, solange `compare` (CEB), `macro` (Tier-Binary-Interface) und `micro`
   (Achsen) in getrennten Targets liegen. **Enthielte eine Übersetzungseinheit Checkpoints zweier
   Ebenen, reichte ein Datei-globales Flag nicht** — dann müsste die Ebene am Aufruf stehen (weiterhin
   compile-time, als Tag wie `IN`/`OUT`). Am Objekt zu prüfen, bevor die Flags gesetzt werden.
3. **Wohin gehört die Gattungs-Interface-Ebene?** Sie verbindet die Achsen-Aufrufe, ohne sie zu
   überwachen; ihr Anteil ist *Macro-Gesamt minus Summe der Micros* und muss separat gemessen werden.
   Ist sie eine **vierte** Ebene im Filter oder ein Sonderfall von `macro`? Das entscheidet, gegen
   welche Ebene ein Micro-Checkpoint seinen Aufrufer sucht.
4. **Kapazität des Stacks** — feste Zahl, aus der XML, oder aus der erwarteten Aufrufzahl gerechnet?
   Hängt an `--check-size`, das die Größe ohnehin auf der CEB rechnet.
5. **Prozess-Ende ohne Auslesen** — ein stiller Verlust wäre derselbe Fehler wie ein verschwiegener
   Überlauf.
6. **Verhältnis der Ebenen-Flags zu den CEB-Gates.** Die sechs CEB-Varianten sind ein- und ausgebaute
   Mess-Ebenen; die neuen Ebenen-Flags beschreiben dieselbe Achse. **Das sollte EIN Mechanismus sein,
   nicht zwei.** Berührt unmittelbar das G3-Folgepaket aus §10.
