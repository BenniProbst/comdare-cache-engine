# hybrid/ -- Hybrid-Tier-Stufe (Klassifikations-Fundament gebaut, Reroute offen)

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
- Kein neuer ABI-Schritt: beide Grenzen der Stufe sind die bestehende Anatomy-ABI (Major 7,
  `anatomy_module_abi_v1_decl.hpp:62`).
- Die Dock-Bestueckung ist Runtime-Konfiguration und gehoert in ein Sidecar-Manifest, NIE in die
  binary_id. Das Sidecar-Format wird erst NACH der A13-Stempel-Regression fixiert.
- Offene Auflagen K1 (Lager-Identitaet der Hybrid-.so), K2 (Schichten-Entscheid Loader/Drive),
  K4 (Verhaeltnis zu LEDGER:187(e)) und K5 (Snapshot-Aggregations-Semantik) sind VOR dem jeweiligen
  Bauschritt zu klaeren -- Details im Design-Dokument Abschnitt 7.
- Erster Meilenstein der Auswertungsphase ist der F8-Minimal-DoD (genau 1 Standard-Dock,
  ctest-bewiesen), nicht der Vollausbau.
