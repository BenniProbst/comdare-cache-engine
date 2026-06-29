#pragma once
// KF-4 (2026-06-02) — AlgorithmResourceControl: host-seitige Steuer-Ebene am Prüf-Dock.
//
// Gegenstück zu anatomy::IResourceControllableTier (resource_controllable_tier.hpp). Der CacheEngineBuilder
// nutzt diesen Controller, um die algorithmus-internen Laufzeit-Properties (Thread-Anzahl, Prefetch-Distanz,
// Pool-Budget, Batch-Größe, Inline-Schwelle) eines geladenen Tier-Moduls zu setzen — AUCH bei abgeschalteter
// Messung. Die Werte werden auf die vom Tier gemeldeten Caps UND die System-Umgebungs-/Ressourcenlimits
// geklammert (Faustregel §7: dynamischer Durchlauf "in den Grenzen der Systemumgebungsvariablen und
// Ressourcenlimits"). Der eigentliche Durchlauf über die einstellbaren Möglichkeiten ist KF-7.
//
// @doku docs/sessions/20260602-cacheline-konfigurator-design-und-hw-recherche.md §7-A

#include "../anatomy/resource_controllable_tier.hpp"

#include <cstdint>

namespace comdare::cache_engine::builder {

class AlgorithmResourceControl {
public:
    /// Gewünschte Werte (z.B. aus runtime_dynamic des Profils; 0 = Feld nicht setzen).
    anatomy::ComdareResourceControlV1 desired{};
    /// Zusätzliche System-Ressourcenlimits als Obergrenze (0 = unbegrenzt), z.B. verfügbare Kerne.
    anatomy::ComdareResourceControlV1 env_limits{};

    /// Wendet `desired` auf das Tier an, geklammert auf min(tier-caps, env_limits). Liefert die Zahl der
    /// Achsen, die den Wert real angenommen haben. tier == nullptr → 0 (sauberes Degradieren).
    [[nodiscard]] std::uint64_t apply_to(anatomy::IResourceControllableTier* tier) const noexcept {
        if (tier == nullptr) return 0;
        anatomy::ComdareResourceControlV1 caps{};
        tier->tier_query_resource_caps(&caps);
        anatomy::ComdareResourceControlV1 eff = clamp(desired, caps, env_limits);
        return tier->tier_apply_resource_control(&eff);
    }

    /// Klammert jedes Feld auf min(cap, env). Semantik: want==0 → 0 (nicht setzen); cap==0 → Achse nicht
    /// steuerbar → 0; env==0 → keine zusätzliche Env-Grenze.
    [[nodiscard]] static anatomy::ComdareResourceControlV1
    clamp(anatomy::ComdareResourceControlV1 const& want, anatomy::ComdareResourceControlV1 const& caps,
          anatomy::ComdareResourceControlV1 const& env) noexcept {
        auto cl = [](std::uint64_t w, std::uint64_t cap, std::uint64_t e) -> std::uint64_t {
            if (w == 0) return 0;                      // nicht setzen
            std::uint64_t hi = cap;                    // Tier-Cap
            if (e != 0 && (hi == 0 || e < hi)) hi = e; // Env-Grenze ggf. strenger
            if (hi == 0) return 0;                     // Achse nicht steuerbar
            return (w > hi) ? hi : w;                  // auf Obergrenze klammern
        };
        anatomy::ComdareResourceControlV1 out{};
        out.thread_count      = cl(want.thread_count, caps.thread_count, env.thread_count);
        out.prefetch_distance = cl(want.prefetch_distance, caps.prefetch_distance, env.prefetch_distance);
        out.pool_budget_bytes = cl(want.pool_budget_bytes, caps.pool_budget_bytes, env.pool_budget_bytes);
        out.batch_size        = cl(want.batch_size, caps.batch_size, env.batch_size);
        out.inline_threshold_bytes =
            cl(want.inline_threshold_bytes, caps.inline_threshold_bytes, env.inline_threshold_bytes);
        return out;
    }
};

} // namespace comdare::cache_engine::builder
