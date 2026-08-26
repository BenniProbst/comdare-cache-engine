// measurement/organ_meta_meta_requirement.hpp -- required-/meaningful-Deklaration der ORGAN-META-META-Welt
// (E-10/#38a2 Gate-Haelfte + #86 ORG-19; Designplan 20260825-DESIGNPLAN-E10-38a2-ORG19 Schritt 2a, 26.08.2026).
//
// GEGENSTAND: der DEKLARATIONS-ORT fuer die harten required- und die sinnhaften meaningful-Flags einer
// Organ-Meta-Meta-Achse, per Binary erreichbar ueber das Traeger-Paar (carrier_axis, carrier_value).
// 18+1-SCHNITT (H-23 C.1): die 18 Kompositions-Zeilen leben in simd_organ_requirement.hpp /
// simd_organ_sensibility.hpp, die +1 Meta-Meta-Zeile lebt HIER -- kCompositionAxisNames bleibt 18,
// ORG-19 ist KEINE 19. Zeile der Kompositions-Tabellen und permutiert NIE die binary_id.
//
// EHRLICH LEER (KN-1 Lesart A): required(disk_io) = LEER, meaningful(disk_io) = LEER. KON16-02:
// "required = hartes Funktions-MINIMUM, kein Beschleunigungs-Wunsch" -- kein SIMD-Flag ist
// Funktionsminimum fuer Festplatten-IO; KON59-01: konkrete required-FLAG-WERTE sind nirgends
// deklariert (Bau-Ermessen, KEINE neue Owner-Runde); Regel "nie raten". Ein spaeterer echter Wert
// (z. B. ein SIMD-beschleunigter Serialisierungs-Pfad des IO-Glieds) traegt seinen eigenen Minor.
// Der Beweis, dass eine NICHT-leere Zeile PER BINARY wirkt, faehrt als consteval-Probe (D-4) ueber
// das detail::kProbeMetaMetaRequirement-Register unten (Aggregations-Beweis am Kern in
// simd_organ_requirement.hpp; Muster MetaMetaVersionProbe, hardware_meta_meta_axis.hpp).
//
// VERORTUNG/LAYER: measurement/ (bei Katalog/Sinnhaftigkeit/Signatur, wie simd_organ_requirement.hpp)
// -- KEIN organ_axes-Include (Schranke: organ_axes/ darf measurement/ nicht ruecksehen, und dieser
// Header wird von simd_organ_requirement.hpp eingebunden). Die Strings DOPPELN darum die Konstanten
// aus organ_axes/organ_meta_meta/axis_disk_io_organ_meta_meta.hpp (1a); die Kopplung haelt der PIN in
// tests/unit/test_org19_meta_meta_requirement.cpp (Praezedenz anatomy_version_stamp.hpp: "Wer ORG-18
// aendert, aendert BEIDE Orte"). INCLUDE-RICHTUNG (deklarierte Plan-Abweichung am Objekt): die
// leere Menge heisst hier kMetaMetaRequiredNone statt kRequiredNone -- kRequiredNone lebt in
// simd_organ_requirement.hpp, und ein Rueck-Include von dort hierher schuefe einen Zyklus (der
// 2b-Aggregations-Kern bindet DIESEN Header ein). Gleiche Rolle, eigener Name, im Test gepinnt leer.
//
// Metaprog: POD-Deskriptoren + constexpr-Freifunktionen, keine vtable, kein std::variant. CT-Wachen
// als BENANNTE constexpr-Funktionen (cppcheck-2.21-Form-Hinweis, kein IIFE-Lambda in static_assert).

#pragma once

#include <cache_engine/measurement/simd_feature_flag.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace comdare::cache_engine::measurement {

// Eine Zeile des Meta-Meta-Registers: Meta-Meta-Label + Traeger-Paar (Achse, Wert) -> Flag-Mengen.
// Das Traeger-Paar macht die Zeile PER BINARY erreichbar: aus der Organ-Signatur einer Tier-Binary
// ((achse,wert)-Paare) loest meta_meta_for_pair auf das Label auf -- nur Binaries MIT Traeger-Paar
// erben die Mengen (die Flotte all_axes_golden ist 100 % persistence_memory_only -> erbt NICHTS).
struct OrganMetaMetaRequirement {
    std::string_view                 meta_meta_label; // doppelt do_axis_label() der 1a-Achse (Test-PIN)
    std::string_view                 carrier_axis;    // doppelt kCarrierAxis (D-1 Single-Source)
    std::string_view                 carrier_value;   // doppelt kCarrierValues[0] (== Strategie-name())
    std::span<SimdFeatureFlag const> required;        // LEER = kein hartes Funktions-Minimum deklariert
    std::span<SimdFeatureFlag const> meaningful;      // LEER = keine sinnhafte Zuordnung deklariert
};

// Default-Leermenge (Rolle von kRequiredNone; eigener Name wegen der Include-Richtung, s. Kopf).
inline constexpr std::array<SimdFeatureFlag, 0> kMetaMetaRequiredNone{};

// Zentrale Meta-Meta-Deklaration: heute GENAU EINE Zeile -- ORG-19-IO (disk_io), ehrlich LEER.
inline constexpr std::array<OrganMetaMetaRequirement, 1> kOrganMetaMetaRequirement{{
    {"disk_io", "persistence_target", "persistence_disk_writeback", kMetaMetaRequiredNone, {}},
}};

// -- Register-parametrisierte Lookups (constexpr-Kern-Bausteine; das Produktions-Register ist EIN --
// -- moeglicher Parameter, das 2c-Probe-Register der andere -- EIN Rechenweg fuer beide, D-4) ------
[[nodiscard]] constexpr std::string_view meta_meta_for_pair_in(std::span<OrganMetaMetaRequirement const> reg,
                                                               std::string_view axis, std::string_view value) noexcept {
    for (auto const& e : reg)
        if (e.carrier_axis == axis && e.carrier_value == value) return e.meta_meta_label;
    return {};
}

[[nodiscard]] constexpr std::span<SimdFeatureFlag const>
required_of_meta_meta_in(std::span<OrganMetaMetaRequirement const> reg, std::string_view label) noexcept {
    for (auto const& e : reg)
        if (e.meta_meta_label == label) return e.required;
    return {};
}

[[nodiscard]] constexpr std::span<SimdFeatureFlag const>
meaningful_of_meta_meta_in(std::span<OrganMetaMetaRequirement const> reg, std::string_view label) noexcept {
    for (auto const& e : reg)
        if (e.meta_meta_label == label) return e.meaningful;
    return {};
}

// -- Produktions-API (ueber kOrganMetaMetaRequirement; leer, wenn nicht deklariert -- kein Wurf) ---
[[nodiscard]] constexpr std::string_view meta_meta_for_pair(std::string_view axis, std::string_view value) noexcept {
    return meta_meta_for_pair_in(kOrganMetaMetaRequirement, axis, value);
}
[[nodiscard]] constexpr std::span<SimdFeatureFlag const> required_of_meta_meta(std::string_view label) noexcept {
    return required_of_meta_meta_in(kOrganMetaMetaRequirement, label);
}
[[nodiscard]] constexpr std::span<SimdFeatureFlag const> meaningful_of_meta_meta(std::string_view label) noexcept {
    return meaningful_of_meta_meta_in(kOrganMetaMetaRequirement, label);
}

// Deklariert IRGENDEINE Meta-Meta harte required-Flags? (Aktivierungs-Indikator; heute false ->
// der Gate-Beitrag bleibt byte-neutral. Das Kompositions-Gegenstueck any_organ_declares_required()
// bleibt in simd_organ_requirement.hpp -- ZWEI Nenner, EIN Schnitt: 18 + 1.)
[[nodiscard]] constexpr bool any_meta_meta_declares_required() noexcept {
    for (auto const& e : kOrganMetaMetaRequirement)
        if (!e.required.empty()) return true;
    return false;
}

// -- CT-Wachen (benannte Funktionen; gelten fuer Produktions- UND Probe-Register) ------------------
// Vokabular-Kopplung: jede deklarierte Flag-Menge referenziert nur Katalog-bekannte Flags. Zusammen
// mit catalog_ids_unique() (23 distinkte cpuinfo-Ids) deckelt das jede cpuinfo-deduplizierte
// Vereinigung auf kSimdFeatureFlagCatalog.size() -- der Kapazitaets-Beweis des Aggregations-Kerns.
[[nodiscard]] constexpr bool meta_meta_requirement_within_catalog(std::span<OrganMetaMetaRequirement const> reg) //
    noexcept {
    for (auto const& e : reg) {
        for (auto const& f : e.required)
            if (!is_known_simd_flag(f.cpuinfo)) return false;
        for (auto const& f : e.meaningful)
            if (!is_known_simd_flag(f.cpuinfo)) return false;
    }
    return true;
}
// Vollstaendige Felder: leere Labels/Traeger machten meta_meta_for_pair-Leer-Rueckgaben mehrdeutig.
[[nodiscard]] constexpr bool meta_meta_requirement_felder_nonempty(std::span<OrganMetaMetaRequirement const> reg) //
    noexcept {
    for (auto const& e : reg)
        if (e.meta_meta_label.empty() || e.carrier_axis.empty() || e.carrier_value.empty()) return false;
    return true;
}
// Eindeutigkeit: doppelte Labels oder doppelte Traeger-Paare machten die Lookups still first-wins.
[[nodiscard]] constexpr bool meta_meta_requirement_eindeutig(std::span<OrganMetaMetaRequirement const> reg) //
    noexcept {
    for (std::size_t i = 0; i < reg.size(); ++i)
        for (std::size_t j = i + 1; j < reg.size(); ++j) {
            if (reg[i].meta_meta_label == reg[j].meta_meta_label) return false;
            if (reg[i].carrier_axis == reg[j].carrier_axis && reg[i].carrier_value == reg[j].carrier_value)
                return false;
        }
    return true;
}

static_assert(kOrganMetaMetaRequirement.size() == 1,
              "18 Komposition (simd_organ_requirement.hpp) + 1 Meta-Meta (HIER) -- der 18+1-Schnitt "
              "nach H-23 C.1; kCompositionAxisNames bleibt 18");
static_assert(!any_meta_meta_declares_required()); // heutiger Stand: LEER -> Gate-Beitrag byte-neutral
static_assert(required_of_meta_meta("disk_io").empty());
static_assert(meaningful_of_meta_meta("disk_io").empty());
static_assert(required_of_meta_meta("unbekannte_meta_meta").empty()); // unbekannt bleibt leer (kein Wurf)
static_assert(meta_meta_for_pair("persistence_target", "persistence_disk_writeback") == "disk_io");
static_assert(meta_meta_for_pair("persistence_target", "persistence_memory_only").empty());
static_assert(meta_meta_requirement_within_catalog(kOrganMetaMetaRequirement));
static_assert(meta_meta_requirement_felder_nonempty(kOrganMetaMetaRequirement));
static_assert(meta_meta_requirement_eindeutig(kOrganMetaMetaRequirement));

namespace detail {

// 2c-PROBE-REGISTER (NUR consteval-Beweis, KEIN Produktions-Wert; Muster MetaMetaVersionProbe,
// hardware_meta_meta_axis.hpp): eine NICHT-leere Zeile an einem Traeger-Paar. Der Beweis, dass sie
// PER BINARY wirkt (Achsen MIT Traeger -> {avx2}, OHNE -> leer), steht als static_assert am
// Aggregations-Kern (simd_organ_requirement.hpp, probe_per_binary_aggregation_ist_exakt) und wird
// vom C-3a-UMSCHLAG (Schritt 3, simd_build_gate.hpp) referenziert. Das Label "probe_meta_meta"
// existiert im Produktions-Register NICHT (Test pinnt die Trennung).
inline constexpr std::array<SimdFeatureFlag, 1> kProbeMetaMetaFlags{kAvx2};

inline constexpr std::array<OrganMetaMetaRequirement, 1> kProbeMetaMetaRequirement{{
    {"probe_meta_meta", "persistence_target", "persistence_disk_writeback", kProbeMetaMetaFlags, kProbeMetaMetaFlags},
}};

static_assert(meta_meta_requirement_within_catalog(kProbeMetaMetaRequirement));
static_assert(meta_meta_requirement_felder_nonempty(kProbeMetaMetaRequirement));
static_assert(meta_meta_requirement_eindeutig(kProbeMetaMetaRequirement));
static_assert(required_of_meta_meta_in(kProbeMetaMetaRequirement, "probe_meta_meta").size() == 1,
              "die Probe-Zeile ist bewusst NICHT leer -- nur so kann sie die per-Binary-Wirkung beweisen");
static_assert(required_of_meta_meta("probe_meta_meta").empty(),
              "Produktions-Lookups kennen das Probe-Label NICHT (2c: kein Produktions-Wert)");

} // namespace detail

} // namespace comdare::cache_engine::measurement
