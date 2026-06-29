#pragma once
// C07 IMigrationEngine — Page-/Subtree-Migration zwischen Tiers
// Termin 7 / 02_md §2 (PageRelocationTree) + P18/P19 Saikkonen

#include <cstdint>

namespace comdare::cache_engine::subsystems::migration {

enum class MigrationKind : std::uint8_t {
    LocalRelocation      = 0, // P19 Saikkonen wait-free
    GlobalReorganization = 1, // P18 Saikkonen BFS
    HotToUltra           = 2, // Tier-Migration
    ColdToStandard       = 3,
    Eviction             = 4,
};

struct MigrationResult {
    bool          success      = true;
    std::uint64_t bytes_moved  = 0;
    std::uint64_t cycles_taken = 0;
};

class IMigrationEngine {
public:
    virtual ~IMigrationEngine() = default;

    [[nodiscard]] virtual MigrationResult migrate(std::uint64_t src_addr, std::uint64_t dst_addr, std::size_t bytes,
                                                  MigrationKind kind) noexcept = 0;

    [[nodiscard]] virtual std::uint64_t total_migrations() const noexcept = 0;
};

} // namespace comdare::cache_engine::subsystems::migration
