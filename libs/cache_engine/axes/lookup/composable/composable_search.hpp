#pragma once
// V41 Saeule-1 (Doku 24 §5.4/§5.5, Doku 14 §3/§11.3) — komponierbares Traversal-Organ-Modell.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// **Zweck (Korrektur Doku 24):** Die bisherigen axis_03a-Wrapper (Array256/BST/B-Baum/…) sind
// monolithische *self-contained* Strukturen ("Tiere") mit eigenem Speicher + schmalem Key (uint8/
// uint16). Das widerspricht der Organ-Metapher (Doku 14 §1–§3: Achse = Organ, Algorithmus =
// Komposition) UND blockiert die Mess-Vereinheitlichung (Doku 24 §5.5: schmale Organ-Keys).
//
// Dieses Modul fuehrt das KORREKTE Modell ein (Doku 14 §11.3 Schicht 3): ein Suchalgorithmus =
//   TRAVERSAL-Organ  ⊕  STORAGE-Organ
// ueber der aktuellen Default-Key-Breite (uint64). Das Traversal-Organ besitzt KEINEN Speicher — es
// operiert via statische insert_into/lookup_in/erase_from auf dem vom Storage-Organ gelieferten
// Substrat. Traversal-Organe sind damit gegeneinander austauschbar ("genetisches Experiment",
// Doku 14 §1.2) und der Key-Type-Mismatch (Doku 24 §5.5) entfaellt strukturell.
//
// Pilot-Increment: 1 Storage-Organ (RawSlotStore) + 2 Traversal-Organe (LinearScan = ART-Node4-Organ,
// SortedBinary = Binärsuch-Organ) + ComposedSearch<Traversal,Store> mit std::map-Interface.
// Folge-Increments: node_type/layout/allocator als echte Storage-Organe; Migration der Tier-Wrapper
// zu Reference-Compositions (Doku 14 §6); Anatomie-Anbindung.

#include <algorithm> // #214 GoF-Iterator: std::partial_sort fuer den geordneten LinearScan-Range-Scan
#include <cstddef>
#include <cstdint>
#include <concepts>
#include <optional>
#include <utility>
#include <vector>

#include "storage_organ_concept.hpp" // Saeule-1: Storage-Organ-Vertrag (aus RawSlotStore extrahiert)

namespace comdare::cache_engine::lookup::composable {

/// STORAGE-Organ (Pilot): rohe, indizierte Slots (key,value) ueber aktueller Default-Breite uint64. Vertritt
/// das node_type/layout/allocator-getriebene Speicher-Substrat; Traversal-Organe operieren darauf.
class RawSlotStore {
public:
    using key_type   = std::uint64_t; // aktuelle Default-Breite; native schmalere Container = #217-2b
    using value_type = std::uint64_t;

    [[nodiscard]] std::size_t slot_count() const noexcept { return slots_.size(); }
    [[nodiscard]] key_type    key_at(std::size_t i) const noexcept { return slots_[i].first; }
    [[nodiscard]] value_type  value_at(std::size_t i) const noexcept { return slots_[i].second; }
    void                      set_value_at(std::size_t i, value_type v) noexcept { slots_[i].second = v; }
    void                      append_slot(key_type k, value_type v) { slots_.emplace_back(k, v); }
    void                      insert_slot_at(std::size_t i, key_type k, value_type v) {
        slots_.emplace(slots_.begin() + static_cast<std::ptrdiff_t>(i), k, v);
    }
    void erase_slot_at(std::size_t i) { slots_.erase(slots_.begin() + static_cast<std::ptrdiff_t>(i)); }
    void clear() noexcept { slots_.clear(); }

private:
    std::vector<std::pair<key_type, value_type>> slots_;
};

/// TRAVERSAL-Organ-Concept: statische insert_into/lookup_in/erase_from auf einem Storage-Organ.
/// KEIN eigener Speicher (Organ, nicht Tier).
template <class T, class Store>
concept TraversalOrgan = requires(Store& s, Store const& cs, typename Store::key_type k, typename Store::value_type v) {
    { T::template insert_into<Store>(s, k, v) } -> std::same_as<void>;
    { T::template lookup_in<Store>(cs, k) } -> std::same_as<std::optional<typename Store::value_type>>;
    { T::template erase_from<Store>(s, k) } -> std::same_as<bool>;
};

/// Additives Capability-Concept (analog ObservableAxis zum StorageOrgan-Kernvertrag, observable_composed_search.hpp:7):
/// ein Traversal-Organ, das einen GEORDNETEN Range-Scan ab start_key beherrscht (GoF-Iterator, YCSB-E #214). Bewusst
/// NICHT im TraversalOrgan-Kernvertrag (insert/lookup/erase) — additiv getrennt geprueft. `sink` ist aufrufbar mit
/// (key,value); Rueckgabe = Anzahl in Key-Reihenfolge besuchter Records (<= max_count). Im Concept mit einem
/// Funktionspointer-Sink geprueft (ein gueltiger Sink-Typ); die Methode selbst bleibt Template-parametrisiert (zero-cost).
template <class T, class Store>
concept ScannableTraversalOrgan =
    TraversalOrgan<T, Store> && requires(Store const& cs, typename Store::key_type k, std::size_t n,
                                         void (*sink)(typename Store::key_type, typename Store::value_type)) {
        { T::template scan_into<Store>(cs, k, n, sink) } -> std::same_as<std::size_t>;
    };

/// Traversal-Organ 1: unsortierter linearer Scan (ART-Node4-Strategie). Store bleibt unsortiert.
struct LinearScanTraversal {
    template <class Store>
    static void insert_into(Store& s, typename Store::key_type k, typename Store::value_type v) {
        for (std::size_t i = 0; i < s.slot_count(); ++i)
            if (s.key_at(i) == k) {
                s.set_value_at(i, v);
                return;
            } // Update
        s.append_slot(k, v);
    }
    template <class Store>
    static std::optional<typename Store::value_type> lookup_in(Store const& s, typename Store::key_type k) {
        for (std::size_t i = 0; i < s.slot_count(); ++i)
            if (s.key_at(i) == k) return s.value_at(i);
        return std::nullopt;
    }
    template <class Store>
    static bool erase_from(Store& s, typename Store::key_type k) {
        for (std::size_t i = 0; i < s.slot_count(); ++i)
            if (s.key_at(i) == k) {
                s.erase_slot_at(i);
                return true;
            }
        return false;
    }
    /// GoF-Iterator (YCSB-E #214): geordneter Range-Scan ab start_key. Der Store ist UNSORTIERT, daher MUSS ein
    /// geordneter Scan alle Slots ansehen (O(n)) — die wahre Kosten fehlender Ordnung, KEIN Apparat-Artefakt
    /// (Meta-Lehre #3: LinearScan durchlaeuft einen anderen Organ-Pfad als die sortierten Organe). Sammelt die
    /// qualifizierenden (key>=start_key), ordnet die kleinsten max_count nach Key (partial_sort) und gibt sie der
    /// Senke (aufrufbar mit (key,value)) IN KEY-REIHENFOLGE. Rueckgabe = Anzahl besuchter Records. KEIN save_state.
    template <class Store, class Sink>
    static std::size_t scan_into(Store const& s, typename Store::key_type start_key, std::size_t max_count,
                                 Sink&& sink) {
        if (max_count == 0) return 0;
        std::vector<std::pair<typename Store::key_type, typename Store::value_type>> hits;
        std::size_t const                                                            n = s.slot_count();
        for (std::size_t i = 0; i < n; ++i) {
            typename Store::key_type const k = s.key_at(i);
            if (k >= start_key) hits.emplace_back(k, s.value_at(i));
        }
        std::size_t const out = (hits.size() < max_count) ? hits.size() : max_count;
        std::partial_sort(hits.begin(), hits.begin() + static_cast<std::ptrdiff_t>(out), hits.end(),
                          [](auto const& a, auto const& b) noexcept { return a.first < b.first; });
        for (std::size_t i = 0; i < out; ++i) sink(hits[i].first, hits[i].second);
        return out;
    }
};

/// Traversal-Organ 2: Binärsuche, haelt den Store SORTIERT (sorted-vector lower_bound).
struct SortedBinaryTraversal {
    template <class Store>
    static std::size_t lower_bound_index(Store const& s, typename Store::key_type k) {
        std::size_t lo = 0, hi = s.slot_count();
        while (lo < hi) {
            std::size_t const m = lo + (hi - lo) / 2;
            if (s.key_at(m) < k)
                lo = m + 1;
            else
                hi = m;
        }
        return lo;
    }
    template <class Store>
    static void insert_into(Store& s, typename Store::key_type k, typename Store::value_type v) {
        std::size_t const i = lower_bound_index(s, k);
        if (i < s.slot_count() && s.key_at(i) == k) {
            s.set_value_at(i, v);
            return;
        } // Update
        s.insert_slot_at(i, k, v);
    }
    template <class Store>
    static std::optional<typename Store::value_type> lookup_in(Store const& s, typename Store::key_type k) {
        std::size_t const i = lower_bound_index(s, k);
        if (i < s.slot_count() && s.key_at(i) == k) return s.value_at(i);
        return std::nullopt;
    }
    template <class Store>
    static bool erase_from(Store& s, typename Store::key_type k) {
        std::size_t const i = lower_bound_index(s, k);
        if (i < s.slot_count() && s.key_at(i) == k) {
            s.erase_slot_at(i);
            return true;
        }
        return false;
    }
    /// GoF-Iterator (YCSB-E #214): geordneter Range-Scan ab start_key. Der Store ist SORTIERT → lower_bound (O(log n))
    /// auf den ersten Key >= start_key, dann sequenzieller Walk (O(scan_len)). KEIN save_state()-O(n)-Kopie + std::sort
    /// je Op (Audit K4). Gibt bis max_count Records IN KEY-REIHENFOLGE an die Senke (aufrufbar mit (key,value)).
    /// Rueckgabe = Anzahl besuchter Records.
    template <class Store, class Sink>
    static std::size_t scan_into(Store const& s, typename Store::key_type start_key, std::size_t max_count,
                                 Sink&& sink) {
        std::size_t const n       = s.slot_count();
        std::size_t       visited = 0;
        for (std::size_t i = lower_bound_index(s, start_key); i < n && visited < max_count; ++i, ++visited)
            sink(s.key_at(i), s.value_at(i));
        return visited;
    }
};

/// KOMPOSITION: ein Such-Algorithmus = Traversal-Organ ⊕ Storage-Organ, mit std::map-Interface.
/// Ueber GEMEINSAMEM Store-Key (uint64). Genetisches Experiment: Traversal frei austauschbar bei
/// gleichbleibendem Store (Doku 14 §1.2). Dies ist der Bauplan, der die monolithischen Tier-Wrapper
/// ersetzt (Folge-Increments: node_type/layout/allocator als Store-Organe).
template <class Traversal, class Store>
    requires TraversalOrgan<Traversal, Store>
class ComposedSearch {
public:
    using key_type   = typename Store::key_type;
    using value_type = typename Store::value_type;

    void insert(key_type k, value_type v) { Traversal::template insert_into<Store>(store_, k, v); }
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        // #188-4a-C: K (k-ary-Arity) ist eine COMPILE-TIME-Permutation (StaticAxisNode) — sie steckt im Organ-Typ
        // selbst (KAryTraversal<K>), NICHT in einem Laufzeit-Aspekt. Daher reiner 2-arg-lookup_in (kein runtime-Kanal).
        return Traversal::template lookup_in<Store>(store_, k);
    }
    bool erase(key_type k) { return Traversal::template erase_from<Store>(store_, k); }

    /// GoF-Iterator (YCSB-E #214): geordneter Range-Scan ab start_key. Delegiert an das Traversal-Organ — O(log n +
    /// scan_len) fuer sortierte Organe (SortedBinary/Interpolation/Galloping), ehrlicher O(n) fuer LinearScan. const +
    /// KEIN Substrat-/Statistik-Effekt (reines Lesen). Ersetzt das save_state()-O(n)+std::sort je Scan-Op (Audit K4).
    /// `sink` ist aufrufbar mit (key,value); Rueckgabe = Anzahl in Key-Reihenfolge besuchter Records (<= max_count).
    template <class Sink>
    [[nodiscard]] std::size_t scan_range(key_type start_key, std::size_t max_count, Sink&& sink) const {
        return Traversal::template scan_into<Store>(store_, start_key, max_count, std::forward<Sink>(sink));
    }
    [[nodiscard]] std::size_t occupied_count() const noexcept { return store_.slot_count(); }
    void                      clear() noexcept { store_.clear(); }
    // Saeule-2: read-only Zugriff auf das Storage-Organ (z.B. fuer den Allocator-Statistik-Durchgriff).
    [[nodiscard]] Store const& store() const noexcept { return store_; }
    // P4 (#123): MUTABLER Store-Zugriff fuer den ECHTEN 2-Ebenen-Migrations-Schritt (organ_migrate_step bewegt
    // Records aus diesem Store in den 2.-Ebenen-Store). Additiv, der const-Zugriff oben bleibt unveraendert.
    [[nodiscard]] Store& store_mut() noexcept { return store_; }

    // ── V5-I6-SUBSTANZ (#44) — MementoAxis: per-Achsen-Zustands-Kapselung (statt Adapter-Pauschalkopie) ──
    // Das /goal verlangt „einheitliche Memento-Hilfsfunktionen JE STATEFUL ACHSEN-INTERFACE" (kein einfacher
    // Snapshot). Dieses Such-Organ implementiert damit das MementoAxis-Concept (memento_aggregate.hpp:44-49):
    //   memento_t       = vollstaendige (key,value)-Liste des Store-Substrats (der gesamte logische Zustand)
    //   save_state()     = kapselt den Zustand (Warmup-Vor-Zustand)
    //   restore_state(m) = rekonstruiert via Traversal::insert_into → erhaelt die Traversal-Invariante
    //                      (z.B. SortedBinary haelt sortiert), nicht nur einen rohen Speicher-Klon.
    using memento_t = std::vector<std::pair<key_type, value_type>>;

    [[nodiscard]] memento_t save_state() const {
        memento_t m;
        m.reserve(store_.slot_count());
        for (std::size_t i = 0; i < store_.slot_count(); ++i) m.emplace_back(store_.key_at(i), store_.value_at(i));
        return m;
    }
    void restore_state(memento_t const& m) {
        store_.clear();
        for (auto const& kv : m) Traversal::template insert_into<Store>(store_, kv.first, kv.second);
    }

private:
    Store store_;
};

// Selbstbeweis: die Pilot-Klasse RawSlotStore erfuellt das neu extrahierte StorageOrgan-Concept exakt
// (Vertrag == Ist-Implementierung, keine erfundene Abstraktion). Bricht dieser static_assert, ist das
// Concept falsch (z.B. faelschlich gefordertes noexcept), NICHT die Implementierung.
static_assert(StorageOrgan<RawSlotStore>);

// #214: Selbstbeweis, dass die beiden Kern-Traversal-Organe den additiven Scan-Vertrag erfuellen (GoF-Iterator).
// Interpolation/Galloping delegieren ihr scan_into an SortedBinary → ihr Selbstbeweis steht in ihren Organ-Dateien.
static_assert(ScannableTraversalOrgan<LinearScanTraversal, RawSlotStore>);
static_assert(ScannableTraversalOrgan<SortedBinaryTraversal, RawSlotStore>);

} // namespace comdare::cache_engine::lookup::composable
