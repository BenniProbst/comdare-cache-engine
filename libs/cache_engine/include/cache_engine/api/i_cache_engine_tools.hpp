// SPDX-License-Identifier: Apache-2.0
// V41.F.4 — ICacheEngineTools: Tools-Plugin-Concept-Facade.
//
// Vermittelt die bereits IN-REPO vorhandenen Analyse-/Codegen-/Workload-Werkzeuge über EIN
// gemeinsames Interface an Konsumenten (ExperimentDriver / Diplomarbeit), ohne dass diese die
// einzelnen Tool-Header direkt einbinden müssen — und erlaubt register_external_tool() für
// Plugin-Werkzeuge (z.B. PMC-Hardware-Counter, externe Statistik-Backends, Cluster-Submitter).
//
// ABGRENZUNG zu ICacheEngine (E11, i_cache_engine.hpp): jene Facade vermittelt die 6 AUSGELAGERTEN
// Submodule (search-engine/core/measurement/isa-dispatch/build-tools/test-system), die via E4.1
// noch leer und damit V42-gated sind. DIESE Facade (F.4) vermittelt dagegen vier KONKRET in-repo
// vorhandene Werkzeuge → vollständig header-only, ohne externe Abhängigkeit sofort nutzbar:
//   • Statistik:   builder/commands/welch_t_test.hpp + mann_whitney_u_test.hpp  (Welch / Mann-Whitney)
//   • Codegen:     builder/codegen/adhoc_emitter.hpp  (render_adhoc_module_source — Modul-.cpp-Emission)
//   • Workloads:   test_infra/workload_generator        (YCSB A–F Workload-Generierung)
//
// Hinweis Layering: Latenz-kritische Hot-Path-Achsen sind compile-time-only (kein virtual). Diese
// Facade ist dagegen ein BUILDER-/HOST-seitiges Werkzeug-Interface (analog ICacheEngine) — virtuelle
// Vermittlung ist hier korrekt und konsistent mit dem E11-Facade-Muster.

#pragma once

#include <builder/commands/welch_t_test.hpp>
#include <builder/commands/mann_whitney_u_test.hpp>
#include <builder/codegen/adhoc_emitter.hpp>
#include <comdare/workload_generator/workload_generator.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace comdare::cache_engine::api {

// ─────────────────────────────────────────────────────────────────────────────
// Sub-Tool-Interfaces (pure virtual) — je Werkzeug-Kategorie ein schmales Interface.
// ─────────────────────────────────────────────────────────────────────────────

/// Statistik-Werkzeug: zwei-Stichproben-Signifikanztests über Latenz-Samples (ns, int64).
class IStatisticsTool {
public:
    virtual ~IStatisticsTool() = default;
    [[nodiscard]] virtual builder::commands::stats::WelchResult welch_t_test(std::span<const std::int64_t> a,
                                                                             std::span<const std::int64_t> b) const = 0;
    [[nodiscard]] virtual builder::commands::stats::MannWhitneyResult
    mann_whitney_u_test(std::span<const std::int64_t> a, std::span<const std::int64_t> b) const = 0;
};

/// Codegen-Werkzeug: emittiert den kompilierbaren Modul-.cpp-Quelltext einer Permutation aus den
/// (bereits extrahierten) FQ-Achsen-Typ-Namen. Laufzeit-Half des template-getriebenen adhoc_emitter.
class ICodegenTool {
public:
    virtual ~ICodegenTool() = default;
    [[nodiscard]] virtual std::string emit_module_source(int                          module_index,
                                                         std::span<const std::string> fq_axis_type_names) const = 0;
};

/// Workload-Werkzeug: erzeugt YCSB-Standard-Workloads (A–F) für die Mess-Pipeline.
class IWorkloadTool {
public:
    virtual ~IWorkloadTool() = default;
    [[nodiscard]] virtual std::vector<workload_generator::Operation>
    generate_ycsb(workload_generator::WorkloadConfig const& config,
                  workload_generator::YcsbWorkload          workload) const = 0;
};

/// Plugin-Punkt für EXTERNE Werkzeuge (register_external_tool): alles, was über die vier
/// Built-in-Tools hinausgeht — z.B. PMC-Counter-Backend, Cluster-Job-Submitter, externe Solver.
class IExternalTool {
public:
    virtual ~IExternalTool()                                 = default;
    [[nodiscard]] virtual std::string_view tool_name() const = 0;
    /// Kategorie-Hinweis (z.B. "measurement", "statistics", "codegen", "transport").
    [[nodiscard]] virtual std::string_view tool_kind() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Master-Tools-Facade.
// ─────────────────────────────────────────────────────────────────────────────
class ICacheEngineTools {
public:
    virtual ~ICacheEngineTools() = default;

    // Built-in-Werkzeuge (Owner bleibt die Facade).
    [[nodiscard]] virtual IStatisticsTool& statistics()         = 0;
    [[nodiscard]] virtual ICodegenTool&    codegen()            = 0;
    [[nodiscard]] virtual IWorkloadTool&   workload_generator() = 0;

    // Plugin-Registry für externe Werkzeuge.
    virtual void register_external_tool(std::string name, std::shared_ptr<IExternalTool> tool)     = 0;
    [[nodiscard]] virtual IExternalTool*           find_external_tool(std::string_view name) const = 0;
    [[nodiscard]] virtual std::vector<std::string> external_tool_names() const                     = 0;

    [[nodiscard]] virtual std::string_view framework_version() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Header-only Default-Implementation — adaptiert die ECHTEN in-repo-Werkzeuge.
// ─────────────────────────────────────────────────────────────────────────────

class DefaultStatisticsTool final : public IStatisticsTool {
public:
    [[nodiscard]] builder::commands::stats::WelchResult welch_t_test(std::span<const std::int64_t> a,
                                                                     std::span<const std::int64_t> b) const override {
        return builder::commands::stats::welch_t_test(a, b);
    }
    [[nodiscard]] builder::commands::stats::MannWhitneyResult
    mann_whitney_u_test(std::span<const std::int64_t> a, std::span<const std::int64_t> b) const override {
        return builder::commands::stats::mann_whitney_u_test(a, b);
    }
};

class DefaultCodegenTool final : public ICodegenTool {
public:
    [[nodiscard]] std::string emit_module_source(int                          module_index,
                                                 std::span<const std::string> fq_axis_type_names) const override {
        std::string args;
        for (auto const& t : fq_axis_type_names) {
            if (!args.empty()) args += ",\n    ";
            args += t;
        }
        // Gemeinsames Emitter-Textformat (DRY mit emit_adhoc_modules<Engine>).
        return builder::codegen::render_adhoc_module_source(module_index, args);
    }
};

class DefaultWorkloadTool final : public IWorkloadTool {
public:
    [[nodiscard]] std::vector<workload_generator::Operation>
    generate_ycsb(workload_generator::WorkloadConfig const& config,
                  workload_generator::YcsbWorkload          workload) const override {
        workload_generator::WorkloadGenerator gen{config};
        return gen.generate_ycsb(workload);
    }
};

class DefaultCacheEngineTools final : public ICacheEngineTools {
public:
    [[nodiscard]] IStatisticsTool& statistics() override { return statistics_; }
    [[nodiscard]] ICodegenTool&    codegen() override { return codegen_; }
    [[nodiscard]] IWorkloadTool&   workload_generator() override { return workload_; }

    void register_external_tool(std::string name, std::shared_ptr<IExternalTool> tool) override {
        external_tools_[std::move(name)] = std::move(tool);
    }
    [[nodiscard]] IExternalTool* find_external_tool(std::string_view name) const override {
        auto const it = external_tools_.find(std::string{name});
        return it == external_tools_.end() ? nullptr : it->second.get();
    }
    [[nodiscard]] std::vector<std::string> external_tool_names() const override {
        std::vector<std::string> names;
        names.reserve(external_tools_.size());
        for (auto const& [k, _] : external_tools_) names.push_back(k);
        return names;
    }
    [[nodiscard]] std::string_view framework_version() const override { return "comdare-tools-v1"; }

private:
    DefaultStatisticsTool                                           statistics_{};
    DefaultCodegenTool                                              codegen_{};
    DefaultWorkloadTool                                             workload_{};
    std::unordered_map<std::string, std::shared_ptr<IExternalTool>> external_tools_{};
};

/// Factory — eine prozessweite Default-Instanz (inline-Funktion ⇒ ein static-Local über alle TUs).
/// Konsumenten linken NUR gegen cache-engine (header-only) und nutzen ICacheEngineTools.
[[nodiscard]] inline ICacheEngineTools& get_cache_engine_tools() {
    static DefaultCacheEngineTools instance;
    return instance;
}

} // namespace comdare::cache_engine::api
