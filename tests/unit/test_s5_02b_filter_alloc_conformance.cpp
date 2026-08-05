// A8-S5 Familie 02_layout, Sub-Scheibe 02b (filter / SuRF) -- FAMILIEN-KONFORMITAETS-WACHE.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4 + 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity").
// Das wiederverwendbare Praedikat liegt in s5_family_alloc_conformance.hpp (Pilot 04_execution); DIESE TU
// besteht aus den Typ-Listen der Sub-Familie, dem literalen Lauf-Ausweis -- und, weil die Familie Form B
// nutzt, zusaetzlich aus dem REALEN VERDRAHTUNGS-BELEG (Block (4)), dem VERSCHACHTELUNGS-BELEG beider
// Ebenen (Block (5)) und dem PAPERTREUE-BELEG (Block (6)).
//
// GELTUNGSBEREICH: 02b ist die filter-/SuRF-Seite der Organ-Gruppe 02_layout (lager_baum_writer.hpp
// kOrganGruppe02 = node_type, memory_layout, path_compression, filter, serialization). Die uebrigen vier
// Achsen der Gruppe sind Sub-Scheibe 02a und werden dort gepinnt -- diese TU pinnt sie NICHT noch einmal
// (zwei Wachen ueber demselben Gegenstand driften auseinander; die 02a-Wache bleibt die Quelle fuer 02a).
//
// ZWEI EBENEN DER filter-ACHSE, und sie sind NICHT dasselbe -- das ist die Besonderheit dieser Scheibe:
//   (I)  die KOMPOSITIONS-getragene filter-Achse = axis_filter_registry.hpp (Bloom/Cuckoo/RangeSurf/Xor).
//        Diese Strategien sind heap-frei (std::array-Bitmaps) und erfuellen Form (A). Sie liegen im
//        T14-Mess-Pfad und wurden vom Scrub NICHT angefasst -- die Wache pinnt sie trotzdem, damit ein
//        kuenftiger Container dort sofort auffaellt (Nachweis-Charakter, wie 02a es fuer memory_layout tut).
//   (II) die COMPOSABLE Filter-Organe (S1 exakt / S2 succinct LOUDS, axes/filter_axis/composable/).
//        DAS ist der Gegenstand des 02b-Scrubs: sie tragen unbeschraenkt wachsende Puffer, erfuellen also
//        Form (B), und ihre Verdrahtung wird hier zusaetzlich real belegt.
//
// WARUM (4)/(5) HIER STEHEN MUESSEN (Form-B-Grenze, s5_family_alloc_conformance.hpp:31): das Praedikat
// prueft, dass ein DEKLARIERTER allocator_type das Achsen-Concept erfuellt -- nicht, dass die
// Member-Allokation real darueber laeuft. Fuer diese Scheibe ist zusaetzlich die VERSCHACHTELUNG scharf:
// die per-Level-Container des LOUDS-Builders sind zweistufig, und eine allokator-erweiterte Kopie des
// AEUSSEREN Vektors rebindet nur DEN -- die inneren Vektoren braechten ueber allocator_traits::construct
// ihren Quell-Allokator mit (uses-allocator-Konstruktion greift ohne scoped_allocator_adaptor NICHT).
// Genau diese Luecke schliesst Block (5), und zwar mit einer Aussage, die am Alt-Stand FAELLT.
//
// PAPERTREUE = MESSAUSSAGE (Block (6), 02b-spezifisch): die SuRF-Organe sind eine original-getreue
// Portierung aus ext/traversal/P10-SuRF. Ein Scrub, der die Membership-Semantik verschoebe, wuerde die
// Filter-Achse messbar machen, aber falsch. Die Wache verlangt deshalb die beiden strukturellen Zusagen
// des Papers am Objekt: NO-FALSE-NEGATIVE (jeder gebaute Key wird gefunden) und der KREUZBELEG gegen die
// exakte S1-Ground-Truth (S2.contains(k) >= S1.contains(k)).
//
// Standalone (plain int main, KEIN gtest) -- konsistent mit den uebrigen Phase-E-Standalone-Wachen.

#include "s5_family_alloc_conformance.hpp"

#include <axes/filter_axis/axis_filter_observable.hpp>
#include <axes/filter_axis/axis_filter_registry.hpp>
#include <axes/filter_axis/composable/exact_prefix_filter_organ.hpp>
#include <axes/filter_axis/composable/exact_prefix_filter_store.hpp>
#include <axes/filter_axis/composable/louds_sparse_filter_organ.hpp>
#include <axes/filter_axis/composable/louds_sparse_filter_store.hpp>
#include <axes/filter_axis/composable/surf_axis_allocator.hpp>
#include <axes/filter_axis/composable/surf_louds_bitvector.hpp>
#include <axes/filter_axis/composable/surf_suffix_bits.hpp>

#include <boost/mp11.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <set>
#include <type_traits>
#include <vector>

namespace mp   = boost::mp11;
namespace s5   = ::comdare::cache_engine::tests::s5;
namespace fx   = ::comdare::cache_engine::filter_axis;
namespace fcmp = ::comdare::cache_engine::filter_axis::composable;

namespace {

int  g_fail = 0;
void tr(char const* what, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what);
    if (!ok) ++g_fail;
}

// -- (I) die KOMPOSITIONS-getragene filter-Achse: Liste AUS DER REGISTRY, nie handgepflegt ------------
using AllFilterStrategies = fx::AllFilters;
using AllFilterHulls      = mp::mp_transform<fx::ObservableFilter, AllFilterStrategies>;

static_assert(mp::mp_size<AllFilterStrategies>::value > 0,
              "S5-02b: die filter-Registry ist leer -- die Wache liefe ins Leere (Anti-Leerlauf).");
static_assert(mp::mp_size<AllFilterHulls>::value == mp::mp_size<AllFilterStrategies>::value,
              "S5-02b: die Huellen-Liste ist nicht aus der Strategie-Liste abgeleitet.");

// -- (II) die COMPOSABLE Filter-Organe: der Gegenstand DIESES Scrubs ----------------------------------
using S1Store = fcmp::ExactPrefixFilterStore;
using S1Organ = fcmp::ComposedExactSurfFilter<fcmp::ExactPrefixFilterQuery, S1Store>;

template <fcmp::SurfSuffixType ST, unsigned H, unsigned R>
using S2Store = fcmp::LoudsSparseFilterStore<ST, H, R>;
template <fcmp::SurfSuffixType ST, unsigned H, unsigned R>
using S2Organ = fcmp::ComposedSurfLoudsFilter<ST, H, R>;

// Die realen Tuning-Punkte der Achse (ST/HashLen/RealLen sind COMPILE-TIME-Parameter, keine Runtime-Wahl):
// der Default kReal/8 (louds_sparse_filter_organ.hpp:131), der Nullpunkt kNone und die beiden anderen
// Suffix-Familien. Jeder Punkt instanziiert einen EIGENEN Store-Typ -- die Wache pinnt sie alle.
using ScrubbedStores = mp::mp_list<S1Store,                                      //
                                   S2Store<fcmp::SurfSuffixType::kNone, 0, 0>,   //
                                   S2Store<fcmp::SurfSuffixType::kReal, 0, 8>,   //
                                   S2Store<fcmp::SurfSuffixType::kReal, 0, 16>,  //
                                   S2Store<fcmp::SurfSuffixType::kHash, 8, 0>,   //
                                   S2Store<fcmp::SurfSuffixType::kMixed, 4, 8>>; //
using ScrubbedOrgans = mp::mp_list<S1Organ,                                      //
                                   S2Organ<fcmp::SurfSuffixType::kNone, 0, 0>,   //
                                   S2Organ<fcmp::SurfSuffixType::kReal, 0, 8>,   //
                                   S2Organ<fcmp::SurfSuffixType::kMixed, 4, 8>>; //
// Die SUBSTRATE, die die Stores intern halten (der eigentliche B-5-Bestand der Scheibe: 10 Treffer in
// surf_louds_bitvector.hpp + der Suffix-Puffer). Sie sind KEINE Organe im Registry-Sinn, tragen aber die
// Puffer -- ohne sie waere die Wache blind fuer den groesseren Teil des Fundes.
using ScrubbedSubstrates = mp::mp_list<fcmp::SurfBitVector, fcmp::SurfRank, fcmp::SurfSelect,
                                       fcmp::SurfSuffixBits<fcmp::SurfSuffixType::kReal, 0, 8>,
                                       fcmp::SurfSuffixBits<fcmp::SurfSuffixType::kNone, 0, 0>>;

static_assert(mp::mp_size<ScrubbedStores>::value == 6, "S5-02b: Store-Liste unerwartet gross/klein.");
static_assert(mp::mp_size<ScrubbedSubstrates>::value == 5, "S5-02b: Substrat-Liste unerwartet gross/klein.");

// -- Der EINE Ausdruck der Wache, je Ebene -----------------------------------------------------------
static_assert(s5::family_alloc_conform_v<AllFilterStrategies>,
              "S5-02b: eine Strategie der KOMPOSITIONS-filter-Achse fuehrt Speicher am Allokator-Achsen-"
              "Interface vorbei (weder heap-frei noch ueber die Achse).");
static_assert(s5::family_alloc_conform_v<AllFilterHulls>,
              "S5-02b: eine Observable-Huelle der filter-Achse fuehrt Speicher am Achsen-Interface vorbei.");
static_assert(s5::family_alloc_conform_v<ScrubbedStores>,
              "S5-02b: ein composable Filter-STORE fuehrt Speicher am Allokator-Achsen-Interface vorbei.");
static_assert(s5::family_alloc_conform_v<ScrubbedOrgans>,
              "S5-02b: ein composable Filter-ORGAN fuehrt Speicher am Allokator-Achsen-Interface vorbei.");
static_assert(s5::family_alloc_conform_v<ScrubbedSubstrates>,
              "S5-02b: ein SuRF-Substrat (BitVector/Rank/Select/SuffixBits) fuehrt Speicher am "
              "Allokator-Achsen-Interface vorbei.");

// -- (3) TYP-BEWEIS der VERSCHACHTELUNG: BEIDE Ebenen tragen den Adapter DERSELBEN Achsen-Strategie --
// Das ist der compile-harte Teil des 02b-Herzstuecks. Am Alt-Stand waren beide Ebenen
// std::vector<..., std::allocator<...>> -- jede dieser Zeilen faellt dort durch.
using HerzStore = S2Store<fcmp::SurfSuffixType::kReal, 0, 8>;

static_assert(std::is_same_v<typename HerzStore::level_index_vector_type::allocator_type,
                             fcmp::surf_axis_adapter_t<typename HerzStore::level_element_vector_type>>,
              "S5-02b: die AEUSSERE Ebene des verschachtelten Level-Index haengt nicht am Achsen-Adapter.");
static_assert(std::is_same_v<typename HerzStore::level_element_vector_type::allocator_type,
                             fcmp::surf_axis_adapter_t<std::uint8_t>>,
              "S5-02b: die INNERE Ebene (ein Label-Level) haengt nicht am Achsen-Adapter -- genau der Fehler, "
              "den uses-allocator-Konstruktion ohne scoped_allocator_adaptor erzeugt.");
static_assert(std::is_same_v<typename HerzStore::word_level_index_vector_type::allocator_type,
                             fcmp::surf_axis_adapter_t<typename HerzStore::word_level_element_vector_type>>,
              "S5-02b: die AEUSSERE Ebene der LOUDS-/Child-Wortlevel haengt nicht am Achsen-Adapter.");
static_assert(std::is_same_v<typename HerzStore::word_level_element_vector_type::allocator_type,
                             fcmp::surf_axis_adapter_t<std::uint64_t>>,
              "S5-02b: die INNERE Ebene der LOUDS-/Child-Wortlevel haengt nicht am Achsen-Adapter.");
static_assert(std::is_same_v<typename HerzStore::allocator_type, fcmp::filter_surf_allocator_t> &&
                  std::is_same_v<typename S1Store::allocator_type, fcmp::filter_surf_allocator_t>,
              "S5-02b: die beiden Organe fuehren nicht DENSELBEN benannten Achsen-Versorger -- ein zweiter "
              "Name kann driften (surf_axis_allocator.hpp ist die EINE Stelle).");

// -- ANTI-STILL-ZURUECKFALLEN: der Adapter darf NICHT default-konstruierbar sein ----------------------
// Waere er es, koennte irgendein Puffer still ohne Strategie-Instanz entstehen. Die Nicht-Default-
// Konstruierbarkeit ist die compile-harte Absicherung, auf die sich der Lebensdauer-Vertrag der Header
// beruft -- hier wird sie belegt statt behauptet.
static_assert(!std::is_default_constructible_v<fcmp::surf_axis_adapter_t<std::uint64_t>>,
              "S5-02b: der Achsen-Adapter ist default-konstruierbar geworden -- ein Puffer koennte still "
              "ohne Strategie-Instanz entstehen.");
// Und: die achsen-gebundenen Substrate duerfen NICHT gewoehnlich kopierbar sein (eine gewoehnliche Kopie
// schleppte den Adapter der QUELLE mit = dangling, sobald die Quelle stirbt).
static_assert(!std::is_copy_constructible_v<fcmp::SurfBitVector> && !std::is_copy_constructible_v<fcmp::SurfRank> &&
                  !std::is_copy_constructible_v<fcmp::SurfSelect> &&
                  !std::is_copy_constructible_v<fcmp::SurfSuffixBits<fcmp::SurfSuffixType::kReal, 0, 8>>,
              "S5-02b: ein SuRF-Substrat ist gewoehnlich kopierbar -- die Kopie brachte den Fremd-Adapter mit.");
// Die ORGANE dagegen MUESSEN kopierbar bleiben (die Kopie rebindet, s. Block (5)).
static_assert(std::is_copy_constructible_v<HerzStore> && std::is_copy_assignable_v<HerzStore> &&
                  std::is_copy_constructible_v<S1Store> && std::is_copy_assignable_v<S1Store>,
              "S5-02b: ein Filter-Store hat seine Kopierbarkeit verloren.");

// -- Lauf-Ausweis: welche Form welches Mitglied erfuellt ---------------------------------------------
template <class List>
void print_forms(char const* ebene) {
    std::printf("   %s:\n", ebene);
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([](auto id) {
        using T = typename decltype(id)::type;
        std::printf("     Form %-24s  (sizeof=%zu)\n", s5::family_alloc_form<T>(), sizeof(T));
    });
}

// -- Test-Keys (identisch zu den bestehenden Verhaltens-Tests, damit der Beleg vergleichbar bleibt) --
std::vector<std::uint64_t> build_keys(std::set<std::uint64_t>& gt) {
    for (std::uint64_t i = 0; i < 800; ++i) gt.insert((i * 2654435761u) % 100000u);
    return {gt.begin(), gt.end()};
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 02b (filter / SuRF) -- Allokator-Achsen-Konformitaet ==\n");

    // -- (1)+(2) TYP-EBENE: die static_asserts oben sind bereits gelaufen (compile-hart). ------------
    std::printf("-- (1) KOMPOSITIONS-filter-Achse (Registry-abgeleitet, %zu Strategien + %zu Huellen) --\n",
                mp::mp_size<AllFilterStrategies>::value, mp::mp_size<AllFilterHulls>::value);
    print_forms<AllFilterStrategies>("Strategien");
    print_forms<AllFilterHulls>("Observable-Huellen");
    std::printf("-- (2) COMPOSABLE Filter-Organe (der Scrub-Gegenstand) --\n");
    print_forms<ScrubbedStores>("Stores");
    print_forms<ScrubbedOrgans>("Organe");
    print_forms<ScrubbedSubstrates>("SuRF-Substrate");
    tr("(1)+(2) Typ-Ebene: alle Listen erfuellen (A) heap-frei ODER (B) ueber die Allokator-Achse "
       "(compile-hart, s. static_asserts)",
       true);
    tr("(3) Verschachtelung compile-hart: BEIDE Ebenen des Level-Index tragen den Achsen-Adapter", true);

#ifdef COMDARE_CE_ENABLE_STATISTICS
    std::set<std::uint64_t> gt;
    auto const              keys = build_keys(gt);

    // -- (4) LAUFZEIT-BELEG der REALEN Verdrahtung ---------------------------------------------------
    // Form B behauptet nur einen deklarierten allocator_type. Hier wird am OBJEKT belegt, dass die
    // Puffer wirklich ueber die Strategie laufen. AM ALT-STAND WAERE JEDER DIESER ZAEHLER 0 GEWESEN --
    // dort beruehrte KEIN Puffer dieser Familie die Achse.
    HerzStore s2;
    S1Store   s1;
    s2.build_from_sorted_keys(keys);
    s1.build_from_sorted_keys(keys);
    auto const st2 = s2.filter_surf_allocator_statistics();
    auto const st1 = s1.filter_surf_allocator_statistics();
    std::printf("-- (4) REALE Verdrahtung am Objekt (keys=%zu) --\n", keys.size());
    std::printf(
        "     S2 (LOUDS kReal/8): alloc_cnt=%llu dealloc_cnt=%llu bytes_alloc=%llu in_use=%llu fail=%llu\n",
        static_cast<unsigned long long>(st2.allocation_count), static_cast<unsigned long long>(st2.deallocation_count),
        static_cast<unsigned long long>(st2.total_bytes_allocated),
        static_cast<unsigned long long>(st2.total_bytes_in_use), static_cast<unsigned long long>(st2.failure_count));
    std::printf(
        "     S1 (exakt):         alloc_cnt=%llu dealloc_cnt=%llu bytes_alloc=%llu in_use=%llu fail=%llu\n",
        static_cast<unsigned long long>(st1.allocation_count), static_cast<unsigned long long>(st1.deallocation_count),
        static_cast<unsigned long long>(st1.total_bytes_allocated),
        static_cast<unsigned long long>(st1.total_bytes_in_use), static_cast<unsigned long long>(st1.failure_count));

    tr("(4) S2: der LOUDS-Bau laeuft REAL ueber die Achse (allocation_count > 0 -- am Alt-Stand 0)",
       st2.allocation_count > 0);
    tr("(4) S2: und der Speicher steht danach auch WIRKLICH auf der Achse (total_bytes_in_use > 0)",
       st2.total_bytes_in_use > 0);
    tr("(4) S1: der exakte Key-Puffer laeuft REAL ueber die Achse (allocation_count > 0 -- am Alt-Stand 0)",
       st1.allocation_count > 0);
    tr("(4) kein Fehlschlag-Pfad im Normalbetrieb (failure_count == 0 bei beiden)",
       st2.failure_count == 0 && st1.failure_count == 0);
    // DISKRIMINIEREND (kein blosses ">0"): das succinct LOUDS-Organ haelt VIER per-Level-Container plus
    // Rank-/Select-/Suffix-/Flach-Puffer, das exakte S1-Organ genau EINEN Key-Vektor. Waere nur die
    // aeussere Ebene an der Achse (der klassische Verschachtelungs-Fehler), lieferte S2 kaum mehr
    // Allokations-Ereignisse als S1. Der Abstand IST die Aussage.
    tr("(4) DISKRIMINIEREND: S2 (verschachtelt, 4 Level-Container + Rank/Select/Suffix) zeigt deutlich MEHR "
       "Achsen-Allokationen als S1 (ein einziger Key-Vektor)",
       st2.allocation_count > st1.allocation_count + 4);

    // -- (5) VERSCHACHTELUNGS-BELEG: die Kopie rebindet BEIDE Ebenen --------------------------------
    // DER DETEKTOR (Muster 01a, "Quell-Zaehler bewegen sich bei Mutation der Kopie NICHT"): eine Kopie
    // anlegen und dann die KOPIE mutieren. Traege auch nur EIN innerer Level-Vektor der Kopie noch den
    // Adapter der QUELLE, dann liefe seine Freigabe (clear() im Bulk-Load) und seine Neu-Allokation gegen
    // die Strategie-Instanz der QUELLE -- deren Zaehler bewegten sich. Genau das ist der Fehler, den
    // uses-allocator-Konstruktion ohne scoped_allocator_adaptor erzeugt, und er ist mit einer
    // Zaehler-GLEICHHEIT (nicht mit einem ">0") gepinnt.
    std::set<std::uint64_t> gt2;
    for (std::uint64_t i = 0; i < 1200; ++i) gt2.insert((i * 40503u) % 65521u + 7u);
    std::vector<std::uint64_t> const keys2(gt2.begin(), gt2.end());

    HerzStore  cpy            = s2;
    auto const cpy_after_copy = cpy.filter_surf_allocator_statistics();
    auto const src_before_mut = s2.filter_surf_allocator_statistics();

    std::size_t missing_in_copy_before = 0;
    for (std::uint64_t const k : gt)
        if (!fcmp::SurfLoudsQuery::contains_in(cpy, k)) ++missing_in_copy_before;

    cpy.build_from_sorted_keys(keys2); // MUTATION DER KOPIE (clear + Neu-Bau, beide Ebenen)

    auto const src_after_mut = s2.filter_surf_allocator_statistics();
    auto const cpy_after_mut = cpy.filter_surf_allocator_statistics();

    std::size_t missing_in_src_after = 0;
    for (std::uint64_t const k : gt)
        if (!fcmp::SurfLoudsQuery::contains_in(s2, k)) ++missing_in_src_after;
    std::size_t missing_in_copy_after = 0;
    for (std::uint64_t const k : gt2)
        if (!fcmp::SurfLoudsQuery::contains_in(cpy, k)) ++missing_in_copy_after;

    std::printf("-- (5) VERSCHACHTELUNGS-BELEG (Kopie rebindet BEIDE Ebenen) --\n");
    std::printf("     Kopie nach copy:      alloc_cnt=%llu in_use=%llu   fehlende Keys=%zu\n",
                static_cast<unsigned long long>(cpy_after_copy.allocation_count),
                static_cast<unsigned long long>(cpy_after_copy.total_bytes_in_use), missing_in_copy_before);
    std::printf("     Quelle vor/nach Kopie-Mutation: alloc_cnt=%llu/%llu  dealloc_cnt=%llu/%llu\n",
                static_cast<unsigned long long>(src_before_mut.allocation_count),
                static_cast<unsigned long long>(src_after_mut.allocation_count),
                static_cast<unsigned long long>(src_before_mut.deallocation_count),
                static_cast<unsigned long long>(src_after_mut.deallocation_count));
    std::printf("     Kopie nach Mutation:  alloc_cnt=%llu (bewegt sich) fehlende Keys(neu)=%zu   "
                "Quelle fehlende Keys(alt)=%zu\n",
                static_cast<unsigned long long>(cpy_after_mut.allocation_count), missing_in_copy_after,
                missing_in_src_after);

    tr("(5) die Kopie haelt EIGENEN Speicher auf IHRER Strategie-Instanz (in_use > 0)",
       cpy_after_copy.total_bytes_in_use > 0);
    tr("(5) die Kopie antwortet unmittelbar nach dem Kopieren vollstaendig (0 fehlende Keys)",
       missing_in_copy_before == 0);
    tr("(5) DETEKTOR (beide Ebenen): die Mutation der KOPIE bewegt die Allokations-Zaehler der QUELLE NICHT "
       "-- kein innerer Level-Vektor trug den Fremd-Adapter",
       src_after_mut.allocation_count == src_before_mut.allocation_count &&
           src_after_mut.deallocation_count == src_before_mut.deallocation_count);
    tr("(5) die Mutation hat wirklich stattgefunden (die Zaehler der KOPIE haben sich bewegt) -- sonst waere "
       "die Gleichheit oben vakuoes",
       cpy_after_mut.allocation_count > cpy_after_copy.allocation_count);
    tr("(5) die QUELLE antwortet nach der Mutation der Kopie unveraendert vollstaendig (0 fehlende Keys)",
       missing_in_src_after == 0);
    tr("(5) und die KOPIE traegt jetzt ihren NEUEN Key-Satz vollstaendig (0 fehlende Keys)",
       missing_in_copy_after == 0);

    // -- (6) PAPERTREUE-BELEG: die beiden strukturellen Zusagen des SuRF-Papers am Objekt -----------
    // Ein Scrub darf die Filter-Semantik NICHT verschieben. no-FN ist die harte Paper-Zusage; der
    // Kreuzbeleg gegen die exakte S1-Ground-Truth ist ihre unabhaengige Gegenprobe.
    HerzStore s2b;
    s2b.build_from_sorted_keys(keys);
    std::size_t   false_negatives = 0;
    std::uint64_t s1_true = 0, s2_true = 0, cross_fn = 0;
    for (std::uint64_t const k : gt)
        if (!fcmp::SurfLoudsQuery::contains_in(s2b, k)) ++false_negatives;
    for (std::uint64_t q = 0; q < 100000ull; ++q) {
        bool const a = fcmp::ExactPrefixFilterQuery::contains_in<S1Store>(s1, q);
        bool const b = fcmp::SurfLoudsQuery::contains_in(s2b, q);
        s1_true += a ? 1u : 0u;
        s2_true += b ? 1u : 0u;
        if (a && !b) ++cross_fn;
    }
    std::printf("-- (6) PAPERTREUE (SuRF-Struktur-Zusagen, nach dem Schnitt) --\n");
    std::printf("     no-FN ueber die gebauten Keys: false_negatives=%zu (Soll 0)\n", false_negatives);
    std::printf("     Kreuzbeleg ueber 100000 Proben: S1_true=%llu S2_true=%llu cross_false_negatives=%llu\n",
                static_cast<unsigned long long>(s1_true), static_cast<unsigned long long>(s2_true),
                static_cast<unsigned long long>(cross_fn));

    tr("(6) NO-FALSE-NEGATIVE: jeder gebaute Key wird gefunden (die harte SuRF-Zusage, Zhang SIGMOD 2018)",
       false_negatives == 0);
    tr("(6) KREUZBELEG: S2.contains(k) >= S1.contains(k) ueber den gesamten Probe-Raum (0 false negatives "
       "gegen die exakte Ground-Truth)",
       cross_fn == 0);
    tr("(6) die exakte S1-Base ist ueberhaupt getrieben (S1_true == key_count) -- ein leerer Anker "
       "belegte nichts",
       s1_true == gt.size());
    tr("(6) S2 bejaht mindestens so oft wie S1 (approximativ, FP erlaubt, FN verboten)", s2_true >= s1_true);
#else
    std::printf("-- (4)/(5)/(6) uebersprungen: COMDARE_CE_ENABLE_STATISTICS ist AUS (die Achsen-Statistik "
                "existiert dann nicht). Die Typ-Ebene (1)-(3) ist compile-hart und gilt unabhaengig davon. --\n");
#endif

    std::printf("== test_s5_02b_filter_alloc_conformance: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
