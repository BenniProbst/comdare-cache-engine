#pragma once
// V32.EE.2 (2026-05-18 spaet) - AutoPermutator fuer CE-Bibliothek-Lookup (AA.3)
//
// @subsystem CEB
// @phase_owner CEB
// @command_pattern AutoPermutate

#include "workload.hpp"
#include "execution_result.hpp"

#include <algorithm>
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::commands {

/**
 * @brief AxisVariant - eine konkrete Auspraegung einer Bausteine-Achse
 * @subsystem CEB
 */
struct AxisVariant {
    std::string axis_id {};           ///< z.B. "12.1" SIMD-Family
    std::string variant_name {};      ///< z.B. "AVX2"
    std::string ce_library_path {};   ///< Pfad zur CE-Bibliothek-Klasse
    bool host_compatible {true};      ///< IPlatformProbe-Filter-Output
    bool user_allowed {true};         ///< messreihen.xml allowed_variants-Filter
};

/**
 * @brief AutoPermutator - Discovery + Filter + Permutation der CE-Bibliothek
 * @subsystem CEB
 * @phase_owner CEB
 *
 * Implementiert User-Direktive AA.3:
 * "Default-Aktion bei fehlender Spezifikation ist Nachschlagen in der Bibliothek.
 *  Pruefung ALLER verfuegbaren Algorithmen der Achse, sofern in Pruef-Konfiguration
 *  nicht explizit limitiert."
 *
 * 5-Schritt-Workflow:
 * 1. discover_axis_implementations(axis_id) -> alle SOTA-Bausteine der Achse
 * 2. platform_filter() -> nur Host-faehige (via IPlatformProbe)
 * 3. user_limit_filter(allowed_variants) -> User-Limit anwenden
 * 4. generate_permutations() -> Permutations-Descriptor pro Variant
 * 5. iterate + ExecuteEngineCommand pro Variant + CompareEngineCommand
 */
class AutoPermutator {
public:
    explicit AutoPermutator(std::string axis_id) noexcept
        : axis_id_{std::move(axis_id)} {}

    /// Schritt 1: Lookup aller verfuegbaren CE-Bausteine fuer diese Achse
    /// V32.HH.1: Lookup via AxisLibraryRegistry (hardcoded Tabelle, V33+ via Doxygen-Tag-Extraktion)
    void discover_axis_implementations();  // implementiert in axis_library_registry.hpp

    /// Schritt 2: IPlatformProbe-basierter Filter
    /// V32.EE.2 Skelett - IPlatformProbe-Verdrahtung in V32.2+
    void platform_filter() {
        // Pro Variant: pruefe IPlatformProbe.supports(variant)
        // setze host_compatible = result
    }

    /// Schritt 3: User-Limit aus messreihen.xml
    void user_limit_filter(std::span<const std::string_view> allowed_variants) {
        for (auto& v : available_variants_) {
            v.user_allowed = std::ranges::any_of(allowed_variants,
                [&v](auto allowed) { return allowed == v.variant_name; });
        }
    }

    /// Schritt 4: Generiert Permutationen aus host_compatible + user_allowed-Variants
    [[nodiscard]] std::vector<AxisVariant> generate_permutations() const {
        std::vector<AxisVariant> result;
        for (const auto& v : available_variants_) {
            if (v.host_compatible && v.user_allowed) {
                result.push_back(v);
            }
        }
        return result;
    }

    /// Schritt 5: Iteriert + fuehrt ExecuteEngineCommand pro Variant aus
    /// V32.EE.2 Skelett - Comparator-Integration in V32.2+
    [[nodiscard]] std::vector<ExecutionResult> execute_all_variants(
        std::string_view engine_name,
        const Workload& workload) {
        std::vector<ExecutionResult> results;
        for (const auto& v : generate_permutations()) {
            ExecuteEngineCommand cmd(engine_name, workload);
            cmd.execute();
            // Variant-Metadata in result einbetten (V32.2 Sprint)
            results.push_back(cmd.result());
        }
        return results;
    }

private:
    std::string axis_id_;
    std::vector<AxisVariant> available_variants_ {};
};

/**
 * @brief MultiAxisPermutationResult - alle Variants ueber alle default-lookup-Achsen
 * @subsystem CEB
 */
struct MultiAxisPermutationResult {
    std::vector<std::string> axis_ids;                                  ///< In Eingabe-Reihenfolge
    std::vector<std::vector<AxisVariant>> variants_per_axis;            ///< parallel zu axis_ids
    [[nodiscard]] std::size_t cartesian_size() const noexcept {
        if (variants_per_axis.empty()) return 0;
        std::size_t prod = 1;
        for (const auto& v : variants_per_axis) {
            prod *= v.empty() ? 1 : v.size();
        }
        return prod;
    }
};

/**
 * @brief MultiAxisAutoPermutator - V33.B.2 Bridge fuer Pruefling-default-lookup
 * @subsystem CEB
 * @phase_owner CEB
 *
 * Konsumiert eine Achsen-Liste (z.B. von prt_art::default_lookup::DefaultLookupRegistry
 * oder XML-Config) und ruft pro Achse AutoPermutator auf. Liefert MultiAxisPermutationResult
 * fuer Cross-Product-Iteration im V32Orchestrator.
 *
 * Beispiel:
 *   std::vector<std::string> axes {"3.B", "11", "12.1"};  // PRT-ART braucht Defaults hier
 *   MultiAxisAutoPermutator multi(axes);
 *   multi.discover_all();
 *   auto plan = multi.build_plan();
 *   // plan.cartesian_size() = 3 * 4 * 5 = 60 Permutationen
 */
class MultiAxisAutoPermutator {
public:
    explicit MultiAxisAutoPermutator(std::vector<std::string> axis_ids) noexcept
        : axis_ids_{std::move(axis_ids)} {}

    void discover_all() {
        per_axis_permutators_.clear();
        per_axis_permutators_.reserve(axis_ids_.size());
        for (const auto& axis : axis_ids_) {
            AutoPermutator p {axis};
            p.discover_axis_implementations();
            p.platform_filter();
            per_axis_permutators_.push_back(std::move(p));
        }
    }

    [[nodiscard]] MultiAxisPermutationResult build_plan() const {
        MultiAxisPermutationResult plan;
        plan.axis_ids = axis_ids_;
        plan.variants_per_axis.reserve(per_axis_permutators_.size());
        for (const auto& p : per_axis_permutators_) {
            plan.variants_per_axis.push_back(p.generate_permutations());
        }
        return plan;
    }

    [[nodiscard]] const std::vector<AutoPermutator>& permutators() const noexcept {
        return per_axis_permutators_;
    }

private:
    std::vector<std::string> axis_ids_;
    std::vector<AutoPermutator> per_axis_permutators_ {};
};

}  // namespace comdare::cache_engine::builder::commands
