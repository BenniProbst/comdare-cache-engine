#pragma once
// Inkrementeller Tier-Binary-Cache (Bauplan Section 2+4, 2026-07-18) -- compile-time
// {axis,variant -> algo_version}-Tabelle.
//
// Der Rebuild-/Neu-Mess-Selektor braucht je Binary eine deterministische Organ-Algorithmus-Signatur (algo_sig).
// Diese Tabelle liefert das Fundament: sie reflektiert die 17 KOMPOSITIONS-Achsen (kCompositionAxisNames-Reihenfolge,
// axis_path_serialization.hpp) aus DENSELBEN Registry-Enabled-Listen wie registry_to_axis_levels.hpp (axes26-Aliase,
// BR-1, REGISTRY-getrieben statt string-getrieben) in Tripel {axis, W::name(), W::algo_version}. compose_algo_signature
// (axis_path_serialization.hpp) schlaegt hier je (axis,value) die Version nach und baut die Flach-Signatur.
//
// **DIES IST DIE UNIVERSELLE COMPILE-ZEIT-DURCHSETZUNG (Bauplan Section 2 "Concept erzwingt version()"):** das
// mp_for_each greift W::algo_version je registrierter Variante ab -- fehlt einer Variante das Member, ist der
// Zugriff ill-formed und
// die Kompilation bricht MIT DEM TYP-NAMEN. Eine Organ-Variante ohne algo_version kann so nicht unbemerkt in eine
// Registry gelangen. Ergaenzend traegt JEDE der 17 Kompositions-StrategyBases (search_algo..queuing_q2) im CRTP-Ctor
// einen fokussierten `static_assert(requires { Derived::algo_version; })` (Bauplan Section 2, Exemplar
// axis_06_allocator_strategy_base.hpp) -- der Concept-Guard je Konstruktion. Bewusst NICHT an der gemeinsamen Wurzel
// topics::OrganAxis: die traegt auch die System-Achse ISA und die 4 Shape-Achsen, die KEIN algo_version fuehren
// (Organ- vs System-/Shape-Provenienz strikt getrennt) -- ein Wurzel-Guard wuerde die faelschlich brechen.
//
// System-Achsen (telemetry/isa/page_type/simd_extension/general_hardware) und die 4 Shape-Achsen bleiben AUSSEN: sie
// tragen KEINE algo_sig (Organ- vs System-Provenienz strikt getrennt; Shapes sind Default-OFF-Anhang). C++23, header-
// only; heap-schwer (inkludiert ALLE Kompositions-Registries via registry_to_axis_levels.hpp) -- nur dort einbinden,
// wo die Registries ohnehin praesent sind (Facade / dedizierter Test), NIE in einen schlanken TU.

#include "axis_path_serialization.hpp" // kCompositionAxisNames (Slot-Reihenfolge der algo_sig)
#include "registry_to_axis_levels.hpp" // axes26::T* Registry-Aliase (dieselbe Quelle wie build_all_axis_levels)

#include <cache_engine/abi/anatomy_version_stamp.hpp>     // A13-M2: abi::OrganMetaMetas (Organ-Meta-Meta-Single-Source)
#include <cache_engine/abi/meta_meta_stamp_suffix.hpp>    // A13-M2: Klammer-Anhang der Meta-Metas (Owner-Q1)
#include <cache_engine/abi/organ_meta_meta_selection.hpp> // E-10 S1 (FIX-1): ungewrappte Entry-Form je Variante
#include <cache_engine/measurement/algo_semver.hpp> // W12-A: algo_semver_string (X.Y.Z-Voll-Form, NUR fuer Stempel)
#include <topics/organ_axis_error_classes.hpp>      // E-24 C9 / FK-5: der Fehlerraum-Zwilling der Versions-Wache

// A13-M3/C3: das Sub-Achsen-Werteset-Segment (Bauplan Section 2) wird nicht mehr HIER gerendert, sondern consteval
// in abi/subaxis_valueset_segment.hpp -- dieselbe Zeichenfolge speist .algos-Sidecar UND Fingerprint-Preimage.
#include <cache_engine/abi/subaxis_valueset_segment.hpp> // abi::kSubAxisValuesetSegment (Single-Source)

#include <boost/mp11.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

namespace mp = boost::mp11;

/// Ein Tabellen-Eintrag: (Kompositions-Achsen-Name, Varianten-name(), algo_version).
struct AxisVariantVersion {
    std::string_view axis;    ///< kCompositionAxisNames-Slot (z.B. "search_algo")
    std::string      variant; ///< W::name() (z.B. "bst")
    /// W::algo_version in der FLAG-GRAMMATIK v2 ("X.Y.Z" + null bis n punkt-getrennte Flags, ce-eigen immer
    /// mit 'c' darunter). Der Bestand ist NICHT durchgehend gleich: alle 26 Strategien der Allokator-Achse
    /// gingen zunaechst auf "1.0.1.c" (1. Bump), 24 davon -- die mit eigener reallocate()-Implementierung --
    /// danach auf "1.0.2.c" (2. Bump, reallocate-Statistik-Korrektur); PmrResourceAllocator und
    /// VampirNfpAllocator implementieren kein reallocate() und blieben auf "1.0.1.c" stehen
    /// (axis_06_allocator_strategy_base.hpp, Abschnitt "A1-VERSIONS-BUMP"; gepinnt in
    /// test_a1_algo_version_pin_alloc_axis), der uebrige Bestand auf "1.0.0.c".
    std::string version;
    /// E-10 S1 (FIX-1, VERIFY-STEMPEL-IDENTITAET): die UNGEWRAPPTE Organ-Meta-Meta-Entry-Form dieser
    /// Variante ("disk_io=code@1.0.0.c"; "" = kein Traeger). BEWUSST ohne Klammern: 4b (Schritt 4)
    /// verkettert die Inner-Entries einer Comp mit ';' und wrappt GENAU EINMAL (Ein-Renderer-Doktrin;
    /// eine gewrappte Tabelle ergaebe im Mehrtraeger-Fall "[a=..];[b=..]" statt "[a=..;b=..]" -- exakt
    /// die O-8-Schritt-12-Falle). HEUTE vom compose-Pfad UNGENUTZT (S1 byte-neutral); EnabledTargets =
    /// {MemoryOnlyTarget} -> alle Eintraege "".
    std::string meta_meta_entries;
};

/// assert_version_grammar<W>() -- die GEMEINSAME Flag-Grammatik-Wache EINER registrierten Variante (Owner-Q3/E2
/// 02.08.2026). Der EINE Ort der vier CT-Pflichten, damit reflect_versions (Enabled-Emit) und
/// guard_all_registered_organ_versions (die VOLLE registrierte Population, CX-W6) nicht zweimal -- und
/// luecken-verschieden -- dasselbe pruefen. Liest AUSSCHLIESSLICH W::algo_version (das die Observable-Huellen
/// forwarden) und emittiert NICHTS -> reine CT-Wache, byte-neutral.
template <class W>
constexpr void assert_version_grammar() {
    // A1 (G2-4a, W12-A, 2026-07-23): Concept-Guard der X.Y.Z-Disziplin -- JEDE registrierte Organ-Variante MUSS eine
    // PARSBARE algo_version tragen. Eine Fehlform (Kurzform "1.2", Tippfehler, eine Klammer ohne Basis) parst zum
    // Sentinel und bricht hier compile-time MIT dem Typ-Namen. Ergaenzt den bestehenden requires-Existenz-Guard
    // (Datei-Kopf) um die WOHLGEFORMTHEIT -- unparsbare Versionen koennen nicht unbemerkt in eine Registry gelangen.
    // A13-M1-Auflage K-5: die Wache prueft das x/y/z-TRIPEL (is_sentinel), NICHT den Struct-Vergleich gegen
    // AlgoSemVer{} -- sonst koennte eine Flag-Sentinel-Form die Wache umgehen.
    // FLAG-GRAMMATIK v2 (Owner-KERN 07.08.2026): die zulaessige Form ist "X.Y.Z[.flag]*" OHNE 'v'-Praefix. Die
    // gesamte Alt-Schreibweise ("v1.0.0c") faellt damit ebenfalls in diese Wache -- sie ist jetzt Sentinel.
    static_assert(!::comdare::cache_engine::measurement::parse_algo_semver(W::algo_version).is_sentinel(),
                  "algo_version unparsbar (Sentinel): erlaubt ist NUR \"X.Y.Z\" mit null bis n punkt-getrennten "
                  "Flags und optionalen Komposit-Klammern (\"1.0.0.c{p.e}.x512{f}\") -- KEIN 'v'-Praefix, ein "
                  "Punkt VOR jedem Flag, hinter '{' nie ein fuehrender Punkt (Owner-KERN 07.08.2026), "
                  "A10-X.Y.Z-Disziplin");
    // WACHE ce_owned_version_is_wellformed -- sie prueft seit S2 (07.08.2026) mehrere Dinge, und der
    // Meldungstext nennt alle. Er nannte anfangs nur das erste; das Literal "1.0.0.c.x512{sse2}" brach
    // damit MIT DER FALSCHEN BEGRUENDUNG ("ohne CPU-Basis"), obwohl es 'c' sehr wohl trug. Eine Wache, die
    // richtig anschlaegt und falsch berichtet, schickt den naechsten auf die falsche Faehrte.
    //   (1) "wenn ein Flag da ist, dann ist 'c' darunter" (Owner-F-10). NICHT tautologisch -- eine
    //       Registry-Variante mit "1.0.0.g" bricht hier MIT dem Typ-Namen, waehrend die FLAGLOSE Form
    //       ("1.0.0") durchgeht (sie prueft nur ein VORHANDENES Flag). Den flaglosen Fall weist der
    //       ENFORCE-Zweig unten zurueck; diese Wache bleibt noetig (sie faengt g/f/n ohne 'c') und ist die
    //       einzige, die auch ohne das Define greift.
    //   (2) KATALOG: jedes Flag-Token existiert und steht unter SEINER Basis
    //       (measurement/flag_grammar_catalog.hpp). "1.0.0.c{quatsch}" und "1.0.0.c.x512{sse2}" brechen
    //       hier -- das erste, weil es das Token nicht gibt, das zweite, weil sse2 unter x128 gehoert.
    //   (3) VORAUSSETZUNGS-KETTE (S-3b, 13.08.2026): ein Knoten mit bekannter Kette hat sein
    //       Voraussetzungs-Element nicht in der Menge ("1.0.0.c.x512{vl}" ohne 'f'). Dieser Grund
    //       fehlte in der Meldung seit der S-3b-Einhaengung -- hier nachgefuehrt (#17-Randfund).
    //   (4) REDUNDANZ (G-2-Semantik #17, 17.08.2026): kein (token,eltern)-Element steht doppelt in der
    //       Flag-Menge. "1.0.0.c.c" und "1.0.0.c{p.p}" brechen hier -- die ORGAN-Kategorie zieht die
    //       Semantik damit ueber ihre VOLLE registrierte Population in T00..T17-Ordnung nach
    //       (KON13-03: "in Reihenfolge der Achsen-Nummerierung").
    static_assert(::comdare::cache_engine::measurement::ce_owned_version_is_wellformed(W::algo_version),
                  "algo_version nicht wohlgeformt -- VIER moegliche Gruende, alle compile-time geprueft: "
                  "(1) die Version traegt Flags, aber 'c' ist nicht darunter (g/f/n sind reserviert und "
                  "werden hier nicht produziert, Owner-F-10 07.08.2026); (2) ein Flag-Token steht NICHT im "
                  "Katalog oder nicht unter SEINER Basis (S2-Katalog-Wache, flag_grammar_catalog.hpp -- "
                  "z.B. 'x512{sse2}': sse2 gehoert unter x128); (3) ein Knoten mit bekannter "
                  "Voraussetzungs-Kette hat sein Voraussetzungs-Element nicht in der Menge (S-3b -- z.B. "
                  "'x512{vl}' ohne 'f'); (4) ein (token,eltern)-Element steht DOPPELT in der Flag-Menge "
                  "(Redundanz-Wache, G-2-Semantik #17 -- z.B. '1.0.0.c.c')");
#if COMDARE_VERSION_HW_FLAG_ENFORCE
    // SCHARFSCHALTUNG (Owner-Q3: "Wir produzieren nur CPU code", in der v2 als F-10 praezisiert: "ce-eigene
    // Achsen tragen mindestens 'c'"). Die Wachen-LOGIK selbst (ce_owned_version_satisfies_cpu_enforce) ist
    // immer kompiliert und in algo_semver.hpp CT-bewiesen; das Define schaltet nur ihre ANWENDUNG auf jede
    // Registry-Variante.
    // WAS HIER MIT DER v2 ENTFALLEN IST: der frueher mitgepruefte Term "!experimental". Er hat keinen
    // Gegenstand mehr -- 'e' bedeutet EFFICIENCY CORE und ist ein legitimes Flag. Die alte B12-Wache
    // "ce-Registry traegt NIE 'e'" ist nicht abgeschwaecht, sondern gegenstandslos; ihre Aufgabe uebernimmt
    // die Pflicht "traegt mindestens 'c'" (Owner-F-10 verbatim: "Der vorschlag trifft ins Schwarze").
    static_assert(::comdare::cache_engine::measurement::ce_owned_version_satisfies_cpu_enforce(W::algo_version),
                  "algo_version ohne CPU-Flag: im CPU-only-Scope MUSS jede Version 'c' unter ihren Flags "
                  "tragen (Owner-Q3 02.08.2026 / F-10 07.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist "
                  "scharf");
#endif
}

/// mp_for_each ueber eine Registry-Enabled-Liste -> {axis, W::name(), W::algo_version} je Variante. mp_identity
/// vermeidet das Default-Konstruieren der Wrapper (nur der Typ wird benoetigt -- analog reflect_names,
/// axis_reflect.hpp).
/// Der W::algo_version-Zugriff ist die harte Compile-Zeit-Durchsetzung (siehe Datei-Kopf); die Grammatik-Wache
/// steht seit CX-W6 in assert_version_grammar<W>() (geteilt mit der Voll-Registry-Wache).
template <class List>
inline void reflect_versions(std::string_view axis, std::vector<AxisVariantVersion>& out) {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([&](auto id) {
        using W = typename decltype(id)::type;
        assert_version_grammar<W>();
        out.push_back(AxisVariantVersion{axis, std::string{W::name()}, std::string{W::algo_version},
                                         ::comdare::cache_engine::abi::organ_meta_meta_entries_for_variant<W>()});
    });
}

/// CX-W6 (Codex-Doppelreview 02.08.2026): die 18 ORGAN-Haupt-Achsen als ROH-REGISTRIERTE (All*) Typ-Listen --
/// die VOLLE Population VOR dem is_enabled-Filter (registry_to_axis_levels.hpp, axes26_registered). reflect_versions
/// oben laeuft ueber die GEFILTERTEN Enabled*-Listen (die Tabelle traegt nur, was gebaut/gemessen wird -- die
/// compose_*-Bytes bleiben unberuehrt); die Flag-Grammatik-Wache aber gilt fuer JEDE registrierte Variante,
/// unabhaengig vom Enable-Schalter. Ein 'e'/falsches Flag an einer deaktivierten Variante (Bestandsfall:
/// Array256SearchAlgo, Default OFF) bliebe sonst bis zur spaeteren Aktivierung unentdeckt, und die A13-M3/C4-
/// ENFORCE-Migration haette es still ausgelassen. mp_size dieser Liste == 18 (kCompositionAxisNames) -- der Assert
/// bindet die Vollstaendigkeit an die Enabled-Seite.
using AllRegisteredOrganAxisLists = mp::mp_list<
    axes26_registered::R00_search_algo, axes26_registered::R01_cache_traversal, axes26_registered::R02_mapping,
    axes26_registered::R03_path_compression, axes26_registered::R04_node_type, axes26_registered::R05_memory_layout,
    axes26_registered::R06_allocator, axes26_registered::R07_prefetch, axes26_registered::R08_concurrency,
    axes26_registered::R09_serialization, axes26_registered::R11_value_handle,
    axes26_registered::R13_index_organization, axes26_registered::R14_io_dispatch,
    axes26_registered::R15_migration_policy, axes26_registered::R16_filter, axes26_registered::R20_queuing_q1,
    axes26_registered::R21_queuing_q2, axes26_registered::R26_persistence_target>;
static_assert(mp::mp_size<AllRegisteredOrganAxisLists>::value == 18,
              "CX-W6: die Voll-Registry-Wache muss GENAU die 18 kCompositionAxisNames-Organ-Achsen tragen "
              "(Drift-Wache gegen build_axis_variant_version_table)");

/// Die 18 All*-Listen zu EINER flachen Typ-Liste ALLER registrierten Organ-Varianten gefaltet (Enabled + NICHT
/// enabled). Traegt die Voll-Registry-Wache und die Zaehl-Konstante.
using AllRegisteredOrganVariantsFlat = mp::mp_apply<mp::mp_append, AllRegisteredOrganAxisLists>;

/// Anzahl ALLER registrierten Organ-Varianten (unabhaengig vom Enable-Schalter). Der Testlauf belegt damit, dass
/// die Voll-Registry-Wache echt MEHR faengt als die Enabled-Tabelle (build_axis_variant_version_table().size()).
inline constexpr std::size_t kAllRegisteredOrganVariantCount = mp::mp_size<AllRegisteredOrganVariantsFlat>::value;

/// CX-W6: die Flag-Grammatik-Wache ueber die VOLLE registrierte Organ-Population. REINE CT-Wache -- der BAU
/// dieser Instanziierung IST die Durchsetzung (mp_for_each ruft assert_version_grammar<W>() je registrierter
/// Variante). Emittiert NICHTS -> kein Stempel, kein Codegen-Byte, golden-/CRC-/binary_id-neutral. Wird in
/// build_axis_variant_version_table() (Produktions-/Facade-Pfad) UND in der dedizierten Test-TU instanziiert.
inline void guard_all_registered_organ_versions() {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, AllRegisteredOrganVariantsFlat>>([](auto id) {
        using W = typename decltype(id)::type;
        assert_version_grammar<W>();
    });
}

/// E-24 C9 / FK-5 -- der FEHLERRAUM-ZWILLING der Wache darueber, auf DERSELBEN Population und nach
/// DEMSELBEN Muster. Warum er hier steht und nicht nur an den 18 CRTP-Basen: der Basis-Guard sitzt im
/// CRTP-Ctor und feuert erst, wenn eine Variante KONSTRUIERT wird. Diese Wache feuert am TYP, ueber die
/// VOLLE registrierte Population (enabled UND deaktiviert) -- eine registrierte Organ-Variante, die an
/// keiner der 18 Basen haengt oder deren Fehlerraum leer waere, kann so nicht unbemerkt in eine Registry
/// gelangen. Exakt die Rolle, die der Datei-Kopf oben fuer den VERSIONSRAUM beschreibt ("das mp_for_each
/// greift W::algo_version je registrierter Variante ab -- fehlt einer Variante das Member, ist der Zugriff
/// ill-formed und die Kompilation bricht MIT DEM TYP-NAMEN"), jetzt fuer den FEHLERRAUM.
///
/// DOMAENEN-TRENNUNG (A15-DESIGN OF-2: "'e'=Versionsraum, Fehlerklassen=Fehlerraum, disjunkte Konzerne"):
/// die beiden Wachen teilen die Population und die Bau-Stelle, aber KEINE Zeile Logik. assert_version_grammar
/// liest ausschliesslich W::algo_version, assert_organ_axis_error_classes ausschliesslich W::error_classes().
/// Sie stehen bewusst als ZWEI Funktionen nebeneinander statt als eine zusammengefasste -- eine gemeinsame
/// Wache haette die Konzerne verschmolzen, und ein Fehlerraum-Bruch haette wie ein Versions-Bruch gemeldet.
///
/// REINE CT-Wache: emittiert NICHTS -> kein Stempel, kein Codegen-Byte, golden-/CRC-/binary_id-neutral.
inline void guard_all_registered_organ_error_classes() {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, AllRegisteredOrganVariantsFlat>>([](auto id) {
        using W = typename decltype(id)::type;
        ::comdare::cache_engine::topics::assert_organ_axis_error_classes<W>();
        // FK-6 (09.08.2026) -- die ALGORITHMEN-Ebene, auf DERSELBEN Population und in DERSELBEN
        // Schleife. Die Zeile darueber prueft, DASS ein Fehlerraum da und nicht leer ist (Achsen-
        // Ebene); diese prueft, ob er fuer DIESEN Algorithmus AUSREICHT -- die per-Varianten-
        // Verfeinerung, die der FK-5-Header als "deklarierte Luecke" offen gelassen hatte.
        //
        // SIE IST BEDINGT, NICHT FLAECHIG, und das ist ein MESSERGEBNIS (Zahlen in
        // organ_axis_error_classes.hpp): von 126 registrierten Organ-Varianten braucht GENAU EINE
        // eine Klasse, die ihre Achse nicht fuehrt (pim_malloc -> quelle_nicht_verfuegbar). Eine
        // flaechige Pflicht haette 126 Abschriften erzwungen und kein falsches Etikett verhindert.
        // Der Bau IST die Durchsetzung: eine kuenftige HW-gatete Variante (OD-5 nennt GPU/FPGA
        // ausdruecklich) ohne die Klasse bricht HIER, mit dem Typ-Namen -- nicht erst in einer CSV,
        // in der "keine PIM-Einheit vorhanden" als "Allokator gescheitert" gelesen haette.
        ::comdare::cache_engine::topics::assert_algo_error_classes<W>();
    });
}

/// Baut die {axis,variant->version}-Tabelle ueber GENAU die 17 Kompositions-Achsen (kCompositionAxisNames-Reihenfolge
/// = algo_sig-Slot-Reihenfolge). Registry-getrieben (axes26-Aliase). Der Alias-Fahrplan spiegelt exakt
/// append_organ_core_axis_levels() + append_composition_tail_axis_levels() (die q1/q2-Slots) -- OHNE die build-only-/
/// System-Achsen und OHNE die Shape-Achsen (die tragen keine algo_sig).
[[nodiscard]] inline std::vector<AxisVariantVersion> build_axis_variant_version_table() {
    // CX-W6: die Flag-Grammatik-Wache greift ueber die VOLLE registrierte Organ-Population (auch deaktivierte
    // Varianten), nicht nur die Enabled*-Emit-Liste unten. REINE CT-Instanziierung (no-op zur Laufzeit) --
    // so feuert die Voll-Registry-Wache ueberall, wo die Tabelle gebaut wird (Facade + Test), byte-neutral.
    guard_all_registered_organ_versions();
    // E-24 C9 / FK-5: derselbe Griff fuer den FEHLERRAUM, unmittelbar daneben und als EIGENER Aufruf
    // (disjunkte Konzerne, s. Funktions-Kommentar). Ebenfalls reine CT-Instanziierung, byte-neutral.
    guard_all_registered_organ_error_classes();

    std::vector<AxisVariantVersion> t;
    reflect_versions<axes26::T00_search_algo>("search_algo", t);
    reflect_versions<axes26::T01_cache_traversal>("cache_traversal", t);
    reflect_versions<axes26::T02_mapping>("mapping", t);
    reflect_versions<axes26::T03_path_compression>("path_compression", t);
    reflect_versions<axes26::T04_node_type>("node_type", t);
    reflect_versions<axes26::T05_memory_layout>("memory_layout", t);
    reflect_versions<axes26::T06_allocator>("allocator", t);
    reflect_versions<axes26::T07_prefetch>("prefetch", t);
    reflect_versions<axes26::T08_concurrency>("concurrency", t);
    reflect_versions<axes26::T09_serialization>("serialization", t);
    reflect_versions<axes26::T11_value_handle>("value_handle", t);
    reflect_versions<axes26::T13_index_organization>("index_organization", t);
    reflect_versions<axes26::T14_io_dispatch>("io_dispatch", t);
    reflect_versions<axes26::T15_migration_policy>("migration_policy", t);
    reflect_versions<axes26::T16_filter>("filter", t);
    reflect_versions<axes26::T20_queuing_q1>("queuing_q1", t);
    reflect_versions<axes26::T21_queuing_q2>("queuing_q2", t);
    // STRUKT-R ORG-18: 18. Organ-Haupt-Achse. Reihenfolge = kCompositionAxisNames (T17 hinter queuing_q2).
    reflect_versions<axes26::T26_persistence_target>("persistence_target", t);
    return t;
}

/// Nachschlag der algo_version einer (axis, variant)-Kombination. Nicht gefunden -> leerer view (der Aufrufer
/// compose_algo_signature emittiert dann den Sentinel "@v0.0.0" statt zu raten -- dreistellig seit A13-M3/C4,
/// s. dort). Lineare Suche ueber die kleine Tabelle.
[[nodiscard]] inline std::string_view lookup_algo_version(std::vector<AxisVariantVersion> const& table,
                                                          std::string_view axis, std::string_view variant) {
    for (AxisVariantVersion const& e : table)
        if (e.axis == axis && e.variant == variant) return e.version;
    return std::string_view{};
}

/// E-10 S1: Nachschlag der UNGEWRAPPTEN Meta-Meta-Entry-Form einer (axis, variant)-Kombination ("" =
/// kein Traeger / nicht gefunden). Konsument ist Schritt 4b (compose_organ_stamp_line sammelt je Comp,
/// fuegt mit ';' und wrappt EINMAL via abi::wrap_meta_meta_entries); bis dahin ungenutzt (byte-neutral).
[[nodiscard]] inline std::string_view lookup_meta_meta_entries(std::vector<AxisVariantVersion> const& table,
                                                               std::string_view axis, std::string_view variant) {
    for (AxisVariantVersion const& e : table)
        if (e.axis == axis && e.variant == variant) return e.meta_meta_entries;
    return std::string_view{};
}

/// Die globalen Sub-Achsen-Werteset-Versionen (Bauplan Section 2): eine Werteset-ERWEITERUNG (z.B. neuer
/// CacheLineConfig-
/// Wert) aendert das serialisierte Bit-Layout, ohne dass eine Varianten-algo_version bumpt -> muss in die algo_sig,
/// sonst wuerde eine layout-geaenderte Binary STILL reused. Sie sind BUILD-global (nicht per-Variante) -> als fester
/// Schwanz an jede algo_sig gehaengt; ein Bump invalidiert konsequenterweise ALLE Binaries (grobkoernig, aber korrekt
/// und selten). Deterministisch, plattform-stabil.
///
/// A13-M3/C3: das Rendering ist auf die consteval-Single-Source abi::kSubAxisValuesetSegment gezogen. Grund:
/// dasselbe Segment ist seit A13-M3 auch ein Preimage-Glied des Anatomie-Fingerprints (F7-VERIFY, "schwerster
/// Befund": unter dem SHA512-only-Skip-Gate faellt die .algos-Signatur als Faenger weg). Zwei getrennte
/// Renderer -- hier zur Laufzeit, dort consteval -- waeren genau der uebersehene dritte Ableitungsweg. Die
/// .algos-Sidecar-BYTES bleiben unveraendert (gleiche Zeichenfolge), es ist kein Sidecar-Ereignis.
[[nodiscard]] inline std::string sub_axis_valueset_segment() {
    return std::string{::comdare::cache_engine::abi::kSubAxisValuesetSegment};
}

/// compose_algo_signature(axes, table) -- die deterministische Organ-Algorithmus-Signatur EINER Binary. Iteriert die
/// kCompositionAxisNames-Slots in FESTER Reihenfolge (plattform-stabil, unabhaengig von der spec.axes-Reihenfolge),
/// sucht je Slot den (axis,value) in spec.axes und schlaegt dessen algo_version in der Tabelle nach. Format je Slot
/// "<axis>=<variant>@<version>", per ';' gejoint (Vorbild perm.algos-Sidecar, Bauplan Section 1), abgeschlossen vom
/// Sub-Achsen-Werteset-Schwanz. Nicht-Kompositions-Achsen (Shapes) und in spec.axes fehlende Slots werden
/// uebersprungen; unbekannte (axis,value) -> "@v0.0.0"-Sentinel (statt zu raten; A13-M3/C4). Der Rebuild-/Neu-Mess-
/// Selektor vergleicht diese Signatur String-gleich gegen den .algos-Sidecar -> nur Binaries mit geaenderter Signatur
/// (= eine gebumpte Variante im 18-Tupel ODER ein Werteset-Bump) werden neu gebaut/gemessen; die binary_id bleibt
/// unberuehrt (die Version lebt ausschliesslich im Sidecar).
[[nodiscard]] inline std::string compose_algo_signature(std::vector<std::pair<std::string, std::string>> const& axes,
                                                        std::vector<AxisVariantVersion> const&                  table) {
    std::string out;
    for (std::string_view const slot : kCompositionAxisNames) {
        for (auto const& [ax, val] : axes) {
            if (ax != slot) continue;
            std::string_view const ver = lookup_algo_version(table, slot, val);
            if (!out.empty()) out += ';';
            out += slot;
            out += '=';
            out += val;
            out += '@';
            // A13-M3/C4 (DV-3 = Rueckbau): die Kurzform "v0" ist hier auf die dreistellige Sentinel-Form
            // gezogen; ein zurueckgelassenes "v0" ergaebe eine GEMISCHTE Signatur und damit einen Verstoss
            // gegen Owner-Q3 ("einheitlich und immer 3-stellig").
            // FLAG-GRAMMATIK v2: der Wortlaut kommt aus kAlgoSemVerSentinelLiteral (Single-Source) statt aus
            // einem zweiten Literal -- mit dem Wegfall des 'v' faellt er ausserdem mit dem GERENDERTEN
            // Sentinel zusammen, den compose_organ_stamp_line unten emittiert.
            out += ver.empty() ? ::comdare::cache_engine::measurement::kAlgoSemVerSentinelLiteral : ver;
            break;
        }
    }
    if (!out.empty()) out += ';';
    out += sub_axis_valueset_segment();
    return out;
}

/// compose_organ_stamp_line(axes, table) -- die kOrganAxisVersionLine "achse=variant@X.Y.Z;..." (Bau W12-A,
/// Section 43, geteilter Emitter-Helfer). Der SHARED-Helfer, den BEIDE Emitter-Pfade (lazy + Katalog) aufrufen
/// -> beide injizieren byte-identisch -> die 320-Round-Trip-Wache bleibt STRIKT (kein modulo).
///
/// LOEST DEN METADATEN-BLOCKER: die Version stammt aus der TABELLE (Registry-Wrapper name->algo_version), NICHT
/// aus den Composition-Strategie-Typen (die tragen kein name()/algo_version). SEPARATE Welt zur .algos-Sig
/// (compose_algo_signature bleibt byte-identisch): hier X.Y.Z-Voll-Form (algo_semver_string), NUR Haupt-Achsen
/// (Section 42.b: kein Sub-Achsen-Schwanz), kanonische kCompositionAxisNames-Ordnung (Entscheid W12-A-5).
/// Unbekannte (axis,value) -> "@0.0.0" (== der "@v0.0.0"-Sentinel des .algos-Pfads, in X.Y.Z). Determiniert.
[[nodiscard]] inline std::string compose_organ_stamp_line(std::vector<std::pair<std::string, std::string>> const& axes,
                                                          std::vector<AxisVariantVersion> const& table) {
    std::string out;
    for (std::string_view const slot : kCompositionAxisNames) {
        for (auto const& [ax, val] : axes) {
            if (ax != slot) continue;
            std::string_view const ver = lookup_algo_version(table, slot, val);
            if (!out.empty()) out += ';';
            out += slot;
            out += '=';
            out += val;
            out += '@';
            // Dreistelliger Sentinel (Owner-Q3), aus derselben Single-Source wie der .algos-Zwilling oben.
            // FLAG-GRAMMATIK v2: rohe und gerenderte Sentinel-Form sind dieselbe Zeichenfolge -- der
            // algo_semver_string-Durchlauf ist hier ab jetzt die Identitaet und bleibt trotzdem stehen,
            // weil er fuer die NICHT-leeren Faelle die kanonische Form erzwingt.
            out += ::comdare::cache_engine::measurement::algo_semver_string(
                ver.empty() ? ::comdare::cache_engine::measurement::kAlgoSemVerSentinelLiteral : ver);
            break;
        }
    }
    // A13-M2 (OP-11-Rueckbau, Owner-E2): der Organ-Meta-Meta-Klammer-Anhang ANS ENDE -- an BEIDEN
    // Organ-Zeilen-Quellen (hier der REALE Emitter-Pfad, in abi::organ_stamp_line<Comp>() der Mock-Pfad).
    // Ein Anhang nur auf einer Seite waere exakt die O-8-Schritt-12-Falle ("uebersehener dritter
    // Ableitungsweg"). HISTORIE (bis E-10 S1): abi::OrganMetaMetas war LEER -> no-op. SEIT E-10 S1 traegt
    // die Vollmenge ORG-19-IO; der typ-globale Anhang hier ist der deklarierte S1-S4-Korridor-
    // Zwischenstand (R-10, LANDE-VERBOT). Schritt 4 (4b) stellt auf die Tabellen-Selektion je Comp um
    // (lookup_meta_meta_entries in kCompositionAxisNames-Ordnung + EINMAL abi::wrap_meta_meta_entries).
    ::comdare::cache_engine::abi::append_meta_meta_suffix(
        out, ::comdare::cache_engine::abi::meta_meta_stamp_suffix_from_members<
                 ::comdare::cache_engine::abi::OrganMetaMetas>());
    return out;
}

} // namespace comdare::cache_engine::builder::experiment
