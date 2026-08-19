#pragma once
// PMC-VENDOR-Registry -- die ZWEI Hardware-Komponenten der Meta-Meta-Mess-Achse "pmc".
//
// OWNER-WORTLAUT (10.08.2026, verbatim): "Das PMC ist eine Meta-Meta-Mess-Achse und wird daher IN DER CEB
// UND DEREN FINGERPRINT VERBAUT, wenn damit gebaut wird. Dabei erkennt jeder Planer auf jeder Maschine fuer
// sich, ob PMC existiert und ob daher das PMC in der CEB verbaut wird. Es entsteht hier WIEDER EINE
// PERMUTATION, weil ohne in die CEB eingebaute PMC, auch die Tier-Binaries keine PMC messen und sich daher
// der Binary Overhead eingebauter Messfuehler unterscheidet/reduziert, was die Ergebnisse wiederum NICHT
// VERGLEICHBAR macht. JEDE HARDWAREFORM EINER PMC AMD/INTEL IST ZU UNTERSCHEIDEN, ES SIND 2 VERSCHIEDENE
// HARDWARE KOMPONENTEN NICHT EIN PMC."
//
// WARUM ZWEI EINTRAEGE UND NICHT ZWEI MERKMALE: eine CEB wird auf GENAU EINEM Host gebaut. Zwei getrennte
// Merkmale ("pmc_amd", "pmc_intel") behaupteten Koexistenz -- eine CEB, die beide Hardwareformen zugleich
// traegt, gibt es nicht. Es ist deshalb EIN Merkmal "pmc" mit ZWEI WERTEN. Die Registry haelt die zwei
// Werte, jeder mit EIGENER Version: die beiden Komponenten duerfen sich unabhaengig voneinander
// weiterentwickeln (ein Event-Set-Wechsel auf der AMD-Seite bewegt die intel-Version nicht).
//
// EHRLICHE GRENZE, ausdruecklich benannt: im HEUTIGEN Bau unterscheidet die beiden Komponenten NUR dieser
// Stempel-Slot -- beide fahren dieselbe vendor-neutrale perf_event_open-Strategie
// (builder/linux_perf_pmc_source.hpp). Der Fingerprint MUSS sie trotzdem trennen, weil das Mess-INSTRUMENT
// ein anderes ist (Owner-Wort: "2 VERSCHIEDENE HARDWARE KOMPONENTEN"). Vendor-spezifische Quellen
// (Raw-Encodings, P-/E-Core je Mikroarchitektur) faechern die Versionen spaeter additiv auf.
//
// ABGRENZUNG: diese Registry ist die IDENTITAETS-Quelle (id/name/version), NICHT die Erkennung. Die
// Erkennung ist Laufzeit-Arbeit des Planers (profile_facade/planner/pmc_host_probe.hpp) und liefert
// hierher nur den gemessenen Vendor.
//
// KEIN Runtime-Switch: reine constexpr-Tabelle + Metaprogrammierungs-Iteration (Muster
// measurement_framework_registry.hpp / measurement_tooling_registry.hpp).
// header-only, C++23. GOLDEN/HOST-NEUTRAL: reine constexpr-Identitaet, keine Host-/Bau-/Mess-Semantik.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

#include <cache_engine/measurement/algo_semver.hpp> // Flag-Grammatik-Wachen (Owner-Q3 / F-10)

namespace comdare::cache_engine::measurement {

/// Die PMC-Hardwareform. ZWEI Komponenten, nicht ein PMC (Owner 10.08.2026).
enum class PmcVendor : std::uint8_t {
    Amd,   ///< AuthenticAMD (cpuid-Vendor-String), heute perf_event_open
    Intel, ///< GenuineIntel (cpuid-Vendor-String), heute perf_event_open
};

// Single-Source: eine dritte Hardwareform (z.B. ARM PMU) bricht hier compile-time, statt still 2 zu bleiben.
inline constexpr std::size_t kPmcVendorCount = 2;

struct PmcVendorInfo {
    PmcVendor        vendor;
    std::string_view id;           ///< kanonischer Stempel-/CMake-Token ("amd" / "intel")
    std::string_view name;         ///< exakt der Enum-Name (Doku/Reporting)
    std::string_view cpuid_vendor; ///< der cpuid-Vendor-String, an dem die Host-Probe erkennt
    std::string_view version;      ///< bump-bare Code-Version DIESER Komponente (Muster MeasurementFrameworkInfo)
};

/// Die EINE Registry der PMC-Vendor-Achse -- Index == PmcVendor-Wert (static_assert-gesichert).
inline constexpr std::array<PmcVendorInfo, kPmcVendorCount> kPmcVendorRegistry{{
    {PmcVendor::Amd, "amd", "Amd", "AuthenticAMD", "1.0.0.c"},
    {PmcVendor::Intel, "intel", "Intel", "GenuineIntel", "1.0.0.c"},
}};

namespace detail {
// Wachen-Batterie wie an Framework-/Tooling-/Organ-Registry, ueber die EINE Politik aus algo_semver.hpp
// (parsbar + Flag-konform). Ohne sie wuerde ein junk-Literal still als @0.0.0 stempeln.
[[nodiscard]] consteval bool pmc_vendor_versionen_wohlgeformt() {
    for (auto const& e : kPmcVendorRegistry)
        if (!ce_owned_version_is_wellformed(e.version)) return false;
    return true;
}
[[nodiscard]] consteval bool pmc_vendor_versionen_cpu_pflicht() {
    for (auto const& e : kPmcVendorRegistry)
        if (!ce_owned_version_satisfies_cpu_enforce(e.version)) return false;
    return true;
}
} // namespace detail
static_assert(detail::pmc_vendor_versionen_wohlgeformt(),
              "PMC-Vendor-Version verletzt die ce-Registry-Politik -- FUENF moegliche Gruende: "
              "(a) UNPARSBAR (und nicht der dokumentierte Sentinel \"0.0.0\") -- ein junk-Literal wuerde "
              "still als @0.0.0 stempeln; oder (b) sie traegt Flags, aber 'c' ist nicht darunter; oder "
              "(c) ein Flag-Token steht NICHT im Katalog oder nicht unter SEINER Basis "
              "(S2-Katalog-Wache, flag_grammar_catalog.hpp); oder (d) ein Knoten mit bekannter "
              "Voraussetzungs-Kette hat sein Voraussetzungs-Element nicht in der Menge (S-3b); oder "
              "(e) ein (token,eltern)-Element steht DOPPELT in der Flag-Menge (Redundanz-Wache, "
              "G-2-Semantik #17)");
#if COMDARE_VERSION_HW_FLAG_ENFORCE
static_assert(detail::pmc_vendor_versionen_cpu_pflicht(),
              "PMC-Vendor-Version ohne CPU-Flag: im CPU-only-Scope MUSS jede Version 'c' unter ihren Flags "
              "tragen (Owner-Q3 02.08.2026 / F-10 07.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf.");
#endif

namespace detail {
[[nodiscard]] consteval bool pmc_vendor_registry_is_complete() {
    for (std::size_t i = 0; i < kPmcVendorCount; ++i) {
        if (static_cast<std::size_t>(kPmcVendorRegistry[i].vendor) != i) return false;
        if (kPmcVendorRegistry[i].id.empty()) return false;
        if (kPmcVendorRegistry[i].name.empty()) return false;
        if (kPmcVendorRegistry[i].cpuid_vendor.empty()) return false;
        if (kPmcVendorRegistry[i].version.empty()) return false;
    }
    return true;
}
/// Die ids muessen PAARWEISE VERSCHIEDEN sein: sie sind der Wert des EINEN Merkmals "pmc" im CEB-Stempel.
/// Zwei gleiche ids kollabierten die beiden Hardware-Komponenten zu einer -- exakt das, was der Owner-Satz
/// "ES SIND 2 VERSCHIEDENE HARDWARE KOMPONENTEN NICHT EIN PMC" ausschliesst, und zwar STILL.
[[nodiscard]] consteval bool pmc_vendor_ids_paarweise_verschieden() {
    for (std::size_t i = 0; i < kPmcVendorCount; ++i)
        for (std::size_t j = i + 1; j < kPmcVendorCount; ++j) {
            if (kPmcVendorRegistry[i].id == kPmcVendorRegistry[j].id) return false;
            if (kPmcVendorRegistry[i].cpuid_vendor == kPmcVendorRegistry[j].cpuid_vendor) return false;
        }
    return true;
}
} // namespace detail
static_assert(kPmcVendorRegistry.size() == kPmcVendorCount,
              "kPmcVendorRegistry: Array-Groesse == kPmcVendorCount (Anzahl-Anker).");
static_assert(detail::pmc_vendor_registry_is_complete(),
              "kPmcVendorRegistry: 2 Eintraege, Index==PmcVendor, id/name/cpuid_vendor/version nie leer.");
static_assert(detail::pmc_vendor_ids_paarweise_verschieden(),
              "kPmcVendorRegistry: id UND cpuid_vendor je paarweise verschieden -- sonst kollabieren die "
              "zwei Hardware-Komponenten im Stempel zu einer (Owner 10.08.2026).");
// Namen-Anker: Drift eines id-Tokens (Umbenennung) bricht hier compile-time. BEIDE ids sind verankert --
// ein Anker auf nur einer Seite liesse die andere still driften.
static_assert(kPmcVendorRegistry[static_cast<std::size_t>(PmcVendor::Amd)].id == std::string_view{"amd"},
              "kPmcVendorRegistry: id-Token der AMD-Komponente ist {amd} (Namen-Anker).");
static_assert(kPmcVendorRegistry[static_cast<std::size_t>(PmcVendor::Intel)].id == std::string_view{"intel"},
              "kPmcVendorRegistry: id-Token der Intel-Komponente ist {intel} (Namen-Anker).");
static_assert(kPmcVendorRegistry[static_cast<std::size_t>(PmcVendor::Amd)].cpuid_vendor ==
                  std::string_view{"AuthenticAMD"},
              "kPmcVendorRegistry: cpuid-Vendor der AMD-Komponente ist {AuthenticAMD} (Erkennungs-Anker).");
static_assert(kPmcVendorRegistry[static_cast<std::size_t>(PmcVendor::Intel)].cpuid_vendor ==
                  std::string_view{"GenuineIntel"},
              "kPmcVendorRegistry: cpuid-Vendor der Intel-Komponente ist {GenuineIntel} (Erkennungs-Anker).");

/// Der STEMPEL-NAME des Merkmals -- die EINE Wahrheit seiner Schreibweise. Er steht im CEB-Selbst-Stempel
/// (builder/ceb_version_stamp.hpp) als "<name>=<vendor-id>@X.Y.Z" im Meta-Meta-Klammer-Anhang.
inline constexpr std::string_view kPmcStampName = "pmc";

/// constexpr-Lookup (Index == PmcVendor-Wert, durch static_assert garantiert).
[[nodiscard]] constexpr PmcVendorInfo const& pmc_vendor_info(PmcVendor v) noexcept {
    return kPmcVendorRegistry[static_cast<std::size_t>(v)];
}

/// Compile-time-Iteration ueber die PMC-Vendor-Achse (Metaprogrammierungs-Interface).
template <class Visitor>
constexpr void for_each_pmc_vendor(Visitor&& visitor) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (visitor(kPmcVendorRegistry[I]), ...);
    }(std::make_index_sequence<kPmcVendorCount>{});
}

/// Der GEMESSENE cpuid-Vendor-String -> Registry-Eintrag. Unbekannt => nullptr (FAIL-CLOSED: der Aufrufer
/// darf daraus KEINE Hardwareform raten). Die Zuordnung lebt nur hier, damit sie nicht an der Probe und am
/// Gegeneingang zweimal existiert und driften kann.
[[nodiscard]] constexpr PmcVendorInfo const* pmc_vendor_from_cpuid(std::string_view cpuid_vendor) noexcept {
    for (std::size_t i = 0; i < kPmcVendorCount; ++i)
        if (kPmcVendorRegistry[i].cpuid_vendor == cpuid_vendor) return &kPmcVendorRegistry[i];
    return nullptr;
}

/// Der Stempel-/CMake-Token -> Registry-Eintrag. Unbekannt => nullptr (fail-closed, s.o.).
[[nodiscard]] constexpr PmcVendorInfo const* pmc_vendor_from_id(std::string_view id) noexcept {
    for (std::size_t i = 0; i < kPmcVendorCount; ++i)
        if (kPmcVendorRegistry[i].id == id) return &kPmcVendorRegistry[i];
    return nullptr;
}

} // namespace comdare::cache_engine::measurement
