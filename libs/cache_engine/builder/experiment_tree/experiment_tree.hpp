#pragma once
// KF-9 (2026-06-02) — Experiment-B+-Baum (Experiment-Manager der Cache-Engine-Bibliothek).
//
// Ersetzt die flache mp_product/4-fach-for-Enumeration + FNV1a-compute_fingerprint. Doc architecture/26.
//
// AUSFÜHRUNGS-/MATERIALISIERUNGSMODELL (User 2026-06-02):
//   • Die permutative Rekombination ALLER STATISCHEN Knoten beschreibt EINE kompilierte Tier-Binary im
//     Prüf-Dock → der materialisierte Baum besteht NUR aus den statischen Ebenen; jedes Static-Blatt = EINE Binary.
//   • Die DYNAMISCHEN Variablen sind VIRTUELL ineinander verschachtelte for-Schleifen (NICHT materialisiert),
//     die alle Rekombinationen ZUR LAUFZEIT über EINER Tier-Binary durchführen (Variablen-Schnittstelle
//     Algorithm_Resource_Control, KF-4/KF-7).
//   • Der Baum ist formal in feste Schichten zu Achsen UND ihren Variablen geordnet, aber erst das BLATT ist
//     eine Akkumulation der dynamischen Variablen-Eigenschaften als EXAKT EINE Experiment-Einstellung
//     (= eine Binary × eine vollständige dynamische Belegung).
//
// Vorteil: der materialisierte Baum bleibt bei der BINARY-Zahl (handhabbar); die Experiment-Einstellungen
// (Binary × dyn. Kartesik) werden virtuell expandiert. Knoten static/dynamic via ABSTRACT FACTORY (zwei
// Einzelklassen). JEDE materialisierte node hält key + value (§2). Für Suche immer Bäume. C++23, header-only.

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

// ── Per-node Value (§2): Observer-Statistics + Mess-Auswertung der Ebene ──
struct NodeValue {
    std::uint64_t measured_setting_count = 0;  // gemessene Experiment-Einstellungen unter diesem Knoten
    std::uint64_t sum_total_cycles       = 0;
    std::uint64_t sum_op_count           = 0;
    bool          has_result             = false;
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
};

/// StaticAxisNode — compile-time-Entscheidung → MATERIALISIERT; je Static-Pfad eine Tier-Binary.
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

/// DynamicVariableNode — Laufzeit-Variable → VIRTUELLE for-Schleife (nicht materialisiert). Wird je
/// Schleifen-Punkt von der Factory erzeugt und in die Experiment-Einstellung akkumuliert.
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

// ── Materialisierte (statische) Baumknoten ──
struct TreeNode {
    std::unique_ptr<INodeDescription>       desc;     // null = Wurzel
    std::string                              key;      // §2: serialisierte Signatur Wurzel→hier
    NodeValue                                value;    // §2: Observer-Statistics/Auswertung
    std::vector<std::unique_ptr<TreeNode>>   children;
    [[nodiscard]] bool is_leaf() const noexcept { return children.empty(); }
};

/// Statisches Achsen-Level (materialisiert). values.size()==1 → gepinnt (Fanout 1), >1 → freigegeben (Fanout N).
struct AxisLevel {
    std::string              axis;
    std::vector<std::string> values;
    bool                     is_static = true;   // hier stets statisch (materialisiert)
    std::string              variable;            // ungenutzt für statische Ebenen
};

/// Dynamische Dimension = eine virtuelle (nicht materialisierte) for-Schleife über einer Binary.
struct DynamicDim {
    std::string              axis;
    std::string              variable;
    std::vector<std::string> values;
};

/// DAS Blatt = exakt EINE Experiment-Einstellung (Binary × eine vollständige dynamische Belegung).
struct ExperimentSetting {
    std::string              binary_id;          // statische Rekombination = die Tier-Binary
    std::string              pinned_signature;   // Paper-Wiedererkennung (KF-15)
    std::vector<std::string> dynamic_assignment; // akkumulierte dyn. Belegung (je "axis.var=value")
    std::string              setting_id;         // binary_id + dyn. Belegung = eindeutige ID (ersetzt fingerprint)
    TreeNode const*          binary_node = nullptr;
};

// ── KF-16: indizierte Sicht auf den STATISCHEN Teilbaum (Bereitstellung der Tier-Binaries vor Experimenten) ──
/// Die statischen Eigenschaften EINER zu kompilierenden Tier-Binary (= ein Static-Pfad/Blatt).
struct BinarySpec {
    std::size_t                                      index = 0;  // 0-basiert, stabile Reihenfolge
    std::string                                      binary_id;        // serialisierter Static-Pfad = die Binary
    std::string                                      pinned_signature; // gepinnte Achsen (Paper-Wiedererkennung)
    std::vector<std::pair<std::string, std::string>> axes;             // (achse, wert) je statischer Ebene
};

/// Indizierter, iterierbarer Blick auf den statischen Teilbaum. „Interface, um die notwendigen statischen
/// Eigenschaften zur Kompilation aller notwendigen Tier-Binaries indexiert mit einem Iterator zu durchlaufen"
/// (User 2026-06-02). Random-Access (operator[]) + Bereichs-Iteration (begin/end) für den Build-Orchestrator.
class StaticBinaryView {
public:
    StaticBinaryView() = default;
    explicit StaticBinaryView(std::vector<BinarySpec> specs) : specs_{std::move(specs)} {}
    [[nodiscard]] std::size_t       size()  const noexcept { return specs_.size(); }
    [[nodiscard]] bool              empty() const noexcept { return specs_.empty(); }
    [[nodiscard]] BinarySpec const& operator[](std::size_t i) const { return specs_.at(i); }
    [[nodiscard]] auto begin() const noexcept { return specs_.begin(); }
    [[nodiscard]] auto end()   const noexcept { return specs_.end(); }
private:
    std::vector<BinarySpec> specs_;
};

class ExperimentTree {
public:
    explicit ExperimentTree(std::shared_ptr<AbstractNodeFactory> factory)
        : factory_{std::move(factory)} { root_ = std::make_unique<TreeNode>(); }

    /// Baut aus dem ZUSAMMENHÄNGENDEN Gesamt-Level-Satz (statische + dynamische Ebenen, geordnet, je via
    /// is_static getaggt). Filtert intern in den statischen Teilbaum (materialisiert → Binaries) + den
    /// dynamischen Teilbaum (virtuelle for-Schleifen). „Baum-Filterung nach statisch/dynamisch, auch wenn der
    /// Gesamtbaum immer zusammenhängend existiert" (User 2026-06-02).
    void build(std::vector<AxisLevel> const& all_levels) {
        all_levels_ = all_levels;
        build_static(static_filter());
        set_dynamic_dims(dynamic_filter());
    }

    /// FILTER: statischer Teilbaum (materialisierte Ebenen → Binary-Identität).
    [[nodiscard]] std::vector<AxisLevel> static_filter() const {
        std::vector<AxisLevel> out;
        for (auto const& l : all_levels_) if (l.is_static) out.push_back(l);
        return out;
    }
    /// FILTER: dynamischer Teilbaum (virtuelle for-Schleifen-Dimensionen).
    [[nodiscard]] std::vector<DynamicDim> dynamic_filter() const {
        std::vector<DynamicDim> out;
        for (auto const& l : all_levels_) if (!l.is_static) out.push_back(DynamicDim{l.axis, l.variable, l.values});
        return out;
    }

    /// Materialisiert NUR die statischen Ebenen → Binary-Blätter (Low-Level; i.d.R. via build()).
    void build_static(std::vector<AxisLevel> const& static_levels) {
        build_recursive(*root_, static_levels, 0, "");
    }
    /// Setzt die dynamischen Dimensionen (virtuelle, ineinander geschachtelte for-Schleifen).
    void set_dynamic_dims(std::vector<DynamicDim> dims) { dynamic_dims_ = std::move(dims); }

    [[nodiscard]] TreeNode const& root() const { return *root_; }

    /// Physische Binary-Blätter (statische Rekombination). visitor(binary_id, pinned_signature, TreeNode&).
    template <class Visitor>
    void for_each_binary(Visitor&& v) const {
        std::vector<INodeDescription const*> path;
        traverse_binaries(*root_, path, v);
    }
    [[nodiscard]] std::size_t binary_count() const {
        std::size_t n = 0; for_each_binary([&](std::string const&, std::string const&, TreeNode const&) { ++n; }); return n;
    }

    /// VIRTUELL: je Binary × geschachtelte for-Schleifen über dynamic_dims_ → je Kombination eine
    /// ExperimentSetting (= das Blatt). visitor(ExperimentSetting const&).
    template <class Visitor>
    void for_each_experiment_setting(Visitor&& v) const {
        for_each_binary([&](std::string const& bin, std::string const& pin, TreeNode const& leaf) {
            std::vector<std::string> assign;
            expand(bin, pin, &leaf, 0, assign, v);
        });
    }
    [[nodiscard]] std::size_t experiment_setting_count() const {
        std::size_t n = 0; for_each_experiment_setting([&](ExperimentSetting const&) { ++n; }); return n;
    }

    /// KF-15: multimap<pinned signature, binary_id> — Paper-Wiedererkennung (Doppelerkennung via multimap).
    [[nodiscard]] std::multimap<std::string, std::string> pinned_signature_index() const {
        std::multimap<std::string, std::string> idx;
        for_each_binary([&](std::string const& bin, std::string const& pin, TreeNode const&) { idx.emplace(pin, bin); });
        return idx;
    }

    /// KF-16: indizierte Sicht auf den statischen Teilbaum (alle zu bauenden Tier-Binaries, geordnet). Der
    /// Build-Orchestrator stellt darüber VOR den Experimenten je Binary genau ein DLL bereit.
    [[nodiscard]] StaticBinaryView static_binary_view() const {
        std::vector<BinarySpec> specs;
        for_each_binary([&](std::string const& bin, std::string const& pin, TreeNode const&) {
            BinarySpec s;
            s.index            = specs.size();
            s.binary_id        = bin;
            s.pinned_signature = pin;
            s.axes             = parse_axes(bin);
            specs.push_back(std::move(s));
        });
        return StaticBinaryView{std::move(specs)};
    }

private:
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

    void build_recursive(TreeNode& parent, std::vector<AxisLevel> const& levels,
                         std::size_t depth, std::string const& prefix) {
        if (depth >= levels.size()) return;
        AxisLevel const& lvl = levels[depth];
        bool const pinned = (lvl.values.size() == 1);
        for (auto const& val : lvl.values) {
            auto child = std::make_unique<TreeNode>();
            child->desc = factory_->make_static(lvl.axis, val, pinned);  // statisch → materialisiert
            std::string const seg = child->desc->serialize();
            child->key = prefix.empty() ? seg : (prefix + "/" + seg);
            build_recursive(*child, levels, depth + 1, child->key);
            parent.children.push_back(std::move(child));
        }
    }

    template <class Visitor>
    void traverse_binaries(TreeNode const& node, std::vector<INodeDescription const*>& path, Visitor& v) const {
        bool const pushed = (node.desc != nullptr);
        if (pushed) path.push_back(node.desc.get());
        if (node.is_leaf() && node.desc) {
            std::string bin, pin;
            auto app = [](std::string& s, std::string const& seg) { if (!s.empty()) s += "/"; s += seg; };
            for (auto const* d : path) {
                if (d->contributes_to_signature())        app(bin, d->serialize());
                if (d->contributes_to_pinned_signature()) app(pin, d->serialize());
            }
            v(bin, pin, node);
        } else {
            for (auto const& c : node.children) traverse_binaries(*c, path, v);
        }
        if (pushed) path.pop_back();
    }

    template <class Visitor>
    void expand(std::string const& bin, std::string const& pin, TreeNode const* leaf,
                std::size_t dim, std::vector<std::string>& assign, Visitor& v) const {
        if (dim >= dynamic_dims_.size()) {
            ExperimentSetting s;
            s.binary_id          = bin;
            s.pinned_signature   = pin;
            s.dynamic_assignment = assign;
            s.binary_node        = leaf;
            s.setting_id         = bin;
            for (auto const& seg : assign) { s.setting_id += "/"; s.setting_id += seg; }
            v(s);
            return;
        }
        DynamicDim const& d = dynamic_dims_[dim];
        for (auto const& val : d.values) {
            auto node = factory_->make_dynamic(d.axis, d.variable, val);  // Factory erzeugt den dyn. Knoten
            assign.push_back(node->serialize());
            expand(bin, pin, leaf, dim + 1, assign, v);
            assign.pop_back();
        }
    }

    std::shared_ptr<AbstractNodeFactory> factory_;
    std::unique_ptr<TreeNode>            root_;
    std::vector<DynamicDim>              dynamic_dims_;
    std::vector<AxisLevel>               all_levels_;  // zusammenhängender Gesamt-Level-Satz (vor Filterung)
};

}  // namespace comdare::cache_engine::builder::experiment
