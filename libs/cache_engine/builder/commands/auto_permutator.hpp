#pragma once
// V32.EE.2 (2026-05-18 spaet) - AutoPermutator fuer CE-Bibliothek-Lookup (AA.3)
//
// @subsystem CEB
// @phase_owner CEB
// @command_pattern AutoPermutate

#include "workload.hpp"
#include "execution_result.hpp"

#include <cstdint>
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

}  // namespace comdare::cache_engine::builder::commands
