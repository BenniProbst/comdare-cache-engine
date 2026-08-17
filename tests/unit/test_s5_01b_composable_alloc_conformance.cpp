// A8-S5 Familie 01_read_path / Sub-Scheibe 01b (der composable-REST unter organ_axes/lookup/composable/, also
// alles ausser den Pool-/Layout-Stores der Scheibe 01a) -- FAMILIEN-KONFORMITAETS-WACHE.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4 + 5-S5.
// Das wiederverwendbare Praedikat liegt in s5_family_alloc_conformance.hpp (Pilot 04_execution) und bleibt
// hier UNVERAENDERT.
//
// WAS GEPINNT WIRD: die KOMPOSITIONS-SCHALE fuehrt keinen Speicher mehr am Allokator-Achsen-Interface vorbei.
// Gegenstand sind -- anders als in 01a -- nicht die Substrate, sondern die Organe darueber und ihre Huellen:
// die Walk-Stacks der zwoelf Composed*Search, der Memento-Puffer und die Scan-Zwischenablage des flachen
// Kompositions-Modells, und der Pilot-Store RawSlotStore, der als einziger Store NICHT in einer *_store.hpp
// wohnt und deshalb 01a durch die Datei-Maschen fiel.
//
// DIE ZWEI SCHNITT-FORMEN NEBENEINANDER -- und warum die Aufteilung KEINE Geschmacksfrage ist:
//   * Form A (heap-frei) gilt fuer die drei TRIE-Familien (ART / HOT-Patricia / START). Ihre Abstiegstiefe ist
//     STRUKTURELL compile-time beschraenkt: je Inner-Knoten mindestens ein Schluessel-Byte (ART/START) bzw.
//     genau ein Bit mit streng monoton wachsender Position (HOT). Der Walk-Stack ist deshalb ein
//     std::array mit typ-abgeleiteter Kappe -- gar keine Allokation, die staerkere Aussage.
//   * Form B (ueber die Achse) gilt fuer BST und B-Baum. Deren Stack-Tiefe haengt an den DATEN (unbalancierter
//     BST bis zur Record-Zahl; B-Baum Hoehe mal Aritaet). Eine CT-Kappe waere dort keine staerkere Aussage,
//     sondern eine FALSCHE -- ein Ueberlauf wuerde still Records verlieren. Diese TU pinnt deshalb BEIDE
//     Formen JE ORGAN, nicht eine Pauschale.
//
// WOHER DIE TYP-LISTE KOMMT (Anti-Leerlauf, KEINE Datei-/Namensliste): dieselbe Ableitung wie in 01a --
//   AllStrategies (axis_03a) -> organ_for_search_algo_t<W> (nicht-void = organ-backed) -> das ORGAN.
// Dazu die drei FLACHEN Kompositionen (LinearScan/SortedBinary/Interpolation ueber RawSlotStore<>), die
// organ_for_search_algo NICHT liefert, weil sie keine eigene Pool-Familie haben -- sie sind aber genau der
// Gegenstand von composable_search.hpp und werden deshalb DEKLARIERT angehaengt statt still zu fehlen.
//
// GEPRUEFTE EBENEN:
//   (1) die ORGANE           -- Ausweis + Achsen-Bindung des Speichers der Schale
//   (2) die ORGAN-HUELLEN    -- ObservableComposedContainer/-Search: reicht die Huelle den Ausweis weiter?
//   (3) FORM A am Objekt     -- die CT-Kappe der drei Trie-Organe ist aus dem TYP abgeleitet, nie ein Literal
//   (4) FORM B LAUFZEIT      -- Verdrahtungs-Beleg: unter Last steigt der Walk-Zaehler der Achse
//   (5) MEMENTO/SCAN LAUFZEIT-- die beiden anderen Schalen-Puffer allozieren real ueber die Achse
//   (6) NULLPUNKTE           -- ungetriebene Organe melden ehrliche 0, nicht "irgendwas > 0"
//
// WARUM (4)-(6) HIER STEHEN MUESSEN (Form-B-Grenze, s5_family_alloc_conformance.hpp:31): das Praedikat prueft,
// dass ein DEKLARIERTER allocator_type das Achsen-Concept erfuellt -- nicht, dass die Allokation real
// darueber laeuft. Fuer diese Familie ist das besonders scharf, weil die Ebenen (1)/(2) in 01a noch reiner
// AUSWEIS waren; erst diese Scheibe macht sie zur Aussage, also muss sie sie auch belegen.
//
// Standalone (plain int main, KEIN gtest) -- konsistent mit den uebrigen S5-/Phase-E-Wachen.

#include "s5_family_alloc_conformance.hpp"

#include <organ_axes/alloc/axis_06_allocator_exgen.hpp>
#include <organ_axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <organ_axes/lookup/axis_03a_search_algo_registry.hpp>
#include <organ_axes/lookup/composable/observable_composed_container.hpp>
#include <organ_axes/lookup/composable/observable_composed_search.hpp>
#include <organ_axes/lookup/composable/organ_for_search_algo.hpp>
#include <organ_axes/lookup/composable/tier_to_organ_mapping.hpp>

#include <boost/mp11.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <type_traits>

namespace mp  = boost::mp11;
namespace s5  = ::comdare::cache_engine::tests::s5;
namespace alx = ::comdare::cache_engine::alloc;
namespace lk  = ::comdare::cache_engine::lookup;
namespace lkc = ::comdare::cache_engine::lookup::composable;

namespace {

// -- (1) Die Familie AUS DER REGISTRY ---------------------------------------------------------------
template <class W>
using organ_of = lkc::organ_for_search_algo_t<W>;
template <class W>
using is_organ_backed_wrapper = mp::mp_bool<!std::is_same_v<organ_of<W>, void>>;

using RegistryWrappers = mp::mp_filter<is_organ_backed_wrapper, lk::AllStrategies>;
using RegistryOrgans   = mp::mp_transform<organ_of, RegistryWrappers>;

/// Die FLACHEN Kompositionen: sie haben keine eigene Pool-Familie und tauchen deshalb in
/// organ_for_search_algo NICHT auf -- sie sind aber der Kern von composable_search.hpp (RawSlotStore +
/// Memento-Puffer + Scan-Zwischenablage) und damit der Kern DIESER Scheibe. Deklariert angehaengt.
using FlatOrgans = mp::mp_list<lkc::LinearScanOrgan, lkc::SortedBinaryOrgan, lkc::InterpolationOrgan>;
/// Wie 01a: das Masstree-Organ hat (noch) keinen Wrapper in AllStrategies, ist aber ein Objekt der Familie.
using DeferredOrgans = mp::mp_list<lkc::MasstreeOrgan>;

using Family01bOrgans = mp::mp_append<RegistryOrgans, DeferredOrgans>;

// -- (2) Die Huellen, wie der ABI-Adapter sie fuehrt -------------------------------------------------
template <class Organ>
using HullOf         = lkc::ObservableComposedContainer<Organ>;
using Family01bHulls = mp::mp_transform<HullOf, Family01bOrgans>;
/// Die flachen Organe tragen die ANDERE Huelle (ObservableComposedSearch<Traversal,Store>) -- sie wird
/// separat gepinnt, weil sie eine eigene Klasse ist und den Ausweis eigenstaendig weiterreichen muss.
using FlatHulls = mp::mp_list<lkc::ObservableComposedSearch<lkc::LinearScanTraversal, lkc::RawSlotStore<>>,
                              lkc::ObservableComposedSearch<lkc::SortedBinaryTraversal, lkc::RawSlotStore<>>>;

// -- Anti-Leerlauf: eine leere Liste macht JEDE Alles-Aussage wahr -----------------------------------
static_assert(mp::mp_size<lk::AllStrategies>::value > 0,
              "S5-01b: die 03a-Registry ist LEER -- dann waere die ganze Ableitungskette wirkungslos.");
static_assert(mp::mp_size<RegistryOrgans>::value >= 11,
              "S5-01b: die organ-backed Familie ist geschrumpft (Planungs-Ist 04.08.: 11) -- ein Rueckbau muss "
              "auffallen, nicht durchrutschen.");
static_assert(mp::mp_size<mp::mp_unique<Family01bOrgans>>::value == mp::mp_size<Family01bOrgans>::value,
              "S5-01b: die Organ-Liste enthaelt Duplikate -- dann deckte ein Eintrag mehrere Familien zu.");
static_assert(mp::mp_size<FlatOrgans>::value == 3,
              "S5-01b: die flachen Kompositionen sind der Kern von composable_search.hpp -- fehlt eine, "
              "prueft diese TU den Pilot-Store und seine Puffer nicht mehr vollstaendig.");

// -- Die Wache selbst (Typ-Ebene) --------------------------------------------------------------------
static_assert(s5::family_alloc_conform_v<Family01bOrgans>,
              "S5-01b: ein Kompositions-Organ fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(s5::family_alloc_conform_v<FlatOrgans>,
              "S5-01b: eine der drei FLACHEN Kompositionen faellt durch -- das ist der Pilot-Pfad "
              "(RawSlotStore + Memento + Scan-Zwischenablage) von composable_search.hpp.");
static_assert(s5::family_alloc_conform_v<Family01bHulls>,
              "S5-01b: eine Organ-HUELLE faellt durch. Das ist genau die Memory-Falle "
              "'Observable-Huellen muessen ALLE Concept-erzwungenen Member forwarden' -- der Ausweis des "
              "Organs darf an der Huelle nicht verschwinden.");
static_assert(s5::family_alloc_conform_v<FlatHulls>,
              "S5-01b: die ObservableComposedSearch-Huelle reicht den Ausweis nicht weiter.");
static_assert(s5::family_alloc_conform_v<mp::mp_list<lkc::RawSlotStore<>>>,
              "S5-01b: der Pilot-Store RawSlotStore<> fuehrt Speicher am Achsen-Interface vorbei. Er ist der "
              "EINE Store der Familie, der nicht in einer *_store.hpp wohnt -- genau deshalb fiel er 01a "
              "durch die Datei-Maschen und wird hier typ-getrieben mitgeprueft.");

/// SCHAERFER als (A)||(B): fuer die Organe ist Form A gar nicht erreichbar -- sie BESITZEN ihr Substrat
/// (Pool/Store mit unbounded Container), sind also nie trivial destruierbar. Ein "heap-frei"-Ausweis waere
/// hier ein Indiz dafuer, dass ein Organ sein Substrat verloren hat, nicht dass es sauber fuehrt.
template <class T>
using is_axis_bound = mp::mp_bool<s5::AxisAllocatorBoundOrgan<T>>;
static_assert(mp::mp_all_of<Family01bOrgans, is_axis_bound>::value,
              "S5-01b: ein Kompositions-Organ hat seinen Form-B-Ausweis (allocator_type) verloren.");
static_assert(mp::mp_all_of<Family01bHulls, is_axis_bound>::value,
              "S5-01b: eine Huelle hat den weitergereichten Ausweis verloren.");
static_assert(mp::mp_all_of<FlatOrgans, is_axis_bound>::value && mp::mp_all_of<FlatHulls, is_axis_bound>::value,
              "S5-01b: der flache Pfad hat seinen Ausweis verloren.");

/// Und die Bindung ist die ACHSEN-Default-Strategie, nicht irgendein Typ, der zufaellig das Concept erfuellt.
/// (Deklarierter Zwischenstand, LEDGER 04.08. abend-11: die Kompositions-Durchbindung ist 01c-Design.)
template <class T>
using is_axis_default_bound = mp::mp_bool<std::is_same_v<typename T::allocator_type, alx::ExgenAllocator>>;
static_assert(mp::mp_all_of<Family01bOrgans, is_axis_default_bound>::value,
              "S5-01b: ein Organ haengt nicht mehr am benannten Achsen-Default (ExgenAllocator).");
static_assert(mp::mp_all_of<Family01bHulls, is_axis_default_bound>::value,
              "S5-01b: eine Huelle meldet einen anderen Allokator als ihr Organ -- die Weiterleitung ist kaputt.");

/// Die HUELLE fuehrt kein eigenes Substrat ein (01a-Wache, hier fuer die Ausweis-Ebene wiederholt).
template <class Organ>
using hull_wraps_exactly = mp::mp_bool<std::is_same_v<typename HullOf<Organ>::container_type, Organ>>;
static_assert(mp::mp_all_of<Family01bOrgans, hull_wraps_exactly>::value,
              "S5-01b: eine Organ-Huelle wrappt nicht mehr genau ihr Organ.");

// -- (3) FORM A: die CT-Kappen der drei Trie-Organe sind aus dem TYP abgeleitet, nie Literale ---------
// Die Kappe MUSS mit der Schluesselbreite mitwachsen. Ein hartes 9 bzw. 65 waere genau die Sorte Literal,
// die bei einer spaeteren Key-Breiten-Aenderung (#217-2b) still zu einem Ueberlauf wuerde.
static_assert(lkc::ArtTrieOrgan::kMaxDescent == sizeof(lkc::ArtTrieOrgan::key_type) + 1U,
              "S5-01b: die ART-Kappe ist kein typ-abgeleiteter Ausdruck mehr (ein Byte je Inner-Ebene).");
static_assert(lkc::StartTrieOrgan::kMaxDescent == sizeof(lkc::StartTrieOrgan::key_type) + 1U,
              "S5-01b: die START-Kappe ist kein typ-abgeleiteter Ausdruck mehr (>= ein Byte je Inner-Ebene).");
static_assert(lkc::HotPatriciaOrgan::kMaxDescent == sizeof(lkc::HotPatriciaOrgan::key_type) * 8U + 1U,
              "S5-01b: die HOT-Kappe ist kein typ-abgeleiteter Ausdruck mehr (ein BIT je Inner-Ebene).");
/// Und die Kappen sind klein genug, um wirklich inline zu liegen -- eine Kappe von Tiefe*Fanout (die alte
/// Walk-Form) waere formal auch "CT-beschraenkt" gewesen und haette trotzdem Kilobytes auf den Rahmen gelegt.
static_assert(lkc::ArtTrieOrgan::kMaxDescent <= 16U && lkc::StartTrieOrgan::kMaxDescent <= 16U &&
                  lkc::HotPatriciaOrgan::kMaxDescent <= 72U,
              "S5-01b: eine CT-Kappe ist aus dem Rahmen gelaufen -- dann ist die Walk-Form wieder "
              "fanout-proportional statt tiefen-proportional.");

/// Die Form-B-Organe fuehren einen NACHWEISBAREN Walk-Zaehler; die Form-A-Organe fuehren KEINEN --
/// gerade weil sie nichts allozieren. Beides wird gepinnt, damit die Aufteilung nicht still kippt.
template <class T>
concept HasWalkStats = requires(T const& t) { t.walk_allocator_statistics(); };
static_assert(HasWalkStats<lkc::BstTreeOrgan> && HasWalkStats<lkc::BTreeSearchOrgan>,
              "S5-01b: ein Form-B-Organ meldet seinen Walk-Zaehler nicht mehr -- dann ist Form B wieder eine "
              "blosse Behauptung ohne Beleg.");
static_assert(!HasWalkStats<lkc::ArtTrieOrgan> && !HasWalkStats<lkc::StartTrieOrgan> &&
                  !HasWalkStats<lkc::HotPatriciaOrgan>,
              "S5-01b: ein Form-A-Organ meldet ploetzlich einen Walk-Zaehler -- dann hat es wieder Heap "
              "bekommen und die Form-A-Aussage stimmt nicht mehr.");
/// Die Huelle reicht den Walk-Zaehler weiter (Memory-Regel), aber nur dort, wo es ihn gibt.
static_assert(HasWalkStats<HullOf<lkc::BstTreeOrgan>> && !HasWalkStats<HullOf<lkc::ArtTrieOrgan>>,
              "S5-01b: die Huelle reicht den Walk-Zaehler nicht formtreu durch.");

// -- Literaler Lauf-Ausweis --------------------------------------------------------------------------
int g_fail = 0;

void tr(char const* what, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what);
    if (!ok) ++g_fail;
}

template <class T>
void report_one(char const* stufe, std::string_view label) {
    bool const conform = s5::FamilyAllocConform<T>;
    std::printf("  [%s] %-6s %-26.*s Form: %s\n", conform ? " ok " : "FAIL", stufe, static_cast<int>(label.size()),
                label.data(), s5::family_alloc_form<T>());
    if (!conform) ++g_fail;
}

template <class W>
using OrganOfWrapper = organ_of<W>;
template <class W>
using HullOfWrapper = HullOf<organ_of<W>>;

template <template <class> class Pick>
void report_registry_level(char const* stufe) {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, RegistryWrappers>>([&](auto id) {
        using W = typename decltype(id)::type;
        report_one<Pick<W>>(stufe, W::name());
    });
}

[[nodiscard]] std::uint64_t spread(std::uint64_t i) noexcept {
    return (i * 2654435761ull) ^ (i << 13) ^ 0x9E3779B97F4A7C15ull;
}

/// (4) FORM-B-VERDRAHTUNGS-BELEG. Der Walk-Zaehler wird VOR und NACH DERSELBEN Op-Schleife gelesen; der
/// Beleg ist das DELTA. Wichtig: gelesen wird VOR jedem Restore -- ein Zaehler, den man erst nach einem
/// Memento-Restore ansieht, ist per Konstruktion wieder auf dem Ausgangsstand und beweist nichts.
template <class Organ>
void probe_walk_wiring(char const* label, std::uint64_t n_ops) {
    Organ organ{};

    // (6) EHRLICHER NULLPUNKT: das frische Organ hat noch NICHT gelaufen -> Walk-Zaehler 0.
    auto const cold = organ.walk_allocator_statistics();
    std::printf("     %-16s walk alloc_cnt kalt %llu\n", label, static_cast<unsigned long long>(cold.allocation_count));
    tr("(6) frisches Organ: Walk-Zaehler ist ehrlich 0 (kein Phantom-Wert)", cold.allocation_count == 0);

    for (std::uint64_t k = 0; k < n_ops; ++k) organ.insert(spread(k), k * 7u + 1u);

    // Auch nach dem Fuellen darf der WALK-Zaehler noch 0 sein: Inserts gehen ins Substrat, nicht in den
    // Walk-Puffer. Das ist zugleich die Trennschaerfe gegen die T6-Doppelzaehlung (Posten 68).
    auto const before = organ.walk_allocator_statistics();
    tr("(4) nach dem Fuellen ist der WALK-Zaehler unbewegt (Substrat != Walk-Speicher, keine T6-Doppelzaehlung)",
       before.allocation_count == cold.allocation_count);

    std::uint64_t     harvested = 0;
    std::size_t const visited   = organ.for_each_record([&](std::uint64_t, std::uint64_t v) { harvested += v; });
    auto const        after     = organ.walk_allocator_statistics();

    std::printf("     %-16s walk alloc_cnt %llu -> %llu   bytes_in_use %llu   visited %zu\n", label,
                static_cast<unsigned long long>(before.allocation_count),
                static_cast<unsigned long long>(after.allocation_count),
                static_cast<unsigned long long>(after.total_bytes_in_use), visited);

    tr("(4) der Walk laeuft REAL ueber die Achse (Delta des Walk-Zaehlers > 0)",
       after.allocation_count > before.allocation_count);
    tr("(4) und er hat wirklich gearbeitet (jeder Record genau einmal besucht)",
       visited == organ.occupied_count() && visited == n_ops);
    tr("(4) Fehlschlaege werden ehrlich gezaehlt und sind hier 0", after.failure_count == 0);
    tr("(4) die Ernte ist nicht leer (der Sink wurde wirklich gerufen)", harvested > 0);
}

/// (3b) FORM-A-BELEG zur LAUFZEIT: das Trie-Organ hat KEINEN Walk-Zaehler (static_assert oben), also wird
/// hier belegt, dass der Walk trotzdem vollstaendig ist -- die staerkere Aussage "gar keine Allokation"
/// darf nicht mit "gar kein Walk" verwechselt werden.
template <class Organ>
void probe_heapfree_walk(char const* label, std::uint64_t n_ops) {
    Organ organ{};
    for (std::uint64_t k = 0; k < n_ops; ++k) organ.insert(spread(k), k * 7u + 1u);
    std::uint64_t     sum     = 0;
    std::size_t const visited = organ.for_each_record([&](std::uint64_t, std::uint64_t v) { sum += v; });
    std::printf("     %-16s heap-freier Walk: visited %zu / occupied %zu\n", label, visited, organ.occupied_count());
    tr("(3b) heap-freier Walk besucht jeden Record genau einmal", visited == organ.occupied_count());
    tr("(3b) und der Bestand ist vollstaendig angekommen", visited == n_ops && sum > 0);
}

/// (5) Die beiden ANDEREN Schalen-Puffer: Memento und Scan-Zwischenablage. Beide melden ihre eigene
/// Achsen-Statistik -- der Beleg, dass composable_search.hpp nicht nur einen Typ-Namen geaendert hat.
void probe_shell_buffers() {
    using Organ = lkc::SortedBinaryOrgan;
    Organ organ{};
    for (std::uint64_t k = 0; k < 256; ++k) organ.insert(spread(k), k * 7u + 1u);

    auto const memento = organ.save_state();
    auto const mstats  = memento.allocator_statistics();
    std::printf("     %-16s memento size %zu   alloc_cnt %llu   bytes_in_use %llu\n", "memento", memento.size(),
                static_cast<unsigned long long>(mstats.allocation_count),
                static_cast<unsigned long long>(mstats.total_bytes_in_use));
    tr("(5) der Memento-Puffer alloziert REAL ueber die Achse (alloc_cnt > 0)", mstats.allocation_count > 0);
    tr("(5) und er haelt den Speicher auch (bytes_in_use > 0)", mstats.total_bytes_in_use > 0);
    tr("(5) der Memento traegt den vollen Bestand", memento.size() == 256u);

    // Der Memento ueberlebt sein Organ -- genau das tut er im abi_adapter (er liegt dort als MEMBER).
    // Deshalb MUSS er seine Strategie selbst besitzen; eine Kopie darf nicht an die Quelle gekettet sein.
    auto const memento_copy = memento;
    tr("(5) die Memento-KOPIE ist inhaltsgleich (Rebind hat den Bestand nicht verloren)",
       memento_copy.size() == memento.size());
    tr("(5) und sie traegt exakt den Quell-Zaehler (Kopier-Pollution per Memento-Restore verworfen)",
       memento_copy.allocator_statistics().allocation_count == mstats.allocation_count);

    // Scan-Zwischenablage des LinearScan-Organs: eigener Typ, eigener Zaehler.
    using Scratch = lkc::LinearScanTraversal::scan_scratch_t<lkc::RawSlotStore<>>;
    Scratch    scratch{};
    auto const cold = scratch.allocator_statistics();
    tr("(6) frische Scan-Zwischenablage: ehrliche 0", cold.allocation_count == 0);
    for (std::uint64_t k = 0; k < 128; ++k) scratch.emplace_back(spread(k), k);
    auto const warm = scratch.allocator_statistics();
    std::printf("     %-16s scan-scratch alloc_cnt %llu -> %llu   size %zu\n", "scan", 0ull,
                static_cast<unsigned long long>(warm.allocation_count), scratch.size());
    tr("(5) die Scan-Zwischenablage alloziert REAL ueber die Achse", warm.allocation_count > 0);

    // Und der Pilot-Store selbst meldet seine Achsen-Statistik (er war 01a-unsichtbar).
    lkc::RawSlotStore<> store;
    auto const          s0 = store.store_allocator_statistics();
    for (std::uint64_t k = 0; k < 512; ++k) store.append_slot(spread(k), k);
    auto const s1 = store.store_allocator_statistics();
    std::printf("     %-16s alloc_cnt %llu -> %llu   slots %zu\n", "RawSlotStore<>",
                static_cast<unsigned long long>(s0.allocation_count),
                static_cast<unsigned long long>(s1.allocation_count), store.slot_count());
    tr("(6) frischer Pilot-Store: ehrliche 0", s0.allocation_count == 0);
    tr("(5) der Pilot-Store fuehrt seinen Slot-Speicher REAL ueber die Achse", s1.allocation_count > 0);
    tr("(5) und traegt die Daten wirklich", store.slot_count() == 512u);
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 01b -- composable-Rest: Allokator-Achsen-Konformitaet ==\n");
    std::printf("   Achsen: search_algo (T0, Kompositions-Schale) . allocator (T6, Versorger)\n");
    std::printf("   Typ-Liste ABGELEITET: AllStrategies -> organ_for_search_algo_t (+ FLACH + DEFERRED)\n");

    std::printf("-- (1) Kompositions-Organe (der Scrub-Gegenstand dieser Scheibe) --\n");
    report_registry_level<OrganOfWrapper>("ORGAN");
    report_one<lkc::MasstreeOrgan>("ORGAN", "masstree (DEFERRED)");
    report_one<lkc::LinearScanOrgan>("ORGAN", "linear_scan (FLACH)");
    report_one<lkc::SortedBinaryOrgan>("ORGAN", "sorted_binary (FLACH)");
    report_one<lkc::InterpolationOrgan>("ORGAN", "interpolation (FLACH)");
    report_one<lkc::RawSlotStore<>>("STORE", "RawSlotStore<> (Pilot)");

    std::printf("-- (2) Organ-Huellen (container_t des ABI-Adapters; in 01a nur AUSWEIS, jetzt Aussage) --\n");
    report_registry_level<HullOfWrapper>("HULL");
    report_one<HullOf<lkc::MasstreeOrgan>>("HULL", "masstree (DEFERRED)");
    report_one<lkc::ObservableComposedSearch<lkc::SortedBinaryTraversal, lkc::RawSlotStore<>>>("HULL",
                                                                                               "sorted_binary (FLACH)");

    std::printf("-- (3b) FORM A zur Laufzeit: heap-freier Walk der drei Trie-Familien --\n");
    probe_heapfree_walk<lkc::ArtTrieOrgan>("art_trie", 512);
    probe_heapfree_walk<lkc::HotPatriciaOrgan>("hot_patricia", 512);
    probe_heapfree_walk<lkc::StartTrieOrgan>("start_trie", 512);

    std::printf("-- (4) FORM B zur Laufzeit: Walk-Verdrahtung der Vergleichsbaum-Familien --\n");
    probe_walk_wiring<lkc::BstTreeOrgan>("bst_tree", 512);
    probe_walk_wiring<lkc::BTreeSearchOrgan>("btree", 512);

    std::printf("-- (5)/(6) Schalen-Puffer: Memento, Scan-Zwischenablage, Pilot-Store --\n");
    probe_shell_buffers();

    std::printf("-- Familien-Bilanz --\n");
    std::printf("  Organe (Registry+DEFERRED): %zu   davon Form A heap-freier Walk: 3   Form B Walk: 2\n",
                static_cast<std::size_t>(mp::mp_size<Family01bOrgans>::value));
    std::printf("  Flache Kompositionen: %zu   Huellen-Klassen geprueft: 2\n",
                static_cast<std::size_t>(mp::mp_size<FlatOrgans>::value));

    std::printf("== test_s5_01b_composable_alloc_conformance: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
