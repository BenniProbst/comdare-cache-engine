#pragma once
// V5-#46-REFERENZ — Disk-persistierender MementoAxis: Beweis des /goal-Kontrakts „Memento inkl. IO/Disk-
// Persistenz (einfacher Snapshot reicht NICHT)".
//
// Hintergrund (Re-Audit + I8-Analyse): die aktuellen io_dispatch/serialization/migration-Achsen sind stateless
// Compile-Time-Deskriptoren (kein Disk-State). Das /goal verlangt aber, dass der Memento-Mechanismus DISK-
// PERSISTENZ TRAGEN KANN, sobald eine Achse echten Disk-Zustand hat. Diese Referenz-Achse demonstriert genau
// das: ihr Zustand liegt auf DISK (Checkpoint-Datei), der `memento_t` trägt NUR den Pfad — ein reiner
// In-Memory-Snapshot würde den Disk-Zustand NICHT erfassen. Damit ist die MementoAxis-Maschinerie (V5-I5)
// nachweislich disk-fähig; eine künftige echte Disk-Achse (mmap-`io_dispatch`, persistente `migration_policy`)
// implementiert dasselbe Muster.
//
// @doku docs/architecture/messarchitektur_v5_i8_memento_vollstaendigkeit.md (Vorwärts-Kontrakt)

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace comdare::cache_engine::builder {

/// Referenz-Achse mit DISK-residentem Zustand. Erfüllt das MementoAxis-Concept (memento_aggregate.hpp) via
/// Disk-Checkpoint: save_state schreibt den Zustand in die Checkpoint-Datei + liefert deren Pfad als memento_t;
/// restore_state liest ihn ZURÜCK von der Disk. Der memento_t (Pfad) ist KEIN Snapshot des Werts — der Wert
/// lebt auf der Platte. Beweist: „einfacher Snapshot reicht NICHT", die Persistenz quert die Disk.
class DiskCheckpointStore {
public:
    using memento_t = std::string; ///< Pfad zur Checkpoint-Datei (Zustand liegt AUF DISK, nicht im Memento)

    explicit DiskCheckpointStore(std::filesystem::path checkpoint_file) : ckpt_{std::move(checkpoint_file)} {}

    void                        set(std::uint64_t v) noexcept { value_ = v; }
    [[nodiscard]] std::uint64_t get() const noexcept { return value_; }

    /// Kapselt den Zustand auf DISK (Checkpoint-Datei) + liefert den Pfad. IO-Fehler → leerer Pfad (Robustheit).
    [[nodiscard]] memento_t save_state() const {
        std::error_code ec;
        if (auto const dir = ckpt_.parent_path(); !dir.empty()) std::filesystem::create_directories(dir, ec);
        std::ofstream os{ckpt_, std::ios::binary | std::ios::trunc};
        if (!os) return {};
        os.write(reinterpret_cast<char const*>(&value_), sizeof value_);
        return os.good() ? ckpt_.string() : std::string{};
    }

    /// Rollt den Zustand von der DISK zurück (liest die Checkpoint-Datei). Leerer/fehlender Pfad → no-op.
    void restore_state(memento_t const& path) {
        if (path.empty()) return;
        std::ifstream is{path, std::ios::binary};
        if (!is) return;
        std::uint64_t v = 0;
        is.read(reinterpret_cast<char*>(&v), sizeof v);
        if (is.good() || is.eof()) value_ = v;
    }

private:
    std::filesystem::path ckpt_;
    std::uint64_t         value_ = 0;
};

} // namespace comdare::cache_engine::builder
