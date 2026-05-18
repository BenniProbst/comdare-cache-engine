#pragma once
// V32.DD.1 (2026-05-18) - AutoPermutateAxisCommand
//
// @subsystem CEB
// @command_pattern AutoPermutate
// @phase_owner CEB (Phase 5 BIND - kann fehlende Achsen-Spec aufloesen)

#include "i_command.hpp"
#include "auto_permutator.hpp"
#include <string>
#include <vector>

namespace comdare::cache_engine::builder::commands {

/**
 * @brief AutoPermutateAxisCommand - lookup CE-Bibliothek + permutiert fehlende Achse
 * @subsystem CEB
 * @command_pattern AutoPermutate
 * @phase_owner CEB
 *
 * KRITISCHE V32.DD.1-FUNKTION (Korrektur AA.3):
 *
 * User-Direktive 2026-05-18: "Wenn bei einem Algorithmus eine Axe nicht explizit
 * definiert wird, dann ist es die Aufgabe des Testsystems aus dem bestehenden Stand
 * der Technik der CacheEngine den besten existierenden Algorithmus der besagten
 * Axe/Kategorie durch Permutation zu ermitteln und auszumessen."
 *
 * Algorithmus:
 * 1. Detect missing axis in algorithm_profile XML
 * 2. Lookup verfuegbare SOTA-Bausteine der Achse in CE-Bibliothek
 * 3. Filter per IPlatformProbe (nur Host-faehige Bausteine)
 * 4. Filter per messreihen.xml allowed_variants (User-Limit)
 * 5. Generiere Permutationen
 * 6. Pro Permutation: ExecuteEngineCommand laufen lassen
 * 7. CompareEngineCommand zum Bestimmen des Besten
 * 8. Ergebnis: best_variant_for_axis pro Algorithmus
 *
 * Beispiel:
 *   PrtArt-Profil definiert Achse 12 HARDWARE_STRATEGY NICHT
 *   -> AutoPermutateAxisCommand fuer Achse 12 wird gestartet
 *   -> Lookup CE-Bibliothek: AVX2, AVX-512, NEON, SVE2, scalar (5 Variants)
 *   -> Host-Filter: nur AVX2 + scalar verfuegbar
 *   -> User-Limit (optional): allowed_variants="AVX2,scalar" -> beide laufen
 *   -> Generiere 2 Permutationen: PrtArt+AVX2, PrtArt+scalar
 *   -> Vergleiche -> "PrtArt mit AVX2" gewinnt
 */
class AutoPermutateAxisCommand : public ICommand {
public:
    AutoPermutateAxisCommand(std::string axis_id,
                             std::string_view engine_name,
                             Workload workload) noexcept
        : permutator_{std::move(axis_id)}, engine_name_{engine_name}, workload_{workload} {}

    [[nodiscard]] std::string_view command_name() const noexcept override {
        return "AutoPermutateAxisCommand";
    }

    int execute() override {
        // V32.EE.2: konkrete Default-Lookup-Loop
        permutator_.discover_axis_implementations();
        permutator_.platform_filter();
        // user_limit_filter() wird via setter aufgerufen (allowed_variants aus XML)
        results_ = permutator_.execute_all_variants(engine_name_, workload_);
        return 0;
    }

    [[nodiscard]] bool is_parallelizable() const noexcept override { return true; }

    [[nodiscard]] const std::vector<ExecutionResult>& results() const noexcept {
        return results_;
    }

private:
    AutoPermutator permutator_;
    std::string_view engine_name_;
    Workload workload_;
    std::vector<ExecutionResult> results_ {};
};

}  // namespace comdare::cache_engine::builder::commands
