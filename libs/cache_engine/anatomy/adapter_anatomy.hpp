#pragma once
// Adapter-Tier-Unterklasse der CONTAINER-Gattung (2026-06-03, #87+#90, AUTORITATIV Doku 14 §28 Invertebrate-Spalte
// + §26.4 + Doc 30 §8.0/§8.1). Ebene 2 unter dem Container-Außen-Interface (Ebene 1, AnatomyGattung::Container),
// gleichrangig zu Set/Sequence/View. Gebaut EXAKT analog SequenceComposition (10 geteilt + growth):
//
//   AdapterComposition<T0..T11, Inner>  =  12 geteilte/delegierte Achsen (§28)  +  inner_container (NEU axis_inner).
//
// §28-Invertebrate-Achsen (13): delegiert (9) search_algo, cache_traversal, memory_layout, allocator, prefetch,
// concurrency, isa, io_dispatch, migration_policy + aktiv (3) serialization, telemetry, value_handle + spezifisch (1)
// inner_container. §26.4: stack/queue→deque, priority_queue→vector+Compare; Pflicht-API push/pop/top/front/back
// (KEIN begin/end). Die Disziplin (FIFO/LIFO/Priority) liegt in der API-NUTZUNG (front vs back), NICHT in einer Achse —
// §28 kennt KEINE „ordering"-Achse (frühere inner+ordering-Version war ein geratener Ebenen-/Achsen-Fehler, verworfen).
//
// NAMEN (#90-Sweep abgeschlossen): Datei adapter_anatomy.hpp + Typen AdapterComposition/AdapterAnatomy
// (konsistent mit set_/sequence_/view_; historisch container_anatomy.hpp / Container*). C++23, header-only.

#include "anatomy_base.hpp"   // AnatomyGenus (Tier-Unterklasse) / AnatomyGattung

#include <algorithm>     // std::push_heap / std::pop_heap (HeapInner = priority_queue-Disziplin)
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>    // std::less (HeapInner Default-Compare)
#include <optional>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::anatomy {

// ════════════════════════════════════════════════════════════════════════════════════════════════════════
// axis_inner (die EINE Adapter-spezifische Achse, §28) — das dekorierte Inner-Substrat. Pflicht-Ops für die
// §26.4-Adapter-API: push_back (ablegen), front/back (beide Enden lesen), pop_front/pop_back, size, clear.
// FIFO (queue) nutzt front+pop_front; LIFO (stack) nutzt back+pop_back — die Disziplin ist die API-Nutzung.
// ════════════════════════════════════════════════════════════════════════════════════════════════════════

/// DequeInner — Inner-Substrat über std::deque (Default für stack/queue, §26.4). Beide Enden O(1).
template <class T = std::uint64_t>
struct DequeInner {
    using element_type = T;
    static constexpr std::string_view name = "DequeInner";
    void                       push_back(element_type v)      { d_.push_back(v); }
    [[nodiscard]] std::size_t  size()           const noexcept { return d_.size(); }
    [[nodiscard]] element_type front()          const          { return d_.front(); }
    [[nodiscard]] element_type back()           const          { return d_.back(); }
    void                       pop_front()                     { d_.pop_front(); }
    void                       pop_back()                      { d_.pop_back(); }
    void                       clear()                noexcept  { d_.clear(); }
private:
    std::deque<element_type> d_{};
};

/// VectorInner — Inner-Substrat über std::vector (Default für priority_queue-Substrat, §26.4). pop_back O(1).
template <class T = std::uint64_t>
struct VectorInner {
    using element_type = T;
    static constexpr std::string_view name = "VectorInner";
    void                       push_back(element_type v)      { v_.push_back(v); }
    [[nodiscard]] std::size_t  size()           const noexcept { return v_.size(); }
    [[nodiscard]] element_type front()          const          { return v_.front(); }
    [[nodiscard]] element_type back()           const          { return v_.back(); }
    void                       pop_front()                     { v_.erase(v_.begin()); }
    void                       pop_back()                      { v_.pop_back(); }
    void                       clear()                noexcept  { v_.clear(); }
private:
    std::vector<element_type> v_{};
};

/// HeapInner — Inner-Substrat mit echter PRIORITY-Disziplin (std::priority_queue, §26.4: vector + Compare). Hält
/// einen Max-Heap über std::vector via std::push_heap/std::pop_heap (Stand der Technik; Default-Compare std::less →
/// Max-Heap, front()==Maximum). Die Priority-Disziplin lebt damit INNERHALB der inner_container-Achse (§28) — KEINE
/// neue Achse. Priority-Nutzung: push + front()(=Max) + pop_front()(=Extract-Max). back()/pop_back() = Roh-Blatt
/// (Heap bleibt gültig, da pop_back ein Blatt entfernt) — nicht priority-relevant.
template <class T = std::uint64_t, class Compare = std::less<T>>
struct HeapInner {
    using element_type = T;
    static constexpr std::string_view name = "HeapInner";
    void                       push_back(element_type v)      { v_.push_back(v); std::push_heap(v_.begin(), v_.end(), comp_); }
    [[nodiscard]] std::size_t  size()           const noexcept { return v_.size(); }
    [[nodiscard]] element_type front()          const          { return v_.front(); }                 // Heap-Top = Maximum
    [[nodiscard]] element_type back()           const          { return v_.back(); }                 // Blatt (nicht priority-relevant)
    void                       pop_front()                     { std::pop_heap(v_.begin(), v_.end(), comp_); v_.pop_back(); }  // Extract-Max
    void                       pop_back()                      { v_.pop_back(); }                    // entfernt ein Blatt (Heap bleibt gültig)
    void                       clear()                noexcept  { v_.clear(); }
private:
    std::vector<element_type> v_{};
    Compare                   comp_{};
};

// ════════════════════════════════════════════════════════════════════════════════════════════════════════
// Adapter-Observer (gattungs-eigen) — flacher uint64-POD, getrennt vom SearchAlgorithm-ObserverAggregate<19>.
// Felder spiegeln den Antrieb des inner_container (die real getriebene spezifische Achse, §28).
// ════════════════════════════════════════════════════════════════════════════════════════════════════════
struct AdapterObserverSnapshot {   // Adapter-Observer (gattungs-eigen, getrennt vom SearchAlgorithm-ObserverAggregate<19>)
    std::uint64_t push_count        = 0;   // push → inner_container
    std::uint64_t pop_count         = 0;   // erfolgreiche pop_front/pop_back
    std::uint64_t front_reads       = 0;   // front()-Zugriffe (FIFO-Disziplin)
    std::uint64_t back_reads        = 0;   // back()/top()-Zugriffe (LIFO-Disziplin)
    std::uint64_t current_occupancy = 0;   // aktuelle inner_container-Größe
    std::uint64_t peak_occupancy    = 0;   // maximale inner_container-Größe
};

/// AdapterComposition — 12 geteilte/delegierte §28-Achsen + inner_container.
/// Reihenfolge T0..T11 = §28-Invertebrate (delegiert + aktiv), dann Inner (spezifisch). Analog SequenceComposition.
template <class T0, class T1, class T2, class T3, class T4, class T5,
          class T6, class T7, class T8, class T9, class T10, class T11,
          class Inner = DequeInner<>>
struct AdapterComposition {
    using search_algo       = T0;    // axis_03a (delegated an inner)
    using cache_traversal   = T1;    // axis_03b (delegated)
    using memory_layout     = T2;    // axis_05  (delegated)
    using allocator         = T3;    // axis_06  (delegated)
    using prefetch          = T4;    // axis_07  (delegated)
    using concurrency       = T5;    // axis_08  (delegated)
    using serialization     = T6;    // axis_10  (aktiv)
    using telemetry         = T7;    // axis_11  (aktiv)
    using value_handle      = T8;    // axis_14  (aktiv)
    using isa               = T9;    // axis_09  (delegated)
    using io_dispatch       = T10;   // axis_io  (delegated)
    using migration_policy  = T11;   // axis_migration (delegated)
    using inner_container   = Inner; // NEU axis_inner (Adapter-spezifisch, §28)

    static constexpr std::size_t      slot_count = 13;   // 12 geteilt/delegiert + inner_container
    static constexpr std::string_view name       = "AdapterComposition";
    static constexpr std::string_view paper_id   = "P00 Adapter (Container-Tier-Unterklasse, Doku 14 §28 Invertebrate)";
};

/// IsAdapterComposition — Concept: 12 geteilte named Achsen + inner_container (§28 Invertebrate).
template <class C>
concept IsAdapterComposition = requires {
    typename C::search_algo;       typename C::cache_traversal;  typename C::memory_layout;
    typename C::allocator;         typename C::prefetch;         typename C::concurrency;
    typename C::serialization;     typename C::telemetry;        typename C::value_handle;
    typename C::isa;               typename C::io_dispatch;      typename C::migration_policy;
    typename C::inner_container;
    { C::slot_count } -> std::convertible_to<std::size_t>;
};

inline constexpr std::size_t kAdapterCompositionSlotCount = 13;

/// AdapterAnatomy — die Container-Gattung, Adapter-Tier-Unterklasse
/// (genus()==Adapter, gattung_of→Container). Treibt die spezifische Achse inner_container REAL über die
/// §26.4-Adapter-API (push/pop/top/front/back); die 12 geteilten/delegierten Achsen werden getragen (im
/// Komposition-Typ; analog SequenceAnatomy, die growth real treibt + die 10 geteilten trägt).
template <class Composition>
class AdapterAnatomy {
public:
    using composition_t = Composition;
    using inner_t       = typename Composition::inner_container;
    using element_type  = typename inner_t::element_type;

    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id()         noexcept { return Composition::paper_id; }
    static constexpr AnatomyGenus     genus()            noexcept { return AnatomyGenus::Adapter; }       // Tier-Unterklasse
    static constexpr AnatomyGattung   gattung()          noexcept { return AnatomyGattung::Container; }   // Außen-Interface
    static constexpr std::size_t      organ_count()      noexcept { return Composition::slot_count; }     // 13

    AdapterAnatomy() = default;
    /// capacity wird für ABI-ctor-Kompatibilität akzeptiert, aber ignoriert (unbeschränkter Adapter).
    explicit AdapterAnatomy(std::size_t /*capacity*/) noexcept {}

    // ── §26.4 Adapter-API (push/pop/top/front/back) — treibt das inner_container-Organ + Observer ──
    void put(element_type v) { push(v); }   // Alias (Bestands-Aufrufe); push = die §26.4-Operation
    void push(element_type v) {
        inner_.push_back(v);
        ++obs_.push_count;
        obs_.current_occupancy = static_cast<std::uint64_t>(inner_.size());
        if (obs_.current_occupancy > obs_.peak_occupancy) obs_.peak_occupancy = obs_.current_occupancy;
    }
    /// FIFO-Entnahme (queue): vorderstes Element.
    [[nodiscard]] std::optional<element_type> pop_front() {
        if (inner_.size() == 0) return std::nullopt;
        element_type const v = inner_.front();
        ++obs_.front_reads;
        inner_.pop_front();
        ++obs_.pop_count;
        obs_.current_occupancy = static_cast<std::uint64_t>(inner_.size());
        return v;
    }
    /// LIFO-Entnahme (stack): hinterstes Element.
    [[nodiscard]] std::optional<element_type> pop_back() {
        if (inner_.size() == 0) return std::nullopt;
        element_type const v = inner_.back();
        ++obs_.back_reads;
        inner_.pop_back();
        ++obs_.pop_count;
        obs_.current_occupancy = static_cast<std::uint64_t>(inner_.size());
        return v;
    }
    /// Bestands-Alias: get() == FIFO-Entnahme (queue-Default, §26.4 Default-Inner deque).
    [[nodiscard]] std::optional<element_type> get() { return pop_front(); }
    [[nodiscard]] std::optional<element_type> front() const {
        if (inner_.size() == 0) return std::nullopt;
        return inner_.front();
    }
    [[nodiscard]] std::optional<element_type> back() const {
        if (inner_.size() == 0) return std::nullopt;
        return inner_.back();
    }
    [[nodiscard]] std::optional<element_type> top() const { return back(); }   // stack-top
    [[nodiscard]] std::size_t size() const noexcept { return inner_.size(); }
    void clear() noexcept { inner_.clear(); obs_.current_occupancy = 0; }

    /// observe_all() — EIGENER Adapter-Observer (NICHT der SearchAlgorithm-ObserverAggregate<19>).
    [[nodiscard]] AdapterObserverSnapshot observe_all() const noexcept { return obs_; }

private:
    inner_t                 inner_{};
    AdapterObserverSnapshot obs_{};
};

}  // namespace comdare::cache_engine::anatomy
