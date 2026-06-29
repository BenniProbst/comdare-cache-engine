#pragma once
// =====================================================================================
// Beispiel-Loader (AP-CE2): example_uint64_file
// -------------------------------------------------------------------------------------
// Demonstriert den Dataset-Loader-Slot an einem minimalen, deterministischen Fall:
// liest eine Textdatei mit je einem uint64-Schluessel pro Zeile und erzeugt daraus eine
// reine Read-Last. Vorlage fuer echte Framework-Loader (SOSD/TPC/SPEC/CloudSuite/gem5/
// Allokator-Suiten) — diese ersetzen nur die Parse-/Mapping-Logik in load().
//
// Aktivierung: diesen Header in einer gelinkten Uebersetzungseinheit einbinden
// (Selbst-Registrierung via inline-Variable, ODR-sicher). loader_id == "example_uint64_file".
// =====================================================================================

#include <comdare/measurement/dataset_loader/dataset_loader.hpp>

#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::measurement::dataset_loader::loaders {

/// Liest uint64-Schluessel (eine pro Zeile) aus @p dataset_id (Dateipfad) -> Read-Operationen.
class ExampleUint64KeyFileLoader final : public DatasetLoaderStrategy {
public:
    [[nodiscard]] std::optional<std::vector<wg::Operation>> load(std::string_view dataset_id,
                                                                 std::uint64_t /*seed*/) override {
        std::ifstream in{std::string{dataset_id}};
        if (!in) return std::nullopt;
        std::vector<wg::Operation> ops;
        std::uint64_t              key{};
        while (in >> key) { ops.push_back(wg::Operation{wg::OperationKind::Read, key, 0}); }
        if (ops.empty()) return std::nullopt;
        return ops;
    }

    [[nodiscard]] std::string metadata() const override {
        return "example_uint64_file: one uint64 key per line -> Read ops (deterministic)";
    }
};

/// Selbst-Registrierung beim Einbinden dieses Headers (inline-Variable -> ODR-gemerged, einmalig).
inline const bool kExampleUint64KeyFileLoaderRegistered = [] {
    DatasetLoaderRegistry::instance().register_loader("example_uint64_file",
                                                      std::make_unique<ExampleUint64KeyFileLoader>());
    return true;
}();

} // namespace comdare::measurement::dataset_loader::loaders
