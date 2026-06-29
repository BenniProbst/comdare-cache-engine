#pragma once
// =====================================================================================
// Dataset-Loader-Slot (AP-CE2) — austauschbare Anbindung Nicht-YCSB-Datensaetze
// -------------------------------------------------------------------------------------
// Bildet externe Datensaetze/Frameworks (SOSD, TPC, SPEC, CloudSuite, gem5, Allokator-
// Suiten) auf das gemeinsame comdare Operation[uint64]-Modell ab, damit ALLE Lastprofile
// gegen ALLE Lebewesen-Binaries laufen koennen (alle-gegen-alle-Matrix; Thesis Kap. 2.4.2/3.4.1).
//
// HEADER-ONLY (INTERFACE-Bibliothek): wird erst kompiliert, wenn ein Consumer ihn einbindet.
// Strategy + Registry-Pattern; Fallback auf den vorhandenen WorkloadGenerator (YCSB).
//
// Stand 2026-06-17 (AP-CE2). Typen verifiziert gegen
//   libs/test_infra/workload_generator/include/comdare/workload_generator/workload_generator.hpp
// Einstieg/Verdrahtung: siehe README.md in diesem Verzeichnis. Achsen-Doku: Doc 35 / Doc 14.
// =====================================================================================

#include <comdare/workload_generator/workload_generator.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace comdare::measurement::dataset_loader {

namespace wg = comdare::workload_generator;

/**
 * @brief Abstrakte Schnittstelle fuer externe Datensatz-Loader.
 *
 * Jeder konkrete Loader (SOSD/TPC/SPEC/CloudSuite/gem5/Allokator-Suite) implementiert
 * load() und bildet seinen Datensatz auf eine Folge von wg::Operation (uint64-Modell) ab.
 */
class DatasetLoaderStrategy {
public:
    virtual ~DatasetLoaderStrategy() = default;

    /// Laedt @p dataset_id und konvertiert in das Operation-Modell.
    /// @param dataset_id loader-spezifischer Bezeichner (Pfad, Tabelle, Trace-Name ...)
    /// @param seed       Reproduzierbarkeits-Schluessel (falls Shuffle/Sampling noetig)
    /// @return Operation-Folge, oder std::nullopt bei Fehler (Aufrufer faellt auf YCSB zurueck).
    [[nodiscard]] virtual std::optional<std::vector<wg::Operation>> load(std::string_view dataset_id,
                                                                         std::uint64_t    seed) = 0;

    /// Optionale Kurzbeschreibung (Quelle, Version) fuer Logs.
    [[nodiscard]] virtual std::string metadata() const { return {}; }
};

/**
 * @brief Registry (Meyers-Singleton): Loader registrieren sich unter einem loader_id-Schluessel.
 *
 * Header-only; loader_id entspricht dem XML-Attribut `dataset_source` (z.B. "sosd", "tpc").
 */
class DatasetLoaderRegistry {
public:
    static DatasetLoaderRegistry& instance() {
        static DatasetLoaderRegistry registry;
        return registry;
    }

    void register_loader(std::string loader_id, std::unique_ptr<DatasetLoaderStrategy> loader) {
        loaders_[std::move(loader_id)] = std::move(loader);
    }

    [[nodiscard]] bool has(std::string_view loader_id) const {
        return loaders_.find(std::string{loader_id}) != loaders_.end();
    }

    /// Sucht den Loader @p loader_id und delegiert an dessen load().
    [[nodiscard]] std::optional<std::vector<wg::Operation>>
    try_load(std::string_view loader_id, std::string_view dataset_id, std::uint64_t seed) const {
        auto it = loaders_.find(std::string{loader_id});
        if (it == loaders_.end() || !it->second) return std::nullopt;
        return it->second->load(dataset_id, seed);
    }

private:
    DatasetLoaderRegistry() = default;
    std::unordered_map<std::string, std::unique_ptr<DatasetLoaderStrategy>> loaders_;
};

/// Mappt einen YCSB-Bezeichner ("YCSB_A".."YCSB_F" oder "A".."F") auf das Enum (Default C).
[[nodiscard]] inline wg::YcsbWorkload parse_ycsb_letter(std::string_view s) {
    if (s == "YCSB_A" || s == "A") return wg::YcsbWorkload::A;
    if (s == "YCSB_B" || s == "B") return wg::YcsbWorkload::B;
    if (s == "YCSB_C" || s == "C") return wg::YcsbWorkload::C;
    if (s == "YCSB_D" || s == "D") return wg::YcsbWorkload::D;
    if (s == "YCSB_E" || s == "E") return wg::YcsbWorkload::E;
    if (s == "YCSB_F" || s == "F") return wg::YcsbWorkload::F;
    return wg::YcsbWorkload::C;
}

/**
 * @brief Komfort-Einstieg fuer Phase 5 (siehe README): erst Registry, sonst YCSB-Generator.
 *
 * @param loader_source XML-`dataset_source` (leer = direkt YCSB-Generator)
 * @param dataset_id    XML-`dataset_id` bzw. YCSB-Buchstabe
 * @param config        WorkloadConfig (num_keys/num_operations/seed ...)
 * @param seed          Reproduzierbarkeits-Schluessel fuer den Loader
 * @return Operation-Folge (nie leer im Fehlerfall — Fallback erzeugt YCSB).
 */
[[nodiscard]] inline std::vector<wg::Operation> load_or_generate_ycsb(std::string_view          loader_source,
                                                                      std::string_view          dataset_id,
                                                                      wg::WorkloadConfig const& config,
                                                                      std::uint64_t             seed) {
    if (!loader_source.empty()) {
        auto loaded = DatasetLoaderRegistry::instance().try_load(loader_source, dataset_id, seed);
        if (loaded && !loaded->empty()) return std::move(*loaded);
    }
    wg::WorkloadGenerator gen{config};
    return gen.generate_ycsb(parse_ycsb_letter(dataset_id));
}

} // namespace comdare::measurement::dataset_loader
