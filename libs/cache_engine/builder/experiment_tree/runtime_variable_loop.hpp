#pragma once
// KF-7 (2026-06-02) — RuntimeVariableLoop: der dynamische Laufzeit-Durchlauf am Prüf-Dock.
//
// Doc 26 §2 (Ausführungssemantik): die DYNAMISCHEN Knoten sind eine FOR-SCHLEIFE auf EINER bereits geladenen
// Tier-Binary — sie probieren nacheinander die Test-Einstellungen über die Variablen-Schnittstelle
// (Algorithm_Resource_Control, KF-4) durch und erzeugen KEINE neue Binary. Dieser Treiber realisiert genau das:
// er durchläuft die virtuelle Kartesik der dynamischen Dimensionen (DynamicDim, KF-9) auf EINEM geladenen
// IResourceControllableTier, wendet je Kombination den geklammerten Resource-Control-POD an (AlgorithmResource-
// Control, KF-4) und ruft je Einstellung einen Mess-Callback — alles OHNE Neu-Laden/Neu-Bauen der Binary.
//
// Variablen→POD-Feld-Abbildung (KF-4 ComdareResourceControlV1): thread_count/prefetch_distance/pool_budget_bytes/
// batch_size/inline_threshold_bytes. ARCHITEKTONISCHE Laufzeit-Ausnahmen (z.B. hw_prefetcher via MSR 0x1A4) sind
// KEIN POD-Feld → werden im Label geführt, aber erst vom SLURM/MSR-Launcher angewandt (KF-12, Cluster-gated).
//
// Schicht-Trennung: dies ist Builder-Seite (das WIE des Messens); die Tier-Binary bleibt unverändert. C++23.

#include "experiment_tree.hpp"
#include "../algorithm_resource_control.hpp"

#include <charconv>
#include <cstdint>
#include <string>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// Eine angewandte Laufzeit-Einstellung (= ein dynamisches Blatt auf der geladenen Binary).
struct RuntimeSetting {
    std::string                       setting_label;        // akkumulierte dyn. Belegung "axis.var=value/…"
    anatomy::ComdareResourceControlV1 requested{};          // gewünschte Werte (vor Clamp)
    anatomy::ComdareResourceControlV1 applied{};            // effektiv angewandt (nach Clamp auf caps∩env)
    std::uint64_t                     applied_axis_count = 0;
};

class RuntimeVariableLoop {
public:
    /// env_limits = System-Ressourcenlimits als Obergrenze (z.B. verfügbare Kerne; 0 = unbegrenzt).
    explicit RuntimeVariableLoop(anatomy::ComdareResourceControlV1 env_limits = {}) noexcept
        : env_{env_limits} {}

    /// Durchläuft die virtuelle Kartesik der dynamischen Dimensionen auf der GELADENEN Binary `tier`
    /// (NICHT neu geladen). visitor(RuntimeSetting const&) je Einstellung. Liefert die Einstellungszahl.
    template <class Visitor>
    std::size_t run(anatomy::IResourceControllableTier& tier,
                    std::vector<DynamicDim> const& dims, Visitor&& visitor) const {
        std::vector<std::string> assign;
        std::size_t count = 0;
        expand(tier, dims, 0, assign, visitor, count);
        return count;
    }

    /// Bildet einen Variablennamen auf das zuständige Resource-Control-POD-Feld ab (0 = unbekannt/architektonisch).
    static void set_field(anatomy::ComdareResourceControlV1& pod, std::string const& var, std::uint64_t v) noexcept {
        if (var == "thread_count")                pod.thread_count           = v;
        else if (var == "prefetch_distance")      pod.prefetch_distance      = v;
        else if (var == "pool_budget_bytes")      pod.pool_budget_bytes      = v;
        else if (var == "batch_size")             pod.batch_size             = v;
        else if (var == "inline_threshold_bytes") pod.inline_threshold_bytes = v;
        // sonst: architektonische Laufzeit-Ausnahme (hw_prefetcher/MSR …) → kein POD-Feld (KF-12-Launcher).
    }

private:
    [[nodiscard]] static std::uint64_t parse_u64(std::string const& s) noexcept {
        std::uint64_t v = 0;
        std::from_chars(s.data(), s.data() + s.size(), v);  // nicht-numerisch (z.B. "off") → 0
        return v;
    }

    template <class Visitor>
    void expand(anatomy::IResourceControllableTier& tier, std::vector<DynamicDim> const& dims,
                std::size_t dim, std::vector<std::string>& assign, Visitor& visitor, std::size_t& count) const {
        if (dim >= dims.size()) {
            anatomy::ComdareResourceControlV1 req{};
            for (auto const& seg : assign) {
                std::size_t const eq = seg.find('=');
                std::size_t const dot = seg.find('.');
                if (eq == std::string::npos) continue;
                std::string const var = (dot != std::string::npos && dot < eq)
                                        ? seg.substr(dot + 1, eq - dot - 1) : seg.substr(0, eq);
                set_field(req, var, parse_u64(seg.substr(eq + 1)));
            }
            anatomy::ComdareResourceControlV1 caps{};
            tier.tier_query_resource_caps(&caps);
            anatomy::ComdareResourceControlV1 applied = AlgorithmResourceControl::clamp(req, caps, env_);
            std::uint64_t const nax = tier.tier_apply_resource_control(&applied);

            std::string label;
            for (auto const& seg : assign) { if (!label.empty()) label += "/"; label += seg; }
            RuntimeSetting s{std::move(label), req, applied, nax};
            visitor(s);
            ++count;
            return;
        }
        DynamicDim const& d = dims[dim];
        for (auto const& val : d.values) {
            assign.push_back(d.axis + "." + d.variable + "=" + val);  // = DynamicVariableNode::serialize
            expand(tier, dims, dim + 1, assign, visitor, count);
            assign.pop_back();
        }
    }

    anatomy::ComdareResourceControlV1 env_;
};

}  // namespace comdare::cache_engine::builder::experiment
