#pragma once
// traeger/ceb/parameter_filter_registry.hpp -- M6: die PARAMETER-FILTER-REGISTRY (#88/P-B),
// W2-D-KATALOG-SKELETT (#91-Vollzug, 2026-08-21; design91-v2 Abschnitt 3/M6 + Slot W2-D).
//
// ORT (Design #29 Par. 4.3): S-9-NEUBAU unter traeger/ceb -- die Registry ist Substanz des
// AUSWERTE-RINGS (R3) der CEB-Modul-Steuerung (design91-v2 Ring-Karte 2.1): sie liefert den
// Gewichtungs-DEFAULT an M7/curve_fit und das XML-waehlbare Release-Kriterium an den Planer
// (Kriterium greift NACH allen 3 Komponentenstufen, KON110-Runde). Eigene Include-Wurzel
// <traeger/ceb/...> (R5) -- die PUBLIC-Wurzel des Monolithen wird NICHT erweitert.
//
// SKELETT, KEIN VOLLAUSBAU (design91-v2, M6 = [RT] fuer den Inhalt):
//   * HIER: die Katalog-Form + der EINE mechanisch entschiedene Eintrag -- Filter 1 =
//     "kuerzeste Gesamtzeit", impact-sortiert an Rang 1 (Skizzen-/Owner-Stand der M6-Zeile).
//   * NICHT HIER (nach Trigger, #88): die Katalog-FUELLUNG aus Deep-Research alt (09.07.) +
//     NEU (gebuendelt mit dem SIMD-Research, Ledger-KON111-02d), die Gewichtungs-Zahlen
//     (constexpr-Default + XML-Override, Default-Doktrin Korb B-15) und die Impact-Herleitung.
//   * NICHT HIER (M12-Sammelzug W2-B, #18/#48/#57(6)): das XML-KRITERIUM-TOKEN der Grammatik.
//
// ABGRENZUNG (Falsch-Freund-Wache, am Objekt gemessen im #91-Design): die 4 grep-Treffer
// "filter_registry|parameter_filter" im Monolithen sind die ORGAN-Such-Filter-ACHSE
// (topics/filter/..., axis_filter_registry, surf_axis_allocator, all_axes_umbrella) -- eine
// ANDERE Sache (Tier-Fach), NICHT diese Auswerte-Registry. Owner hat T-9 als Quelle VERNEINT
// (parallele Registry). VORBILD der Form ist heuristik/axis_optimization_catalog.hpp
// (kAxisObjectives-Katalogstil) -- NUR Vorbild, heuristik/ bleibt unberuehrt.
//
// SELBSTCHECK: reine constexpr-Identitaet, keine Host-/Bau-/Mess-Semantik, kein Stempel-/golden-
// Byte; header-only; ASCII-only; keine Beruehrung von axes/, topics/, heuristik/; einzige
// Includes = Standard-Bibliothek (Stufen-Vertrag KON43-01 unberuehrt).

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::traeger::ceb {

// Single-Source: eine 2. Katalog-Zeile ERWEITERT diese Zahl im selben Zug (#88-Fuellung) --
// still bleiben kann sie nicht (Anzahl-Anker unten).
inline constexpr std::size_t kParameterFilterCount = 1;

/// EIN Parameter-Filter des Auswerte-Katalogs: stabiler Token, Katalog-Formulierung und sein
/// Rang in der Impact-Sortierung (1 = groesster Hebel; die Sortierung ist Teil des Vertrags).
// Default-Initialisierer nach dem Muster des heuristik-Bestands (axis_optimization_catalog.hpp):
// jedes Feld auch bei kuenftiger Teil-Initialisierung bestimmt; cppcheck-gruen.
struct ParameterFilterInfo {
    std::string_view id          = {}; ///< stabiler ASCII-Token (XML-/Legenden-Seite folgt M12)
    std::string_view zielgroesse = {}; ///< die Katalog-Formulierung (Rueckverfolgung)
    std::size_t      impact_rang = 0;  ///< Rang in der Impact-Sortierung (1 = zuerst)
};

/// DER Katalog (impact-sortiert; Reihenfolge == Rang-Reihenfolge, static_assert-gesichert).
inline constexpr std::array<ParameterFilterInfo, kParameterFilterCount> kParameterFilterRegistry{{
    // Filter 1 (M6-SOLL verbatim): "kuerzeste Gesamtzeit, impact-sortiert" -- der erste und
    // einzige mechanisch entschiedene Eintrag des Skeletts.
    {"kuerzeste_gesamtzeit", "kuerzeste Gesamtzeit des Experiments (minimieren)", 1},
}};

/// constexpr-Lookup ueber den Token; nullptr fuer unbekannte Filter (Gegeneingang testbar) --
/// ein unbekanntes Kriterium darf NIE still auf einen Default fallen.
[[nodiscard]] constexpr ParameterFilterInfo const* parameter_filter_of(std::string_view id) noexcept {
    for (std::size_t i = 0; i < kParameterFilterCount; ++i)
        if (kParameterFilterRegistry[i].id == id) return &kParameterFilterRegistry[i];
    return nullptr;
}

namespace detail {
[[nodiscard]] consteval bool parameter_filter_registry_ist_konsistent() {
    for (std::size_t i = 0; i < kParameterFilterCount; ++i) {
        ParameterFilterInfo const& z = kParameterFilterRegistry[i];
        if (z.id.empty() || z.zielgroesse.empty()) return false;
        if (z.impact_rang != i + 1) return false; // impact-sortiert: Rang == Index + 1, lueckenlos
        for (std::size_t j = 0; j < i; ++j)
            if (kParameterFilterRegistry[j].id == z.id) return false; // Token eindeutig
    }
    return true;
}
} // namespace detail

static_assert(kParameterFilterRegistry.size() == kParameterFilterCount,
              "kParameterFilterRegistry: Array-Groesse == kParameterFilterCount (Anzahl-Anker).");
static_assert(detail::parameter_filter_registry_ist_konsistent(),
              "kParameterFilterRegistry: id/zielgroesse nie leer, impact-sortiert (Rang == Index+1), Token eindeutig.");
// Namen-Anker (Pin-Doktrin): Filter 1 ist die kuerzeste Gesamtzeit -- Drift bricht compile-time.
static_assert(kParameterFilterRegistry[0].id == std::string_view{"kuerzeste_gesamtzeit"} &&
                  kParameterFilterRegistry[0].impact_rang == 1,
              "M6-SOLL: Filter 1 = kuerzeste Gesamtzeit an Impact-Rang 1.");

} // namespace comdare::traeger::ceb
