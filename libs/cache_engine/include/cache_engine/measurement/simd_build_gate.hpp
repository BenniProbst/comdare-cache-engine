// measurement/simd_build_gate.hpp -- flag-genaues Bau-Gate (Pruef-Dock-State-Pattern) (Bau Section 40.a, E4).
//
// User-Direktive (Section 37/40.a): System-Achsen geben HW-Features frei, Organ-Achsen setzen sie durch --
// "Organ-Nutzung <= Maschinen-Signatur GESCHNITTEN Organ-Sinnhaftigkeit". Dieses Gate sitzt (konzeptionell)
// an der CEB-Bau-Delegation (provision_all/CompileFn), NICHT im Tier/Planer. Es ist die aktive Durchsetzung
// der drei Section-40.a-Deklarationen: Maschinen-Signatur (machine_simd_signature.hpp), Organ-Sinnhaftigkeit
// (simd_organ_sensibility.hpp) und Flag-Katalog (simd_feature_flag.hpp).
//
// STATE PATTERN (GoF, zero-cost/constexpr realisiert -- KEINE vtable): das Pruef-Dock durchlaeuft
// Ungeprueft -> {NotApplicable | Freigegeben | Abgelehnt}. NotApplicable ist der Default, WENN ein Organ KEINE
// required-Flags deklariert -> das Gate ist dann inert und emittiert NICHTS (byte-/golden-neutral, Ist-Verhalten
// unveraendert; heute deklariert kein Organ required-Flags). Erst ein Organ, das required-Flags deklariert,
// aktiviert das Gate.
//
// FEHLERKLASSEN (an das bestehende measurement/axis_error.hpp angedockt, KEINE neue Taxonomie): eine Verletzung
// ist D1 CompilerCompilerErrorClass (Log, Experiment misst weiter -- kein Abbruch):
//   - HardwareErweiterungFehlt: required-Flag NICHT in der Maschinen-Signatur (Section 37: Organ <= Freigabe verletzt).
//   - CompileKombination:       required-Flag nicht sinnvoll (nicht in organ_meaningful) ODER von der Grob-Route
//                               (Runner-Tag) nicht zugelassen.
//   - ToolchainFehlt:           der aktive Compiler-Dialekt kennt das Flag nicht (leere Dialekt-Schreibweise).
//
// binary_id UNBERUEHRT: die effektiven Flags gehen an die CompileFn/den build_version-Suffix (Provenienz im
// H-10-Sidecar), NIE in die binary_id (golden==131072/320 neutral). Metaprog: constexpr, CRTP/Concept-frei
// (reine POD-Deskriptoren + Freifunktionen), kein std::variant, keine vtable.

#pragma once

#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/simd_feature_flag.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::measurement {

// GoF-State-Pattern-Zustaende des Pruef-Docks (zero-cost, reine Enum -- keine polymorphe State-Klasse).
enum class SimdGateState : std::uint8_t {
    Ungeprueft    = 0, // Anfangszustand vor der Dock-Pruefung
    NotApplicable = 1, // Organ deklariert keine required-Flags -> nichts zu gaten (inert, byte-neutral)
    Freigegeben   = 2, // required <= Signatur GESCHNITTEN Sinnhaftigkeit GESCHNITTEN Route -> effective_flags frei
    Abgelehnt     = 3, // Verletzung -> D1-Fehlerklasse (Log, misst weiter; kein Abbruch)
};

// Grob-Route (Runner-Routing-Vorstufe: die grobe simd-Sub-Achse). Bestimmt, welche Flag-Tiers ueberhaupt
// zulaessig sind; die Feinsignatur entscheidet DANN die einzelnen Flags.
enum class SimdRoute : std::uint8_t {
    NoExtension = 0, // generisch, keine SIMD-Flags
    Avx2        = 1, // 256-bit VEX + Begleiter + Scalar (kein AVX-512)
    Avx512      = 2, // 512-bit kumulativ (alle Tiers)
};

// Compiler-Dialekt fuer die -m-Flag-Auswahl (ToolchainFehlt greift bei leerer Dialekt-Schreibweise).
enum class SimdDialect : std::uint8_t { Gpp = 0, Clang = 1 };

[[nodiscard]] constexpr bool route_allows(SimdRoute route, SimdFlagTier tier) noexcept {
    switch (route) {
        case SimdRoute::NoExtension: return false;
        case SimdRoute::Avx2: return tier != SimdFlagTier::Avx512; // 256-bit-Route laesst AVX-512 nicht zu
        case SimdRoute::Avx512: return true;                       // 512-bit-Route: alle Tiers kumulativ
    }
    return false;
}

// Grob-simd-Id (SimdNoExtOption/Avx2/Avx512Option::simd_id()) -> Route.
[[nodiscard]] constexpr SimdRoute route_of_simd_id(std::string_view simd_id) noexcept {
    if (simd_id == "avx512") return SimdRoute::Avx512;
    if (simd_id == "avx2") return SimdRoute::Avx2;
    return SimdRoute::NoExtension; // no_extension / unbekannt -> konservativ generisch
}

[[nodiscard]] constexpr std::string_view dialect_flag_of(SimdFeatureFlag const& f, SimdDialect d) noexcept {
    return d == SimdDialect::Clang ? f.clang : f.gpp;
}

[[nodiscard]] constexpr bool span_contains(std::span<SimdFeatureFlag const> set, SimdFeatureFlag const& f) noexcept {
    for (auto const& e : set)
        if (e.cpuinfo == f.cpuinfo) return true;
    return false;
}

// Ergebnis der Dock-Pruefung: Zustand + optionale Fehlerklasse (Abgelehnt) + effektive Flags (Freigegeben).
// Bounded auf die Katalog-Groesse (effective ist stets eine Teilmenge des Katalogs).
struct SimdGateResult {
    SimdGateState                                               state = SimdGateState::Ungeprueft;
    std::optional<CompilerCompilerErrorClass>                   error{}; // nur gesetzt, wenn state==Abgelehnt
    std::array<SimdFeatureFlag, kSimdFeatureFlagCatalog.size()> effective{};
    std::size_t effective_count = 0; // NotApplicable/Abgelehnt -> 0 (byte-neutral)
};

// Das Pruef-Dock: Ungeprueft -> {NotApplicable | Freigegeben | Abgelehnt}. Rein constexpr, static-dispatch.
//   organ_required   : die harten Voraussetzungen des Organs (LEER => NotApplicable => inert/byte-neutral)
//   organ_meaningful  : die Sinnhaftigkeits-Obergrenze (aus simd_organ_sensibility.hpp)
//   machine_signature : die deklarierte Maschinen-Signatur (aus machine_simd_signature.hpp)
//   route             : die Grob-Route (Runner-Tag)
//   dialect           : aktiver Compiler-Dialekt (ToolchainFehlt-Pruefung)
[[nodiscard]] constexpr SimdGateResult pruef_dock(std::span<SimdFeatureFlag const> organ_required,
                                                  std::span<SimdFeatureFlag const> organ_meaningful,
                                                  std::span<SimdFeatureFlag const> machine_signature, SimdRoute route,
                                                  SimdDialect dialect = SimdDialect::Gpp) noexcept {
    SimdGateResult r{};

    // Ungeprueft -> NotApplicable: kein required deklariert -> nichts zu gaten (inert, byte-neutral).
    if (organ_required.empty()) {
        r.state = SimdGateState::NotApplicable;
        return r;
    }

    for (auto const& f : organ_required) {
        // Section 37 harte Zulassung: required <= Maschinen-Signatur.
        if (!span_contains(machine_signature, f)) {
            r.state = SimdGateState::Abgelehnt;
            r.error = CompilerCompilerErrorClass::HardwareErweiterungFehlt;
            return r;
        }
        // Sinnhaftigkeit: required <= organ_meaningful.
        if (!span_contains(organ_meaningful, f)) {
            r.state = SimdGateState::Abgelehnt;
            r.error = CompilerCompilerErrorClass::CompileKombination;
            return r;
        }
        // Route (Grob-Tag) muss den Tier des required-Flags zulassen.
        if (!route_allows(route, f.tier)) {
            r.state = SimdGateState::Abgelehnt;
            r.error = CompilerCompilerErrorClass::CompileKombination;
            return r;
        }
        // Toolchain: der aktive Dialekt muss das Flag kennen.
        if (dialect_flag_of(f, dialect).empty()) {
            r.state = SimdGateState::Abgelehnt;
            r.error = CompilerCompilerErrorClass::ToolchainFehlt;
            return r;
        }
    }

    // Freigegeben: effektive Flags = Signatur GESCHNITTEN Sinnhaftigkeit GESCHNITTEN Route (kumulativ, nicht nur required).
    r.state = SimdGateState::Freigegeben;
    for (auto const& f : machine_signature)
        if (span_contains(organ_meaningful, f) && route_allows(route, f.tier) && !dialect_flag_of(f, dialect).empty())
            r.effective[r.effective_count++] = f;
    return r;
}

// CompileFn-Emission (runtime, an der provision_all/CompileFn-Naht): die einzelnen -m-Flags des Ergebnisses.
// Freigegeben -> je effektives Flag ein -m<flag>; NotApplicable/Abgelehnt -> LEER (CompileFn unveraendert).
[[nodiscard]] inline std::vector<std::string> effective_march_flags(SimdGateResult const& r,
                                                                    SimdDialect           dialect = SimdDialect::Gpp) {
    std::vector<std::string> out;
    if (r.state != SimdGateState::Freigegeben) return out; // byte-neutral, wenn nicht freigegeben
    out.reserve(r.effective_count);
    for (std::size_t i = 0; i < r.effective_count; ++i) out.emplace_back(dialect_flag_of(r.effective[i], dialect));
    return out;
}

// -- CompileFn-Naht-Konsumtion (an provision_all/CompileFn, Bau Section 40.a-E4) -------------------
// Route aus dem bereits aufgeloesten Grob-march-Flag (-mavx2/-mavx512f/leer) -- die CompileFn-Naht kennt
// den march_flag-String, nicht die simd-id.
[[nodiscard]] constexpr SimdRoute route_of_march_flag(std::string_view march_flag) noexcept {
    if (march_flag.find("avx512") != std::string_view::npos) return SimdRoute::Avx512;
    if (march_flag.find("avx2") != std::string_view::npos) return SimdRoute::Avx2;
    return SimdRoute::NoExtension;
}

// Die AKTIVEN Organ-Anforderungen des laufenden Baus. HEUTE LEER: kein Organ deklariert required-Flags ->
// das Gate ist NotApplicable/inert -> die CompileFn-Naht haengt KEINE Zusatz-Flags an (byte-/golden-neutral,
// Ist-Verhalten identisch). Diese drei Hooks sind der Single-Source-Einstieg, den die per-Organ-/per-Binary-
// Aktivierung spaeter befuellt (organ_required aus den Organ-Wrappern, machine_signature aus der Host-Wahl).
[[nodiscard]] inline std::span<SimdFeatureFlag const> active_organ_required() noexcept { return {}; }
[[nodiscard]] inline std::span<SimdFeatureFlag const> active_organ_meaningful() noexcept { return {}; }
[[nodiscard]] inline std::span<SimdFeatureFlag const> active_machine_signature() noexcept { return {}; }

// Die einzelnen -m-Flags, die das Gate an der CompileFn-Naht fuer die gegebene Grob-Route beisteuert.
// Freigegeben -> die effektiven Flags; NotApplicable/Abgelehnt -> LEER (CompileFn byte-identisch). Heute stets leer.
[[nodiscard]] inline std::vector<std::string> gate_extra_march_flags_for_build(SimdRoute   route,
                                                                               SimdDialect dialect = SimdDialect::Gpp) {
    SimdGateResult const r =
        pruef_dock(active_organ_required(), active_organ_meaningful(), active_machine_signature(), route, dialect);
    return effective_march_flags(r, dialect);
}

// -- Wohlgeformtheit + State-Pattern-Uebergaenge (alles compile-time) -----------------------------
static_assert(route_allows(SimdRoute::Avx512, SimdFlagTier::Avx512));
static_assert(!route_allows(SimdRoute::Avx2, SimdFlagTier::Avx512)); // 256-bit-Route sperrt AVX-512
static_assert(route_allows(SimdRoute::Avx2, SimdFlagTier::Avx256) &&
              route_allows(SimdRoute::Avx2, SimdFlagTier::Companion));
static_assert(!route_allows(SimdRoute::NoExtension, SimdFlagTier::Avx256));
static_assert(route_of_simd_id("avx512") == SimdRoute::Avx512 &&
              route_of_simd_id("no_extension") == SimdRoute::NoExtension);
static_assert(route_of_march_flag("-mavx512f") == SimdRoute::Avx512 &&
              route_of_march_flag("-mavx2") == SimdRoute::Avx2);
static_assert(route_of_march_flag("") == SimdRoute::NoExtension);
// leere required-Menge -> NotApplicable, KEINE effektiven Flags (byte-/golden-neutral, Ist-Verhalten):
static_assert(pruef_dock({}, {}, {}, SimdRoute::Avx512).state == SimdGateState::NotApplicable);
static_assert(pruef_dock({}, {}, {}, SimdRoute::Avx512).effective_count == 0);

} // namespace comdare::cache_engine::measurement
