# Hybrid-Tier-Stufe -- SOLL-Design (Owner-Entscheid E1 vom 02.08.2026)

**Status:** DESIGN FESTGESCHRIEBEN, BAU IN DER AUSWERTUNGSPHASE. Dieses Dokument haelt die
Schnittstellen frei; es entsteht KEIN Code und KEIN Byte-Ereignis vor dem Voll-Bau-4.
**Paket:** HY-D2 (Architektur-Doc + Verzeichnis-Stub + XSD-Kommentar-Reserve), Strang A17.
**Autoritaet:** Owner-Entscheid E1, `docs/sessions/20260802-OWNER-entscheide-hybrid-tier-stempel-regression-os-unterachsen.md`
(super-Repo, Zeile 7 verbatim + Zeilen 27-31 Manager-Auslegung). Owner-KERN = Gesetz.
**Bauplan-Einordnung:** `docs/sessions/20260802-BAUPLAN-owner-entscheide-synthese-kritischer-pfad.md` §1 (A17).
**Anker-Stand:** alle `datei:zeile`-Belege dieses Dokuments sind gegen den development-Stand
`6b8eee0f` verifiziert (Abschnitt 9 fuehrt die Liste).
**Namenskonvention Pfade:** `ce` = dieses Repo (`Code/external/comdare-cache-engine`),
`LEDGER` = `docs/DIPLOMARBEIT-ZIELE-OFFENE-PUNKTE-LEDGER.md` im super-Repo.

---

## 1. Owner-Wortlaut (Gesetz)

Zitat Owner-Entscheid 1 vom 02.08.2026, ASCII-Transliteration (Umlaute als ue/ae/oe/ss);
die Fassung mit Original-Orthografie steht verbatim in der oben genannten Owner-Session-Datei,
Zeile 7 -- bei jedem Zweifel gilt dort der Wortlaut, nicht diese Transliteration:

> "Das ist kein Konflikt, weil die Hybrid-Tier Binaries bei exakter Designplanung durch Fable 5
> xhigh und ultracode klar als eine weitere Stufe hinter der CEB definiert sind. Sie haben mehrere
> Pruef-docks und verwenden fuer den Einsatz ihrer Pruef-Docks zur Proxy Verwendung ihrer
> Tier-Binaries als Factory Pattern, in der Regel als Ausnahmen std::variant . Aber in den plain
> Tier-Binaries ist das verboten. Die hybrid-Tier-Binaries fahren eine Zwischenloesung zwischen
> statischen Pruef-Docks und austauschbaren plain Tier-Binaries je Pruefdock, das ist eine XML
> Konfiguration auf Wunsch des anwenders in der Auswertungsphase. Es ist auch einstellbar, dass die
> Anzahl der ABI stabilen Pruefdocks an der hybrid-Tier-Binary zahlenmaessig dynamisch variieren
> kann, genau dafuer braucht es std::variant in einem wahlweise statischen oder runtime array."

Daraus die sieben bindenden Design-Saetze:

1. Die Hybrid-Tier-Binary ist eine **eigene Stufe HINTER der CEB** (kein CEB-Teil, kein plain Tier).
2. Sie traegt **mehrere ABI-stabile Pruef-Docks**.
3. Der Dock-Einsatz auf ihre Tier-Binaries laeuft als **Factory Pattern in Proxy-Verwendung**.
4. **std::variant ist dort die Ausnahme** -- in einem **wahlweise statischen ODER Runtime-Array**,
   weil die **Dock-Anzahl zahlenmaessig dynamisch** variieren koennen muss.
5. In den **plain Tier-Binaries bleibt std::variant verboten** (uneingeschraenkt).
6. Die **Zwischenloesung** statische Pruef-Docks <-> austauschbare plain Tier-Binaries je Pruefdock
   ist eine **XML-Konfiguration auf Anwender-Wunsch in der AUSWERTUNGSPHASE**.
7. Designplanung jetzt, **Bau in der Auswertungsphase**.

Bindend bleibt zusaetzlich die aeltere Owner-KORREKTUR §49 (LEDGER:2663): variant NUR als Traeger
abweichender Unter-Pruef-Dock-Typen/-Vertraege, gelesen und verarbeitet per Abstract-Factory-Methode;
die **Haupt-Kommunikation zu den Tier-Binary-Observern bleibt statisch** (IObservableTier, zero-cost).

---

## 2. Einordnung in die rekursive Dock-Kette (Rekursions-Ebene 3)

Die Kette wird um GENAU EINE Rekursions-Ebene nach dem bestehenden Muster verlaengert
(LEDGER:490 + LEDGER:1692-1698):

| Ebene | Dock | Richtung | Stand |
|---|---|---|---|
| 1 | Planer-Dock (Planer -> CEB) | eindimensional | im Code noch nicht als benanntes Dock materialisiert (LEDGER:1698); Live-Kette laeuft ueber emittierten CMake-/YAML-Text, POD `experiment_dock_payload.hpp:58` ist DEPRECATED-Referenz |
| 2 | CEB-Pruef-Dock (CEB <-> Tier-Binary) | bidirektional, ABI-7 | gebaut: `IPruefDock` + `PruefDockRegistry` + `SearchAlgorithmDock` + Sequencer |
| 3 | **Hybrid-Pruef-Docks (Hybrid-Tier-Binary <-> plain Tier-Binaries)** | bidirektional | **NEU, dieses Design**; N Docks statt einem, N dynamisch |

Nach OBEN ist die Hybrid-Tier-Binary selbst ein gewoehnliches Tier-Modul am CEB-Pruef-Dock: sie
exportiert die 4 Pflicht-Symbole der Anatomy-ABI (`anatomy_module_abi_v1_decl.hpp:62` Major 7, -- ABI-HISTORIE gegen SHA 6b8eee0f
`:66` Magic `.A7.`) und liefert `IAnatomyBase` + `IObservableTier` als virtuelles ganzes -- ABI-HISTORIE gegen SHA 6b8eee0f
Tier-Binary (SearchAlgorithm-Huelle). Nach UNTEN spricht jedes ihrer Docks seine plain Tier-Binary
ueber EXAKT dieselbe Anatomy-ABI (`AnatomyModuleLoader`-Wiederverwendung).

> **NACHTRAG 08.08.2026 (Paket HY-0) -- die beiden Zahlen im Absatz darueber sind Historie.**
> Seit E-24 C8 (04.08.2026, ce `4f569051`) gilt:
>
> - **Major 8** -- Beleg `anatomy_module_abi_v1_decl.hpp:89`
> - **Magic `.A8.`** = `0x434F4D444141382EULL` -- Beleg `anatomy_module_abi_v1_decl.hpp:93`
>
> Die Aussage des Absatzes -- kein eigener ABI-Schritt fuer die Hybrid-Stufe, beide Grenzen sind
> die bestehende versionierte Anatomy-ABI -- ist davon UNBERUEHRT; nur die Zahl und die
> Zeilen-Anker sind gewandert. Die alten Angaben bleiben als verifizierte Historie stehen (gegen
> SHA `6b8eee0f`, den in Abschnitt 11 selbst genannten Basisstand, zeigen `:62` und `:66` wirklich
> auf die beiden `#define`-Zeilen der damaligen ABI). Erzwungen wird das
> Nachziehen ab jetzt von `scripts/ci_hy_label_gate.sh` (ctest: `hy_label_gate`), die den lebenden
> Major maschinell aus dem Decl-Header liest und jeden Anker gegen Header bzw. SHA-Objekt nachrechnet.

**Folge (wichtigster Freihalte-Entscheid): die Hybrid-Stufe braucht KEINEN eigenen ABI-Schritt.**
Beide Grenzen sind die bestehende versionierte ABI. Siehe Abschnitt 6.

---

## 3. Architektur: Header-Schnitt, Ownership, Lebenszyklus

Neues Stufen-Verzeichnis mit Namespace `comdare::cache_engine::hybrid`. Verzeichnis JETZT nur als
Stub reserviert (`ce libs/cache_engine/hybrid/README.md`), Dateien entstehen in der
Auswertungsphase:

```
ce libs/cache_engine/hybrid/
  hybrid_dock_contract.hpp   // DockContractDescriptor (POD) + HybridDockContract-Concept
                             // + constexpr contract-Registry (Muster: run_methodology_registry.hpp)
  hybrid_pruef_dock.hpp      // StandardHybridDock + je abweichendem Vertrag ein Alternativ-Dock-Typ
                             // (Rollback / Scan / ResourceControl)
  hybrid_dock_factory.hpp    // Abstract Factory (§49-KORREKTUR): Descriptor -> emplaced variant-
                             // Alternative; EINZIGER Konstruktions-Ort der Alternativen
  hybrid_dock_array.hpp      // DockArray<Policy>: statisch ODER runtime (Owner-Wahlfreiheit)
  hybrid_binary_proxy.hpp    // HybridBinaryProxy: Factory-Proxy je Dock auf seine plain Tier-Binary
                             // (besitzt AnatomyModuleHandle)
  hybrid_eviction.hpp        // Verdraengungs-Strategie (CT-Strategy, §49 Dock-Array-Verdraengung)
  hybrid_router.hpp          // Break-Even-Router (speist sich aus heuristik/break_even.hpp
                             // + measurement_curve_loader.hpp)
  hybrid_config_xml.hpp      // Parser der <hybrid_tier>-XML-Sektion (common-DOM xml_reader;
                             // Doktrin NUR EIN XML-Programm)
  hybrid_tier_module.cpp     // die Hybrid-Tier-Binary: COMDARE_DEFINE_ANATOMY_MODULE-Export
                             // der SearchAlgorithm-Huelle
```

### 3.1 Signaturen-Skizze (Kern, unverbindlich im Detail, bindend in der Struktur)

```cpp
namespace comdare::cache_engine::hybrid {

// (a) Vertrags-Deskriptor -- aus XML gelesen, von der Abstract Factory verarbeitet
//     (POD, trivially_copyable).
struct DockContractDescriptor {
    std::uint8_t contract_id;      // Index in die constexpr contract-Registry
                                   // ("standard"/"rollback"/"scan"/"resource_control")
    anatomy::AnatomyGenus genus;   // Gattung der andockbaren plain Tiers
    // + Vertrags-Parameter (z.B. two_phase, range_scan) -- additiv erweiterbar
};

// (b) DIE eine erlaubte variant-Stelle der Gesamt-Architektur ausserhalb der CEB
//     (Owner-E1 + §49-KORREKTUR):
using HybridDockVariant = std::variant<StandardHybridDock, RollbackContractDock,
                                       ScanContractDock, ResourceControlDock
                                       /* additiv je neuem Vertrag */>;

struct DockSlot {
    HybridDockVariant       dock;   // variant NUR hier: Traeger der Dock-Typ-/Vertrags-Varianz
    HybridBinaryProxy       proxy;  // KEIN variant: RAII-Besitz der plain Tier-Binary dieses Docks
    DockContractDescriptor  desc;
};

// (c) Dock-Array wahlweise statisch/runtime (Owner: "wahlweise statisches oder runtime array"):
template <std::size_t MaxN> struct StaticDockArrayPolicy {
    using storage = std::array<std::optional<DockSlot>, MaxN>;
};
struct RuntimeDockArrayPolicy { using storage = std::vector<DockSlot>; };

template <class Policy> class DockArray {          // Policy per XML gewaehlt; Belegung IMMER dynamisch
    [[nodiscard]] int attach(DockContractDescriptor const&,
                             std::filesystem::path const& so);   // Factory + Loader, errno-Stil
    void detach(std::size_t slot) noexcept;                      // destroy_anatomy -> dlclose
    template <class EvictionStrategy>
    std::size_t evict_for(EvictionStrategy&, std::size_t bytes_needed);
    [[nodiscard]] std::size_t size() const noexcept;             // dynamisch in BEIDEN Policies
                                                                 // (statisch = nur CT-KAPAZITAET)
};

// (d) Factory-Proxy (GoF Proxy + Factory Method; Owner: "Proxy Verwendung ihrer Tier-Binaries
//     als Factory Pattern"):
class HybridBinaryProxy {
    [[nodiscard]] int acquire(std::filesystem::path const& so);  // AnatomyModuleLoader::load
                                                                 // + EINMALIGES Drive-Buendel-Probing
    void release() noexcept;                                     // destroy-vor-dlclose-Ordnung
    [[nodiscard]] anatomy::IObservableTier* obs() const noexcept; // gecachter statischer Antrieb --
                                                                  // Hot-Path variant- und cast-frei
private:
    anatomy_loader::AnatomyModuleHandle handle_;      // RAII, move-only
    builder::pruef_dock::SearchAlgorithmDrive drive_; // dynamic_cast NUR bei acquire
};

// (e) Abstract Factory (§49-KORREKTUR woertlich: "per Abstract-Factory-Methode gelesen
//     und verarbeitet"):
class HybridDockFactory {
    [[nodiscard]] static int make_dock(DockContractDescriptor const&, HybridDockVariant& out);
};

} // namespace comdare::cache_engine::hybrid
```

### 3.2 std::variant -- Erlaubnis- und Verbots-Kante (exakt)

1. **ERLAUBT:** genau `HybridDockVariant` innerhalb `DockSlot` im Dock-Array der Hybrid-Tier-Binary
   -- der Traeger dafuer, dass N ABI-stabile Pruef-Docks ZAHLENMAESSIG dynamisch variieren und
   TYPLICH abweichende Vertraege tragen koennen (Owner-Wortlaut: "genau dafuer braucht es
   std::variant").
2. `std::visit` NUR zu Konfigurations-/Umschalt-Zeitpunkten (attach/detach/Break-Even-Umschaltpunkt
   des Routers). Nach dem visit wird der monomorphisierte Antrieb (`IObservableTier*` +
   Drive-Buendel) im Slot GECACHED; der Op-Hot-Path ist variant-frei und cast-frei
   (§49-KORREKTUR: Haupt-Kommunikation = statisches IObservableTier, zero-cost, §9-Doktrin).
3. **VERBOTEN bleibt variant:** in jeder plain Tier-Binary (uneingeschraenkt), in der
   Haupt-Observer-Kommunikation, als Achsen-Traeger (V7.2 "variadisches Array, KEIN std::variant"
   fuer Meta-Metas bleibt unberuehrt) und im Planer.
4. In der CEB bleibt variant GEDULDET (LEDGER:3489, unveraendert).

### 3.3 Ownership / Lebenszyklus (rekursives RAII)

- Die CEB besitzt den vollen Lebenszyklus der Hybrid-Binary: Konfiguration + Compile + Messung am
  CEB-Pruef-Dock (`pruef_dock.hpp:4-9`-Doktrin unveraendert).
- In der Hybrid-Binary: die per `comdare_create_anatomy` erzeugte Instanz besitzt das `DockArray`
  -> `DockSlot` besitzt `HybridBinaryProxy` -> Proxy besitzt `AnatomyModuleHandle`.
- Zerstoerungs-Reihenfolge: Router-Quiesce -> DockArray-clear (je Slot `destroy_anatomy` ->
  `dlclose`, Ordnung nach `anatomy_module_loader.hpp:70-75`) -> Hybrid-Instanz-destroy -> die CEB
  dlclosed die Hybrid-.so.
- Kein Slot wird waehrend einer laufenden Operation verdraengt: Quiesce-Punkt am Op-Rand
  (Arbeitsmodus-Hot-Switch an der bewussten ABI-Grenze, LEDGER:187(e)).
- **Delegierter Bau (LEDGER:2230):** die Hybrid-Stufe kompiliert NIE selbst; sie fordert plain
  Tier-Binaries per Anforderungs-Manifest (binary_id-Liste) bei der CEB an; die CEB baut oder holt
  aus dem Lager und liefert .so-Pfade zurueck.

### 3.4 Break-Even-Router (§32-F8 / §49)

`hybrid_router.hpp` konsumiert AxisSpline-Kurven (`heuristik/measurement_curve_loader.hpp`) und
BreakEvenPoint-Listen (`heuristik/break_even.hpp`) und bildet daraus die
Compile-Time-Chain-of-Responsibility der Heuristik-Ebene (LEDGER:490 Form (a): "noch eine Ebene NACH
dem Command-Pattern in einer compile-time Chain-of-Responsibilities").

§49-Option 1 (heuristik-erkannte Algorithmen DERSELBEN Achse direkt einkompiliert) bleibt eine
BAU-Option der Hybrid-Binary selbst: die CEB kompiliert die Kandidaten-Algorithmen statisch hinein,
die Auswahl laeuft ueber die CT-CoR -- KEIN variant dafuer, weil die §49-KORREKTUR variant fuer
Break-Even-Algo-Swapping der Messkommunikation ausschliesst. §49-Option 3 ist das Dock-Array +
Eviction dieses Designs.

**Wiederverwendungs-Kandidaten fuer den Router (Review-Auflage K3):** das CoR-Substrat im Repo ist
groesser als urspruenglich kartiert. Vor einem Neubau sind zu pruefen:
`builder/decision_lambda_trees/`, `builder/experiment_tree/selection_filter_chain.hpp` (GoF-CoR),
`include/cache_engine/measurement/ram_probe_chain.hpp` und
`include/cache_engine/measurement/axis_error.hpp`.

---

## 4. XML-Konfiguration (Auswertungsphase) und die Kommentar-Reserve JETZT

Owner-E1: die Zwischenloesung ist eine **XML-Konfiguration auf Anwender-Wunsch in der
Auswertungsphase**. Der Ziel-Entwurf (additives OPTIONALES Element in der
`comdare_experiment`-Root-Sequenz, `minOccurs=0`, damit alle Bestands-XMLs valide bleiben und das
Schema byte-neutral ist):

```xml
<hybrid_tier enabled="true" genus="SearchAlgorithm">
  <dock_array storage="static|runtime" max_docks="8"/>
  <eviction strategy="lru|lfu|arc|cost_aware" ram_budget_mb="4096"/>
  <docks>
    <dock id="d0" contract="standard">
      <tier_binary select="binary_id" value="..."/>
    </dock>
    <dock id="d1" contract="rollback">
      <tier_binary select="heuristic"/>
    </dock>
  </docks>
  <router><break_even source="measurement_store" curves="axis_splines"/></router>
</hybrid_tier>
```

Die Owner-Zwischenloesung bildet sich exakt ab: `<dock contract=...>` ist die STATISCHE Seite (der
Dock-Typ je Slot steht nach der Konfiguration fest), `<tier_binary select=...>` ist die
AUSTAUSCHBARE Seite (plain Binary je Pruefdock swappbar unter Eviction). `storage="static"` setzt
nur die COMPILE-TIME-KAPAZITAET; die Belegung bleibt in beiden Policies dynamisch.

Validierung additiv in `profile_facade/validate_profile.hpp`; contract- und strategy-Token kommen
NUR aus constexpr-Registries (Registry-ist-ANGEBOT-Doktrin, kein Runtime-Switch).

**JETZT wird ausschliesslich die XSD-KOMMENTAR-RESERVE eingetragen** -- nach dem im Repo etablierten
COMPARE-Muster: der Modus `compare` lebte zuerst nur als XSD-Kommentar-Reserve
(`experiment_schema.xsd:60-61`) und wurde erst spaeter Typ
(`include/cache_engine/measurement/run_methodology_registry.hpp:7-10` dokumentiert genau diesen
Weg). Der XSD-Typ `HybridTierType` kommt erst mit Paket HY-B3.

**Wichtig zur Zustaendigkeit:** `Code/test_data_xml/experiment_schema.xsd` liegt im SUPER-Repo und
wird von diesem Paket NICHT editiert. Der Reserve-Text ist als Lead-Nachtrag entworfen und an den
Lead uebergeben (Paket-Rueckgabe HY-D2); reserviert ist der Platz in der Root-Sequenz nach
`<xs:element name="run_methodology" .../>` (heute `:62`) und vor
`<xs:element name="measurement_framework" .../>` (heute `:63`). XSD-Kommentar-Falle beachten: kein
doppelter Bindestrich im Kommentar-Text.

---

## 5. Abgrenzung CEB gegen Hybrid-Stufe

**Die CEB BLEIBT zustaendig fuer:** Kompilation + System + Scheduling der Binary-Basics (V7.2); den
Bau ALLER Binaries inklusive der Hybrid-Binary und der delegiert angeforderten plain Tiers;
Experiment-XML- und Registry-Parsing; `PruefDockRegistry` (EIN Dock je Gattung) + Sequencer +
Konformitaets-Gate; Messung + CSV-Rueckschrieb. Die Hybrid-Binary wird am CEB-Pruef-Dock gemessen
wie jedes andere Tier (§64-Gemischt-Abnahme unveraendert). variant bleibt in der CEB geduldet.

**Die HYBRID-STUFE WIRD zustaendig fuer:** das DockArray (N Docks, dynamisch) + Factory-Proxies +
Break-Even-Router + Eviction; das Delegieren von Bau-Wuenschen an die CEB; die Aggregation der
Sub-Tier-Observer-Snapshots hybrid-intern zu ihrem EINEN nach oben gemeldeten POD. Sie schreibt NIE
selbst CSV -- der Ergebnis-Rueckschrieb bleibt CEB-Aufgabe nach Planer-Bestimmung.

---

## 6. JETZT freizuhaltende Schnittstellen (kein Rebuild der Voll-Bau-Binaries)

1. **ABI-Nulllast (hart):** Die Hybrid-Stufe fordert KEINEN neuen vtable-Slot, KEIN POD-Feld, KEINE
   ABI-Aenderung. Alles Benoetigte (tier_insert/lookup/erase/observe/reset, memento_all, scan,
   resource-control) existiert als Sub-Interface der ABI-7-Flaeche (`observable_tier.hpp:185/:193`,
   `search_algorithm_dock.hpp:65-86`).
2. **E-24-Sequenz:** Die Container-Gattung ist der LETZTE ABI-Schritt und MUSS VOR dem
   Voll-Bau-4-Trigger in der Gate-Reihenfolge liegen (Verankerung in A16). Die Hybrid-Stufe erbt
   dann den finalen Major und laedt die Voll-Bau-Binaries ohne Neubau. Es wird KEIN eigener
   Hybrid-ABI-Schritt eingeplant.
3. **XSD-Kommentar-Reserve `<hybrid_tier>`** jetzt eintragen (Lead, super-Repo): kein Parser-Zwang,
   aber der Platz in der Root-Sequenz ist benannt (Abschnitt 4).
4. **A13-Stempel-Schnittstelle:** Die Hybrid-Identitaet ist die normale Tier-Stempel-Form
   (`[[System],[Organ]]` + SHA512-Zeile) der Hybrid-Binary. Die Dock-Bestueckung ist
   RUNTIME-Konfiguration und gehoert in ein Sidecar-Manifest, NIE in die binary_id (golden-neutral).
   Von A13 wird nur gebraucht, dass die 'e'-Versions-Grammatik generisch je Achsen-Algorithmus gilt
   (ist per Owner-E2 gegeben). **Das Sidecar-Format darf erst NACH gelandetem A13 fixiert werden**,
   sonst traegt es die defekte 5-Zeilen-Identitaet weiter.
5. **Lager-Strecken-Schnittstelle (nur benannt, nicht mitgeplant):** Hybrid konsumiert (a) den
   Binaries-Realm (Lookup binary_id -> .so-Pfad + SHA512-Identitaet) und (b) den Messdaten-Realm
   (CSV-/Kurven-Lookup je binary_id fuer den measurement_curve_loader). Beides ist exakt die
   Bestandslog-/Baum-API der laufenden Lager-Strecke; Hybrid-Sonderknoten sind nicht noetig.
6. **heuristik/-API-Freeze:** `heuristik/axis_spline.hpp`, `heuristik/break_even.hpp`,
   `heuristik/measurement_curve_loader.hpp` im §75-Abschluss-Aufraeumpass NICHT umbenennen und NICHT
   entfernen -- sie speisen den Router. Eintrag in die §75-Kandidatenliste als NICHT-Kandidaten (Lead, super-Repo -- Text unten, Vollzug beim Lead).
7. **`IPruefDock::measure`-Vertrag stabil halten** (`pruef_dock.hpp:78-79`): die Hybrid-Binary haengt
   als normales Modul daran. Ebenfalls NICHT-Kandidat des §75-Passes (Lead, super-Repo).
8. **Verzeichnis-/Namespace-Reservierung** `ce libs/cache_engine/hybrid/` (dieses Paket, Stub-README),
   damit kein anderes Paket den Namen belegt.

---

## 7. Offene Abschnitte aus der adversarialen Review (K1-K6) -- BINDEND vor dem Bau

Die sechs Auflagen der Design-Review sind bewusst NICHT stillschweigend geschlossen worden. Sie sind
die Tagesordnung fuer die Auswertungsphase; K1 und K4 beruehren zusaetzlich laufende Strecken
(A13-Stempel, Lager) und muessen VOR dem jeweiligen Freeze beantwortet sein.

### K1 (offen, VOR Lager-/A13-Freeze): Lager-Identitaet der Hybrid-Binary selbst

Dieses Design regelt die Dock-Bestueckung (Sidecar, nie binary_id -- korrekt golden-neutral), aber
die Hybrid-.so hat KEINE Organ-Permutations-binary_id und damit keinen definierten Slot im
Binaries-Realm-Baum. Zu entscheiden ist genau eine der beiden Formen:

- **(a) eigene Einlagerungsform:** die Hybrid-.so bekommt einen eigenen Identitaets-Schluessel im
  Binaries-Realm (Vorschlag: Stempel-SHA512 statt Organ-binary_id, plus Sidecar-Manifest der
  Dock-Bestueckung); oder
- **(b) keine Einlagerung:** die Hybrid-.so ist ein reines Auswertungs-Artefakt der CEB und wird
  nicht eingelagert.

Solange das offen ist, darf kein Lager-Schema Hybrid-Knoten annehmen. Owner-/A13-Entscheid noetig.

### K2 (offen, Entscheid vor HY-B1): Schichten-Zuordnung von Loader und Drive-Buendel

`HybridBinaryProxy` haelt `anatomy_loader::AnatomyModuleHandle` und
`builder::pruef_dock::SearchAlgorithmDrive`. `AnatomyModuleLoader::load/load_all`
(`anatomy_module_loader.hpp:152/:156`) sind in der Builder-Lib definiert -- die Hybrid-Tier-.so
muesste also Builder-Code linken. Das kollidiert mit der stehenden Doktrin-Zeile
`pruef_dock.hpp:10-14` ("IPruefDock lebt nur im Builder-Binary") und mit der
CEB-superset-Tier-Schichtung (LEDGER 18.1.2). Zu entscheiden:

- **(a)** Loader- und Drive-Buendel-Typen in eine **stufen-neutrale Lib** extrahieren
  (Vorzug, weil es die Doktrin-Zeile unberuehrt laesst); oder
- **(b)** die Hybrid-Stufe linkt bewusst die Builder-Loader-Lib, mit **additiver Praezisierung** der
  Doktrin-Kommentare (`pruef_dock.hpp:10-14`).

Ohne diesen Schritt ist "AnatomyModuleLoader-Wiederverwendung" kein vollstaendiger Migrationsplan.

### K3 (erledigt in diesem Dokument): CoR-Substrat vollstaendig kartiert

Siehe Abschnitt 3.4: das Substrat ist `decision_lambda_trees` PLUS
`experiment_tree/selection_filter_chain.hpp`, `measurement/ram_probe_chain.hpp` und
`measurement/axis_error.hpp`. `selection_filter_chain.hpp` ist als Wiederverwendungs-Kandidat fuer
den HY-B2-Router zu pruefen, bevor eine neue Kette entsteht.

### K4 (ENTSCHIEDEN, Owner-GO Q6 vom 02.08.2026 ~17:4x): Verhaeltnis zum Framing "Heuristik-Optimierung als eigene Systemachse"

LEDGER:187(e) beschreibt das CEB-Ziel als 4 Modi und nennt den Hybrid-Modus dort
"Heuristik-Optimierungs-Achse als **eigene Systemachse**". Owner-E1 definiert die Hybrid-Natur nun
als **eigene STUFE hinter der CEB**. Unter V7.2 (genau drei System-Glieder) ist beides nicht
gleichzeitig ohne Klaerung haltbar. **ENTSCHIEDEN:** Der Owner hat am 02.08.2026 ~17:4x den
Q6-Empfehlungs-Default verbatim freigegeben ("Q6 Ein-Gattung-Hybrid + MaxN=8 + LEDGER:187(e)
superseded", persistiert in `docs/sessions/20260802-OWNER-entscheide-hybrid-tier-stempel-regression-os-unterachsen.md`).
Damit gilt: Owner-E1 ist die juengere und speziellere Aussage; die Hybrid-Natur ist eine STUFE, keine
Systemachse; das Systemachsen-Framing aus LEDGER:187(e) ist **superseded** (kein offener Regelkonflikt
mehr; Codex-B8/CX-W7-Schliessung 02.08. abends). Der Ledger-Supersede-Vermerk zu 187(e) folgt im
gebuendelten Nachzugs-Paket.

### K5 (offen, benannter Entscheid in HY-B4): Semantik der Snapshot-Aggregation

Die Hybrid-Binary meldet nach oben EINEN `ComdareTierObserverSnapshot` mit festem
`axis_stats`-Layout, kann aber Docks mit Tiers unterschiedlicher Organ-Komposition tragen. Damit ist
die Aggregation NICHT trivial. Zu entscheiden und mit einem Konformitaets-Test zu belegen:

- **(a) Summe ueber alle Docks** (verliert die Provenienz je Sub-Tier) oder
- **(b) Passthrough der aktiven Binary** (verliert die Beitraege der uebrigen Docks),
  jeweils mit Provenienz-Vermerk im Sidecar.

Bis zum Entscheid gilt keine Variante als implizit gesetzt.

### K6 (bindend fuer die Paketreihenfolge): F8-Minimal-DoD zuerst

§32-F8 (LEDGER:2265-2270) macht den Hybrid-DoD (minimale Baseline + Spline-Heuristik) zur
ABGABE-PFLICHT. Die Paketreihenfolge in Abschnitt 8 fuehrt deshalb einen **HY-B1-Minimalschnitt**
(genau 1 Standard-Dock, Standard-Vertrag, delegierter Bau, ctest-bewiesen, ohne Eviction und ohne
XML) als ERSTEN Meilenstein der Auswertungsphase -- vor Router-, Eviction- und XML-Vollausbau. Damit
ist die Abgabe-Pflicht auch bei schrumpfendem Zeitfenster gedeckt.

---

## 8. Paketreihenfolge und Migrationsschritte

**JETZT (vor Trigger; golden-neutral, byte-neutral bis auf Doku und XSD-Kommentar):**

| Paket | Inhalt | Status |
|---|---|---|
| HY-D1 | V7.2-Praezisierung + E-11-Aufloesung als additiver Ledger-Vermerk (Lead-only, super-Repo) + K4-Einordnung | Text entworfen, Lead vollzieht |
| HY-D2 | dieses Architektur-Doc + Verzeichnis-Stub `libs/cache_engine/hybrid/` | dieses Paket (geliefert) |
| HY-D2/Lead | XSD-Kommentar-Reserve-Text + Freeze-Vermerke (§75-NICHT-Kandidaten, E-24-Sequenz) | Text entworfen, Lead vollzieht (super-Repo) |

**AUSWERTUNGSPHASE (nach Voll-Bau-4 + Voll-Messung, nach E-24):**

| Paket | Inhalt | Groesse |
|---|---|---|
| HY-B1a | **F8-Minimal-DoD (K6):** 1 Standard-Dock, Standard-Vertrag, delegierter Bau, ctest-Beweis | S/M |
| HY-B1b | Dock-Array + Abstract Factory + Binary-Proxy voll (Static-/Runtime-Policy, Attach-Zeit-Monomorphisierung, destroy-vor-dlclose, Hot-Path-variant-frei-Nachweis als Konformitaets-Test) | L |
| HY-B2 | Break-Even-Router (CT-CoR) + Verdraengungs-Strategie (CT-Strategy; Shortlist LRU/LFU/ARC/cost-aware nach der §49-beauftragten Web-Recherche) + Anschluss `measurement_curve_loader` an den Messdaten-Realm | M |
| HY-B3 | XML scharf: XSD-Typ `HybridTierType` aus der Kommentar-Reserve heben, `hybrid_config_xml.hpp` (common-DOM), validate_profile-Naht, Fixture-Erweiterung (TABUs unberuehrt) | M |
| HY-B4 | Hybrid-als-Tier-Export (`COMDARE_DEFINE_ANATOMY_MODULE`), Snapshot-Aggregation nach K5-Entscheid, Messung der Hybrid-Binary am CEB-Pruef-Dock (§64 gemischt), delegierter Bau-Kanal, Release-Schritt: Mess-Observer per Metaprogrammierung abschaltbar (LEDGER:3250) + Wallclock-Beweis | L |

Jeder Schritt mit der stehenden Kadenz: ultracode-Planung -> Implementierung -> adversariale
Pruefung -> CI gruen.

---

## 9. Risiken (bewusst benannt, mit Gegenmassnahme)

| Risiko | Gegenmassnahme |
|---|---|
| variant-Leak in den Hot-Path (visit pro Operation statt pro Umschaltpunkt) -- verletzt die §49-KORREKTUR | Attach-Zeit-Monomorphisierung + gecachter `IObservableTier*`; Konformitaets-Test in HY-B1 PFLICHT |
| ABI-Major-Drift: bumpt E-24 den Major NACH dem Voll-Bau-4, kann eine neu gebaute Hybrid-Stufe die eingelagerten plain Binaries nicht laden (Major-Check des Loaders) -> Neubau aller Binaries | E-24 MUSS letzter ABI-Schritt VOR dem Voll-Bau-4-Trigger sein (Abschnitt 6.2, Verankerung in A16) |
| Verschachteltes dlopen (die Hybrid-.so laedt selbst N Sub-.so) | legal, aber Entlade-Reihenfolge, RTLD-Flags und static-TLS-Budget in HY-B1 auf beiden Prod-Maschinen empirisch belegen (kuenftig auch macOS .dylib und RISC-V), nicht annehmen |
| A13-Stempel-Regression aendert das Stempel-Layout (merge-Zeile faellt, Meta-Meta ans Realm-Ende) | Sidecar-Manifest-Format erst NACH gelandetem A13 fixieren (Abschnitt 6.4) |
| Eviction gegen laufende Operationen (UB an der ABI-Grenze) | Quiesce-Punkt-Design (Umschalten nur am Op-Rand) im Router hart durchsetzen und testen |
| Lager-API-Drift (die parallele Lager-Strecke definiert binary_id -> Pfad und CSV-Lookup) | Schnittstelle ist benannt (Abschnitt 6.5), aber nicht vertraglich eingefroren -- Anpassungsschleife fuer HY-B2 einplanen |
| Gattungs-Erweiterung: nach E-24 koennten Container-Gattungs-Hybride kommen | Design ist auf "ein Hybrid je Gattung" angelegt; je Gattung eine eigene Huelle (Mehraufwand, kein Umbau). Owner-Bestaetigung offen (Abschnitt 10) |

---

## 10. Owner-Fragen

**Owner-GO Q6 (02.08.2026 ~17:4x, verbatim) hat Punkte 1 und 2 ENTSCHIEDEN** (siehe K4):

1. **Gattungs-Kardinalitaet -- ENTSCHIEDEN: EIN Hybrid-Binary je Gattung** ("Ein-Gattung-Hybrid",
   gestuetzt auf die Doktrin "uebernimmt per Metaprogrammierung DIE Gattung ihrer Tier-Binaries").
   Gemischte Gattungen in einer Huelle sind damit ausgeschlossen; je Gattung eine eigene Huelle.
2. **CT-Kapazitaet der statischen Dock-Array-Policy -- ENTSCHIEDEN: Default-MaxN = 8** (per XML
   `max_docks` als Compile-Define beim delegierten Hybrid-Bau ueberschreibbar).
3. **Stempel-Detail (OFFEN, Owner-Vorlage V4):** bekommt die Hybrid-Binary im System-Stempel-Array ein
   eigenes Kennzeichen (hybrid-Flag/Version) oder bleibt die Hybrid-Natur ausschliesslich im
   Sidecar-Manifest? -- NICHT Teil von Q6, weiterhin offen; im gebuendelten Owner-Vorlagen-Block (V4).
   (klein, nach A13 entscheidbar; haengt an K1)
4. **Verdraengungs-Heuristik-Shortlist:** die §49-beauftragte Web-Recherche der Speicher-Heuristiken
   (LRU/LFU/ARC/GDSF/cost-aware) ist offen -- Zeitpunkt Auswertungsphase (HY-B2) bestaetigen oder
   vorziehen?
5. **Plattform-Querschnitt (Owner-E4: RISC-V, macOS M1, macOS x86):** `platform_suffix()` des
   Loaders (`anatomy_module_loader.hpp:159`) kennt `.dylib` bereits -- reicht die bestehende
   Loader-Flaeche, oder soll die Hybrid-Stufe explizit in die neue Plattform-Matrix (E-19-Soll)
   aufgenommen werden?
6. **K4 (siehe Abschnitt 7):** ist das Framing LEDGER:187(e) "Heuristik-Optimierung als eigene
   Systemachse" durch das E1-Stufen-Modell superseded?

---

## 11. Verifizierte Ist-Anker (Stand development `6b8eee0f`)

| Anker | Datei:Zeile | Inhalt |
|---|---|---|
| Owner-Spec | super `docs/sessions/20260802-OWNER-entscheide-hybrid-tier-stempel-regression-os-unterachsen.md:7` | Entscheid 1 verbatim |
| ABI-Major | `libs/cache_engine/include/cache_engine/abi/anatomy_module_abi_v1_decl.hpp:62` | `COMDARE_ANATOMY_ABI_MAJOR 7` -- ABI-HISTORIE gegen SHA 6b8eee0f |
| ABI-Magic | dieselbe Datei `:59` | Magic von `.A6.` auf `.A7.` bewegt -- ABI-HISTORIE gegen SHA 6b8eee0f |
| ABI-Major (lebend seit 04.08.2026) | `anatomy_module_abi_v1_decl.hpp:89` | Major 8 -- E-24 C8, ce `4f569051`, HY-0-Nachtrag oben |
| ABI-Magic (lebend seit 04.08.2026) | `anatomy_module_abi_v1_decl.hpp:93` | Magic `.A8.` = `0x434F4D444141382EULL` |
| Gattungs-Enum | `libs/cache_engine/anatomy/anatomy_base.hpp:78-84` | `AnatomyGenus` {SearchAlgorithm, Set, Sequence, Adapter, View} |
| statische Haupt-Kommunikation | `libs/cache_engine/anatomy/observable_tier.hpp:185/:193` | `class IObservableTier : public IDriveableTier` / `tier_observe(...)` |
| Dock-Doktrin | `libs/cache_engine/builder/pruef_dock/pruef_dock.hpp:10-14` | IPruefDock ist KEINE ABI-Grenze; ABI-Grenze = Antriebs-Sub-Interface + POD |
| Dock-Interface | dieselbe Datei `:57`, `:78` | `class IPruefDock`, `virtual int measure(...)` mit Konformitaets-Gate-Vertrag |
| Dock-Registry | `libs/cache_engine/builder/pruef_dock/pruef_dock_registry.hpp:22/:28` | EIN Dock je Gattung, `select_for` ueber die im Modul deklarierte Gattung |
| konkretes Dock | `libs/cache_engine/builder/pruef_dock/search_algorithm_dock.hpp:26/:65/:77` | `SearchAlgorithmDock`, `SearchAlgorithmDrive`, `acquire_search_algorithm_drive` (dynamic_cast EINMALIG) |
| Sequencer | `libs/cache_engine/builder/pruef_dock/pruef_dock_sequencer.hpp:47` | `measure_genus_sequential` |
| Loader | `libs/cache_engine/builder/anatomy_module_loader/anatomy_module_loader.hpp:70-75`, `:76`, `:139-159` | destroy-vor-dlclose-Ordnung, `AnatomyModuleHandle`, `AnatomyModuleLoader::load/load_all/platform_suffix` (7-Schritt-Validierung inkl. Major-Check) |
| Heuristik-Substrat | `libs/cache_engine/heuristik/{axis_spline,break_even,measurement_curve_loader}.hpp` | Router-Speisung; `break_even.hpp:3` Schnittpunkt-Finder zweier AxisSpline derselben Organ-Achse |
| CoR-Substrat (K3) | `builder/decision_lambda_trees/`, `builder/experiment_tree/selection_filter_chain.hpp`, `include/cache_engine/measurement/ram_probe_chain.hpp`, `include/cache_engine/measurement/axis_error.hpp` | vier reale Ketten, nicht eine |
| Reserve-Muster | super `Code/test_data_xml/experiment_schema.xsd:50-64` + `include/cache_engine/measurement/run_methodology_registry.hpp:7-10` | COMPARE lebte zuerst als XSD-Kommentar-Reserve, dann als Typ |
| Planer-Dock-POD | `libs/cache_engine/profile_facade/planner/experiment_dock_payload.hpp:58` | DEPRECATED; Muster fuer ein spaeteres Manifest-Wire-Format, keine Live-Abhaengigkeit |
| PRT-ART-Naht | `libs/cache_engine/anatomy/pruefling_merge.hpp:1-16` | PrueflingSlot + drei kompositionale Joins |
| Kein Hybrid-Code | Identifier-/Verzeichnis-Ebene: `hybrid_tier`/`HybridTier` = 0 Treffer (das nackte Wort "hybrid" trifft zusaetzlich unverwandte Kommentar-Stellen) | nur `common/platform/hybrid_core_pinning` (unverwandt: CPU-Kern-Pinning) und der synthetische Vorbau-Test `tests/unit/test_hybrid_spline_selector_scaffold.cpp` -- KEINE Hybrid-Tier-Stufe |
| Ledger-Anker | LEDGER `:89`, `:187(e)`, `:490`, `:1692-1698`, `:2230`, `:2265-2270`, `:2654-2663`, `:3250`, `:3489` | V7.2-variant-Satz, 4-Modi-Ziel, rekursive Dock-Doktrin, delegierte Bau-Rekursion, §32-F8, §49 + §49-KORREKTUR, Release-Observer-Abschaltung, CEB-Duldung |

**Anker-Korrektur gegenueber der Design-Review:** die Review-Notiz K7 verortete `AnatomyGenus` bei
`anatomy_base.hpp:80-86`; im Stand `6b8eee0f` liegt das Enum bei `:78-84` (grep-belegt). Dieses
Dokument fuehrt die verifizierte Zeile.

---

## 12. Was dieses Paket NICHT tut

- Kein C++-Code, keine CMake-Anbindung, keine XSD-Edits, keine Fixture-Aenderung.
- Keine Beruehrung der TABUs (`golden_fullpilot_320*`, `permutation_axes.xml`,
  `m3v2_study.profile.xml`, CRC-Anker).
- Kein Vorgriff auf A13 (Stempel-Layout) und keinen Vorgriff auf die Lager-Schema-Entscheidung
  (K1 bleibt offen).
- Byte-Neutralitaet: dieses Paket aendert ausschliesslich Markdown-Dateien; keine Binary-Identitaet,
  kein binary_id, kein Stempel und keine Messgroesse werden beruehrt.
