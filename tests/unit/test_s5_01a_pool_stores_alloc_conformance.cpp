// A8-S5 Familie 01_read_path / Sub-Scheibe 01a (die Pool-/Layout-Stores unter organ_axes/lookup/composable/)
// -- FAMILIEN-KONFORMITAETS-WACHE.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4 + 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity").
// Das wiederverwendbare Praedikat liegt in s5_family_alloc_conformance.hpp (Pilot 04_execution) und bleibt
// hier UNVERAENDERT; DIESE TU besteht aus den Typ-Listen der Familie, dem literalen Lauf-Ausweis -- und, weil
// die Familie ausschliesslich Form B nutzt, aus dem REALEN VERDRAHTUNGS-BELEG (Block (4)) plus dem
// COW-/Memento-Beleg (Block (5)), Vorbild test_s5_03_placement_alloc_conformance.cpp.
//
// WAS GEPINNT WIRD: kein Substrat-Store dieser Familie fuehrt Speicher an der Allokator-Achse vorbei.
//
// WOHER DIE TYP-LISTE KOMMT (Anti-Leerlauf, KEINE Datei-/Namensliste): die Stores sind Template-Argumente der
// REALEN Konsumenten-Kompositionen. Die Liste wird deshalb aus der Achsen-Registry abgeleitet --
//   AllStrategies (axis_03a) -> organ_for_search_algo_t<W> (nicht-void = organ-backed Familie)
//   -> pool_of<Organ> (das zweite Template-Argument des ComposedXxxSearch-Organs) = DER STORE.
// Waechst die 03a-Achse um eine organ-backed Familie, waechst die Wache mit. EINZIGE Handnennung: das
// DEFERRED Masstree-Substrat -- es hat (noch) keinen Wrapper in AllStrategies, ist aber ein Objekt dieser
// Scheibe (MasstreeOrgan existiert und wird in tier_to_organ_mapping.hpp gefuehrt). Es wird als solches
// DEKLARIERT und explizit angehaengt, statt still zu fehlen.
//
// GEPRUEFTE EBENEN:
//   (1) die SUBSTRAT-STORES selbst     -- der Scrub-Gegenstand
//   (2) die ORGAN-Kompositionen        -- ComposedXxxSearch<Traversal, Store> (der reale Konsument);
//                                         AUSWEIS + Struktur-Wache, KEINE Form-B-Behauptung (s. u., Scheibe 01b)
//   (3) die ORGAN-HUELLEN              -- ObservableComposedContainer<Organ> (container_t des ABI-Adapters);
//                                         dito
//   (4) LAUFZEIT-Beleg der Verdrahtung -- Form B behauptet nicht nur, sie ist am Objekt nachgewiesen
//   (5) LAUFZEIT-Beleg von COW/Memento -- inkl. der VERSCHACHTELTEN Container (Skip-Liste / START)
//
// WARUM (4)/(5) HIER STEHEN MUESSEN (Form-B-Grenze, s5_family_alloc_conformance.hpp:31): das Praedikat prueft,
// dass ein DEKLARIERTER allocator_type das Achsen-Concept erfuellt -- nicht, dass die Member-Allokation real
// darueber laeuft. Diese Familie ist die dichteste Form-B-Flaeche des S5-Scrubs, und zwei ihrer Mitglieder
// tragen einen Container IM Element (SkipListNodePoolStore: Forward-Turm je Knoten; StartTrieNodePoolStore:
// disc/kids je Inner-Knoten). Genau dort waere eine gewoehnliche Vektor-Kopie still an den Allokator der
// QUELLE gebunden geblieben -- Block (5) ist der Detektor dafuer und wuerde es SEHEN.
//
// DEKLARIERTE LUECKE (ehrlich, nicht still): ComposedEytzingerSearch und ComposedMasstreeSearch reichen
// store_allocator_statistics() (noch) NICHT durch -- die uebrigen zehn Composed*Search tun es. Fuer diese
// beiden Familien bleibt die T6-Spalte damit honest-0, obwohl ihr Store die Achse jetzt benutzt. Die
// composed_*-Ebene ist Gegenstand der Folge-Scheibe 01b (Include-Kette); diese TU belegt beide deshalb
// DIREKT AM STORE (Block (4b)) statt an der Huelle -- der Beleg ist damit nicht schwaecher, nur an anderer
// Stelle, und die Luecke steht ausgeschrieben statt zu verschwinden.
//
// Standalone (plain int main, KEIN gtest) -- konsistent mit den uebrigen S5-/Phase-E-Wachen.

#include "s5_family_alloc_conformance.hpp"

#include <organ_axes/alloc/axis_06_allocator_exgen.hpp>
#include <organ_axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <organ_axes/lookup/axis_03a_search_algo_registry.hpp>
#include <organ_axes/lookup/composable/observable_composed_container.hpp>
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

// -- Ableitungs-Werkzeug: der Store IST das zweite Template-Argument des Organs ---------------------
// Variadisch, weil ComposedSwissSearch<Traversal, Pool, Isa> ein drittes (ISA-)Argument traegt.
template <class Organ>
struct pool_of;
template <template <class...> class Org, class Traversal, class Pool, class... Rest>
struct pool_of<Org<Traversal, Pool, Rest...>> {
    using type = Pool;
};
template <class Organ>
using pool_of_t = typename pool_of<Organ>::type;

// -- (1)/(2) Die Familie AUS DER REGISTRY ----------------------------------------------------------
template <class W>
using organ_of = lkc::organ_for_search_algo_t<W>;
template <class W>
using is_organ_backed_wrapper = mp::mp_bool<!std::is_same_v<organ_of<W>, void>>;

/// Genau die 03a-Strategien, deren search_algo ein NATIVES Organ traegt (der Rest ist flach/store-traversierbar).
using RegistryWrappers = mp::mp_filter<is_organ_backed_wrapper, lk::AllStrategies>;
using RegistryOrgans   = mp::mp_transform<organ_of, RegistryWrappers>;
using RegistryStores   = mp::mp_transform<pool_of_t, RegistryOrgans>;

/// DEFERRED: kein Wrapper in AllStrategies, aber existierendes Organ + geSCRUBbter Store dieser Scheibe.
using DeferredOrgans = mp::mp_list<lkc::MasstreeOrgan>;
using DeferredStores = mp::mp_transform<pool_of_t, DeferredOrgans>;

using Family01aOrgans = mp::mp_append<RegistryOrgans, DeferredOrgans>;
using Family01aStores = mp::mp_append<RegistryStores, DeferredStores>;

// -- (3) Organ-HUELLEN, wie der ABI-Adapter sie als container_t haelt -------------------------------
template <class Organ>
using HullOf         = lkc::ObservableComposedContainer<Organ>;
using Family01aHulls = mp::mp_transform<HullOf, Family01aOrgans>;

// -- Anti-Leerlauf: eine leere Liste macht JEDE Alles-Aussage wahr ----------------------------------
static_assert(mp::mp_size<lk::AllStrategies>::value > 0,
              "S5-01a: die 03a-Registry ist LEER -- dann waere die ganze Ableitungskette wirkungslos.");
static_assert(mp::mp_size<RegistryStores>::value > 0,
              "S5-01a: aus der Registry faellt KEIN organ-backed Store -- die Wache pruefte nichts "
              "(organ_for_search_algo nicht eingebunden?).");
static_assert(mp::mp_size<Family01aStores>::value == mp::mp_size<Family01aOrgans>::value,
              "S5-01a: zu jedem Familien-Organ gehoert genau ein Substrat-Store (die Liste ist ABGELEITET, "
              "nicht handgepflegt).");
static_assert(mp::mp_size<mp::mp_unique<Family01aStores>>::value == mp::mp_size<Family01aStores>::value,
              "S5-01a: die Store-Liste enthaelt Duplikate -- dann deckte ein Eintrag mehrere Familien zu und "
              "die Zahl der geprueften Substrate waere kleiner als sie aussieht.");
static_assert(mp::mp_size<RegistryStores>::value >= 11,
              "S5-01a: die Familie ist geschrumpft -- 10 organ-backed Pool-Familien PLUS die Eytzinger-Layout-"
              "Familie waren der Bestand des Schnitts (Planungs-Ist 04.08.); ein Rueckbau muss auffallen, "
              "nicht durchrutschen.");

// -- Die Wache selbst (Typ-Ebene) ------------------------------------------------------------------
static_assert(s5::family_alloc_conform_v<Family01aStores>,
              "S5-01a: ein Substrat-Store der Familie fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
/// EBENE (2)/(3) -- WAS HIER BEWUSST NICHT BEHAUPTET WIRD (deklariert, nicht verschwiegen):
/// Die Organ-Kompositionen (ComposedXxxSearch) und ihre Huellen tragen HEUTE keinen eigenen allocator_type;
/// sie delegieren ihren Speicher an das Substrat, halten aber teils noch eigene Hilfs-Container am
/// Default-Allokator. Das ist der Gegenstand der FOLGE-Scheibe 01b (composable-Rest), nicht dieser. Ein
/// `using allocator_type = typename Pool::allocator_type;` waere hier billig zu haben -- und genau die
/// Sorte Form-B-Deklaration, vor der die Form-B-Grenze warnt (s5_family_alloc_conformance.hpp:31): eine
/// huebsche using-Zeile neben einem Default-Allokator-Container. Sie wird deshalb NICHT gesetzt.
/// Was auf diesen Ebenen SEHR WOHL behauptet und geprueft wird, weil es hier und heute gilt:
///   * die Huelle fuehrt KEIN eigenes Substrat ein -- ihr container_type IST das Organ,
///   * und der Speicher-Besitz des Organs ist genau der Store aus Ebene (1).
template <class Organ>
using hull_wraps_exactly = mp::mp_bool<std::is_same_v<typename HullOf<Organ>::container_type, Organ>>;
static_assert(mp::mp_all_of<Family01aOrgans, hull_wraps_exactly>::value,
              "S5-01a: eine Organ-Huelle wrappt nicht mehr genau ihr Organ -- dann saesse zwischen "
              "ABI-Adapter und geSCRUBbtem Substrat eine dritte, ungepruefte Speicher-Ebene.");

/// SCHAERFER als (A)||(B): fuer DIESE Familie ist Form A gar nicht zulaessig -- jedes Substrat besitzt einen
/// unbounded Container, also MUSS jedes den Achsen-Weg gehen. Ein "heap-frei"-Ausweis waere hier ein Indiz
/// dafuer, dass ein Store seinen Speicher verloren hat, nicht dass er ihn sauber fuehrt.
template <class T>
using is_axis_bound = mp::mp_bool<s5::AxisAllocatorBoundOrgan<T>>;
static_assert(mp::mp_all_of<Family01aStores, is_axis_bound>::value,
              "S5-01a: ein Substrat-Store hat seinen Form-B-Ausweis (allocator_type) verloren -- fuer diese "
              "Familie ist Form A KEIN gueltiger Ausweg (jedes Substrat besitzt einen unbounded Container).");

/// Und die Bindung ist die ACHSEN-Default-Strategie, nicht irgendein Typ, der zufaellig das Concept erfuellt.
template <class T>
using is_axis_default_bound = mp::mp_bool<std::is_same_v<typename T::allocator_type, alx::ExgenAllocator>>;
static_assert(mp::mp_all_of<Family01aStores, is_axis_default_bound>::value,
              "S5-01a: ein Substrat-Store haengt nicht mehr am Achsen-DEFAULT (ExgenAllocator). Das ist der "
              "deklarierte ZWISCHENSTAND dieser Scheibe (LEDGER 04.08. abend-11, T6 = Option B strikt); die "
              "spaetere metaprogrammierte Kompositions-Durchbindung ersetzt ihn BEWUSST -- dann wandert diese "
              "Zeile mit, sie faellt nicht weg.");

// Referenz-Anker: das ZIEL-Muster der Scheibe traegt Form B weiterhin (btree, nur GELESEN).
static_assert(s5::AxisAllocatorBoundOrgan<lkc::BTreeNodePoolStore<>>,
              "S5-Gate-Muster: der Form-(B)-Zweig findet am Referenz-Muster keinen allocator_type mehr -- dann "
              "pruefte die Wache nur noch Heap-Freiheit und liesse den Adapter-Weg ungedeckt.");

// -- Literaler Lauf-Ausweis (kein Erfolgs-Haken ohne Ausgabe) --------------------------------------
int g_fail = 0;

void tr(char const* what, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what);
    if (!ok) ++g_fail;
}

/// `wache` = true: der Ausweis ist zugleich eine Wache (Ebene 1, der Scrub-Gegenstand -- nicht-konform
/// laesst die TU rot werden). `wache` = false: reiner AUSWEIS (Ebenen 2/3, Gegenstand der Scheibe 01b) --
/// er wird literal gedruckt und als "offen" markiert, aber er faelscht das Urteil dieser Scheibe nicht.
template <class T>
void report_one(char const* stufe, std::string_view label, bool wache) {
    bool const  conform = s5::FamilyAllocConform<T>;
    char const* mark    = conform ? " ok " : (wache ? "FAIL" : "01b ");
    std::printf("  [%s] %-6s %-24.*s Form: %s\n", mark, stufe, static_cast<int>(label.size()), label.data(),
                s5::family_alloc_form<T>());
    if (!conform && wache) ++g_fail;
}

template <class W>
using StoreOfWrapper = pool_of_t<organ_of<W>>;
template <class W>
using OrganOfWrapper = organ_of<W>;
template <class W>
using HullOfWrapper = HullOf<organ_of<W>>;

/// Beschriftet wird ueber den REGISTRY-WRAPPER (er traegt name()); die Substrate selbst sind Struktur und
/// tragen keinen Achsen-Namen -- dieselbe Loesung wie im 03er-Vorbild fuer die Slot-Backings.
template <template <class> class Pick>
void report_registry_level(char const* stufe, bool wache) {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, RegistryWrappers>>([&](auto id) {
        using W = typename decltype(id)::type;
        report_one<Pick<W>>(stufe, W::name(), wache);
    });
}

[[nodiscard]] std::uint64_t spread(std::uint64_t i) noexcept {
    return (i * 2654435761ull) ^ (i << 13) ^ 0x9E3779B97F4A7C15ull;
}

/// Inhalts-Gleichheit ohne operator== (die Organe fuehren keinen): jeder Quell-Key ist in der Kopie mit
/// demselben Wert auffindbar.
template <class Organ>
[[nodiscard]] bool copy_has_all_keys(Organ const& src, Organ const& cp, std::uint64_t n) {
    (void)src;
    for (std::uint64_t k = 0; k < n; ++k) {
        auto const found = cp.lookup(spread(k));
        if (!found.has_value() || *found != k * 7u + 1u) return false;
    }
    return true;
}

/// (4) VERDRAHTUNGS-BELEG: unter Last steigt der Achsen-Zaehler -- die Member-Allokation laeuft also WIRKLICH
/// ueber die Versorger-Achse und nicht ueber einen Default-Allokator neben einer huebschen using-Zeile.
/// Getrieben wird das ORGAN (der reale Konsument); gelesen wird die Achsen-Statistik seines Stores.
template <class Organ>
void probe_organ_wiring(char const* label, std::uint64_t n_ops) {
    Organ      organ{};
    auto const before = organ.store_allocator_statistics();
    for (std::uint64_t k = 0; k < n_ops; ++k) (void)organ.insert(spread(k), k * 7u + 1u);
    auto const after = organ.store_allocator_statistics();

    std::printf("     %-18s alloc_cnt %llu -> %llu   bytes_in_use %llu   occupied %llu\n", label,
                static_cast<unsigned long long>(before.allocation_count),
                static_cast<unsigned long long>(after.allocation_count),
                static_cast<unsigned long long>(after.total_bytes_in_use),
                static_cast<unsigned long long>(organ.occupied_count()));

    tr("(4) unter Last steigt der Achsen-Allokationszaehler (alloc_cnt waechst)",
       after.allocation_count > before.allocation_count);
    tr("(4) die Achse haelt den Speicher auch (bytes_in_use > 0)", after.total_bytes_in_use > 0);
    tr("(4) und das Organ traegt die Daten wirklich (occupied_count > 0)", organ.occupied_count() > 0);
    tr("(4) Fehlschlaege werden ehrlich gezaehlt und sind hier 0 (kein verdeckter OOM-Pfad)", after.failure_count == 0);
}

/// (5) COW-/MEMENTO-BELEG -- der Detektor fuer stille Cross-Strategy-Allokation.
/// Beweisfuehrung rein aus den Achsen-Zaehlern, ohne Zeiger-Introspektion:
///   (a) die Kopie darf die Zaehler der QUELLE nicht bewegen -> sie hat NICHT aus der Quelle alloziert,
///       und der Memento-Restore setzt sie exakt auf den Quell-Stand (Kopier-Pollution verworfen),
///   (b) eine Mutation der KOPIE bewegt die Zaehler der Kopie, aber weiterhin NICHT die der Quelle
///       -> auch die Container IM Element (Forward-Turm / disc+kids) haengen am Allokator der Kopie,
///   (c) der Inhalt ist vollstaendig uebernommen (alle Quell-Keys in der Kopie auffindbar).
template <class Organ>
void probe_memento_rebind(char const* label, std::uint64_t n_ops) {
    Organ src{};
    for (std::uint64_t k = 0; k < n_ops; ++k) (void)src.insert(spread(k), k * 7u + 1u);
    auto const src_before = src.store_allocator_statistics();

    Organ      cp{src}; // COW-Kopie (genau das tut der Memento-Pfad des Adapters)
    auto const src_after_copy = src.store_allocator_statistics();
    auto const cp_after_copy  = cp.store_allocator_statistics();

    for (std::uint64_t k = n_ops; k < 2u * n_ops; ++k) (void)cp.insert(spread(k), k * 7u + 1u); // NUR die Kopie
    auto const src_after_mut = src.store_allocator_statistics();
    auto const cp_after_mut  = cp.store_allocator_statistics();

    std::printf("     %-18s src alloc_cnt %llu -> %llu (Kopie) -> %llu (Mutation der Kopie)\n", label,
                static_cast<unsigned long long>(src_before.allocation_count),
                static_cast<unsigned long long>(src_after_copy.allocation_count),
                static_cast<unsigned long long>(src_after_mut.allocation_count));
    std::printf("     %-18s cp  alloc_cnt %llu (Memento-restauriert) -> %llu (Mutation)   occ src/cp %llu/%llu\n",
                label, static_cast<unsigned long long>(cp_after_copy.allocation_count),
                static_cast<unsigned long long>(cp_after_mut.allocation_count),
                static_cast<unsigned long long>(src.occupied_count()),
                static_cast<unsigned long long>(cp.occupied_count()));

    tr("(5a) die Kopie hat NICHT aus dem Allokator der Quelle alloziert (Quell-Zaehler unveraendert)",
       src_after_copy.allocation_count == src_before.allocation_count);
    tr("(5a) Memento: die Kopie traegt exakt den Quell-Stand (Kopier-Pollution verworfen)",
       cp_after_copy.allocation_count == src_before.allocation_count);
    tr("(5b) Mutation der Kopie bewegt den Allokator der KOPIE (Kopie-Zaehler steigt)",
       cp_after_mut.allocation_count > cp_after_copy.allocation_count);
    tr("(5b) VERSCHACHTELUNGS-DETEKTOR: die Mutation der Kopie laesst die QUELLE unberuehrt -- auch die "
       "Container IM Element haengen am Allokator der Kopie",
       src_after_mut.allocation_count == src_before.allocation_count);
    tr("(5b) und die Quelle behaelt ihren Bestand (keine geteilte Struktur)", src.occupied_count() == n_ops);
    tr("(5c) Memento-Vertrag: die Kopie traegt den vollen Quell-Inhalt", copy_has_all_keys(src, cp, n_ops));

    Organ assigned{};
    assigned = src; // der Rueckspiel-Pfad
    tr("(5c) Rueckspiel per copy-assign ist inhaltsgleich", copy_has_all_keys(src, assigned, n_ops));
    tr("(5c) und der Rueckspiel-Empfaenger hat aus SEINEM Allokator alloziert",
       assigned.store_allocator_statistics().allocation_count > 0);
}

/// (4b) Die zwei Familien, deren ORGAN store_allocator_statistics() noch nicht durchreicht (Luecke 01b):
/// der Verdrahtungs-Beleg wird DIREKT AM STORE gefuehrt, mit derselben Beweisform.
void probe_store_direct_eytzinger() {
    lkc::EytzingerLayoutStore<> store;
    auto const                  before = store.store_allocator_statistics();
    for (std::uint64_t k = 0; k < 512; ++k) store.append_slot(k, k * 7u + 1u); // aufsteigend = sortierte Basis
    store.rebuild_if_dirty(); // der lazy BFS-Puffer, die Mess-Eigenschaft des Tiers
    auto const after = store.store_allocator_statistics();
    std::printf("     %-18s alloc_cnt %llu -> %llu   bytes_in_use %llu   slots %zu\n", "eytzinger[store]",
                static_cast<unsigned long long>(before.allocation_count),
                static_cast<unsigned long long>(after.allocation_count),
                static_cast<unsigned long long>(after.total_bytes_in_use), store.slot_count());
    tr("(4b) frischer Eytzinger-Store hat noch KEINE Achsen-Allokation (ehrlicher Nullpunkt)",
       before.allocation_count == 0);
    tr("(4b) Basis UND lazy BFS-Rebuild laufen REAL ueber die Achse (alloc_cnt > 0)", after.allocation_count > 0);
    tr("(4b) die Achse haelt den Speicher auch (bytes_in_use > 0)", after.total_bytes_in_use > 0);
    tr("(4b) und die Basis traegt die Daten wirklich (slot_count == 512)", store.slot_count() == 512u);
}

void probe_store_direct_masstree() {
    lkc::MasstreeLayerNodePoolStore<> store;
    auto const                        before = store.store_allocator_statistics();
    for (int i = 0; i < 64; ++i) {
        (void)store.new_leaf();
        (void)store.new_internode();
    }
    auto const after = store.store_allocator_statistics();
    std::printf("     %-18s alloc_cnt %llu -> %llu   bytes_in_use %llu\n", "masstree[store]",
                static_cast<unsigned long long>(before.allocation_count),
                static_cast<unsigned long long>(after.allocation_count),
                static_cast<unsigned long long>(after.total_bytes_in_use));
    tr("(4b) frischer Masstree-Store hat noch KEINE Achsen-Allokation (ehrlicher Nullpunkt)",
       before.allocation_count == 0);
    tr("(4b) Leaf-/Internode-Pool laeuft REAL ueber die Achse (alloc_cnt > 0)", after.allocation_count > 0);
    tr("(4b) die Achse haelt den Speicher auch (bytes_in_use > 0)", after.total_bytes_in_use > 0);
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 01a -- Pool-/Layout-Stores: Allokator-Achsen-Konformitaet ==\n");
    std::printf("   Achsen: search_algo (T0, organ-backed Substrate) . allocator (T6, Versorger)\n");
    std::printf("   Typ-Liste ABGELEITET: AllStrategies -> organ_for_search_algo_t -> pool_of (+ DEFERRED Masstree)\n");

    std::printf("-- (1) Substrat-Stores, beschriftet mit ihrer 03a-Strategie (der Scrub-Gegenstand) --\n");
    report_registry_level<StoreOfWrapper>("STORE", true);
    report_one<pool_of_t<lkc::MasstreeOrgan>>("STORE", "masstree (DEFERRED)", true);
    std::printf("-- (2) Organ-Kompositionen (der reale Konsument; eigene Hilfs-Container = Scheibe 01b) --\n");
    report_registry_level<OrganOfWrapper>("ORGAN", false);
    report_one<lkc::MasstreeOrgan>("ORGAN", "masstree (DEFERRED)", false);
    std::printf("-- (3) Organ-Huellen (container_t des ABI-Adapters; ebenfalls 01b) --\n");
    report_registry_level<HullOfWrapper>("HULL", false);
    report_one<HullOf<lkc::MasstreeOrgan>>("HULL", "masstree (DEFERRED)", false);

    std::printf("-- (4) LAUFZEIT: reale Verdrahtung der geSCRUBbten Stores (getrieben ueber das Organ) --\n");
    probe_organ_wiring<lkc::ArtTrieOrgan>("art_trie", 512);
    probe_organ_wiring<lkc::HotPatriciaOrgan>("hot_patricia", 512);
    probe_organ_wiring<lkc::StartTrieOrgan>("start_trie", 512);
    probe_organ_wiring<lkc::WormholeOrgan>("wormhole", 512);
    probe_organ_wiring<lkc::SkipListOrgan>("skip_list", 512);
    probe_organ_wiring<lkc::HashSearchOrgan>("hash_bucket", 512);
    probe_organ_wiring<lkc::SwissTableOrgan>("swiss_group", 512);

    std::printf("-- (4b) DIREKT AM STORE: die zwei Familien, deren Organ store_allocator_statistics() noch\n");
    std::printf("        NICHT durchreicht -- deklarierte Luecke, gehoert der Folge-Scheibe 01b --\n");
    probe_store_direct_eytzinger();
    probe_store_direct_masstree();

    std::printf("-- (5) LAUFZEIT: COW/Memento rebindet auf den EIGENEN Allokator --\n");
    // Skip-Liste + START ZUERST: sie sind die beiden Familien mit Containern IM Element -- der eigentliche
    // Gegenstand des Verschachtelungs-Detektors.
    probe_memento_rebind<lkc::SkipListOrgan>("skip_list[nested]", 256);
    probe_memento_rebind<lkc::StartTrieOrgan>("start_trie[nested]", 256);
    probe_memento_rebind<lkc::ArtTrieOrgan>("art_trie", 256);
    probe_memento_rebind<lkc::HotPatriciaOrgan>("hot_patricia", 256);
    probe_memento_rebind<lkc::WormholeOrgan>("wormhole", 256);
    probe_memento_rebind<lkc::HashSearchOrgan>("hash_bucket", 256);
    probe_memento_rebind<lkc::SwissTableOrgan>("swiss_group", 256);

    std::printf("-- Familien-Bilanz --\n");
    std::printf("  Organe: %zu   Substrat-Stores: %zu   davon DEFERRED (ohne Registry-Wrapper): %zu\n",
                static_cast<std::size_t>(mp::mp_size<Family01aOrgans>::value),
                static_cast<std::size_t>(mp::mp_size<Family01aStores>::value),
                static_cast<std::size_t>(mp::mp_size<DeferredStores>::value));
    std::printf("  Bereits vor dieser Scheibe konform (nur mitgepinnt): tree/btree/surf_fst.\n");

    std::printf("== test_s5_01a_pool_stores_alloc_conformance: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
