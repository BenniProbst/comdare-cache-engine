#pragma once
// Layout-honorierender Storage — LayoutAwareChunkedStore<N,L,A> (P-MD1-ERDUNG / #167, 2026-06-18).
//
// @topic nodes @achse 04 @schicht composable-bridge (node_type ⊕ layout ⊕ allocator ALS Storage-Organ, L-WIRKSAM)
//
// **Zweck (User 2026-06-18 „5 REALE distinkte memory_layout-Repraesentationen"):** Frueher (Phase 1, 2026-06-04)
// speicherte der Store ALLE Layouts als 16-B-`[key|value]` an einem einzigen `eff_stride` (16 oder 64) — nur die
// AoS-Familie war ehrlich; SoA/AoSoA/packed_bitmap wurden wie AoS abgelegt, und die CLU kam aus einem ENTKOPPELTEN
// Deskriptor-Modell (record_useful_bytes/record_line_span je Strategie), das den realen Store IGNORIERTE
// (Phantom-Muster P-MD1). Diese Fassung ERDET das: jedes Layout speichert ueber seine `RepresentationKind`
// (axis_05_memory_layout_strategy_base.hpp) WIRKLICH unterschiedlich, compile-time-dispatched (`if constexpr`,
// zero-cost), und die CLU/field_bytes/cache_lines kommen aus dem REAL beruehrten Key-Scan-Footprint der echten
// Repraesentation.
//
// **Die 5 REALEN Repraesentationen (Strategy je Layout, store_record/load_key/load_value):**
//   • aos_interleaved_packed (aos_strict):        [key|value] adjazent, 16-B-Stride, dicht.
//   * aos_interleaved_padded (cache_line_aligned): [key|value|pad] auf Cache-Line-Stride der cacheline-
//     Unterachse (am Achsen-Default 64 B: [key|value|48 B pad]).
//   • soa_split_columns (soa):                     keys[]-Spalte gefolgt von values[]-Spalte (ZWEI Arrays je Chunk).
//   • aosoa_blocked_columns (aosoa):               pro Block B keys dann B values, Bloecke als Array (SIMD-tiled).
//   • succinct_hot_cold_split (packed_bitmap):     2-B-Hot-Key-Spalte + 6-B-Cold-Residue + values; VERLUSTFREI.
// JEDE Rep speichert den vollen uint64-Key+Value VERLUSTFREI (Round-Trip insert→lookup korrekt) — der UNTERSCHIED
// liegt im physischen Byte-Layout und damit im REALEN Key-Scan-Footprint (CLU 5-fach distinkt).
//
// Erfuellt das StorageOrgan-Concept (aktuelle Default-Key-Breite uint64, 8 Methoden, byte-codiert) → Drop-in fuer ComposedSearch,
// parallel zu NodeChunkedStore (das Bestehende bleibt unangetastet). Memento ist LOGISCH (copy_from_ kopiert die
// Chunk-Buffer byte-genau → deckt ALLE Reps ab, weil die Bytes selbst die Repraesentation sind).
//
// A8-S5 Familie 02a (2026-08-04) -- HERZ-SCHNITT der Scheibe (Dossier 20260803-a8_f2 Abschn. 3.4,
// "Speicher NUR ueber das Allokator-Achsen-Interface"): die RECORD-BYTES liefen hier von Anfang an ueber den
// Kompositions-Allokator A (append_slot/copy_from_ -> alloc_.allocate, free_chunks_ -> alloc_.deallocate) --
// der CHUNK-INDEX aber nicht. `std::vector<Chunk> chunks_` hing am Default-Allokator: die Verwaltung der
// ueber die Achse besorgten Bloecke wurde an der Achse VORBEI besorgt. Das war der letzte Fremdgang dieses
// Stores und zugleich der einzige, bei dem die ECHTE Kompositions-Bindung moeglich ist (der Store ist Template
// ueber A) -- ein benannter Achsen-Default waere hier eine zweite, stille Speicherquelle gewesen.
// WIRKUNG AUF DIE MESSUNG (Auflage 6, ehrlich ausgewiesen statt stillschweigend): allocator_statistics() ist
// die T6-Route dieses Stores und meldet alloc_.statistics(). Seit dem Schnitt zaehlt sie ZUSAETZLICH die
// (wenigen, geometrisch wachsenden) Index-Allokationen mit -- vorher waren sie unsichtbar, obwohl sie real
// stattfanden. Die Zahl steigt also, sie wird dabei WAHRER: T6 sieht jetzt alle Speicher-Ereignisse des
// Organs (Owner-KERN 04.08. abend-11: T6 = Option B strikt). Groessenordnung: chunk_alloc_count() Record-
// Allokationen (linear in der Slot-Zahl) gegen O(log chunk_count) Index-Allokationen. Vor dem Schnitt galt
// allocation_count == chunk_alloc_count() exakt; danach gilt allocation_count > chunk_alloc_count(), sobald
// mehr als ein Chunk existiert -- genau daran beisst die Familien-Wache.

#include "axis_04_node_type_node4.hpp" // Pilot-NodeType (Selbstbeweis)
#include "concepts/axis_04_node_type_concept.hpp"
#include <axes/cacheline/cacheline_line_bytes.hpp> // P-CACHELINE-LITERAL: Line-Groesse NUR aus der cacheline-Achse
#include <axes/cacheline/node_width_config.hpp>    // C2/FF2: Knoten-Breite-in-Cache-Lines-Unterachse (Konsum hier)
#include <topics/memory_layout/axis_05_memory_layout/concepts/axis_05_memory_layout_concept.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_strategy_base.hpp> // RepresentationKind
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_concept.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/storage_organ_concept.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/composable_search.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace comdare::cache_engine::node {

namespace _ml_la = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace _al_la = ::comdare::cache_engine::allocator::axis_06_allocator;

/// layout-honorierendes, node-gechunktes 3-Achsen-Storage-Organ: Slots liegen in node-grossen Chunks der
/// Kapazitaet N::max_capacity(). Das PHYSISCHE Byte-Layout je Chunk ist compile-time-dispatched ueber
/// L::representation_kind() (zero-cost `if constexpr`) — 5 REALE distinkte Repraesentationen (#167).
template <class N, class L, class A>
    requires concepts::NodeTypeStrategy<N> && _ml_la::concepts::MemoryLayoutStrategy<L> &&
             _al_la::concepts::AllocatorStrategy<A>
class LayoutAwareChunkedStore {
private:
    using RK                 = ::comdare::cache_engine::layout::RepresentationKind;
    static constexpr RK kRep = L::representation_kind();

    static constexpr std::size_t kKeyBytes = sizeof(std::uint64_t); // 8
    static constexpr std::size_t kValBytes = sizeof(std::uint64_t); // 8
    static constexpr std::size_t kKvBytes  = kKeyBytes + kValBytes; // 16 logische Nutzlast
    // P-CACHELINE-LITERAL (2026-08-04, generalisierte Schnitt-Regel): die Cache-Line-Groesse (CLU-Bezugs-
    // groesse, AoS-Padding-Granularitaet, FF2-Knotenbreiten-Faktor, Chunk-Alignment) ist Eigentum der
    // cacheline-Unterachse und wird hier BEZOGEN statt hartkodiert. Quelle = das Layout-Organ L, dessen
    // MemoryLayoutStrategyBase die Unterachse als NTTP traegt (cacheline_subaxis_line_bytes()); traegt ein
    // L die Naht nicht, greift der Achsen-Default (kDefaultLineBytes == 64) -- NIE ein eigenes Literal.
    // Am heutigen Default ist der Bezug wertgleich zur frueheren 64 (static_assert am Dateiende).
    // NICHT L::cache_line_size(): das ist der intrinsische Design-Deskriptor der Layout-Strategie
    // (aos_strict = 1, packed_bitmap = 8) und waere der Duplikat-Bug aus axis_05_..._cache_line_aligned:54.
    // OFFENER PUNKT FUER KF-6 (Posten 62), hier sichtbar gemacht statt still entschieden: die
    // cacheline-Unterachse ist PER ORGAN, dieser Store ist aber ein 3-Achsen-Verbund (N, L, A) und alle
    // drei koennen unter KF-6 eine eigene Config tragen. Heute stehen alle am selben Default, die Frage ist
    // also unsichtbar. Sie wird es NICHT bleiben: cacheline_study.profile.xml fuehrt als per_organ-Traeger
    // "page_type node_type cache_traversal allocator" -- memory_layout ist dort NICHT gelistet. Wer die
    // Durchbindung baut, muss also zuerst entscheiden, WELCHES Organ die Chunk-Geometrie regiert (bzw. eine
    // Vorrang-/Konflikt-Regel deklarieren); erst danach ist diese Zeile die richtige oder die falsche.
    static constexpr std::size_t kLineBytes = ::comdare::cache_engine::cacheline::line_bytes_of<L>();
    static constexpr std::size_t kHotBytes  = 2;                     // succinct: low16-Hot-Key-Spalte
    static constexpr std::size_t kColdBytes = kKeyBytes - kHotBytes; // succinct: high48-Cold-Residue (6)

    static constexpr std::size_t round_up_(std::size_t v, std::size_t a) noexcept {
        return a <= 1 ? v : ((v + a - 1u) & ~(a - 1u));
    }

    // C2/FF2 (GO4/#8 F-C, 2026-07-12): physische Record-Bytes als private constexpr-Quelle, damit die
    // width-getriebene Kapazitaet (cap_, unten) sie VOR der public-Deklaration von record_phys_bytes()
    // nutzen kann. Wert-identisch zu record_phys_bytes() (das jetzt hierauf delegiert).
    static constexpr std::size_t record_bytes_() noexcept {
        return (kRep == RK::aos_interleaved_padded) ? round_up_(kKvBytes, kLineBytes) : kKvBytes;
    }

    // C2/FF2: die vom Node-Organ deklarierte Knoten-Breite in Cache-Lines (NodeWidthAware, node_width_config).
    // 0 = Native (keine Vorgabe) -> Kapazitaet bleibt N::max_capacity() (byte-identisch zum Ist-Stand).
    static constexpr std::size_t node_width_lines_ = [] {
        if constexpr (::comdare::cache_engine::cacheline::NodeWidthConfigurable<N>)
            return static_cast<std::size_t>(N::node_width_in_lines());
        else
            return std::size_t{0};
    }();

public:
    using key_type       = std::uint64_t; // aktuelle Default-Breite; native schmalere Container = #217-2b
    using value_type     = std::uint64_t;
    using node_type      = N;
    using layout_type    = L;
    using allocator_type = A;

    /// Natuerliche Kapazitaet (Records je Chunk) aus dem Node-Organ (bisheriges Verhalten).
    static constexpr std::size_t nat_cap_ = (N::max_capacity() == 0 ? std::size_t{1} : N::max_capacity());
    /// C2/FF2 „Knoten-Breite in Cache-Lines" (1..16): deklariert das Node-Organ eine nicht-native Breite W
    /// (NodeWidthAware, profil-aktivierte Unterachse), wird der Chunk (= Knoten-Backing) W Cache-Lines breit —
    /// die Kapazitaet folgt aus dem physischen Record-Layout: floor(W * kLineBytes / record_bytes), min. 1.
    /// kLineBytes ist die Line-Groesse der cacheline-Unterachse (P-CACHELINE-LITERAL, am Default 64). Native
    /// (Default aller bestehenden Node-Blaetter) → nat_cap_, byte-identisch zum Ist-Stand (golden-/ABI-neutral).
    /// Thesis FF2 (01_einleitung.tex:94-99): CSS/CSB+ = 1 Cache-Line vs. Hankins/Patel = 16 Cache-Lines.
    static constexpr std::size_t cap_ = (node_width_lines_ == 0)
                                            ? nat_cap_
                                            : (((node_width_lines_ * kLineBytes) / record_bytes_()) == 0
                                                   ? std::size_t{1}
                                                   : (node_width_lines_ * kLineBytes) / record_bytes_());
    /// AoSoA PHYSISCHE Store-Blockbreite (B keys dann B values, Bloecke als Array). Bewusst KEINE Line-teilende
    /// Lane-Zahl (waere lane-aligned identisch zum SoA-Key-Footprint), sondern eine Tile-Breite, deren Key-Lane
    /// die Cache-Linien der Unterachse STRADDLET -> der Key-Scan-Footprint liegt ECHT zwischen SoA (dicht)
    /// und AoS (strided),
    /// byte-distinkt von beiden (Node-kapazitaets-gedeckelt; „B = node-Kapazitaet o.ae.", Aufgabe #167). Distinkt
    /// von der SIMD-`block_width()` (=8) der Strategie, die NUR den scan_field_sum-Vergleich steuert (unveraendert).
    static constexpr std::size_t kStoreBlock = 10;
    static constexpr std::size_t kBlockW     = (kStoreBlock < cap_) ? kStoreBlock : cap_;

    /// record_phys_bytes: die PHYSISCH pro Record im Chunk verbrauchten Bytes (Single-Record-Stride bzw.
    /// amortisierter Spaltenanteil). AoS-padded = round_up(16, kLineBytes) (am Achsen-Default 64), sonst 16
    /// (alle uebrigen Reps speichern Key+Value
    /// verlustfrei in 16 B, nur ANDERS angeordnet). Bestimmt die Chunk-Allokationsgroesse.
    static constexpr std::size_t record_phys_bytes() noexcept {
        return record_bytes_(); // C2/FF2: delegiert an die private constexpr-Quelle (wert-identisch)
    }
    static constexpr std::size_t eff_stride = record_phys_bytes(); // Rueckwaerts-Kompat-Name (AoS-Stride)

    static constexpr std::size_t node_capacity_v = cap_;
    static constexpr std::size_t cache_line_size = L::cache_line_size();

    /// chunk_bytes: die GESAMTE ueber A allozierte Chunk-Kapazitaet (cap_ Records) je Repraesentation. AoSoA
    /// rundet auf VOLLE Bloecke auf (ceil(cap/B) Bloecke * B * 16 B), weil ein angebrochener letzter Block sein
    /// volles Key+Value-Lane-Paar reserviert (sonst OOB, wenn cap kein B-Vielfaches ist).
    static constexpr std::size_t chunk_bytes() noexcept {
        if constexpr (kRep == RK::aosoa_blocked_columns) {
            std::size_t const blocks = (cap_ + kBlockW - 1u) / kBlockW;
            return blocks * kBlockW * kKvBytes;
        } else {
            return cap_ * record_phys_bytes();
        }
    }

    [[nodiscard]] static constexpr std::size_t node_capacity() noexcept { return cap_; }
    /// P-CACHELINE-LITERAL: die wirksame Cache-Line-Groesse dieses Stores -- ausschliesslich aus der
    /// cacheline-Unterachse des Layout-Organs bezogen (Wache test_p_cacheline_store_line_source pinnt das).
    [[nodiscard]] static constexpr std::size_t cacheline_line_bytes() noexcept { return kLineBytes; }
    /// P-CACHELINE-LITERAL: Alignment der ueber A allozierten Chunks (== Cache-Line-Groesse der Achse).
    [[nodiscard]] static constexpr std::size_t chunk_align() noexcept { return kChunkAlign; }
    /// C2/FF2: die wirksame Knoten-Breite in Cache-Lines (0 = Native, Knoten-Backing unveraendert).
    [[nodiscard]] static constexpr std::size_t      node_width_in_lines() noexcept { return node_width_lines_; }
    [[nodiscard]] static constexpr std::size_t      record_stride() noexcept { return record_phys_bytes(); }
    [[nodiscard]] static constexpr std::string_view organ_name() noexcept { return "node_layout_aware"; }
    [[nodiscard]] static constexpr std::string_view node_name() noexcept { return N::name(); }
    [[nodiscard]] static constexpr std::string_view layout_name() noexcept { return L::name(); }
    [[nodiscard]] static constexpr std::string_view allocator_name() noexcept { return A::name(); }
    [[nodiscard]] static constexpr RK               representation() noexcept { return kRep; }

    [[nodiscard]] std::size_t chunk_count() const noexcept { return chunks_.size(); }
    [[nodiscard]] std::size_t chunk_alloc_count() const noexcept { return chunk_allocs_; }

    // -- A8-S5-02a Lebensdauer-Vertrag der Achsen-Verdrahtung (HERZ-SCHNITT, s. Kopf) -------------------
    // Seit dem Schnitt haengt AUCH der Chunk-INDEX am Adapter von alloc_. Der StdAllocatorAdapter haelt
    // einen Zeiger auf alloc_ -> dieselbe Regel wie im Referenz-Muster btree_node_pool_store.hpp:19/:84-105:
    //   (a) alloc_ ist VOR chunks_ deklariert (Member-Reihenfolge unten -- war schon so, gilt jetzt hart),
    //   (b) chunks_ wird IMMER mit alloc_.as_std_allocator<Chunk>() konstruiert (der Adapter traegt keinen
    //       Default-Ctor -> ein stilles Zurueckfallen auf einen Default-Allokator ist compile-hart
    //       ausgeschlossen; das war GENAU der Fremdgang, den diese Scheibe schliesst),
    //   (c) die Kopie legt ihr eigenes alloc_ an und materialisiert ueber copy_from_ ein VOLLSTAENDIG
    //       eigenes Backing (Index UND Record-Bytes) -- unveraendertes Verhalten, jetzt nur beide Ebenen
    //       an derselben Achse. KEIN restore_statistics (Abweichung zum btree-Muster, bewusst): es gibt
    //       keine geteilte Struktur und damit keine COW-Pollution zu verwerfen; die Zaehler der Kopie
    //       beschreiben ehrlich IHR eigenes Backing. Das ist zugleich das VOR-Schnitt-Verhalten.
    //   (d) Move ist BEWUSST ENTFALLEN (vorher: noexcept-Move, der chunks_ samt Fremd-Adapter stahl). Mit
    //       dem Index an der Achse waere ein Move entweder falsch (der Adapter der Kopie zeigte auf die
    //       Quelle) oder allokator-erweitert -- und der allokator-erweiterte Move ALLOZIERT (die Adapter
    //       zweier Stores sind nie gleich), duerfte also nicht noexcept heissen. Ein noexcept-Versprechen,
    //       das bei OOM terminiert, waere eine stille Verschlechterung. Die benutzerdeklarierte Kopie
    //       unterdrueckt den impliziten Move; ein std::move/`return s;` degradiert zur korrekt rebindenden
    //       Kopie (semantisch identisch, copy_from_ dupliziert die Bytes ohnehin byte-genau).
    LayoutAwareChunkedStore() : chunks_(alloc_.template as_std_allocator<Chunk>()) {}
    ~LayoutAwareChunkedStore() { free_chunks_(); }
    LayoutAwareChunkedStore(LayoutAwareChunkedStore const& o) : chunks_(alloc_.template as_std_allocator<Chunk>()) {
        copy_from_(o);
    }
    LayoutAwareChunkedStore& operator=(LayoutAwareChunkedStore const& o) {
        if (this != &o) {
            free_chunks_();
            // A8-S5-02a: der Index-PUFFER selbst haengt jetzt an alloc_ -> er muss NOCH ueber die aktuelle
            // Strategie-Instanz freigegeben werden. Ein blosses clear() liesse ihn stehen; seine spaetere
            // Freigabe liefe dann gegen das frisch zurueckgesetzte alloc_ (Zaehler-Asymmetrie).
            release_index_();
            size_                           = 0;
            chunk_allocs_                   = 0;
            runtime_pool_budget_bytes_      = 0;
            runtime_pool_budget_rejections_ = 0;
            alloc_                          = A{};
            copy_from_(o);
        }
        return *this;
    }

    // --- StorageOrgan-Concept: 8 Methoden ueber logischem Flach-Index (Chunk c=i/cap_, Slot j=i%cap_) ---
    [[nodiscard]] std::size_t slot_count() const noexcept { return size_; }
    [[nodiscard]] key_type key_at(std::size_t i) const noexcept { return load_key_(chunks_[i / cap_].data, i % cap_); }
    [[nodiscard]] value_type value_at(std::size_t i) const noexcept {
        return load_value_(chunks_[i / cap_].data, i % cap_);
    }
    void set_value_at(std::size_t i, value_type v) noexcept { store_value_(chunks_[i / cap_].data, i % cap_, v); }

    // K9-Fix (prefetch REAL): NUR-LESE-Adresse des KEY von Slot i im realen Chunk-Backing (representation-aware).
    [[nodiscard]] unsigned char const* slot_address(std::size_t i) const noexcept {
        return (i < size_) ? key_ptr_(chunks_[i / cap_].data, i % cap_) : nullptr;
    }
    [[nodiscard]] unsigned char const* backing_begin() const noexcept {
        return chunks_.empty() ? nullptr : chunks_.front().data;
    }
    [[nodiscard]] unsigned char const* backing_end() const noexcept {
        return chunks_.empty() ? nullptr : chunks_.front().data + chunks_.front().capacity;
    }
    [[nodiscard]] std::size_t chunk_capacity_bytes() const noexcept {
        return chunks_.empty() ? 0u : chunks_.front().capacity;
    }

    /// SONDERFALL [[allocation-failure-exception]] (A8-S5-02a, Auflage 11 -- Fehlerklassen): ZWEI
    /// Speicher-Ereignisse, seit dem HERZ-Schnitt beide an DERSELBEN Achse. (1) die Record-Bytes ueber
    /// alloc_.allocate -- die Strategie meldet OOM per nullptr, den der Store hier UNGEPRUEFT weiterreicht
    /// (memset auf nullptr = der vorbestehende UB-Pfad dieser Zeile; er gehoert dem alloc-/A15-Strang und
    /// ist NICHT Gegenstand dieser Scheibe -- als offener Punkt gemeldet, nicht heimlich gedreht).
    /// (2) das Wachstum des Chunk-INDEX ueber den StdAllocatorAdapter -- dort uebersetzt Posten 64 den
    /// nullptr an EINER Stelle in std::bad_alloc (axis_06_allocator_strategy_base.hpp). Fehlerklasse
    /// unveraendert der FK-5-Boden der Allokator-Achse (kOrganAxisErrorFloor).
    void append_slot(key_type k, value_type v) {
        if (chunks_.empty() || chunks_.back().count == cap_) {
            Chunk c;
            c.capacity = chunk_bytes();
            c.data     = static_cast<unsigned char*>(alloc_.allocate(c.capacity, kChunkAlign));
            std::memset(c.data, 0, c.capacity);
            c.count = 0;
            chunks_.push_back(c);
            ++chunk_allocs_;
        }
        Chunk& c = chunks_.back();
        store_record_(c.data, c.count, k, v);
        ++c.count;
        ++size_;
    }
    void insert_slot_at(std::size_t i, key_type k, value_type v) {
        auto flat = flatten_();
        flat.emplace(flat.begin() + static_cast<std::ptrdiff_t>(i), k, v);
        rebuild_(flat);
    }
    void erase_slot_at(std::size_t i) {
        auto flat = flatten_();
        flat.erase(flat.begin() + static_cast<std::ptrdiff_t>(i));
        rebuild_(flat);
    }
    void clear() noexcept {
        free_chunks_();
        chunks_.clear();
        size_                           = 0;
        chunk_allocs_                   = 0;
        runtime_pool_budget_rejections_ = 0;
    }

    // Observer-Strategy (M2, Dossier 18 A.3): set_runtime_pool_budget ist die austauschbare
    // Wirkungs-Strategie der Achse; Min/Max-Semantik: weniger Fragmentierung vs. Alloc-Durchsatz.
    void set_runtime_pool_budget(std::uint64_t budget_bytes) noexcept { runtime_pool_budget_bytes_ = budget_bytes; }
    [[nodiscard]] std::uint64_t runtime_pool_budget() const noexcept { return runtime_pool_budget_bytes_; }
    [[nodiscard]] bool          runtime_budget_allows_growth() const noexcept {
        if (runtime_pool_budget_bytes_ == 0) return true;
        if (!chunks_.empty() && chunks_.back().count < cap_) return true;
        return bytes_in_use_() + chunk_bytes() <= runtime_pool_budget_bytes_;
    }
    void                        note_runtime_pool_budget_rejection() noexcept { ++runtime_pool_budget_rejections_; }
    [[nodiscard]] std::uint64_t runtime_pool_budget_rejections() const noexcept {
        return runtime_pool_budget_rejections_;
    }

    // --- Drop-in-Paritaet zu ComposedStore/NodeChunkedStore ---
#ifdef COMDARE_CE_ENABLE_STATISTICS
    using allocator_snapshot_t = typename A::snapshot_t;
    [[nodiscard]] allocator_snapshot_t allocator_statistics() const noexcept { return alloc_.statistics(); }
    /// A8-S5-02a VERDRAHTUNGS-BELEG unter EINEM Namen fuer beide node-Chunk-Stores (NodeChunkedStore traegt
    /// dieselbe Methode; dort ist allocator_statistics() die synthetische node-wirksame Groesse, hier die
    /// echte). Die Familien-Wache fragt deshalb IMMER diese Zeile -- so ist der Beleg ueber beide Stores
    /// derselbe Ausdruck und kann nicht am Namen vorbeidriften.
    [[nodiscard]] allocator_snapshot_t node_store_allocator_statistics() const noexcept { return alloc_.statistics(); }
#endif

    // V2-Auto-Kopplung node_type: low-Byte je gespeichertem Key → Format-divergenter Self-Lookup.
    template <class NodeOrgan>
    std::uint64_t organ_observe_node_type(NodeOrgan& org) const {
        std::vector<std::uint8_t> kb;
        kb.reserve(size_);
        for_each_slot_([&](key_type k, value_type) { kb.push_back(static_cast<std::uint8_t>(k & 0xFFu)); });
        return org.observe_node_find(kb.data(), kb.size(), kb.data(), kb.size());
    }

    // V2-Auto-Kopplung layout (CLU-Treiber, P-MD1-ERDUNG): treibt den REALEN, representation-spezifischen
    // Key-Scan-Footprint je Chunk in den Observer. Der Footprint (field_bytes = real beruehrte Key-Nutzbytes,
    // cache_lines = real beruehrte Cache-Linien der Unterachse) wird byte-genau aus der echten Repraesentation berechnet
    // (key_scan_footprint_), die Checksumme aus dem echten Key-Scan (Korrektheits-Anker). KEIN entkoppelter
    // Deskriptor mehr.
    template <class LayoutOrgan>
    std::uint64_t organ_observe_layout(LayoutOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) {
            std::uint64_t checksum  = 0;
            std::uint64_t key_bytes = 0, lines = 0;
            key_scan_footprint_(c, checksum, key_bytes, lines);
            // B14-NB4: der Store nennt die EINHEIT seiner Linienzahl mit. `lines` entsteht in
            // key_scan_footprint_ aus kLineBytes = line_bytes_of<L>() -- diese Zeile reicht GENAU diese
            // Quelle weiter, damit der Verbraucher hinter der Modul-ABI (ObserverSnapshotSystemAxis) die
            // CLU mit demselben Nenner bildet, mit dem hier gezaehlt wurde. Ohne sie waere die Einheit
            // am Verbraucher nicht rekonstruierbar (observable_tier.hpp-POD transportierte sie nicht).
            //
            // DER STILLE FALLBACK, DEN DIESE SIGNATUR-AENDERUNG SONST GEOEFFNET HAETTE (B14-NB4): das
            // `else` unten ist ein LEGITIMER Weg fuer Organe, die gar keine Real-Footprint-API haben --
            // aber ein Organ, das die ALTE Vier-Argument-Form noch traegt, faende hier ab jetzt lautlos
            // denselben Ausgang und maesse plotzlich ueber observe_scan (anderer Zaehler, anderer
            // Stride). Genau das ist die Klasse "stiller Fallback, gegen den man selbst argumentiert".
            // Deshalb: wer die Real-Footprint-API ueberhaupt anbietet, MUSS die Einheit mitfuehren.
            static_assert(!requires {
                              org.observe_real_footprint(std::uint64_t{}, std::size_t{}, std::uint64_t{},
                                                         std::uint64_t{});
                          } || requires {
                              org.observe_real_footprint(std::uint64_t{}, std::size_t{}, std::uint64_t{},
                                                         std::uint64_t{}, std::uint64_t{});
                          },
                          "B14-NB4: dieses Layout-Organ traegt die ALTE observe_real_footprint-Form ohne "
                          "line_bytes. Es wuerde lautlos auf den observe_scan-Pfad zurueckfallen und eine "
                          "ANDERE Groesse messen. Die Einheit der Linienzahl gehoert in die Signatur -- "
                          "Organ auf die Fuenf-Argument-Form ziehen.");
            if constexpr (requires {
                              org.observe_real_footprint(checksum, std::size_t{}, std::uint64_t{}, std::uint64_t{},
                                                         std::uint64_t{});
                          }) {
                acc += org.observe_real_footprint(checksum, c.count, key_bytes, lines,
                                                  static_cast<std::uint64_t>(kLineBytes));
            } else {
                // Fallback (alte Observer ohne Real-Footprint-API): der bestehende observe_scan-Pfad.
                acc += org.observe_scan(c.data, c.count, record_phys_bytes());
            }
        }
        return acc;
    }
    template <class SerOrgan>
    std::uint64_t organ_observe_serialization(SerOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) acc += org.observe_serialize(c.data, c.count, record_phys_bytes());
        return acc;
    }
    template <class VhOrgan>
    std::uint64_t organ_observe_value_handle(VhOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) acc += org.observe_value_handle(c.data, c.count, record_phys_bytes());
        return acc;
    }
    template <class IsaOrgan>
    std::uint64_t organ_observe_isa(IsaOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) {
            std::size_t const words = (c.count * record_phys_bytes()) / sizeof(std::uint32_t);
            acc += org.observe_simd_field_sum(c.data, words);
        }
        return acc;
    }
    template <class IdxOrgan>
    std::uint64_t organ_observe_index_org(IdxOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) acc += org.index_org_observe(c.data, c.count, record_phys_bytes());
        return acc;
    }
    template <class IoOrgan>
    std::uint64_t organ_observe_io_dispatch(IoOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) acc += org.observe_dispatch(c.data, c.count, record_phys_bytes());
        return acc;
    }
    template <class MigOrgan>
    std::uint64_t organ_observe_migration(MigOrgan& org) const {
        for (auto const& c : chunks_) org.observe_decide(c.data, c.count, record_phys_bytes());
        return 0;
    }
    template <class FltOrgan>
    std::uint64_t organ_observe_filter(FltOrgan& org) const {
        if constexpr (requires { org.observe_probe_keys(static_cast<std::uint64_t const*>(nullptr), std::size_t{}); }) {
            std::vector<std::uint64_t> ks;
            ks.reserve(size_);
            for_each_slot_([&](key_type k, value_type) { ks.push_back(k); });
            return org.observe_probe_keys(ks.data(), ks.size());
        } else {
            std::vector<unsigned char> kb;
            kb.reserve(size_);
            for_each_slot_([&](key_type k, value_type) { kb.push_back(static_cast<unsigned char>(k & 0xFFu)); });
            return org.observe_probe(kb.data(), kb.size(), kb.data(), kb.size());
        }
    }

    // P4 — ECHTER 2-Ebenen-Migrations-Schritt (representation-agnostisch ueber die logische (k,v)-Sicht).
    template <class MigOrgan>
    std::uint64_t organ_migrate_step(MigOrgan const& org, LayoutAwareChunkedStore& tier1, std::uint64_t max_moves = 0) {
        auto const          flat = flatten_();
        std::vector<slot_t> survivors;
        survivors.reserve(flat.size());
        std::uint64_t moved                                      = 0;
        std::uint32_t prev_recency                               = 0;
        unsigned char rec[sizeof(key_type) + sizeof(value_type)] = {};
        for (auto const& s : flat) {
            std::memcpy(rec, &s.first, sizeof(s.first));
            std::memcpy(rec + sizeof(s.first), &s.second, sizeof(s.second));
            std::uint32_t cur = 0;
            std::memcpy(&cur, rec, sizeof(cur));
            bool const migrate =
                (max_moves == 0 || moved < max_moves) && org.should_migrate_record(rec, sizeof(rec), prev_recency);
            if (migrate) {
                tier1.append_slot(s.first, s.second);
                ++moved;
            } else {
                survivors.push_back(s);
            }
            prev_recency = cur;
        }
        if (moved != 0) rebuild_(survivors);
        return moved;
    }

private:
    using slot_t = std::pair<std::uint64_t, std::uint64_t>;

    // P-CACHELINE-LITERAL: das Chunk-Alignment ist eine Cache-Line-Groesse, kein eigener Wert -> es folgt
    // derselben Achsen-Quelle wie kLineBytes (am Default 64, also unveraendert). BEWUSST NICHT
    // cacheline::alignment_bytes(cfg): das ist die alignment-DIMENSION (None -> alignof(max_align_t)) und
    // wuerde am Default von 64 auf 16 fallen -- eine Verhaltensaenderung, die dieser Schnitt nicht macht.
    static constexpr std::size_t kChunkAlign = kLineBytes;
    struct Chunk {
        unsigned char* data     = nullptr;
        std::size_t    count    = 0; // belegte Records (<= cap_)
        std::size_t    capacity = 0; // ueber A allozierte Bytes (== chunk_bytes())
    };

    // A8-S5-02a HERZ-SCHNITT: der Chunk-INDEX ueber DERSELBEN Kompositions-Achse A, ueber die die
    // Record-Bytes schon laufen (alloc_.allocate/:deallocate in append_slot/free_chunks_/copy_from_).
    // Der Store IST ein Template ueber A -- hier ist die ECHTE Kompositions-Bindung moeglich, ein
    // benannter Achsen-DEFAULT (wie in den Nicht-Template-Familien 03/01d) waere hier eine zweite,
    // stille Speicherquelle neben der Komposition und damit ein neuer Fremdgang.
    using chunk_alloc = typename A::template StdAllocatorAdapter<Chunk>;
    using chunk_vec_t = std::vector<Chunk, chunk_alloc>;

public:
    /// A8-S5-02a HERZ-BELEG auf TYP-EBENE (oeffentlich fuer die Familien-Wache): der Allokator-Typ, an dem
    /// der Chunk-INDEX haengt. Er ist per Konstruktion ein StdAllocatorAdapter DERSELBEN Achse A wie die
    /// Record-Bytes -- ein Default-Allokator oder der Adapter einer FREMDEN Strategie waere in den Adapter
    /// von A nicht konvertierbar. Genau das pinnt test_s5_02a_layout_alloc_conformance.
    using chunk_index_allocator_type = chunk_alloc;

private:
    // ── Representation-aware store/load (compile-time-dispatch, zero-cost) ─────────────────────────────────
    // Adress-Helfer pro Repraesentation. `base` = Chunk-Start, `j` = Slot-Index im Chunk (0..cap_-1).

    [[nodiscard]] static unsigned char* key_ptr_(unsigned char* base, std::size_t j) noexcept {
        if constexpr (kRep == RK::aos_interleaved_packed)
            return base + j * kKvBytes; // [k|v] @ 16
        else if constexpr (kRep == RK::aos_interleaved_padded)
            return base + j * record_phys_bytes(); // [k|v|pad] @ Cache-Line-Stride
        else if constexpr (kRep == RK::soa_split_columns)
            return base + j * kKeyBytes; // keys[] Spalte
        else if constexpr (kRep == RK::aosoa_blocked_columns) {
            std::size_t const b = j / kBlockW, w = j % kBlockW;
            return base + b * (kBlockW * kKvBytes) + w * kKeyBytes; // Block-Key-Lane
        } else {                                                    /* succinct_hot_cold_split */
            return base + j * kHotBytes;                            // 2-B-Hot-Spalte
        }
    }
    [[nodiscard]] static unsigned char const* key_ptr_(unsigned char const* base, std::size_t j) noexcept {
        return key_ptr_(const_cast<unsigned char*>(base), j);
    }
    [[nodiscard]] static unsigned char* value_ptr_(unsigned char* base, std::size_t j) noexcept {
        if constexpr (kRep == RK::aos_interleaved_packed)
            return base + j * kKvBytes + kKeyBytes;
        else if constexpr (kRep == RK::aos_interleaved_padded)
            return base + j * record_phys_bytes() + kKeyBytes;
        else if constexpr (kRep == RK::soa_split_columns)
            return base + cap_ * kKeyBytes + j * kValBytes; // values[] nach keys[]
        else if constexpr (kRep == RK::aosoa_blocked_columns) {
            std::size_t const b = j / kBlockW, w = j % kBlockW;
            return base + b * (kBlockW * kKvBytes) + kBlockW * kKeyBytes + w * kValBytes; // Block-Value-Lane
        } else { /* succinct: [hot2[cap]][cold6[cap]][values8[cap]] — value-Region nach hot+cold */
            return base + cap_ * kHotBytes + cap_ * kColdBytes + j * kValBytes;
        }
    }
    [[nodiscard]] static unsigned char const* value_ptr_(unsigned char const* base, std::size_t j) noexcept {
        return value_ptr_(const_cast<unsigned char*>(base), j);
    }
    /// succinct-only: Cold-Residue (high48) Adresse.
    [[nodiscard]] static unsigned char* cold_ptr_(unsigned char* base, std::size_t j) noexcept {
        return base + cap_ * kHotBytes + j * kColdBytes;
    }
    [[nodiscard]] static unsigned char const* cold_ptr_(unsigned char const* base, std::size_t j) noexcept {
        return cold_ptr_(const_cast<unsigned char*>(base), j);
    }

    static void store_record_(unsigned char* base, std::size_t j, key_type k, value_type v) noexcept {
        if constexpr (kRep == RK::succinct_hot_cold_split) {
            std::uint16_t const hot  = static_cast<std::uint16_t>(k & 0xFFFFu);
            std::uint64_t const cold = k >> 16; // high48 (passt in 6 B)
            std::memcpy(key_ptr_(base, j), &hot, kHotBytes);
            std::memcpy(cold_ptr_(base, j), &cold, kColdBytes); // nur die unteren 6 B von cold
            std::memcpy(value_ptr_(base, j), &v, kValBytes);
        } else {
            std::memcpy(key_ptr_(base, j), &k, kKeyBytes);
            std::memcpy(value_ptr_(base, j), &v, kValBytes);
        }
    }
    static void store_value_(unsigned char* base, std::size_t j, value_type v) noexcept {
        std::memcpy(value_ptr_(base, j), &v, kValBytes);
    }
    static key_type load_key_(unsigned char const* base, std::size_t j) noexcept {
        if constexpr (kRep == RK::succinct_hot_cold_split) {
            std::uint16_t hot = 0;
            std::memcpy(&hot, key_ptr_(base, j), kHotBytes);
            std::uint64_t cold = 0;
            std::memcpy(&cold, cold_ptr_(base, j), kColdBytes); // liest 6 B in low48
            return static_cast<key_type>(hot) | (cold << 16);   // VERLUSTFREIE Rekonstruktion
        } else {
            key_type k;
            std::memcpy(&k, key_ptr_(base, j), kKeyBytes);
            return k;
        }
    }
    static value_type load_value_(unsigned char const* base, std::size_t j) noexcept {
        value_type v;
        std::memcpy(&v, value_ptr_(base, j), kValBytes);
        return v;
    }

    template <class F>
    void for_each_slot_(F&& f) const {
        for (auto const& c : chunks_)
            for (std::size_t j = 0; j < c.count; ++j) f(load_key_(c.data, j), load_value_(c.data, j));
    }

    // A8-S5-02a SCOPE-DEKLARATION (begruendet stehen gelassen, nicht vergessen): flatten_/rebuild_ und die
    // organ_observe_*-Streu-Puffer (kb/ks/survivors) halten TRANSIENTE Umbau-/Beobachtungs-Puffer, keinen
    // Organ-Zustand. Sie bleiben bewusst am Default-Allokator -- exakt wie die gleichartigen Puffer des
    // bereits konformen ComposedStore (axis_04_node_type_composed_store.hpp:113). Sie ueber alloc_ zu
    // fuehren waere hier KEIN Fremdgang-Fix, sondern eine MESSWERT-Aenderung mit Doppelzaehlungs-Charakter:
    // allocator_statistics() ist die T6-Route dieses Stores, und die Beobachtungs-Puffer wuerden je
    // observe-Aufruf mitzaehlen. Die Einsammel-Naht + Doppelzaehlungs-Regel gehoert in das Mess-Schnitt-
    // Fenster (Owner-KERN 04.08. abend-11, T6 = Option B strikt), nicht in diese Scheibe.
    [[nodiscard]] std::vector<slot_t> flatten_() const {
        std::vector<slot_t> fl;
        fl.reserve(size_);
        for_each_slot_([&](key_type k, value_type v) { fl.emplace_back(k, v); });
        return fl;
    }
    void rebuild_(std::vector<slot_t> const& flat) {
        clear();
        for (auto const& s : flat) append_slot(s.first, s.second);
    }

    // ── REALER Key-only-Scan-Footprint (P-MD1-ERDUNG, CLU-Quelle) ─────────────────────────────────────────
    // Liest ALLE Keys EINES Chunks aus der ECHTEN Repraesentation (Checksumme = Korrektheits-Anker) und zaehlt
    // dabei (a) die NUTZbaren Key-Bytes (`key_bytes`) und (b) die DISTINKTEN Cache-Linien (`lines`) in der
    // Groesse der cacheline-Unterachse (kLineBytes, am Default 64), die
    // die realen Key-Adressen beruehren. Die Lines werden aus den realen Byte-Offsets des Chunk-Backings
    // bestimmt (Offset_div_kLineBytes-Markierung) -- kein Modell, sondern der echte Speicher-Footprint
    // des Zugriffs.
    void key_scan_footprint_(Chunk const& c, std::uint64_t& checksum, std::uint64_t& key_bytes,
                             std::uint64_t& lines) const noexcept {
        checksum  = 0;
        key_bytes = 0;
        // Bitset der beruehrten Cache-Linien im Chunk (chunk_bytes()/kLineBytes + 2 Linien-Indizes).
        constexpr std::size_t kMaxLines          = (chunk_bytes() / kLineBytes) + 2u;
        bool                  touched[kMaxLines] = {};
        auto                  mark               = [&](std::size_t off, std::size_t w) noexcept {
            for (std::size_t b = off; b < off + w; ++b) {
                std::size_t const li = b / kLineBytes;
                if (li < kMaxLines) touched[li] = true;
            }
        };
        for (std::size_t j = 0; j < c.count; ++j) {
            checksum += load_key_(c.data, j);
            if constexpr (kRep == RK::succinct_hot_cold_split) {
                // succinct: der Key-Scan beruehrt die 2-B-HOT-Spalte (diskriminierende, NUTZbare Bytes) UND die
                // 6-B-COLD-Residue (fuer die verlustfreie Rekonstruktion mit-beruehrt, aber NICHT als Nutz-Key
                // zaehlend) → useful = 2 B/Key, touched lines = Hot- + Cold-Spalten-Linien → CLU echt NIEDRIG.
                mark(static_cast<std::size_t>(key_ptr_(c.data, j) - c.data), kHotBytes);
                mark(static_cast<std::size_t>(cold_ptr_(c.data, j) - c.data), kColdBytes);
                key_bytes += kHotBytes;
            } else {
                // dichte/strided Reps: die vollen 8 Key-Bytes sind NUTZbar; touched lines = ihre realen Adressen.
                mark(static_cast<std::size_t>(key_ptr_(c.data, j) - c.data), kKeyBytes);
                key_bytes += kKeyBytes;
            }
        }
        std::uint64_t cnt = 0;
        for (std::size_t li = 0; li < kMaxLines; ++li)
            if (touched[li]) ++cnt;
        lines = cnt;
    }

    void free_chunks_() noexcept {
        for (auto& c : chunks_)
            if (c.data) {
                alloc_.deallocate(c.data, c.capacity, kChunkAlign);
                c.data = nullptr;
            }
    }
    /// A8-S5-02a: gibt den PUFFER des Chunk-INDEX frei -- noch ueber die aktuelle Strategie-Instanz.
    /// (clear() leert nur die Elemente und laesst den Puffer stehen.) Beide Adapter zeigen auf DASSELBE
    /// alloc_, sind also gleich -> der swap ist wohldefiniert (POCS==false verlangt genau das). Weder das
    /// Anlegen des leeren Vektors noch der swap alloziert; die Freigabe passiert im Block-Ende.
    void release_index_() noexcept {
        chunk_vec_t empty(alloc_.template as_std_allocator<Chunk>());
        chunks_.swap(empty);
    }
    void copy_from_(LayoutAwareChunkedStore const& o) {
        chunks_.reserve(o.chunks_.size());
        for (auto const& oc : o.chunks_) {
            Chunk c;
            c.capacity = oc.capacity;
            c.data     = static_cast<unsigned char*>(alloc_.allocate(c.capacity, kChunkAlign));
            std::memcpy(c.data, oc.data, oc.capacity); // byte-genaue Kopie → deckt ALLE Reps ab (Memento)
            c.count = oc.count;
            chunks_.push_back(c);
        }
        size_                           = o.size_;
        chunk_allocs_                   = o.chunk_allocs_;
        runtime_pool_budget_bytes_      = o.runtime_pool_budget_bytes_;
        runtime_pool_budget_rejections_ = o.runtime_pool_budget_rejections_;
    }

    [[nodiscard]] std::uint64_t bytes_in_use_() const noexcept {
        std::uint64_t bytes = 0;
        for (auto const& c : chunks_) bytes += static_cast<std::uint64_t>(c.capacity);
        return bytes;
    }

    // A8-S5-02a: alloc_ VOR chunks_ (Lebensdauer des Zeigers im StdAllocatorAdapter) -- war schon so,
    // ist seit dem HERZ-Schnitt aber tragend und nicht mehr nur Kosmetik.
    mutable A     alloc_{};
    chunk_vec_t   chunks_;
    std::size_t   size_                           = 0;
    std::size_t   chunk_allocs_                   = 0;
    std::uint64_t runtime_pool_budget_bytes_      = 0;
    std::uint64_t runtime_pool_budget_rejections_ = 0;

    // A8-S5-02a TYP-BEWEIS im Header (kein Byte, keine Laufzeit): der Index-Vektor traegt den Adapter DER
    // Kompositions-Achse A. Faellt er auf einen Default-Allokator zurueck, bricht das compile-hart.
    static_assert(std::is_same_v<typename chunk_vec_t::allocator_type, chunk_alloc>);
};

// Compile-Time-Selbstbeweis: layout-honorierendes Organ erfuellt StorageOrgan UND ist von beiden Traversal-Organen nutzbar.
namespace _la_cmp = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;
using PilotLayoutAwareStore =
    LayoutAwareChunkedStore<Node4NodeType, _ml_la::CacheLineAlignedMemoryLayout, _al_la::MimallocAllocator>;
static_assert(_la_cmp::StorageOrgan<PilotLayoutAwareStore>);
static_assert(_la_cmp::TraversalOrgan<_la_cmp::LinearScanTraversal, PilotLayoutAwareStore>);
static_assert(_la_cmp::TraversalOrgan<_la_cmp::SortedBinaryTraversal, PilotLayoutAwareStore>);
static_assert(PilotLayoutAwareStore::record_phys_bytes() == 64); // CLA → 64-B-Stride (Padding) verifiziert
// C2/FF2-Neutralitaets-Selbstbeweis: alle bestehenden Node-Blaetter sind Native (Breite 0) → Kapazitaet und
// damit Chunk-Layout byte-identisch zum Ist-Stand (der Wide-Pfad wird in test_ff2_node_width_subaxis bewiesen).
static_assert(PilotLayoutAwareStore::node_width_in_lines() == 0);
static_assert(PilotLayoutAwareStore::node_capacity() == Node4NodeType::max_capacity());

// P-CACHELINE-LITERAL-Verhaltensneutralitaets-Beweis (2026-08-04): der Bezug aus der cacheline-Unterachse
// liefert am Achsen-Default EXAKT die frueher hartkodierte 64 -- an der Quelle, an ihrem Konsum im Store
// und an jeder davon abgeleiteten Geometrie. Bricht einer dieser Saetze, ist die Substitution NICHT mehr
// neutral und der Bruch faellt compile-hart auf, nicht erst in den Messwerten.
static_assert(::comdare::cache_engine::cacheline::kDefaultLineBytes == 64);
static_assert(PilotLayoutAwareStore::cacheline_line_bytes() == 64);
static_assert(PilotLayoutAwareStore::chunk_align() == 64);
static_assert(PilotLayoutAwareStore::record_phys_bytes() == 64);                            // round_up(16, 64)
static_assert(PilotLayoutAwareStore::chunk_bytes() == Node4NodeType::max_capacity() * 64u); // 4 * 64
static_assert(PilotLayoutAwareStore::cacheline_line_bytes() ==
              _ml_la::CacheLineAlignedMemoryLayout::cacheline_subaxis_line_bytes()); // Achse == Store

} // namespace comdare::cache_engine::node
