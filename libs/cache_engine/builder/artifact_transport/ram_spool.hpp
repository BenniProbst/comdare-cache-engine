#pragma once
// ram_spool.hpp -- G3 / #46b Lagerhaltung, Scheibe B6 (Ledger §62-B-NACHTRAG-2).
//
// Der RAM-SAMMELPUFFER der CEB: die fertig kompilierten Binaries werden NICHT einzeln auf Platte
// geschrieben (das laesst den Cache erkalten), sondern als Objekt im RAM gehalten und erst beim
// AUSLOESER gebuendelt persistiert. Ausloeser = Groesse >= Budget (Vorschlag max 256 MB) ODER Anzahl
// >= Dutzend-Trigger, was ZUERST kommt. Der eigentliche Rueckschreib-IO laeuft im SpoolWriter-Thread
// (spool_writer.hpp), getrennt vom Compile-Pool.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib.

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::artifact_transport {

// Budget-Vorschlag max 256 MB (§62-B-NACHTRAG-2); Dutzend-Trigger = 12.
inline constexpr std::size_t kMaxSpoolBytes     = 256ull * 1024 * 1024;
inline constexpr std::size_t kSpoolCountTrigger = 12;

// Ein Spool-Eintrag: Zielpfad + der im RAM gehaltene Binary-Inhalt.
struct SpoolEntry {
    std::filesystem::path dest;
    std::string           bytes;
};

// RAM-Sammelpuffer. Akkumuliert Eintraege bis ein Trigger greift. Nicht selbst thread-sicher --
// der SpoolWriter serialisiert die Zugriffe unter einem Mutex.
class RamSpool {
public:
    explicit RamSpool(std::size_t max_bytes = kMaxSpoolBytes, std::size_t count_trigger = kSpoolCountTrigger)
        : max_bytes_{max_bytes}, count_trigger_{count_trigger} {}

    void add(SpoolEntry e) {
        total_bytes_ += e.bytes.size();
        entries_.push_back(std::move(e));
    }

    // Trigger: Groesse ODER Anzahl erreicht (was zuerst kommt).
    [[nodiscard]] bool should_flush() const noexcept {
        return total_bytes_ >= max_bytes_ || entries_.size() >= count_trigger_;
    }

    [[nodiscard]] bool        empty() const noexcept { return entries_.empty(); }
    [[nodiscard]] std::size_t count() const noexcept { return entries_.size(); }
    [[nodiscard]] std::size_t total_bytes() const noexcept { return total_bytes_; }

    // Entnimmt ALLE Eintraege (leert den Puffer) -> Uebergabe an den Writer-Thread als ein Batch.
    [[nodiscard]] std::vector<SpoolEntry> drain() {
        std::vector<SpoolEntry> out = std::move(entries_);
        entries_.clear();
        total_bytes_ = 0;
        return out;
    }

private:
    std::vector<SpoolEntry> entries_;
    std::size_t             total_bytes_ = 0;
    std::size_t             max_bytes_;
    std::size_t             count_trigger_;
};

} // namespace comdare::cache_engine::builder::artifact_transport
