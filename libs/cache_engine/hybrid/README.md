# hybrid/ -- Hybrid-Tier-Stufe (Klassifikation + Dock-Schicht + XML-Parser gebaut, Reroute offen)

> **NACHTRAG 17.08.2026 (Paket HY-A, Dock-Schicht + Parser) -- ZWEI ABSAETZE DIESER README SIND AB
> HIER UEBERHOLT.** Der Text darunter bleibt vollstaendig stehen (Doku wird deprecated, nicht
> geloescht) und ist in zwei Punkten nicht mehr der Ist-Stand:
>
> **1. Die Tabelle "Geplante Dateien" ist nicht mehr vollstaendig unerfuellt.** Fuenf der neun
> Zeilen sind jetzt gebaut, vier bewusst nicht:
>
> | Datei | Stand 17.08.2026 |
> |---|---|
> | `hybrid_dock_contract.hpp` | **GEBAUT** -- 4 Vertraege (standard/rollback/scan/resource_control) als constexpr-Registry nach dem Muster `run_methodology_registry.hpp`, POD-Deskriptor, FAIL-CLOSED-Lookup in zwei Formen (consteval fuer CT-Verwendungsstellen, constexpr/optional fuer den Parser), 10 benannte Status-Codes mit ableitbarer Namens-Wache |
> | `hybrid_pruef_dock.hpp` | **GEBAUT (Minimal)** -- `HybridDockVertrag`-Concept + `StandardHybridDock`. Alternativ-Dock-Typen fuer die uebrigen drei Vertraege: NICHT gebaut (F8-Minimal) |
> | `hybrid_dock_factory.hpp` | **GEBAUT** -- Abstract Factory nach Paragraf-49-KORREKTUR, einziger Konstruktions-Ort; traegt zugleich die `attach`-Definition, damit es keine zweite Vertrags-Abbildung gibt |
> | `hybrid_dock_array.hpp` | **GEBAUT** -- `HybridDockVariant` (die EINE erlaubte variant-Stelle), `DockSlot`, beide Policies, `DockArray` mit attach/detach/Antriebs-Bindung |
> | `hybrid_config_xml.hpp` | **GEBAUT** -- Parser der `<hybrid_tier>`-Sektion gegen die common-DOM. ZWEIWEIG-Schalter (`enabled`, XOR je Kette) + XML-Override von `max_docks` (KON42-01). Ohne XSD-Aenderung, ohne validate_profile-Anschluss |
> | `hybrid_binary_proxy.hpp` | **NICHT gebaut** -- er haelt `AnatomyModuleHandle` + Drive-Buendel aus der Builder-Lib. Das ist die offene Auflage **K2** (Abschnitt 7 des Design-Dokuments, "Entscheid vor HY-B1"). Der Slot nimmt den Antriebs-Zeiger stattdessen ENTGEGEN (`antrieb_binden`) -- die Naht ist genau eine Funktion breit |
> | `hybrid_tier_module.cpp` | **NICHT gebaut** -- der .so-Export haengt an derselben K2-Frage: eine Hybrid-.so muesste Builder-Code linken |
> | `hybrid_eviction.hpp` | **NICHT gebaut** -- HY-B2, ausdruecklich ausserhalb des F8-Minimal-DoD |
> | `hybrid_router.hpp` | **NICHT gebaut** -- HY-B2/HY-C, braucht echte Messkurven |
>
> **2. Die Stufe ist damit K2-NEUTRAL baubar, und das ist der tragende Schnitt.** Solange in
> `hybrid/` kein Loader vorkommt, beruehrt die Stufe die offene Auflage K2 nicht. Sie bleibt
> header-only (kein `add_subdirectory`), genau wie die Klassifikations-Schicht.
>
> **Was den variant-Satz betrifft:** "DockSlot ist die einzige variant-Ausnahme" ist eine
> ZONEN-Aussage ueber dieses Verzeichnis, KEINE System-Aussage (systemweit gibt es die
> CEB-Duldung, zwei legitime achsenfremde Nutzungen und einen test-only Cluster). Bewacht wird
> jetzt die Zone: `test_hy_a1_dock_contract` liest die hybrid/-Quelldateien und belegt, dass
> `std::variant<` darin an genau einer Stelle steht.
>
> **Weiterhin offen:** K1 (Lager-Identitaet), K2 (Loader-/Drive-Schichtung), K5
> (Snapshot-Aggregations-Semantik) sowie die Owner-Frage E-6 (welche Registry "22->23" meint --
> heute steht KEINE Achsen-Registry auf 22, der Eintrag ginge in die falsche Datei).

## Stand 09.08.-13.08.2026 (Historie) -- Klassifikations-Fundament

> **NACHTRAG 09.08.2026 (Paket HY-A1) -- DIESE README WAR AB HIER UEBERHOLT.**
> Der ganze folgende Text beschreibt den Stand bis zum 08.08.2026: einen reinen Namens-Stub
> ohne Code. Er bleibt unveraendert stehen (Doku wird deprecated, nicht geloescht), ist aber in
> zwei Punkten nicht mehr der Ist-Stand:
>
> 1. **Der Ordner enthaelt jetzt Code.** Vier Header, alle header-only, kein `add_subdirectory`
>    noetig (sie werden ueber den bestehenden Include-Pfad `libs/cache_engine` gefunden --
>    genauso wie `anatomy/`, das ebenfalls nicht als Unterverzeichnis eingetragen ist):
>
>    | Datei | Inhalt |
>    |---|---|
>    | `heuristik_adapter_klassifikation.hpp` | Einzelquelle aller Enum-Werte + Vollstaendigkeits-Wache + Partition ABI-sichtbar/Klassifikation |
>    | `heuristik_adapter_gate.hpp` | das Concept-Gate (S1 Zielfaehigkeit, S2 Paar-Konsistenz, S3 Ein-Gattung-Hybrid) |
>    | `heuristik_adapter_strategy.hpp` | Strategy je bedientem Genus (Primaertemplate + 5 Spezialisierungen + CRTP-Wache) |
>    | `heuristik_adapter_synthese_matrix.hpp` | 2D-Matrix-Liste Layer x Node, compile-time Rekursion |
>
> 2. **Der Bau-Zeitpunkt "erst in der Auswertungsphase" gilt fuer dieses Fundament nicht mehr.**
>    Er stammt aus Owner-Entscheid E1 (02.08.), der die Hybrid-Stufe als spaetere Stufe hinter der
>    CEB einordnete. Am 08.08. hat der Owner mit GO-3 die Hybrid-Natur zusaetzlich als eigene
>    **Gattung** benannt (`HEURISTIK-ADAPTER` / `Function-Interface-Reroute`, LEDGER:2488) und am
>    09.08. mit E-1 final ausdruecklich angeordnet: "Bitte lege es an." Die Klassifikations- und
>    Concept-Ebene ist damit vorgezogen; der eigentliche **Reroute**, das **Hybrid-Pruefdock** und
>    das **Tier-Modul** bleiben Folgepakete (HY-A2/A3) und sind hier weiterhin NICHT gebaut.
>
> Die unten stehende Tabelle "Geplante Dateien" ist davon UNBERUEHRT gueltig -- keine der dort
> genannten Dateien existiert bisher. Sie beschreibt die Dock-/Proxy-/Router-Schicht, HY-A1 hat
> die Klassifikations-Schicht DARUNTER gebaut.
>
> **Weiterhin offen und in HY-A1 bewusst NICHT entschieden:** K1 (Lager-Identitaet), K2
> (Schichten-Entscheid Loader/Drive), K5 (Snapshot-Aggregations-Semantik) -- und neu der
> Zahlen-Widerspruch **Node-Obergrenze 32 (09.08.) gegen Dock-Array-MaxN 8 (Q6, 02.08.)**,
> benannt im Kopf von `heuristik_adapter_synthese_matrix.hpp`.
>
> **STAND 13.08.2026:** der Zahlen-Widerspruch ist AUFGELOEST -- 32 loest die 8 als
> Dock-Obergrenze ab (KON28-03, Owner 12.08.: "maximal 32"). Die 32 ist ein PROGRAMM-DECKEL
> (statische Maximal-Variable, Wert willkuerlich, W7-anpassbar), KEIN Fach-Nenner -- Docks !=
> Mess-Permutationen (KON41-03). Default-Doktrin: constexpr-Default im Planer, jede XML-Eingabe
> ueberschreibt (KON42-01). Details im NACHZUG 13.08.2026 im Kopf von
> `heuristik_adapter_synthese_matrix.hpp`; der XML-Override-Mechanismus (max_docks-Parser) bleibt
> HY-A3. K1/K2/K5 bleiben unveraendert offen.

---

## Stand bis 08.08.2026 (Historie) -- RESERVIERTER STUB der Hybrid-Tier-Stufe (KEIN CODE)

<!--
  ZWECK DIESES ORDNERS (Paket HY-D2, 02.08.2026)
  Dieser Ordner ist eine reine NAMENS- UND SCHNITTSTELLEN-RESERVIERUNG fuer die Hybrid-Tier-Stufe
  nach Owner-Entscheid E1 vom 02.08.2026. Er enthaelt bewusst NUR diese README.
  KEIN Header, KEINE Quelle, KEINE CMakeLists, KEIN Build-Anschluss - libs/cache_engine/CMakeLists.txt
  listet seine Unterverzeichnisse einzeln per add_subdirectory (kein GLOB), dieser Ordner ist dort
  NICHT eingetragen und damit build- und byte-neutral.
  ANFASSEN ERST in der Auswertungsphase (Pakete HY-B1..HY-B4), nach Voll-Bau-4 und nach E-24.
-->

**Stufe:** Rekursions-Ebene 3 der Dock-Kette (Planer-Dock -> CEB-Pruef-Dock -> **Hybrid-Pruef-Docks**).
**Namespace (reserviert):** `comdare::cache_engine::hybrid`.
**Design (bindend):** `docs/architecture/20260802-hybrid_tier_stufe_soll_design.md`.
**Owner-Autoritaet:** Entscheid E1, super-Repo
`docs/sessions/20260802-OWNER-entscheide-hybrid-tier-stempel-regression-os-unterachsen.md:7`.
**Bau-Zeitpunkt:** Auswertungsphase. Vorher entsteht hier KEIN Code.

## Warum der Ordner jetzt schon existiert

Owner-E1 definiert die Hybrid-Tier-Binary als eigene Stufe HINTER der CEB, mit mehreren
ABI-stabilen Pruef-Docks, Factory-Pattern-Proxy auf ihre Tier-Binaries und `std::variant` als
begrenzte Ausnahme in einem wahlweise statischen oder Runtime-Dock-Array. Der Bau faellt in die
Auswertungsphase; die SCHNITTSTELLEN muessen aber JETZT freigehalten werden, damit die
Voll-Bau-4-Binaries spaeter nicht neu gebaut werden muessen (kein eigener ABI-Schritt fuer die
Hybrid-Stufe, siehe Design Abschnitt 6). Die Reservierung verhindert zusaetzlich, dass ein anderes
Paket den Namen `hybrid` in `libs/cache_engine/` belegt.

## Geplante Dateien (Auswertungsphase, Design Abschnitt 3)

| Datei | Inhalt |
|---|---|
| `hybrid_dock_contract.hpp` | `DockContractDescriptor` (POD) + Contract-Concept + constexpr contract-Registry |
| `hybrid_pruef_dock.hpp` | Standard-Dock + Alternativ-Dock-Typen je abweichendem Vertrag |
| `hybrid_dock_factory.hpp` | Abstract Factory (§49-KORREKTUR): einziger Konstruktions-Ort der variant-Alternativen |
| `hybrid_dock_array.hpp` | `DockArray<Policy>`: statische ODER Runtime-Policy, Belegung immer dynamisch |
| `hybrid_binary_proxy.hpp` | Proxy je Dock auf seine plain Tier-Binary (besitzt `AnatomyModuleHandle`) |
| `hybrid_eviction.hpp` | Verdraengungs-Strategie (Compile-Time-Strategy) |
| `hybrid_router.hpp` | Break-Even-Router (Compile-Time-Chain-of-Responsibility) |
| `hybrid_config_xml.hpp` | Parser der `<hybrid_tier>`-XML-Sektion (common-DOM, Doktrin NUR EIN XML-Programm) |
| `hybrid_tier_module.cpp` | Export der Hybrid-Binary als normales Tier-Modul (4 ABI-Pflicht-Symbole) |

## Harte Randbedingungen fuer den spaeteren Bau

- `std::variant` ausschliesslich als `HybridDockVariant` im `DockSlot`; `std::visit` NUR an
  Konfigurations-/Umschaltpunkten. Der Op-Hot-Path bleibt variant-frei und cast-frei
  (Haupt-Kommunikation = statisches `IObservableTier`). In plain Tier-Binaries bleibt `std::variant`
  verboten.
- Kein neuer ABI-Schritt: beide Grenzen der Stufe sind die bestehende Anatomy-ABI. Der Satz nannte
  hier bis zum 08.08.2026 eine Zahl, die inzwischen gewandert ist; sie bleibt als Historie stehen:
  Stand 02.08.2026: "Major 7, `anatomy_module_abi_v1_decl.hpp:62`" -- ABI-HISTORIE gegen SHA 6b8eee0f
  Stand 04.08.2026: "Major 8, Magic `.A8.` = `0x434F4D444141382EULL`" -- ABI-HISTORIE gegen SHA 0f08fab5
  **LEBENDER STAND seit NAHT-1 (09.08.2026, Mess-Naht am Genus-Interface): Major 9** -- Beleg `anatomy_module_abi_v1_decl.hpp:109`.
  **Magic `.A9.`** (`0x434F4D444141392EULL`) -- Beleg `anatomy_module_abi_v1_decl.hpp:114`.
- Die Dock-Bestueckung ist Runtime-Konfiguration und gehoert in ein Sidecar-Manifest, NIE in die
  binary_id. Das Sidecar-Format wird erst NACH der A13-Stempel-Regression fixiert.
- Offene Auflagen K1 (Lager-Identitaet der Hybrid-.so), K2 (Schichten-Entscheid Loader/Drive),
  K4 (Verhaeltnis zu LEDGER:187(e)) und K5 (Snapshot-Aggregations-Semantik) sind VOR dem jeweiligen
  Bauschritt zu klaeren -- Details im Design-Dokument Abschnitt 7.
- Erster Meilenstein der Auswertungsphase ist der F8-Minimal-DoD (genau 1 Standard-Dock,
  ctest-bewiesen), nicht der Vollausbau.

## Nachtrag 08.08.2026 (Paket HY-0) -- ABI-Etikett und zwei Lesefallen

Nichts oben ist geloescht. Dieser Abschnitt zieht nach, was sich seit dem 02.08. bewegt hat, und
benennt zwei Stellen, an denen ein Leser die falsche Schicht bauen wuerde.

### (1) Die ABI-Major dieser Doku war zwei Tage nach ihrer Entstehung ueberholt

Der Bau-Zeitpunkt der Hybrid-Stufe liegt in der Auswertungsphase, ihre Doku entstand am 02.08. --
dazwischen fiel am 04.08. der E-24-C8-Bump. Wer die obige Randbedingung woertlich nahm, baute
gegen eine Anatomy-ABI, die der Loader heute per Major-Mismatch ablehnt (`host_compatible_with`,
Schritt 5 der 7-Schritt-Validierung). Der Freihalte-Entscheid selbst haelt unveraendert: die Stufe
braucht KEINEN eigenen ABI-Schritt -- sie haengt nur an einer anderen Zahl als aufgeschrieben.

Stand 02.08.2026, unveraendert stehengelassen (jede Zeile gegen ihren SHA nachgerechnet):

- ABI-Major 7 -- Anker `anatomy_module_abi_v1_decl.hpp:62` -- ABI-HISTORIE gegen SHA 6b8eee0f
- ABI-Magic `.A7.` -- Anker `anatomy_module_abi_v1_decl.hpp:66` -- ABI-HISTORIE gegen SHA 6b8eee0f

Lebender Stand seit E-24 C8 (04.08.2026, ce `4f569051`):

- Stand 04.08.2026: "Major 8, Magic `.A8.` = `0x434F4D444141382EULL`" -- ABI-HISTORIE gegen SHA 0f08fab5
- **Major 9** -- Beleg `anatomy_module_abi_v1_decl.hpp:109`
- **Magic `.A9.`** = `0x434F4D444141392EULL` -- Beleg `anatomy_module_abi_v1_decl.hpp:114`

Die alten Zeilen-Anker sind nicht falsch geschrieben, sondern GEWANDERT: gegen SHA `6b8eee0f`
-- den Stand, den Abschnitt 11 des Design-Dokuments selbst als seine Basis nennt -- zeigen `:62`
und `:66` wirklich auf die beiden `#define`-Zeilen der damaligen ABI. Deshalb werden sie als
verifizierte Historie gekennzeichnet und nicht stillschweigend korrigiert -- wer die Herkunft
einer Aussage tilgt, nimmt dem naechsten Leser die Moeglichkeit, sie nachzupruefen.

**Was das Halten erzwingt:** `scripts/ci_hy_label_gate.sh` (ctest-Test `hy_label_gate`). Die Wache
liest `COMDARE_ANATOMY_ABI_MAJOR` maschinell aus dem Decl-Header -- sie traegt selbst KEINE
Major-Zahl -- und haelt jede Major-/Magic-Angabe dieser Doku dagegen. Zusaetzlich rechnet sie jeden
Anker nach: ein LEBEND-Anker muss im heutigen Header auf den genannten `#define` zeigen, ein
Historien-Anker im gepinnten SHA-Objekt. Beim naechsten Bump wird diese Doku ROT, nicht still falsch.

### (2) LESEFALLE: "vierte Mess-Ebene" bedeutet hier etwas anderes als bei `checkpoint_measure`

Es gibt im Haus zwei verschiedene "vierte Ebenen". Sie sind NICHT dieselbe Sache, und wer sie
gleichsetzt, baut die falsche Schicht:

- **Die 4. MESS-EBENE der Hybrid-Stufe** (Owner-KERN 08.08.2026, Gattung `HEURISTIK-ADAPTER`,
  Genus `Function-Interface-Reroute`): die Hybrid-Tier-Binary bringt eine EIGENE
  Macro-Benchmarking-Schicht mit. Die drei kanonischen Mess-Ebenen werden dadurch auf VIER
  erweitert, und zwar **dazwischengequetscht, nicht angehaengt**. Gemessen wird der **Overhead des
  Reroutes zu multiplen Tier-Binary-Zielen, am Hybrid-Pruefdock**. Das ist eine Schicht mit eigenem
  Code, eigener Gattung und eigenen Pflicht-Zahlen in der Mess-Kampagne.
- **Die offene "vierte Ebene" bei `checkpoint_measure`** (`docs/architecture/20260808-checkpoint_measure_soll_design.md`,
  Offene Punkte 3): dort ist gefragt, ob die **Gattungs-Interface-Ebene** -- der Verbinder zwischen
  den Achsen-Aufrufen, dessen Anteil *Macro-Gesamt minus Summe der Micros* ist -- eine eigene Stufe
  im Ebenen-FILTER wird oder ein Sonderfall von `macro` bleibt. Das ist eine Frage an die
  Aufloesungsregel des Checkpoint-Stacks (gegen welche Ebene ein Micro-Checkpoint seinen Aufrufer
  sucht), KEINE neue Mess-Schicht.

Kurz: die eine Vier ist eine **gebaute Schicht**, die andere ein **Filter-Rang**. Wer die
Hybrid-Ebene fuer den Gattungs-Interface-Sonderfall haelt, baut die Hybrid-Macro-Schicht gar nicht
und haelt die Owner-Pflicht faelschlich fuer erledigt; wer umgekehrt den offenen Punkt 3 mit dem
Owner-KERN beantwortet zu haben glaubt, hat eine Frage geschlossen, die nie gestellt war. Die
Zaehlung "drei Ebenen" in `checkpoint_measure_soll_design.md` bleibt fuer den Checkpoint-Stack
richtig; die Hybrid-Ebene tritt dort NICHT automatisch als vierter Flag-Wert hinzu -- ob sie es
soll, ist ein eigener Entscheid und in beiden Dokumenten als offen benannt.
