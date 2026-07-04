#pragma once
// =====================================================================================
// String-Korpus-Loader (AP-10): string_corpus
// -------------------------------------------------------------------------------------
// Liest eine Textdatei mit einem String pro Zeile und bildet jede Zeile deterministisch
// via FNV-1a 64-bit auf das bestehende Operation[uint64]-Modell ab. Keine String-Keys,
// keine externe Hash-Abhaengigkeit, keine Zufallsquelle.
//
// Aktivierung: diesen Header in einer gelinkten Uebersetzungseinheit einbinden
// (Selbst-Registrierung via inline-Variable, ODR-sicher). loader_id == "string_corpus".
// =====================================================================================

#include <comdare/measurement/dataset_loader/dataset_loader.hpp>

#include <cstdint>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::measurement::dataset_loader::loaders {

namespace string_corpus_detail {

inline constexpr std::uint64_t kFnv1a64OffsetBasis = 14695981039346656037ull;
inline constexpr std::uint64_t kFnv1a64Prime       = 1099511628211ull;

[[nodiscard]] inline std::uint64_t fnv1a64(std::string_view bytes) noexcept {
    std::uint64_t hash = kFnv1a64OffsetBasis;
    for (char const byte : bytes) {
        hash ^= static_cast<unsigned char>(byte);
        hash *= kFnv1a64Prime;
    }
    return hash;
}

} // namespace string_corpus_detail

/// Liest Strings (eine Zeile = ein Schluessel) aus @p dataset_id (Dateipfad) -> Read-Operationen.
class StringCorpusLoader final : public DatasetLoaderStrategy {
public:
    [[nodiscard]] std::optional<std::vector<wg::Operation>> load(std::string_view dataset_id,
                                                                 std::uint64_t /*seed*/) override {
        std::ifstream in{std::string{dataset_id}};
        if (!in) return std::nullopt;

        std::vector<wg::Operation> ops;
        std::string                line;
        while (std::getline(in, line)) {
            ops.push_back(wg::Operation{wg::OperationKind::Read, string_corpus_detail::fnv1a64(line), 0});
        }
        if (ops.empty()) return std::nullopt;
        return ops;
    }

    [[nodiscard]] std::string metadata() const override {
        return "string_corpus: one string per line -> FNV-1a uint64 Read ops (deterministic)";
    }
};

/// Selbst-Registrierung beim Einbinden dieses Headers (inline-Variable -> ODR-gemerged, einmalig).
inline const bool kStringCorpusLoaderRegistered = [] {
    DatasetLoaderRegistry::instance().register_loader("string_corpus", std::make_unique<StringCorpusLoader>());
    return true;
}();

} // namespace comdare::measurement::dataset_loader::loaders