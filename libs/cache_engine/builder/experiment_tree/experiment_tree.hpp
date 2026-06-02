#pragma once
// KF-9 (2026-06-02) — Experiment-B+-Baum (Experiment-Manager der Cache-Engine-Bibliothek).
//
// Ersetzt die flache mp_product/4-fach-for-Enumeration (builder/permutation_loop) + den FNV1a-
// compute_fingerprint. Maßgebliches Modell: docs/architecture/26 (Permutations-B+-Baum + inverse Signatur).
//
// AUSFÜHRUNGSSEMANTIK (User 2026-06-02):
//   • STATISCHE Knoten (StaticAxisNode) = compile-time-Entscheidung → je distinkter Static-Pfad lädt EINE
//     NEUE Tier-Binary ins Prüf-Dock. Umfasst Achsen-Algorithmen UND compile-time-Sub-Properties (cacheline
//     line_size/alignment/sw_hint — in die Binary gebacken). Der Static-Pfad = die Binary-Identität.
//   • DYNAMISCHE Knoten (DynamicVariableNode) = Laufzeit-FOR-SCHLEIFE auf EINER bereits geladenen Tier-Binary:
//     probiert nacheinander Test-Einstellungen über die Variablen-Schnittstelle (Algorithm_Resource_Control,
//     KF-4) durch — thread_count, hw_prefetcher, prefetch_distance, pool_budget, batch_size. KEINE neue Binary.
//
// SCHICHT: das WIE (Permutation/Organisation). Tiere/Organe UNVERÄNDERT — der Baum lädt/misst/organisiert nur.
// Für Suche immer Bäume (lineare Traversierung). Knoten static/dynamic via ABSTRACT FACTORY (zwei Einzelklassen,
// KEIN enum-Discriminator-Ersatz). JEDE node hält key (serialisierte Signatur Wurzel→Knoten) + value
// (Observer-Statistics/Mess-Auswertung der Ebene, §2). C++23, header-only.

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

// ── Per-node Value (§2): Observer-Statistics + Mess-Auswertung der Ebene (uint64-nah) ──
struct NodeValue {
    std::uint64_t measured_leaf_count = 0;
    std::uint64_t sum_total_cycles    = 0;
    std::uint64_t sum_op_count        = 0;
    bool          has_result          = false;
};

enum class NodeKind { Static, Dynamic };  // Introspektion/Serialisierung — NICHT als Verhaltens-Discriminator

class INodeDescription {
public:
    virtual ~INodeDescription() = default;
    [[nodiscard]] virtual NodeKind    kind() const noexcept = 0;
    [[nodiscard]] virtual std::string axis() const = 0;
    [[nodiscard]] virtual std::string value() const = 0;
    [[nodiscard]] virtual std::string serialize() const = 0;
    [[nodiscard]] virtual bool contributes_to_signature() const noexcept = 0;         // = Binary-Identität (statisch)
    [[nodiscard]] virtual bool contributes_to_pinned_signature() const noexcept = 0;  // nur gepinnt (Paper-Signatur)
    [[nodiscard]] virtual bool is_runtime_loop() const noexcept = 0;                  // dynamisch = Laufzeit-Schleife
};

/// StaticAxisNode — compile-time-Entscheidung (Achsen-Algorithmus ODER compile-time-Sub-Property wie cacheline).
/// Jeder distinkte Static-Pfad → eine NEUE Binary ins Prüf-Dock. pinned=true → global nicht permutiert (Paper-Signatur).
class StaticAxisNode final : public INodeDescription {
public:
    StaticAxisNode(std::string axis, std::string value, bool pinned)
        : axis_{std::move(axis)}, value_{std::move(value)}, pinned_{pinned} {}
    NodeKind    kind() const noexcept override { return NodeKind::Static; }
    std::string axis() const override { return axis_; }
    std::string value() const override { return value_; }
    std::string serialize() const override { return axis_ + "=" + value_; }
    bool contributes_to_signature() const noexcept override { return true; }
    bool contributes_to_pinned_signature() const noexcept override { return pinned_; }
    bool is_runtime_loop() const noexcept override { return false; }
    [[nodiscard]] bool pinned() const noexcept { return pinned_; }
private:
    std::string axis_, value_;
    bool pinned_;
};

/// DynamicVariableNode — Laufzeit-Variable, die als FOR-SCHLEIFE auf einer GELADENEN Binary über die
/// Variablen-Schnittstelle (Algorithm_Resource_Control) durchprobiert wird (thread_count, hw_prefetcher, ...).
/// Erzeugt KEINE neue Binary, sondern einen Mess-Lauf je Wert.
class DynamicVariableNode final : public INodeDescription {
public:
    DynamicVariableNode(std::string axis, std::string var, std::string value)
        : axis_{std::move(axis)}, var_{std::move(var)}, value_{std::move(value)} {}
    NodeKind    kind() const noexcept override { return NodeKind::Dynamic; }
    std::string axis() const override { return axis_; }
    std::string value() const override { return value_; }
    std::string serialize() const override { return axis_ + "." + var_ + "=" + value_; }
    bool contributes_to_signature() const noexcept override { return false; }
    bool contributes_to_pinned_signature() const noexcept override { return false; }
    bool is_runtime_loop() const noexcept override { return true; }
    [[nodiscard]] std::string variable() const { return var_; }
private:
    std::string axis_, var_, value_;
};

class AbstractNodeFactory {
public:
    virtual ~AbstractNodeFactory() = default;
    [[nodiscard]] virtual std::unique_ptr<INodeDescription>
        make_static(std::string axis, std::string value, bool pinned) const = 0;
    [[nodiscard]] virtual std::unique_ptr<INodeDescription>
        make_dynamic(std::string axis, std::string var, std::string value) const = 0;
};

class ExperimentNodeFactory final : public AbstractNodeFactory {
public:
    std::unique_ptr<INodeDescription>
    make_static(std::string axis, std::string value, bool pinned) const override {
        return std::make_unique<StaticAxisNode>(std::move(axis), std::move(value), pinned);
    }
    std::unique_ptr<INodeDescription>
    make_dynamic(std::string axis, std::string var, std::string value) const override {
        return std::make_unique<DynamicVariableNode>(std::move(axis), std::move(var), std::move(value));
    }
};

struct TreeNode {
    std::unique_ptr<INodeDescription>       desc;     // null = Wurzel
    std::string                              key;      // §2: serialisierte Signatur Wurzel→hier
    NodeValue                                value;    // §2: Observer-Statistics/Auswertung der Ebene
    std::vector<std::unique_ptr<TreeNode>>   children;
    [[nodiscard]] bool is_leaf() const noexcept { return children.empty(); }
};

/// Achsen-Level der Profil-Spezifikation. values.size()==1 → gepinnt (Fanout 1), >1 → freigegeben (Fanout N).
/// is_static=true → StaticAxisNode (compile-time → Binary); false → DynamicVariableNode (Laufzeit-Schleife).
/// Konvention: statische (compile-time) Levels ZUERST, dann dynamische (Laufzeit) Levels.
struct AxisLevel {
    std::string              axis;
    std::vector<std::string> values;
    bool                     is_static = true;
    std::string              variable;   // nur dynamic (Variablen-Name)
};

/// Ein traversiertes Blatt = EIN Mess-Lauf (Binary × Laufzeit-Einstellung).
struct LeafView {
    std::string path;              // voller serialisierter Pfad = eindeutige ID (ersetzt fingerprint)
    std::string binary_id;         // nur STATISCHE Knoten = die zu ladende Tier-Binary
    std::string runtime_setting;   // nur DYNAMISCHE Knoten = die Laufzeit-for-Schleifen-Einstellung
    std::string pinned_signature;  // nur gepinnte Knoten = Paper-Wiedererkennung (KF-15)
    TreeNode const* node = nullptr;
};

class ExperimentTree {
public:
    explicit ExperimentTree(std::shared_ptr<AbstractNodeFactory> factory)
        : factory_{std::move(factory)} { root_ = std::make_unique<TreeNode>(); }

    void build(std::vector<AxisLevel> const& levels) { build_recursive(*root_, levels, 0, ""); }
    [[nodiscard]] TreeNode const& root() const { return *root_; }

    template <class Visitor>
    void for_each_leaf(Visitor&& v) const {
        std::vector<INodeDescription const*> path;
        traverse(*root_, path, v);
    }

    [[nodiscard]] std::size_t leaf_count() const {
        std::size_t n = 0; for_each_leaf([&](LeafView const&) { ++n; }); return n;
    }

    /// Zahl der distinkten zu ladenden Tier-Binaries (= distinkte Static-Pfade).
    [[nodiscard]] std::size_t binary_count() const {
        std::map<std::string, int> bins;
        for_each_leaf([&](LeafView const& lv) { bins[lv.binary_id]; });
        return bins.size();
    }

    /// Prüf-Dock-Modell: je Binary EINMAL laden, dann FOR-SCHLEIFE über ihre Laufzeit-Einstellungen.
    /// visitor(binary_id, vector<runtime_setting>).
    template <class Visitor>
    void for_each_binary(Visitor&& v) const {
        std::map<std::string, std::vector<std::string>> by_binary;  // sortiert (Baum/Map, nicht quadratisch)
        for_each_leaf([&](LeafView const& lv) { by_binary[lv.binary_id].push_back(lv.runtime_setting); });
        for (auto const& [bin, settings] : by_binary) v(bin, settings);
    }

    /// KF-15: multimap<pinned signature, binary_id> — Paper-Wiedererkennung (Doppelerkennung via multimap).
    [[nodiscard]] std::multimap<std::string, std::string> pinned_signature_index() const {
        std::multimap<std::string, std::string> idx;
        for_each_leaf([&](LeafView const& lv) { idx.emplace(lv.pinned_signature, lv.binary_id); });
        return idx;
    }

private:
    void build_recursive(TreeNode& parent, std::vector<AxisLevel> const& levels,
                         std::size_t depth, std::string const& prefix) {
        if (depth >= levels.size()) return;
        AxisLevel const& lvl = levels[depth];
        bool const pinned = (lvl.values.size() == 1);
        for (auto const& val : lvl.values) {
            auto child = std::make_unique<TreeNode>();
            child->desc = lvl.is_static ? factory_->make_static(lvl.axis, val, pinned)
                                        : factory_->make_dynamic(lvl.axis, lvl.variable, val);
            std::string const seg = child->desc->serialize();
            child->key = prefix.empty() ? seg : (prefix + "/" + seg);
            build_recursive(*child, levels, depth + 1, child->key);
            parent.children.push_back(std::move(child));
        }
    }

    template <class Visitor>
    void traverse(TreeNode const& node, std::vector<INodeDescription const*>& path, Visitor& v) const {
        bool const pushed = (node.desc != nullptr);
        if (pushed) path.push_back(node.desc.get());
        if (node.is_leaf() && node.desc) {
            LeafView lv; lv.node = &node;
            auto append = [](std::string& s, std::string const& seg) { if (!s.empty()) s += "/"; s += seg; };
            for (auto const* d : path) {
                append(lv.path, d->serialize());
                if (d->contributes_to_signature())        append(lv.binary_id, d->serialize());
                else if (d->is_runtime_loop())            append(lv.runtime_setting, d->serialize());
                if (d->contributes_to_pinned_signature()) append(lv.pinned_signature, d->serialize());
            }
            v(lv);
        } else {
            for (auto const& c : node.children) traverse(*c, path, v);
        }
        if (pushed) path.pop_back();
    }

    std::shared_ptr<AbstractNodeFactory> factory_;
    std::unique_ptr<TreeNode>            root_;
};

}  // namespace comdare::cache_engine::builder::experiment
