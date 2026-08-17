// A8-S5 Familie 02_layout, Sub-Scheibe 02a (node_type + memory_layout + path_compression + serialization)
// -- FAMILIEN-KONFORMITAETS-WACHE.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4 + 5-S5
// ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity").
// Das wiederverwendbare Praedikat liegt in s5_family_alloc_conformance.hpp (Pilot 04_execution); DIESE TU
// besteht aus den Typ-Listen der Familie, dem literalen Lauf-Ausweis -- und, weil die Familie Form B nutzt,
// zusaetzlich aus dem REALEN VERDRAHTUNGS-BELEG (unten Block (4)) und dem MEMENTO-BELEG (Block (5)).
//
// GELTUNGSBEREICH: 02a ist die Sub-Scheibe node/path_compression/serialization/layout der Organ-Gruppe
// 02_layout (lager_baum_writer.hpp kOrganGruppe02 = node_type, memory_layout, path_compression, filter,
// serialization). Die filter-/SuRF-Seite ist Sub-Scheibe 02b (faithful-Portierung, eigene Scheibe) und wird
// hier bewusst NICHT gepinnt -- eine Wache, die eine noch ungeschnittene Achse gruen meldet, waere eine
// Vorab-Zementierung. NACHTRAG A8-S5-02b (2026-08-04): 02b IST inzwischen geschnitten und hat eine EIGENE
// Wache (test_s5_02b_filter_alloc_conformance). Die Trennung BLEIBT trotzdem bestehen -- nicht wegen der
// Reihenfolge, sondern weil 02b eine andere Registry, andere Schnitt-Formen (die Kompositions-filter-Achse
// ist Form A, die composable-Organe Form B) und eine eigene PAPERTREUE-Aussage traegt; zwei Wachen ueber
// demselben Gegenstand driften auseinander. Wer die filter-Achse sucht, findet sie dort, nicht hier.
// memory_layout gehoert zu 02a und hatte 0 Code-Treffer im Familien-grep: NACHWEIS-Achse,
// hier mitgepinnt, damit ein kuenftiger Container dort sofort auffaellt.
//
// WAS GEPINNT WIRD: kein Organ dieser Sub-Familie fuehrt Speicher an der Allokator-Achse vorbei. Die Pruefung
// laeuft auf TYP-EBENE gegen die REALEN Kompositions-Typen -- die Listen kommen aus den Achsen-Registries
// (AllNodeTypes / AllLayouts / AllCompressions / AllSerializers), NICHT aus einer handgepflegten Aufzaehlung.
// Waechst eine Achse um eine Strategie, waechst die Wache mit.
//
// GEPRUEFTE EBENEN:
//   (1) die STRATEGIE-Typen selbst        -- node/layout/path_compression/serialization aus den Registries
//   (2) die ORGAN-HUELLEN der Komposition -- ObservableNodeType/ObservableMemoryLayout/
//                                            ObservablePathCompression/ObservableSerialization
//   (3) die inneren BACKINGS              -- real_trie_t<S> je path_compression-Strategie + die drei
//                                            node-Chunk-Stores ueber Registry-abgeleitete Achsen-Kreuzungen
//                                            + SignalingStream (der Scrub-Gegenstand dieser Scheibe)
//   (4) LAUFZEIT-Beleg der Verdrahtung    -- Form B behauptet nicht nur, sie ist am Objekt nachgewiesen;
//                                            INKLUSIVE des HERZ-Befundes (Chunk-INDEX ueber Kompositions-A)
//   (5) LAUFZEIT-Beleg des R1-Mementos    -- die Trie-Kopie rebindet, sie erbt keinen fremden Allokator
//
// WARUM (4)/(5) HIER STEHEN MUESSEN (Form-B-Grenze, s5_family_alloc_conformance.hpp:31): das Praedikat prueft,
// dass ein DEKLARIERTER allocator_type das Achsen-Concept erfuellt -- nicht, dass die Member-Allokation real
// darueber laeuft. Fuer diese Scheibe ist das besonders scharf: die node-Stores DEKLARIERTEN ihren
// allocator_type schon VOR dem Scrub (A war Template-Parameter und Provenienz-Name), sie haetten Form B also
// bereits bestanden -- und trotzdem lief der Chunk-INDEX am Default-Allokator. Genau diese Luecke schliesst
// Block (4)-HERZ, und zwar mit einer Aussage, die am ALT-Stand FAELLT (s. dort).
//
// Standalone (plain int main, KEIN gtest) -- konsistent mit den uebrigen Phase-E-Standalone-Wachen.

#include "s5_family_alloc_conformance.hpp"

#include <organ_axes/layout/axis_05_memory_layout_observable.hpp>
#include <organ_axes/layout/axis_05_memory_layout_registry.hpp>
#include <organ_axes/node/axis_04_node_type_chunked_store.hpp>
#include <organ_axes/node/axis_04_node_type_composed_store.hpp>
#include <organ_axes/node/axis_04_node_type_layout_aware_store.hpp>
#include <organ_axes/node/axis_04_node_type_observable.hpp>
#include <organ_axes/node/axis_04_node_type_registry.hpp>
#include <organ_axes/path_compression/axis_02_path_compression_observable.hpp>
#include <organ_axes/path_compression/axis_02_path_compression_real_trie.hpp>
#include <organ_axes/path_compression/axis_02_path_compression_registry.hpp>
#include <organ_axes/serialization_axis/axis_10_serialization_observable.hpp>
#include <organ_axes/serialization_axis/axis_10_serialization_primitives.hpp>
#include <organ_axes/serialization_axis/axis_10_serialization_registry.hpp>

#include <organ_axes/alloc/axis_06_allocator_exgen.hpp>
#include <organ_axes/alloc/axis_06_allocator_mimalloc.hpp>
#include <organ_axes/alloc/axis_06_allocator_registry.hpp>

// Form-(B)-REFERENZ am realen Repo-Typ (Pilot-Vorbild): der B-Baum-Knoten-Pool fuehrt seinen unbounded
// Knoten-Speicher ueber die Allokator-Achse (StdAllocatorAdapter-Rebind + COW-Memento). Er gehoert NICHT zu
// dieser Familie und wird hier nur GELESEN -- er ist der Anker, an dem der Scrub dieser Familie Mass nimmt.
#include <organ_axes/lookup/composable/btree_node_pool_store.hpp>

#include <boost/mp11.hpp>

#include <cstdio>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>

namespace mp   = boost::mp11;
namespace s5   = ::comdare::cache_engine::tests::s5;
namespace ndx  = ::comdare::cache_engine::node;
namespace mlx  = ::comdare::cache_engine::layout;
namespace pcx  = ::comdare::cache_engine::path_compression;
namespace serx = ::comdare::cache_engine::serialization_axis;
namespace alx  = ::comdare::cache_engine::alloc;
namespace lkc  = ::comdare::cache_engine::lookup::composable;

namespace {

// -- (1) Strategie-Typen der Sub-Familie, direkt aus den Achsen-Registries -------------------------
using Family02aStrategies =
    mp::mp_append<ndx::AllNodeTypes, mlx::AllLayouts, pcx::AllCompressions, serx::AllSerializers>;

// -- (2) Organ-Huellen, wie die Komposition sie als Member haelt ------------------------------------
template <class S>
using NodeOrganOf = ndx::ObservableNodeType<S>;
template <class S>
using LayoutOrganOf = mlx::ObservableMemoryLayout<S>;
template <class S>
using PcOrganOf = pcx::ObservablePathCompression<S>;
template <class S>
using SerOrganOf = serx::ObservableSerialization<S>;
using Family02aOrgans =
    mp::mp_append<mp::mp_transform<NodeOrganOf, ndx::AllNodeTypes>, mp::mp_transform<LayoutOrganOf, mlx::AllLayouts>,
                  mp::mp_transform<PcOrganOf, pcx::AllCompressions>,
                  mp::mp_transform<SerOrganOf, serx::AllSerializers>>;

// -- (3) Innere Backings (der Scrub-Gegenstand dieser Scheibe) --------------------------------------
// (3a) das Trie-Backing je path_compression-Strategie (compile-zeit-selektiert, NICHT handgepflegt).
template <class S>
using RealTrieOf           = pcx::real_trie_t<S>;
using Family02aTrieBacking = mp::mp_transform<RealTrieOf, pcx::AllCompressions>;

// (3b) die node-Chunk-Stores. Der Store ist ein 3-Achsen-Verbund (N,L,A); die Listen werden deshalb aus
//      den Registries GEKREUZT, nicht aufgezaehlt: einmal alle Node-Typen am Pilot-Layout, einmal alle
//      Layouts am Pilot-Node. Der Allokator bleibt der Pilot-Allokator (die Allokator-Achse ist hier der
//      VERSORGER, nicht der Pruefgegenstand -- ihre eigene Konformitaet prueft Block (0)).
using PilotAlloc  = alx::MimallocAllocator;
using PilotLayout = mlx::CacheLineAlignedMemoryLayout;
using PilotNode   = ndx::Node4NodeType;

template <class N>
using LayoutAwareOverNode = ndx::LayoutAwareChunkedStore<N, PilotLayout, PilotAlloc>;
template <class L>
using LayoutAwareOverLayout = ndx::LayoutAwareChunkedStore<PilotNode, L, PilotAlloc>;
template <class N>
using ChunkedOverNode = ndx::NodeChunkedStore<N, PilotLayout, PilotAlloc>;
template <class N>
using ComposedOverNode = ndx::ComposedStore<N, PilotLayout, PilotAlloc>;

using Family02aStoreBacking = mp::mp_append<
    mp::mp_transform<LayoutAwareOverNode, ndx::AllNodeTypes>, mp::mp_transform<LayoutAwareOverLayout, mlx::AllLayouts>,
    mp::mp_transform<ChunkedOverNode, ndx::AllNodeTypes>, mp::mp_transform<ComposedOverNode, ndx::AllNodeTypes>>;

// (3c) das serialization-Backing (EIN besitzender Container der Achse, kein Strategie-Fanout).
using Family02aStreamBacking = mp::mp_list<serx::SignalingStream>;

// -- (0) Die Versorger-Achse: anderes Kriterium (sie IST der Versorger, nicht ein Verbraucher) ------
template <class A>
using is_valid_supplier = mp::mp_bool<alx::concepts::AllocatorStrategy<A>>;

// -- Anti-Leerlauf: eine leere Liste macht JEDE Alles-Aussage wahr ---------------------------------
static_assert(mp::mp_size<Family02aStrategies>::value > 0,
              "S5-02a: die Familien-Strategie-Liste ist LEER -- eine leere Liste macht jede Alles-Aussage wahr "
              "und die Wache wertlos (Registry nicht eingebunden?).");
static_assert(mp::mp_size<alx::AllVendors>::value > 0,
              "S5-02a: die Allokator-Registry ist LEER -- dann pruefte Block (0) nichts.");
static_assert(mp::mp_size<Family02aStrategies>::value ==
                  mp::mp_size<ndx::AllNodeTypes>::value + mp::mp_size<mlx::AllLayouts>::value +
                      mp::mp_size<pcx::AllCompressions>::value + mp::mp_size<serx::AllSerializers>::value,
              "S5-02a: die Familien-Liste ist nicht mehr die Summe der vier Achsen-Registries.");
static_assert(mp::mp_size<Family02aOrgans>::value == mp::mp_size<Family02aStrategies>::value,
              "S5-02a: zu jeder Familien-Strategie gehoert genau eine Organ-Huelle.");
static_assert(mp::mp_size<Family02aTrieBacking>::value == mp::mp_size<pcx::AllCompressions>::value,
              "S5-02a: zu jeder path_compression-Strategie gehoert genau ein Trie-Backing -- die Backing-Liste "
              "ist aus der Strategie-Liste abgeleitet, nicht handgepflegt.");
static_assert(mp::mp_size<Family02aStoreBacking>::value ==
                  3u * mp::mp_size<ndx::AllNodeTypes>::value + mp::mp_size<mlx::AllLayouts>::value,
              "S5-02a: die Store-Kreuzungen sind nicht mehr aus den Registries abgeleitet (drei Store-Familien "
              "ueber alle Node-Typen plus der LayoutAware-Store ueber alle Layouts).");

// -- Die Wache selbst (Typ-Ebene) -----------------------------------------------------------------
static_assert(s5::family_alloc_conform_v<Family02aStrategies>,
              "S5-02a: eine Strategie der Sub-Familie 02a fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(s5::family_alloc_conform_v<Family02aOrgans>,
              "S5-02a: eine Organ-Huelle der Sub-Familie 02a fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(s5::family_alloc_conform_v<Family02aTrieBacking>,
              "S5-02a: ein path_compression-Trie-Backing fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(s5::family_alloc_conform_v<Family02aStoreBacking>,
              "S5-02a: ein node-Chunk-Store fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(s5::family_alloc_conform_v<Family02aStreamBacking>,
              "S5-02a: das serialization-Stream-Backing fuehrt Speicher weder heap-frei noch ueber das "
              "Allokator-Achsen-Interface (F2-Schnitt-Regel, Dossier 3.4).");
static_assert(mp::mp_all_of<alx::AllVendors, is_valid_supplier>::value,
              "S5-02a: eine Strategie der Allokator-Achse erfuellt AllocatorStrategy nicht mehr -- dann waere sie "
              "kein gueltiger Versorger, und der Form-B-Zweig der ganzen Scheibe stuende auf Sand.");

// Die beiden Versorger, die der Scrub dieser Scheibe BENANNT hat (EINE Quelle je Achse).
static_assert(alx::concepts::AllocatorStrategy<pcx::path_compression_trie_allocator_t>,
              "S5-02a: der in axis_02_path_compression_real_trie.hpp benannte Versorger erfuellt das "
              "Achsen-Concept nicht.");
static_assert(alx::concepts::AllocatorStrategy<serx::serialization_stream_allocator_t>,
              "S5-02a: der in axis_10_serialization_primitives.hpp benannte Versorger erfuellt das Concept nicht.");

// Form (B) traegt an echtem Bestand (Pilot-Vorbild, Referenz-Anker ausserhalb der Familie).
static_assert(s5::AxisAllocatorBoundOrgan<lkc::BTreeNodePoolStore<>>,
              "S5-Gate-Muster: der Form-(B)-Zweig findet am realen Referenz-Muster keinen allocator_type mehr -- "
              "dann pruefte die Wache nur noch Heap-Freiheit und liesse den Adapter-Weg ungedeckt.");
static_assert(s5::AxisAllocatorBoundOrgan<pcx::PatriciaTrie> && s5::AxisAllocatorBoundOrgan<serx::SignalingStream>,
              "S5-02a: PatriciaTrie/SignalingStream haben ihren Form-B-Ausweis (allocator_type) verloren.");
// EmptyPatriciaTrie (none, M3-Pin) bleibt die STAERKERE Form A -- der Scrub hat den Nullpunkt nicht beschwert.
static_assert(s5::HeapFreeOrgan<pcx::EmptyPatriciaTrie>,
              "S5-02a: das none-Trie-Backing ist nicht mehr heap-frei -- der M3-Pin waere nicht mehr messneutral.");

// -- HERZ-BELEG auf TYP-EBENE ---------------------------------------------------------------------
// Der Chunk-INDEX des LayoutAwareChunkedStore haengt am Adapter DERSELBEN Kompositions-Achse A wie die
// Record-Bytes. Der Test dafuer ist die KONVERTIERBARKEIT in einen anderen Adapter DESSELBEN A: die
// StdAllocatorAdapter<U> einer Strategie tragen untereinander einen (nicht-expliziten) Umwandlungs-Ctor --
// ein std::allocator<X> und der Adapter einer FREMDEN Strategie tragen ihn NICHT. Die Aussage ist damit
// weder eine Selbstauskunft noch eine Textsuche, sondern eine Typ-Relation.
using HerzStore = ndx::LayoutAwareChunkedStore<PilotNode, PilotLayout, PilotAlloc>;
static_assert(std::is_convertible_v<typename HerzStore::chunk_index_allocator_type,
                                    typename PilotAlloc::template StdAllocatorAdapter<std::uint64_t>>,
              "S5-02a HERZ: der Chunk-INDEX haengt nicht mehr am Adapter der Kompositions-Achse A.");
static_assert(!std::is_convertible_v<std::allocator<std::uint64_t>, typename HerzStore::chunk_index_allocator_type>,
              "S5-02a HERZ (Gegenprobe): ein Default-Allokator ist in den Achsen-Adapter konvertierbar -- dann "
              "pinnt die Konvertierbarkeits-Relation nichts.");
static_assert(!std::is_convertible_v<typename alx::ExgenAllocator::template StdAllocatorAdapter<std::uint64_t>,
                                     typename HerzStore::chunk_index_allocator_type>,
              "S5-02a HERZ (Gegenprobe): der Adapter einer FREMDEN Strategie ist in den Adapter der "
              "Kompositions-Achse konvertierbar -- dann unterschiede die Relation die Achsen nicht.");

// -- Literaler Lauf-Ausweis (kein Erfolgs-Haken ohne Ausgabe) -------------------------------------
int g_fail = 0;

void tr(char const* what, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what);
    if (!ok) ++g_fail;
}

template <class T>
void report_organ(char const* stufe, std::string_view label) {
    bool const ok = s5::FamilyAllocConform<T>;
    std::printf("  [%s] %-8s %-46.*s Form: %s\n", ok ? " ok " : "FAIL", stufe, static_cast<int>(label.size()),
                label.data(), s5::family_alloc_form<T>());
    if (!ok) ++g_fail;
}

template <class List>
void report_list(char const* stufe) {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([&](auto id) {
        using T = typename decltype(id)::type;
        report_organ<T>(stufe, T::name());
    });
}

/// Die Trie-Backings tragen selbst kein name() (sie sind Struktur, keine Strategie) -- beschriftet wird
/// deshalb ueber die Strategie, die das Backing compile-zeit-selektiert hat (real_trie_selector).
void report_trie_backings() {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, pcx::AllCompressions>>([&](auto id) {
        using S = typename decltype(id)::type;
        report_organ<pcx::real_trie_t<S>>("TRIE", S::name());
    });
}

/// Die Stores tragen organ_name() (Store-Familie) -- beschriftet wird mit organ_name + node_name/layout_name,
/// damit die Kreuzung im Protokoll sichtbar ist. Ausgabe direkt, weil die Beschriftung dreiteilig ist.
template <class Store>
void report_store() {
    bool const ok = s5::FamilyAllocConform<Store>;
    std::printf("  [%s] %-8s %-14.*s N=%-10.*s L=%-18.*s Form: %s\n", ok ? " ok " : "FAIL", "STORE",
                static_cast<int>(Store::organ_name().size()), Store::organ_name().data(),
                static_cast<int>(Store::node_name().size()), Store::node_name().data(),
                static_cast<int>(Store::layout_name().size()), Store::layout_name().data(),
                s5::family_alloc_form<Store>());
    if (!ok) ++g_fail;
}

void report_store_backings() {
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, Family02aStoreBacking>>([&](auto id) {
        using T = typename decltype(id)::type;
        report_store<T>();
    });
}

/// (4) VERDRAHTUNGS-BELEG: ein frisches Organ hat 0 Achsen-Allokationen; nach echter Nutzung > 0.
/// Damit ist belegt, dass die Member-Allokation WIRKLICH ueber die Versorger-Achse laeuft und nicht
/// ueber einen Default-Allokator neben einer huebschen using-Zeile (Form-B-Grenze).
void probe_trie_wiring() {
    pcx::PatriciaTrie t{};
    auto const        before = t.trie_allocator_statistics();
    for (std::uint64_t k = 0; k < 512; ++k) t.insert_key(k * 2654435761ull + 7u);
    auto const after = t.trie_allocator_statistics();
    std::printf("     %-18s alloc_cnt %llu -> %llu   bytes_in_use %llu   keys %zu nodes %zu\n", "PatriciaTrie",
                static_cast<unsigned long long>(before.allocation_count),
                static_cast<unsigned long long>(after.allocation_count),
                static_cast<unsigned long long>(after.total_bytes_in_use), t.key_count(), t.node_count());
    tr("(4) frischer Trie hat noch KEINE Achsen-Allokation (ehrlicher Nullpunkt)", before.allocation_count == 0);
    tr("(4) nach 512 insert_key laeuft der Knoten-Pool REAL ueber die Achse (alloc_cnt > 0)",
       after.allocation_count > 0);
    tr("(4) die Achse haelt den Speicher auch (bytes_in_use > 0)", after.total_bytes_in_use > 0);
    tr("(4) und der Trie traegt die Schluessel wirklich (key_count == 512, Set-Semantik)", t.key_count() == 512u);
}

void probe_stream_wiring() {
    serx::SignalingStream st{};
    auto const            before  = st.stream_allocator_statistics();
    std::byte             pay[24] = {};
    for (std::size_t i = 0; i < 256; ++i) st.append(serx::SignalKind::Normal, std::span<std::byte const>(pay, 24));
    auto const after = st.stream_allocator_statistics();
    std::printf("     %-18s alloc_cnt %llu -> %llu   bytes_in_use %llu   entries %zu bytes %zu\n", "SignalingStream",
                static_cast<unsigned long long>(before.allocation_count),
                static_cast<unsigned long long>(after.allocation_count),
                static_cast<unsigned long long>(after.total_bytes_in_use), st.entry_count(), st.byte_size());
    tr("(4) frischer Stream hat noch KEINE Achsen-Allokation (ehrlicher Nullpunkt)", before.allocation_count == 0);
    tr("(4) nach 256 append laeuft der Puffer REAL ueber die Achse (alloc_cnt > 0)", after.allocation_count > 0);
    tr("(4) die Achse haelt den Speicher auch (bytes_in_use > 0)", after.total_bytes_in_use > 0);
    tr("(4) und das Wire-Format ist unveraendert (entry_count == 256, byte_size == 256*(1+1+24))",
       st.entry_count() == 256u && st.byte_size() == 256u * 26u);
}

/// (4-HERZ) DER Befund dieser Scheibe, zur Laufzeit: der Chunk-INDEX laeuft ueber DENSELBEN
/// Kompositions-Allokator wie die Record-Bytes. Die Aussage ist so gebaut, dass sie am ALT-Stand FAELLT:
/// vorher zaehlte allocator_statistics() AUSSCHLIESSLICH die Record-Block-Allokationen, also galt
/// allocation_count == chunk_alloc_count() EXAKT. Laeuft der Index mit, muss allocation_count ECHT groesser
/// sein (der Index-Vektor waechst geometrisch mit). Ein "> 0" allein haette hier NICHTS gepinnt -- die
/// Record-Bytes liefen ja schon immer ueber die Achse.
void probe_herz_chunk_index() {
    HerzStore  s{};
    auto const before = s.node_store_allocator_statistics();
    // Node4 -> cap_ = 4 Records je Chunk; 4096 Slots ergeben 1024 Chunks, also ein oft gewachsener Index.
    for (std::uint64_t i = 0; i < 4096; ++i) s.append_slot(i * 2654435761ull, i);
    auto const after = s.node_store_allocator_statistics();
    std::printf("     %-18s alloc_cnt %llu (frisch) -> %llu   chunk_alloc_count %zu   chunk_count %zu\n",
                "LayoutAwareStore", static_cast<unsigned long long>(before.allocation_count),
                static_cast<unsigned long long>(after.allocation_count), s.chunk_alloc_count(), s.chunk_count());
    tr("(4-HERZ) frischer Store hat noch KEINE Achsen-Allokation (ehrlicher Nullpunkt)", before.allocation_count == 0);
    tr("(4-HERZ) die Record-Bloecke laufen ueber die Achse (alloc_cnt >= chunk_alloc_count)",
       after.allocation_count >= s.chunk_alloc_count());
    tr("(4-HERZ) UND der Chunk-INDEX laeuft mit ueber DIESELBE Achse (alloc_cnt ECHT > chunk_alloc_count -- "
       "am Alt-Stand waren beide exakt gleich)",
       after.allocation_count > s.chunk_alloc_count());
    tr("(4-HERZ) der Index ist auch wirklich gewachsen (chunk_count == 1024 bei cap_=4)",
       s.chunk_count() == 1024u && s.chunk_alloc_count() == 1024u);
    // Gegenprobe innerhalb derselben Familie: der NodeChunkedStore meldet ueber allocator_statistics()
    // weiter die NODE-WIRKSAME (synthetische) Groesse -- dort darf der Schnitt NICHTS bewegt haben.
    ndx::NodeChunkedStore<PilotNode, PilotLayout, PilotAlloc> c{};
    auto const                                                c_before = c.node_store_allocator_statistics();
    for (std::uint64_t i = 0; i < 4096; ++i) c.append_slot(i * 2654435761ull, i);
    auto const c_axis = c.node_store_allocator_statistics();
    auto const c_node = c.allocator_statistics();
    std::printf("     %-18s Achse alloc_cnt %llu -> %llu | node-wirksam alloc_cnt %llu == chunk_alloc_count %zu\n",
                "NodeChunkedStore", static_cast<unsigned long long>(c_before.allocation_count),
                static_cast<unsigned long long>(c_axis.allocation_count),
                static_cast<unsigned long long>(c_node.allocation_count), c.chunk_alloc_count());
    tr("(4-HERZ) NodeChunkedStore: beide Ebenen des verschachtelten Containers laufen ueber die Achse",
       c_before.allocation_count == 0 && c_axis.allocation_count > 0);
    tr("(4-HERZ) NodeChunkedStore: die node-WIRKSAME Mess-Zeile ist vom Schnitt UNBEWEGT "
       "(allocator_statistics().alloc_cnt == chunk_alloc_count())",
       c_node.allocation_count == static_cast<std::uint64_t>(c.chunk_alloc_count()));
}

/// (5) MEMENTO-BELEG (R1, Leitplanke 3 der Par.-4.3-Direktive): die Kopie muss auf das EIGENE allocator_
/// rebinden. Beweis ohne Zeiger-Introspektion, rein aus den Achsen-Zaehlern -- und das DELTA wird VOR dem
/// restore-Effekt genommen (B2-Lehre): restore_statistics setzt die Kopie im Copy-Ctor auf den QUELL-Stand,
/// ein Vergleich nur der absoluten Zahlen saehe deshalb NICHTS. Gemessen wird die Bewegung, die eine
/// Mutation der Kopie DANACH erzeugt.
void probe_trie_memento() {
    pcx::PatriciaTrie src{};
    for (std::uint64_t k = 0; k < 512; ++k) src.insert_key(k * 2654435761ull + 7u);
    auto const src_before = src.trie_allocator_statistics();

    pcx::PatriciaTrie cp{src}; // R1-Memento-Kopie (genau das tut saved_pc_.emplace(pc_organ_))
    auto const        src_after_copy = src.trie_allocator_statistics();
    auto const        cp_after_copy  = cp.trie_allocator_statistics();

    for (std::uint64_t k = 512; k < 1536; ++k) cp.insert_key(k * 2654435761ull + 7u); // Mutation NUR der Kopie
    auto const src_after_mut = src.trie_allocator_statistics();
    auto const cp_after_mut  = cp.trie_allocator_statistics();

    std::printf("     %-18s src alloc_cnt %llu -> %llu (nach Kopie) -> %llu (nach Mutation der Kopie)\n",
                "PatriciaTrie", static_cast<unsigned long long>(src_before.allocation_count),
                static_cast<unsigned long long>(src_after_copy.allocation_count),
                static_cast<unsigned long long>(src_after_mut.allocation_count));
    std::printf("     %-18s cp  alloc_cnt %llu (nach Kopie, restore-bereinigt) -> %llu (nach Mutation)   "
                "keys src/cp %zu/%zu\n",
                "PatriciaTrie", static_cast<unsigned long long>(cp_after_copy.allocation_count),
                static_cast<unsigned long long>(cp_after_mut.allocation_count), src.key_count(), cp.key_count());

    tr("(5a) die Kopie hat NICHT aus dem Allokator der Quelle alloziert (Quell-Zaehler unveraendert)",
       src_after_copy.allocation_count == src_before.allocation_count);
    tr("(5b) DELTA-Probe: die Mutation der Kopie bewegt den Allokator DER KOPIE (Kopie-Zaehler steigt)",
       cp_after_mut.allocation_count > cp_after_copy.allocation_count);
    tr("(5b) und laesst die Quelle unberuehrt (Quell-Zaehler weiterhin unveraendert)",
       src_after_mut.allocation_count == src_before.allocation_count);
    tr("(5b) die Quelle behaelt ihren Inhalt (512 Schluessel, keine geteilte Struktur)", src.key_count() == 512u);
    tr("(5b) und die Kopie ist eigenstaendig gewachsen (1536 Schluessel)", cp.key_count() == 1536u);

    pcx::PatriciaTrie fresh{};
    for (std::uint64_t k = 0; k < 512; ++k) fresh.insert_key(k * 2654435761ull + 7u);
    tr("(5c) Memento-Vertrag: gleicher Inhalt vergleicht bit-exakt gleich (operator==)", fresh == src);
    tr("(5c) und ungleicher Inhalt NICHT (Gegenprobe -- sonst pinnt operator== nichts)", !(cp == src));
    pcx::PatriciaTrie assigned{};
    assigned = src; // der Rueckspiel-Pfad (pc_organ_ = *saved_pc_)
    tr("(5c) Rueckspiel per copy-assign ist inhaltsgleich (operator==)", assigned == src);
    tr("(5c) und der Rueckspiel-Empfaenger hat aus SEINEM Allokator alloziert",
       assigned.trie_allocator_statistics().allocation_count > 0);
}

/// (5-node) Die node-Store-Kopie hat KEIN restore_statistics (bewusste Abweichung vom btree-Muster, im
/// Header begruendet): sie materialisiert ein vollstaendig eigenes Backing. Ihr Zaehler zeigt deshalb ihr
/// eigenes Backing DIREKT -- die Delta-Klammer der Trie-Probe ist hier gar nicht noetig, und genau das
/// macht die Probe zur unabhaengigen zweiten Aussage.
void probe_store_copy_rebind() {
    HerzStore src{};
    for (std::uint64_t i = 0; i < 1024; ++i) src.append_slot(i * 2654435761ull, i);
    auto const src_before = src.node_store_allocator_statistics();

    HerzStore  cp{src};
    auto const src_after = src.node_store_allocator_statistics();
    auto const cp_after  = cp.node_store_allocator_statistics();
    std::printf("     %-18s src alloc_cnt %llu -> %llu (nach Kopie)   cp alloc_cnt %llu   slots src/cp %zu/%zu\n",
                "LayoutAwareStore", static_cast<unsigned long long>(src_before.allocation_count),
                static_cast<unsigned long long>(src_after.allocation_count),
                static_cast<unsigned long long>(cp_after.allocation_count), src.slot_count(), cp.slot_count());
    tr("(5-node) die Kopie hat NICHT aus dem Allokator der Quelle alloziert (Quell-Zaehler unveraendert)",
       src_after.allocation_count == src_before.allocation_count);
    tr("(5-node) die Kopie hat aus IHREM eigenen Allokator alloziert (kein restore -> direkt sichtbar)",
       cp_after.allocation_count > 0);
    tr("(5-node) und der Inhalt ist byte-genau uebernommen (gleiche Slot-Zahl, gleiche Keys)", [&] {
        if (src.slot_count() != cp.slot_count()) return false;
        for (std::size_t i = 0; i < src.slot_count(); ++i)
            if (src.key_at(i) != cp.key_at(i) || src.value_at(i) != cp.value_at(i)) return false;
        return true;
    }());
}

} // namespace

int main() {
    std::printf("== A8-S5 Familie 02_layout / Sub-Scheibe 02a -- Allokator-Achsen-Konformitaet ==\n");
    std::printf("   Achsen: node_type (T4) . memory_layout (T5) . path_compression (T3) . serialization (T9)\n");
    std::printf("   (filter/SuRF = Sub-Scheibe 02b, bewusst NICHT hier gepinnt -- eigene Wache: "
                "test_s5_02b_filter_alloc_conformance)\n");

    std::printf("-- (1) Strategie-Typen aus den Achsen-Registries --\n");
    report_list<Family02aStrategies>("STRAT");
    std::printf("-- (2) Organ-Huellen (Member-Typen der Komposition) --\n");
    report_list<Family02aOrgans>("ORGAN");
    std::printf("-- (3a) path_compression-Trie-Backings, beschriftet mit ihrer Strategie (Scrub-Gegenstand) --\n");
    report_trie_backings();
    std::printf("-- (3b) node-Chunk-Stores, Achsen-Kreuzungen aus den Registries (Scrub-Gegenstand) --\n");
    report_store_backings();
    std::printf("-- (3c) serialization-Stream-Backing (Scrub-Gegenstand) --\n");
    report_organ<serx::SignalingStream>("STREAM", "signaling_stream");

    std::printf("-- (4) LAUFZEIT: reale Verdrahtung der Form-B-Organe --\n");
    probe_trie_wiring();
    probe_stream_wiring();
    std::printf("-- (4-HERZ) LAUFZEIT: der Chunk-INDEX laeuft ueber den KOMPOSITIONS-Allokator --\n");
    probe_herz_chunk_index();

    std::printf("-- (5) LAUFZEIT: R1-Memento rebindet auf den EIGENEN Allokator (path_compression) --\n");
    probe_trie_memento();
    std::printf("-- (5-node) LAUFZEIT: die Store-Kopie rebindet ebenfalls (ohne restore, direkt sichtbar) --\n");
    probe_store_copy_rebind();

    std::printf("-- Familien-Bilanz --\n");
    std::printf("  Strategien: %zu   Organ-Huellen: %zu   Trie-Backings: %zu   Store-Kreuzungen: %zu   "
                "Allokator-Versorger: %zu\n",
                static_cast<std::size_t>(mp::mp_size<Family02aStrategies>::value),
                static_cast<std::size_t>(mp::mp_size<Family02aOrgans>::value),
                static_cast<std::size_t>(mp::mp_size<Family02aTrieBacking>::value),
                static_cast<std::size_t>(mp::mp_size<Family02aStoreBacking>::value),
                static_cast<std::size_t>(mp::mp_size<alx::AllVendors>::value));
    std::printf("  memory_layout: Familien-grep 0 Code-Treffer -- kein Scrub noetig, aber von (1)/(2)/(3b)\n");
    std::printf("  mitgepinnt, damit ein kuenftiger Container dort sofort auffaellt.\n");

    std::printf("== test_s5_02a_layout_alloc_conformance: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
