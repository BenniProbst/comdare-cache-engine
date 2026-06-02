#pragma once
// KF-9 (2026-06-02) — Experiment-B+-Baum (Experiment-Manager der Cache-Engine-Bibliothek).
//
// Ersetzt die flache mp_product/4-fach-for-Enumeration + FNV1a-compute_fingerprint. Doc architecture/26.
//
// AUSFÜHRUNGS-/MATERIALISIERUNGSMODELL (User 2026-06-02, KORRIGIERT):
//   • Der Gesamtbaum wird NIEMALS voll materialisiert. Das volle Produkt ∏ mp_size(Enabled_i) ist astronomisch
//     (≥ 1e15) → ein eager Aufbau (ein Knoten je Wert je Ebene) zieht OOM. Stattdessen: es wird IMMER nur EIN
//     Pfad Wurzel→Blatt zur Zeit materialisiert + durchiteriert (lazy mixed-radix Odometer, O(Tiefe) Speicher).
//   • `binary_count()` = ∏ der statischen Ebenen-Größen — REIN ARITHMETISCH, OHNE Knoten zu materialisieren
//     (Doc 26 §5: der Baum ZÄHLT die Kardinalität, materialisiert sie nicht).
//   • Beim Build werden nur so viele Pfade gleichzeitig materialisiert wie zulässige DLL-Build-Prozesse
//     (`StaticBinaryView::operator[](i)` dekodiert genau EINEN Pfad on-demand — der Orchestrator hält nie alle ∏).
//   • Die DYNAMISCHEN Variablen sind VIRTUELL ineinander verschachtelte for-Schleifen (nicht materialisiert),
//     die alle Rekombinationen ZUR LAUFZEIT über EINER Tier-Binary durchführen (Algorithm_Resource_Control, KF-4/7).
//   • Das BLATT ist eine Akkumulation der dynamischen Variablen als EXAKT EINE Experiment-Einstellung
//     (= eine Binary × eine vollständige dynamische Belegung).
//
// Knoten static/dynamic via ABSTRACT FACTORY (zwei Einzelklassen). Die per-node Observer-Statistics (§2) leben
// in einer SPARSE Map (key=binary_id → NodeValue) — NUR für tatsächlich GEMESSENE Binaries, nie für alle ∏.
// Für Suche immer Bäume. C++23, header-only.

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

// ── BR-3 (Doc 27 §3): flacher, komposition-UNABHÄNGIGER Per-Achsen-Observer-Snapshot ──
/// Layout-identisch zu anatomy::ComdareTierObserverSnapshotV1 (NUR uint64 → standard_layout); der Mess-Treiber
/// (node_value_measurement.hpp) flacht `observe_all()`/`tier_observe` einer REALEN Komposition hier hinein.
/// KEIN komposition-typisiertes Member → experiment_tree.hpp bleibt umbrella-unabhängig (Doc 27 §3 BR-3).
/// R5.B-Grenze (ehrlich): real getrieben werden search_algo + allocator; `observable_axis_count` macht
/// transparent, wie viele der Achsen real beobachtet sind (Rest = passive Compile-Time-Deskriptoren / Default 0).
struct NodeObserverSnapshot {
    std::uint64_t search_lookup_count    = 0, search_hit_count        = 0, search_miss_count   = 0,
                  search_insert_count    = 0, search_erase_count      = 0, search_peak_occupancy = 0;
    std::uint64_t alloc_bytes_allocated  = 0, alloc_bytes_in_use      = 0, alloc_allocation_count = 0,
                  alloc_deallocation_count = 0, alloc_failure_count   = 0;
    std::uint64_t observable_axis_count  = 0;  // ObserverAggregate::observable_count() — wie viele Achsen real
    std::uint64_t tier_fill_level        = 0;  // tier_size() zum Snapshot-Zeitpunkt
};

// ── Per-node Value (§2): Observer-Statistics + Mess-Auswertung der Ebene ──
struct NodeValue {
    std::uint64_t measured_setting_count = 0;  // gemessene Experiment-Einstellungen unter diesem Knoten
    std::uint64_t sum_total_cycles       = 0;
    std::uint64_t sum_op_count           = 0;
    bool          has_result             = false;
    // BR-3: ECHTER Per-Achsen-Observer-Snapshot (kein 4-uint64-Stub mehr), via observe_all/tier_observe.
    NodeObserverSnapshot observer{};
    bool                 observer_real = false;  // true = via realem observe_all einer Komposition gezogen
};

enum class NodeKind { Static, Dynamic };

class INodeDescription {
public:
    virtual ~INodeDescription() = default;
    [[nodiscard]] virtual NodeKind    kind() const noexcept = 0;
    [[nodiscard]] virtual std::string axis() const = 0;
    [[nodiscard]] virtual std::string value() const = 0;
    [[nodiscard]] virtual std::string serialize() const = 0;
    [[nodiscard]] virtual bool contributes_to_signature() const noexcept = 0;
    [[nodiscard]] virtual bool contributes_to_pinned_signature() const noexcept = 0;
    [[nodiscard]] virtual bool is_runtime_loop() const noexcept = 0;
    /// RÜCK-Referenz (Bidirektionalität, User 2026-06-02): die ID des AxisBlock, dem dieser Knoten gehört.
    /// Macht statische UND dynamische Knoten auf dem GESAMTBAUM-Objekt block-filterbar + -zuordbar.
    [[nodiscard]] virtual std::string block_id() const = 0;
};

/// StaticAxisNode — compile-time-Entscheidung → je Static-Pfad eine Tier-Binary.
class StaticAxisNode final : public INodeDescription {
public:
    StaticAxisNode(std::string axis, std::string value, bool pinned, std::string block_id = {})
        : axis_{std::move(axis)}, value_{std::move(value)}, pinned_{pinned}, block_id_{std::move(block_id)} {}
    NodeKind    kind() const noexcept override { return NodeKind::Static; }
    std::string axis() const override { return axis_; }
    std::string value() const override { return value_; }
    std::string serialize() const override { return axis_ + "=" + value_; }
    bool contributes_to_signature() const noexcept override { return true; }
    bool contributes_to_pinned_signature() const noexcept override { return pinned_; }
    bool is_runtime_loop() const noexcept override { return false; }
    std::string block_id() const override { return block_id_; }   // Rück-Referenz auf den compile-time AxisBlock
    [[nodiscard]] bool pinned() const noexcept { return pinned_; }
private:
    std::string axis_, value_;
    bool pinned_;
    std::string block_id_;
};

/// DynamicVariableNode — Laufzeit-Variable → VIRTUELLE for-Schleife (nicht materialisiert). Wird je
/// Schleifen-Punkt von der Factory erzeugt und in die Experiment-Einstellung akkumuliert.
class DynamicVariableNode final : public INodeDescription {
public:
    DynamicVariableNode(std::string axis, std::string var, std::string value, std::string block_id = {})
        : axis_{std::move(axis)}, var_{std::move(var)}, value_{std::move(value)}, block_id_{std::move(block_id)} {}
    NodeKind    kind() const noexcept override { return NodeKind::Dynamic; }
    std::string axis() const override { return axis_; }
    std::string value() const override { return value_; }
    std::string serialize() const override { return axis_ + "." + var_ + "=" + value_; }
    bool contributes_to_signature() const noexcept override { return false; }
    bool contributes_to_pinned_signature() const noexcept override { return false; }
    bool is_runtime_loop() const noexcept override { return true; }
    std::string block_id() const override { return block_id_; }   // Rück-Referenz auf den compile-time AxisBlock
    [[nodiscard]] std::string variable() const { return var_; }
private:
    std::string axis_, var_, value_;
    std::string block_id_;
};

class AbstractNodeFactory {
public:
    virtual ~AbstractNodeFactory() = default;
    [[nodiscard]] virtual std::unique_ptr<INodeDescription>
        make_static(std::string axis, std::string value, bool pinned, std::string block_id = {}) const = 0;
    [[nodiscard]] virtual std::unique_ptr<INodeDescription>
        make_dynamic(std::string axis, std::string var, std::string value, std::string block_id = {}) const = 0;
};

class ExperimentNodeFactory final : public AbstractNodeFactory {
public:
    std::unique_ptr<INodeDescription>
    make_static(std::string axis, std::string value, bool pinned, std::string block_id = {}) const override {
        return std::make_unique<StaticAxisNode>(std::move(axis), std::move(value), pinned, std::move(block_id));
    }
    std::unique_ptr<INodeDescription>
    make_dynamic(std::string axis, std::string var, std::string value, std::string block_id = {}) const override {
        return std::make_unique<DynamicVariableNode>(std::move(axis), std::move(var), std::move(value), std::move(block_id));
    }
};

// ── (Optional materialisierter) Baumknoten — bei lazy Iteration je Pfad EIN temporäres Blatt-Objekt ──
struct TreeNode {
    std::unique_ptr<INodeDescription>       desc;     // null = Wurzel/lazy-Blatt
    std::string                              key;      // §2: serialisierte Signatur Wurzel→hier (= binary_id)
    NodeValue                                value;    // §2: Observer-Statistics/Auswertung (aus der sparse value_map)
    std::vector<std::unique_ptr<TreeNode>>   children;
    [[nodiscard]] bool is_leaf() const noexcept { return children.empty(); }
};

/// Statisches Achsen-Level. values.size()==1 → gepinnt (Fanout 1), >1 → freigegeben (Fanout N).
struct AxisLevel {
    std::string              axis;
    std::vector<std::string> values;
    bool                     is_static = true;   // static/dynamic = KNOTEN-EIGENSCHAFT (gleichrangig im Baum)
    std::string              variable;            // ungenutzt für statische Ebenen
    std::string              block_id;            // Rück-Referenz auf den compile-time AxisBlock (Bidirektionalität)
};

/// Dynamische Dimension = eine virtuelle (nicht materialisierte) for-Schleife über einer Binary.
struct DynamicDim {
    std::string              axis;
    std::string              variable;
    std::vector<std::string> values;
    std::string              block_id;   // Rück-Referenz auf den compile-time AxisBlock (Bidirektionalität)
};

// HINWEIS (User 2026-06-02): AxisBlock ist ein COMPILE-TIME-Metaprogrammier-Konstrukt (registrier-/erweiterbar),
// definiert in der Reflektions-Schicht (registry_to_axis_levels.hpp / axis_block compile-time), NICHT hier als
// Runtime-Struktur. Im GESAMTBAUM sind statische und dynamische Achsen GLEICHWERTIG (flache, gleichrangige
// Knoten-Folge) — die static/dynamic-Unterscheidung ist eine KNOTEN-EIGENSCHAFT (StaticAxisNode/DynamicVariableNode,
// Abstract Factory), KEINE strukturelle Block-Hierarchie. Der compile-time AxisBlock wird in flache AxisLevel/
// DynamicDim reflektiert, jeweils mit `block_id` getaggt (Bidirektionalität: Knoten → Block, INodeDescription::block_id()).

/// DAS Blatt = exakt EINE Experiment-Einstellung (Binary × eine vollständige dynamische Belegung).
struct ExperimentSetting {
    std::string              binary_id;          // statische Rekombination = die Tier-Binary
    std::string              pinned_signature;   // Paper-Wiedererkennung (KF-15)
    std::vector<std::string> dynamic_assignment; // akkumulierte dyn. Belegung (je "axis.var=value")
    std::string              setting_id;         // binary_id + dyn. Belegung = eindeutige ID (ersetzt fingerprint)
    TreeNode const*          binary_node = nullptr;  // bei lazy Iteration nullptr (kein persistenter Baum)
};

// ── KF-16 / Doc 26 §2 — indizierte LAZY Sicht auf den STATISCHEN Teilbaum ──
/// Die statischen Eigenschaften EINER zu kompilierenden Tier-Binary (= ein Static-Pfad/Blatt).
struct BinarySpec {
    std::size_t                                      index = 0;  // 0-basiert, stabile Reihenfolge
    std::string                                      binary_id;        // serialisierter Static-Pfad = die Binary
    std::string                                      pinned_signature; // gepinnte Achsen (Paper-Wiedererkennung)
    std::vector<std::pair<std::string, std::string>> axes;             // (achse, wert) je statischer Ebene
};

/// LAZY indizierter Blick auf den statischen Teilbaum. „Interface, um die notwendigen statischen Eigenschaften
/// zur Kompilation aller Tier-Binaries indexiert mit einem Iterator zu durchlaufen" (User 2026-06-02). Hält NUR
/// die Ebenen-Metadaten + Divisoren; `operator[](i)` DEKODIERT den Index i mixed-radix on-demand zu genau EINER
/// BinarySpec (Ebene 0 = höchstwertig). Es wird NIE ein ∏-großes vector vorgehalten → der Build-Orchestrator
/// hält nur so viele Specs gleichzeitig wie zulässige DLL-Build-Prozesse.
class StaticBinaryView {
public:
    StaticBinaryView() = default;
    explicit StaticBinaryView(std::vector<AxisLevel> static_levels) : levels_{std::move(static_levels)} {
        size_ = levels_.empty() ? 0 : 1;
        for (auto const& l : levels_) size_ *= l.values.size();   // size_t-Kardinalität (∏ mod 2^64 bei Overflow)
        // Divisoren-Cache (most-significant = Ebene 0): div_[d] = ∏_{e>d} size_e
        div_.assign(levels_.size(), 1);
        for (std::size_t d = levels_.size(); d-- > 0;)
            div_[d] = (d + 1 < levels_.size()) ? div_[d + 1] * levels_[d + 1].values.size() : 1;
    }
    [[nodiscard]] std::size_t size()  const noexcept { return size_; }
    [[nodiscard]] bool        empty() const noexcept { return size_ == 0; }

    // ── D1 (L-SEL): Ebenen-Metadaten für die endliche BuildSelection (Coverage-Sampling) ──
    /// Zahl der statischen Ebenen (= Achsen der Binary-Identität).
    [[nodiscard]] std::size_t level_count() const noexcept { return levels_.size(); }
    /// Varianten-Zahl der Ebene d (0 wenn d außerhalb) — Eingabe für one_wise_cover_sample.
    [[nodiscard]] std::size_t level_size(std::size_t d) const noexcept {
        return d < levels_.size() ? levels_[d].values.size() : 0;
    }
    /// mixed-radix ENKODIERUNG (Inverse von operator[]): tuple[d] = Varianten-Index der Ebene d → flacher
    /// View-Index i (== Σ_d tuple[d]·div_[d]). Da div_ die Mixed-Radix-Stellenwerte hält, gilt
    /// (i / div_[d]) % level_size(d) == tuple[d] (für tuple[d] < level_size(d)) → operator[](flat_index(t)) trifft t.
    [[nodiscard]] std::size_t flat_index(std::span<const std::size_t> tuple) const noexcept {
        std::size_t i = 0;
        for (std::size_t d = 0; d < levels_.size() && d < tuple.size(); ++d) i += tuple[d] * div_[d];
        return i;
    }

    /// On-demand: dekodiert Index i mixed-radix → EINE BinarySpec (materialisiert genau einen Pfad).
    [[nodiscard]] BinarySpec operator[](std::size_t i) const {
        BinarySpec s; s.index = i;
        std::string bin, pin;
        for (std::size_t d = 0; d < levels_.size(); ++d) {
            auto const& l = levels_[d];
            if (l.values.empty()) continue;
            std::size_t const k = (i / div_[d]) % l.values.size();
            std::string seg = l.axis; seg += '='; seg += l.values[k];
            if (!bin.empty()) bin += '/'; bin += seg;
            if (l.values.size() == 1) { if (!pin.empty()) pin += '/'; pin += seg; }
            s.axes.emplace_back(l.axis, l.values[k]);
        }
        s.binary_id = std::move(bin); s.pinned_signature = std::move(pin);
        return s;
    }

    /// LAZY input-iterator (materialisiert je Schritt genau EINE BinarySpec — by value).
    class iterator {
    public:
        using value_type = BinarySpec; using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag; using pointer = void; using reference = BinarySpec;
        iterator() = default;
        iterator(StaticBinaryView const* v, std::size_t i) : v_{v}, i_{i} {}
        [[nodiscard]] BinarySpec operator*() const { return (*v_)[i_]; }
        iterator& operator++() { ++i_; return *this; }
        iterator  operator++(int) { auto t = *this; ++i_; return t; }
        [[nodiscard]] bool operator==(iterator const& o) const { return i_ == o.i_; }
        [[nodiscard]] bool operator!=(iterator const& o) const { return i_ != o.i_; }
    private:
        StaticBinaryView const* v_ = nullptr; std::size_t i_ = 0;
    };
    [[nodiscard]] iterator begin() const { return {this, 0}; }
    [[nodiscard]] iterator end()   const { return {this, size_}; }

private:
    std::vector<AxisLevel>   levels_;
    std::vector<std::size_t> div_;
    std::size_t              size_ = 0;
};

class ExperimentTree {
public:
    explicit ExperimentTree(std::shared_ptr<AbstractNodeFactory> factory)
        : factory_{std::move(factory)} { root_ = std::make_unique<TreeNode>(); }

    /// Baut aus dem ZUSAMMENHÄNGENDEN Gesamt-Level-Satz (statische + dynamische Ebenen). Speichert NUR die
    /// Ebenen-Metadaten (statisch/dynamisch gefiltert) — es wird KEIN ∏-großer Knotenbaum materialisiert
    /// (User 2026-06-02: nie der ganze Baum, nur ein Pfad zur Zeit; Doc 26 §5: zählen statt materialisieren).
    void build(std::vector<AxisLevel> const& all_levels) {
        all_levels_    = all_levels;
        static_levels_ = static_filter();
        dynamic_dims_  = dynamic_filter();
    }

    /// FILTER: statische Ebenen (Binary-Identität).
    [[nodiscard]] std::vector<AxisLevel> static_filter() const {
        std::vector<AxisLevel> out;
        for (auto const& l : all_levels_) if (l.is_static) out.push_back(l);
        return out;
    }
    /// FILTER: dynamische Dimensionen (virtuelle for-Schleifen).
    [[nodiscard]] std::vector<DynamicDim> dynamic_filter() const {
        std::vector<DynamicDim> out;
        for (auto const& l : all_levels_) if (!l.is_static) out.push_back(DynamicDim{l.axis, l.variable, l.values, l.block_id});
        return out;
    }

    [[nodiscard]] TreeNode const& root() const { return *root_; }

    /// Kardinalität der statischen Binaries = ∏ der statischen Ebenen-Größen — REIN ARITHMETISCH (Doc 26 §5),
    /// OHNE Materialisierung (== ∏ mp_size(Enabled_i) == PermutationEngine::count(), Doc 27 §6).
    [[nodiscard]] std::size_t binary_count() const {
        if (static_levels_.empty()) return 0;
        std::size_t n = 1;
        for (auto const& l : static_levels_) n *= l.values.size();
        return n;
    }

    /// LAZY Iteration über die Binary-Blätter: materialisiert je Schritt EINEN Pfad Wurzel→Blatt (mixed-radix
    /// Odometer, O(Tiefe) Speicher) und ruft visitor(binary_id, pinned_signature, leaf). `leaf` ist ein
    /// temporäres Blatt-Objekt (key=binary_id, value=sparse NodeValue), gültig NUR während des Aufrufs.
    /// ⚠️ O(∏) Zeit — nur auf handhabbaren (Pilot-)Bäumen voll durchlaufen; für riesige Inventare zählt
    /// binary_count() / greift static_binary_view()[i] indiziert zu.
    template <class Visitor>
    void for_each_binary(Visitor&& v) const {
        if (static_levels_.empty()) return;
        for (auto const& l : static_levels_) if (l.values.empty()) return;  // leere Ebene → 0 Binaries
        std::vector<std::size_t> idx(static_levels_.size(), 0);
        for (;;) {
            std::string bin, pin;
            for (std::size_t d = 0; d < static_levels_.size(); ++d) {
                auto const& l = static_levels_[d];
                std::string seg = l.axis; seg += '='; seg += l.values[idx[d]];
                if (!bin.empty()) bin += '/'; bin += seg;
                if (l.values.size() == 1) { if (!pin.empty()) pin += '/'; pin += seg; }
            }
            TreeNode leaf; leaf.key = bin;
            if (auto it = value_map_.find(bin); it != value_map_.end()) leaf.value = it->second;
            v(bin, pin, static_cast<TreeNode const&>(leaf));
            std::size_t d = static_levels_.size(); bool carry = true;
            while (d-- > 0) { if (++idx[d] < static_levels_[d].values.size()) { carry = false; break; } idx[d] = 0; }
            if (carry) break;
        }
    }
    [[nodiscard]] std::size_t binary_count_traversed() const {
        std::size_t n = 0; for_each_binary([&](std::string const&, std::string const&, TreeNode const&) { ++n; }); return n;
    }

    /// Bidirektionalität (User 2026-06-02): besucht EINEN Repräsentanten-Knoten je Achse (statische Ebene +
    /// dynamische Dim) — über kind()/block_id()/axis() sind die Achsen-BLÖCKE auf dem Gesamtbaum filter-/zuordbar
    /// (z.B. „alle Knoten des Blocks X"). EIN Knoten je Achse (Block-Struktur), NICHT ∏ (kein OOM). visitor(INodeDescription const&).
    template <class Visitor>
    void for_each_node(Visitor&& v) const {
        for (auto const& l : static_levels_) {
            if (l.values.empty()) continue;
            auto node = factory_->make_static(l.axis, l.values.front(), l.values.size() == 1, l.block_id);
            v(static_cast<INodeDescription const&>(*node));
        }
        for (auto const& d : dynamic_dims_) {
            if (d.values.empty()) continue;
            auto node = factory_->make_dynamic(d.axis, d.variable, d.values.front(), d.block_id);
            v(static_cast<INodeDescription const&>(*node));
        }
    }

    /// VIRTUELL: je Binary × geschachtelte for-Schleifen über dynamic_dims_ → je Kombination eine
    /// ExperimentSetting (= das Blatt). visitor(ExperimentSetting const&). ⚠️ O(∏) — Pilot/Limit-Nutzung.
    template <class Visitor>
    void for_each_experiment_setting(Visitor&& v) const {
        for_each_binary([&](std::string const& bin, std::string const& pin, TreeNode const&) {
            std::vector<std::string> assign;
            expand(bin, pin, 0, assign, v);
        });
    }
    [[nodiscard]] std::size_t experiment_setting_count() const {
        std::size_t n = binary_count();
        for (auto const& d : dynamic_dims_) n *= d.values.size();   // ∏ — arithmetisch, ohne Materialisierung
        return n;
    }

    /// KF-15: multimap<pinned signature, binary_id> — Paper-Wiedererkennung. ⚠️ O(∏) + ∏ Einträge: nur auf
    /// handhabbaren Bäumen aufrufen (die inverse Auswertung projiziert real GEMESSENE Blätter, Doc 26 §3).
    [[nodiscard]] std::multimap<std::string, std::string> pinned_signature_index() const {
        std::multimap<std::string, std::string> idx;
        for_each_binary([&](std::string const& bin, std::string const& pin, TreeNode const&) { idx.emplace(pin, bin); });
        return idx;
    }

    /// KF-16: LAZY indizierte Sicht auf den statischen Teilbaum (alle zu bauenden Tier-Binaries, geordnet).
    /// Hält NUR die Ebenen — operator[](i) dekodiert je Binary on-demand; der Build-Orchestrator materialisiert
    /// nur so viele Pfade gleichzeitig wie zulässige DLL-Build-Prozesse (User 2026-06-02).
    [[nodiscard]] StaticBinaryView static_binary_view() const { return StaticBinaryView{static_levels_}; }

    // ── §2 per-node Observer-Statistics: SPARSE (nur GEMESSENE Binaries; BR-3) ──
    /// Setzt/aktualisiert die Mess-Auswertung EINES gemessenen Binary-Pfads (key = binary_id). Nur gemessene
    /// Pfade haben einen Eintrag — NIE alle ∏ (OOM-sicher).
    void set_node_value(std::string const& binary_id, NodeValue const& value) { value_map_[binary_id] = value; }
    [[nodiscard]] NodeValue node_value(std::string const& binary_id) const {
        auto it = value_map_.find(binary_id); return it == value_map_.end() ? NodeValue{} : it->second;
    }
    [[nodiscard]] std::size_t measured_node_count() const noexcept { return value_map_.size(); }

    /// Zerlegt einen serialisierten Static-Pfad ("achse=wert/achse=wert/…") in (achse, wert)-Paare.
    [[nodiscard]] static std::vector<std::pair<std::string, std::string>> parse_axes(std::string const& path) {
        std::vector<std::pair<std::string, std::string>> out;
        std::size_t i = 0;
        while (i < path.size()) {
            std::size_t slash = path.find('/', i);
            std::string seg   = path.substr(i, (slash == std::string::npos ? path.size() : slash) - i);
            std::size_t eq    = seg.find('=');
            if (eq != std::string::npos) out.emplace_back(seg.substr(0, eq), seg.substr(eq + 1));
            if (slash == std::string::npos) break;
            i = slash + 1;
        }
        return out;
    }

private:
    template <class Visitor>
    void expand(std::string const& bin, std::string const& pin,
                std::size_t dim, std::vector<std::string>& assign, Visitor& v) const {
        if (dim >= dynamic_dims_.size()) {
            ExperimentSetting s;
            s.binary_id          = bin;
            s.pinned_signature   = pin;
            s.dynamic_assignment = assign;
            s.setting_id         = bin;
            s.binary_node        = nullptr;   // lazy: kein persistenter Baum-Knoten
            for (auto const& seg : assign) { s.setting_id += "/"; s.setting_id += seg; }
            v(s);
            return;
        }
        DynamicDim const& d = dynamic_dims_[dim];
        for (auto const& val : d.values) {
            auto node = factory_->make_dynamic(d.axis, d.variable, val, d.block_id);  // EIN dyn. Knoten zur Zeit
            assign.push_back(node->serialize());
            expand(bin, pin, dim + 1, assign, v);
            assign.pop_back();
        }
    }

    std::shared_ptr<AbstractNodeFactory>      factory_;
    std::unique_ptr<TreeNode>                 root_;          // leere Wurzel (kein materialisierter Voll-Baum)
    std::vector<AxisLevel>                    static_levels_; // gefilterte statische Ebenen (für lazy Iteration)
    std::vector<DynamicDim>                   dynamic_dims_;  // virtuelle dynamische for-Schleifen
    std::vector<AxisLevel>                    all_levels_;    // zusammenhängender Gesamt-Level-Satz (vor Filterung)
    std::map<std::string, NodeValue>          value_map_;     // §2 SPARSE: key=binary_id → NodeValue (nur gemessene)
};

}  // namespace comdare::cache_engine::builder::experiment
