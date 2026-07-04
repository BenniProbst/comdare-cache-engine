#pragma once
// Per-Achsen-Observer Phase B (2026-06-04) — ObservableIsa<Strategy>: ObservableAxis-Huelle um eine isa-Strategie
// (T12, axis_09). EXAKT analog axis_05_memory_layout_observable.hpp / axis_io_dispatch_observable.hpp: die Strategie
// selbst (Amd64/Aarch64/RiscV/PowerPc) traegt zwar die verhaltens-tragende static Methode simd_field_sum(), hat aber
// KEIN statistics()/snapshot_t (ist KEIN ObservableAxis). Die Mess-Mechanik gehoert daher in diese Huelle, die
// simd_field_sum static durchreicht (der seg19-Aufrufer abi_adapter T12 `Isa::simd_field_sum` bleibt heil) UND einen
// Instanz-Driver observe_simd_field_sum() ergaenzt, der beim Treiben trackt.
//
// @topic hardware @achse 09 @saeule 2 (Per-Achsen-Observer) @task Phase-B
//
// **Achsen-Semantik (treu, KEINE erfundene Konstante):** Die isa-Achse charakterisiert die SIMD-Breite der Ziel-CPU-
// ISA. simd_field_sum addiert n 32-bit-Worte lane-weise; auf x86_64 ECHTES SSE2 (4 Lanes/Iteration via _mm_add_epi32),
// sonst skalarer Fallback (1 Wort/Iteration). Die Huelle macht diese Aktivitaet observable; jeder Zaehler folgt der
// ECHTEN Op-Schleife je observe-Runde:
//   - simd_calls            : Anzahl observe_simd_field_sum-Aufrufe (SIMD-Reduktions-Runden)
//   - elements_processed    : Σ der lane-weise verarbeiteten 32-bit-Worte (n je Runde) — Durchsatz-Mass
//   - simd_iterations       : Σ der VEKTOR-Iterationen = floor(n / lane_width) je Runde (SSE2: n/4, Scalar: 0) —
//                             strategie-charakteristisch (breitere ISA → weniger Iterationen fuer dasselbe n)
//   - scalar_fallback_count : Σ der SKALAR-Rest-Iterationen = n mod lane_width (+ ganz-n bei Nicht-SIMD-Build) —
//                             ehrlicher Anteil ohne Vektorisierung
//   - last_checksum         : letztes simd_field_sum-Ergebnis (Korrektheits-/Anti-Wegopt-Anker)
// lane_width ist die statisch bekannte SIMD-Breite der ISA (Compile-Time, [[no-runtime-switch]]): x86_64 SSE2 = 4
// uint32-Lanes; sonst 1 (skalar). KEIN erfundener Wert — die Zaehler reproduzieren GENAU die Schleifenstruktur von
// simd_field_sum (4er-SSE2-Block + skalarer Rest).
//
// Gating exakt nach Praezedenz (axis_05): snapshot_t/statistics()/reset() unter COMDARE_CE_ENABLE_STATISTICS.
// Bei OFF: simd_field_sum = nackter Pass-Through (0 Footprint), ObservableAxis<...> = false → observe_all()
// faellt auf EmptyAxisSnapshot zurueck (Release-Pfad, korrekt).

#include "concepts/axis_09_isa_concept.hpp"
#include "concepts/axis_09_isa_cache_engine_permutation_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::simd {

/// ABI-taugliches Isa-Snapshot (NUR uint64 → standard_layout + trivially_copyable, Cross-ABI-POD-mappbar).
struct IsaStatistics {
    std::uint64_t simd_calls            = 0; ///< Anzahl observe_simd_field_sum-Aufrufe (SIMD-Reduktions-Runden)
    std::uint64_t elements_processed    = 0; ///< Σ der lane-weise verarbeiteten 32-bit-Worte (Σ n)
    std::uint64_t simd_iterations       = 0; ///< Σ der Vektor-Iterationen = floor(n / lane_width) (SSE2: n/4)
    std::uint64_t scalar_fallback_count = 0; ///< Σ der skalaren Rest-Iterationen (n mod lane_width, bzw. ganz-n skalar)
    std::uint64_t last_checksum         = 0; ///< letztes simd_field_sum-Ergebnis (Korrektheits-/Anti-Wegopt-Anker)

    [[nodiscard]] bool operator==(IsaStatistics const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<IsaStatistics>);
static_assert(std::is_trivially_copyable_v<IsaStatistics>);

/// ObservableAxis-Huelle: isa-Strategie + Per-Achsen-Mess-Mechanik (gegated).
/// KEIN Aggregat (private member + Methoden) → direkt als Anatomie-Member `ObservableIsa<S> m;` haltbar
/// (umgeht das `{}`-Aggregat-Init-Problem nackter Strategie-Wrapper, test_d_v42_probe2).
template <class Strategy>
    requires concepts::IsaStrategy<Strategy>
class ObservableIsa {
public:
    using strategy_type = Strategy;

    // statische Forwarding-/Instrumentierungs-Hülle (KEIN GoF-Decorator: hält keine Komponenten-Instanz, kein Voll-Interface): Strategie-Inspektion durchgereicht (composition_registry / axis_path_serialization
    // rufen C::isa::name()).
    [[nodiscard]] static constexpr bool             is_64bit() noexcept { return Strategy::is_64bit(); }
    [[nodiscard]] static constexpr std::string_view cpu_family() noexcept { return Strategy::cpu_family(); }
    [[nodiscard]] static constexpr bool supports_native_simd() noexcept { return Strategy::supports_native_simd(); }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return Strategy::name(); }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept
        requires requires { Strategy::family_name(); }
    {
        return Strategy::family_name();
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept
        requires requires { Strategy::flag_suffix(); }
    {
        return Strategy::flag_suffix();
    }
    [[nodiscard]] static constexpr std::string_view get_compiler() noexcept
        requires requires { Strategy::get_compiler(); }
    {
        return Strategy::get_compiler();
    }

    /// STATIC Pass-Through (Drop-in-Kompatibilität): die Strategie-Methode wird unveraendert durchgereicht, damit
    /// die Huelle als isa-Slot die bestehenden seg19-Aufrufer NICHT bricht (abi_adapter.hpp T12 `Isa::simd_field_sum`).
    /// Diese Variante trackt NICHT (static, kein Instanz-State).
    [[nodiscard]] static std::uint64_t simd_field_sum(unsigned char const* buf, std::size_t n) noexcept {
        return Strategy::simd_field_sum(buf, n);
    }

private:
    // Statisch bekannte SIMD-Breite der ISA in uint32-Lanes (Compile-Time, [[no-runtime-switch]]). Spiegelt die
    // simd_field_sum-Schleifenstruktur: auf x86_64-Build mit SSE2-Baseline (supports_native_simd && x86_64) = 4
    // Lanes; sonst skalar (1). Wird je nach BUILD-Host gewaehlt — exakt der Zweig, den simd_field_sum nimmt.
    [[nodiscard]] static constexpr std::uint64_t lane_width_() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
        if (Strategy::cpu_family() == std::string_view{"x86_64"} && Strategy::supports_native_simd()) return 4;
#endif
        return 1; // skalarer Pfad (Nicht-x86-Build-Host oder Nicht-x86-ISA-Strategie → kein aktiver SSE2-Zweig)
    }

public:
    /// Mess-Kopplung (der eigentliche „Driver", Instanz): treibt den ECHTEN simd_field_sum ueber die uebergebenen
    /// Bytes + trackt die strategie-charakteristische Vektor-/Skalar-Schleifenstruktur. Der Observer-Treiber
    /// (abi_adapter::fill_observer_v3 via store_observe_isa) ruft dies → die SIMD-Aktivitaet wird observable.
    [[nodiscard]] std::uint64_t observe_simd_field_sum(unsigned char const* buf, std::size_t n) noexcept {
        std::uint64_t const checksum = Strategy::simd_field_sum(buf, n);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::uint64_t const lw = lane_width_();
        ++stats_.simd_calls;
        stats_.elements_processed += static_cast<std::uint64_t>(n);
        std::uint64_t const vec_iters = (lw > 1) ? (static_cast<std::uint64_t>(n) / lw) : 0;
        std::uint64_t const scalar_rest =
            (lw > 1) ? (static_cast<std::uint64_t>(n) % lw) : static_cast<std::uint64_t>(n); // ganz skalar
        stats_.simd_iterations += vec_iters;
        stats_.scalar_fallback_count += scalar_rest;
        stats_.last_checksum = checksum;
#endif
        return checksum;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = IsaStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept { stats_ = {}; }

private:
    snapshot_t stats_{};
#endif
};

} // namespace comdare::cache_engine::simd
