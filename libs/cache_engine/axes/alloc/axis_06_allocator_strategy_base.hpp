#pragma once
// V41.F.6.1.A CRTP-Basis-Klasse fuer Allocator-Achse 6 (2026-05-25, W1-revidiert)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// CRTP-Basis-Klasse mit Concept-Guard fuer alle Allokator-Familien.
//
// Pattern (siehe docs/architektur/11_konzept_achsen_extension_visitor_pattern.md §3.6):
//   - static_assert Concept-Check im Konstruktor (NICHT als template-constraint —
//     wegen CRTP-Henne-Ei: Derived ist incomplete bei Basis-Instantiation)
//   - Default-Methoden via CRTP-Inlining (Zero-Cost, KEINE virtual)
//
// API ist Standard-konform (PMR-Naming):
//   - allocate(bytes, alignment)
//   - deallocate(p, bytes, alignment)  noexcept
//
// Vendor-spezifische Sub-Refinements (calloc/realloc/usable_size/...) sind als
// separate Concepts vorhanden (siehe concepts/axis_06_allocator_*_strategy_concept.hpp).
//
// ==================================================================================================
// A1-VERSIONS-BUMP (2026-08-06) -- WARUM ALLE 26 STRATEGIEN DER ACHSE 6 AUF v1.0.1c GEHEN
// ==================================================================================================
// Der A1-Wurf-Vertrag hat den FEHLSCHLAG-Vertrag der Achse geaendert: der PmrResourceAdapter uebersetzt
// den OOM-nullptr jetzt nach std::bad_alloc (und laesst bei bytes == 0 keinen nullptr mehr durch), der
// StdAllocatorAdapter wacht die n*sizeof(T)-Multiplikation, allocate_or_throw ist neu, und
// PoolResourceAllocator meldet OOM nicht mehr als Wurf, sondern als nullptr + failure_count.
//
// DAS PROBLEM, DAS DIESER BUMP LOEST: keine dieser Aenderungen bewegt eine Registry-, XML- oder
// Fingerprint-Flaeche. Der inkrementelle Tier-Binary-Cache waehlt aber ueber die algo_sig, die je Achse
// aus W::algo_version zusammengesetzt wird (builder/experiment_tree/axis_variant_version_table.hpp ->
// compose_algo_signature). OHNE Bump waere die Vertrags-Aenderung fuer den Cache UNSICHTBAR: er wuerde
// vor diesem Commit gebaute Binaries als aktuell erkennen und weiterverwenden -- gemessen wuerde dann
// der ALTE Wurf-Vertrag unter dem NEUEN Quellstand. Genau das ist die Stale-Green-Wurzel (#50).
//
// WARUM AUSGERECHNET DIE ALLOKATOR-ACHSE UND WARUM SIE ALLEIN GENUEGT: die beiden Adapter und
// allocate_or_throw liegen in DIESER CRTP-Wurzel, jede der 26 Strategien traegt sie geerbt -- betroffen
// sind also alle 26, nicht eine Auswahl. Und weil jede Permutation GENAU EINEN Allokator-Slot fuehrt,
// aendert der Bump dieser einen Achse die algo_sig JEDER Permutation. Der Store-Konsum (axis_04
// LayoutAwareChunkedStore) ist damit mit abgedeckt, ohne die Knoten-Achse mitzubumpen: es gibt keine
// Permutation ohne Allokator. MINIMAL und VOLLSTAENDIG zugleich.
//
// GRAMMATIK: PATCH-Stelle, Owner-Q3-Flag bleibt 'c' (CPU-only-Flotte) -> "v1.0.0c" -> "v1.0.1c".
// Wohlgeformt nach assert_version_grammar/ce_owned_version_satisfies_cpu_enforce (ENFORCE ist scharf).
// FROZEN-NEUTRAL: die drei eingefrorenen Fingerprint-Fixtures (test_g3_sha512_index,
// test_w10_system_cell_values, test_m_w12_stamp_bausteine) bilden ihre Organ-Zeile aus LITERALEN
// ("search_algo=k_ary@1.0.0c;path_compression=path_compression_none@1.0.0c") ohne Allokator-Slot --
// sie koennen sich durch diesen Bump nicht bewegen und tun es nachweislich nicht.
// PATCH statt MINOR: der Erfolgs-Pfad ist verhaltens-gleich, geaendert hat sich ausschliesslich das
// Fehlschlag-Signal; die Achsen-API waechst nur um allocate_or_throw an der Wurzel.
//
// PRAEZISIERUNG DER RUECKWAERTSVERTRAEGLICHKEIT (A1-Nachbesserung 06.08.2026, Review-Befund
// "Source-Breaking fuer Fremdallokatoren ausserhalb der CRTP-Basis"): die Erst-Fassung schrieb hier
// "rueckwaertsvertraeglich fuer jeden bestehenden Konsumenten". Das war zu weit. Genau gilt:
//   * Fuer die REGISTRIERTE CRTP-Population (die 26 Strategien an AllocatorStrategyBase) ist die
//     Aenderung vollstaendig rueckwaertsvertraeglich -- allocate_or_throw und die Standard-Container-
//     Naht sitzen an DIESER Wurzel, jede Strategie erbt sie, keine Varianten-Datei wurde angefasst.
//   * Ein FREMD-Allokator ausserhalb dieser Wurzel, der als A in einen roh haltenden Konsumenten
//     gebunden wird, muss die vom Konsumenten geforderten Nahten tragen (fuer
//     LayoutAwareChunkedStore: allocate_or_throw, StdAllocatorAdapter<T>/as_std_allocator<T>,
//     Wert-Semantik, snapshot_t/statistics). Er musste sie auch VORHER schon tragen -- der Rumpf hat
//     sie immer benutzt; neu ist nur, dass der Bruch jetzt am KOPF auffaellt statt tief im Rumpf.
//     Das ist eine Diagnose-Verbesserung, keine zusaetzliche Anforderung. Belegt wird das positiv:
//     test_a1_wurf_vertrag_allokator_store bindet VollstaendigeFremdStrategie -- einen Typ OHNE
//     CRTP-Abstammung, aber MIT allen vier Nahten -- erfolgreich an den Store-Kopf.
//   * IST-BEFUND zum Stand 06.08.2026: es gibt KEINEN solchen Fremd-Konsumenten.
//       grep -rn "LayoutAwareChunkedStore<" libs/ tests/ ext/            -> 39 Fundstellen
//         davon 2 in der Definitions-Datei selbst, 1 in dieser Kopf-Doku, 10 Kommentar-Erwaehnungen
//         -> 26 ECHTE Typ-Bindungen.
//       grep -rn "LayoutAwareChunkedStore"  ext/                         -> 0
//     Das dritte Template-Argument dieser 26 ist ausnahmslos: MimallocAllocator (bzw. das Alias
//     PilotAlloc), Composition::allocator (eine Registry-Variante) oder eine an AllocatorStrategyBase
//     haengende Test-Variante. Die Aenderung kann heute also keinen Uebersetzungs-Fehler ausloesen,
//     den es nicht schon vorher gab.
//
// ==================================================================================================
// A1-VERSIONS-BUMP, 2. BUMP (2026-08-06, Owner-Entscheid nach Lens-Pass) -- v1.0.1c -> v1.0.2c FUER
// GENAU DIE 24 STRATEGIEN MIT EIGENER reallocate()-IMPLEMENTIERUNG
// ==================================================================================================
// DIE UMKEHR, DIE DIESER ABSCHNITT DOKUMENTIERT: die reallocate()-Statistik-Symmetrie-Korrektur
// (Phantom-Bytes, volle Begruendung: axis_06_allocator_pool_resource.hpp, Abschnitt "reallocate")
// wurde urspruenglich OHNE Bump ausgeliefert -- Begruendung damals: "reallocate() wird auf dem
// gesamten Mess-/Produktions-Pfad nie gerufen (grep-belegt: 0 Treffer ausserhalb von Tests und der
// Concept-Deklaration selbst), ein Bump wuerde den Tier-Binary-Cache grundlos komplett invalidieren."
//
// DER EINWAND (Lens-Pass 06.08.2026, vom Owner uebernommen): dieselbe Aussageform -- "wird unter den
// heutigen Umstaenden nicht beobachtet" -- hatte den 1. Bump oben NICHT von der Bump-Pflicht befreit
// (dort: "jede Aenderung liegt hinter einer Bedingung, die im Normalbetrieb nie feuert", trotzdem
// wurden alle 26 Strategien gebumpt, weil die Cache-Staleness-Sorge Vorrang hatte). Owner-Entscheid:
// "HEUTE UNERREICHBAR" ENTLASTET NICHT. reallocate() ist keine tote oder experimentelle Faehigkeit,
// sondern eine offiziell im Typsystem gefuehrte Achsen-Faehigkeit (ReallocatingStrategy-Concept). Ein
// KUENFTIGER Konsument koennte sie in den Mess-Pfad ziehen; ohne diesen Bump waere ein VOR der
// Statistik-Korrektur unter UNVERAENDERTEM v1.0.1c gecachtes Binary weiterhin auswaehlbar und braechte
// den Phantom-Byte-Fehler still zurueck -- ein Cache-IDENTITAETS-Risiko, kein Kosmetikposten.
//
// WARUM NUR 24 UND NICHT ALLE 26 (Umkehrung der "MINIMAL und VOLLSTAENDIG"-Regel des 1. Bumps): der
// 1. Bump betraf die CRTP-WURZEL (allocate_or_throw, beide Adapter) -- geerbt von allen 26, deshalb
// alle 26. Die reallocate()-Statistik-Korrektur sitzt dagegen JE STRATEGIE in deren EIGENER
// reallocate()-Implementierung, nicht an der Wurzel. PmrResourceAllocator und VampirNfpAllocator
// implementieren KEIN reallocate() (PMR-Interface bietet das nicht direkt, s.
// axis_06_allocator_pmr_resource.hpp; VampirNfpAllocator dito) -- der Phantom-Byte-Fehler kann in
// ihnen strukturell nicht existieren, ein Bump ohne Code-Aenderung waere selbst ein Verstoss gegen die
// Bump-Disziplin ("Startwert 'v1'; Bump-Disziplin ab dem 1. Bump" -- ein Bump zeigt eine Aenderung AN,
// er ist kein routinemaessiges Hochzaehlen). Sie bleiben auf v1.0.1c stehen.
//
// DIE 24: buddy, cama, crystalline, dlmalloc, exgen, hmalloc, hoard, jemalloc, lrmalloc, michael_lf,
// mimalloc, numalloc, pim_malloc, pool_resource, ptmalloc2, rpmalloc, scalloc, slab, snmalloc,
// starmalloc, std_malloc, tcmalloc, tcmalloc_wh, vmem_mag -- exakt die Menge, die den reallocate-Fix
// erhielt (Beleg: `git show --stat` auf den reallocate-Fix-Commit listet genau diese 24 .hpp-Dateien).
//
// FROZEN-NEUTRALITAET GEPRUEFT (dieselbe Auflage wie beim 1. Bump, VOR dem Bau gemessen): die drei
// eingefrorenen Fingerprint-Fixtures (test_g3_sha512_index.cpp, test_w10_system_cell_values.cpp,
// test_m_w12_stamp_bausteine.cpp) bilden ihren "allocator"-Slot AUSSCHLIESSLICH aus SYNTHETISCHEN
// Mock-Literalen (MockAxisV1::algo_version = "v1.0.0" bzw. das handgeschriebene Fixture-Literal
// "allocator=a@1.0.0c") -- beide sind vom Typ her von der REALEN AllVendors-Registry entkoppelt und
// koennen sich durch diesen Bump strukturell nicht bewegen. Keine XML-Registry-Datei (system_axis_
// registry.xml) und keine golden_fullpilot_320_binary_ids*-Datei enthaelt einen algo_version-String
// dieser Achse (repo-weiter grep nach "v1.0.1c"/"v1.0.2c" ausserhalb von axes/alloc/ und tests/unit/
// bleibt leer). Die algo_sig BEWEGT sich dagegen bewusst (das ist der Zweck des Bumps): der
// inkrementelle Tier-Binary-Cache verwirft die 24 betroffenen Binaries und baut sie neu.
//
// GRAMMATIK: PATCH-Stelle, Owner-Q3-Flag bleibt 'c' -> "v1.0.1c" -> "v1.0.2c" fuer die 24; die 2
// reallocate-losen Strategien bleiben bei "v1.0.1c". Wohlgeformt nach assert_version_grammar/
// ce_owned_version_satisfies_cpu_enforce (ENFORCE ist scharf); gepinnt in
// test_a1_algo_version_pin_alloc_axis (Nachtrag 2. Bump).

#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "alloc_hw_config.hpp" // F-B: NUMA/Page->allocator-Unterachse (GO4/#8, 2026-07-12)
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp> // INC-1a: OrganAxis<Derived> = topics::Axis-Dach + AxisBase (axis_kind()==organ)
#include <topics/organ_axis_error_classes.hpp> // FK-5: der Fehlerraum neben dem Versionsraum
#include <axes/cacheline/cacheline_config.hpp> // KF-5: per-Organ Cache-Line-Unterachse

#include <concepts>
#include <cstddef>
#include <limits> // A1-Posten 72: SIZE_MAX-Schranke der n*sizeof(T)-Ueberlauf-Wache
#include <memory_resource>
#include <new> // Posten 64: std::bad_alloc -- der Standard-Allokator-Vertrag des StdAllocatorAdapter
#include <type_traits>

namespace comdare::cache_engine::alloc {

/**
 * @brief AllocatorStrategyBase - CRTP-Basis fuer alle Allokator-Familien
 * @topic allocator
 * @achse 6
 *
 * HINWEIS V41.F.6.1.A: Template-Constraint `requires AllocatorStrategy<Derived>` ist
 * bei CRTP NICHT moeglich — Derived ist zur Zeit der Basis-Klassen-Instanziierung
 * noch incomplete. Loesung: Concept-Check NUR per static_assert im Konstruktor
 * (Derived ist dann vollstaendig).
 *
 * **CRTP-Pattern:** Default-Methoden delegieren via static_cast an Derived.
 * Compile-Time-Polymorphie, KEINE virtual call, Inlining-faehig.
 *
 * **V41.F.6.1.R7.4 (2026-05-29):** Adapter-Methoden as_std_allocator<T>() + as_pmr_resource()
 * implementiert (vorher static_assert-Stubs). Beide liefern WERT-basierte Adapter, die an die
 * allocate/deallocate-API der Strategie weiterleiten — KEINE Basis-Datenmember (Empty-Base-
 * Optimization + Wrapper-Groesse bleiben erhalten; der Adapter haelt nur einen Derived*).
 */
// KF-5 (2026-06-02): zusätzlicher defaulted NTTP CacheLineCfg + Erbe von CacheLineAware<Cfg> — macht JEDEN
// Allokator-Wrapper cacheline-fähig (cacheline_config/cacheline_alignment/cacheline_prefetch). Default {} =
// B64/None/None = unverändertes Verhalten (nicht-brechend, ODR-sicher: Default ist ein Literal, kein Makro).
// Die per-Binary-Bäckung wählt der Codegen über eine DISTINKTE Organ-Instanz (KF-6/KF-8), nicht über den Default.
// F-B (GO4/#8, 2026-07-12): dritter defaulted NTTP AllocHwCfg + AllocHwAware<Cfg> → jeder Allokator-Wrapper
// trägt die NUMA/Page-Unterachse (alloc_hw.numa_node {auto,0,1} / alloc_hw.page {4k,2m}; alloc_hw_config.hpp).
// Default {} = Auto/Native = keine Vorgabe → Verhalten/Bytes unverändert (exakt das KF-5-/node_width-Muster:
// Blätter bleiben konkrete Klassen, Registry-mp_list unberührt, golden-/Gate-1-neutral). Konsum:
// axis_06_allocator_numalloc.hpp (Node-Bindung) + axis_06_allocator_pool_resource.hpp (Page-Hint→pool_options).
template <typename Derived,
          ::comdare::cache_engine::cacheline::CacheLineConfig CacheLineCfg =
              ::comdare::cache_engine::cacheline::CacheLineConfig{},
          AllocHwConfig AllocHwCfg = AllocHwConfig{}>
class AllocatorStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived>,
                              public ::comdare::cache_engine::cacheline::CacheLineAware<CacheLineCfg>,
                              public AllocHwAware<AllocHwCfg> {
public:
    /// Concept-Check im Konstruktor: Pflicht-Set AllocatorStrategy + CacheEnginePermutationStrategy + AxisBase
    constexpr AllocatorStrategyBase() noexcept {
        // Inkrementeller Tier-Binary-Cache (Bauplan §2): Pflicht-algo_version je Kompositions-Organ-Variante — ohne
        // sie kann der Rebuild-/Neu-Mess-Selektor die Binary nicht organ-genau invalidieren. CRTP-Ctor-Guard;
        // universell zusaetzlich via build_axis_variant_version_table() (Typ-Ebene, alle 17 Kompositions-Registries).
        static_assert(
            requires { Derived::algo_version; },
            "Kompositions-Organ-Variante ohne 'static constexpr std::string_view algo_version' "
            "(Bauplan §2): Rebuild-Selektor kann nicht organ-genau invalidieren.");

        // FK-5 (E-24 C9): der Fehlerraum-Zwilling der Versions-Wache darueber -- existiert die
        // Deklaration, und ist sie nicht leer? Beides bricht compile-hart MIT dem Typ-Namen.
        ::comdare::cache_engine::topics::assert_organ_axis_error_classes<Derived>();
        static_assert(concepts::AllocatorStrategy<Derived>, "Derived must satisfy AllocatorStrategy concept "
                                                            "(see concepts/axis_06_allocator_concept.hpp)");
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>,
                      "Derived must satisfy CacheEnginePermutationStrategy concept "
                      "(see concepts/axis_06_allocator_cache_engine_permutation_concept.hpp)");
        static_assert(
            ::comdare::cache_engine::topics::AxisBaseConcept<Derived>,
            "Derived must satisfy AxisBaseConcept (get_compiler() Pflicht-API). "
            "AllocatorStrategyBase erbt AxisBase via OrganAxis — Derived bekommt Default 'original' automatisch.");
        static_assert(::comdare::cache_engine::topics::OrganAxisConcept<Derived>,
                      "Derived must satisfy OrganAxisConcept: Organ-Achse unterm gemeinsamen Dach topics::Axis "
                      "(axis_kind()==organ, EBO-neutral). INC-1a: AllocatorStrategyBase haengt via OrganAxis am Dach.");
    }

    // ───────────────────────────────────────────────────────────────────────
    // V41.F.6.1.P2.C ENTFERNT: has_original_paper_code + is_original_module
    // Kommen jetzt generisch via AxisBase Default (is_original_module = false).
    // Paper-Wrappers (z.B. MimallocAllocator) ueberschreiben via Mixin-Inheritance.
    // ───────────────────────────────────────────────────────────────────────

    // ───────────────────────────────────────────────────────────────────────
    // V41.F.6.1.R7.4 Cross-Axis-Default resource_ownership() ([[cross-axis-defaults-no-bloat]]):
    // Das Besitz-Modell des memory_resource-Objekts ist fuer die GANZE malloc-Familie (libc +
    // alle Vendor-Allokatoren) None — sie verwalten kein std::pmr::memory_resource. Der Default
    // gehoert daher EINMAL hierher in die CRTP-Wurzel, NICHT 25x in die Wrapper. Nur die beiden
    // PMR-Familien-Wrapper ueberschreiben (PoolResourceAllocator=Owned, PmrResourceAllocator=Borrowed).
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] static constexpr concepts::ResourceOwnership resource_ownership() noexcept {
        return concepts::ResourceOwnership::None;
    }

    // ───────────────────────────────────────────────────────────────────────
    // CRTP-Delegate-Methoden zu Derived (Standard-PMR-Naming)
    // ───────────────────────────────────────────────────────────────────────

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) {
        return derived().allocate(bytes, alignment);
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) noexcept {
        derived().deallocate(p, bytes, alignment);
    }

    /**
     * @brief allocate_or_throw -- die EINE Uebersetzungsstelle fuer ROHE Achsen-Konsumenten.
     *
     * **WARUM SIE FEHLTE (A1-Wurf-Vertrag, Posten 74):** Posten 64 hat den Fehlschlag-Vertrag genau dort
     * uebersetzt, wo ein STANDARD-Container die Achse konsumiert -- im StdAllocatorAdapter. Ein Konsument,
     * der die Strategie ROH haelt (`A alloc_;` als Kompositions-Template-Parameter) und deren Rueckgabe
     * direkt beschreibt, kommt an dieser Uebersetzung VORBEI: die Achse meldet OOM per nullptr, der
     * Konsument memsetzt/memcpyt hinein -- undefiniertes Verhalten, still, im Mess-Pfad
     * (Fundstelle: axis_04_node_type_layout_aware_store.hpp, append_slot/copy_from_).
     *
     * Die Heilung ist bewusst DIESELBE Form wie bei Posten 64 und liegt an DERSELBEN Stelle: EINE
     * Uebersetzung in der CRTP-Wurzel statt einer Pruefung je Konsument. Damit gilt fuer die ganze Achse
     * genau EIN Wurf-Vertrag, unabhaengig davon, ob der Konsument ueber den Standard-Adapter, ueber das
     * pmr-Resource oder roh zugreift.
     *
     * **ZERO-SIZE AUSGENOMMEN -- wie StdAllocatorAdapter, ANDERS als PmrResourceAdapter:** `bytes == 0`
     * reicht den Strategie-Rueckgabewert unangetastet durch, die Zero-Size-Wachen der Organe bleiben
     * gueltig. Der pmr-Weg kann diese Ausnahme NICHT teilen -- dort verbietet der Standard den nullptr
     * auch bei `bytes == 0` (Begruendung an PmrResourceAdapter::do_allocate). Die Abweichung ist also
     * standard-getrieben und keine Inkonsistenz dieser Achse.
     *
     * **TIMING-VERMERK (A1-Nachbesserung 2026-08-06, ehrlicher als die Erst-Fassung):** der Erfolgs-Pfad
     * ist NICHT kostenfrei. Er kostet GENAU EINEN vorhersagbaren Vergleich (`p == nullptr`, im
     * Messbetrieb immer nicht-genommen) und stellt die Aufrufstelle unter EH-Pflicht: der Aufruf ist ab
     * jetzt potenziell werfend, der Konsument braucht also Landing-Pads/Unwind-Flaeche, wo vorher ein
     * nicht-werfender Aufruf stand (Code-Groesse, keine Laufzeit auf dem genommenen Pfad). Beides liegt
     * Groessenordnungen unter einer Allokation und faellt in keiner Achsen-Messreihe auf -- aber "ohne
     * jede Zusatzarbeit" waere falsch, und in einem MESS-Projekt darf das nicht behauptet werden. Die
     * Statistik-Ehrlichkeit bleibt gewahrt, weil die Strategie ihren failure_count VOR dem
     * `return nullptr` zaehlt (Auflage aus Posten 64).
     *
     * **ANFORDERUNG AN ROHE KONSUMENTEN (A1-Nachbesserung):** wer diese Uebersetzung roh konsumiert,
     * bindet seine Strategie an `concepts::ThrowTranslatingStrategy` (axis_06_allocator_concept.hpp)
     * und NICHT nur an `AllocatorStrategy` -- letzteres fordert `allocate_or_throw` nicht, eine Strategie
     * ohne diesen Member faellt sonst erst als Instanziierungs-Fehler tief im Konsumenten auf.
     * Fundstelle des Konsums: axis_04_node_type_layout_aware_store.hpp (Kopf-Constraint).
     * Fehlerklasse: unveraendert der FK-5-Boden der Allokator-Achse (kOrganAxisErrorFloor).
     */
    [[nodiscard]] void* allocate_or_throw(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) {
        void* const p = derived().allocate(bytes, alignment);
        if (p == nullptr && bytes != 0) throw std::bad_alloc{};
        return p;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    // -- SELBST-REKURSIONS-WACHE der drei Statistik-Weiterleiter (A8-S5 01c Vorlauf 0, 2026-08-05) -------
    //
    // WAS HIER SCHIEFGEHEN KANN (und bis heute schiefging): jeder Weiterleiter unten ruft
    // `derived().<name>(...)`. Deklariert die Strategie KEINEN eigenen Member dieses Namens, dann findet
    // die Namenssuche in Derived nur den GEERBTEN Basis-Member wieder -- der Weiterleiter ruft SICH SELBST.
    // Das ist keine Diagnose wert, die der Compiler von sich aus stellt: g++ 15.3 uebersetzt die
    // unbeschraenkte Selbst-Rekursion unter -O3 als Endlosschleife ('jmp .', Tail-Call-Faltung) und unter
    // -O1 als rekursiven Selbstaufruf bis zum Stack-Ueberlauf. ASan/UBSan melden NICHTS -- es ist kein
    // Speicherfehler, sondern ein Programm, das nie zurueckkehrt. Genau dieser Befund wurde in Scheibe 01b
    // an AxisBoundBuffer<MimallocAllocator,T> beobachtet und dort (mangels Ursache) als moeglicher
    // Compiler-/UB-Verdacht notiert; er ist WEDER das eine NOCH das andere, sondern diese fehlende
    // Deklaration: bis 2026-08-05 trug ausschliesslich ExgenAllocator ein eigenes restore_statistics.
    //
    // WARUM ALS static_assert IM RUMPF und nicht als Concept-Constraint: der Rumpf einer Member-Funktion
    // eines Klassen-Templates wird erst bei BENUTZUNG instanziiert -- erst dann ist Derived vollstaendig
    // (CRTP-Henne-Ei, dieselbe Begruendung wie bei den Ctor-Guards oben). Der Beweis ist self-proving statt
    // geraten: ist der Klassen-Teil des Member-Zeigers die BASIS, hat Derived nichts Eigenes deklariert.
    template <class MemPtr>
    static constexpr bool kIsBaseForwarder = std::is_same_v<MemPtr, void (AllocatorStrategyBase::*)() noexcept>;

    [[nodiscard]] concepts::AllocationStatistics statistics() const noexcept {
        static_assert(!std::is_same_v<decltype(&Derived::statistics),
                                      concepts::AllocationStatistics (AllocatorStrategyBase::*)() const noexcept>,
                      "axis_06: die Strategie deklariert kein EIGENES statistics() -- der CRTP-Weiterleiter der "
                      "Basis wuerde sich selbst aufrufen (unbeschraenkte Rekursion). Eigenen Member nachziehen.");
        return derived_const().statistics();
    }

    void reset() noexcept {
        // V41.F.6.1.A User-Klarstellung: reset() = Statistik-Reset, NICHT Pool-Reset!
        static_assert(!kIsBaseForwarder<decltype(&Derived::reset)>,
                      "axis_06: die Strategie deklariert kein EIGENES reset() -- der CRTP-Weiterleiter der Basis "
                      "wuerde sich selbst aufrufen (unbeschraenkte Rekursion). Eigenen Member nachziehen.");
        derived().reset();
    }

    // Phase 0.3 (Hebel B, Memento-Pattern GoF): Statistik auf einen zuvor via statistics() gezogenen Snapshot
    // zuruecksetzen — spiegelbildlich zu reset(). Nutzung: ein strategie-besitzender Store (z.B. TreeNodePoolStore)
    // verwirft damit im Copy-Ctor/Assign die durch die COW-Vollkopie entstandene transiente Re-Allokations-
    // Pollution (die Zwei-Phasen-Mess-Doppelzaehlung), sodass T6 = save-Stand + measure-Delta bleibt.
    void restore_statistics(concepts::AllocationStatistics const& s) noexcept {
        static_assert(
            !std::is_same_v<decltype(&Derived::restore_statistics),
                            void (AllocatorStrategyBase::*)(concepts::AllocationStatistics const&) noexcept>,
            "axis_06: die Strategie deklariert kein EIGENES restore_statistics() -- der CRTP-Weiterleiter der "
            "Basis wuerde sich selbst aufrufen (unbeschraenkte Rekursion; -O3 emittiert 'jmp .'). Das ist der "
            "Memento-Pfad JEDES strategie-besitzenden Stores/Puffers: ohne den Member haengt der Copy-Ctor. "
            "Eigenen Member nachziehen (Vorlage: axis_06_allocator_exgen.hpp).");
        derived().restore_statistics(s);
    }
#endif

    // ───────────────────────────────────────────────────────────────────────
    // V41.F.6.1.R7.4 Adapter-Methoden — Allocator-Achse an std::allocator / std::pmr anbinden.
    // WERT-basiert (kein Basis-Datenmember → EBO + Wrapper-Groesse bleiben erhalten).
    // ───────────────────────────────────────────────────────────────────────

    /**
     * @brief StdAllocatorAdapter<T> — erfuellt die std::allocator-Named-Requirements (C++23) und
     *        leitet allocate/deallocate an die zugrundeliegende Achsen-Strategie weiter.
     *        Nutzbar mit std::vector<T, StdAllocatorAdapter<T>>, std::allocator_traits, rebind.
     *
     * **FEHLSCHLAG-VERTRAG (Posten 64, 2026-08-04) -- die EINE Uebersetzungsstelle:**
     * Die Achsen-Strategie meldet OOM per **nullptr** (Achsen-Semantik, z.B. ExgenAllocator::allocate
     * -> portable_aligned_alloc). Die std::allocator-Named-Requirements verlangen dagegen, dass
     * `allocate()` bei Fehlschlag **WIRFT** (C++23 [allocator.requirements]: "Throws: bad_alloc if the
     * storage cannot be obtained"); ein besitzender Container wie std::vector prueft den Rueckgabewert
     * NICHT und konstruiert bei nullptr in Nullspeicher -- UB. Genau hier, im Adapter, wird die
     * Achsen-Semantik in die Standard-Semantik uebersetzt: EINE Stelle statt einer Pruefung je
     * Konsument. Damit sind die `[[allocation-failure-exception]]`-Aussagen der besitzenden Organe
     * (mapping/value_handle/cache_traversal + die vier Pool-Stores) wieder WAHR -- der Wurf kommt
     * jetzt vom Adapter statt vom Default-Allokator, die Fehlerklasse bleibt der FK-5-Boden.
     *
     * **EHRLICHKEIT BLEIBT (Auflage):** der `failure_count` der Strategie ist zum Zeitpunkt des Wurfs
     * BEREITS gezaehlt -- die Strategie zaehlt ihn VOR dem `return nullptr` (axis_06_allocator_exgen.hpp).
     * Der Wurf verdeckt also keinen Messwert, er ersetzt nur den UB-Pfad dahinter.
     *
     * **ZERO-SIZE UNVERAENDERT:** fuer `n == 0` bleibt der Rueckgabewert der Strategie unangetastet
     * durchgereicht (der Standard fordert dort keinen Nicht-Null-Zeiger, und kein Konsument
     * dereferenziert ihn) -- die Zero-Size-Wachen der Organe bleiben damit gueltig.
     */
    template <typename T>
    class StdAllocatorAdapter {
    public:
        using value_type = T;
        explicit StdAllocatorAdapter(Derived* strat) noexcept : strat_(strat) {}
        template <typename U>
        StdAllocatorAdapter(StdAllocatorAdapter<U> const& other) noexcept : strat_(other.strat_) {}

        [[nodiscard]] T* allocate(std::size_t n) {
            // UEBERLAUF-WACHE (A1-Wurf-Vertrag, Posten 72): `n * sizeof(T)` war eine UNGEWACHTE
            // size_t-Multiplikation. Der allocator_traits-Default fuer max_size() schuetzt nur Aufrufer,
            // die ueber die Wachstums-Pfade eines Containers gehen (vector/deque reserve/push_back); ein
            // DIREKTER allocate(n) mit n > SIZE_MAX/sizeof(T) laesst das Produkt umlaufen, die Strategie
            // "gelingt" mit einem viel zu kleinen Puffer -- und der Fehler faellt erst beim SCHREIBEN auf,
            // als Heap-Overflow statt als Fehlschlag. Der Standard-Praezedenzfall dafuer ist
            // std::bad_array_new_length ([expr.new]/[allocator.requirements]); die Wache kostet einen
            // Vergleich und feuert bei realen Workload-Groessen nie.
            if (n > (std::numeric_limits<std::size_t>::max)() / sizeof(T)) throw std::bad_array_new_length{};
            void* const p = strat_->allocate(n * sizeof(T), alignof(T));
            // Fehlschlag-Vertrag (s. Klassen-Doku): nullptr aus der Strategie -> std::bad_alloc.
            // n == 0 bleibt bewusst ausgenommen (Zero-Size-Verhalten unveraendert).
            if (p == nullptr && n != 0) throw std::bad_alloc{};
            return static_cast<T*>(p);
        }
        void deallocate(T* p, std::size_t n) noexcept { strat_->deallocate(p, n * sizeof(T), alignof(T)); }
        template <typename U>
        [[nodiscard]] bool operator==(StdAllocatorAdapter<U> const& other) const noexcept {
            return strat_ == other.strat_;
        }

    private:
        template <typename U>
        friend class StdAllocatorAdapter;
        Derived* strat_;
    };

    /**
     * @brief PmrResourceAdapter — std::pmr::memory_resource ueber die Achsen-Strategie. Konkreter,
     *        kopierbarer Wert-Typ (haelt nur Derived*); der Aufrufer haelt ihn am Leben und
     *        uebergibt &resource an pmr-Container.
     *
     * **FEHLSCHLAG-VERTRAG (A1-Wurf-Vertrag, Posten 71) -- der ZWEITE Uebersetzungspunkt:** dieselbe
     * Luecke wie in Posten 64, nur an der pmr-Naht. `std::pmr::memory_resource::allocate()` -- und damit
     * das hier ueberschriebene `do_allocate` -- ist standardvertraglich WERFEND ("Throws: bad_alloc if
     * storage cannot be obtained"); ein pmr-Container darf sich darauf verlassen und prueft den
     * Rueckgabewert NICHT. Die Achsen-Strategie meldet OOM aber per nullptr. Bis zu dieser Haertung
     * reichte der Adapter den nullptr ungeprueft an den pmr-Container durch -- dieselbe UB-Klasse wie
     * vor Posten 64, mit realem Konsumenten (std::pmr::vector ueber as_pmr_resource()).
     *
     * **ZERO-SIZE IST HIER KEINE AUSNAHME (A1-Nachbesserung 2026-08-06) -- und der Unterschied zum
     * StdAllocatorAdapter ist standard-getrieben, nicht Geschmack:** [mem.res.public] verlangt von
     * `memory_resource::allocate` einen Zeiger auf Speicher "of at least bytes bytes" und kennt als
     * Fehlerausgang AUSSCHLIESSLICH den Wurf. Ein nullptr ist an dieser Naht in KEINEM Fall ein
     * gueltiger Rueckgabewert -- auch nicht bei `bytes == 0`: der pmr-Container prueft nicht nach,
     * er speichert den Zeiger und rechnet mit ihm weiter. Die Erst-Fassung dieser Wache trug hier ein
     * `&& bytes != 0` und liess damit genau den einen nullptr stehen, den der Standard verbietet.
     * Der Vorbehalt ist entfernt: eine Strategie, die fuer 0 Bytes nullptr meldet, laeuft an dieser
     * Naht in std::bad_alloc statt in UB beim Konsumenten. Die beiden anderen Wege
     * (StdAllocatorAdapter::allocate, allocate_or_throw) BEHALTEN ihre Zero-Size-Ausnahme -- dort ist
     * der nullptr vertraglich zulaessig und die Zero-Size-Wachen der Organe bauen darauf.
     */
    class PmrResourceAdapter final : public std::pmr::memory_resource {
    public:
        explicit PmrResourceAdapter(Derived* strat) noexcept : strat_(strat) {}

    private:
        void* do_allocate(std::size_t bytes, std::size_t alignment) override {
            void* const p = strat_->allocate(bytes, alignment);
            // BEWUSST OHNE `&& bytes != 0` (s. Klassen-Doku): [mem.res.public] laesst den nullptr an
            // dieser Naht in KEINEM Fall zu, der Zero-Size-Vorbehalt der anderen zwei Wege gilt hier nicht.
            if (p == nullptr) throw std::bad_alloc{};
            return p;
        }
        void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
            strat_->deallocate(p, bytes, alignment);
        }
        [[nodiscard]] bool do_is_equal(std::pmr::memory_resource const& other) const noexcept override {
            auto const* o = dynamic_cast<PmrResourceAdapter const*>(&other);
            return o != nullptr && o->strat_ == strat_;
        }
        Derived* strat_;
    };

    /// Liefert einen std::allocator-kompatiblen Adapter (Wert) fuer Element-Typ T.
    template <typename T>
    [[nodiscard]] StdAllocatorAdapter<T> as_std_allocator() noexcept {
        return StdAllocatorAdapter<T>(&derived());
    }

    /// Liefert einen pmr-memory_resource-Adapter (Wert). Aufrufer haelt ihn am Leben und uebergibt
    /// dessen Adresse an pmr-Container (z.B. std::pmr::polymorphic_allocator).
    [[nodiscard]] PmrResourceAdapter as_pmr_resource() noexcept { return PmrResourceAdapter(&derived()); }

public:
    /// FK-5 (A15 EBENE 4, Fehlerraum) -- WELCHE D2-Fehlerklassen diese Organ-Achse ueberhaupt
    /// hervorbringen kann. Deklariert an DERSELBEN Stelle, die schon algo_version erzwingt (K2-Muster):
    /// eine Stelle je Achse statt einer Zeile je Varianten-Datei.
    /// Boden: der Allokator kann real scheitern (OOM ist der Lehrbuch-Fall von Failed).
    [[nodiscard]] static constexpr auto error_classes() noexcept {
        return ::comdare::cache_engine::topics::kOrganAxisErrorFloor;
    }

protected:
    [[nodiscard]] Derived&       derived() noexcept { return static_cast<Derived&>(*this); }
    [[nodiscard]] Derived const& derived_const() const noexcept { return static_cast<Derived const&>(*this); }
};

} // namespace comdare::cache_engine::alloc
