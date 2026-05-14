#pragma once
// C02 IPinningEngine — Thread/Daten-Pinning (REV 3 K3.2 generisch)
// Termin 7 / 13_md §4 + 10_korrektur §K3.2 (generische Heuristiken statt CPU-Spez.)

#include <cstdint>
#include <thread>

namespace comdare::cache_engine::subsystems::pinning {

enum class PinningTarget : std::uint8_t {
    AnyCore           = 0,
    LargestL3CcdCore  = 1,   // generisch fuer X3D — REV 3 K3.2
    HighIpcCore       = 2,   // generisch fuer P-Cores — REV 3 K3.2
    NumaLocalCore     = 3,
    SpecializedCore   = 4,   // generisch fuer Tiles/PEs — REV 3 K3.2
};

class IPinningEngine {
public:
    virtual ~IPinningEngine() = default;

    // Empfehlung fuer einen Thread auf Basis Live-Modell
    [[nodiscard]] virtual PinningTarget recommend(std::thread::id tid) const noexcept = 0;

    // Pin durchfuehren (kann no-op sein in Simulationsmodi)
    virtual void apply_pin(std::thread::id tid, PinningTarget target) noexcept = 0;

    [[nodiscard]] virtual std::uint64_t total_pins_applied() const noexcept = 0;
};

}  // namespace comdare::cache_engine::subsystems::pinning
